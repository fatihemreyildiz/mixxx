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
    if (m_pCoverArtArchiveImageTask) {
        m_pCoverArtArchiveImageTask->invokeAbort();
        DEBUG_ASSERT(!m_pCoverArtArchiveImageTask);
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

void TagFetcher::startFetchCoverArtLinks(
        const QUuid& albumReleaseId) {
    //In here the progress message can be handled better.
    //emit fetchProgress(tr("Finding Possible Cover Arts on Cover Art Archive"));
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    cancel();

    m_pCoverArtArchiveLinksTask = make_parented<mixxx::CoverArtArchiveLinksTask>(
            &m_network,
            std::move(albumReleaseId),
            this);

    connect(m_pCoverArtArchiveLinksTask,
            &mixxx::CoverArtArchiveLinksTask::succeeded,
            this,
            &TagFetcher::slotCoverArtArchiveLinksTaskSucceeded);
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

void TagFetcher::slotCoverArtArchiveLinksTaskNetworkError(
        QNetworkReply::NetworkError errorCode,
        const QString& errorString,
        const mixxx::network::WebResponseWithContent& responseWithContent) {
    Q_UNUSED(responseWithContent);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveLinksTaskTerminated()) {
        return;
    }

    // (TODO) Handle the error better for Cover Art Archive Links.
    emit coverArtLinkNotFound();
    cancel();
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
        const QList<QString>& allUrls) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveLinksTaskTerminated()) {
        return;
    }

    auto pTrack = std::move(m_pTrack);
    cancel();

    emit coverArtArchiveLinksAvailable(std::move(allUrls));
}

void TagFetcher::startFetchCoverArtImage(
        const QString& coverArtUrl) {
    m_pCoverArtArchiveImageTask = make_parented<mixxx::CoverArtArchiveImageTask>(
            &m_network,
            coverArtUrl,
            this);

    connect(m_pCoverArtArchiveImageTask,
            &mixxx::CoverArtArchiveImageTask::succeeded,
            this,
            &TagFetcher::slotCoverArtArchiveImageTaskSucceeded);
    connect(m_pCoverArtArchiveImageTask,
            &mixxx::CoverArtArchiveImageTask::failed,
            this,
            &TagFetcher::slotCoverArtArchiveImageTaskFailed);
    connect(m_pCoverArtArchiveImageTask,
            &mixxx::CoverArtArchiveImageTask::aborted,
            this,
            &TagFetcher::slotCoverArtArchiveImageTaskAborted);
    connect(m_pCoverArtArchiveImageTask,
            &mixxx::CoverArtArchiveImageTask::networkError,
            this,
            &TagFetcher::slotCoverArtArchiveImageTaskNetworkError);

    m_pCoverArtArchiveImageTask->invokeStart(
            kCoverArtArchiveImageTimeoutMilis);
}

void TagFetcher::slotCoverArtArchiveImageTaskSucceeded(const QByteArray& coverArtBytes) {
    if (!onCoverArtArchiveImageTaskTerminated()) {
        return;
    }

    auto pTrack = std::move(m_pTrack);
    cancel();

    emit coverArtImageFetchAvailable(coverArtBytes);
}

void TagFetcher::slotCoverArtArchiveImageTaskAborted() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveImageTaskTerminated()) {
        return;
    }

    cancel();
}

bool TagFetcher::onCoverArtArchiveImageTaskTerminated() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    auto* const pCoverArtArchiveImageTask = m_pCoverArtArchiveImageTask.get();
    DEBUG_ASSERT(sender());
    VERIFY_OR_DEBUG_ASSERT(pCoverArtArchiveImageTask ==
            qobject_cast<mixxx::CoverArtArchiveImageTask*>(sender())) {
        return false;
    }
    m_pCoverArtArchiveImageTask = nullptr;
    const auto taskDeleter = mixxx::ScopedDeleteLater(pCoverArtArchiveImageTask);
    pCoverArtArchiveImageTask->disconnect(this);
    return true;
}

void TagFetcher::slotCoverArtArchiveImageTaskFailed(
        const mixxx::network::WebResponse& response,
        int errorCode,
        const QString& errorMessage) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveImageTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            response.statusCode(),
            "CoverArtImageTask",
            errorMessage,
            errorCode);
}

void TagFetcher::slotCoverArtArchiveImageTaskNetworkError(
        QNetworkReply::NetworkError errorCode,
        const QString& errorString,
        const mixxx::network::WebResponseWithContent& responseWithContent) {
    Q_UNUSED(responseWithContent);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    if (!onCoverArtArchiveImageTaskTerminated()) {
        return;
    }

    cancel();

    emit networkError(
            mixxx::network::kHttpStatusCodeInvalid,
            QStringLiteral("CoverArtArchive"),
            errorString,
            errorCode);
}
