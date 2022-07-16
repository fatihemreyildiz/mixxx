#include "musicbrainz/web/coverartarchivetask.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

#include "network/httpstatuscode.h"
#include "util/assert.h"
#include "util/logger.h"

namespace mixxx {

namespace {

const Logger kLogger("CoverArtArchiveTask");

const QUrl kBaseUrl = QStringLiteral("https://coverartarchive.org/");

const QString kRequestPath = QStringLiteral("/release/");

network::JsonWebRequest lookupRequest() {
    return network::JsonWebRequest{
            network::HttpRequestMethod::Get,
            kRequestPath,
            QUrlQuery(),
            QJsonDocument()};
}

QNetworkRequest createNetworkRequest(
        const QString& releaseID) {
    DEBUG_ASSERT(kBaseUrl.isValid());
    DEBUG_ASSERT(!releaseID.isNull());
    QUrl url = kBaseUrl;
    url.setPath(kRequestPath + releaseID);
    DEBUG_ASSERT(url.isValid());
    QNetworkRequest networkRequest(url);
    return networkRequest;
}

} // anonymous namespace

CoverArtArchiveTask::CoverArtArchiveTask(
        QNetworkAccessManager* networkAccessManager,
        const QString& releaseId,
        QObject* parent)
        : network::JsonWebTask(
                  networkAccessManager,
                  kBaseUrl,
                  lookupRequest(),
                  parent),
          m_AlbumReleaseId(releaseId) {
}

QNetworkReply* CoverArtArchiveTask::sendNetworkRequest(
        QNetworkAccessManager* networkAccessManager,
        network::HttpRequestMethod method,
        const QUrl& url,
        const QJsonDocument& content) {
    networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    const QNetworkRequest networkRequest = createNetworkRequest(m_AlbumReleaseId);

    DEBUG_ASSERT(networkAccessManager);
    VERIFY_OR_DEBUG_ASSERT(url.isValid()) {
        kLogger.warning() << "Invalid URL" << url;
        return nullptr;
    }
    Q_UNUSED(method);
    DEBUG_ASSERT(method == network::HttpRequestMethod::Get);
    if (kLogger.traceEnabled()) {
        kLogger.trace()
                << "Get"
                << url;
    }
    return networkAccessManager->get(networkRequest);
}

void CoverArtArchiveTask::onFinished(
        const network::JsonWebResponse& response) {
    if (!response.isStatusCodeSuccess()) {
        kLogger.warning()
                << "Request failed with HTTP status code"
                << response.statusCode();
        emitFailed(response);
        return;
    }
    VERIFY_OR_DEBUG_ASSERT(response.statusCode() == network::kHttpStatusCodeOk) {
        kLogger.warning()
                << "Unexpected HTTP status code"
                << response.statusCode();
        emitFailed(response);
        return;
    }
    VERIFY_OR_DEBUG_ASSERT(response.content().isObject()) {
        kLogger.warning()
                << "Invalid JSON content"
                << response.content();
        emitFailed(response);
        return;
    }
    const auto jsonObject = response.content().object();

    if (jsonObject.isEmpty()) {
        kLogger.warning()
                << "Empty Json";
        emitFailed(response);
        return;
    }

    QList<QString> coverArtPaths;
    DEBUG_ASSERT(jsonObject.value(QLatin1String("images")).isArray());
    const QJsonArray images = jsonObject.value(QLatin1String("images")).toArray();
    for (const auto& image : images) {
        DEBUG_ASSERT(image.isObject());
        const auto imageObject = image.toObject();
        const auto imageLink =
                imageObject.value(QLatin1String("image")).toString();

        const auto thumbnails =
                imageObject.value(QLatin1String("thumbnails")).toObject();

        for (const auto& thumbnail : thumbnails) {
            if (thumbnail.isUndefined()) {
                if (kLogger.debugEnabled()) {
                    kLogger.debug()
                            << "No thumbnail(s) available for image"
                            << imageLink;
                }
                continue;
            } else {
                const auto thumbnailLink =
                        QString(thumbnail.toString());
                coverArtPaths.append(thumbnailLink);
            }
        }
    }
    emitSucceeded(coverArtPaths);
}

void CoverArtArchiveTask::emitSucceeded( //Dummy function, will be developed later.
        const QList<QString>& coverArtPaths) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&CoverArtArchiveTask::succeeded)) {
        kLogger.warning()
                << "Unhandled succeeded signal";
        deleteLater();
        return;
    }
    emit succeeded(coverArtPaths);
}

} // namespace mixxx
