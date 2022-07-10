#include "musicbrainz/web/coverartarchivetask.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QMetaMethod>
#include <QNetworkRequest>

#include "musicbrainz/musicbrainzxml.h"
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

void CoverArtArchiveTask::doNetworkReplyFinished(
        QNetworkReply* finishedNetworkReply,
        network::HttpStatusCode statusCode) {
    DEBUG_ASSERT(finishedNetworkReply);
    kLogger.info() << "Networkreply finished. So We got:";
    kLogger.info() << statusCode;
    const QByteArray body = finishedNetworkReply->readAll();
    QJsonDocument theCoverArtResult = QJsonDocument::fromJson(body);

    // HTTP status codes:
    // 307: Found, redirect to an index.json file.
    // 400: MBID cannot be parsed as a valid UUID.
    // 404: there is no release with this MBID.
    // 405: The method is not one GET or HEAD.
    // 406: the server is unable to generate a response suitable to the Accept header.
    // 503: Rate limit error.

    if (statusCode == 200) {
        kLogger.info() << "GET Reply";
        kLogger.info() << "statuscode:" << statusCode;
        kLogger.info() << theCoverArtResult;
    }

    if (statusCode == 404) {
        kLogger.info()
                << "GET reply"
                << "statusCode:" << statusCode
                << "body:" << body;

        return;
    }
}

void CoverArtArchiveTask::emitSucceeded( //Dummy function, will be developed later.
        const QList<QUuid>& coverArtPaths) {
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
