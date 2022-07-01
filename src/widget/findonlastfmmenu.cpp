#include "findonlastfmmenu.h"

#include <QMenu>

#include "track/track.h"
#include "util/parented_ptr.h"

namespace {
const QString kServiceTitle = QStringLiteral("LastFm");

const QString kSearchUrlArtist = QStringLiteral("https://www.last.fm/search/artists?");

const QString kSearchUrlTitle = QStringLiteral("https://www.last.fm/search/tracks?");

const QString kSearchUrlAlbum = QStringLiteral("https://www.last.fm/search/albums?");
} //namespace

FindOnLastfmMenu::FindOnLastfmMenu(QMenu* pFindOnMenu, const Track& track) {
    const QString artist = track.getArtist();
    const QString trackTitle = track.getTitle();
    const QString album = track.getAlbum();
    auto pLastfmMenu = make_parented<QMenu>(pFindOnMenu);
    pLastfmMenu->setTitle(kServiceTitle);
    pFindOnMenu->addMenu(pLastfmMenu);
    pLastfmMenu->addSeparator();
    if (!artist.isEmpty()) {
        pLastfmMenu->addAction(composeActionText(tr("Artist"), artist),
                this,
                [this, artist] {
                    this->openInBrowser(artist, kSearchUrlArtist);
                });
    }
    if (!trackTitle.isEmpty()) {
        pLastfmMenu->addAction(composeActionText(tr("Title"), trackTitle),
                this,
                [this, trackTitle] {
                    this->openInBrowser(trackTitle, kSearchUrlTitle);
                });
    }
    if (!album.isEmpty()) {
        pLastfmMenu->addAction(composeActionText(tr("Album"), album),
                this,
                [this, album] {
                    this->openInBrowser(album, kSearchUrlAlbum);
                });
    }
}
