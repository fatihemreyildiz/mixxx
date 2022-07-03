#pragma once

#include "util/parented_ptr.h"
#include "widget/wfindonwebmenu.h"

class FindOnDiscogsMenu : public WFindOnWebMenu {
  public:
    FindOnDiscogsMenu(QMenu* pFindOnMenu, const Track& track);
};
