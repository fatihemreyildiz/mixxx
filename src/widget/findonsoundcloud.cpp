#include "findonsoundcloud.h"

#include <QMenu>

#include "track/track.h"
#include "util/parented_ptr.h"

namespace {
const QString kSearchUrlArtist = QStringLiteral("https://soundcloud.com/search/people?");

const QString kSearchUrlTitle = QStringLiteral("https://soundcloud.com/search/sounds?");

const QString kSearchUrlAlbum = QStringLiteral("https://soundcloud.com/search/albums?");
} // namespace

FindOnSoundcloud::FindOnSoundcloud(
        QMenu* pFindOnMenu, const Track& track, const QString& serviceTitle) {
    const QString artist = track.getArtist();
    const QString trackTitle = track.getTitle();
    const QString album = track.getAlbum();
    auto m_pServiceMenu = make_parented<QMenu>(this);
    m_pServiceMenu->setTitle(serviceTitle);
    pFindOnMenu->addMenu(m_pServiceMenu);
    m_pServiceMenu->addSeparator();
    if (!artist.isEmpty()) {
        m_pServiceMenu->addAction(artist,
                this,
                [this, artist] {
                    this->openInBrowser(artist, kSearchUrlArtist);
                });
    }
    if (!trackTitle.isEmpty()) {
        m_pServiceMenu->addAction(trackTitle,
                this,
                [this, trackTitle] {
                    this->openInBrowser(trackTitle, kSearchUrlTitle);
                });
    }
    if (!album.isEmpty()) {
        m_pServiceMenu->addAction(album,
                this,
                [this, album] {
                    this->openInBrowser(album, kSearchUrlAlbum);
                });
    }
}
