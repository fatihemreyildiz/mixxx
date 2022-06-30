#pragma once

#include <QMenu>

#include "assert.h"
#include "track/track.h"

class FindOnWeb : public QMenu {
    Q_OBJECT
  public:
    explicit FindOnWeb(
            QWidget* parent = nullptr);
    ~FindOnWeb() override = default;

    static bool hasEntriesForTrack(const Track& track);

    virtual void addSubmenusForServices(QMenu* pFindOnMenu, const Track& track);

  protected:
    void openInBrowser(const QString& query, const QString& serviceUrl);

    void openInBrowser(const QString& query, const QString& queryType, const QString& serviceUrl);
};
