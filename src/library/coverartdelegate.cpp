#include "library/coverartdelegate.h"
#include "library/coverartcache.h"
#include "library/dao/trackdao.h"

CoverArtDelegate::CoverArtDelegate(QObject *parent)
        : QStyledItemDelegate(parent),
          m_bOnlyCachedCover(false),
          m_iCoverSourceColumn(-1),
          m_iCoverTypeColumn(-1),
          m_iCoverLocationColumn(-1),
          m_iCoverHashColumn(-1),
          m_iTrackLocationColumn(-1),
          m_iIdColumn(-1) {
    // This assumes that the parent is wtracktableview
    connect(parent, SIGNAL(onlyCachedCoverArt(bool)),
            this, SLOT(slotOnlyCachedCoverArt(bool)));

    TrackModel* pTrackModel = NULL;
    QTableView* pTableView = NULL;
    if (QTableView *tableView = qobject_cast<QTableView*>(parent)) {
        pTableView = tableView;
        pTrackModel = dynamic_cast<TrackModel*>(pTableView->model());
    }

    if (pTrackModel) {
        m_iCoverSourceColumn = pTrackModel->fieldIndex(
            LIBRARYTABLE_COVERART_SOURCE);
        m_iCoverTypeColumn = pTrackModel->fieldIndex(
            LIBRARYTABLE_COVERART_TYPE);
        m_iCoverHashColumn = pTrackModel->fieldIndex(
            LIBRARYTABLE_COVERART_HASH);
        m_iCoverLocationColumn = pTrackModel->fieldIndex(
            LIBRARYTABLE_COVERART_LOCATION);
        m_iTrackLocationColumn = pTrackModel->fieldIndex(
            TRACKLOCATIONSTABLE_LOCATION);
        m_iIdColumn = pTrackModel->fieldIndex(
            LIBRARYTABLE_ID);

        int coverColumn = pTrackModel->fieldIndex(LIBRARYTABLE_COVERART);
        pTableView->setColumnWidth(coverColumn, 100);
    }
}

CoverArtDelegate::~CoverArtDelegate() {
}

void CoverArtDelegate::slotOnlyCachedCoverArt(bool b) {
    m_bOnlyCachedCover = b;
}

void CoverArtDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const {
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }

    CoverArtCache* pCache = CoverArtCache::instance();
    if (pCache == NULL || m_iIdColumn == -1 || m_iCoverSourceColumn == -1 ||
            m_iCoverTypeColumn == -1 || m_iCoverLocationColumn == -1 ||
            m_iCoverHashColumn == -1) {
        return;
    }

    CoverInfo info;
    info.type = static_cast<CoverInfo::Type>(
        index.sibling(index.row(), m_iCoverTypeColumn).data().toInt());

    // We don't support types other than METADATA or FILE currently.
    if (info.type != CoverInfo::METADATA && info.type != CoverInfo::FILE) {
        return;
    }

    info.source = static_cast<CoverInfo::Source>(
        index.sibling(index.row(), m_iCoverSourceColumn).data().toInt());
    info.coverLocation = index.sibling(index.row(), m_iCoverLocationColumn).data().toString();
    info.hash = index.sibling(index.row(), m_iCoverHashColumn).data().toString();
    info.trackId = index.sibling(index.row(), m_iIdColumn).data().toInt();
    info.trackLocation = index.sibling(index.row(), m_iTrackLocationColumn).data().toString();

    QSize coverSize(100, option.rect.height());

    // Do not signal when done sine we don't listen to CoverArtCache for updates
    // and instead refresh on a timer in WTrackTableView.
    QPixmap pixmap = pCache->requestCover(info, coverSize,
                                          m_bOnlyCachedCover, false);


    if (!pixmap.isNull()) {
        int width = pixmap.width();
        if (option.rect.width() < width) {
            width = option.rect.width();
        }

        QRect target(option.rect.x(), option.rect.y(),
                     width, option.rect.height());
        QRect source(0, 0, width, pixmap.height());
        painter->drawPixmap(target, pixmap, source);
    }
}
