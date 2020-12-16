#ifndef MIXXX_CRATETABLEMODEL_H
#define MIXXX_CRATETABLEMODEL_H


#include "library/basesqltablemodel.h"

#include "library/crate/crateid.h"

class CrateTableModel final : public BaseSqlTableModel {
    Q_OBJECT

  public:
    CrateTableModel(QObject* parent, TrackCollectionManager* pTrackCollectionManager);
    ~CrateTableModel() final;

    void selectCrate(
        CrateId crateId = CrateId());
    CrateId selectedCrate() const {
        return m_selectedCrate;
    }

    bool addTrack(const QModelIndex& index, const QString& location);

    // From TrackModel
    bool isColumnInternal(int column) final;
    void removeTracks(const QModelIndexList& indices) final;
    // Returns the number of unsuccessful track additions
    int addTracks(const QModelIndex& index, const QList<QString>& locations) final;
    CapabilitiesFlags getCapabilities() const final;

    QString getModelSetting(QString name) override;
    bool setModelSetting(QString name, QVariant value) override;

  private:
    CrateId m_selectedCrate;
};

#endif // MIXXX_CRATETABLEMODEL_H
