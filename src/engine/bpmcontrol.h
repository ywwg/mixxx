
// bpmcontrol.h
// Created 7/5/2009 by RJ Ryan (rryan@mit.edu)

#ifndef BPMCONTROL_H
#define BPMCONTROL_H

#include "engine/enginecontrol.h"
#include "engine/enginesync.h"
#include "tapfilter.h"

class ControlObject;
class ControlPushButton;
class EngineBuffer;
class ControlObjectSlave;

class BpmControl : public EngineControl {
    Q_OBJECT

  public:
    BpmControl(const char* _group, ConfigObject<ConfigValue>* _config);
    virtual ~BpmControl();

    double getBpm() const;
    double getFileBpm() const { return m_dFileBpm; }
    void onEngineRateChange(double rate);
    double getSyncAdjustment(bool userTweakingSync);
    double getSyncedRate() const { return m_dSyncedRate; }
    // Get the phase offset from the specified position.
    double getPhaseOffset(double reference_position);

    void setCurrentSample(const double dCurrentSample, const double dTotalSamples);
    double process(const double dRate,
                   const double dCurrentSample,
                   const double dTotalSamples,
                   const int iBufferSize);

  public slots:
    virtual void trackLoaded(TrackPointer pTrack);
    virtual void trackUnloaded(TrackPointer pTrack);
    void slotControlBeatSync(double);

  private slots:
    void slotSetEngineBpm(double);
    void slotFileBpmChanged(double);
    void slotControlPlay(double);
    void slotControlBeatSyncPhase(double);
    void slotControlBeatSyncTempo(double);
    void slotTapFilter(double,int);
    void slotBpmTap(double);
    void slotAdjustBpm();
    void slotUpdatedTrackBeats();
    void slotBeatsTranslate(double);
    void slotMasterBpmChanged(double);
    void slotSyncMasterChanged(double);
    void slotSyncSlaveChanged(double);
    void slotSyncInternalChanged(double);
    void slotSyncStateChanged(double);
    void slotSetStatuses();

  private:
    double getBeatDistance(double dThisPosition) const;
    bool syncTempo();
    bool syncPhase();

    // ControlObjects that come from PlayerManager
    ControlObject* m_pNumDecks;

    // ControlObjects that come from EngineBuffer
    ControlObjectSlave* m_pPlayButton;
    ControlObjectSlave* m_pRateSlider;
    ControlObject* m_pQuantize;
    ControlObject* m_pRateRange;
    ControlObject* m_pRateDir;

    ControlObject* m_pThisBeatDistance;

    // Is vinyl control enabled?
    ControlObject* m_pVCEnabled;

    // ControlObjects that come from LoopingControl
    ControlObject* m_pLoopEnabled;
    ControlObject* m_pLoopStartPosition;
    ControlObject* m_pLoopEndPosition;

    /** The current loaded file's detected BPM */
    ControlObject* m_pFileBpm;

    /** The current effective BPM of the engine */
    ControlObject* m_pEngineBpm;

    // Used for bpm tapping from GUI and MIDI
    ControlPushButton* m_pButtonTap;

    /** Button for sync'ing with the other EngineBuffer */
    ControlPushButton* m_pButtonSync;
    ControlPushButton* m_pButtonSyncPhase;
    ControlPushButton* m_pButtonSyncTempo;

    // Button that translates the beats so the nearest beat is on the current
    // playposition.
    ControlPushButton* m_pTranslateBeats;

    double m_dFileBpm;
    double m_dLoopSize; // Only used to see if we shouldn't quantize position.
    double m_dPreviousSample;

    // Master Sync objects and values.
    ControlObject *m_pMasterBpm;
    ControlObject *m_pSyncInternalEnabled;
    ControlPushButton *m_pSyncMasterEnabled, *m_pSyncSlaveEnabled;
    ControlObject *m_pSyncState;
    ControlObject* m_pMasterBeatDistance;
    double m_dSyncAdjustment;
    double m_dUserOffset;
    double m_dSyncedRate;
    int m_iSyncState;

    TapFilter m_tapFilter;

    TrackPointer m_pTrack;
    BeatsPointer m_pBeats;

    QString m_sGroup;
};


#endif /* BPMCONTROL_H */
