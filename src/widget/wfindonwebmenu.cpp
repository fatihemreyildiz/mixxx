#include "wfindonwebmenu.h"

#include <QDesktopServices>
#include <QMenu>
#include <QUrlQuery>
#include <QtDebug>

#include "util/parented_ptr.h"
#include "util/widgethelper.h"

WFindOnWebMenu::WFindOnWebMenu(QWidget* parent)
        : QMenu(tr("Find on Web"), parent) {
}

bool WFindOnWebMenu::hasEntriesForTrack(const Track& track) {
    return !(track.getArtist().isEmpty() &&
            track.getAlbum().isEmpty() &&
            track.getTitle().isEmpty());
}

QString WFindOnWebMenu::composeActionText(const QString& prefix, const QString& trackProperty) {
    return prefix + QStringLiteral(" | ") + trackProperty;
}

QString WFindOnWebMenu::composeSearchQuery(
        const QString& artist, const QString& trackAlbumOrTitle) {
    return artist + QStringLiteral(" ") + trackAlbumOrTitle;
}

void WFindOnWebMenu::openInBrowser(const QString& query, const QString& serviceUrl) {
    QUrlQuery urlQuery;
    urlQuery.addQueryItem("q", query);
    QUrl url(serviceUrl);
    url.setQuery(urlQuery);
    if (!QDesktopServices::openUrl(url)) {
        qWarning() << "QDesktopServices::openUrl() failed for " << url;
        DEBUG_ASSERT(false);
    }
}

void WFindOnWebMenu::openInBrowser(const QString& query,
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
