#pragma once

#include <QMenu>

class WFindOnWebMenu;
class Track;

class FindOnWebMenuFactory {
  public:
    enum Service {
        Discogs,
        Soundcloud,
        Lastfm
    };

    static void createFindOnWebSubmenus(QMenu* pFindOnWebMenu, const Track& track);

  private:
    static bool serviceIsEnabled(Service service);
};
