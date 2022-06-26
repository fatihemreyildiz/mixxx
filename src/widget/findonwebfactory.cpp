#include "findonwebfactory.h"

#include <QMenu>
#include <QtDebug>

#include "findondiscogs.h"
#include "findonsoundcloud.h"
#include "findonweb.h"
#include "track/track.h"

FindOnWebFactory::FindOnWebFactory(
        QWidget* parent)
        : QMenu(tr("Find on Web"), parent) {
}

void FindOnWebFactory::createServiceMenus(QMenu* menu, const Track& track) { // Call Service Menus
    if (FindOnWebFactory::serviceIsEnabled(Service::Soundcloud)) {
        FindOnWeb* pFindOnWeb = new FindOnSoundcloud();

        pFindOnWeb->addSubmenusForServices(menu, track);
    }
    if (FindOnWebFactory::serviceIsEnabled(Service::Discogs)) {
        FindOnWeb* pFindOnWeb = new FindOnDiscogs();

        pFindOnWeb->addSubmenusForServices(menu, track);
    }
}

void FindOnWebFactory::openInBrowser(const QString& query) { // Implement OpenURL for all services
}

bool FindOnWebFactory::serviceIsEnabled(Service service)
        const { //Later, add feature that can be turned on-off on preferences.
    switch (service) {
    case Service::Soundcloud:
        return true;
    case Service::Discogs:
        return true;
    default:
        return false;
    }
}
