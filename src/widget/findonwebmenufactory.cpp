#include "findonwebmenufactory.h"

#include <QMenu>

#include "findondiscogsmenu.h"
#include "findonlastfmmenu.h"
#include "findonsoundcloudmenu.h"

void FindOnWebMenuFactory::createFindOnWebSubmenus(QMenu* pFindOnWebMenu,
        const Track& track) {
    auto pSoundCloudMenu = make_parented<QMenu>(
            new FindOnSoundcloudMenu(pFindOnWebMenu, track));

    auto pFindOnDiscogsMenu = make_parented<QMenu>(
            new FindOnDiscogsMenu(pFindOnWebMenu, track));

    auto pFindOnLastFmMenu = make_parented<QMenu>(new FindOnLastfmMenu(pFindOnWebMenu, track));
}
