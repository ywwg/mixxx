#ifndef ENGINEFILTERBUTTERWORTH8_H_
#define ENGINEFILTERBUTTERWORTH8_H_

#define MAX_COEFS 17
#define MAX_INTERNAL_BUF 16

#include "engine/enginefilter.h"
#include "util/defs.h"

class EngineFilterButterworth8 : public EngineObjectConstIn {
    Q_OBJECT
  public:
    EngineFilterButterworth8(int bufSize);
    virtual ~EngineFilterButterworth8();

    // Update filter without recreating it
    void initBuffers();
    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOut,
                         const int iBufferSize) = 0;

  protected:
    int m_sampleRate;

    double m_coef[MAX_COEFS];
    // Old coefficients needed for ramping
    double m_oldCoef[MAX_COEFS];

    int m_bufSize;
    // Channel 1 state
    double m_buf1[MAX_INTERNAL_BUF];
    // Old channel 1 buffer needed for ramping
    double m_oldBuf1[MAX_INTERNAL_BUF];

    // Channel 2 state
    double m_buf2[MAX_INTERNAL_BUF];
    // Old channel 2 buffer needed for ramping
    double m_oldBuf2[MAX_INTERNAL_BUF];

    // Flag set to true if ramping needs to be done
    bool m_doRamping;
};

class EngineFilterButterworth8Low : public EngineFilterButterworth8 {
    Q_OBJECT
  public:
    EngineFilterButterworth8Low(int sampleRate, double freqCorner1);

    void setFrequencyCorners(int sampleRate, double freqCorner1);
    void process(const CSAMPLE* pIn, CSAMPLE* pOut, const int iBufferSize);
};

class EngineFilterButterworth8Band : public EngineFilterButterworth8 {
    Q_OBJECT
  public:
    EngineFilterButterworth8Band(int sampleRate, double freqCorner1,
                                 double freqCorner2);

    void setFrequencyCorners(int sampleRate,
                             double freqCorner1, double freqCorner2 = 0);
    void process(const CSAMPLE* pIn, CSAMPLE* pOut, const int iBufferSize);
};

class EngineFilterButterworth8High : public EngineFilterButterworth8 {
    Q_OBJECT
  public:
    EngineFilterButterworth8High(int sampleRate, double freqCorner1);

    void setFrequencyCorners(int sampleRate, double freqCorner1);
    void process(const CSAMPLE* pIn, CSAMPLE* pOut, const int iBufferSize);
};

#endif  // ENGINE_ENGINEFILTERBUTTERWORTH8_H_
