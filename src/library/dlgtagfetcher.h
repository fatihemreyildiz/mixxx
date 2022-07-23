#pragma once

#include <QDialog>
#include <QList>
#include <QTreeWidget>

#include "library/coverartlabel.h"
#include "library/trackmodel.h"
#include "library/ui_dlgtagfetcher.h"
#include "musicbrainz/tagfetcher.h"
#include "track/track_decl.h"
#include "track/trackrecord.h"
#include "util/parented_ptr.h"

class CoverArtLabel;

/// A dialog box to fetch track metadata from MusicBrainz.
/// Use TrackPointer to load a track into the dialog or
/// QModelIndex along with TrackModel to enable previous and next buttons
/// to switch tracks within the context of the TrackModel.
class DlgTagFetcher : public QDialog, public Ui::DlgTagFetcher {
    Q_OBJECT

  public:
    // TODO: Remove dependency on TrackModel
    explicit DlgTagFetcher(
            UserSettingsPointer pConfig, const TrackModel* pTrackModel = nullptr);
    ~DlgTagFetcher() override = default;

    void init();

  public slots:
    void loadTrack(const TrackPointer& track);
    void loadTrack(const QModelIndex& index);

  signals:
    void next();
    void previous();

  private slots:
    void fetchTagFinished(
            TrackPointer pTrack,
            const QList<mixxx::musicbrainz::TrackRelease>& guessedTrackReleases);
    void fetchCoverArtUrlFinished(const QMap<QUuid, QList<QString>>& coverArtAllUrls);
    void fetchThumbnailFinished(const QMap<QUuid, QByteArray>& thumbnailBytes);
    void resultSelected();
    void fetchTagProgress(const QString&);
    void slotNetworkResult(int httpStatus, const QString& app, const QString& message, int code);
    void slotTrackChanged(TrackId trackId);
    void apply();
    void quit();
    void fetchCoverArt();
    void slotNext();
    void slotPrev();
    void slotCoverFound(
            const QObject* pRequestor,
            const CoverInfo& coverInfo,
            const QPixmap& pixmap,
            mixxx::cache_key_t requestedCacheKey,
            bool coverInfoUpdated);
    void updateFetchedCoverArtLayout(const QByteArray& thumbnailResultBytes);
    void downloadCoverAndApply(const QByteArray& data);

  private:
    void loadCurrentTrackCover();
    void loadTrackInternal(const TrackPointer& track);
    void updateStack();
    void addDivider(const QString& text, QTreeWidget* parent) const;
    void getCoverArt(const QString& url);

    UserSettingsPointer m_pConfig;

    const TrackModel* const m_pTrackModel;

    TagFetcher m_tagFetcher;

    TrackPointer m_track;

    QModelIndex m_currentTrackIndex;

    parented_ptr<CoverArtLabel> m_pCurrentCoverArt;
    parented_ptr<CoverArtLabel> m_pFetchedCoverArt;

    mixxx::TrackRecord m_trackRecord;

    QMap<QUuid, QByteArray> m_resultsThumbnails;

    QMap<QUuid, QList<QString>> m_resultsCoverArtAllUrls;

    struct Data {
        Data()
                : m_pending(true),
                  m_selectedResult(-1) {
        }

        bool m_pending;
        int m_selectedResult;
        QList<mixxx::musicbrainz::TrackRelease> m_results;
    };
    Data m_data;

    enum class NetworkResult {
        Ok,
        HttpError,
        UnknownError,
    };
    NetworkResult m_networkResult;
};
