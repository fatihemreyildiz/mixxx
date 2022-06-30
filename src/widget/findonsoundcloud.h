#pragma once

#include "findonweb.h"
#include "util/parented_ptr.h"

class FindOnSoundcloud : public FindOnWeb {
  public:
    FindOnSoundcloud(QMenu* pFindOnMenu, const Track& track, const QString& serviceTitle);
    //void addSubmenusForServices(QMenu* pFindOnMenu, const Track& track) override;
};
