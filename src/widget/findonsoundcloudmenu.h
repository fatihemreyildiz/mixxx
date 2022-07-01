#pragma once

#include "util/parented_ptr.h"
#include "wfindonwebmenu.h"

class FindOnSoundcloudMenu : public WFindOnWebMenu {
  public:
    FindOnSoundcloudMenu(QMenu* pFindOnMenu, const Track& track);
};
