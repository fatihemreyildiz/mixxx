#include "findonsoundcloud.h"

#include <QDesktopServices>
#include <QMenu>
#include <QScreen>
#include <QUrl>
#include <QUrlQuery>
#include <QtDebug>

#include "track/track.h"
#include "util/parented_ptr.h"

namespace {
const QString kServiceTitle = QStringLiteral("Soundcloud");

const QString kSearchUrlArtist = QStringLiteral("https://soundcloud.com/search/people?");

const QString kSearchUrlTitle = QStringLiteral("https://soundcloud.com/search/sounds?");

const QString kSearchUrlAlbum = QStringLiteral("https://soundcloud.com/search/albums?");
} // namespace

void FindOnSoundcloud::addSubmenusForServices(QMenu* pFindOnMenu, const Track& track) {
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
