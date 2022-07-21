#pragma once

#include <QList>
#include <QMap>
#include <QUuid>

#include "network/webtask.h"

namespace mixxx {

class CoverArtArchiveThumbnailsTask : public network::WebTask {
    Q_OBJECT

  public:
    CoverArtArchiveThumbnailsTask(
            QNetworkAccessManager* networkAccessManager,
            const QMap<QUuid, QString>& smallThumbnailsUrls,
            QObject* parent = nullptr);
    ~CoverArtArchiveThumbnailsTask() override = default;

  signals:
    void succeeded(
            const QMap<QUuid, QByteArray>& m_smallThumbnailBytes);
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

    void emitSuceeded(const QMap<QUuid, QByteArray>& m_smallThumbnailBytes);

    void emitFailed(
            const network::WebResponse& response,
            int errorCode,
            const QString& errorMessage);

    QMap<QUuid, QString> m_queuedSmallThumbnailUrls;

    QMap<QUuid, QByteArray> m_smallThumbnailBytes;

    int m_parentTimeoutMillis;
};

} // namespace mixxx
