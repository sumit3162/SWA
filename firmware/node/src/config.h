#pragma once

#ifndef BED_ID
#error "set BED_ID, use one of the [env:bedN] sections in platformio.ini"
#endif

// must be the same on the hub
#define ESPNOW_CHANNEL 1

// hub mac. broadcast works without knowing it, but then anybody can listen
// and the hub can't tell who is who. put the real one in once you have the hub.
static const uint8_t HUB_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// i2c to the max30102 board
#define PIN_SDA 21
#define PIN_SCL 22
#define PIN_INT 19          // wired but not used yet, i just poll the fifo

#define PIN_LED 2
#define PIN_BAT_ADC 34      // battery through 2 x 100k divider

#define SEND_PERIOD_MS 1000
