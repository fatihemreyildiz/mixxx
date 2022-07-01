#include "findonwebmenufactory.h"

#include <QMenu>
#include <QtDebug>

#include "findondiscogsmenu.h"
#include "findonlastfmmenu.h"
#include "findonsoundcloudmenu.h"
#include "track/track.h"
#include "wfindonwebmenu.h"

void FindOnWebMenuFactory::createFindOnWebSubmenus(
        QMenu* pFindOnWebMenu, const Track& track) {
    if (FindOnWebMenuFactory::serviceIsEnabled(FindOnWebMenuFactory::Service::Soundcloud)) {
        new FindOnSoundcloudMenu(pFindOnWebMenu, track);
    }
    if (FindOnWebMenuFactory::serviceIsEnabled(FindOnWebMenuFactory::Service::Discogs)) {
        new FindOnDiscogsMenu(pFindOnWebMenu, track);
    }
    if (FindOnWebMenuFactory::serviceIsEnabled(FindOnWebMenuFactory::Service::Lastfm)) {
        new FindOnLastfmMenu(pFindOnWebMenu, track);
    }
}

bool FindOnWebMenuFactory::serviceIsEnabled(
        Service service) { // TODO: Add feature that can be turned on-off on preferences.
    switch (service) {
    case Service::Soundcloud:
        return true;
    case Service::Discogs:
        return true;
    case Service::Lastfm:
        return true;
    default:
        return false;
    }
}
