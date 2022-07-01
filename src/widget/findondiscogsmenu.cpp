#include "findondiscogsmenu.h"

#include <QMenu>

#include "track/track.h"
#include "util/parented_ptr.h"

namespace {
const QString kServiceTitle = QStringLiteral("Discogs");

const QString kQueryTypeRelease = QStringLiteral("release");

const QString kQueryTypeArtist = QStringLiteral("artist");

const QString kSearchUrl = QStringLiteral(
        "https://www.discogs.com/search/?");
} //namespace

FindOnDiscogsMenu::FindOnDiscogsMenu(QMenu* pFindOnMenu, const Track& track) {
    const QString artist = track.getArtist();
    const QString trackTitle = track.getTitle();
    const QString album = track.getAlbum();
    auto pDiscogsMenu = make_parented<QMenu>(this);
    pDiscogsMenu->setTitle(kServiceTitle);
    pFindOnMenu->addMenu(pDiscogsMenu);
    addSeparator();
    if (!artist.isEmpty()) {
        pDiscogsMenu->addAction(composeActionText(tr("Artist"), artist),
                this,
                [this, artist] {
                    this->openInBrowser(artist, kQueryTypeArtist, kSearchUrl);
                });
    }
    if (!trackTitle.isEmpty()) {
        pDiscogsMenu->addAction(composeActionText(tr("Title"), trackTitle),
                this,
                [this, trackTitle] {
                    this->openInBrowser(trackTitle, kQueryTypeRelease, kSearchUrl);
                });
    }
    if (!album.isEmpty()) {
        pDiscogsMenu->addAction(composeActionText(tr("Album"), album),
                this,
                [this, album] {
                    this->openInBrowser(album, kSearchUrl);
                });
    }
}
