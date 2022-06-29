#pragma once

#include <QMenu>

class Track;

class FindOnWeb { // : public QMenu // Q_OBJECT // Interface
  public:
    virtual void addSubmenusForServices(QMenu* pFindOnMenu, const Track& track);

    virtual void writeOnConsole();

    //signals:
    //  bool triggerBrowser();
};
