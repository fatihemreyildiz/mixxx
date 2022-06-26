#pragma once

#include <QAction>
#include <QMenu>
#include <QUrlQuery>

class Track;

class FindOnWeb : public QMenu { // Interface
    Q_OBJECT
  public:
    virtual void addSubmenusForServices(QMenu* pFindOnMenu, const Track& track);
};
