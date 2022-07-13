#pragma once

#include <QDialog>
#include <QPoint>
#include <QTimer>

#include "library/ui_dlgcoverartfullsize.h"
#include "mixer/basetrackplayer.h"
#include "util/parented_ptr.h"

//This class is going to use the ByteArray of the fetched images.
//It is almost identical with the "DLGCoverArtFullSize"
//Except with the wcoverartmenu, for now right click doesn't work.
//This class can have its own menu Such as;
//Set this cover art.
//Download this cover art.
//I have derived this menu from the DlgCoverArtFullSize
//It seems like it didn't work in the first place, because this dlg
//shouldn't have the coverartmenu and other one should so, I don't know.

class DlgCoverArtFetchedFullSize
        : public QDialog,
          public Ui::DlgCoverArtFullSize {
    Q_OBJECT
  public:
    explicit DlgCoverArtFetchedFullSize(QWidget* parent, BaseTrackPlayer* pPlayer = nullptr);
    ~DlgCoverArtFetchedFullSize() override = default;

    void init(const QByteArray& data); //override
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

  private:
    QPixmap m_pixmap;
    QTimer m_clickTimer;
    QPoint m_dragStartPosition;
    bool m_coverPressed;
};
