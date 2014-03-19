#include "effects/effectrack.h"

#include "effects/effectsmanager.h"
#include "effects/effectchainmanager.h"
#include "engine/effects/engineeffectrack.h"

EffectRack::EffectRack(EffectsManager* pEffectsManager,
                       EffectChainManager* pEffectChainManager,
                       const unsigned int iRackNumber)
        : m_iRackNumber(iRackNumber),
          m_group(formatGroupString(m_iRackNumber)),
          m_pEffectsManager(pEffectsManager),
          m_pEffectChainManager(pEffectChainManager),
          m_controlNumEffectChainSlots(ConfigKey(m_group, "num_effectunits")),
          m_controlClearRack(ConfigKey(m_group, "clear")),
          m_pEngineEffectRack(new EngineEffectRack(iRackNumber)) {
    connect(&m_controlClearRack, SIGNAL(valueChanged(double)),
            this, SLOT(slotClearRack(double)));
    m_controlNumEffectChainSlots.connectValueChangeRequest(
            this, SLOT(slotNumEffectChainSlots(double)), Qt::AutoConnection);
    addToEngine();
}

EffectRack::~EffectRack() {
    removeFromEngine();
}

EngineEffectRack* EffectRack::getEngineEffectRack() {
    return m_pEngineEffectRack;
}

void EffectRack::addToEngine() {
    EffectsRequest* pRequest = new EffectsRequest();
    pRequest->type = EffectsRequest::ADD_EFFECT_RACK;
    pRequest->AddEffectRack.pRack = m_pEngineEffectRack;
    m_pEffectsManager->writeRequest(pRequest);

    // Add all effect chains.
    for (int i = 0; i < m_effectChainSlots.size(); ++i) {
        EffectChainSlotPointer pSlot = m_effectChainSlots[i];
        EffectChainPointer pChain = pSlot->getEffectChain();
        if (pChain) {
            // Add the effect to the engine.
            pChain->addToEngine(m_pEngineEffectRack, i);
            // Update its parameters in the engine.
            pChain->updateEngineState();
        }
    }
}

void EffectRack::removeFromEngine() {
    // Order doesn't matter when removing.
    for (int i = 0; i < m_effectChainSlots.size(); ++i) {
        EffectChainSlotPointer pSlot = m_effectChainSlots[i];
        EffectChainPointer pChain = pSlot->getEffectChain();
        if (pChain) {
            pChain->removeFromEngine(m_pEngineEffectRack);
        }
    }

    EffectsRequest* pRequest = new EffectsRequest();
    pRequest->type = EffectsRequest::REMOVE_EFFECT_RACK;
    pRequest->RemoveEffectRack.pRack = m_pEngineEffectRack;
    m_pEffectsManager->writeRequest(pRequest);
}

void EffectRack::registerGroup(const QString& group) {
    foreach (EffectChainSlotPointer pChainSlot, m_effectChainSlots) {
        pChainSlot->registerGroup(group);
    }
}

void EffectRack::slotNumEffectChainSlots(double v) {
    // Ignore sets to num_effectchain_slots
    qDebug() << debugString() << "slotNumEffectChainSlots" << v;
    qDebug() << "WARNING: num_effectchain_slots is a read-only control.";
}

void EffectRack::slotClearRack(double v) {
    if (v > 0) {
        foreach (EffectChainSlotPointer pChainSlot, m_effectChainSlots) {
            pChainSlot->clear();
        }
    }
}

int EffectRack::numEffectChainSlots() const {
    return m_effectChainSlots.size();
}

EffectChainSlotPointer EffectRack::addEffectChainSlot() {
    EffectChainSlot* pChainSlot =
            new EffectChainSlot(this, m_iRackNumber, m_effectChainSlots.size());

    // TODO(rryan) How many should we make default? They create controls that
    // the GUI may rely on, so the choice is important to communicate to skin
    // designers.
    // TODO(rryan): This should not be done here.
    pChainSlot->addEffectSlot();
    pChainSlot->addEffectSlot();
    pChainSlot->addEffectSlot();
    pChainSlot->addEffectSlot();

    connect(pChainSlot, SIGNAL(nextChain(const unsigned int, EffectChainPointer)),
            this, SLOT(loadNextChain(const unsigned int, EffectChainPointer)));
    connect(pChainSlot, SIGNAL(prevChain(const unsigned int, EffectChainPointer)),
            this, SLOT(loadPrevChain(const unsigned int, EffectChainPointer)));

    // Register all the existing channels with the new EffectChain
    const QSet<QString>& registeredGroups =
            m_pEffectChainManager->registeredGroups();
    foreach (const QString& group, registeredGroups) {
        pChainSlot->registerGroup(group);
    }

    EffectChainSlotPointer pChainSlotPointer = EffectChainSlotPointer(pChainSlot);
    m_effectChainSlots.append(pChainSlotPointer);
    m_controlNumEffectChainSlots.setAndConfirm(
            m_controlNumEffectChainSlots.get() + 1);
    return pChainSlotPointer;
}

EffectChainSlotPointer EffectRack::getEffectChainSlot(int i) {
    if (i < 0 || i >= m_effectChainSlots.size()) {
        qDebug() << "WARNING: Invalid index for getEffectChainSlot";
        return EffectChainSlotPointer();
    }
    return m_effectChainSlots[i];
}

void EffectRack::loadNextChain(const unsigned int iChainSlotNumber,
                               EffectChainPointer pLoadedChain) {
    if (pLoadedChain) {
        // TODO(rryan) GC pLoadedChain.
        pLoadedChain->removeFromEngine(m_pEngineEffectRack);
        pLoadedChain = pLoadedChain->prototype();
    }

    EffectChainPointer pNextChain = m_pEffectChainManager->getNextEffectChain(
        pLoadedChain);

    pNextChain = EffectChain::clone(pNextChain);
    if (pNextChain) {
        pNextChain->addToEngine(m_pEngineEffectRack, iChainSlotNumber);
        pNextChain->updateEngineState();
    }
    m_effectChainSlots[iChainSlotNumber]->loadEffectChain(pNextChain);
}


void EffectRack::loadPrevChain(const unsigned int iChainSlotNumber,
                               EffectChainPointer pLoadedChain) {
    if (pLoadedChain) {
        pLoadedChain->removeFromEngine(m_pEngineEffectRack);
        // TODO(rryan) GC pLoadedChain.
        pLoadedChain = pLoadedChain->prototype();
    }

    EffectChainPointer pPrevChain = m_pEffectChainManager->getPrevEffectChain(
        pLoadedChain);

    pPrevChain = EffectChain::clone(pPrevChain);
    if (pPrevChain) {
        pPrevChain->addToEngine(m_pEngineEffectRack, iChainSlotNumber);
        pPrevChain->updateEngineState();
    }
    m_effectChainSlots[iChainSlotNumber]->loadEffectChain(pPrevChain);
}
