#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>
#include <QWidget>

#include "track/track_decl.h"
#include "util/parented_ptr.h"

class WCoverArtMenu;
class DlgCoverArtFullSize;
class DlgCoverArtFetchedFullSize;
class CoverInfo;
class CoverInfoRelative;

//This class is identical with the "WCoverArtLabel"
//It just doesn't have the cover art menu.
//In order to prevent duplication this became the base class
//of WCoverArtMenu, I don't know if it was the correct solution
//Only difference is cover art menu defined in the WCoverArtLabel.
//What do you think generally?

class CoverArtLabel : public QLabel {
    Q_OBJECT
  public:
    explicit CoverArtLabel(QWidget* parent = nullptr);
    ~CoverArtLabel() override; // Verifies that the base destructor is virtual

    void setCoverArt(const CoverInfo& coverInfo, const QPixmap& px);
    void loadTrack(TrackPointer pTrack);
    void loadData(const QByteArray& data);

  signals:
    void coverInfoSelected(const CoverInfoRelative& coverInfo);

  protected:
    void mousePressEvent(QMouseEvent* event) override;

  private:
    const parented_ptr<DlgCoverArtFullSize> m_pDlgFullSize;
    const parented_ptr<DlgCoverArtFetchedFullSize> m_pFetchedFullSize;

    const QPixmap m_defaultCover;

    QByteArray m_Data;

    TrackPointer m_pLoadedTrack;

    QPixmap m_loadedCover;
};
