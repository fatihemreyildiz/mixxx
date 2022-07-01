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
        new FindOnSoundcloudMenu(pFindOnWebMenu, track);
    }
    if (isDiscogsEnabled) {
        new FindOnDiscogsMenu(pFindOnWebMenu, track);
    }
    if (isLastfmEnabled) {
        new FindOnLastfmMenu(pFindOnWebMenu, track);
    }
}
