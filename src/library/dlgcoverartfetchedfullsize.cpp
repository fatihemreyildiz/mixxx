#include "library/dlgcoverartfetchedfullsize.h"

#include <QRect>
#include <QScreen>
#include <QStyle>
#include <QWheelEvent>

#include "library/coverartutils.h"
#include "util/widgethelper.h"

DlgCoverArtFetchedFullSize::DlgCoverArtFetchedFullSize(QWidget* parent, BaseTrackPlayer* pPlayer)
        : QDialog(parent) {
    setupUi(this);
}

void DlgCoverArtFetchedFullSize::closeEvent(QCloseEvent* event) {
    if (parentWidget()) {
        hide();
        event->ignore();
    } else {
        QDialog::closeEvent(event);
    }
}

//This method is going to use the ByteArray of the fetched images.
//For now it will use the default cover's location.
void DlgCoverArtFetchedFullSize::init(const QByteArray& data) {
    //This is only for testing, will be deleted later.
    auto defaultCover = QPixmap(CoverArtUtils::defaultCoverLocation());

    QPixmap image;
    image.loadFromData(data);
    //It will pop with the w and h of the fetched cover art.
    resize(image.size().width(), image.size().height());
    show();
    raise();
    activateWindow();
    // TODO: This can be changed to release info found on the musicbrainz
    QString fetchedCoverArtWindowTitle = "Fetched Cover Art";

    setWindowTitle(fetchedCoverArtWindowTitle);

    coverArt->setPixmap(defaultCover);
}

void DlgCoverArtFetchedFullSize::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_clickTimer.setSingleShot(true);
        m_clickTimer.start(500);
        m_coverPressed = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QPoint eventPosition = event->globalPosition().toPoint();
#else
        QPoint eventPosition = event->globalPos();
#endif
        m_dragStartPosition = eventPosition - frameGeometry().topLeft();
    }
}

void DlgCoverArtFetchedFullSize::mouseReleaseEvent(QMouseEvent* event) {
    m_coverPressed = false;

    if (event->button() == Qt::LeftButton && isVisible()) {
        if (m_clickTimer.isActive()) {
            // short press
            close();
        } else {
            // long press
            return;
        }
        event->accept();
    }
}

// delete
void DlgCoverArtFetchedFullSize::mouseMoveEvent(QMouseEvent* event) {
    if (m_coverPressed) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QPoint eventPosition = event->globalPosition().toPoint();
#else
        QPoint eventPosition = event->globalPos();
#endif
        move(eventPosition - m_dragStartPosition);
        event->accept();
    } else {
        return;
    }
}
// delete
void DlgCoverArtFetchedFullSize::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event);
    if (m_pixmap.isNull()) {
        return;
    }
    QPixmap resizedPixmap = m_pixmap.scaled(size() * devicePixelRatioF(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
    resizedPixmap.setDevicePixelRatio(devicePixelRatioF());
    coverArt->setPixmap(resizedPixmap);
}
// delete
void DlgCoverArtFetchedFullSize::wheelEvent(QWheelEvent* event) {
    // Scale the image size
    int oldWidth = width();
    int oldHeight = height();
    auto newWidth = static_cast<int>(oldWidth + (0.2 * event->angleDelta().y()));
    auto newHeight = static_cast<int>(oldHeight + (0.2 * event->angleDelta().y()));
    QSize newSize = size();
    newSize.scale(newWidth, newHeight, Qt::KeepAspectRatio);

    QPoint oldOrigin = geometry().topLeft();
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QPoint oldPointUnderCursor = event->position().toPoint();
#else
    QPoint oldPointUnderCursor = event->pos();
#endif

    const auto newPointX = static_cast<int>(
            static_cast<double>(oldPointUnderCursor.x()) / oldWidth * newSize.width());
    const auto newPointY = static_cast<int>(
            static_cast<double>(oldPointUnderCursor.y()) / oldHeight * newSize.height());
    QPoint newOrigin = QPoint(
            oldOrigin.x() + (oldPointUnderCursor.x() - newPointX),
            oldOrigin.y() + (oldPointUnderCursor.y() - newPointY));

    setGeometry(QRect(newOrigin, newSize));

    event->accept();
}
