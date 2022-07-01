#pragma once

#include "util/parented_ptr.h"
#include "wfindonwebmenu.h"

class FindOnDiscogsMenu : public WFindOnWebMenu {
  public:
    FindOnDiscogsMenu(QMenu* pFindOnMenu, const Track& track);
};
