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

    connect(btnApply, &QPushButton::clicked, this, &DlgTagFetcher::apply);
    connect(btnQuit, &QPushButton::clicked, this, &DlgTagFetcher::quit);
    connect(btnRetry, &QPushButton::clicked, this, &DlgTagFetcher::retry);
    connect(results, &QTreeWidget::currentItemChanged, this, &DlgTagFetcher::resultSelected);

    connect(&m_tagFetcher, &TagFetcher::resultAvailable, this, &DlgTagFetcher::fetchTagFinished);
    connect(&m_tagFetcher,
            &TagFetcher::fetchProgress,
            this,
            &DlgTagFetcher::slotUpdateProgressBarMessage);
    connect(&m_tagFetcher,
            &TagFetcher::currentRecordingFetched,
            this,
            &DlgTagFetcher::progressBarSetCurrentStep);
    connect(&m_tagFetcher,
            &TagFetcher::numberOfRecordingsToFetch,
            this,
            &DlgTagFetcher::progressBarSetTotalSteps);
    connect(&m_tagFetcher, &TagFetcher::networkError, this, &DlgTagFetcher::slotNetworkResult);

    btnRetry->setDisabled(true);

    CoverArtCache* pCache = CoverArtCache::instance();
    if (pCache) {
        connect(pCache,
                &CoverArtCache::coverFound,
                this,
                &DlgTagFetcher::slotCoverFound);
    }

    connect(&m_tagFetcher,
            &TagFetcher::coverArtArchiveLinksAvailable,
            this,
            &DlgTagFetcher::slotStartFetchCoverArt);

    connect(&m_tagFetcher,
            &TagFetcher::coverArtImageFetchAvailable,
            this,
            &DlgTagFetcher::slotLoadBytesToLabel);

    connect(&m_tagFetcher,
            &TagFetcher::coverArtLinkNotFound,
            this,
            &DlgTagFetcher::slotCoverArtLinkNotFound);
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
    if (!m_fetchedCoverArtByteArrays.isNull()) {
        m_fetchedCoverArtByteArrays.clear();
    }

    if (m_track && m_track->getId() == trackId) {
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

    if (!m_fetchedCoverArtByteArrays.isNull()) {
        // Worker can be called here.
        QString coverArtLocation = m_track->getLocation() + ".jpg";
        QFile coverArtFile(coverArtLocation);
        coverArtFile.open(QIODevice::WriteOnly);
        coverArtFile.write(m_fetchedCoverArtByteArrays);
        coverArtFile.close();

        auto selectedCover = mixxx::FileAccess(mixxx::FileInfo(coverArtLocation));
        QImage updatedCoverArtFound(coverArtLocation);
        if (!updatedCoverArtFound.isNull()) {
            CoverInfoRelative coverInfo;
            coverInfo.type = CoverInfo::FILE;
            coverInfo.source = CoverInfo::USER_SELECTED;
            coverInfo.coverLocation = coverArtLocation;
            coverInfo.setImage(updatedCoverArtFound);
            m_pWCurrentCoverArtLabel->setCoverArt(
                    CoverInfo{}, QPixmap::fromImage(updatedCoverArtFound));
            m_track->setCoverInfo(coverInfo);
            m_fetchedCoverArtByteArrays.clear();
            m_pWFetchedCoverArtLabel->loadData(m_fetchedCoverArtByteArrays);
            m_pWFetchedCoverArtLabel->setCoverArt(CoverInfo{},
                    QPixmap(CoverArtUtils::defaultCoverLocation()));
            QString coverArtAppliedMessage = tr("Cover art applied.");
            statusMessage->setText(coverArtAppliedMessage);
        }
    }

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

void DlgTagFetcher::slotUpdateProgressBarMessage(const QString& text) {
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

    statusMessage->setVisible(false);
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

    QString updateStackMessage = tr("The results are ready to be applied");
    statusMessage->setText(updateStackMessage);
    statusMessage->setVisible(true);
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

    if (!m_fetchedCoverArtByteArrays.isNull()) {
        m_fetchedCoverArtByteArrays.clear();
        CoverInfo coverInfo;
        QPixmap pixmap;
        m_pWFetchedCoverArtLabel->setCoverArt(coverInfo, pixmap);
    }

    const mixxx::musicbrainz::TrackRelease& trackRelease = m_data.m_results[resultIndex];
    QUuid selectedResultAlbumId = trackRelease.albumReleaseId;
    statusMessage->setVisible(false);

    QString coverArtMessage = tr("Looking for cover art");
    loadingProgressBar->setVisible(true);
    loadingProgressBar->setFormat(coverArtMessage);
    loadingProgressBar->setMinimum(0);
    loadingProgressBar->setMaximum(100);
    loadingProgressBar->setValue(10);

    m_tagFetcher.startFetchCoverArtLinks(selectedResultAlbumId);
}

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

void DlgTagFetcher::slotUpdateStatusMessage(const QString& message) {
    loadingProgressBar->setVisible(false);
    statusMessage->setVisible(true);
    statusMessage->setText(message);
}

void DlgTagFetcher::slotStartFetchCoverArt(const QList<QString>& allUrls) {
    int fetchedCoverArtQualityConfigValue =
            m_pConfig->getValue(mixxx::library::prefs::kCoverArtFetcherQualityConfigKey,
                    static_cast<int>(DlgPrefLibrary::CoverArtFetcherQuality::Low));

    DlgPrefLibrary::CoverArtFetcherQuality fetcherQuality =
            static_cast<DlgPrefLibrary::CoverArtFetcherQuality>(
                    fetchedCoverArtQualityConfigValue);

    // Maximum size of the allUrls are 4.
    if (fetcherQuality == DlgPrefLibrary::CoverArtFetcherQuality::Highest) {
        getCoverArt(allUrls.last());
        qDebug() << allUrls.last();
        return;
    } else if (fetcherQuality == DlgPrefLibrary::CoverArtFetcherQuality::High) {
        qDebug() << allUrls.size();
        qDebug() << allUrls;
        allUrls.size() < 3 ? getCoverArt(allUrls.at(1)) : getCoverArt(allUrls.at(2));
        return;
    } else if (fetcherQuality == DlgPrefLibrary::CoverArtFetcherQuality::Medium) {
        getCoverArt(allUrls.at(1));
        qDebug() << allUrls.at(1);
        return;
    } else {
        getCoverArt(allUrls.first());
    }
}

void DlgTagFetcher::slotLoadBytesToLabel(const QByteArray& data) {
    loadingProgressBar->setValue(80);
    QPixmap fetchedCoverArtPixmap;
    fetchedCoverArtPixmap.loadFromData(data);
    CoverInfo coverInfo;
    coverInfo.type = CoverInfo::NONE;
    coverInfo.source = CoverInfo::USER_SELECTED;
    coverInfo.setImage();

    loadingProgressBar->setVisible(false);
    QString coverArtMessage = tr("Cover art found and it is ready to be applied.");
    statusMessage->setVisible(true);
    statusMessage->setText(coverArtMessage);

    m_fetchedCoverArtByteArrays = data;
    m_pWFetchedCoverArtLabel->loadData(
            m_fetchedCoverArtByteArrays); //This data loaded because for full size.
    m_pWFetchedCoverArtLabel->setCoverArt(coverInfo, fetchedCoverArtPixmap);
}

void DlgTagFetcher::getCoverArt(const QString& url) {
    QString coverArtMessage = tr("Cover art found getting image");
    loadingProgressBar->setFormat(coverArtMessage);
    loadingProgressBar->setValue(40);

    m_tagFetcher.startFetchCoverArtImage(url);
}

void DlgTagFetcher::slotCoverArtLinkNotFound() {
    loadingProgressBar->setVisible(false);
    statusMessage->setVisible(true);
    QString message = tr("Cover Art not available for that tag. Please try another.");
    statusMessage->setText(message);
}
