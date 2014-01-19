#include "effects/native/highloweffect.h"

// static
QString HighLowEffect::getId() {
    return "org.mixxx.effects.highlow";
}

// static
EffectManifest HighLowEffect::getManifest() {
    EffectManifest manifest;
    manifest.setId(getId());
    manifest.setName(QObject::tr("Filter"));
    manifest.setAuthor("The Mixxx Team");
    manifest.setVersion("1.0");
    manifest.setDescription("TODO");

    EffectManifestParameter* color = manifest.addParameter();
    color->setId("depth");
    color->setName(QObject::tr("Depth"));
    color->setDescription("TODO");
    color->setControlHint(EffectManifestParameter::CONTROL_KNOB_LINEAR);
    color->setValueHint(EffectManifestParameter::EffectManifestParameter::VALUE_FLOAT);
    color->setSemanticHint(EffectManifestParameter::SEMANTIC_UNKNOWN);
    color->setUnitsHint(EffectManifestParameter::UNITS_UNKNOWN);
    color->setDefault(0.0);
    color->setMinimum(-1.0);
    color->setMaximum(1.0);

    return manifest;
}

HighLowEffect::HighLowEffect(EngineEffect* pEffect,
                           const EffectManifest& manifest)
        : m_pDepthParameter(pEffect->getParameterById("depth")) {
}

HighLowEffect::~HighLowEffect() {
    qDebug() << debugString() << "destroyed";
}

double HighLowEffect::getLowFrequencyCorner(double depth) {
    // depth goes from 0 to -1
    // freq corner should go from 2^(5 + 8) down to 2^5.
    return pow(2.0, 14.0 - (-depth * 9.0));
}

double HighLowEffect::getHighFrequencyCorner(double depth) {
    // depth goes from 0 to 1
    // freq corner should go from 2^5 up to 2^(5+8).
    return pow(2.0, 5.0 + depth * 9.0);
}

void HighLowEffect::processGroup(const QString& group,
                                HighLowGroupState* pState,
                                const CSAMPLE* pInput, CSAMPLE* pOutput,
                                const unsigned int numSamples) {
    double depth = m_pDepthParameter ?
            m_pDepthParameter->value().toDouble() : 0.0;

    bool parametersChanged = (depth != pState->oldDepth);

    if (parametersChanged) {
        if (pState->oldDepth == 0.0) {
            SampleUtil::copyWithGain(pState->crossfadeBuffer, pInput, 1.0, numSamples);
        } else if (pState->oldDepth < 0.0) {
            pState->lowFilter.process(pInput, pState->crossfadeBuffer, numSamples);
        } else {
            pState->highFilter.process(pInput, pState->crossfadeBuffer, numSamples);
        }
        if (depth < 0.0) {
            double freq = getLowFrequencyCorner(depth);
            pState->lowFilter.setFrequencyCorners(freq);
        } else if (depth > 0.0) {
            double freq = getHighFrequencyCorner(depth);
            pState->highFilter.setFrequencyCorners(freq);
        }
    }

    // Create a little space around the center in case for noop.
    if (depth < 0.025 && depth > -0.025) {
        SampleUtil::copyWithGain(pOutput, pInput, 1.0, numSamples);
    } else {
        if (depth < 0.0) {
            pState->lowFilter.process(pInput, pOutput, numSamples);
        } else {
            pState->highFilter.process(pInput, pOutput, numSamples);
        }
    }

    if (parametersChanged) {
        SampleUtil::linearCrossfadeBuffers(pOutput, pState->crossfadeBuffer,
                                           pOutput, numSamples);
        pState->oldDepth = depth;
    }
}
