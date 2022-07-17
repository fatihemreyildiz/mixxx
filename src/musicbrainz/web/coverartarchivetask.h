#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QUuid>

#include "network/jsonwebtask.h"

namespace mixxx {

class CoverArtArchiveTask : public network::JsonWebTask {
    Q_OBJECT

  public:
    CoverArtArchiveTask(
            QNetworkAccessManager* networkAccessManager,
            QList<QUuid>&& albumReleaseIds,
            QObject* parent = nullptr);

    ~CoverArtArchiveTask() override = default;

  signals:
    void succeeded(
            const QMap<QUuid, QString>& smallThumbnailUrls);

    void failed(
            const mixxx::network::JsonWebResponse& response);

  private:
    QNetworkReply* sendNetworkRequest(
            QNetworkAccessManager* networkAccessManager,
            network::HttpRequestMethod method,
            int parentTimeoutMillis,
            const QUrl& url,
            const QJsonDocument& content) override;

    void onFinished(
            const network::JsonWebResponse& response) override;

    void emitSucceeded(
            const QMap<QUuid, QString>& smallThumbnailUrls);

    void emitAllUrls();

    QList<QUuid> m_queuedAlbumReleaseIds;

    QList<QString> m_allThumbnailUrls;

    QMap<QUuid, QString> m_smallThumbnailUrls;

    QMap<QUuid, QList<QString>> m_coverArtUrls; // This can be used later on when
                                                // preferences implementation.

    int m_parentTimeoutMillis;
};
} // namespace mixxx
