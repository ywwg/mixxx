#ifndef HIGHLOWEFFECT_H
#define HIGHLOWEFFECT_H

#include <QMap>

#include "effects/effect.h"
#include "effects/effectprocessor.h"
#include "engine/effects/engineeffect.h"
#include "engine/effects/engineeffectparameter.h"
#include "engine/enginefilterbutterworth8.h"
#include "sampleutil.h"
#include "util.h"

struct HighLowGroupState {
    HighLowGroupState()
            // TODO(XXX) 44100 should be changed to real sample rate
            // https://bugs.launchpad.net/mixxx/+bug/1208816.
            : lowFilter(44100, 20),
              highFilter(44100, 20),
              oldDepth(0) {
        SampleUtil::applyGain(crossfadeBuffer, 0, MAX_BUFFER_LEN);
    }

    EngineFilterButterworth8Low lowFilter;
    EngineFilterButterworth8High highFilter;

    CSAMPLE crossfadeBuffer[MAX_BUFFER_LEN];
    double oldDepth;
};

class HighLowEffect : public GroupEffectProcessor<HighLowGroupState> {
  public:
    HighLowEffect(EngineEffect* pEffect, const EffectManifest& manifest);
    virtual ~HighLowEffect();

    static QString getId();
    static EffectManifest getManifest();

    // See effectprocessor.h
    void processGroup(const QString& group,
                      HighLowGroupState* pState,
                      const CSAMPLE* pInput, CSAMPLE *pOutput,
                      const unsigned int numSamples);

  private:
    QString debugString() const {
        return getId();
    }

    double getLowFrequencyCorner(double depth);
    double getHighFrequencyCorner(double depth);

    EngineEffectParameter* m_pDepthParameter;

    DISALLOW_COPY_AND_ASSIGN(HighLowEffect);
};

#endif /* HIGHLOWEFFECT_H */
