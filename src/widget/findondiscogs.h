#pragma once

#include "findonweb.h"
#include "util/parented_ptr.h"

class FindOnDiscogs : public FindOnWeb {
  public:
    FindOnDiscogs(QMenu* pFindOnMenu, const Track& track);
};
