#include "findonweb.h"

#include <QDesktopServices>
#include <QMenu>
#include <QUrlQuery>
#include <QtDebug>

#include "util/parented_ptr.h"
#include "util/widgethelper.h" // ¿I have included this because I wanted to use DEBUG_ASSERT, If it is not correct I will delete this line in the next commit?

FindOnWeb::FindOnWeb(QWidget* parent)
        : QMenu(tr("Find on Web"), parent) {
}

bool FindOnWeb::hasEntriesForTrack(const Track& track) {
    return !(track.getArtist().isEmpty() &&
            track.getAlbum().isEmpty() &&
            track.getTitle().isEmpty());
}

QString FindOnWeb::composeActionText(const QString& prefix, const QString& trackProperty) {
    return prefix + QStringLiteral(" | ") + trackProperty;
}

void FindOnWeb::openInBrowser(const QString& query, const QString& serviceUrl) {
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    QUrl url(serviceUrl);
    url.setQuery(urlQuery);
    if (!QDesktopServices::openUrl(url)) {
        qWarning() << "QDesktopServices::openUrl() failed for " << url;
        DEBUG_ASSERT(false);
    }
}

void FindOnWeb::openInBrowser(const QString& query,
        const QString& queryType,
        const QString& serviceUrl) {
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    urlQuery.addQueryItem("type", queryType);
    QUrl url(serviceUrl);
    url.setQuery(urlQuery);
    if (!QDesktopServices::openUrl(url)) {
        qWarning() << "QDesktopServices::openUrl() failed for " << url;
        DEBUG_ASSERT(false);
    }
}
