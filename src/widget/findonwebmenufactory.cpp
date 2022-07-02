#include "findonwebmenufactory.h"

#include <QMenu>
#include <QtDebug>

#include "findondiscogsmenu.h"
#include "findonlastfmmenu.h"
#include "findonsoundcloudmenu.h"
#include "track/track.h"
#include "wfindonwebmenu.h"

void FindOnWebMenuFactory::createFindOnWebSubmenus(QMenu* pFindOnWebMenu,
        const Track& track,
        bool isSoundcloudEnabled,
        bool isDiscogsEnabled,
        bool isLastfmEnabled) {
    if (isSoundcloudEnabled) {
        auto pSoundCloudMenu = make_parented<QMenu>(
                new FindOnSoundcloudMenu(pFindOnWebMenu, track));
    }
    if (isDiscogsEnabled) {
        auto pFindOnDiscogsMenu = make_parented<QMenu>(
                new FindOnDiscogsMenu(pFindOnWebMenu, track));
    }
    if (isLastfmEnabled) {
        auto pFindOnLastFmMenu = make_parented<QMenu>(new FindOnLastfmMenu(pFindOnWebMenu, track));
    }
}
