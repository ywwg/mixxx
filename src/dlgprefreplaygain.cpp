#include "dlgprefreplaygain.h"

#include "controlobject.h"

#define CONFIG_KEY "[ReplayGain]"


DlgPrefReplayGain::DlgPrefReplayGain(QWidget * parent, ConfigObject<ConfigValue> * _config)
        :  DlgPreferencePage(parent),
           config(_config),
           m_COTInitialBoost(CONFIG_KEY, "InitialReplayGainBoost"),
           m_COTEnabled(CONFIG_KEY, "ReplayGainEnabled") {
    setupUi(this);

    //Connections
    connect(EnableGain,      SIGNAL(stateChanged(int)), this, SLOT(slotSetRGEnabled()));
    connect(EnableAnalyser,  SIGNAL(stateChanged(int)), this, SLOT(slotSetRGAnalyserEnabled()));
    connect(SliderBoost,     SIGNAL(valueChanged(int)), this, SLOT(slotUpdateBoost()));
    connect(SliderBoost,     SIGNAL(sliderReleased()),  this, SLOT(slotApply()));

    loadSettings();
}

DlgPrefReplayGain::~DlgPrefReplayGain() {
}

void DlgPrefReplayGain::loadSettings() {
    if(config->getValueString(ConfigKey(CONFIG_KEY,"ReplayGainEnabled"))==QString("")) {
        slotResetToDefaults();
    } else {
        int iReplayGainBoost =
                config->getValueString(ConfigKey(CONFIG_KEY, "InitialReplayGainBoost")).toInt();
        SliderBoost->setValue(iReplayGainBoost);
        lcddB->display(iReplayGainBoost);

        bool gainEnabled =
                config->getValueString(ConfigKey(CONFIG_KEY, "ReplayGainEnabled")).toInt() == 1;
        EnableGain->setChecked(gainEnabled);

        bool analyserEnabled =
                config->getValueString(ConfigKey(CONFIG_KEY, "ReplayGainAnalyserEnabled")).toInt();
        EnableAnalyser->setChecked(analyserEnabled);
    }
    slotUpdate();
    slotUpdateBoost();
}

void DlgPrefReplayGain::slotResetToDefaults() {
    EnableGain->setChecked(true);
    // Turn ReplayGain Analyser on by default as it does not give appreciable
    // delay on recent hardware (<5 years old).
    EnableAnalyser->setChecked(true);
    SliderBoost->setValue(6);
    lcddB->display(6);
    slotUpdate();
    slotApply();
}

void DlgPrefReplayGain::slotSetRGEnabled() {
    if (EnableGain->isChecked()) {
        config->set(ConfigKey(CONFIG_KEY,"ReplayGainEnabled"), ConfigValue(1));
    } else {
        config->set(ConfigKey(CONFIG_KEY,"ReplayGainEnabled"), ConfigValue(0));
        config->set(ConfigKey(CONFIG_KEY,"ReplayGainAnalyserEnabled"),
                    ConfigValue(0));
    }

    slotUpdate();
    slotApply();
}

void DlgPrefReplayGain::slotSetRGAnalyserEnabled() {
    int enabled = EnableAnalyser->isChecked() ? 1 : 0;
    config->set(ConfigKey(CONFIG_KEY,"ReplayGainAnalyserEnabled"),
                ConfigValue(enabled));
    slotApply();
}

void DlgPrefReplayGain::slotUpdateBoost() {
    config->set(ConfigKey(CONFIG_KEY, "InitialReplayGainBoost"),
                ConfigValue(SliderBoost->value()));
    slotApply();
}

void DlgPrefReplayGain::slotUpdate() {
    if (config->getValueString(ConfigKey(CONFIG_KEY,"ReplayGainEnabled")).toInt()==1)
    {
        EnableAnalyser->setEnabled(true);
        SliderBoost->setEnabled(true);
    }
    else
    {
        EnableAnalyser->setChecked(false);
        EnableAnalyser->setEnabled(false);
        SliderBoost->setEnabled(false);
    }
}

void DlgPrefReplayGain::slotApply() {
    m_COTInitialBoost.slotSet(SliderBoost->value());
    int iRGenabled = 0;
    if (EnableGain->isChecked()) iRGenabled = 1;
    m_COTEnabled.slotSet(iRGenabled);
}
