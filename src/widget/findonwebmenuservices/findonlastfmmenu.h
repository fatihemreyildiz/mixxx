#pragma once

#include "util/parented_ptr.h"
#include "widget/wfindonwebmenu.h"

class FindOnLastfmMenu : public WFindOnWebMenu {
  public:
    FindOnLastfmMenu(QMenu* pFindOnMenu, const Track& track);
};
