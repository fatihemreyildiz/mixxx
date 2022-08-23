#include "musicbrainz/web/coverartarchivethumbnailstask.h"

#include <QMetaMethod>

#include "defs_urls.h"
#include "network/httpstatuscode.h"
#include "util/assert.h"
#include "util/logger.h"
#include "util/thread_affinity.h"
#include "util/versionstore.h"

namespace mixxx {

namespace {

const Logger kLogger("CoverArtArchiveThumbnailsTask");

QNetworkRequest createNetworkRequest(const QString& smallThumbnailUrl) {
    QUrl url = smallThumbnailUrl;
    DEBUG_ASSERT(url.isValid());
    QNetworkRequest networkRequest(url);
    return networkRequest;
}

} // anonymous namespace

CoverArtArchiveThumbnailsTask::CoverArtArchiveThumbnailsTask(
        QNetworkAccessManager* networkAccessManager,
        const QMap<QUuid, QString>& smallThumbnailsUrls,
        QObject* parent)
        : network::WebTask(
                  networkAccessManager,
                  parent),
          m_queuedSmallThumbnailUrls(smallThumbnailsUrls),
          m_parentTimeoutMillis(0) {
}

QNetworkReply* CoverArtArchiveThumbnailsTask::doStartNetworkRequest(
        QNetworkAccessManager* networkAccessManager,
        int parentTimeoutMillis) {
    networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    DEBUG_ASSERT(networkAccessManager);

    m_parentTimeoutMillis = parentTimeoutMillis;

    VERIFY_OR_DEBUG_ASSERT(!m_queuedSmallThumbnailUrls.isEmpty()) {
        return nullptr;
    }

    const auto smallThumbnailUrl = m_queuedSmallThumbnailUrls.first();
    DEBUG_ASSERT(!smallThumbnailUrl.isNull());

    const QNetworkRequest networkRequest =
            createNetworkRequest(smallThumbnailUrl);

    if (kLogger.traceEnabled()) {
        kLogger.trace()
                << "GET"
                << networkRequest.url();
    }
    return networkAccessManager->get(networkRequest);
}

bool CoverArtArchiveThumbnailsTask::doNetworkReplyFinished(
        QNetworkReply* finishedNetworkReply,
        network::HttpStatusCode statusCode) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);

    const QByteArray resultImageBytes = finishedNetworkReply->readAll();

    if (statusCode != 200) {
        kLogger.info()
                << "GET reply"
                << "statusCode:" << statusCode;
        return false;
    }

    m_smallThumbnailBytes.insert(m_queuedSmallThumbnailUrls.firstKey(), resultImageBytes);
    m_queuedSmallThumbnailUrls.erase(m_queuedSmallThumbnailUrls.begin());
    if (m_queuedSmallThumbnailUrls.isEmpty()) {
        qDebug() << m_smallThumbnailBytes.size() << " Cover arts found.";
        emit succeeded(m_smallThumbnailBytes);
        return true;
    }
    slotStart(m_parentTimeoutMillis);
    return false;
}

void CoverArtArchiveThumbnailsTask::emitSuceeded(
        const QMap<QUuid, QByteArray>& m_smallThumbnailBytes) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&CoverArtArchiveThumbnailsTask::succeeded)) {
        kLogger.warning()
                << "Unhandled succeeded signal";
        deleteLater();
        return;
    }
    emit succeeded(m_smallThumbnailBytes);
}

void CoverArtArchiveThumbnailsTask::emitFailed(
        const network::WebResponse& response,
        int errorCode,
        const QString& errorMessage) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&CoverArtArchiveThumbnailsTask::failed)) {
        kLogger.warning()
                << "Unhandled failed signal"
                << response
                << errorCode
                << errorMessage;
        deleteLater();
        return;
    }
    emit failed(
            response,
            errorCode,
            errorMessage);
}

} // namespace mixxx
