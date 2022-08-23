#include "musicbrainz/tagfetcher.h"

#include <QFuture>
#include <QtConcurrentRun>

#include "moc_tagfetcher.cpp"
#include "musicbrainz/chromaprinter.h"
#include "track/track.h"
#include "util/thread_affinity.h"

namespace {

// Long timeout to cope with occasional server-side unresponsiveness
constexpr int kAcoustIdTimeoutMillis = 60000; // msec

// Long timeout to cope with occasional server-side unresponsiveness
constexpr int kMusicBrainzTimeoutMillis = 60000; // msec

// Long timeout to cope with occasional server-side unresponsiveness
constexpr int kCoverArtArchiveLinksTimeoutMilis = 60000; // msec

// Long timeout to cope with occasional server-side unresponsiveness
constexpr int kCoverArtArchiveThumbnailsTimeoutMilis = 60000; // msec

// Long timeout to cope with occasional server-side unresponsiveness
constexpr int kCoverArtArchiveImageTimeoutMilis = 60000; // msec

} // anonymous namespace

TagFetcher::TagFetcher(QObject* parent)
        : QObject(parent),
          m_fingerprintWatcher(this) {
}

void TagFetcher::startFetch(
        TrackPointer pTrack) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    cancel();

    m_pTrack = pTrack;

    emit fetchProgress(tr("Fingerprinting track"));
    const auto fingerprintTask = QtConcurrent::run([pTrack] {
        return ChromaPrinter().getFingerprint(pTrack);
    });
    m_fingerprintWatcher.setFuture(fingerprintTask);
    DEBUG_ASSERT(!m_pAcoustIdTask);
    connect(
            &m_fingerprintWatcher,
            &QFutureWatcher<QString>::finished,
            this,
            &TagFetcher::slotFingerprintReady);
}

void TagFetcher::cancel() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    m_pTrack.reset();
    m_fingerprintWatcher.disconnect(this);
    m_fingerprintWatcher.cancel();
    if (m_pAcoustIdTask) {
        m_pAcoustIdTask->invokeAbort();
        DEBUG_ASSERT(!m_pAcoustIdTask);
    }
    if (m_pMusicBrainzTask) {
        m_pMusicBrainzTask->invokeAbort();
        DEBUG_ASSERT(!m_pMusicBrainzTask);
    }
    if (m_pCoverArtArchiveLinksTask) {
        m_pCoverArtArchiveLinksTask->invokeAbort();
        DEBUG_ASSERT(!m_pCoverArtArchiveLinksTask);
    }
    if (m_pCoverArtArchiveThumbnailsTask) {
        m_pCoverArtArchiveThumbnailsTask->invokeAbort();
        DEBUG_ASSERT(!m_pCoverArtArchiveThumbnailsTask);
    }
}

void TagFetcher::slotFingerprintReady() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!m_pTrack ||
            !m_fingerprintWatcher.isFinished()) {
        return;
    }

    DEBUG_ASSERT(m_fingerprintWatcher.isFinished());
    const QString fingerprint = m_fingerprintWatcher.result();
    if (fingerprint.isEmpty()) {
        emit resultAvailable(
                m_pTrack,
                QList<mixxx::musicbrainz::TrackRelease>());
        return;
    }

    emit fetchProgress(tr("Identifying track through Acoustid"));
    DEBUG_ASSERT(!m_pAcoustIdTask);
    m_pAcoustIdTask = make_parented<mixxx::AcoustIdLookupTask>(
            &m_network,
            fingerprint,
            m_pTrack->getDurationSecondsInt(),
            this);
    connect(m_pAcoustIdTask,
            &mixxx::AcoustIdLookupTask::succeeded,
            this,
            &TagFetcher::slotAcoustIdTaskSucceeded);
    connect(m_pAcoustIdTask,
            &mixxx::AcoustIdLookupTask::failed,
            this,
            &TagFetcher::slotAcoustIdTaskFailed);
    connect(m_pAcoustIdTask,
            &mixxx::AcoustIdLookupTask::aborted,
            this,
            &TagFetcher::slotAcoustIdTaskAborted);
    connect(m_pAcoustIdTask,
            &mixxx::AcoustIdLookupTask::networkError,
            this,
            &TagFetcher::slotAcoustIdTaskNetworkError);
    m_pAcoustIdTask->invokeStart(
            kAcoustIdTimeoutMillis);
}

void TagFetcher::slotAcoustIdTaskSucceeded(
        QList<QUuid> recordingIds) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    auto* const pAcoustIdTask = m_pAcoustIdTask.get();
    VERIFY_OR_DEBUG_ASSERT(pAcoustIdTask ==
            qobject_cast<mixxx::AcoustIdLookupTask*>(sender())) {
        return;
    }
    m_pAcoustIdTask = nullptr;
    const auto taskDeleter = mixxx::ScopedDeleteLater(pAcoustIdTask);
    pAcoustIdTask->disconnect(this);

    if (recordingIds.isEmpty()) {
        auto pTrack = std::move(m_pTrack);
        cancel();

        emit resultAvailable(
                std::move(pTrack),
                QList<mixxx::musicbrainz::TrackRelease>());
        return;
    }

    emit fetchProgress(tr("Retrieving metadata from MusicBrainz"));
    emit numberOfRecordingsToFetch(recordingIds.size());

    DEBUG_ASSERT(!m_pMusicBrainzTask);
    m_pMusicBrainzTask = make_parented<mixxx::MusicBrainzRecordingsTask>(
            &m_network,
            std::move(recordingIds),
            this);
    connect(m_pMusicBrainzTask,
            &mixxx::MusicBrainzRecordingsTask::succeeded,
            this,
            &TagFetcher::slotMusicBrainzTaskSucceeded);
    connect(m_pMusicBrainzTask,
            &mixxx::MusicBrainzRecordingsTask::failed,
            this,
            &TagFetcher::slotMusicBrainzTaskFailed);
    connect(m_pMusicBrainzTask,
            &mixxx::MusicBrainzRecordingsTask::aborted,
            this,
            &TagFetcher::slotMusicBrainzTaskAborted);
    connect(m_pMusicBrainzTask,
            &mixxx::MusicBrainzRecordingsTask::networkError,
            this,
            &TagFetcher::slotMusicBrainzTaskNetworkError);
    connect(m_pMusicBrainzTask,
            &mixxx::MusicBrainzRecordingsTask::currentRecordingFetched,
            this,
            &TagFetcher::currentRecordingFetched);
    m_pMusicBrainzTask->invokeStart(
            kMusicBrainzTimeoutMillis);
}

bool TagFetcher::onAcoustIdTaskTerminated() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    auto* const pAcoustIdTask = m_pAcoustIdTask.get();
    DEBUG_ASSERT(sender());
    VERIFY_OR_DEBUG_ASSERT(pAcoustIdTask ==
            qobject_cast<mixxx::AcoustIdLookupTask*>(sender())) {
        return false;
    }
    m_pAcoustIdTask = nullptr;
    const auto taskDeleter = mixxx::ScopedDeleteLater(pAcoustIdTask);
    pAcoustIdTask->disconnect(this);
    return true;
}

void TagFetcher::slotAcoustIdTaskFailed(
        const mixxx::network::JsonWebResponse& response) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onAcoustIdTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            response.statusCode(),
            "AcoustID",
            response.content().toJson(),
            -1);
}

void TagFetcher::slotAcoustIdTaskAborted() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onAcoustIdTaskTerminated()) {
        return;
    }

    cancel();
}

void TagFetcher::slotAcoustIdTaskNetworkError(
        QNetworkReply::NetworkError errorCode,
        const QString& errorString,
        const mixxx::network::WebResponseWithContent& responseWithContent) {
    Q_UNUSED(responseWithContent);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onAcoustIdTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            mixxx::network::kHttpStatusCodeInvalid,
            QStringLiteral("AcoustID"),
            errorString,
            errorCode);
}

bool TagFetcher::onMusicBrainzTaskTerminated() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    auto* const pMusicBrainzTask = m_pMusicBrainzTask.get();
    DEBUG_ASSERT(sender());
    VERIFY_OR_DEBUG_ASSERT(pMusicBrainzTask ==
            qobject_cast<mixxx::MusicBrainzRecordingsTask*>(sender())) {
        return false;
    }
    m_pMusicBrainzTask = nullptr;
    const auto taskDeleter = mixxx::ScopedDeleteLater(pMusicBrainzTask);
    pMusicBrainzTask->disconnect(this);
    return true;
}

void TagFetcher::slotMusicBrainzTaskAborted() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onMusicBrainzTaskTerminated()) {
        return;
    }

    cancel();
}

void TagFetcher::slotMusicBrainzTaskNetworkError(
        QNetworkReply::NetworkError errorCode,
        const QString& errorString,
        const mixxx::network::WebResponseWithContent& responseWithContent) {
    Q_UNUSED(responseWithContent);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onMusicBrainzTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            mixxx::network::kHttpStatusCodeInvalid,
            QStringLiteral("MusicBrainz"),
            errorString,
            errorCode);
}

void TagFetcher::slotMusicBrainzTaskFailed(
        const mixxx::network::WebResponse& response,
        int errorCode,
        const QString& errorMessage) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onMusicBrainzTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            response.statusCode(),
            "MusicBrainz",
            errorMessage,
            errorCode);
}

void TagFetcher::slotMusicBrainzTaskSucceeded(
        const QList<mixxx::musicbrainz::TrackRelease>& guessedTrackReleases) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onMusicBrainzTaskTerminated()) {
        return;
    }

    auto pTrack = std::move(m_pTrack);
    cancel();

    emit resultAvailable(
            std::move(pTrack),
            std::move(guessedTrackReleases));
}

void TagFetcher::startFetchForCoverArt(
        const QList<mixxx::musicbrainz::TrackRelease>& guessedTrackReleases) {
    emit fetchProgress(tr("Finding Possible Cover Arts on Cover Art Archive"));

    QList<QUuid> albumReleaseIds;

    foreach (auto& guessedTrackRelease, guessedTrackReleases) {
        albumReleaseIds.append(guessedTrackRelease.albumReleaseId);
    }

    m_pCoverArtArchiveLinksTask = make_parented<mixxx::CoverArtArchiveLinksTask>(
            &m_network,
            std::move(albumReleaseIds),
            this);

    connect(m_pCoverArtArchiveLinksTask,
            &mixxx::CoverArtArchiveLinksTask::succeeded,
            this,
            &TagFetcher::slotCoverArtArchiveLinksTaskSucceeded);
    connect(m_pCoverArtArchiveLinksTask,
            &mixxx::CoverArtArchiveLinksTask::succeededLinks,
            this,
            &TagFetcher::slotCoverArtArchiveLinksTaskSucceededLinks);
    connect(m_pCoverArtArchiveLinksTask,
            &mixxx::CoverArtArchiveLinksTask::failed,
            this,
            &TagFetcher::slotCoverArtArchiveLinksTaskFailed);
    connect(m_pCoverArtArchiveLinksTask,
            &mixxx::CoverArtArchiveLinksTask::aborted,
            this,
            &TagFetcher::slotCoverArtArchiveLinksTaskAborted);
    connect(m_pCoverArtArchiveLinksTask,
            &mixxx::CoverArtArchiveLinksTask::networkError,
            this,
            &TagFetcher::slotCoverArtArchiveLinksTaskNetworkError);
    connect(m_pCoverArtArchiveLinksTask,
            &mixxx::CoverArtArchiveLinksTask::notFound,
            this,
            &TagFetcher::slotCoverArtArchiveLinksTaskNotFound);

    m_pCoverArtArchiveLinksTask->invokeStart(
            kCoverArtArchiveLinksTimeoutMilis);
}

bool TagFetcher::onCoverArtArchiveLinksTaskTerminated() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    auto* const pCoverArtArchiveLinksTask = m_pCoverArtArchiveLinksTask.get();
    DEBUG_ASSERT(sender());
    VERIFY_OR_DEBUG_ASSERT(pCoverArtArchiveLinksTask ==
            qobject_cast<mixxx::CoverArtArchiveLinksTask*>(sender())) {
        return false;
    }
    m_pCoverArtArchiveLinksTask = nullptr;
    const auto taskDeleter = mixxx::ScopedDeleteLater(pCoverArtArchiveLinksTask);
    pCoverArtArchiveLinksTask->disconnect(this);
    return true;
}

void TagFetcher::slotCoverArtArchiveLinksTaskAborted() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveLinksTaskTerminated()) {
        return;
    }

    cancel();
}

void TagFetcher::slotCoverArtArchiveLinksTaskNotFound() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveLinksTaskTerminated()) {
        return;
    }
    qDebug() << "No image found on Cover Art Archive";
    cancel();
}

void TagFetcher::slotCoverArtArchiveLinksTaskNetworkError(
        QNetworkReply::NetworkError errorCode,
        const QString& errorString,
        const mixxx::network::WebResponseWithContent& responseWithContent) {
    Q_UNUSED(responseWithContent);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveLinksTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            mixxx::network::kHttpStatusCodeInvalid,
            QStringLiteral("CoverArtArchive"),
            errorString,
            errorCode);
}

void TagFetcher::slotCoverArtArchiveLinksTaskFailed(
        const mixxx::network::JsonWebResponse& response) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveLinksTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            response.statusCode(),
            "CoverArtArchive",
            response.content().toJson(),
            -1);
}

void TagFetcher::slotCoverArtArchiveLinksTaskSucceeded(
        const QMap<QUuid, QString>& coverArtThumbnailUrls) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveLinksTaskTerminated()) {
        return;
    }

    auto pTrack = std::move(m_pTrack);
    cancel();

    emit fetchProgress(tr("Cover Arts found, Getting Thumbnails."));

    m_pCoverArtArchiveThumbnailsTask = make_parented<mixxx::CoverArtArchiveThumbnailsTask>(
            &m_network,
            std::move(coverArtThumbnailUrls),
            this);

    connect(m_pCoverArtArchiveThumbnailsTask,
            &mixxx::CoverArtArchiveThumbnailsTask::succeeded,
            this,
            &TagFetcher::slotCoverArtArchiveThumbnailsTaskSucceeded);
    connect(m_pCoverArtArchiveThumbnailsTask,
            &mixxx::CoverArtArchiveThumbnailsTask::failed,
            this,
            &TagFetcher::slotCoverArtArchiveThumbnailsTaskFailed);
    connect(m_pCoverArtArchiveThumbnailsTask,
            &mixxx::CoverArtArchiveThumbnailsTask::aborted,
            this,
            &TagFetcher::slotCoverArtArchiveThumbnailsTaskAborted);
    connect(m_pCoverArtArchiveThumbnailsTask,
            &mixxx::CoverArtArchiveThumbnailsTask::networkError,
            this,
            &TagFetcher::slotCoverArtArchiveThumbnailsTaskNetworkError);

    m_pCoverArtArchiveThumbnailsTask->invokeStart(
            kCoverArtArchiveThumbnailsTimeoutMilis);
}

void TagFetcher::slotCoverArtArchiveThumbnailsTaskAborted() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveThumbnailsTaskTerminated()) {
        return;
    }

    cancel();
}

void TagFetcher::slotCoverArtArchiveThumbnailsTaskSucceeded(
        const QMap<QUuid, QByteArray>& smallThumbnailsBytes) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveThumbnailsTaskTerminated()) {
        return;
    }
    emit coverArtThumbnailFetchAvailable(smallThumbnailsBytes);

    auto pTrack = std::move(m_pTrack);
}

bool TagFetcher::onCoverArtArchiveThumbnailsTaskTerminated() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    auto* const pCoverArtArchiveThumbnailsTask = m_pCoverArtArchiveThumbnailsTask.get();
    DEBUG_ASSERT(sender());
    VERIFY_OR_DEBUG_ASSERT(pCoverArtArchiveThumbnailsTask ==
            qobject_cast<mixxx::CoverArtArchiveThumbnailsTask*>(sender())) {
        return false;
    }
    m_pCoverArtArchiveThumbnailsTask = nullptr;
    const auto taskDeleter = mixxx::ScopedDeleteLater(pCoverArtArchiveThumbnailsTask);
    pCoverArtArchiveThumbnailsTask->disconnect(this);
    return true;
}

void TagFetcher::slotCoverArtArchiveThumbnailsTaskFailed(
        const mixxx::network::WebResponse& response,
        int errorCode,
        const QString& errorMessage) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveThumbnailsTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            response.statusCode(),
            "CoverArtImageTask",
            errorMessage,
            errorCode);
}

void TagFetcher::slotCoverArtArchiveThumbnailsTaskNetworkError(
        QNetworkReply::NetworkError errorCode,
        const QString& errorString,
        const mixxx::network::WebResponseWithContent& responseWithContent) {
    Q_UNUSED(responseWithContent);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveThumbnailsTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            mixxx::network::kHttpStatusCodeInvalid,
            QStringLiteral("CoverArtArchive"),
            errorString,
            errorCode);
}

void TagFetcher::slotCoverArtArchiveLinksTaskSucceededLinks(
        const QMap<QUuid, QList<QString>>& allCoverArtUrls) {
    emit coverArtUrlsAvailable(allCoverArtUrls);
}

void TagFetcher::fetchDesiredResolutionCoverArt(
        const QString& coverArtUrl) {
    m_pCoverArtArchiveImageTask = make_parented<mixxx::CoverArtArchiveImageTask>(
            &m_network,
            coverArtUrl,
            this);

    connect(m_pCoverArtArchiveImageTask,
            &mixxx::CoverArtArchiveImageTask::succeeded,
            this,
            &TagFetcher::slotCoverArtArchiveImageTaskSucceeded);

    m_pCoverArtArchiveImageTask->invokeStart(
            kCoverArtArchiveImageTimeoutMilis);
}

void TagFetcher::slotCoverArtArchiveImageTaskSucceeded(const QByteArray& coverArtBytes) {
    emit coverArtImageFetchAvailable(coverArtBytes);
}
