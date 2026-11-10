#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <string.h>

#include "display.h"
#include "config.h"

static Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_LCD_DC, PIN_LCD_CS, PIN_LCD_SCK, PIN_LCD_MOSI, GFX_NOT_DEFINED);
// if you only get a white / blank screen swap this for Arduino_ST7789
static Arduino_GFX *gfx = new Arduino_ILI9341(bus, PIN_LCD_RST, LCD_ROTATION, false);

#define C_BG     0x0000
#define C_TEXT   0xFFFF
#define C_DIM    0x8410
#define C_OK     0x07E0
#define C_WARN   0xFE60
#define C_CRIT   0xF800
#define C_GREY   0x4208

static const int ROW_Y0 = 40;
static const int ROW_H = 20;

static BedInfo shown[SWA_MAX_BEDS + 1];
static int shownActive = -1;
static bool first = true;

static uint16_t colourFor(BedView v) {
    switch (v) {
        case V_OK:       return C_OK;
        case V_WARN:     return C_WARN;
        case V_CRIT:     return C_CRIT;
        case V_NOFINGER: return C_GREY;
        case V_LOST:     return C_GREY;
        default:         return C_BG;
    }
}

static const char *labelFor(BedView v) {
    switch (v) {
        case V_OK:       return "OK";
        case V_WARN:     return "WARN";
        case V_CRIT:     return "CRIT";
        case V_NOFINGER: return "OFF";
        case V_LOST:     return "LOST";
        default:         return "";
    }
}

static bool same(const BedInfo &a, const BedInfo &b) {
    return a.view == b.view && a.hr == b.hr && a.spo2 == b.spo2 &&
           a.acked == b.acked && (int)(a.score * 2) == (int)(b.score * 2);
}

static void drawHeader(int active) {
    gfx->fillRect(0, 0, 320, 38, C_BG);
    gfx->setTextSize(2);
    gfx->setTextColor(C_TEXT);
    gfx->setCursor(4, 3);
    gfx->print("Smart Ward Assistant");
    gfx->setCursor(276, 3);
    gfx->printf("%d/%d", active, SWA_MAX_BEDS);

    gfx->setTextSize(1);
    gfx->setTextColor(C_DIM);
    gfx->setCursor(4, 26);   gfx->print("BED");
    gfx->setCursor(48, 26);  gfx->print("HR");
    gfx->setCursor(104, 26); gfx->print("SpO2");
    gfx->setCursor(164, 26); gfx->print("RISK");
    gfx->setCursor(262, 26); gfx->print("STATUS");
}

static void drawRow(int bed, const BedInfo &b) {
    int y = ROW_Y0 + (bed - 1) * ROW_H;
    gfx->fillRect(0, y, 320, ROW_H, C_BG);
    gfx->setTextSize(2);

    gfx->setTextColor(b.view == V_NODATA ? C_GREY : C_TEXT);
    gfx->setCursor(4, y + 2);
    gfx->printf("%d", bed);

    if (b.view == V_NODATA) {
        gfx->setTextColor(C_GREY);
        gfx->setCursor(48, y + 2);
        gfx->print("--");
        return;
    }

    uint16_t col = colourFor(b.view);

    gfx->setTextColor(C_TEXT);
    gfx->setCursor(48, y + 2);
    if (b.hr) gfx->printf("%d", b.hr); else gfx->print("--");
    gfx->setCursor(104, y + 2);
    if (b.spo2) gfx->printf("%d", b.spo2); else gfx->print("--");

    // risk bar, full at a score of 10
    int w = (int)(b.score / 10.0f * 90.0f);
    if (w > 90) w = 90;
    if (w < 0) w = 0;
    gfx->drawRect(164, y + 4, 92, 12, C_GREY);
    if (w > 0) gfx->fillRect(165, y + 5, w, 10, col);

    gfx->fillRect(262, y + 1, 56, ROW_H - 2, col);
    gfx->setTextColor(C_BG);
    gfx->setCursor(266, y + 3);
    gfx->print(labelFor(b.view));
    if (b.acked) {                 // little dot = someone has acknowledged it
        gfx->fillCircle(256, y + 10, 2, C_TEXT);
    }
}

void displayBegin() {
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);
    if (!gfx->begin()) {
        Serial.println("display init failed");
        return;
    }
    gfx->invertDisplay(LCD_INVERT);
    gfx->fillScreen(C_BG);
    memset(shown, 0, sizeof(shown));
}

void displayDraw(const BedInfo *beds, int activeNodes) {
    if (first || activeNodes != shownActive) {
        drawHeader(activeNodes);
        shownActive = activeNodes;
    }
    for (int b = 1; b <= SWA_MAX_BEDS; b++) {
        if (first || !same(shown[b], beds[b])) {
            drawRow(b, beds[b]);
            shown[b] = beds[b];
        }
    }
    first = false;
}
