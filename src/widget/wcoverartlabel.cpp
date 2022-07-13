#include "widget/wcoverartlabel.h"

#include <QtDebug>

#include "library/coverartutils.h"
#include "library/dlgcoverartfullsize.h"
#include "moc_wcoverartlabel.cpp"
#include "track/track.h"
#include "widget/wcoverartmenu.h"

namespace {

constexpr QSize kDeviceIndependentCoverLabelSize = QSize(100, 100);

inline QPixmap scaleCoverLabel(
        QWidget* parent,
        QPixmap pixmap) {
    const auto devicePixelRatioF = parent->devicePixelRatioF();
    pixmap.setDevicePixelRatio(devicePixelRatioF);
    return pixmap.scaled(
            kDeviceIndependentCoverLabelSize * devicePixelRatioF,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
}

QPixmap createDefaultCover(QWidget* parent) {
    auto defaultCover = QPixmap(CoverArtUtils::defaultCoverLocation());
    return scaleCoverLabel(parent, defaultCover);
}

} // anonymous namespace

WCoverArtLabel::WCoverArtLabel(QWidget* parent)
        : CoverArtLabel(parent),
          m_pCoverMenu(make_parented<WCoverArtMenu>(this)),
          m_pDlgFullSize(make_parented<DlgCoverArtFullSize>(this)),
          m_defaultCover(createDefaultCover(this)) {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setFrameShape(QFrame::Box);
    setAlignment(Qt::AlignCenter);
    connect(m_pCoverMenu,
            &WCoverArtMenu::coverInfoSelected,
            this,
            &WCoverArtLabel::coverInfoSelected);
    connect(m_pCoverMenu, &WCoverArtMenu::reloadCoverArt, this, &WCoverArtLabel::reloadCoverArt);

    setPixmap(m_defaultCover);
}

WCoverArtLabel::~WCoverArtLabel() = default;

void WCoverArtLabel::slotCoverMenu(const QPoint& pos) {
    m_pCoverMenu->popup(mapToGlobal(pos));
}

void WCoverArtLabel::contextMenuEvent(QContextMenuEvent* event) {
    event->accept();
    m_pCoverMenu->popup(event->globalPos());
}
