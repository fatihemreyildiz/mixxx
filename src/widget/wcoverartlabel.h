#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QWidget>

#include "library/coverartlabel.h"
#include "track/track_decl.h"
#include "util/parented_ptr.h"

class WCoverArtMenu;
class DlgCoverArtFullSize;
class CoverInfo;
class CoverInfoRelative;

class WCoverArtLabel : public CoverArtLabel {
    Q_OBJECT
  public:
    explicit WCoverArtLabel(QWidget* parent = nullptr);
    ~WCoverArtLabel() override; // Verifies that the base destructor is virtual

  signals:
    void reloadCoverArt();

  protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

  private slots:
      void slotCoverMenu(const QPoint& pos);

  private:
    const parented_ptr<WCoverArtMenu> m_pCoverMenu;
    const parented_ptr<DlgCoverArtFullSize> m_pDlgFullSize;

    const QPixmap m_defaultCover;
};
