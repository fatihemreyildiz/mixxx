#include "library/coverartlabel.h"

#include <QtDebug>

#include "library/coverartutils.h"
#include "library/dlgcoverartfetchedfullsize.h"
#include "library/dlgcoverartfullsize.h"
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

CoverArtLabel::CoverArtLabel(QWidget* parent)
        : QLabel(parent),
          m_pDlgFullSize(make_parented<DlgCoverArtFullSize>(this)),
          m_pFetchedFullSize(make_parented<DlgCoverArtFetchedFullSize>(this)),
          m_defaultCover(createDefaultCover(this)) {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    setFrameShape(QFrame::Box);
    setAlignment(Qt::AlignCenter);

    setPixmap(m_defaultCover);
}

CoverArtLabel::~CoverArtLabel() = default;

void CoverArtLabel::setCoverArt(const CoverInfo& coverInfo,
        const QPixmap& px) {
    if (px.isNull()) {
        m_loadedCover = px;
        setPixmap(m_defaultCover);
    } else {
        m_loadedCover = scaleCoverLabel(this, px);
        setPixmap(m_loadedCover);
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    QSize frameSize = pixmap(Qt::ReturnByValue).size() / devicePixelRatioF();
#else
    QSize frameSize = pixmap()->size() / devicePixelRatioF();
#endif
    frameSize += QSize(2, 2); // margin
    setMinimumSize(frameSize);
    setMaximumSize(frameSize);
}

void CoverArtLabel::loadTrack(TrackPointer pTrack) {
    m_pLoadedTrack = pTrack;
}

void CoverArtLabel::loadData(const QByteArray& data) {
    m_Data = data;
}

void CoverArtLabel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (m_pDlgFullSize->isVisible()) {
            m_pDlgFullSize->close();
        } else {
            if (!m_pLoadedTrack) // && !m_Data.isNull(), will be added after the cover art fetched
            {
                m_pFetchedFullSize->init(m_Data);
            } else {
                m_pDlgFullSize->init(m_pLoadedTrack);
            }
        }
    }
}
