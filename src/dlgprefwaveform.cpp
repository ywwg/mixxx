#include "dlgprefwaveform.h"

#include "mixxx.h"
#include "waveform/waveformwidgetfactory.h"
#include "waveform/renderers/waveformwidgetrenderer.h"

DlgPrefWaveform::DlgPrefWaveform(QWidget* pParent, MixxxMainWindow* pMixxx,
                                 ConfigObject<ConfigValue>* pConfig)
        : DlgPreferencePage(pParent),
          m_pConfig(pConfig),
          m_pMixxx(pMixxx) {
    setupUi(this);

    // Waveform overview init
    waveformOverviewComboBox->addItem( tr("Filtered") ); // "0"
    waveformOverviewComboBox->addItem( tr("HSV") ); // "1"
    waveformOverviewComboBox->addItem( tr("RGB") ); // "2"

    // The GUI is not fully setup so connecting signals before calling
    // slotUpdate can generate rebootMixxxView calls.
    // TODO(XXX): Improve this awkwardness.
    slotUpdate();

    connect(frameRateSpinBox, SIGNAL(valueChanged(int)),
            this, SLOT(slotSetFrameRate(int)));
    connect(frameRateSpinBox, SIGNAL(valueChanged(int)),
            frameRateSlider, SLOT(setValue(int)));
    connect(frameRateSlider, SIGNAL(valueChanged(int)),
            frameRateSpinBox, SLOT(setValue(int)));

    connect(waveformTypeComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSetWaveformType(int)));
    connect(defaultZoomComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSetDefaultZoom(int)));
    connect(synchronizeZoomCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(slotSetZoomSynchronization(bool)));
    connect(allVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(slotSetVisualGainAll(double)));
    connect(lowVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(slotSetVisualGainLow(double)));
    connect(midVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(slotSetVisualGainMid(double)));
    connect(highVisualGain,SIGNAL(valueChanged(double)),
            this,SLOT(slotSetVisualGainHigh(double)));
    connect(normalizeOverviewCheckBox,SIGNAL(toggled(bool)),
            this,SLOT(slotSetNormalizeOverview(bool)));

    connect(WaveformWidgetFactory::instance(), SIGNAL(waveformMeasured(float,int)),
            this, SLOT(slotWaveformMeasured(float,int)));

    connect(waveformOverviewComboBox,SIGNAL(currentIndexChanged(int)),
            this,SLOT(slotSetWaveformOverviewType(int)));
}

DlgPrefWaveform::~DlgPrefWaveform() {
}

void DlgPrefWaveform::slotUpdate() {
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();

    if (factory->isOpenGLAvailable()) {
        openGlStatusIcon->setText(factory->getOpenGLVersion());
    } else {
        openGlStatusIcon->setText(tr("OpenGL not available"));
    }

    WaveformWidgetType::Type currentType = factory->getType();
    int currentIndex = -1;

    waveformTypeComboBox->clear();
    QVector<WaveformWidgetAbstractHandle> handles = factory->getAvailableTypes();
    for (int i = 0; i < handles.size(); ++i) {
        waveformTypeComboBox->addItem(handles[i].getDisplayName());
        if (handles[i].getType() == currentType) {
            currentIndex = i;
        }
    }

    if (currentIndex != -1) {
        waveformTypeComboBox->setCurrentIndex(currentIndex);
    }

    frameRateSpinBox->setValue(factory->getFrameRate());
    synchronizeZoomCheckBox->setChecked(factory->isZoomSync());
    allVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::All));
    lowVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::Low));
    midVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::Mid));
    highVisualGain->setValue(factory->getVisualGain(WaveformWidgetFactory::High));
    normalizeOverviewCheckBox->setChecked(factory->isOverviewNormalized());

    for (int i = WaveformWidgetRenderer::s_waveformMinZoom;
         i <= WaveformWidgetRenderer::s_waveformMaxZoom; i++) {
        defaultZoomComboBox->addItem(QString::number(100/double(i), 'f', 1) + " %");
    }
    defaultZoomComboBox->setCurrentIndex(factory->getDefaultZoom() - 1);

    // By default we set filtered woverview = "0"
    waveformOverviewComboBox->setCurrentIndex(
            m_pConfig->getValueString(ConfigKey("[Waveform]","WaveformOverviewType"), "0").toInt());
}

void DlgPrefWaveform::slotApply() {
}

void DlgPrefWaveform::slotResetToDefaults() {
    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();

    // Get the default we ought to use based on whether the user has OpenGL or
    // not.
    WaveformWidgetType::Type defaultType = factory->autoChooseWidgetType();
    int currentIndex = -1;
    QVector<WaveformWidgetAbstractHandle> handles = factory->getAvailableTypes();
    for (int i = 0; i < handles.size(); ++i) {
        if (handles[i].getType() == defaultType) {
            currentIndex = i;
        }
    }
    if (currentIndex != -1) {
        waveformTypeComboBox->setCurrentIndex(currentIndex);
    }

    allVisualGain->setValue(1.5);
    lowVisualGain->setValue(1.0);
    midVisualGain->setValue(1.0);
    highVisualGain->setValue(1.0);

    // Default zoom level is 3 in WaveformWidgetFactory.
    defaultZoomComboBox->setCurrentIndex(3 + 1);

    // Don't synchronize zoom by default.
    synchronizeZoomCheckBox->setChecked(false);

    // Filtered overview.
    waveformOverviewComboBox->setCurrentIndex(0);

    // Don't normalize overview.
    normalizeOverviewCheckBox->setChecked(false);

    // 30FPS is the default
    frameRateSlider->setValue(30);
}

void DlgPrefWaveform::slotSetFrameRate(int frameRate) {
    WaveformWidgetFactory::instance()->setFrameRate(frameRate);
}

void DlgPrefWaveform::slotSetWaveformType(int index) {
    // Ignore sets for -1 since this happens when we clear the combobox.
    if (index < 0) {
        return;
    }

    if (WaveformWidgetFactory::instance()->setWidgetTypeFromHandle(index)) {
        // It was changed to a valid type. Previously we rebooted the Mixxx GUI
        // here but now we can update the waveforms on the fly.
    }
}

void DlgPrefWaveform::slotSetWaveformOverviewType(int index) {
    m_pConfig->set(ConfigKey("[Waveform]","WaveformOverviewType"), ConfigValue(index));
    m_pMixxx->rebootMixxxView();
}

void DlgPrefWaveform::slotSetDefaultZoom(int index) {
    WaveformWidgetFactory::instance()->setDefaultZoom( index + 1);
}

void DlgPrefWaveform::slotSetZoomSynchronization(bool checked) {
    WaveformWidgetFactory::instance()->setZoomSync(checked);
}

void DlgPrefWaveform::slotSetVisualGainAll(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::All,gain);
}

void DlgPrefWaveform::slotSetVisualGainLow(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Low,gain);
}

void DlgPrefWaveform::slotSetVisualGainMid(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::Mid,gain);
}

void DlgPrefWaveform::slotSetVisualGainHigh(double gain) {
    WaveformWidgetFactory::instance()->setVisualGain(WaveformWidgetFactory::High,gain);
}

void DlgPrefWaveform::slotSetNormalizeOverview( bool normalize) {
    WaveformWidgetFactory::instance()->setOverviewNormalized(normalize);
}

void DlgPrefWaveform::slotWaveformMeasured(float frameRate, int rtErrorCnt) {
    frameRateAverage->setText(
            QString::number((double)frameRate, 'f', 2) +
            " e" +
            QString::number(rtErrorCnt));
}
