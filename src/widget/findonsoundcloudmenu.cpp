#include "findonsoundcloudmenu.h"

#include <QMenu>

#include "track/track.h"
#include "util/parented_ptr.h"

namespace {
const QString kServiceTitle = QStringLiteral("Soundcloud");

const QString kSearchUrlArtist = QStringLiteral("https://soundcloud.com/search/people?");

const QString kSearchUrlTitle = QStringLiteral("https://soundcloud.com/search/sounds?");

const QString kSearchUrlAlbum = QStringLiteral("https://soundcloud.com/search/albums?");
} // namespace

FindOnSoundcloudMenu::FindOnSoundcloudMenu(
        QMenu* pFindOnMenu, const Track& track) {
    const QString artist = track.getArtist();
    const QString trackTitle = track.getTitle();
    const QString album = track.getAlbum();
    auto pSoundcloudMenu = make_parented<QMenu>(pFindOnMenu);
    pSoundcloudMenu->setTitle(kServiceTitle);
    pFindOnMenu->addMenu(pSoundcloudMenu);
    pSoundcloudMenu->addSeparator();
    if (!artist.isEmpty()) {
        pSoundcloudMenu->addAction(composeActionText(tr("Artist"), artist),
                this,
                [this, artist] {
                    this->openInBrowser(artist, kSearchUrlArtist);
                });
    }
    if (!trackTitle.isEmpty()) {
        if (!artist.isEmpty()) {
            const QString artistWithTrackTitle = composeSearchQuery(artist, trackTitle);
            pSoundcloudMenu->addAction(composeActionText(tr("Artist + Title"),
                                               artistWithTrackTitle),
                    this,
                    [this, artistWithTrackTitle] {
                        this->openInBrowser(artistWithTrackTitle, kSearchUrlTitle);
                    });
        }
        pSoundcloudMenu->addAction(composeActionText(tr("Title"), trackTitle),
                this,
                [this, trackTitle] {
                    this->openInBrowser(trackTitle, kSearchUrlTitle);
                });
    }
    if (!album.isEmpty()) {
        if (!artist.isEmpty()) {
            const QString artistWithAlbum = composeSearchQuery(artist, album);
            pSoundcloudMenu->addAction(composeActionText(tr("Artist + Album"), artistWithAlbum),
                    this,
                    [this, artistWithAlbum] {
                        this->openInBrowser(artistWithAlbum, kSearchUrlAlbum);
                    });
        } else {
            pSoundcloudMenu->addAction(composeActionText(tr("Album"), album),
                    this,
                    [this, album] {
                        this->openInBrowser(album, kSearchUrlAlbum);
                    });
        }
    }
}
