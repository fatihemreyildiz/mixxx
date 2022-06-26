#pragma once

#include <QMenu>

class FindOnWeb;
class Track;

class FindOnWebFactory : public QMenu {
    Q_OBJECT
  public:
    enum Service {
        Discogs,
        Soundcloud,
        Lastfm
    };

    explicit FindOnWebFactory(
            QWidget* parent = nullptr);

    ~FindOnWebFactory() override = default;

    void createServiceMenus(QMenu* menu, const Track& track);

    bool serviceIsEnabled(Service service) const;

    void openInBrowser(const QString& query);

  signals:
    bool triggerBrowser(const QString& query);
};
