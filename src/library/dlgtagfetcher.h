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
#include "widget/wcoverartlabel.h"
#include "widget/wcoverartmenu.h"

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
    void resultSelected();
    void progressBarSetTotalSteps(int totalRecordingsFound);
    void progressBarSetCurrentStep();
    void slotNetworkResult(int httpStatus, const QString& app, const QString& message, int code);
    void slotTrackChanged(TrackId trackId);
    void apply();
    void retry();
    void quit();
    void slotNext();
    void slotPrev();
    void slotCoverFound(
            const QObject* pRequestor,
            const CoverInfo& coverInfo,
            const QPixmap& pixmap,
            mixxx::cache_key_t requestedCacheKey,
            bool coverInfoUpdated);
    void slotUpdateProgressBarMessage(const QString& message);
    void slotUpdateStatusMessage(const QString& message);
    void slotStartFetchCoverArt(const QList<QString>& allUrls);
    void slotLoadBytesToLabel(const QByteArray& data);
    void slotCoverArtLinkNotFound();

  private:
    void loadTrackInternal(const TrackPointer& track);
    void updateStack();
    void addDivider(const QString& text, QTreeWidget* parent) const;
    void getCoverArt(const QString& url);
    void loadCurrentTrackCover();

    UserSettingsPointer m_pConfig;

    const TrackModel* const m_pTrackModel;

    TagFetcher m_tagFetcher;

    TrackPointer m_track;

    QModelIndex m_currentTrackIndex;

    parented_ptr<WCoverArtMenu> m_pWCoverArtMenu;
    parented_ptr<WCoverArtLabel> m_pWCurrentCoverArtLabel;
    parented_ptr<WCoverArtLabel> m_pWFetchedCoverArtLabel;

    mixxx::TrackRecord m_trackRecord;

    int m_progressBarStep;

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

    QByteArray m_fetchedCoverArtByteArrays;
};
