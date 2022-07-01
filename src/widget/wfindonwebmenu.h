#pragma once

#include <QMenu>

#include "assert.h"
#include "track/track.h"

class WFindOnWebMenu : public QMenu {
    Q_OBJECT
  public:
    explicit WFindOnWebMenu(
            QWidget* parent = nullptr);
    ~WFindOnWebMenu() override = default;

    static bool hasEntriesForTrack(const Track& track);

  protected:
    QString composeActionText(const QString& prefix, const QString& trackProperty);

    void openInBrowser(const QString& query, const QString& serviceUrl);

    void openInBrowser(const QString& query, const QString& queryType, const QString& serviceUrl);
};
