#pragma once
#include <stdint.h>

enum RiskLevel : uint8_t { RISK_NORMAL = 0, RISK_WARNING = 1, RISK_CRITICAL = 2 };

struct RiskConfig {
    float wHr         = 0.3f;     // weight for heart rate deviation (bpm)
    float wSpo2       = 0.7f;     // weight for spo2 drop (% points), higher on purpose
    float trendGain   = 1.5f;     // extra weight on how much spo2 fell in the last minute
    float warnAt      = 5.0f;
    float critAt      = 7.5f;
    float spo2Floor   = 90.0f;    // below this it is critical whatever the score says
    float hrLow       = 40.0f;    // outside these = warning at least
    float hrHigh      = 140.0f;
    float baseAlpha   = 1.0f / 300.0f;  // ~5 min baseline at one reading per second
    int   warmup      = 30;       // readings before the baseline is trusted
    int   confirmUp   = 2;        // evaluations in a row before raising
    int   confirmDown = 4;        // and before lowering again
};

class RiskEngine {
public:
    RiskEngine();
    explicit RiskEngine(const RiskConfig &c);
    void reset();

    // call for every packet that has valid hr and spo2
    void addReading(uint8_t hr, uint8_t spo2, uint32_t nowMs);
    // call every ~1.5 s
    RiskLevel evaluate(uint32_t nowMs);

    RiskLevel level() const { return level_; }
    float score() const { return score_; }
    bool baselineReady() const { return warmCount_ >= cfg_.warmup; }
    float hrBaseline() const { return hrBase_; }
    float spo2Baseline() const { return spo2Base_; }

private:
    float spo2Trend(uint32_t nowMs) const;

    struct Sample { uint32_t t; uint8_t spo2; };
    static const int HIST = 96;

    RiskConfig cfg_;
    Sample hist_[HIST];
    int histHead_, histCount_;

    float hrBase_, spo2Base_;
    int warmCount_;
    bool have_;
    uint8_t lastHr_, lastSpo2_;

    float score_;
    RiskLevel level_;
    int upCount_, downCount_;
};
