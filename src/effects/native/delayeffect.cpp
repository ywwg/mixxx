#include <QtDebug>

#include "effects/native/DelayEffect.h"
#include "effects/native/waveguide_nl.h"

#include "mathstuff.h"
#include "sampleutil.h"

// static
QString DelayEffect::getId() {
    return "org.mixxx.effects.reverb";
}

// static
EffectManifest DelayEffect::getManifest() {
    EffectManifest manifest;
    manifest.setId(getId());
    manifest.setName(QObject::tr("Reverb"));
    manifest.setAuthor("The Mixxx Team, Steve Harris");
    manifest.setVersion("1.0");
    manifest.setDescription(QObject::tr("Tape Delay Simulation.  Correctly "
            "models the tape motion and some of the smear effect, there is no "
            "simulation for the head saturation yet, as I don't have a good "
            "model of it. When I get one I will add it. The way the tape "
            "accelerates and decelerates gives a nicer delay effect for many "
            "purposes."));

    EffectManifestParameter* time = manifest.addParameter();
    time->setId("delay_time");
    time->setName(QObject::tr("delay_time"));
    time->setDescription(QObject::tr("Delay time (seconds)"));
    time->setControlHint(EffectManifestParameter::CONTROL_KNOB_LINEAR);
    time->setValueHint(EffectManifestParameter::VALUE_FLOAT);
    time->setSemanticHint(EffectManifestParameter::SEMANTIC_UNKNOWN);
    time->setUnitsHint(EffectManifestParameter::UNITS_TIME);
    time->setMinimum(0.01);
    time->setDefault(0.25);
    time->setMaximum(2.0);

    return manifest;
}

DelayEffect::DelayEffect(EngineEffect* pEffect, const EffectManifest& manifest)
        : m_pDelayParameter(pEffect->getParameterById("delay_time")) {
}

DelayEffect::~DelayEffect() {
    for (QMap<QString, GroupState*>::iterator it = m_groupState.begin();
            it != m_groupState.end();) {
        SampleUtil::free((*it)->delay_buf);
        delete it.value();
        it = m_groupState.erase(it);
    }
    qDebug() << debugString() << "destroyed";
}

void DelayEffect::process(const QString& group, const CSAMPLE* pInput,
                          CSAMPLE* pOutput, const unsigned int numSamples) {
    GroupState* pGroupState = m_groupState.value(group, NULL);
    if (pGroupState == NULL) {
        pGroupState = new GroupState();
        m_groupState[group] = pGroupState;
    }
    GroupState& group_state = *pGroupState;

    CSAMPLE delay_time =
            m_pDelayParameter ? m_pTimeParameter->value().toDouble() : 1.0f;

    // TODO(owilliams) check ranges?

    int i;

    if (write_phase == 0) {
        plugin_data->last_delay_time = delay_time;
        plugin_data->delay_samples = delay_samples = CALC_DELAY(delay_time);
    }

    if (delay_time == last_delay_time) {
        long read_phase = write_phase - (long) delay_samples;
        LADSPA_Data *readptr = buffer + (read_phase & buffer_mask);
        LADSPA_Data *writeptr = buffer + (write_phase & buffer_mask);
        LADSPA_Data *lastptr = buffer + buffer_mask + 1;

        long remain = sample_count;

        while (remain) {
            long read_space = lastptr - readptr;
            long write_space = lastptr - writeptr;
            long to_process = MIN(MIN (read_space, remain), write_space);

            if (to_process == 0)
                return;  // buffer not allocated.

            remain -= to_process;

            for (i = 0; i < to_process; i++) {
                float read = *(readptr++);
                *(writeptr++) = in[i];
                buffer_write(out[i], read);
            }

            if (readptr == lastptr)
                readptr = buffer;
            if (writeptr == lastptr)
                writeptr = buffer;
        }

        write_phase += sample_count;
    } else {
        float next_delay_samples = CALC_DELAY(delay_time);
        float delay_samples_slope = (next_delay_samples - delay_samples)
                / sample_count;

        for (i = 0; i < sample_count; i++) {
            long read_phase;
            LADSPA_Data read;

            delay_samples += delay_samples_slope;
            write_phase++;
            read_phase = write_phase - (long) delay_samples;

            read = buffer[read_phase & buffer_mask];
            buffer[write_phase & buffer_mask] = in[i];
            buffer_write(out[i], read);
        }

        plugin_data->last_delay_time = delay_time;
        plugin_data->delay_samples = delay_samples;
    }

    plugin_data->write_phase = write_phase;
}
