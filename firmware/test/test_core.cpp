// builds on a normal pc, no esp needed:  make test
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "ppg.h"
#include "risk.h"
#include "swa_packet.h"

static int fails = 0;
#define CHECK(cond, msg) do { if (!(cond)) { printf("FAIL: %s\n", msg); fails++; } else printf("ok:   %s\n", msg); } while (0)

// fake finger signal. R is the ratio-of-ratios we want the dsp to see
static void feedPpg(PpgProcessor &p, float bpm, float R, int seconds, unsigned seed) {
    srand(seed);
    float f = bpm / 60.0f;
    for (int n = 0; n < seconds * 100; n++) {
        float t = n / 100.0f;
        float ph = 2 * M_PI * f * t;
        float pulse = sinf(ph) + 0.25f * sinf(2 * ph + 0.8f);
        float noise = ((rand() % 2000) - 1000) / 1000.0f * 20.0f;
        float irDc = 60000, irAc = 600;
        float redDc = 55000, redAc = R * (irAc / irDc) * redDc;
        p.push((uint32_t)(redDc - redAc * pulse + noise), (uint32_t)(irDc - irAc * pulse + noise));
    }
}

static void testPpg() {
    PpgProcessor p;
    feedPpg(p, 72, 0.6f, 15, 1);          // R=0.6 -> 110-25*0.6 = 95
    printf("hr=%d spo2=%.1f\n", p.heartRate(), p.spo2());
    CHECK(abs(p.heartRate() - 72) <= 2, "heart rate 72 bpm within 2");
    CHECK(fabsf(p.spo2() - 95.0f) < 1.5f, "spo2 about 95 for R=0.6");

    PpgProcessor q;
    feedPpg(q, 110, 0.5f, 15, 2);         // 97.5
    printf("hr=%d spo2=%.1f\n", q.heartRate(), q.spo2());
    CHECK(abs(q.heartRate() - 110) <= 3, "heart rate 110 bpm within 3");
    CHECK(fabsf(q.spo2() - 97.5f) < 1.5f, "spo2 about 97.5 for R=0.5");

    // finger off
    for (int i = 0; i < 100; i++) q.push(1000, 1000);
    CHECK(!q.fingerOn() && q.heartRate() == 0 && q.spo2() == 0, "finger removed -> no values");
}

static void testPacket() {
    SwaPacket p = {};
    p.bedId = 7; p.hr = 80; p.spo2 = 97; p.seq = 12;
    swaSeal(p);
    CHECK(swaCheck((uint8_t *)&p, sizeof p), "good packet accepted");
    p.hr = 81;
    CHECK(!swaCheck((uint8_t *)&p, sizeof p), "changed byte rejected");
    CHECK(sizeof(SwaPacket) == 9, "packet is 9 bytes");
}

static uint32_t T = 0;
static RiskLevel step(RiskEngine &e, int hr, int spo2) {
    T += 1000;
    e.addReading(hr, spo2, T);
    return e.evaluate(T);
}

static void testRisk() {
    RiskEngine e;
    T = 0;
    RiskLevel l = RISK_NORMAL;
    for (int i = 0; i < 90; i++) l = step(e, 75 + (i % 3), 97);
    CHECK(l == RISK_NORMAL && e.baselineReady(), "stable patient stays normal");

    // one bad reading (finger moved) must not alarm
    l = step(e, 75, 82);
    l = step(e, 75, 97);
    CHECK(l == RISK_NORMAL, "single glitch does not raise an alert");

    // real fall: 97 -> 86 over 40 s while hr climbs
    int firstWarn = -1, firstCrit = -1;
    for (int i = 0; i < 60; i++) {
        int spo2 = 97 - (i * 11) / 40; if (spo2 < 86) spo2 = 86;
        int hr = 76 + i / 2;
        l = step(e, hr, spo2);
        if (l >= RISK_WARNING && firstWarn < 0) firstWarn = i;
        if (l == RISK_CRITICAL && firstCrit < 0) firstCrit = i;
    }
    printf("warning after %d s, critical after %d s of the fall, score=%.1f\n", firstWarn, firstCrit, e.score());
    CHECK(firstWarn >= 0 && firstCrit >= firstWarn, "falling spo2 goes warning then critical");
    CHECK(firstCrit < 30, "critical before spo2 reaches 86");

    // recovery steps down, not instantly
    int downAfter = -1;
    for (int i = 0; i < 30; i++) {
        l = step(e, 78, 97);
        if (l == RISK_NORMAL && downAfter < 0) downAfter = i;
    }
    CHECK(downAfter >= 3, "needs a few good readings before going back to normal");

    // sitting low but stable (copd type patient) should not be critical forever once baseline is there
    RiskEngine c; T = 0;
    for (int i = 0; i < 90; i++) l = step(c, 80, 92);
    CHECK(l == RISK_NORMAL, "stable 92 baseline is normal (above the 90 floor)");
    for (int i = 0; i < 5; i++) l = step(c, 80, 88);
    CHECK(l == RISK_CRITICAL, "88 is critical because of the floor");
}

int main() {
    testPpg();
    testPacket();
    testRisk();
    printf("\n%s (%d failed)\n", fails ? "SOME TESTS FAILED" : "all good", fails);
    return fails ? 1 : 0;
}
