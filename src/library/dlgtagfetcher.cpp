#include "library/dlgtagfetcher.h"

#include <QTreeWidget>
#include <QtDebug>

#include "defs_urls.h"
#include "library/coverartcache.h"
#include "library/coverartutils.h"
#include "library/library_prefs.h"
#include "moc_dlgtagfetcher.cpp"
#include "preferences/dialog/dlgpreflibrary.h"
#include "track/track.h"
#include "track/tracknumbers.h"

namespace {

QStringList trackColumnValues(
        const Track& track) {
    const mixxx::TrackMetadata trackMetadata = track.getMetadata();
    const QString trackNumberAndTotal = TrackNumbers::joinAsString(
            trackMetadata.getTrackInfo().getTrackNumber(),
            trackMetadata.getTrackInfo().getTrackTotal());
    QStringList columnValues;
    columnValues.reserve(6);
    columnValues
            << trackMetadata.getTrackInfo().getTitle()
            << trackMetadata.getTrackInfo().getArtist()
            << trackMetadata.getAlbumInfo().getTitle()
            << trackMetadata.getTrackInfo().getYear()
            << trackNumberAndTotal
            << trackMetadata.getAlbumInfo().getArtist();
    return columnValues;
}

QStringList trackReleaseColumnValues(
        const mixxx::musicbrainz::TrackRelease& trackRelease) {
    const QString trackNumberAndTotal = TrackNumbers::joinAsString(
            trackRelease.trackNumber,
            trackRelease.trackTotal);
    QStringList columnValues;
    columnValues.reserve(6);
    columnValues
            << trackRelease.title
            << trackRelease.artist
            << trackRelease.albumTitle
            << trackRelease.date
            << trackNumberAndTotal
            << trackRelease.albumArtist;
    return columnValues;
}

void addTrack(
        const QStringList& trackRow,
        int resultIndex,
        QTreeWidget* parent) {
    QTreeWidgetItem* item = new QTreeWidgetItem(parent, trackRow);
    item->setData(0, Qt::UserRole, resultIndex);
    item->setData(0, Qt::TextAlignmentRole, Qt::AlignLeft);
}

} // anonymous namespace

DlgTagFetcher::DlgTagFetcher(UserSettingsPointer pConfig, const TrackModel* pTrackModel)
        // No parent because otherwise it inherits the style parent's
        // style which can make it unreadable. Bug #673411
        : QDialog(nullptr),
          m_pConfig(pConfig),
          m_pTrackModel(pTrackModel),
          m_tagFetcher(this),
          m_pWCoverArtMenu(make_parented<WCoverArtMenu>(this)),
          m_pWCurrentCoverArtLabel(make_parented<WCoverArtLabel>(this)),
          m_pWFetchedCoverArtLabel(make_parented<WCoverArtLabel>(this)),
          m_networkResult(NetworkResult::Ok) {
    init();
}

void DlgTagFetcher::init() {
    setupUi(this);
    setWindowIcon(QIcon(MIXXX_ICON_PATH));

    currentCoverArtLayout->setAlignment(Qt::AlignRight | Qt::AlignTop | Qt::AlignCenter);
    currentCoverArtLayout->setSpacing(0);
    currentCoverArtLayout->setContentsMargins(0, 0, 0, 0);
    currentCoverArtLayout->insertWidget(0, m_pWCurrentCoverArtLabel.get());

    fetchedCoverArtLayout->setAlignment(Qt::AlignRight | Qt::AlignBottom | Qt::AlignCenter);
    fetchedCoverArtLayout->setSpacing(0);
    fetchedCoverArtLayout->setContentsMargins(0, 0, 0, 0);
    fetchedCoverArtLayout->insertWidget(0, m_pWFetchedCoverArtLabel.get());

    if (m_pTrackModel) {
        connect(btnPrev, &QPushButton::clicked, this, &DlgTagFetcher::slotPrev);
        connect(btnNext, &QPushButton::clicked, this, &DlgTagFetcher::slotNext);
    } else {
        btnNext->hide();
        btnPrev->hide();
    }
    //connect(btnCover,
    //        &QPushButton::clicked,
    //        this,
    //        &DlgTagFetcher::switchToCoverArtFetcher);
    connect(btnApply, &QPushButton::clicked, this, &DlgTagFetcher::apply);
    connect(btnQuit, &QPushButton::clicked, this, &DlgTagFetcher::quit);
    connect(btnRetry, &QPushButton::clicked, this, &DlgTagFetcher::retry);
    connect(results, &QTreeWidget::currentItemChanged, this, &DlgTagFetcher::resultSelected);

    connect(&m_tagFetcher, &TagFetcher::resultAvailable, this, &DlgTagFetcher::fetchTagFinished);
    connect(&m_tagFetcher, &TagFetcher::fetchProgress, this, &DlgTagFetcher::fetchTagProgress);
    connect(&m_tagFetcher,
            &TagFetcher::currentRecordingFetched,
            this,
            &DlgTagFetcher::progressBarSetCurrentStep);
    connect(&m_tagFetcher,
            &TagFetcher::numberOfRecordingsToFetch,
            this,
            &DlgTagFetcher::progressBarSetTotalSteps);
    connect(&m_tagFetcher, &TagFetcher::networkError, this, &DlgTagFetcher::slotNetworkResult);

    CoverArtCache* pCache = CoverArtCache::instance();
    if (pCache) {
        connect(pCache,
                &CoverArtCache::coverFound,
                this,
                &DlgTagFetcher::slotCoverFound);
    }

    btnRetry->setDisabled(true);

    connect(&m_tagFetcher,
            &TagFetcher::coverArtThumbnailFetchAvailable,
            this,
            &DlgTagFetcher::fetchThumbnailFinished);
    //connect(coverArtResults,
    //        &QTreeWidget::currentItemChanged,
    //        this,
    //        &DlgTagFetcher::coverArtResultSelected);

    connect(&m_tagFetcher,
            &TagFetcher::coverArtUrlsAvailable,
            this,
            &DlgTagFetcher::fetchCoverArtUrlFinished);
    connect(&m_tagFetcher,
            &TagFetcher::coverArtImageFetchAvailable,
            this,
            &DlgTagFetcher::downloadCoverAndApply);

    //CoverArtCache* pCache = CoverArtCache::instance();
    //if (pCache) {
    //    connect(pCache,
    //            &CoverArtCache::coverFound,
    //            this,
    //            &DlgTagFetcher::slotCoverFound);
    //}
}

void DlgTagFetcher::slotNext() {
    QModelIndex nextRow = m_currentTrackIndex.sibling(
            m_currentTrackIndex.row() + 1, m_currentTrackIndex.column());
    if (nextRow.isValid()) {
        loadTrack(nextRow);
        emit next();
    }
}

void DlgTagFetcher::slotPrev() {
    QModelIndex prevRow = m_currentTrackIndex.sibling(
            m_currentTrackIndex.row() - 1, m_currentTrackIndex.column());
    if (prevRow.isValid()) {
        loadTrack(prevRow);
        emit previous();
    }
}

void DlgTagFetcher::loadTrackInternal(const TrackPointer& track) {
    if (!track) {
        return;
    }
    results->clear();
    //This is added because if the track changed prev or next
    //Previous results of cover arts were on the second label
    //This deletes the results of cover arts from previous track.
    m_resultsThumbnails.clear();

    //This is also added because of the same situation,
    //After we change the track fetched cover art from previous
    //Track stays there.
    CoverInfo coverInfo;
    QPixmap pixmap;
    m_pWFetchedCoverArtLabel->setCoverArt(coverInfo, pixmap);

    disconnect(m_track.get(),
            &Track::changed,
            this,
            &DlgTagFetcher::slotTrackChanged);

    m_track = track;
    m_data = Data();
    m_networkResult = NetworkResult::Ok;

    connect(m_track.get(),
            &Track::changed,
            this,
            &DlgTagFetcher::slotTrackChanged);

    m_tagFetcher.startFetch(m_track);
    btnRetry->setDisabled(true);
    updateStack();
}

void DlgTagFetcher::loadTrack(const TrackPointer& track) {
    VERIFY_OR_DEBUG_ASSERT(!m_pTrackModel) {
        return;
    }
    loadTrackInternal(track);
}

void DlgTagFetcher::loadTrack(const QModelIndex& index) {
    VERIFY_OR_DEBUG_ASSERT(m_pTrackModel) {
        return;
    }
    TrackPointer pTrack = m_pTrackModel->getTrack(index);
    m_currentTrackIndex = index;
    loadTrackInternal(pTrack);
}

void DlgTagFetcher::slotTrackChanged(TrackId trackId) {
    if (m_track && m_track->getId() == trackId) {
        //    if (!btnCover->isEnabled()) {
        //        updateCoverFetcher();
        //    }
        //    else {
        //    }
        updateStack();
    }
}

void DlgTagFetcher::apply() {
    int resultIndex = m_data.m_selectedResult;
    if (resultIndex < 0) {
        return;
    }
    DEBUG_ASSERT(resultIndex < m_data.m_results.size());
    const mixxx::musicbrainz::TrackRelease& trackRelease =
            m_data.m_results[resultIndex];
    mixxx::TrackMetadata trackMetadata = m_track->getMetadata();
    if (!trackRelease.artist.isEmpty()) {
        trackMetadata.refTrackInfo().setArtist(
                trackRelease.artist);
    }
    if (!trackRelease.title.isEmpty()) {
        trackMetadata.refTrackInfo().setTitle(
                trackRelease.title);
    }
    if (!trackRelease.trackNumber.isEmpty()) {
        trackMetadata.refTrackInfo().setTrackNumber(
                trackRelease.trackNumber);
    }
    if (!trackRelease.trackTotal.isEmpty()) {
        trackMetadata.refTrackInfo().setTrackTotal(
                trackRelease.trackTotal);
    }
    if (!trackRelease.date.isEmpty()) {
        trackMetadata.refTrackInfo().setYear(
                trackRelease.date);
    }
    if (!trackRelease.albumArtist.isEmpty()) {
        trackMetadata.refAlbumInfo().setArtist(
                trackRelease.albumArtist);
    }
    if (!trackRelease.albumTitle.isEmpty()) {
        trackMetadata.refAlbumInfo().setTitle(
                trackRelease.albumTitle);
    }
#if defined(__EXTRA_METADATA__)
    if (!trackRelease.artistId.isNull()) {
        trackMetadata.refTrackInfo().setMusicBrainzArtistId(
                trackRelease.artistId);
    }
    if (!trackRelease.recordingId.isNull()) {
        trackMetadata.refTrackInfo().setMusicBrainzRecordingId(
                trackRelease.recordingId);
    }
    if (!trackRelease.trackReleaseId.isNull()) {
        trackMetadata.refTrackInfo().setMusicBrainzReleaseId(
                trackRelease.trackReleaseId);
    }
    if (!trackRelease.albumArtistId.isNull()) {
        trackMetadata.refAlbumInfo().setMusicBrainzArtistId(
                trackRelease.albumArtistId);
    }
    if (!trackRelease.albumReleaseId.isNull()) {
        trackMetadata.refAlbumInfo().setMusicBrainzReleaseId(
                trackRelease.albumReleaseId);
    }
    if (!trackRelease.releaseGroupId.isNull()) {
        trackMetadata.refAlbumInfo().setMusicBrainzReleaseGroupId(
                trackRelease.releaseGroupId);
    }
#endif // __EXTRA_METADATA__

    //if (!btnCover->isEnabled()) {
    //    QString status = "Cover art is applying";
    //    //coverArtStatus->setText(status);
    //    btnApply->setDisabled(true);
    //    btnNext->setDisabled(true);
    //    btnPrev->setDisabled(true);
    //    btnQuit->setDisabled(true);
    //    fetchCoverArt();
    //}

    m_track->replaceMetadataFromSource(
            std::move(trackMetadata),
            // Prevent re-import of outdated metadata from file tags
            // by explicitly setting the synchronization time stamp
            // to the current time.
            QDateTime::currentDateTimeUtc());
}

void DlgTagFetcher::retry() {
    results->clear();
    disconnect(m_track.get(),
            &Track::changed,
            this,
            &DlgTagFetcher::slotTrackChanged);

    addDivider(tr("Original tags"), results);
    addTrack(trackColumnValues(*m_track), -1, results);

    m_data = Data();
    m_networkResult = NetworkResult::Ok;

    connect(m_track.get(),
            &Track::changed,
            this,
            &DlgTagFetcher::slotTrackChanged);

    m_tagFetcher.startFetch(m_track);
    btnRetry->setDisabled(true);
    updateStack();
}

void DlgTagFetcher::quit() {
    m_tagFetcher.cancel();
    accept();
}

void DlgTagFetcher::loadCurrentTrackCover() {
    m_pWCurrentCoverArtLabel->loadTrack(m_track);
    CoverArtCache* pCache = CoverArtCache::instance();
    pCache->requestTrackCover(this, m_track);
}

void DlgTagFetcher::fetchTagProgress(const QString& text) {
    QString status = tr("Status: %1");
    loadingProgressBar->setFormat(status.arg(text));
    m_progressBarStep++;
    loadingProgressBar->setValue(m_progressBarStep);
}

void DlgTagFetcher::progressBarSetCurrentStep() {
    QString status = tr("Fetching track data from the MusicBrainz database");
    loadingProgressBar->setFormat(status);
    m_progressBarStep++;
    loadingProgressBar->setValue(m_progressBarStep);
}

// TODO(fatihemreyildiz): display the task results one by one.
void DlgTagFetcher::progressBarSetTotalSteps(int totalRecordingsFound) {
    // This function updates the bar with total metadata found for the track.
    // Tag Fetcher has 3 steps these are:
    // Fingerprinting the track, Identifiying the track, Retrieving metadata.
    // For every step the progress bar is incrementing by one in order to have better scaling.
    // If the track has less metadata then 4, the scaling jumps from %0 to %100.
    // That is why we need to rearrenge the maximum value for to have better scaling.
    while (totalRecordingsFound < 4) {
        totalRecordingsFound++;
    }

    loadingProgressBar->setMaximum(totalRecordingsFound);
}

void DlgTagFetcher::fetchTagFinished(
        TrackPointer pTrack,
        const QList<mixxx::musicbrainz::TrackRelease>& guessedTrackReleases) {
    VERIFY_OR_DEBUG_ASSERT(pTrack == m_track) {
        return;
    }
    m_data.m_pending = false;
    m_data.m_results = guessedTrackReleases;
    // qDebug() << "number of results = " << guessedTrackReleases.size();
    updateStack();
    loadCurrentTrackCover();
}

//void DlgTagFetcher::updateCoverFetcher() {
//    if (m_data.m_pending) {
//        stack->setCurrentWidget(loading_page);
//        return;
//    } else if (m_networkResult == NetworkResult::HttpError) {
//        stack->setCurrentWidget(networkError_page);
//        return;
//    } else if (m_networkResult == NetworkResult::UnknownError) {
//        stack->setCurrentWidget(generalnetworkError_page);
//        return;
//    } else if (m_data.m_results.isEmpty()) {
//        stack->setCurrentWidget(error_page);
//        return;
//    }
//
//    btnApply->setEnabled(true);
//    //btnCover->setDisabled(true);
//
//    stack->setCurrentWidget(coverFetcher_page);
//
//    coverArtResults->clear();
//
//    addDivider(tr("Original tags"), coverArtResults);
//    addTrack(trackColumnValues(*m_track), -1, coverArtResults);
//
//    addDivider(tr("Suggested tags"), coverArtResults);
//    {
//        int trackIndex = 0;
//        //Since duplicated results filtered from previous method
//        //This time we only filter the tracks which have cover art
//        for (const auto& trackRelease : qAsConst(m_data.m_results)) {
//            const auto columnValues = trackReleaseColumnValues(trackRelease);
//            if (m_resultsThumbnails.contains(trackRelease.albumReleaseId)) {
//                addTrack(columnValues, trackIndex, coverArtResults);
//            }
//            ++trackIndex;
//        }
//    }
//
//    for (int i = 0; i < coverArtResults->model()->columnCount(); i++) {
//        coverArtResults->resizeColumnToContents(i);
//        int sectionSize = (coverArtResults->columnWidth(i) + 10);
//        coverArtResults->header()->resizeSection(i, sectionSize);
//    }
//}

void DlgTagFetcher::switchToCoverArtFetcher() {
    m_tagFetcher.startFetchForCoverArt(m_data.m_results);
    m_data.m_pending = true;
    //btnCover->setDisabled(true);
    //updateCoverFetcher();
}

void DlgTagFetcher::slotNetworkResult(
        int httpError,
        const QString& app,
        const QString& message,
        int code) {
    m_networkResult = httpError == 0 ? NetworkResult::UnknownError : NetworkResult::HttpError;
    m_data.m_pending = false;

    if (m_networkResult == NetworkResult::UnknownError) {
        QString unknownError = tr("Can't connect to %1 for an unknown reason.");
        loadingProgressBar->setFormat(unknownError.arg(app));
    } else {
        QString cantConnect = tr("Can't connect to %1.");
        loadingProgressBar->setFormat(cantConnect.arg(app));
    }

    btnRetry->setEnabled(true);
    updateStack();
}

void DlgTagFetcher::updateStack() {
    loadCurrentTrackCover();
    m_progressBarStep = 0;
    loadingProgressBar->setValue(m_progressBarStep);

    btnApply->setDisabled(true);

    successMessage->setVisible(false);
    loadingProgressBar->setVisible(true);

    results->clear();
    addDivider(tr("Original tags"), results);
    addTrack(trackColumnValues(*m_track), -1, results);

    if (m_data.m_pending) {
        return;
    } else if (m_networkResult == NetworkResult::HttpError ||
            m_networkResult == NetworkResult::UnknownError) {
        loadingProgressBar->setValue(loadingProgressBar->maximum());
        return;
    } else if (m_data.m_results.isEmpty()) {
        loadingProgressBar->setValue(loadingProgressBar->maximum());
        QString emptyMessage = tr("Could not find this track in the MusicBrainz database.");
        btnRetry->setEnabled(true);
        loadingProgressBar->setFormat(emptyMessage);
        return;
    }

    btnApply->setEnabled(true);
    btnRetry->setEnabled(false);

    loadingProgressBar->setValue(loadingProgressBar->maximum());
    successMessage->setVisible(true);
    loadingProgressBar->setVisible(false);

    VERIFY_OR_DEBUG_ASSERT(m_track) {
        return;
    }

    addDivider(tr("Suggested tags"), results);
    {
        int trackIndex = 0;
        QSet<QStringList> allColumnValues; // deduplication
        for (const auto& trackRelease : qAsConst(m_data.m_results)) {
            const auto columnValues = trackReleaseColumnValues(trackRelease);
            // Ignore duplicate results
            if (!allColumnValues.contains(columnValues)) {
                allColumnValues.insert(columnValues);
                addTrack(columnValues, trackIndex, results);
            }
            ++trackIndex;
        }
    }

    for (int i = 0; i < results->model()->columnCount(); i++) {
        results->resizeColumnToContents(i);
        int sectionSize = (results->columnWidth(i) + 10);
        results->header()->resizeSection(i, sectionSize);
    }

    // Find the item that was selected last time
    for (int i = 0; i < results->model()->rowCount(); ++i) {
        const QModelIndex index = results->model()->index(i, 0);
        const QVariant id = index.data(Qt::UserRole);
        if (!id.isNull() && id.toInt() == m_data.m_selectedResult) {
            results->setCurrentIndex(index);
            break;
        }
    }
}

void DlgTagFetcher::addDivider(const QString& text, QTreeWidget* parent) const {
    QTreeWidgetItem* item = new QTreeWidgetItem(parent);
    item->setFirstColumnSpanned(true);
    item->setText(0, text);
    item->setFlags(Qt::NoItemFlags);
    item->setForeground(0, palette().color(QPalette::Disabled, QPalette::Text));

    QFont bold_font(font());
    bold_font.setBold(true);
    item->setFont(0, bold_font);
}

void DlgTagFetcher::resultSelected() {
    if (!results->currentItem()) {
        return;
    }

    if (results->currentItem()->data(0, Qt::UserRole).toInt() == -1) {
        results->currentItem()->setFlags(Qt::ItemIsEnabled);
        return;
    }
    const int resultIndex = results->currentItem()->data(0, Qt::UserRole).toInt();
    m_data.m_selectedResult = resultIndex;
}

//void DlgTagFetcher::coverArtResultSelected() {
//    if (!coverArtResults->currentItem()) {
//        return;
//    }
//    const int resultIndex = coverArtResults->currentItem()->data(0, Qt::UserRole).toInt();
//    m_data.m_selectedResult = resultIndex;
//
//    CoverInfo coverInfo;
//    QPixmap pixmap;
//
//    if (!m_resultsThumbnails.isEmpty()) {
//        if (resultIndex < 0) {
//            m_pWFetchedCoverArtLabel->setCoverArt(coverInfo, pixmap);
//        } else {
//            const mixxx::musicbrainz::TrackRelease& trackRelease = m_data.m_results[resultIndex];
//            if (m_resultsThumbnails.contains(trackRelease.albumReleaseId)) {
//                updateFetchedCoverArtLayout(m_resultsThumbnails.value(trackRelease.albumReleaseId));
//            } else {
//                m_pWFetchedCoverArtLabel->setCoverArt(coverInfo, pixmap);
//            }
//        }
//    }
//}

void DlgTagFetcher::slotCoverFound(
        const QObject* pRequestor,
        const CoverInfo& coverInfo,
        const QPixmap& pixmap,
        mixxx::cache_key_t requestedCacheKey,
        bool coverInfoUpdated) {
    Q_UNUSED(requestedCacheKey);
    Q_UNUSED(coverInfoUpdated);
    if (pRequestor == this &&
            m_track &&
            m_track->getLocation() == coverInfo.trackLocation) {
        m_trackRecord.setCoverInfo(coverInfo);
        m_pWCurrentCoverArtLabel->setCoverArt(coverInfo, pixmap);
    }
}

void DlgTagFetcher::updateFetchedCoverArtLayout(const QByteArray& thumbnailResultBytes) {
    QPixmap fetchedThumbnailPixmap;
    fetchedThumbnailPixmap.loadFromData(thumbnailResultBytes);
    CoverInfo coverInfo;
    coverInfo.type = CoverInfo::NONE;
    coverInfo.source = CoverInfo::USER_SELECTED;
    coverInfo.setImage();
    //m_pWFetchedCoverArtLabel->loadData(thumbnailResultBytes); //This data loaded because for full size.
    //m_pWFetchedCoverArtLabel->setCoverArt(coverInfo, fetchedThumbnailPixmap);
}

void DlgTagFetcher::fetchCoverArt() {
    int resultIndex = m_data.m_selectedResult;
    if (resultIndex < 0) {
        return;
    }

    int fetchedCoverArtQualityConfigValue =
            m_pConfig->getValue(mixxx::library::prefs::kCoverArtFetcherQualityConfigKey,
                    static_cast<int>(DlgPrefLibrary::CoverArtFetcherQuality::Lowest));
    DlgPrefLibrary::CoverArtFetcherQuality fetcherQuality =
            static_cast<DlgPrefLibrary::CoverArtFetcherQuality>(
                    fetchedCoverArtQualityConfigValue);

    const mixxx::musicbrainz::TrackRelease& trackRelease = m_data.m_results[resultIndex];
    if (m_resultsCoverArtAllUrls.contains(trackRelease.albumReleaseId)) {
        const QList listOfAllCoverArtUrls =
                m_resultsCoverArtAllUrls.value(trackRelease.albumReleaseId);
        if (fetcherQuality == DlgPrefLibrary::CoverArtFetcherQuality::Lowest) {
            getCoverArt(listOfAllCoverArtUrls.last());
            return;
        } else if (fetcherQuality == DlgPrefLibrary::CoverArtFetcherQuality::Medium) {
            if (listOfAllCoverArtUrls.size() < 3) {
                getCoverArt(listOfAllCoverArtUrls.first());
            } else {
                getCoverArt(listOfAllCoverArtUrls.at(1));
            }
            return;
        } else if (fetcherQuality == DlgPrefLibrary::CoverArtFetcherQuality::Highest) {
            getCoverArt(listOfAllCoverArtUrls.first());
            return;
        }
    }
}

void DlgTagFetcher::downloadCoverAndApply(const QByteArray& data) {
    QString coverArtLocation = m_track->getLocation() + ".jpg";
    QFile coverArtFile(coverArtLocation);
    coverArtFile.open(QIODevice::WriteOnly);
    coverArtFile.write(data);
    coverArtFile.close();

    auto selectedCover = mixxx::FileAccess(mixxx::FileInfo(coverArtLocation));
    QImage updatedCoverArtFound(coverArtLocation);
    if (updatedCoverArtFound.isNull()) {
        return;
    }

    CoverInfoRelative coverInfo;
    coverInfo.type = CoverInfo::FILE;
    coverInfo.source = CoverInfo::USER_SELECTED;
    coverInfo.coverLocation = coverArtLocation;
    coverInfo.setImage(updatedCoverArtFound);
    m_pWCurrentCoverArtLabel->setCoverArt(CoverInfo{}, QPixmap::fromImage(updatedCoverArtFound));
    m_track->setCoverInfo(coverInfo);

    QString status = "Cover art is applied";
    //coverArtStatus->setText(status);
    btnApply->setEnabled(true);
    btnNext->setEnabled(true);
    btnPrev->setEnabled(true);
    btnQuit->setEnabled(true);
}

void DlgTagFetcher::getCoverArt(const QString& url) {
    m_tagFetcher.fetchDesiredResolutionCoverArt(url);
}

void DlgTagFetcher::fetchCoverArtUrlFinished(const QMap<QUuid, QList<QString>>& coverArtAllUrls) {
    m_resultsCoverArtAllUrls = coverArtAllUrls;
}

void DlgTagFetcher::fetchThumbnailFinished(const QMap<QUuid, QByteArray>& thumbnailBytes) {
    QString status = "Cover art is found";
    //coverArtStatus->setText(status);
    m_resultsThumbnails = thumbnailBytes;
    m_data.m_pending = false;
    //updateCoverFetcher();
}

//void DlgTagFetcher::slotCoverFound(
//        const QObject* pRequestor,
//        const CoverInfo& coverInfo,
//        const QPixmap& pixmap,
//        mixxx::cache_key_t requestedCacheKey,
//        bool coverInfoUpdated) {
//    Q_UNUSED(requestedCacheKey);
//    Q_UNUSED(coverInfoUpdated);
//    if (pRequestor == this &&
//            m_track &&
//            m_track->getLocation() == coverInfo.trackLocation) {
//        m_trackRecord.setCoverInfo(coverInfo);
//        m_pWCurrentCoverArtLabel->setCoverArt(coverInfo, pixmap);
//    }
//}
