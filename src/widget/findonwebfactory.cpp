#include "findonwebfactory.h"

#include <QMenu>
#include <QtDebug>

#include "findondiscogs.h"
#include "findonlastfm.h"
#include "findonsoundcloud.h"
#include "findonweb.h"
#include "track/track.h"

void FindOnWebFactory::createServiceMenus(
        QMenu* menu, const Track& track) {
    if (FindOnWebFactory::serviceIsEnabled(FindOnWebFactory::Service::Soundcloud)) {
        new FindOnSoundcloud(menu, track);
    }
    if (FindOnWebFactory::serviceIsEnabled(FindOnWebFactory::Service::Discogs)) {
        new FindOnDiscogs(menu, track);
    }
    if (FindOnWebFactory::serviceIsEnabled(FindOnWebFactory::Service::Lastfm)) {
        new FindOnLastfm(menu, track);
    }
}

bool FindOnWebFactory::serviceIsEnabled(
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
