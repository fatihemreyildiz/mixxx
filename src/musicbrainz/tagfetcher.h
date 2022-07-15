#pragma once

#include <QFutureWatcher>
#include <QObject>

#include "musicbrainz/web/acoustidlookuptask.h"
#include "musicbrainz/web/coverartarchiveimagetask.h"
#include "musicbrainz/web/coverartarchivetask.h"
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

    void coverArtSend(const QString& albumReleaseId);
    void coverArtSendImageRequest();

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

    void slotCoverArtArchiveTaskSucceeded();
    void slotCoverArtArchiveTaskFailed(
            const mixxx::network::JsonWebResponse& response);
    void slotCoverArtArchiveTaskAborted();
    void slotCoverArtArchiveTaskNetworkError(
            QNetworkReply::NetworkError errorCode,
            const QString& errorString,
            const mixxx::network::WebResponseWithContent& responseWithContent);
    void slotCoverArtArchiveTaskNotFound();

  private:
    bool onAcoustIdTaskTerminated();
    bool onMusicBrainzTaskTerminated();
    bool onCoverArtArchiveTaskTerminated();

    QNetworkAccessManager m_network;

    QFutureWatcher<QString> m_fingerprintWatcher;

    parented_ptr<mixxx::AcoustIdLookupTask> m_pAcoustIdTask;

    parented_ptr<mixxx::MusicBrainzRecordingsTask> m_pMusicBrainzTask;

    parented_ptr<mixxx::CoverArtArchiveTask> m_pCoverArtArchiveTask;

    parented_ptr<mixxx::CoverArtArchiveImageTask> m_pCoverArtArchiveImageTask;

    TrackPointer m_pTrack;
};
