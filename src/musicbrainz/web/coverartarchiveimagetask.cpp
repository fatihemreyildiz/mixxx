#include "musicbrainz/web/coverartarchiveimagetask.h"

#include <QMetaMethod>

#include "defs_urls.h"
#include "network/httpstatuscode.h"
#include "util/assert.h"
#include "util/logger.h"
#include "util/thread_affinity.h"
#include "util/versionstore.h"

namespace mixxx {

namespace {

const Logger kLogger("CoverArtArchiveImageTask");

QNetworkRequest createNetworkRequest(const QUrl& thumbnailUrl) {
    DEBUG_ASSERT(thumbnailUrl.isValid());
    QNetworkRequest networkRequest(thumbnailUrl);
    return networkRequest;
}

} // anonymous namespace

CoverArtArchiveImageTask::CoverArtArchiveImageTask(
        QNetworkAccessManager* networkAccessManager,
        const QUrl& coverUrl,
        QObject* parent)
        : network::WebTask(
                  networkAccessManager,
                  parent),
          m_ThumbnailUrl(coverUrl) {
}

QNetworkReply* CoverArtArchiveImageTask::doStartNetworkRequest(
        QNetworkAccessManager* networkAccessManager,
        int parentTimeoutMillis) {
    networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    DEBUG_ASSERT(networkAccessManager);
    const QNetworkRequest networkRequest =
            createNetworkRequest(m_ThumbnailUrl);

    if (kLogger.traceEnabled()) {
        kLogger.trace()
                << "GET"
                << networkRequest.url();
    }
    return networkAccessManager->get(networkRequest);
}

void CoverArtArchiveImageTask::doNetworkReplyFinished(
        QNetworkReply* finishedNetworkReply,
        network::HttpStatusCode statusCode) {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);

    const QByteArray body = finishedNetworkReply->readAll();

    if (statusCode != 200) {
        kLogger.info()
                << "GET reply"
                << "statusCode:" << statusCode
                << "body:" << body;
        return;
    }

    qDebug() << "Second label cover art should have been updated.";
    emit succeeded(body);
}

void CoverArtArchiveImageTask::emitFailed(
        const network::WebResponse& response,
        int errorCode,
        const QString& errorMessage) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&CoverArtArchiveImageTask::failed)) {
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
