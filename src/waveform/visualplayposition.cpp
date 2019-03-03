#include "waveform/visualplayposition.h"

#include <QtDebug>

#include "control/controlproxy.h"
#include "control/controlobject.h"
#include "util/math.h"

namespace {
// The offset is limited to two callback intervals.
// This should be sufficiant to compensate jitter,
// but does not continue in case of underflows.
constexpr int kMaxOffsetBufferCnt = 2;
constexpr int kMicrosPerMillis = 1000; // 1 ms contains 1000 µs
} // anonymous namespace


//static
QMap<QString, QWeakPointer<VisualPlayPosition> > VisualPlayPosition::m_listVisualPlayPosition;
PerformanceTimer VisualPlayPosition::m_timeInfoTime;
double VisualPlayPosition::m_dCallbackEntryToDacSecs = 0;

VisualPlayPosition::VisualPlayPosition(const QString& key)
        : m_valid(false),
          m_key(key) {
    m_audioBufferSize = new ControlProxy(
            "[Master]", "audio_buffer_size", this);
    m_audioBufferSize->connectValueChanged(this, &VisualPlayPosition::slotAudioBufferSizeChanged);
    m_audioBufferMicros = static_cast<int>(m_audioBufferSize->get() * kMicrosPerMillis);
}

VisualPlayPosition::~VisualPlayPosition() {
    m_listVisualPlayPosition.remove(m_key);
}

void VisualPlayPosition::set(double playPos, double rate, double positionStep,
        double slipPosition, double tempoTrackSeconds) {
    VisualPlayPositionData data;
    data.m_referenceTime = m_timeInfoTime;
    data.m_callbackEntrytoDac = m_dCallbackEntryToDacSecs * 1000000; // s to µs
    data.m_enginePlayPos = playPos;
    data.m_rate = rate;
    data.m_positionStep = positionStep;
    data.m_slipPosition = slipPosition;
    data.m_tempoTrackSeconds = tempoTrackSeconds;

    // Atomic write
    m_data.setValue(data);
    m_valid = true;
}

double VisualPlayPosition::getAtNextSwap(mixxx::Duration estimatedTimeUntilSwap) {
    //static double testPos = 0;
    //testPos += 0.000017759; //0.000016608; //  1.46257e-05;
    //return testPos;

    if (m_valid) {
        VisualPlayPositionData data = m_data.getValue();
        int microsUntilSwap = estimatedTimeUntilSwap.toIntegerMicros();
        int microsUntilDac = data.m_callbackEntrytoDac - data.m_referenceTime.elapsed().toIntegerMicros();

        // playPos will be played in microsUntilDac seconds. If swap comes
        // first, offset will be negative so that playpos goes backward.
        int offsetMicros = microsUntilSwap - microsUntilDac;
        offsetMicros = math_clamp(offsetMicros,
                                      -m_audioBufferMicros * kMaxOffsetBufferCnt,
                                      m_audioBufferMicros * kMaxOffsetBufferCnt);
        double playPos = data.m_enginePlayPos;  // load playPos for the first sample in Buffer
        // add the offset for the position of the sample that will be transferred to the DAC
        // When the next display frame is displayed
        //qDebug() << "playPos" << playPos << offset;
        // TODO(rryan): m_audioBufferMicros is not accurate for this purpose. We
        // should use the PortAudio reported actual latency instead of Mixxx's
        // configured audio buffer size.
        playPos += data.m_positionStep * offsetMicros * data.m_rate / m_audioBufferMicros;
        return playPos;
    }
    return -1;
}

void VisualPlayPosition::getPlaySlipAt(mixxx::Duration estimatedTimeUntilSwap,
                                       double* pPlayPosition, double* pSlipPosition) {
    //static double testPos = 0;
    //testPos += 0.000017759; //0.000016608; //  1.46257e-05;
    //return testPos;

    if (m_valid) {
        VisualPlayPositionData data = m_data.getValue();
        int microsUntilSwap = estimatedTimeUntilSwap.toIntegerMicros();
        int microsUntilDac = data.m_callbackEntrytoDac - data.m_referenceTime.elapsed().toIntegerMicros();
        int offsetMicros = microsUntilSwap - microsUntilDac;
        offsetMicros = math_clamp(offsetMicros,
                                  -m_audioBufferMicros * kMaxOffsetBufferCnt,
                                  m_audioBufferMicros * kMaxOffsetBufferCnt);
        double playPos = data.m_enginePlayPos;  // load playPos for the first sample in Buffer
        // TODO(rryan): m_audioBufferMicros is not accurate for this purpose. We
        // should use the PortAudio reported actual latency instead of Mixxx's
        // configured audio buffer size.
        playPos += data.m_positionStep * offsetMicros * data.m_rate / m_audioBufferMicros;
        *pPlayPosition = playPos;
        *pSlipPosition = data.m_slipPosition;
    }
}

double VisualPlayPosition::getEnginePlayPos() {
    if (m_valid) {
        VisualPlayPositionData data = m_data.getValue();
        return data.m_enginePlayPos;
    } else {
        return -1;
    }
}

void VisualPlayPosition::getTrackTime(double* pPlayPosition, double* pTempoTrackSeconds) {
    if (m_valid) {
        VisualPlayPositionData data = m_data.getValue();
        *pPlayPosition = data.m_enginePlayPos;
        *pTempoTrackSeconds = data.m_tempoTrackSeconds;
    } else {
        *pPlayPosition = 0;
        *pTempoTrackSeconds = 0;
    }
}

void VisualPlayPosition::slotAudioBufferSizeChanged(double sizeMillis) {
    m_audioBufferMicros = static_cast<int>(sizeMillis * kMicrosPerMillis);
}

//static
QSharedPointer<VisualPlayPosition> VisualPlayPosition::getVisualPlayPosition(QString group) {
    QSharedPointer<VisualPlayPosition> vpp = m_listVisualPlayPosition.value(group);
    if (vpp.isNull()) {
        vpp = QSharedPointer<VisualPlayPosition>(new VisualPlayPosition(group));
        m_listVisualPlayPosition.insert(group, vpp);
    }
    return vpp;
}

//static
void VisualPlayPosition::setCallbackEntryToDacSecs(double secs, const PerformanceTimer& time) {
    // the time is valid only just NOW, so measure the time from NOW for
    // later correction
    m_timeInfoTime = time;
    m_dCallbackEntryToDacSecs = secs;
}
