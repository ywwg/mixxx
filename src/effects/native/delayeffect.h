// Ported from SWH Plate Reverb 1423.
// This effect is GPL code.

#ifndef DELAYEFFECT_H
#define DELAYEFFECT_H

#include <QMap>

#include "defs.h"
#include "util.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "effects/effectprocessor.h"
#include "sampleutil.h"

class DelayEffect : public EffectProcessor {
  public:
    DelayEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~DelayEffect();

    static QString getId();
    static EffectManifest getManifest();

    // See effectprocessor.h
    void process(const QString& group,
                 const CSAMPLE* pInput, CSAMPLE* pOutput,
                 const unsigned int numSamples);

  private:
    QString debugString() const {
        return getId();
    }

    EngineEffectParameter* m_pDelayParameter;

    struct GroupState {
        GroupState() {
            delay_buf = SampleUtil::alloc(MAX_BUFFER_LEN);
        }
        CSAMPLE* delay_buf;
    };
    QMap<QString, GroupState*> m_groupState;

    DISALLOW_COPY_AND_ASSIGN(DelayEffect);
};

#endif /* DELAYEFFECT_H */
