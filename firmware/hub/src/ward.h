#pragma once
#include <stdint.h>

// what the screen / web page needs to know about one bed
enum BedView : uint8_t {
    V_NODATA = 0,
    V_OK,
    V_WARN,
    V_CRIT,
    V_NOFINGER,
    V_LOST
};

struct BedInfo {
    BedView view;
    uint8_t hr;
    uint8_t spo2;
    uint8_t battery;
    uint8_t acked;
    float   score;
};
