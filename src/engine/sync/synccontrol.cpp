#include "engine/sync/synccontrol.h"

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "control/controlpushbutton.h"
#include "engine/channels/enginechannel.h"
#include "engine/controls/bpmcontrol.h"
#include "engine/controls/ratecontrol.h"
#include "engine/enginebuffer.h"
#include "engine/enginemaster.h"
#include "moc_synccontrol.cpp"
#include "track/track.h"
#include "util/assert.h"
#include "util/logger.h"
#include "util/math.h"

const double SyncControl::kBpmUnity = 1.0;
const double SyncControl::kBpmHalve = 0.5;
const double SyncControl::kBpmDouble = 2.0;

namespace {
const mixxx::Logger kLogger("SyncControl");
} // namespace

SyncControl::SyncControl(const QString& group,
        UserSettingsPointer pConfig,
        EngineChannel* pChannel,
        SyncableListener* pEngineSync)
        : EngineControl(group, pConfig),
          m_sGroup(group),
          m_pChannel(pChannel),
          m_pEngineSync(pEngineSync),
          m_pBpmControl(nullptr),
          m_pRateControl(nullptr),
          m_bOldScratching(false),
          m_leaderBpmAdjustFactor(kBpmUnity),
          m_unmultipliedTargetBeatDistance(0.0),
          m_pBpm(nullptr),
          m_pLocalBpm(nullptr),
          m_pRateRatio(nullptr),
          m_pVCEnabled(nullptr),
          m_pSyncPhaseButton(nullptr) {
    // Play button.  We only listen to this to disable leader if the deck is
    // stopped.
    m_pPlayButton = new ControlProxy(group, "play", this);
    m_pPlayButton->connectValueChanged(this, &SyncControl::slotControlPlay,
                                       Qt::DirectConnection);

    m_pSyncMode.reset(new ControlPushButton(ConfigKey(group, "sync_mode")));
    m_pSyncMode->setButtonMode(ControlPushButton::TOGGLE);
    m_pSyncMode->setStates(SYNC_NUM_MODES);
    m_pSyncMode->connectValueChangeRequest(
            this, &SyncControl::slotSyncModeChangeRequest, Qt::DirectConnection);

    m_pSyncLeaderEnabled.reset(
            new ControlPushButton(ConfigKey(group, "sync_leader")));
    m_pSyncLeaderEnabled->setButtonMode(ControlPushButton::TOGGLE);
    m_pSyncLeaderEnabled->setStates(3);
    m_pSyncLeaderEnabled->connectValueChangeRequest(
            this, &SyncControl::slotSyncLeaderEnabledChangeRequest, Qt::DirectConnection);
    ControlDoublePrivate::insertAlias(ConfigKey(group, "sync_master"),
            ConfigKey(group, "sync_leader"));

    m_pSyncEnabled.reset(
            new ControlPushButton(ConfigKey(group, "sync_enabled")));
    m_pSyncEnabled->setButtonMode(ControlPushButton::LONGPRESSLATCHING);
    m_pSyncEnabled->connectValueChangeRequest(
            this, &SyncControl::slotSyncEnabledChangeRequest, Qt::DirectConnection);

    // The relative position between two beats in the range 0.0 ... 1.0
    m_pBeatDistance.reset(
            new ControlObject(ConfigKey(group, "beat_distance")));

    m_pPassthroughEnabled = new ControlProxy(group, "passthrough", this);
    m_pPassthroughEnabled->connectValueChanged(this,
            &SyncControl::slotPassthroughChanged, Qt::DirectConnection);

    m_pQuantize = new ControlProxy(group, "quantize", this);

    // Adopt an invalid to not ignore the first call setLocalBpm()
    m_prevLocalBpm.setValue(-1);

    // BPMControl and RateControl will be initialized later.
}

SyncControl::~SyncControl() {
}

void SyncControl::setEngineControls(RateControl* pRateControl,
                                    BpmControl* pBpmControl) {
    m_pRateControl = pRateControl;
    m_pBpmControl = pBpmControl;

    // We set this to change the effective BPM in BpmControl. We do not listen
    // to changes from this control because changes in rate, rate_dir, rateRange
    // and file_bpm result in changes to this control.
    m_pBpm = new ControlProxy(getGroup(), "bpm", this);

    m_pLocalBpm = new ControlProxy(getGroup(), "local_bpm", this);

    m_pRateRatio = new ControlProxy(getGroup(), "rate_ratio", this);
    m_pRateRatio->connectValueChanged(this, &SyncControl::slotRateChanged,
                                       Qt::DirectConnection);

    m_pSyncPhaseButton = new ControlProxy(getGroup(), "beatsync_phase", this);

#ifdef __VINYLCONTROL__
    m_pVCEnabled = new ControlProxy(
            getGroup(), "vinylcontrol_enabled", this);

    // Throw a hissy fit if somebody moved us such that the vinylcontrol_enabled
    // control doesn't exist yet. This will blow up immediately, won't go unnoticed.
    DEBUG_ASSERT(m_pVCEnabled->valid());

    m_pVCEnabled->connectValueChanged(this, &SyncControl::slotVinylControlChanged,
                                      Qt::DirectConnection);
#endif
}

SyncMode SyncControl::getSyncMode() const {
    return syncModeFromDouble(m_pSyncMode->get());
}

void SyncControl::setSyncMode(SyncMode mode) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << "SyncControl::setSyncMode" << getGroup() << mode;
    }
    // SyncControl has absolutely no say in the matter. This is what EngineSync
    // requires. Bypass confirmation by using setAndConfirm.
    m_pSyncMode->setAndConfirm(mode);
    m_pSyncEnabled->setAndConfirm(mode != SYNC_NONE);
    m_pSyncLeaderEnabled->setAndConfirm(SyncModeToLeaderLight(mode));
    if (mode == SYNC_FOLLOWER) {
        if (m_pVCEnabled && m_pVCEnabled->toBool()) {
            // If follower mode is enabled, disable vinyl control.
            m_pVCEnabled->set(0.0);
        }
    }
    if (mode != SYNC_NONE && m_pPassthroughEnabled->toBool()) {
        // If any sync mode is enabled and passthrough was on somehow, disable passthrough.
        // This is very unlikely to happen so this deserves a warning.
        qWarning() << "Notified of sync mode change when passthrough was "
                      "active -- "
                      "must disable passthrough";
        m_pPassthroughEnabled->set(0.0);
    }
    if (mode == SYNC_NONE) {
        m_leaderBpmAdjustFactor = kBpmUnity;
    }
}

void SyncControl::notifyUniquePlaying() {
    // If we are the only remaining playing sync deck, we can reset the user
    // tweak info.
    m_pBpmControl->resetSyncAdjustment();
}

void SyncControl::requestSync() {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << "SyncControl::requestSync" << this->getGroup()
                        << isPlaying() << m_pQuantize->toBool();
    }
    if (isPlaying() && m_pQuantize->toBool()) {
        // only sync phase if the deck is playing and if quantize is enabled.
        // this way the it is up to the user to decide if a seek is desired or not.
        // This is helpful if the beatgrid of the track does not fit at the current
        // playposition
        m_pChannel->getEngineBuffer()->requestSyncPhase();
    }
}

bool SyncControl::isPlaying() const {
    return m_pPlayButton->toBool();
}

bool SyncControl::isAudible() const {
    return m_audible;
}

double SyncControl::adjustSyncBeatDistance(double beatDistance) const {
    // Similar to adjusting the target beat distance, when we report our beat
    // distance we need to adjust it by the leader bpm adjustment factor.  If
    // we've been doubling the leader bpm, we need to divide it in half.  If
    // we've been halving the leader bpm, we need to double it.  Both operations
    // also need to account for if the longer beat is past its halfway point.
    //
    // This is the inverse of the updateTargetBeatDistance function below.
    if (m_leaderBpmAdjustFactor == kBpmDouble) {
        beatDistance /= kBpmDouble;
        if (m_unmultipliedTargetBeatDistance >= 0.5) {
            beatDistance += 0.5;
        }
    } else if (m_leaderBpmAdjustFactor == kBpmHalve) {
        if (beatDistance >= 0.5) {
            beatDistance -= 0.5;
        }
        beatDistance /= kBpmHalve;
    }
    return beatDistance;
}

double SyncControl::getBeatDistance() const {
    double beatDistance = m_pBeatDistance->get();
    return adjustSyncBeatDistance(beatDistance);
}

double SyncControl::getBaseBpm() const {
    return m_pLocalBpm->get() / m_leaderBpmAdjustFactor;
}

void SyncControl::updateLeaderBeatDistance(double beatDistance) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::setLeaderBeatDistance"
                        << beatDistance;
    }
    // Set the BpmControl target beat distance to beatDistance, adjusted by
    // the multiplier if in effect.  This way all of the multiplier logic
    // is contained in this single class.
    m_unmultipliedTargetBeatDistance = beatDistance;
    // Update the target beat distance based on the multiplier.
    updateTargetBeatDistance();
}

void SyncControl::updateLeaderBpm(double bpm) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::setLeaderBpm" << bpm;
    }

    VERIFY_OR_DEBUG_ASSERT(isSynchronized()) {
        qWarning() << "WARNING: Logic Error: setBpm called on SYNC_NONE syncable.";
        return;
    }

    // Vinyl Control overrides.
    if (m_pVCEnabled && m_pVCEnabled->get() > 0.0) {
        return;
    }

    double localBpm = m_pLocalBpm->get();
    if (localBpm > 0.0) {
        m_pRateRatio->set(bpm * m_leaderBpmAdjustFactor / localBpm);
    }
}

void SyncControl::notifyLeaderParamSource() {
    m_leaderBpmAdjustFactor = kBpmUnity;
}

void SyncControl::reinitLeaderParams(
        double beatDistance, double baseBpm, double bpm) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << "SyncControl::setLeaderParams" << getGroup()
                        << beatDistance << baseBpm << bpm;
    }
    m_leaderBpmAdjustFactor = determineBpmMultiplier(fileBpm(), baseBpm);
    updateLeaderBpm(bpm);
    updateLeaderBeatDistance(beatDistance);
}

double SyncControl::determineBpmMultiplier(double myBpm, double targetBpm) const {
    if (myBpm == 0.0 || targetBpm == 0.0) {
        return kBpmUnity;
    }
    double unityRatio = myBpm / targetBpm;
    // the square root of 2 (1.414) is the
    // rate threshold that works vice versa for this and the target.
    double unityRatioSquare = unityRatio * unityRatio;
    if (unityRatioSquare > kBpmDouble) {
        return kBpmDouble;
    } else if (unityRatioSquare < kBpmHalve) {
        return kBpmHalve;
    }
    return kBpmUnity;
}

void SyncControl::updateTargetBeatDistance() {
    double targetDistance = m_unmultipliedTargetBeatDistance;
    if (kLogger.traceEnabled()) {
        kLogger.trace()
                << getGroup()
                << "SyncControl::updateTargetBeatDistance, unmult distance"
                << targetDistance << m_leaderBpmAdjustFactor;
    }

    // Determining the target distance is not as simple as x2 or /2.  Since one
    // of the beats is twice the length of the other, we need to know if the
    // position of the longer beat is past its halfway point.  e.g. 0.0 for the
    // long beat is 0.0 of the short beat, but 0.5 for the long beat is also
    // 0.0 for the short beat.
    if (m_leaderBpmAdjustFactor == kBpmDouble) {
        if (targetDistance >= 0.5) {
            targetDistance -= 0.5;
        }
        targetDistance *= kBpmDouble;
    } else if (m_leaderBpmAdjustFactor == kBpmHalve) {
        targetDistance *= kBpmHalve;
        // Our beat distance CO is still a buffer behind, so take the current value.
        if (m_pBpmControl->getBeatDistance(getSampleOfTrack().current) >= 0.5) {
            targetDistance += 0.5;
        }
    }
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::updateTargetBeatDistance, adjusted target is" << targetDistance;
    }
    m_pBpmControl->setTargetBeatDistance(targetDistance);
}

double SyncControl::getBpm() const {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::getBpm()"
                        << m_pBpm->get() << "/" << m_leaderBpmAdjustFactor;
    }
    return m_pBpm->get() / m_leaderBpmAdjustFactor;
}

void SyncControl::updateInstantaneousBpm(double bpm) {
    // Adjust the incoming bpm by the multiplier.
    m_pBpmControl->updateInstantaneousBpm(bpm * m_leaderBpmAdjustFactor);
}

void SyncControl::reportTrackPosition(double fractionalPlaypos) {
    // If we're close to the end, and leader, disable leader so we don't stop
    // the party.
    if (isLeader(getSyncMode()) && fractionalPlaypos >= 1.0) {
        m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_FOLLOWER);
    }
}

// called from an engine worker thread
void SyncControl::trackLoaded(TrackPointer pNewTrack) {
    mixxx::BeatsPointer pBeats;
    if (pNewTrack) {
        pBeats = pNewTrack->getBeats();
    }
    // This slot is fired by a new file is loaded or if the user
    // has adjusted the beatgrid.
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::trackLoaded";
    }

    VERIFY_OR_DEBUG_ASSERT(m_pLocalBpm) {
        // object not initialized
        return;
    }

    bool hadBeats = m_pBeats != nullptr;
    m_pBeats = pBeats;
    m_leaderBpmAdjustFactor = kBpmUnity;

    m_pBpmControl->updateLocalBpm();
    if (isSynchronized()) {
        if (!m_pBeats) {
            // If we were soft leader and now we have no beats, go to follower.
            // This is a bit of "enginesync" logic that has bled into this Syncable,
            // is there a better way to handle "soft leaders no longer have bpm"?
            if (getSyncMode() == SYNC_LEADER_SOFT) {
                m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_FOLLOWER);
            }
            return;
        }

        m_pBpmControl->syncTempo();
        if (!hadBeats) {
            // There is a chance we were beatless leader before, so we notify a basebpm change
            // to possibly reinit leader params.
            m_pChannel->getEngineBuffer()->requestSyncMode(getSyncMode());
            m_pEngineSync->notifyBaseBpmChanged(this, getBaseBpm());
        }
    }
}

void SyncControl::trackBeatsUpdated(mixxx::BeatsPointer pBeats) {
    // This slot is fired by if the user has adjusted the beatgrid.
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::trackBeatsUpdated";
    }

    m_pBeats = pBeats;
    m_leaderBpmAdjustFactor = kBpmUnity;

    SyncMode syncMode = getSyncMode();
    if (isLeader(syncMode)) {
        if (!m_pBeats) {
            // If the track was ejected or suddenly has no beats, we can no longer
            // be leader.
            m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_FOLLOWER);
        } else {
            // We should not change playback speed.
            m_pBpmControl->updateLocalBpm();
            m_pEngineSync->notifyBaseBpmChanged(this, getBaseBpm());
        }
    } else if (isFollower(syncMode)) {
        // If we were a follower, requesting sync mode refreshes
        // the soft leader -- if we went from having no bpm to having
        // a bpm, we might need to become leader.
        m_pChannel->getEngineBuffer()->requestSyncMode(syncMode);
        m_pBpmControl->updateLocalBpm();
    }
}

void SyncControl::slotControlPlay(double play) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << "SyncControl::slotControlPlay" << getSyncMode() << play;
    }
    m_pEngineSync->notifyPlayingAudible(this, play > 0.0 && m_audible);
}

void SyncControl::slotVinylControlChanged(double enabled) {
    if (enabled != 0 && getSyncMode() == SYNC_FOLLOWER) {
        // If vinyl control was enabled and we're a follower, disable sync mode.
        m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_NONE);
    }
}

void SyncControl::slotPassthroughChanged(double enabled) {
    if (enabled != 0 && isSynchronized()) {
        // If passthrough was enabled and sync was on, disable it.
        m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_NONE);
    }
}

void SyncControl::slotSyncModeChangeRequest(double state) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::slotSyncModeChangeRequest";
    }
    SyncMode mode = syncModeFromDouble(state);
    if (m_pPassthroughEnabled->toBool() && mode != SYNC_NONE) {
        qDebug() << "Disallowing enabling of sync mode when passthrough active";
    } else {
        m_pChannel->getEngineBuffer()->requestSyncMode(mode);
    }
}

void SyncControl::slotSyncLeaderEnabledChangeRequest(double state) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << "SyncControl::slotSyncLeaderEnabledChangeRequest" << getGroup();
    }
    SyncMode mode = getSyncMode();
    if (state > 0.0) {
        if (m_pPassthroughEnabled->toBool()) {
            qDebug() << "Disallowing enabling of sync mode when passthrough active";
            return;
        }
        m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_LEADER_EXPLICIT);
    } else {
        // Turning off leader goes back to follower mode.
        switch (mode) {
        case SYNC_LEADER_EXPLICIT:
            m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_LEADER_SOFT);
            break;
        case SYNC_LEADER_SOFT:
            m_pChannel->getEngineBuffer()->requestSyncMode(SYNC_FOLLOWER);
            break;
        default:
            return;
        }
    }
}

void SyncControl::slotSyncEnabledChangeRequest(double enabled) {
    if (kLogger.traceEnabled()) {
        kLogger.trace() << "SyncControl::slotSyncEnabledChangeRequest" << getGroup();
    }
    bool bEnabled = enabled > 0.0;

    // Allow a request for state change even if it's the same as the current
    // state.  We might have toggled on and off in the space of one buffer.
    if (bEnabled && m_pPassthroughEnabled->toBool()) {
        qDebug() << "Disallowing enabling of sync mode when passthrough active";
        return;
    }
    m_pChannel->getEngineBuffer()->requestEnableSync(bEnabled);
}

void SyncControl::setLocalBpm(double local_bpm) {
    if (local_bpm == m_prevLocalBpm.getValue()) {
        return;
    }
    m_prevLocalBpm.setValue(local_bpm);

    SyncMode syncMode = getSyncMode();
    if (syncMode <= SYNC_NONE) {
        return;
    }

    double bpm = local_bpm * m_pRateRatio->get();

    if (isFollower(syncMode)) {
        // In this case we need an update from the current leader to adjust
        // the rate that we continue with the leader BPM. If there is no
        // leader bpm, our bpm value is adopted and the m_leaderBpmAdjustFactor
        // is reset to 1;
        m_pEngineSync->requestBpmUpdate(this, bpm);
    } else {
        DEBUG_ASSERT(isLeader(syncMode));
        // We might have adopted an adjust factor when becoming leader.
        // Keep it when reporting our bpm.
        m_pEngineSync->notifyBaseBpmChanged(this, bpm / m_leaderBpmAdjustFactor);
    }
}

void SyncControl::updateAudible() {
    int channelIndex = m_pChannel->getChannelIndex();
    if (channelIndex >= 0) {
        CSAMPLE_GAIN gain = getEngineMaster()->getMasterGain(channelIndex);
        bool newAudible = gain > CSAMPLE_GAIN_ZERO;
        if (m_audible != newAudible) {
            m_audible = newAudible;
            m_pEngineSync->notifyPlayingAudible(this, m_pPlayButton->toBool() && m_audible);
        }
    }
}

void SyncControl::slotRateChanged() {
    double bpm = m_pLocalBpm ? m_pLocalBpm->get() * m_pRateRatio->get() : 0.0;
    if (kLogger.traceEnabled()) {
        kLogger.trace() << getGroup() << "SyncControl::slotRateChanged" << m_pRateRatio->get() << bpm;
    }
    if (bpm > 0 && isSynchronized()) {
        // When reporting our bpm, remove the multiplier so the leaders all
        // think the followers have the same bpm.
        m_pEngineSync->notifyRateChanged(this, bpm / m_leaderBpmAdjustFactor);
    }
}

void SyncControl::reportPlayerSpeed(double speed, bool scratching) {
    if (m_bOldScratching ^ scratching) {
        m_pEngineSync->notifyScratching(this, scratching);
        m_bOldScratching = scratching;
        // No need to disable sync mode while scratching, the engine won't
        // get confused.
    }
    // When reporting our speed, remove the multiplier so the leaders all
    // think the followers have the same bpm.
    double instantaneous_bpm = m_pLocalBpm->get() * speed / m_leaderBpmAdjustFactor;
    m_pEngineSync->notifyInstantaneousBpmChanged(this, instantaneous_bpm);
}

void SyncControl::notifySeek(double dNewPlaypos) {
    // qDebug() << "SyncControl::notifySeek" << dNewPlaypos;
    EngineControl::notifySeek(dNewPlaypos);
    m_pBpmControl->notifySeek(dNewPlaypos);
    updateTargetBeatDistance();
}

double SyncControl::fileBpm() const {
    mixxx::BeatsPointer pBeats = m_pBeats;
    if (pBeats) {
        return pBeats->getBpm();
    }
    return 0.0;
}
