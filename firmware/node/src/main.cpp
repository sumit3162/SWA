#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "MAX30105.h"       // sparkfun lib, works for the max30102 too

#include "config.h"
#include "ppg.h"
#include "swa_packet.h"

MAX30105 sensor;
PpgProcessor ppg;

static uint16_t seq = 0;
static uint32_t lastSend = 0;

static uint8_t batteryPercent() {
    // divider is 1:2 so multiply back
    float v = analogReadMilliVolts(PIN_BAT_ADC) * 2.0f / 1000.0f;
    int pct = (int)((v - 3.3f) / (4.2f - 3.3f) * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}

static void radioBegin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

    if (esp_now_init() != ESP_OK) {
        Serial.println("esp-now init failed");
        delay(1000);
        ESP.restart();
    }
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, HUB_MAC, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) Serial.println("add peer failed");
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    setCpuFrequencyMhz(80);     // no need for 240, saves some current

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);
    if (!sensor.begin(Wire, I2C_SPEED_FAST)) {
        Serial.println("max30102 not found, check wiring");
        while (1) {
            digitalWrite(PIN_LED, !digitalRead(PIN_LED));
            delay(150);
        }
    }
    // brightness, avg 4, red+ir, 400 sps -> 100 samples/s after averaging
    sensor.setup(0x3F, 4, 2, 400, 411, 4096);

    radioBegin();
    Serial.printf("bed %d node up\n", BED_ID);
}

void loop() {
    sensor.check();
    while (sensor.available()) {
        uint32_t red = sensor.getFIFORed();
        uint32_t ir = sensor.getFIFOIR();
        sensor.nextSample();
        ppg.push(red, ir);
    }

    uint32_t now = millis();
    if (now - lastSend >= SEND_PERIOD_MS) {
        lastSend = now;

        SwaPacket p = {};
        p.bedId = BED_ID;
        p.flags = ppg.fingerOn() ? SWA_FLAG_FINGER : 0;
        p.battery = batteryPercent();
        p.hr = (uint8_t)ppg.heartRate();
        p.spo2 = (uint8_t)(ppg.spo2() + 0.5f);
        p.seq = seq++;
        swaSeal(p);
        esp_now_send(HUB_MAC, (uint8_t *)&p, sizeof(p));

        digitalWrite(PIN_LED, p.flags & SWA_FLAG_FINGER);
        Serial.printf("hr=%u spo2=%u finger=%u bat=%u%%\n", p.hr, p.spo2, p.flags & 1, p.battery);
    }
    delay(10);      // fifo holds 32 samples = 320 ms, so this is fine
}
