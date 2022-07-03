#include "findonwebmenufactory.h"

#include <QMenu>

#include "findonwebmenuservices/findonwebmenudiscogs.h"
#include "findonwebmenuservices/findonwebmenulastfm.h"
#include "findonwebmenuservices/findonwebmenusoundcloud.h"

void FindOnWebMenuFactory::createFindOnWebSubmenus(QMenu* pFindOnWebMenu,
        const Track& track) {
    auto pFindOnWebMenuSoundcloud = make_parented<QMenu>(
            new FindOnWebMenuSoundcloud(pFindOnWebMenu, track));

    auto pFindOnWebMenuDiscogs = make_parented<QMenu>(
            new FindOnWebMenuDiscogs(pFindOnWebMenu, track));

    auto pFindOnWebMenuLastfm = make_parented<QMenu>(
            new FindOnWebMenuLastfm(pFindOnWebMenu, track));
}
