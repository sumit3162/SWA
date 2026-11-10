#include "risk.h"
#include <math.h>

RiskEngine::RiskEngine() : cfg_() { reset(); }
RiskEngine::RiskEngine(const RiskConfig &c) : cfg_(c) { reset(); }

void RiskEngine::reset() {
    histHead_ = histCount_ = 0;
    hrBase_ = spo2Base_ = 0;
    warmCount_ = 0;
    have_ = false;
    lastHr_ = lastSpo2_ = 0;
    score_ = 0;
    level_ = RISK_NORMAL;
    upCount_ = downCount_ = 0;
}

void RiskEngine::addReading(uint8_t hr, uint8_t spo2, uint32_t nowMs) {
    if (hr == 0 || spo2 == 0) return;
    lastHr_ = hr;
    lastSpo2_ = spo2;
    have_ = true;

    hist_[histHead_].t = nowMs;
    hist_[histHead_].spo2 = spo2;
    histHead_ = (histHead_ + 1) % HIST;
    if (histCount_ < HIST) histCount_++;

    if (warmCount_ < cfg_.warmup) {
        // plain running mean while we learn what is normal for this patient
        warmCount_++;
        hrBase_   += ((float)hr   - hrBase_)   / warmCount_;
        spo2Base_ += ((float)spo2 - spo2Base_) / warmCount_;
    } else if (level_ == RISK_NORMAL && spo2 >= spo2Base_ - 2.0f) {
        // only drift the baseline while things look fine, otherwise a slow
        // decline would drag the baseline down with it
        hrBase_   += cfg_.baseAlpha * ((float)hr   - hrBase_);
        spo2Base_ += cfg_.baseAlpha * ((float)spo2 - spo2Base_);
    }
}

// average of the newest 5 readings vs readings from about a minute ago
float RiskEngine::spo2Trend(uint32_t nowMs) const {
    if (histCount_ < 8) return 0;

    float newSum = 0;
    for (int k = 1; k <= 5; k++) {
        int idx = (histHead_ - k + HIST) % HIST;
        newSum += hist_[idx].spo2;
    }
    float newAvg = newSum / 5.0f;

    float oldSum = 0;
    int oldN = 0;
    for (int k = 1; k <= histCount_; k++) {
        int idx = (histHead_ - k + HIST) % HIST;
        uint32_t age = nowMs - hist_[idx].t;
        if (age >= 45000 && age <= 65000) {
            oldSum += hist_[idx].spo2;
            oldN++;
        }
    }
    if (oldN < 3) return 0;
    float drop = oldSum / oldN - newAvg;
    return drop > 0 ? drop : 0;
}

RiskLevel RiskEngine::evaluate(uint32_t nowMs) {
    if (!have_) return level_;

    float hrDev = 0, drop = 0, trend = 0;
    if (baselineReady()) {
        hrDev = fabsf((float)lastHr_ - hrBase_);
        drop = spo2Base_ - (float)lastSpo2_;
        if (drop < 0) drop = 0;
        trend = spo2Trend(nowMs);
    }
    score_ = cfg_.wHr * hrDev + cfg_.wSpo2 * (drop + cfg_.trendGain * trend);

    RiskLevel want = RISK_NORMAL;
    if (score_ >= cfg_.critAt || lastSpo2_ < cfg_.spo2Floor) want = RISK_CRITICAL;
    else if (score_ >= cfg_.warnAt || lastHr_ < cfg_.hrLow || lastHr_ > cfg_.hrHigh) want = RISK_WARNING;

    if (want > level_) {
        downCount_ = 0;
        if (++upCount_ >= cfg_.confirmUp) {
            level_ = want;
            upCount_ = 0;
        }
    } else if (want < level_) {
        upCount_ = 0;
        if (++downCount_ >= cfg_.confirmDown) {
            level_ = want;
            downCount_ = 0;
        }
    } else {
        upCount_ = downCount_ = 0;
    }
    return level_;
}
