#include "library/dlgtagfetcher.h"

#include <QTreeWidget>
#include <QtDebug>

#include "defs_urls.h"
#include "moc_dlgtagfetcher.cpp"
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

DlgTagFetcher::DlgTagFetcher(
        const TrackModel* pTrackModel)
        // No parent because otherwise it inherits the style parent's
        // style which can make it unreadable. Bug #673411
        : QDialog(nullptr),
          m_pTrackModel(pTrackModel),
          m_tagFetcher(this),
          m_networkResult(NetworkResult::Ok) {
    init();
}

void DlgTagFetcher::init() {
    setupUi(this);
    setWindowIcon(QIcon(MIXXX_ICON_PATH));

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

    btnRetry->setDisabled(true);
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
