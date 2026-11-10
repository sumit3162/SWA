#pragma once

// ---- radio / wifi ----
// has to match ESPNOW_CHANNEL on the nodes. the hub's own access point is
// pinned to this channel too, otherwise esp-now and the ap fight over it.
#define ESPNOW_CHANNEL 1
#define AP_SSID "SWA-Ward"
#define AP_PASS "change-me-123"       // min 8 chars. please change it

// ---- timing ----
#define EVAL_PERIOD_MS      1500
#define NODE_LOST_MS        10000     // no packet for this long = node lost
#define FINGER_OFF_WARN_MS  60000     // sensor off a patient for this long = warning
#define REPEAT_CRIT_MS      8000      // voice repeat while critical and not acked
#define REPEAT_WARN_MS      30000

// ---- esp32-s3-box-3 pins ----
// taken from the box-3 schematic / bsp. if something stays dark or silent,
// check these first.
#define PIN_I2C_SDA   8
#define PIN_I2C_SCL   18

#define PIN_I2S_MCLK  2
#define PIN_I2S_BCLK  17
#define PIN_I2S_WS    45
#define PIN_I2S_DOUT  15
#define PIN_I2S_DIN   16
#define PIN_PA_EN     46              // speaker amplifier enable
#define ES8311_ADDR   0x18

#define PIN_LCD_MOSI  6
#define PIN_LCD_SCK   7
#define PIN_LCD_CS    5
#define PIN_LCD_DC    4
#define PIN_LCD_RST   48
#define PIN_LCD_BL    47
#define LCD_ROTATION  1               // landscape. try 3 if it is upside down
#define LCD_INVERT    true            // box-3 panel needs inverted colours (i think)

#define PIN_ACK_BTN   1               // the mute button on the box-3
