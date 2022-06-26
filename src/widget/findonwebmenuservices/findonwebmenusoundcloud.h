#pragma once

#include "util/parented_ptr.h"
#include "widget/wfindonwebmenu.h"

class FindOnWebMenuSoundcloud : public WFindOnWebMenu {
  public:
    FindOnWebMenuSoundcloud(QMenu* pFindOnWebMenu, const Track& track);
};
