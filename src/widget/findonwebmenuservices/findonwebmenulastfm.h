#pragma once

#include "util/parented_ptr.h"
#include "widget/wfindonwebmenu.h"

class FindOnWebMenuLastfm : public WFindOnWebMenu {
  public:
    FindOnWebMenuLastfm(QMenu* pFindOnWebMenu, const Track& track);
};
