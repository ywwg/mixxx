#include <QtDebug>

#include "visualplayposition.h"
#include "controlobjectslave.h"
#include "controlobject.h"
#include "util/math.h"
#include "waveform/vsyncthread.h"

//static
QMap<QString, QWeakPointer<VisualPlayPosition> > VisualPlayPosition::m_listVisualPlayPosition;
PaStreamCallbackTimeInfo VisualPlayPosition::m_timeInfo = { 0.0, 0.0, 0.0 };
PerformanceTimer VisualPlayPosition::m_timeInfoTime;
bool VisualPlayPosition::m_bClampFailedWarning = false;

VisualPlayPosition::VisualPlayPosition(const QString& key)
        : m_valid(false),
          m_key(key) {
    m_audioBufferSize = new ControlObjectSlave("[Master]", "audio_buffer_size");
    m_pTrackSamples = new ControlObjectSlave(group, "track_samples");
    m_pTrackSamples->connectValueChanged(this, SLOT(slotTrackSamplesChanged(double)));
    m_track_samples = m_pTrackSamples->get();
    m_pTrackSampleRate = new ControlObjectSlave(group, "track_samplerate");
    m_pTrackSampleRate->connectValueChanged(this, SLOT(slotTrackSampleRateChanged(double)));
    m_track_samplerate = m_pTrackSampleRate->get();
    m_pSpinnyAngle = new ControlObject(ConfigKey(group, "spinny_angle"));

#ifdef __VINYLCONTROL__
    m_pVinylControlSpeedType = new ControlObjectSlave(group, "vinylcontrol_speed_type");
    // Match the vinyl control's set RPM so that the spinny widget rotates at the same
    // speed as your physical decks, if you're using vinyl control.
    m_pVinylControlSpeedType->connectValueChanged(this, SLOT(updateVinylControlSpeed(double)));
    updateVinylControlSpeed(m_pVinylControlSpeedType->get());
#else
    // If no vinyl control, just call it 33.3333
    this->updateVinylControlSpeed(33. + 1./3.);
#endif
}

void VisualPlayPosition::updateVinylControlSpeed(double rpm) {
    m_dRotationsPerSecond = rpm/60.;
    m_audioBufferSize->connectValueChanged(
            this, SLOT(slotAudioBufferSizeChanged(double)));
    m_dAudioBufferSize = m_audioBufferSize->get();
}

VisualPlayPosition::~VisualPlayPosition() {
    m_listVisualPlayPosition.remove(m_key);
    delete m_audioBufferSize;
    delete m_pTrackSamples;
    delete m_pTrackSampleRate;
    delete m_pSpinnyAngle;
}

void VisualPlayPosition::slotTrackSamplesChanged(double samples) {
    m_track_samples = samples;
}

void VisualPlayPosition::slotTrackSampleRateChanged(double samplerate) {
    m_track_samplerate = samplerate;
}

void VisualPlayPosition::set(double playPos, double rate,
                             double positionStep, double pSlipPosition) {
    VisualPlayPositionData data;
    data.m_referenceTime = m_timeInfoTime;
    // Time from reference time to Buffer at DAC in µs
    data.m_callbackEntrytoDac = (m_timeInfo.outputBufferDacTime - m_timeInfo.currentTime) * 1000000;
    data.m_enginePlayPos = playPos;
    data.m_rate = rate;
    data.m_positionStep = positionStep;
    data.m_pSlipPosition = pSlipPosition;

    // Atomic write
    m_data.setValue(data);
    m_valid = true;
}

double VisualPlayPosition::getAtNextVSync(VSyncThread* vsyncThread) {
    //static double testPos = 0;
    //testPos += 0.000017759; //0.000016608; //  1.46257e-05;
    //return testPos;

    if (m_valid) {
        VisualPlayPositionData data = m_data.getValue();
        int usRefToVSync = vsyncThread->usFromTimerToNextSync(&data.m_referenceTime);
        int offset = usRefToVSync - data.m_callbackEntrytoDac;
        double playPos = data.m_enginePlayPos;  // load playPos for the first sample in Buffer
        // add the offset for the position of the sample that will be transfered to the DAC
        // When the next display frame is displayed
        playPos += data.m_positionStep * offset * data.m_rate / m_dAudioBufferSize / 1000;
        //qDebug() << "delta Pos" << playPos - m_playPosOld << offset;
        //m_playPosOld = playPos;
        m_pSpinnyAngle->set(calculateAngle(
                m_track_samples, m_track_samplerate, m_dRotationsPerSecond, playPos));
        return playPos;
    }
    return -1;
}

void VisualPlayPosition::getPlaySlipAt(int usFromNow, double* playPosition, double* slipPosition) {
    //static double testPos = 0;
    //testPos += 0.000017759; //0.000016608; //  1.46257e-05;
    //return testPos;

    if (m_valid) {
        VisualPlayPositionData data = m_data.getValue();
        int usElapsed = data.m_referenceTime.elapsed() / 1000.;
        int dacFromNow = usElapsed - data.m_callbackEntrytoDac;
        int offset = dacFromNow - usFromNow;
        double playPos = data.m_enginePlayPos;  // load playPos for the first sample in Buffer
        playPos += data.m_positionStep * offset * data.m_rate / m_dAudioBufferSize / 1000;
        *playPosition = playPos;
        *slipPosition = data.m_pSlipPosition;
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

void VisualPlayPosition::slotAudioBufferSizeChanged(double size) {
    m_dAudioBufferSize = size;

/* Convert between a normalized playback position (0.0 - 1.0) and an angle
   in our polar coordinate system.
   Returns an angle clamped between -180 and 180 degrees. */
// static
double VisualPlayPosition::calculateAngle(
        double tracksamples, double trackSampleRate, double rotations_per_sec, double playpos) {
    double trackFrames = tracksamples / 2;
    if (trackFrames <= 0 || trackSampleRate <= 0) {
        return 0.0;
    }

    // Convert playpos to seconds.
    double t = playpos * trackFrames / trackSampleRate;

    // 33 RPM is approx. 0.5 rotations per second.
    double angle = 360.0 * rotations_per_sec * t;
    // Clamp within -180 and 180 degrees
    //qDebug() << "pc:" << angle;
    //angle = ((angle + 180) % 360.) - 180;
    //modulo for doubles :)
    const double originalAngle = angle;
    if (angle > 0) {
        int x = (angle + 180) / 360;
        angle = angle - (360 * x);
    } else {
        int x = (angle - 180) / 360;
        angle = angle - (360 * x);
    }

    if (angle <= -180 || angle > 180) {
        // Only warn once per session. This can tank performance since it prints
        // like crazy.  Users may not notice it but we'll see it in the logs.
        if (!VisualPlayPosition::m_bClampFailedWarning) {
            qWarning() << "Angle clamping failed!" << t << originalAngle << "->" << angle
                       << "Please file a bug or email mixxx-devel@lists.sourceforge.net";
            VisualPlayPosition::m_bClampFailedWarning = true;
        }
        return 0.0;
    }
    return angle;
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
void VisualPlayPosition::setTimeInfo(const PaStreamCallbackTimeInfo* timeInfo) {
    // the timeInfo is valid only just NOW, so measure the time from NOW for
    // later correction
    m_timeInfoTime.start();
    m_timeInfo = *timeInfo;
    //qDebug() << "TimeInfo" << (timeInfo->currentTime - floor(timeInfo->currentTime)) << (timeInfo->outputBufferDacTime - floor(timeInfo->outputBufferDacTime));
}
