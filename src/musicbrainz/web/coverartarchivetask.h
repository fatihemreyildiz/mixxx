#pragma once

#include <QList>
#include <QString>

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
            const QList<QString>& coverArtPaths);

    void failed(
            const mixxx::network::JsonWebResponse& response);

  private:
    QNetworkReply* sendNetworkRequest(
            QNetworkAccessManager* networkAccessManager,
            network::HttpRequestMethod method,
            const QUrl& url,
            const QJsonDocument& content) override;

    void onFinished(
            const network::JsonWebResponse& response) override;

    void emitSucceeded(
            const QList<QString>& recordingIds);

    const QString m_AlbumReleaseId;
};
} // namespace mixxx
