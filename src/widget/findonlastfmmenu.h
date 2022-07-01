#pragma once

#include "util/parented_ptr.h"
#include "wfindonwebmenu.h"

class FindOnLastfmMenu : public WFindOnWebMenu {
  public:
    FindOnLastfmMenu(QMenu* pFindOnMenu, const Track& track);
};
