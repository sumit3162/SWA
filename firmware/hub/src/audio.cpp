#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
#include <LittleFS.h>
#include <math.h>

#include "audio.h"
#include "config.h"
#include "risk.h"

static I2SClass i2s;
static QueueHandle_t queue;
static const int RATE = 16000;

struct Job { uint8_t bed; uint8_t level; };

static void codecWrite(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(ES8311_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

// minimal es8311 setup, dac only, 16 kHz, mclk = 256 * fs.
// register values are from espressif's es8311 driver as far as i could follow it.
// if the speaker is silent but the amp pin is high, try esp_codec_dev instead.
static void codecInit() {
    codecWrite(0x00, 0x1F);     // reset
    delay(20);
    codecWrite(0x00, 0x00);
    codecWrite(0x00, 0x80);     // power on state machine
    codecWrite(0x01, 0x3F);     // clocks on, mclk from pin
    codecWrite(0x02, 0x00);
    codecWrite(0x03, 0x10);
    codecWrite(0x04, 0x10);
    codecWrite(0x05, 0x00);
    codecWrite(0x06, 0x03);
    codecWrite(0x07, 0x00);
    codecWrite(0x08, 0xFF);
    codecWrite(0x09, 0x0C);     // i2s, 16 bit
    codecWrite(0x0A, 0x0C);
    codecWrite(0x0D, 0x01);     // analog power
    codecWrite(0x0E, 0x02);
    codecWrite(0x12, 0x00);     // dac power
    codecWrite(0x13, 0x10);     // output to speaker driver
    codecWrite(0x1C, 0x6A);
    codecWrite(0x37, 0x08);
    codecWrite(0x31, 0x00);     // unmute
    codecWrite(0x32, 0xB0);     // dac volume, raise if too quiet
}

static void writeMono(const int16_t *s, int n) {
    int16_t buf[256];
    while (n > 0) {
        int k = n > 128 ? 128 : n;
        for (int i = 0; i < k; i++) buf[2 * i] = buf[2 * i + 1] = s[i];
        i2s.write((uint8_t *)buf, k * 4);
        s += k;
        n -= k;
    }
}

static void tone(int freq, int ms, int amp = 12000) {
    int total = RATE * ms / 1000;
    int16_t chunk[128];
    float phase = 0, step = 2 * M_PI * freq / RATE;
    int done = 0;
    while (done < total) {
        int k = min(128, total - done);
        for (int i = 0; i < k; i++) {
            int n = done + i;
            // short fade in/out so it doesn't click
            float env = 1.0f;
            if (n < 80) env = n / 80.0f;
            if (total - n < 80) env = (total - n) / 80.0f;
            chunk[i] = (int16_t)(sinf(phase) * amp * env);
            phase += step;
        }
        writeMono(chunk, k);
        done += k;
    }
}

static void quiet(int ms) {
    int16_t z[128] = {0};
    int total = RATE * ms / 1000;
    while (total > 0) {
        int k = min(128, total);
        writeMono(z, k);
        total -= k;
    }
}

// wav files are 16 kHz, 16 bit, mono. find the data chunk, don't trust a 44 byte header
static bool playWav(const char *path) {
    File f = LittleFS.open(path, "r");
    if (!f) return false;

    uint8_t hdr[12];
    if (f.read(hdr, 12) != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4)) {
        f.close();
        return false;
    }
    uint32_t dataLen = 0;
    while (f.available()) {
        uint8_t ch[8];
        if (f.read(ch, 8) != 8) break;
        uint32_t len = ch[4] | (ch[5] << 8) | (ch[6] << 16) | ((uint32_t)ch[7] << 24);
        if (!memcmp(ch, "data", 4)) { dataLen = len; break; }
        f.seek(f.position() + len + (len & 1));
    }
    if (!dataLen) { f.close(); return false; }

    int16_t buf[256];
    while (dataLen > 0) {
        int want = dataLen > sizeof(buf) ? sizeof(buf) : dataLen;
        int got = f.read((uint8_t *)buf, want);
        if (got <= 0) break;
        writeMono(buf, got / 2);
        dataLen -= got;
    }
    f.close();
    return true;
}

static void speak(const Job &j) {
    digitalWrite(PIN_PA_EN, HIGH);
    delay(10);

    // attention chime
    tone(660, 120);
    tone(880, 160);
    quiet(150);

    char path[24];
    snprintf(path, sizeof(path), "/bed_%d.wav", j.bed);
    if (!playWav(path)) {
        // no recording, beep the bed number instead
        for (int i = 0; i < j.bed; i++) {
            tone(1000, 100);
            quiet(130);
        }
    }
    quiet(200);

    if (j.level == RISK_CRITICAL) {
        if (!playWav("/critical.wav")) {
            for (int i = 0; i < 4; i++) { tone(1800, 90); quiet(70); }
        }
    } else {
        if (!playWav("/warning.wav")) tone(500, 450);
    }

    quiet(100);
    digitalWrite(PIN_PA_EN, LOW);
}

static void audioTask(void *) {
    Job j;
    for (;;) {
        if (xQueueReceive(queue, &j, portMAX_DELAY) == pdTRUE) speak(j);
    }
}

void audioBegin() {
    pinMode(PIN_PA_EN, OUTPUT);
    digitalWrite(PIN_PA_EN, LOW);

    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    i2s.setPins(PIN_I2S_BCLK, PIN_I2S_WS, PIN_I2S_DOUT, PIN_I2S_DIN, PIN_I2S_MCLK);
    if (!i2s.begin(I2S_MODE_STD, RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
        Serial.println("i2s begin failed");
        return;
    }
    codecInit();

    queue = xQueueCreate(6, sizeof(Job));
    xTaskCreatePinnedToCore(audioTask, "audio", 6144, NULL, 1, NULL, 1);
}

void audioAnnounce(uint8_t bed, uint8_t level) {
    if (!queue) return;
    Job j = {bed, level};
    xQueueSend(queue, &j, 0);       // if the queue is full we just skip, next repeat will catch it
}
