#include "findonwebmenudiscogs.h"

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

FindOnWebMenuDiscogs::FindOnWebMenuDiscogs(QMenu* pFindOnWebMenu, const Track& track) {
    const QString artist = track.getArtist();
    const QString trackTitle = track.getTitle();
    const QString album = track.getAlbum();
    auto pDiscogsMenu = make_parented<QMenu>(pFindOnWebMenu);
    pDiscogsMenu->setTitle(kServiceTitle);
    pFindOnWebMenu->addMenu(pDiscogsMenu);
    addSeparator();
    if (!artist.isEmpty()) {
        pDiscogsMenu->addAction(composeActionText(tr("Artist"), artist),
                this,
                [this, artist] {
                    openInBrowser(artist, kQueryTypeArtist, kSearchUrl);
                });
    }
    if (!trackTitle.isEmpty()) {
        if (!artist.isEmpty()) {
            const QString artistWithTrackTitle = composeSearchQuery(artist, trackTitle);
            pDiscogsMenu->addAction(composeActionText(tr("Artist + Title"), artistWithTrackTitle),
                    this,
                    [this, artistWithTrackTitle] {
                        openInBrowser(artistWithTrackTitle, kQueryTypeRelease, kSearchUrl);
                    });
        }
        pDiscogsMenu->addAction(composeActionText(tr("Title"), trackTitle),
                this,
                [this, trackTitle] {
                    openInBrowser(trackTitle, kQueryTypeRelease, kSearchUrl);
                });
    }
    if (!album.isEmpty()) {
        if (!artist.isEmpty()) {
            const QString artistWithAlbum = composeSearchQuery(artist, album);
            pDiscogsMenu->addAction(composeActionText(tr("Artist + Album"), artistWithAlbum),
                    this,
                    [this, artistWithAlbum] {
                        openInBrowser(artistWithAlbum, kSearchUrl);
                    });
        } else {
            pDiscogsMenu->addAction(composeActionText(tr("Album"), album),
                    this,
                    [this, album] {
                        openInBrowser(album, kSearchUrl);
                    });
        }
    }
}
