#pragma once

#include <QList>
#include <QString>
#include <QUuid>

#include "network/jsonwebtask.h"

namespace mixxx {

class CoverArtArchiveTask : public network::JsonWebTask {
    Q_OBJECT

  public:
    CoverArtArchiveTask(
            QNetworkAccessManager* networkAccessManager,
            const QString& releaseId,
            QObject* parent = nullptr);

    ~CoverArtArchiveTask() override = default;

  signals:
    void succeeded(
            const QList<QUuid>& coverArtPaths);

    void failed(
            const mixxx::network::JsonWebResponse& response);

  private:
    QNetworkReply* sendNetworkRequest(
            QNetworkAccessManager* networkAccessManager,
            network::HttpRequestMethod method,
            const QUrl& url,
            const QJsonDocument& content) override;
    void doNetworkReplyFinished(
            QNetworkReply* finishedNetworkReply,
            network::HttpStatusCode statusCode) override;

    void emitSucceeded(
            const QList<QUuid>& recordingIds);

    const QString m_AlbumReleaseId;
};
} // namespace mixxx
