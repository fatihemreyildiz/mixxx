#include "network/networktask.h"

#include "moc_networktask.cpp"
#include "util/counter.h"
#include "util/logger.h"
#include "util/thread_affinity.h"

namespace mixxx {

namespace network {

namespace {

const Logger kLogger("mixxx::network::NetworkTask");

// count = even number (ctor + dtor)
// sum = 0 (no memory leaks)
Counter s_instanceCounter(QStringLiteral("mixxx::network::NetworkTask"));

} // anonymous namespace

NetworkTask::NetworkTask(
        QNetworkAccessManager* networkAccessManager,
        QObject* parent)
        : QObject(parent),
          m_networkAccessManagerWeakPtr(networkAccessManager) {
    s_instanceCounter.increment(1);
}

NetworkTask::~NetworkTask() {
    s_instanceCounter.increment(-1);
}

void NetworkTask::invokeStart(int timeoutMillis) {
    QMetaObject::invokeMethod(
            this,
            [this, timeoutMillis] {
                this->slotStart(timeoutMillis);
            });
}

void NetworkTask::invokeAbort() {
    QMetaObject::invokeMethod(
            this,
            [this] {
                this->slotAbort();
            });
}

void NetworkTask::abort() {
    DEBUG_ASSERT_QOBJECT_THREAD_AFFINITY(this);
    slotAbort();
}

void NetworkTask::emitAborted(
        const QUrl& requestUrl) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&NetworkTask::aborted)) {
        kLogger.warning()
                << this
                << "Unhandled abort signal"
                << requestUrl;
        deleteLater();
        return;
    }
    emit aborted(requestUrl);
}

void NetworkTask::emitNotFound(QNetworkReply::NetworkError errorCode,
        const QString& errorString,
        const QUrl& requestUrl) {
    VERIFY_OR_DEBUG_ASSERT(
            isSignalFuncConnected(&NetworkTask::notFound)) {
        kLogger.warning()
                << this
                << errorString
                << "Unhandled not found signal"
                << errorCode
                << requestUrl;
        deleteLater();
        return;
    }
    qDebug() << errorString;
    qDebug() << errorCode;
    emit notFound(errorCode, errorString, requestUrl);
}

} // namespace network

} // namespace mixxx
