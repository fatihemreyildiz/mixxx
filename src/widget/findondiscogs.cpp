#include "findondiscogs.h"

#include <QMenu>
#include <QtDebug>

#include "track/track.h"
#include "util/parented_ptr.h"

namespace {
const QString kServiceTitle = QStringLiteral("Discogs");

const QString kSearchUrl = QStringLiteral(
        "https://www.discogs.com/search/?"); // List of Query needed.
} //namespace

void FindOnDiscogs::addSubmenusForServices(QMenu* pFindOnMenu, const Track& track) {
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

void FindOnDiscogs::writeOnConsole() {
    qDebug() << "Discogs?";
}
