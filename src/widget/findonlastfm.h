#pragma once

#include "findonweb.h"
#include "util/parented_ptr.h"

class FindOnLastfm : public FindOnWeb {
  public:
    FindOnLastfm(QMenu* pFindOnMenu, const Track& track);
};
