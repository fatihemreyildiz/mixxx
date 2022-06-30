#include "findonwebfactory.h"

#include <QMenu>
#include <QtDebug>

#include "findondiscogs.h"
#include "findonlastfm.h"
#include "findonsoundcloud.h"
#include "findonweb.h"
#include "track/track.h"

namespace {
const QString kSoundcloud = QStringLiteral("Soundcloud");
}

void FindOnWebFactory::createServiceMenus(
        QMenu* menu, const Track& track) { // TODO: Call More Service Menus
    if (FindOnWebFactory::serviceIsEnabled(FindOnWebFactory::Service::Soundcloud)) {
        new FindOnSoundcloud(menu, track, kSoundcloud); // ¿Should we use this?
        // For this one we need to define every service menu as a constructor
    }
    if (FindOnWebFactory::serviceIsEnabled(FindOnWebFactory::Service::Discogs)) {
        FindOnWeb* pFindOnWeb = new FindOnDiscogs();
        pFindOnWeb->addSubmenusForServices(menu, track); // ¿Or this?
        // With this one we can simply create services on without define for every service.
    }
    if (FindOnWebFactory::serviceIsEnabled(FindOnWebFactory::Service::Lastfm)) {
        FindOnWeb* pFindOnWeb = new FindOnLastfm();
        pFindOnWeb->addSubmenusForServices(menu, track);
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
