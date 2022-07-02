#pragma once

#include <QMenu>

class Track;

class FindOnWebMenuFactory {
  public:
    static void createFindOnWebSubmenus(QMenu* pFindOnWebMenu,
            const Track& track,
            bool isSoundcloudEnabled,
            bool isDiscogsEnabled,
            bool isLastfmEnabled);
};
