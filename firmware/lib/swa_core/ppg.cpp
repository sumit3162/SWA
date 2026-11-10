#include "ppg.h"
#include <math.h>

static const float DC_ALPHA = 0.03f;      // high-pass, roughly 0.5 Hz
static const float LP_ALPHA = 0.25f;      // two of these in a row, roughly 5 Hz
static const int   MIN_GAP  = 33;         // samples between beats, 330 ms -> 180 bpm max

PpgProcessor::PpgProcessor() {
    fingerMin_ = 20000;
    calA_ = 110.0f;
    calB_ = 25.0f;
    reset();
}

void PpgProcessor::reset() {
    finger_ = false;
    n_ = 0;
    red_ = {0, 0, 0};
    ir_ = {0, 0, 0};
    for (int i = 0; i < WIN; i++) redBuf_[i] = irBuf_[i] = 0;
    redSq_ = irSq_ = 0;
    head_ = filled_ = 0;
    s1_ = s2_ = env_ = 0;
    lastPeak_ = 0;
    ivCount_ = ivHead_ = 0;
}

float PpgProcessor::filter(Chan &c, float raw) {
    c.dc += DC_ALPHA * (raw - c.dc);
    float ac = raw - c.dc;
    c.lp1 += LP_ALPHA * (ac - c.lp1);
    c.lp2 += LP_ALPHA * (c.lp1 - c.lp2);
    return c.lp2;
}

void PpgProcessor::push(uint32_t red, uint32_t ir) {
    if (ir < fingerMin_) {
        if (finger_) reset();
        return;
    }
    if (!finger_) {
        reset();
        finger_ = true;
        red_.dc = (float)red;
        ir_.dc = (float)ir;
    }
    n_++;

    float r = filter(red_, (float)red);
    float i = filter(ir_, (float)ir);

    // running sum of squares for rms
    if (filled_ == WIN) {
        redSq_ -= (double)redBuf_[head_] * redBuf_[head_];
        irSq_  -= (double)irBuf_[head_] * irBuf_[head_];
    } else {
        filled_++;
    }
    redBuf_[head_] = r;
    irBuf_[head_] = i;
    redSq_ += (double)r * r;
    irSq_  += (double)i * i;
    head_ = (head_ + 1) % WIN;

    // reflective sensor: more blood = less light back, so flip the sign
    float s = -i;

    if (fabsf(s) > env_) env_ = fabsf(s);
    else env_ *= 0.998f;

    // s1_ is the previous sample, s2_ the one before
    bool isPeak = (s2_ < s1_) && (s1_ >= s) && (s1_ > 0.4f * env_);
    if (isPeak && n_ > (uint32_t)WARMUP && (n_ - 1 - lastPeak_) > (uint32_t)MIN_GAP) {
        if (lastPeak_ != 0) {
            float dt = (float)(n_ - 1 - lastPeak_) / FS;
            if (dt > 0.3f && dt < 2.0f) {
                iv_[ivHead_] = dt;
                ivHead_ = (ivHead_ + 1) % 5;
                if (ivCount_ < 5) ivCount_++;
            } else {
                ivCount_ = 0;     // missed beats, start again
            }
        }
        lastPeak_ = n_ - 1;
    }
    s2_ = s1_;
    s1_ = s;
}

int PpgProcessor::heartRate() const {
    if (!ready() || ivCount_ < 3) return 0;
    if (n_ - lastPeak_ > 3 * FS) return 0;       // no beat for 3 s
    float sum = 0;
    for (int k = 0; k < ivCount_; k++) sum += iv_[k];
    float hr = 60.0f / (sum / ivCount_);
    if (hr < 30 || hr > 220) return 0;
    return (int)(hr + 0.5f);
}

float PpgProcessor::spo2() const {
    if (!ready() || filled_ < WIN) return 0;
    float rmsRed = sqrtf((float)(redSq_ / WIN));
    float rmsIr  = sqrtf((float)(irSq_ / WIN));
    if (red_.dc < 1 || ir_.dc < 1) return 0;
    float acdcIr = rmsIr / ir_.dc;
    if (acdcIr < 0.0003f) return 0;              // signal too weak
    float R = (rmsRed / red_.dc) / acdcIr;
    float s = calA_ - calB_ * R;
    if (s > 100) s = 100;
    if (s < 0) s = 0;
    return s;
}
