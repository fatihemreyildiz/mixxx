#pragma once

#include <QFutureWatcher>
#include <QObject>

#include "musicbrainz/web/acoustidlookuptask.h"
#include "musicbrainz/web/coverartarchivelinkstask.h"
#include "musicbrainz/web/coverartarchivethumbnailstask.h"
#include "musicbrainz/web/musicbrainzrecordingstask.h"
#include "track/track_decl.h"
#include "util/parented_ptr.h"

class TagFetcher : public QObject {
    Q_OBJECT

    // Implements retrieval of metadata in 3 subsequent stages:
    //   1. Chromaprint -> AcoustID fingerprint
    //   2. AcoustID -> MusicBrainz recording UUIDs
    //   3. MusicBrainz -> MusicBrainz track releases

  public:
    explicit TagFetcher(
            QObject* parent = nullptr);
    ~TagFetcher() override = default;

    void startFetch(
            TrackPointer pTrack);

  public slots:
    void cancel();

  signals:
    void resultAvailable(
            TrackPointer pTrack,
            const QList<mixxx::musicbrainz::TrackRelease>& guessedTrackReleases);
    void fetchProgress(
            const QString& message);
    void networkError(
            int httpStatus,
            const QString& app,
            const QString& message,
            int code);
    void fetchedCoverUpdate(const QByteArray& coverInfo);
    void coverArtThumbnailFetchAvailable(const QMap<QUuid, QByteArray>& smallThumbnailsBytes);

  private slots:
    void slotFingerprintReady();

    void slotAcoustIdTaskSucceeded(
            QList<QUuid> recordingIds);
    void slotAcoustIdTaskFailed(
            const mixxx::network::JsonWebResponse& response);
    void slotAcoustIdTaskAborted();
    void slotAcoustIdTaskNetworkError(
            QNetworkReply::NetworkError errorCode,
            const QString& errorString,
            const mixxx::network::WebResponseWithContent& responseWithContent);

    void slotMusicBrainzTaskSucceeded(
            const QList<mixxx::musicbrainz::TrackRelease>& guessedTrackReleases);
    void slotMusicBrainzTaskFailed(
            const mixxx::network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);
    void slotMusicBrainzTaskAborted();
    void slotMusicBrainzTaskNetworkError(
            QNetworkReply::NetworkError errorCode,
            const QString& errorString,
            const mixxx::network::WebResponseWithContent& responseWithContent);

    void slotCoverArtArchiveLinksTaskSucceeded(const QMap<QUuid, QString>& coverArtThumbnailUrls);
    void slotCoverArtArchiveLinksTaskFailed(
            const mixxx::network::JsonWebResponse& response);
    void slotCoverArtArchiveLinksTaskAborted();
    void slotCoverArtArchiveLinksTaskNetworkError(
            QNetworkReply::NetworkError errorCode,
            const QString& errorString,
            const mixxx::network::WebResponseWithContent& responseWithContent);
    void slotCoverArtArchiveLinksTaskNotFound();

    void slotCoverArtArchiveThumbnailsTaskSucceeded(
            const QMap<QUuid, QByteArray>& smallThumbnailsBytes);
    void slotCoverArtArchiveThumbnailsTaskFailed(
            const mixxx::network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);
    void slotCoverArtArchiveThumbnailsTaskAborted();
    void slotCoverArtArchiveThumbnailsTaskNetworkError(
            QNetworkReply::NetworkError errorCode,
            const QString& errorString,
            const mixxx::network::WebResponseWithContent& responseWithContent);

  private:
    bool onAcoustIdTaskTerminated();
    bool onMusicBrainzTaskTerminated();
    bool onCoverArtArchiveLinksTaskTerminated();
    bool onCoverArtArchiveThumbnailsTaskTerminated();

    QNetworkAccessManager m_network;

    QFutureWatcher<QString> m_fingerprintWatcher;

    parented_ptr<mixxx::AcoustIdLookupTask> m_pAcoustIdTask;

    parented_ptr<mixxx::MusicBrainzRecordingsTask> m_pMusicBrainzTask;

    parented_ptr<mixxx::CoverArtArchiveLinksTask> m_pCoverArtArchiveLinksTask;

    parented_ptr<mixxx::CoverArtArchiveThumbnailsTask> m_pCoverArtArchiveThumbnailsTask;

    TrackPointer m_pTrack;
};
