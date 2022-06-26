#include "findonweb.h"

#include <QtDebug>

#include "track/track.h"
#include "util/parented_ptr.h"
#include "wtrackmenu.h"

namespace {
const QString kServiceTitle = QStringLiteral("Find On Web");
} //namespace

void FindOnWeb::addSubmenusForServices(QMenu* pFindOnMenu, const Track& track) {
    const auto artist = track.getArtist();
    const auto trackTitle = track.getTitle();
    const auto album = track.getAlbum();
    auto m_pServiceMenu = make_parented<QMenu>(this);
    m_pServiceMenu->setTitle(kServiceTitle);
    pFindOnMenu->addMenu(m_pServiceMenu);
    addSeparator();
    if (!artist.isEmpty()) {
        m_pServiceMenu->addAction(artist);
    }
    if (!trackTitle.isEmpty()) {
        m_pServiceMenu->addAction(trackTitle);
    }
    if (!album.isEmpty()) {
        m_pServiceMenu->addAction(album);
    }
}
