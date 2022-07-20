#pragma once

#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPixmap>
#include <QWidget>

#include "track/track_decl.h"
#include "util/parented_ptr.h"

class WCoverArtMenu;
class DlgCoverArtFullSize;
class CoverInfo;
class CoverInfoRelative;

class WCoverArtLabel : public QLabel {
    Q_OBJECT
  public:
    explicit WCoverArtLabel(QWidget* parent = nullptr, QMenu* m_pWCoverArtMenu = nullptr);

    ~WCoverArtLabel() override; // Verifies that the base destructor is virtual

    void setCoverArt(const CoverInfo& coverInfo, const QPixmap& px);
    void loadTrack(TrackPointer pTrack);

  protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

  private slots:
      void slotCoverMenu(const QPoint& pos);

  private:
    QMenu* m_pCoverArtMenu;

    const parented_ptr<DlgCoverArtFullSize> m_pDlgFullSize;

    const QPixmap m_defaultCover;

    TrackPointer m_pLoadedTrack;

    QPixmap m_loadedCover;
};
