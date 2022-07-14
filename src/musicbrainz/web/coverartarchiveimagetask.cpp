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

const QUrl kBaseUrl = QStringLiteral(
        "http://coverartarchive.org/release/"
        "f79a29d3-70a3-47ed-a2c4-8779d32411b1/26136610492-250.jpg");

const QString kRequestPath = QStringLiteral("/ws/2/recording/");

QNetworkRequest createNetworkRequest() {
    DEBUG_ASSERT(kBaseUrl.isValid());
    QUrl url = kBaseUrl;
    DEBUG_ASSERT(url.isValid());
    QNetworkRequest networkRequest(url);
    return networkRequest;
}

} // anonymous namespace

CoverArtArchiveImageTask::CoverArtArchiveImageTask(
        QNetworkAccessManager* networkAccessManager,
        const QString& coverUrl,
        QObject* parent)
        : network::WebTask(
                  networkAccessManager,
                  parent) {
    qDebug() << "CoverArtArchiveImageTask::CoverArtArchiveImageTask";
}

QNetworkReply* CoverArtArchiveImageTask::doStartNetworkRequest(
        QNetworkAccessManager* networkAccessManager,
        int parentTimeoutMillis) {
    networkAccessManager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    DEBUG_ASSERT(networkAccessManager);
    qDebug() << "CoverArtArchiveImageTask::doStartNetworkRequest";

    const QNetworkRequest networkRequest =
            createNetworkRequest();

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
    qDebug() << body;
    qDebug() << "This is the ByteArray of the image. This can be used to present the cover-art";
}

void CoverArtArchiveImageTask::emitSucceeded(
        const QByteArray& imageInfo) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&CoverArtArchiveImageTask::succeeded)) {
        kLogger.warning()
                << "Unhandled succeeded signal";
        deleteLater();
        return;
    }
    emit succeeded(imageInfo);
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
