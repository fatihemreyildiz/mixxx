#pragma once

#include <QList>
#include <QMap>
#include <QSet>
#include <QUrlQuery>
#include <QUuid>

#include "musicbrainz/musicbrainz.h"
#include "network/webtask.h"
#include "util/timer.h"

namespace mixxx {

class MusicBrainzRecordingsTask : public network::WebTask {
    Q_OBJECT

  public:
    MusicBrainzRecordingsTask(
            QNetworkAccessManager* networkAccessManager,
            QList<QUuid>&& recordingIds,
            QObject* parent = nullptr);
    ~MusicBrainzRecordingsTask() override = default;

  signals:
    void succeeded(
            const QList<musicbrainz::TrackRelease>& trackReleases);
    void failed(
            const network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);
    void currentRecordingFetched();

  private:
    QNetworkReply* doStartNetworkRequest(
            QNetworkAccessManager* networkAccessManager,
            int parentTimeoutMillis) override;
    void doNetworkReplyFinished(
            QNetworkReply* finishedNetworkReply,
            network::HttpStatusCode statusCode) override;

    void emitSucceeded(
            const QList<musicbrainz::TrackRelease>& trackReleases);
    void emitFailed(
            const network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);

    void continueWithNextRequest();

    const QUrlQuery m_urlQuery;

    QList<QUuid> m_queuedRecordingIds;
    QSet<QUuid> m_finishedRecordingIds;

    QMap<QUuid, musicbrainz::TrackRelease> m_trackReleases;

    Timer m_timer;

    int m_parentTimeoutMillis;
};

} // namespace mixxx
