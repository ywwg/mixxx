/***************************************************************************
                          enginesync.h  -  master sync control for
                          maintaining beatmatching amongst n decks
                             -------------------
    begin                : Mon Mar 12 2012
    copyright            : (C) 2012 by Owen Williams
    email                : owilliams@mixxx.org
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef ENGINESYNC_H
#define ENGINESYNC_H

#include "engine/sync/internalclock.h"
#include "engine/sync/syncable.h"
#include "preferences/usersettings.h"

/// EngineSync is the heart of the Mixxx Master Sync engine.  It knows which objects
/// (Decks, Internal Clock, etc) are participating in Sync and what their statuses
/// are. It also orchestrates sync handoffs between different decks as they play,
/// stop, or request their status to change.
class EngineSync : public SyncableListener {
  public:
    explicit EngineSync(UserSettingsPointer pConfig);
    ~EngineSync() override;

    void addSyncableDeck(Syncable* pSyncable);
    EngineChannel* getMaster() const;
    void onCallbackStart(int sampleRate, int bufferSize);
    void onCallbackEnd(int sampleRate, int bufferSize);

    // Used by Syncables to tell EngineSync it wants to be enabled in a
    // specific mode. If the state change is accepted, EngineSync calls
    // Syncable::notifySyncModeChanged.
    void requestSyncMode(Syncable* pSyncable, SyncMode state) override;

    // Used by Syncables to tell EngineSync it wants to be enabled in any mode
    // (master/follower).
    void requestEnableSync(Syncable* pSyncable, bool enabled) override;

    // Syncables notify EngineSync directly about various events. EngineSync
    // does not have a say in whether these succeed or not, they are simply
    // notifications.
    void notifyBpmChanged(Syncable* pSyncable, double bpm) override;
    void requestBpmUpdate(Syncable* pSyncable, double bpm) override;
    void notifyInstantaneousBpmChanged(Syncable* pSyncable, double bpm) override;
    void notifyBeatDistanceChanged(Syncable* pSyncable, double beatDistance) override;
    void notifyPlaying(Syncable* pSyncable, bool playing) override;
    void notifyScratching(Syncable* pSyncable, bool scratching) override;

    // Used to pick a sync target for non-master-sync mode.
    EngineChannel* pickNonSyncSyncTarget(EngineChannel* pDontPick) const;

    // Used to test whether changing the rate of a Syncable would change the rate
    // of other Syncables that are playing
    bool otherSyncedPlaying(const QString& group);

    // Only for testing. Do not use.
    Syncable* getSyncableForGroup(const QString& group);
    Syncable* getMasterSyncable() override {
        return m_pMasterSyncable;
    }

  private:
    // This utility method returns true if it finds a deck not in SYNC_NONE mode.
    bool syncDeckExists() const;

    // Return the current BPM of the master Syncable. If no master syncable is
    // set then returns the BPM of the internal clock.
    double masterBpm() const;

    // Returns the current beat distance of the master Syncable. If no master
    // Syncable is set, then returns the beat distance of the internal clock.
    double masterBeatDistance() const;

    // Returns the current BPM of the master Syncable if it were playing
    // at 1.0 rate.
    double masterBaseBpm() const;

    // Set the BPM on every sync-enabled Syncable except pSource.
    void setMasterBpm(Syncable* pSource, double bpm);

    // Set the master instantaneous BPM on every sync-enabled Syncable except
    // pSource.
    void setMasterInstantaneousBpm(Syncable* pSource, double bpm);

    // Set the master beat distance on every sync-enabled Syncable except
    // pSource.
    void setMasterBeatDistance(Syncable* pSource, double beat_distance);

    void setMasterParams(Syncable* pSource, double beat_distance, double base_bpm, double bpm);

    // Check if there is only one playing syncable deck, and notify it if so.
    void checkUniquePlayingSyncable();

    // Iterate over decks, and based on sync and play status, pick a new master.
    // if enabling_syncable is not null, we treat it as if it were enabled because we may
    // be in the process of enabling it.
    Syncable* pickMaster(Syncable* enabling_syncable);

    // Find a deck to match against, used in the case where there is no sync master.
    // Looks first for a playing deck, and falls back to the first non-playing deck.
    // Returns nullptr if none can be found.
    Syncable* findBpmMatchTarget(Syncable* requester);

    // Activate a specific syncable as master.
    void activateMaster(Syncable* pSyncable, bool explicitMaster);

    // Activate a specific channel as Follower. Sets the syncable's bpm and
    // beat_distance to match the master.
    void activateFollower(Syncable* pSyncable);

    // Unsets all sync state on a Syncable.
    void deactivateSync(Syncable* pSyncable);

    UserSettingsPointer m_pConfig;
    // The InternalClock syncable.
    InternalClock* m_pInternalClock;
    // The current Syncable that is the master.
    Syncable* m_pMasterSyncable;
    // The list of all Syncables registered with EngineSync via
    // addSyncableDeck.
    QList<Syncable*> m_syncables;
};

#endif
