#pragma once

#include <QList>

#include "network/webtask.h"

namespace mixxx {
// This task is just for to get Images from the coverartarchive.
class CoverArtArchiveImageTask : public network::WebTask {
    Q_OBJECT

  public:
    CoverArtArchiveImageTask(
            QNetworkAccessManager* networkAccessManager,
            const QString& coverUrl,
            QObject* parent = nullptr);
    ~CoverArtArchiveImageTask() override = default;

  signals:
    void succeeded(
            const QByteArray& imageInfo);
    void failed(
            const network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);

  private:
    QNetworkReply* doStartNetworkRequest(
            QNetworkAccessManager* networkAccessManager,
            int parentTimeoutMillis) override;
    void doNetworkReplyFinished(
            QNetworkReply* finishedNetworkReply,
            network::HttpStatusCode statusCode) override;

    void emitSucceeded(
            const QByteArray& imageInfo);

    void emitFailed(
            const network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);
};

} // namespace mixxx
