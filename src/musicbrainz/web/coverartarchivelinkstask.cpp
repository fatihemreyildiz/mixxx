#include "musicbrainz/web/coverartarchivelinkstask.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

#include "network/httpstatuscode.h"
#include "util/assert.h"
#include "util/logger.h"

namespace mixxx {

namespace {

const Logger kLogger("CoverArtArchiveLinksTask");

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
        const QUuid& releaseID) {
    DEBUG_ASSERT(kBaseUrl.isValid());
    DEBUG_ASSERT(!releaseID.isNull());
    QUrl url = kBaseUrl;
    url.setPath(kRequestPath + releaseID.toString(QUuid::WithoutBraces));
    DEBUG_ASSERT(url.isValid());
    QNetworkRequest networkRequest(url);
    return networkRequest;
}

} // anonymous namespace

CoverArtArchiveLinksTask::CoverArtArchiveLinksTask(
        QNetworkAccessManager* networkAccessManager,
        const QUuid& albumReleaseId,
        QObject* parent)
        : network::JsonWebTask(
                  networkAccessManager,
                  kBaseUrl,
                  lookupRequest(),
                  parent),
          m_albumReleaseId(albumReleaseId),
          m_parentTimeoutMillis(0) {
}

QNetworkReply* CoverArtArchiveLinksTask::sendNetworkRequest(
        QNetworkAccessManager* networkAccessManager,
        network::HttpRequestMethod method,
        int parentTimeoutMillis,
        const QUrl& url,
        const QJsonDocument& content) {
    DEBUG_ASSERT(networkAccessManager);
    //DEBUG_ASSERT(!m_queuedAlbumReleaseIds.first().isNull());
    Q_UNUSED(method);
    DEBUG_ASSERT(method == network::HttpRequestMethod::Get);
    networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    m_parentTimeoutMillis = parentTimeoutMillis;

    const QNetworkRequest networkRequest = createNetworkRequest(m_albumReleaseId);

    VERIFY_OR_DEBUG_ASSERT(url.isValid()) {
        kLogger.warning() << "Invalid URL" << url;
        return nullptr;
    }
    if (kLogger.traceEnabled()) {
        kLogger.trace()
                << "Get"
                << url;
    }
    return networkAccessManager->get(networkRequest);
}

void CoverArtArchiveLinksTask::onFinished(
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

    QList<QString> allUrls;
    DEBUG_ASSERT(jsonObject.value(QLatin1String("images")).isArray());
    const QJsonArray images = jsonObject.value(QLatin1String("images")).toArray();
    for (const auto& image : images) {
        DEBUG_ASSERT(image.isObject());
        const auto imageObject = image.toObject();

        const auto thumbnails = imageObject.value(QLatin1String("thumbnails")).toObject();
        DEBUG_ASSERT(!thumbnails.isEmpty());

        const auto smallThumbnailUrl = thumbnails.value(QLatin1String("small")).toString();
        DEBUG_ASSERT(!smallThumbnailUrl.isNull());
        allUrls.append(smallThumbnailUrl);

        const auto largeThumbnailUrl = thumbnails.value(QLatin1String("large")).toString();
        DEBUG_ASSERT(!largeThumbnailUrl.isNull());
        allUrls.append(largeThumbnailUrl);

        if (thumbnails.value(QLatin1String("1200")).toString() != nullptr) {
            const auto largestThumbnailUrl = thumbnails.value(QLatin1String("1200")).toString();
            allUrls.append(largestThumbnailUrl);
        }

        if (!(imageObject.value(QLatin1String("image")).isNull())) {
            const auto highestResolutionImageUrl =
                    imageObject.value(QLatin1String("image")).toString();
            allUrls.append(highestResolutionImageUrl);
        }

        break;
    }
    emitSucceeded(allUrls);
}

void CoverArtArchiveLinksTask::emitSucceeded(
        const QList<QString>& allUrls) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&CoverArtArchiveLinksTask::succeeded)) {
        kLogger.warning()
                << "Unhandled succeeded signal";
        deleteLater();
        return;
    }
    emit succeeded(allUrls);
}

} // namespace mixxx
