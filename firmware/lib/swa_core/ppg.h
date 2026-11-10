#pragma once
#include <stdint.h>

// raw red / ir samples (100 Hz) in, heart rate and spo2 out
// no arduino stuff in here so it can be tested on the pc

class PpgProcessor {
public:
    static const int FS = 100;            // samples per second
    static const int WIN = 300;           // 3 s window for the ac rms
    static const int WARMUP = 200;        // ignore first 2 s after finger is placed

    PpgProcessor();
    void reset();
    void push(uint32_t red, uint32_t ir);

    bool fingerOn() const { return finger_; }
    bool ready() const { return finger_ && n_ > WARMUP; }
    int   heartRate() const;              // bpm, 0 = nothing valid
    float spo2() const;                   // percent, 0 = nothing valid

    // spo2 = a - b*R, needs calibration against a real oximeter
    void setCalibration(float a, float b) { calA_ = a; calB_ = b; }
    void setFingerThreshold(uint32_t t) { fingerMin_ = t; }

private:
    struct Chan {
        float dc, lp1, lp2;
    };
    float filter(Chan &c, float raw);

    bool finger_;
    uint32_t fingerMin_;
    uint32_t n_;                          // samples since finger came on

    Chan red_, ir_;
    float redBuf_[WIN], irBuf_[WIN];
    double redSq_, irSq_;
    int head_, filled_;

    // peak finding
    float s1_, s2_, env_;
    uint32_t lastPeak_;
    float iv_[5];
    int ivCount_, ivHead_;

    float calA_, calB_;
};
