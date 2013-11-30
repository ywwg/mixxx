/***************************************************************************
                          enginesync.cpp  -  master sync control for
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

#include <QStringList>

#include "controlobject.h"
#include "controlpotmeter.h"
#include "controlpushbutton.h"
#include "engine/enginebuffer.h"
#include "engine/enginechannel.h"
#include "engine/enginecontrol.h"
#include "engine/enginesync.h"
#include "engine/ratecontrol.h"

static const char* kMasterSyncGroup = "[Master]";

EngineSync::EngineSync(ConfigObject<ConfigValue>* _config)
        : EngineControl(kMasterSyncGroup, _config),
          m_pConfig(_config),
          m_pChannelMaster(NULL),
          m_sSyncSource(""),
          m_dPseudoBufferPos(0.0f) {
    m_pMasterBeatDistance = new ControlObject(ConfigKey(kMasterSyncGroup, "beat_distance"));

    m_pSampleRate = ControlObject::getControl(ConfigKey(kMasterSyncGroup, "samplerate"));
    connect(m_pSampleRate, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotSampleRateChanged(double)),
            Qt::DirectConnection);
    connect(m_pSampleRate, SIGNAL(valueChanged(double)),
            this, SLOT(slotSampleRateChanged(double)),
            Qt::DirectConnection);

    if (m_pSampleRate->get()) {
        m_pSampleRate->set(44100);
    }

    m_pMasterBpm = new ControlObject(ConfigKey(kMasterSyncGroup, "sync_bpm"));
    // Initialize with a default value (will get overridden by config).
    m_pMasterBpm->set(124.0);
    connect(m_pMasterBpm, SIGNAL(valueChanged(double)),
            this, SLOT(slotMasterBpmChanged(double)),
            Qt::DirectConnection);
    connect(m_pMasterBpm, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotMasterBpmChanged(double)),
            Qt::DirectConnection);

    m_pSyncInternalEnabled = new ControlPushButton(ConfigKey(kMasterSyncGroup, "sync_master"));
    m_pSyncInternalEnabled->setButtonMode(ControlPushButton::TOGGLE);
    connect(m_pSyncInternalEnabled, SIGNAL(valueChanged(double)),
            this, SLOT(slotInternalMasterChanged(double)),
            Qt::DirectConnection);

    m_pSyncRateSlider = new ControlPotmeter(ConfigKey(kMasterSyncGroup, "sync_slider"), 40.0, 200.0);
    connect(m_pSyncRateSlider, SIGNAL(valueChanged(double)),
            this, SLOT(slotSyncRateSliderChanged(double)),
            Qt::DirectConnection);
    connect(m_pSyncRateSlider, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotSyncRateSliderChanged(double)),
            Qt::DirectConnection);

    updateSamplesPerBeat();
}

EngineSync::~EngineSync() {
    // We use the slider value because that is never set to 0.0.
    m_pConfig->set(ConfigKey("[Master]", "sync_bpm"), ConfigValue(m_pSyncRateSlider->get()));
    delete m_pMasterBpm;
    delete m_pMasterBeatDistance;
    delete m_pSyncRateSlider;
}

void EngineSync::addChannel(EngineChannel* pChannel) {
    foreach (RateControl* pRate, m_ratecontrols) {
        if (pRate->getGroup() == pChannel->getGroup()) {
            pRate->setEngineChannel(pChannel);
            return;
        }
    }
    qDebug() << "No RateControl found for group (probably not a playback deck) " << pChannel->getGroup();
}

void EngineSync::addDeck(RateControl *pNewRate) {
    foreach (RateControl* pRate, m_ratecontrols) {
        if (pRate->getGroup() == pNewRate->getGroup()) {
            qDebug() << "EngineSync: already has channel for" << pRate->getGroup() << ", replacing";
            pRate = pNewRate;
            return;
        }
    }
    m_ratecontrols.append(pNewRate);
}

void EngineSync::disableCurrentMaster() {
    RateControl* pOldChannelMaster = m_pChannelMaster;
    if (m_sSyncSource == kMasterSyncGroup) {
        m_pSyncInternalEnabled->set(false);
    }
    if (pOldChannelMaster) {
        ControlObject* pSourceRateEngine =
                ControlObject::getControl(ConfigKey(pOldChannelMaster->getGroup(), "rateEngine"));
        if (pSourceRateEngine) {
            disconnect(pSourceRateEngine, SIGNAL(valueChangedFromEngine(double)),
                       this, SLOT(slotSourceRateEngineChanged(double)));
        }
        ControlObject* pSourceBpm =
                ControlObject::getControl(ConfigKey(pOldChannelMaster->getGroup(), "bpm"));
        if (pSourceBpm) {
            disconnect(pSourceBpm, SIGNAL(valueChangedFromEngine(double)),
                       this, SLOT(slotSourceBpmChanged(double)));
        }
        ControlObject* pSourceBeatDistance = pOldChannelMaster->getBeatDistanceControl();
        if (pSourceBeatDistance) {
            disconnect(pSourceBeatDistance, SIGNAL(valueChangedFromEngine(double)),
                       this, SLOT(slotSourceBeatDistanceChanged(double)));
        }
        pOldChannelMaster->setMode(SYNC_SLAVE);
    }
    m_sSyncSource = "";
    m_pChannelMaster = NULL;
}

void EngineSync::setMaster(const QString& group) {
    // Convenience function that can split out to either set internal
    // or set deck master.
    if (group == kMasterSyncGroup) {
        setInternalMaster();
    } else {
        RateControl* pRateControl = getRateControlForGroup(group);
        if (!setChannelMaster(pRateControl)) {
            qDebug() << "WARNING: failed to set selected master" << group << ", going with Internal instead";
            setInternalMaster();
        } else {
            pRateControl->setMode(SYNC_MASTER);
        }
    }
}

void EngineSync::setInternalMaster() {
    if (m_sSyncSource == kMasterSyncGroup) {
        return;
    }
    double master_bpm = m_pMasterBpm->get();
    if (master_bpm != 0) {
        m_pSyncRateSlider->set(master_bpm);
    }
    QString old_master = m_sSyncSource;
    resetInternalBeatDistance();
    disableCurrentMaster();
    m_sSyncSource = kMasterSyncGroup;
    updateSamplesPerBeat();

    // This is all we have to do, we'll start using the pseudoposition right away.
    m_pSyncInternalEnabled->set(true);
}

bool EngineSync::setChannelMaster(RateControl* pRateControl) {
    if (!pRateControl) {
        return false;
    }

    // If a channel is master, disable it.
    if (m_sSyncSource == pRateControl->getGroup()) {
        return true;
    }
    disableCurrentMaster();

    // Only accept channels with an EngineBuffer.
    EngineChannel* pChannel = pRateControl->getChannel();
    if (!pChannel || !pChannel->getEngineBuffer()) {
        return false;
    }

    const QString& group = pChannel->getGroup();
    m_sSyncSource = group;

    // Only consider channels that have a track loaded and are in the master
    // mix.
    m_pChannelMaster = pRateControl;

    qDebug() << "Setting up master " << m_sSyncSource;

    ControlObject *pSourceRateEngine =
            ControlObject::getControl(ConfigKey(pRateControl->getGroup(), "rateEngine"));
    Q_ASSERT(pSourceRateEngine);
    connect(pSourceRateEngine, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotSourceRateEngineChanged(double)),
            Qt::DirectConnection);

    ControlObject *pSourceBpm =
            ControlObject::getControl(ConfigKey(pRateControl->getGroup(), "bpm"));
    Q_ASSERT(pSourceBpm);
    connect(pSourceBpm, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotSourceBpmChanged(double)),
            Qt::DirectConnection);

    ControlObject *pSourceBeatDistance = pRateControl->getBeatDistanceControl();
    Q_ASSERT(pSourceBeatDistance);
    connect(pSourceBeatDistance, SIGNAL(valueChangedFromEngine(double)),
            this, SLOT(slotSourceBeatDistanceChanged(double)),
            Qt::DirectConnection);

    // Reset internal beat distance to equal the new master
    resetInternalBeatDistance();

    m_pSyncInternalEnabled->set(false);
    slotSourceRateEngineChanged(pSourceRateEngine->get());
    slotSourceBpmChanged(pSourceBpm->get());

    return true;
}

void EngineSync::chooseNewMaster(const QString& dontpick) {
    foreach (RateControl* pRateControl, m_ratecontrols) {
        const QString& group = pRateControl->getGroup();
        if (group == dontpick) {
            continue;
        }

        double sync_mode = pRateControl->getMode();
        if (sync_mode == SYNC_MASTER) {
            qDebug() << "Already have a new master" << group;
            return;
        } else if (sync_mode == SYNC_NONE) {
            continue;
        }

        EngineChannel* pChannel = pRateControl->getChannel();
        if (pChannel && pChannel->isActive() && pChannel->isMaster()) {
            EngineBuffer* pBuffer = pChannel->getEngineBuffer();
            if (pBuffer && pBuffer->getBpm() > 0) {
                // If the channel is playing then go with it immediately.
                if (fabs(pBuffer->getRate()) > 0) {
                    pRateControl->setMode(SYNC_MASTER);
                    setChannelSyncMode(pRateControl, SYNC_MASTER);
                    return;
                }
            }
        }
    }
    if (dontpick != kMasterSyncGroup) {
        setInternalMaster();
    }
}

void EngineSync::setChannelRateSlider(RateControl* pRateControl, double new_bpm) {
    if (pRateControl->getMode() == SYNC_MASTER) {
        m_pSyncRateSlider->set(new_bpm);
        m_pMasterBpm->set(new_bpm);
    }
}

void EngineSync::setChannelSyncMode(RateControl* pRateControl, int state) {
    if (!pRateControl) {
        return;
    }

    const QString& group = pRateControl->getGroup();
    const bool channelIsMaster = m_sSyncSource == group;

    // In the following logic, m_sSyncSource acts like "previous sync source".
    if (state == SYNC_MASTER) {
        // TODO(owilliams): should we reject requests to be master from
        // non-playing decks?  If so, then that creates a weird situation on
        // startup where the user can't turn on Master.

        // If setting this channel as master fails, pick a new master.
        if (!setChannelMaster(pRateControl)) {
            chooseNewMaster(group);
        }
    } else if (state == SYNC_SLAVE) {
        // Was this deck master before?  If so do a handoff.
        if (channelIsMaster) {
            // Choose a new master, but don't pick the current one.
            chooseNewMaster(group);
        } else if (m_sSyncSource == "") {
            // If there is no current master, use internal master.
            setInternalMaster();
        }
    } else {
        // if we were the master, choose a new one.
        if (channelIsMaster) {
            chooseNewMaster("");
        }
        pRateControl->setMode(SYNC_NONE);
    }
}

void EngineSync::slotSourceRateEngineChanged(double rate_engine) {
    // Master buffer can be null due to timing issues
    if (m_pChannelMaster) {
        // This will trigger all of the slaves to change rate.
        m_pMasterBpm->set(rate_engine * m_pChannelMaster->getFileBpm());
    }
}

void EngineSync::slotSourceBpmChanged(double bpm) {
    // Master buffer can be null due to timing issues
    if (m_pChannelMaster) {
        m_pSyncRateSlider->set(bpm);
    }
}

void EngineSync::slotSourceBeatDistanceChanged(double beat_dist) {
    // Pass it on to slaves and update internal position marker.
    m_pMasterBeatDistance->set(beat_dist);
    setPseudoPosition(beat_dist);
}


void EngineSync::slotSyncRateSliderChanged(double new_bpm) {
    if (m_sSyncSource == kMasterSyncGroup && m_pMasterBpm->get() != new_bpm) {
        m_pMasterBpm->set(new_bpm);
    }
}

void EngineSync::slotMasterBpmChanged(double new_bpm) {
    if (new_bpm != m_pMasterBpm->get()) {
//        if (m_sSyncSource != kMasterSyncGroup) {
//            // XXX(Owen):
//            // it looks like this is Good Enough for preventing accidental
//            // tweaking of rate.  But maybe it should set master to internal?

//            // Changing to internal is weird, feels like a bug having master
//            // designation turn off
//            // setInternalMaster();

//            // how about just setting the bpm value for the deck master?
//            // problem with that is here we have bpm, but deck expects
//            // a percentage.  Let's keep this to "no you can't do that" for now

//            // TODO: Use CO validation instead of this pattern.
//            qDebug() << "not master, reset " <<m_sSyncSource << " " << kMasterSyncGroup;
//            m_pMasterBpm->set(m_dMasterBpm);
//            return;
//        }
        updateSamplesPerBeat();

        // This change could hypothetically push us over distance 1.0, so check
        // XXX: is this code correct?  I think it'll work but it seems off
        if (m_dSamplesPerBeat <= 0) {
            qDebug() << "ERROR: Calculated <= 0 samples per beat which is impossible.  Forcibly "
                     << "setting to about 124bpm at 44.1Khz.";
            m_dSamplesPerBeat = 21338;
        }
        while (m_dPseudoBufferPos >= m_dSamplesPerBeat) {
            m_dPseudoBufferPos -= m_dSamplesPerBeat;
        }
    }
}

void EngineSync::slotSampleRateChanged(double srate) {
    int new_rate = static_cast<int>(srate);
    double internal_position = getInternalBeatDistance();
    // Recalculate pseudo buffer position based on new sample rate.
    m_dPseudoBufferPos = new_rate * internal_position / m_dSamplesPerBeat;
    updateSamplesPerBeat();
}

void EngineSync::slotInternalMasterChanged(double state) {
    if (state) {
        setInternalMaster();
    } else {
        // Internal has been turned off. Pick a slave.
        m_sSyncSource = "";
        chooseNewMaster(kMasterSyncGroup);
    }
}

double EngineSync::getInternalBeatDistance() const {
    // Returns number of samples distance from the last beat.
    if (m_dPseudoBufferPos < 0) {
        qDebug() << "ERROR: Internal beat distance should never be less than zero";
        return 0.0;
    }
    return m_dPseudoBufferPos / m_dSamplesPerBeat;
}

void EngineSync::resetInternalBeatDistance() {
    ControlObject* pSourceBeatDistance = m_pChannelMaster ?
            m_pChannelMaster->getBeatDistanceControl() : NULL;
    double beat_distance = pSourceBeatDistance ? pSourceBeatDistance->get() : 0;

    m_dPseudoBufferPos = beat_distance * m_dSamplesPerBeat;
    qDebug() << "Resetting internal beat distance to new master"
             << m_dPseudoBufferPos << " " << beat_distance;
}

void EngineSync::updateSamplesPerBeat() {
    //to get samples per beat, do:
    //
    // samples   samples     60 seconds     minutes
    // ------- = -------  *  ----------  *  -------
    //   beat    second       1 minute       beats

    // that last term is 1 over bpm.
    double master_bpm = m_pMasterBpm->get();
    double sample_rate = m_pSampleRate->get();
    if (master_bpm == 0) {
        m_dSamplesPerBeat = sample_rate;
        return;
    }
    m_dSamplesPerBeat = (sample_rate * 60.0) / master_bpm;
    if (m_dSamplesPerBeat <= 0) {
        qDebug() << "WARNING: Tried to set samples per beat <=0";
        m_dSamplesPerBeat = sample_rate;
    }
}

void EngineSync::process(int bufferSize) {
    // EngineMaster calls this function, it is used to keep track of the
    // internal clock (when there is no other master like a deck or MIDI) the
    // pseudo position is a double because we want to be precise, and beats may
    // not line up exactly with samples.

    if (m_sSyncSource != kMasterSyncGroup) {
        // We don't care, it will get set in setPseudoPosition.
        return;
    }

    m_dPseudoBufferPos += bufferSize / 2; // stereo samples, so divide by 2

    // Can't use mod because we're in double land.
    if (m_dSamplesPerBeat <= 0) {
        qDebug() << "ERROR: Calculated <= 0 samples per beat which is impossible.  Forcibly "
                 << "setting to about 124 bpm at 44.1Khz.";
        m_dSamplesPerBeat = 21338;
    }
    while (m_dPseudoBufferPos >= m_dSamplesPerBeat) {
        m_dPseudoBufferPos -= m_dSamplesPerBeat;
    }

    m_pMasterBeatDistance->set(getInternalBeatDistance());
}

void EngineSync::setPseudoPosition(double percent) {
    m_dPseudoBufferPos = percent * m_dSamplesPerBeat;
}

EngineChannel* EngineSync::getMaster() const {
    return m_pChannelMaster ? m_pChannelMaster->getChannel() : NULL;
}

RateControl* EngineSync::getRateControlForGroup(const QString& group) {
    foreach (RateControl* pChannel, m_ratecontrols) {
        if (pChannel->getGroup() == group) {
            return pChannel;
        }
    }
    return NULL;
}
