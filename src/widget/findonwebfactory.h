#pragma once

#include <QMenu>

class FindOnWeb;
class Track;

class FindOnWebFactory {
  public:
    enum Service {
        Discogs,
        Soundcloud,
        Lastfm
    };

    static void createServiceMenus(QMenu* menu, const Track& track);

  private:
    static bool serviceIsEnabled(Service service);
};
