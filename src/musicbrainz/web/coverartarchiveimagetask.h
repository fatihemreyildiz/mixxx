#pragma once

#include <QList>

#include "network/webtask.h"

namespace mixxx {

class CoverArtArchiveImageTask : public network::WebTask {
    Q_OBJECT

  public:
    CoverArtArchiveImageTask(
            QNetworkAccessManager* networkAccessManager,
            const QUrl& coverUrl,
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

    void emitFailed(
            const network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);

    const QUrl m_ThumbnailUrl;
};

} // namespace mixxx
