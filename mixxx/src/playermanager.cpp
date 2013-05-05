// playermanager.cpp
// Created 6/1/2010 by RJ Ryan (rryan@mit.edu)

#include "playermanager.h"

#include "controlobject.h"
#include "trackinfoobject.h"
#include "deck.h"
#include "sampler.h"
#include "previewdeck.h"
#include "analyserqueue.h"
#include "controlobject.h"
#include "samplerbank.h"
#include "library/library.h"
#include "library/trackcollection.h"
#include "engine/enginemaster.h"
#include "soundmanager.h"
#include "vinylcontrol/vinylcontrolmanager.h"
#include "util/stat.h"

PlayerManager::PlayerManager(ConfigObject<ConfigValue>* pConfig,
                             SoundManager* pSoundManager,
                             EngineMaster* pEngine,
                             VinylControlManager* pVCManager) :
        m_pConfig(pConfig),
        m_pSoundManager(pSoundManager),
        m_pEngine(pEngine),
        m_pVCManager(pVCManager),
        // NOTE(XXX) LegacySkinParser relies on these controls being COs and
        // not COTMs listening to a CO.
        m_pAnalyserQueue(NULL),
        m_pCONumDecks(new ControlObject(ConfigKey("[Master]", "num_decks"), true, true)),
        m_pCONumSamplers(new ControlObject(ConfigKey("[Master]", "num_samplers"), true, true)),
        m_pCONumPreviewDecks(new ControlObject(ConfigKey("[Master]", "num_preview_decks"), true, true)) {

    connect(m_pCONumDecks, SIGNAL(valueChanged(double)),
            this, SLOT(slotNumDecksControlChanged(double)));
    connect(m_pCONumSamplers, SIGNAL(valueChanged(double)),
            this, SLOT(slotNumSamplersControlChanged(double)));
    connect(m_pCONumPreviewDecks, SIGNAL(valueChanged(double)),
            this, SLOT(slotNumPreviewDecksControlChanged(double)));

    // This is parented to the PlayerManager so does not need to be deleted
    SamplerBank* pSamplerBank = new SamplerBank(this);
    Q_UNUSED(pSamplerBank);

    // Redundant
    m_pCONumDecks->set(0);
    m_pCONumSamplers->set(0);
    m_pCONumPreviewDecks->set(0);

    // register the engine's outputs
    m_pSoundManager->registerOutput(AudioOutput(AudioOutput::MASTER),
            m_pEngine);
    m_pSoundManager->registerOutput(AudioOutput(AudioOutput::HEADPHONES),
            m_pEngine);
}

PlayerManager::~PlayerManager() {
    // No need to delete anything because they are all parented to us and will
    // be destroyed when we are destroyed.
    m_players.clear();
    m_decks.clear();
    m_samplers.clear();

    delete m_pCONumSamplers;
    delete m_pCONumDecks;
    delete m_pCONumPreviewDecks;
    if (m_pAnalyserQueue) {
        delete m_pAnalyserQueue;
    }
}

void PlayerManager::bindToLibrary(Library* pLibrary) {
    connect(pLibrary, SIGNAL(loadTrackToPlayer(TrackPointer, QString, bool)),
            this, SLOT(slotLoadTrackToPlayer(TrackPointer, QString, bool)));
    connect(pLibrary, SIGNAL(loadTrack(TrackPointer)),
            this, SLOT(slotLoadTrackIntoNextAvailableDeck(TrackPointer)));
    connect(this, SIGNAL(loadLocationToPlayer(QString, QString)),
            pLibrary, SLOT(slotLoadLocationToPlayer(QString, QString)));

    m_pAnalyserQueue = AnalyserQueue::createDefaultAnalyserQueue(m_pConfig,
            pLibrary->getTrackCollection());

    // Connect the player to the analyser queue so that loaded tracks are
    // analysed.
    foreach(Deck* pDeck, m_decks) {
        connect(pDeck, SIGNAL(newTrackLoaded(TrackPointer)),
                m_pAnalyserQueue, SLOT(slotAnalyseTrack(TrackPointer)));
    }

    // Connect the player to the analyser queue so that loaded tracks are
    // analysed.
    foreach(Sampler* pSampler, m_samplers) {
        connect(pSampler, SIGNAL(newTrackLoaded(TrackPointer)),
                m_pAnalyserQueue, SLOT(slotAnalyseTrack(TrackPointer)));
    }

    // Connect the player to the analyser queue so that loaded tracks are
    // analysed.
    foreach(PreviewDeck* pPreviewDeck, m_preview_decks) {
        connect(pPreviewDeck, SIGNAL(newTrackLoaded(TrackPointer)),
                m_pAnalyserQueue, SLOT(slotAnalyseTrack(TrackPointer)));
    }
}

// static
unsigned int PlayerManager::numDecks() {
    // We do this to cache the control once it is created so callers don't incur
    // a hashtable lookup every time they call this.
    static ControlObject* pNumCO = NULL;
    if (pNumCO == NULL) {
        pNumCO = ControlObject::getControl(
            ConfigKey("[Master]", "num_decks"));
    }
    return pNumCO ? pNumCO->get() : 0;
}

// static
bool PlayerManager::isDeckGroup(const QString& group, int* number) {
    if (!group.startsWith("[Channel")) {
        return false;
    }

    bool ok = false;
    int deckNum = group.mid(8,group.lastIndexOf("]")-8).toInt(&ok);
    if (!ok || deckNum <= 0) {
        return false;
    }
    if (number != NULL) {
        *number = deckNum;
    }
    return true;
}

// static
unsigned int PlayerManager::numSamplers() {
    // We do this to cache the control once it is created so callers don't incur
    // a hashtable lookup every time they call this.
    static ControlObject* pNumCO = NULL;
    if (pNumCO == NULL) {
        pNumCO = ControlObject::getControl(
            ConfigKey("[Master]", "num_samplers"));
    }
    return pNumCO ? pNumCO->get() : 0;
}

// static
unsigned int PlayerManager::numPreviewDecks() {
    // We do this to cache the control once it is created so callers don't incur
    // a hashtable lookup every time they call this.
    static ControlObject* pNumCO = NULL;
    if (pNumCO == NULL) {
        pNumCO = ControlObject::getControl(
            ConfigKey("[Master]", "num_preview_decks"));
    }
    return pNumCO ? pNumCO->get() : 0;
}

void PlayerManager::slotNumDecksControlChanged(double v) {
    m_pCONumDecks->set(m_decks.size());
    int num = v;
    if (num == m_decks.size())
        return;
        
    while (num < m_decks.size()) {
        QString group = groupForDeck(m_decks.size() - 1);
        m_players.remove(group);
        m_decks.removeLast();
    }

    while (m_decks.size() < num) {
        addDeck(num);
    }
    
	// Redistribute decks left and right based on new count.
    QList<Deck*>::iterator it = m_decks.begin();
    for (int i = 1; it != m_decks.end(); ++i, ++it) {
        if (i > m_decks.size() / 2) {
            ControlObject::getControl(ConfigKey(QString("[Channel%1]").arg(i), 
                                                "orientation"))->set(EngineChannel::RIGHT);
        } else {
            ControlObject::getControl(ConfigKey(QString("[Channel%1]").arg(i), 
                                                "orientation"))->set(EngineChannel::LEFT);
        }
    }
    
    m_pCONumDecks->set(m_decks.size());
}

void PlayerManager::slotNumSamplersControlChanged(double v) {
    m_pCONumSamplers->set(m_samplers.size());
    int num = v;
    while (num < m_samplers.size()) {
        QString group = groupForSampler(m_samplers.size() - 1);
        m_players.remove(group);
        m_samplers.removeLast();
    }

    while (m_samplers.size() < num) {
        addSampler();
    }
    m_pCONumSamplers->set(m_samplers.size());
}

void PlayerManager::slotNumPreviewDecksControlChanged(double v) {
    m_pCONumPreviewDecks->set(m_preview_decks.size());
    int num = v;
    while (num < m_preview_decks.size()) {
        QString group = groupForPreviewDeck(m_preview_decks.size() - 1);
        m_players.remove(group);
        m_preview_decks.removeLast();
    }

    while (m_preview_decks.size() < num) {
        addPreviewDeck();
    }
    m_pCONumPreviewDecks->set(m_preview_decks.size());
}

Deck* PlayerManager::addDeck(int total_decks) {
    QString group = groupForDeck(numDecks());
    int number = numDecks() + 1;

    EngineChannel::ChannelOrientation orientation = EngineChannel::LEFT;
    if (number > total_decks / 2) {
        orientation = EngineChannel::RIGHT;
    }

    Deck* pDeck = new Deck(this, m_pConfig, m_pEngine, orientation, group);
    if (m_pAnalyserQueue) {
        connect(pDeck, SIGNAL(newTrackLoaded(TrackPointer)),
                m_pAnalyserQueue, SLOT(slotAnalyseTrack(TrackPointer)));
    }

    Q_ASSERT(!m_players.contains(group));
    m_players[group] = pDeck;
    m_decks.append(pDeck);
    m_pCONumDecks->add(1);

    // Register the deck output with SoundManager (deck is 0-indexed to SoundManager)
    m_pSoundManager->registerOutput(
        AudioOutput(AudioOutput::DECK, 0, number-1), m_pEngine);

    // If vinyl control manager exists, then register a VC input with
    // SoundManager.
    if (m_pVCManager) {
        m_pSoundManager->registerInput(
            AudioInput(AudioInput::VINYLCONTROL, 0, number-1), m_pVCManager);
    }

    return pDeck;
}

Sampler* PlayerManager::addSampler() {
    QString group = groupForSampler(numSamplers());

    // All samplers are in the center
    EngineChannel::ChannelOrientation orientation = EngineChannel::CENTER;

    Sampler* pSampler = new Sampler(this, m_pConfig, m_pEngine, orientation, group);
    if (m_pAnalyserQueue) {
        connect(pSampler, SIGNAL(newTrackLoaded(TrackPointer)),
                m_pAnalyserQueue, SLOT(slotAnalyseTrack(TrackPointer)));
    }

    Q_ASSERT(!m_players.contains(group));
    m_players[group] = pSampler;
    m_samplers.append(pSampler);
    m_pCONumSamplers->add(1);

    return pSampler;
}

PreviewDeck* PlayerManager::addPreviewDeck() {
    QString group = groupForPreviewDeck(numPreviewDecks());

    // All preview decks are in the center
    EngineChannel::ChannelOrientation orientation = EngineChannel::CENTER;

    PreviewDeck* pPreviewDeck = new PreviewDeck(this, m_pConfig, m_pEngine, orientation, group);
    if (m_pAnalyserQueue) {
        connect(pPreviewDeck, SIGNAL(newTrackLoaded(TrackPointer)),
                m_pAnalyserQueue, SLOT(slotAnalyseTrack(TrackPointer)));
    }

    Q_ASSERT(!m_players.contains(group));
    m_players[group] = pPreviewDeck;
    m_preview_decks.append(pPreviewDeck);
    m_pCONumPreviewDecks->add(1);
    return pPreviewDeck;
}

BaseTrackPlayer* PlayerManager::getPlayer(QString group) const {
    if (m_players.contains(group)) {
        return m_players[group];
    }
    return NULL;
}


Deck* PlayerManager::getDeck(unsigned int deck) const {
    if (deck < 1 || deck > numDecks()) {
        qWarning() << "Warning PlayerManager::getDeck() called with invalid index: "
                   << deck;
        return NULL;
    }
    return m_decks[deck - 1];
}

PreviewDeck* PlayerManager::getPreviewDeck(unsigned int libPreviewPlayer) const {
    if (libPreviewPlayer < 1 || libPreviewPlayer > numPreviewDecks()) {
        qWarning() << "Warning PlayerManager::getPreviewDeck() called with invalid index: "
                   << libPreviewPlayer;
        return NULL;
    }
    return m_preview_decks[libPreviewPlayer - 1];
}

Sampler* PlayerManager::getSampler(unsigned int sampler) const {
    if (sampler < 1 || sampler > numSamplers()) {
        qWarning() << "Warning PlayerManager::getSampler() called with invalid index: "
                   << sampler;
        return NULL;
    }
    return m_samplers[sampler - 1];
}

void PlayerManager::slotLoadTrackToPlayer(TrackPointer pTrack, QString group, bool play) {
    BaseTrackPlayer* pPlayer = getPlayer(group);

    if (pPlayer == NULL) {
        qWarning() << "Invalid group argument " << group << " to slotLoadTrackToPlayer.";
        return;
    }

    pPlayer->slotLoadTrack(pTrack, play);
}

void PlayerManager::slotLoadToPlayer(QString location, QString group) {
    // The library will get the track and then signal back to us to load the
    // track via slotLoadTrackToPlayer.
    emit(loadLocationToPlayer(location, group));
}

void PlayerManager::slotLoadToDeck(QString location, int deck) {
    slotLoadToPlayer(location, groupForDeck(deck-1));
}

void PlayerManager::slotLoadToPreviewDeck(QString location, int previewDeck) {
    slotLoadToPlayer(location, groupForPreviewDeck(previewDeck-1));
}

void PlayerManager::slotLoadToSampler(QString location, int sampler) {
    slotLoadToPlayer(location, groupForSampler(sampler-1));
}

void PlayerManager::slotLoadTrackIntoNextAvailableDeck(TrackPointer pTrack)
{
    QList<Deck*>::iterator it_b = m_decks.begin();
    QList<Deck*>::iterator it_e = m_decks.end();
    bool try_b = true;
    while (it_b != it_e) {
        Deck* pDeck = *it_b;
        if (try_b) {
            ++it_b;
        } else {
            --it_e;
            pDeck = *it_e;
        }
        try_b = !try_b;
        
        ControlObject* vinylControlEnabled =
                ControlObject::getControl(ConfigKey(pDeck->getGroup(), 
                                          "vinylcontrol_enabled"));
                                          
        if (vinylControlEnabled && vinylControlEnabled->get())
        {
            // With vinyl, we can't rely solely on play-button status.  Load if
            // either no track is loaded, or if the scratch2 rate is very low.
            TrackPointer tp = pDeck->getLoadedTrack();
            ControlObject* vinylRate =
                    ControlObject::getControl(ConfigKey(pDeck->getGroup(), 
                                              "scratch2"));
            if (vinylRate && (tp == NULL || fabs(vinylRate->get()) < 0.1 )) {
                pDeck->slotLoadTrack(pTrack, false);
                break;
            }
        }
        else
        {
            ControlObject* playControl =
                    ControlObject::getControl(ConfigKey(pDeck->getGroup(), "play"));
            if (playControl && playControl->get() != 1. ) {
                pDeck->slotLoadTrack(pTrack, false);
                break;
            }
        }
    }
}

void PlayerManager::slotLoadTrackIntoNextAvailableSampler(TrackPointer pTrack)
{
    QList<Sampler*>::iterator it = m_samplers.begin();
    while (it != m_samplers.end()) {
        Sampler* pSampler = *it;
        ControlObject* playControl =
                ControlObject::getControl(ConfigKey(pSampler->getGroup(), "play"));
        if (playControl && playControl->get() != 1.) {
            pSampler->slotLoadTrack(pTrack, false);
            break;
        }
        it++;
    }
}


