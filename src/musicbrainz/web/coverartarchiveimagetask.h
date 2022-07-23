#pragma once

#include "network/webtask.h"

namespace mixxx {

class CoverArtArchiveImageTask : public network::WebTask {
    Q_OBJECT

  public:
    CoverArtArchiveImageTask(
            QNetworkAccessManager* networkAccessManager,
            const QString& coverArtLink,
            QObject* parent = nullptr);
    ~CoverArtArchiveImageTask() override = default;

  signals:
    void succeeded(
            const QByteArray& coverArtImageBytes);

  private:
    QNetworkReply* doStartNetworkRequest(
            QNetworkAccessManager* networkAccessManager,
            int parentTimeoutMillis) override;

    void doNetworkReplyFinished(
            QNetworkReply* finishedNetworkReply,
            network::HttpStatusCode statusCode) override;

    QString m_coverArtUrl;

    QByteArray coverArtImageBytes;
};

} // namespace mixxx
