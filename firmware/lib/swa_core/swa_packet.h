#pragma once
#include <stdint.h>

// what a node sends to the hub once a second over esp-now
// keep it small, esp-now payload max is 250 bytes but smaller = less airtime

#define SWA_MAGIC     0x53
#define SWA_MAX_BEDS  10

#define SWA_FLAG_FINGER  0x01   // sensor sees a finger

struct __attribute__((packed)) SwaPacket {
    uint8_t  magic;
    uint8_t  bedId;      // 1..SWA_MAX_BEDS
    uint8_t  flags;
    uint8_t  battery;    // percent
    uint8_t  hr;         // bpm, 0 = no valid value
    uint8_t  spo2;       // percent, 0 = no valid value
    uint16_t seq;
    uint8_t  crc;        // crc8 of everything above
};

inline uint8_t swaCrc8(const uint8_t *d, int len) {
    uint8_t crc = 0;
    while (len--) {
        crc ^= *d++;
        for (int i = 0; i < 8; i++)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
    return crc;
}

inline void swaSeal(SwaPacket &p) {
    p.magic = SWA_MAGIC;
    p.crc = swaCrc8((const uint8_t *)&p, sizeof(SwaPacket) - 1);
}

inline bool swaCheck(const uint8_t *d, int len) {
    if (len != (int)sizeof(SwaPacket)) return false;
    const SwaPacket *p = (const SwaPacket *)d;
    if (p->magic != SWA_MAGIC) return false;
    if (p->bedId < 1 || p->bedId > SWA_MAX_BEDS) return false;
    return p->crc == swaCrc8(d, len - 1);
}
