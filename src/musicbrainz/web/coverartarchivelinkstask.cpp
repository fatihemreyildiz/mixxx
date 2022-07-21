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
        QList<QUuid>&& albumReleaseIds,
        QObject* parent)
        : network::JsonWebTask(
                  networkAccessManager,
                  kBaseUrl,
                  lookupRequest(),
                  parent),
          m_queuedAlbumReleaseIds(albumReleaseIds),
          m_parentTimeoutMillis(0) {
}

QNetworkReply* CoverArtArchiveLinksTask::sendNetworkRequest(
        QNetworkAccessManager* networkAccessManager,
        network::HttpRequestMethod method,
        int parentTimeoutMillis,
        const QUrl& url,
        const QJsonDocument& content) {
    DEBUG_ASSERT(networkAccessManager);
    DEBUG_ASSERT(!m_queuedAlbumReleaseIds.first().isNull());
    Q_UNUSED(method);
    DEBUG_ASSERT(method == network::HttpRequestMethod::Get);
    networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    m_parentTimeoutMillis = parentTimeoutMillis;

    // It is not takefirst because we will need it to store the QMap's later on, before the new request this index will be deleted
    const QNetworkRequest networkRequest = createNetworkRequest(m_queuedAlbumReleaseIds.first());

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
    //This is added only for to get Url of the images
    //I tried few ways, they didn't work at all
    //This was the only one who solved the issue behind
    //blocking the sequence of requests
    if (response.statusCode() == 404) {
        qDebug() << "No image link found for this request";
        m_queuedAlbumReleaseIds.removeFirst();
        if (m_queuedAlbumReleaseIds.isEmpty()) {
            qDebug() << m_smallThumbnailUrls.size() << " Possible Thumbnail Links found.";
            // emit all links.
            emitSucceeded(m_smallThumbnailUrls);
            return;
        }
        slotStart(m_parentTimeoutMillis);
        return;
    }

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

    DEBUG_ASSERT(jsonObject.value(QLatin1String("images")).isArray());
    const QJsonArray images = jsonObject.value(QLatin1String("images")).toArray();
    for (const auto& image : images) {
        DEBUG_ASSERT(image.isObject());
        const auto imageObject = image.toObject();
        const auto thumbnails = imageObject.value(QLatin1String("thumbnails")).toObject();
        DEBUG_ASSERT(!thumbnails.isEmpty());
        const auto smallThumbnailUrl = thumbnails.value(QLatin1String("small")).toString();
        DEBUG_ASSERT(!smallThumbnailUrl.isNull());
        const auto largeThumbnailUrl = thumbnails.value(QLatin1String("large")).toString();
        DEBUG_ASSERT(!largeThumbnailUrl.isNull());
        m_smallThumbnailUrls.insert(m_queuedAlbumReleaseIds.first(), smallThumbnailUrl);
        //m_allThumbnailUrls.append(largeThumbnailUrl);
        //m_allThumbnailUrls.append(smallThumbnailUrl);
        //m_coverArtUrls.insert(m_queuedAlbumReleaseIds.first(), m_allThumbnailUrls);
        break;
    }
    m_queuedAlbumReleaseIds.removeFirst();
    if (m_queuedAlbumReleaseIds.isEmpty()) {
        qDebug() << m_smallThumbnailUrls.size() << " Possible Thumbnail Links found.";
        //TODO: Emit all links
        emitSucceeded(m_smallThumbnailUrls);
        return;
    }
    DEBUG_ASSERT(!m_queuedAlbumReleaseIds.isEmpty());
    // Continue with next album release id
    slotStart(m_parentTimeoutMillis);
}

void CoverArtArchiveLinksTask::emitSucceeded(
        const QMap<QUuid, QString>& coverArtThumbnailUrls) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&CoverArtArchiveLinksTask::succeeded)) {
        kLogger.warning()
                << "Unhandled succeeded signal";
        deleteLater();
        return;
    }
    emit succeeded(coverArtThumbnailUrls);
}

} // namespace mixxx
