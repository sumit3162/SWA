#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

#include "config.h"
#include "swa_packet.h"
#include "risk.h"
#include "ward.h"
#include "audio.h"
#include "display.h"

struct Patient {
    bool seen = false;
    bool finger = false;
    uint8_t hr = 0, spo2 = 0, battery = 0;
    uint32_t lastRx = 0;
    uint32_t offAt = 0;             // when the finger came off, 0 = on
    RiskEngine engine;
    RiskLevel lvl = RISK_NORMAL;
    int ackLevel = -1;              // level that was acknowledged, -1 = nothing
    uint32_t lastSpoken = 0;
};

static Patient pts[SWA_MAX_BEDS + 1];
static BedInfo view[SWA_MAX_BEDS + 1];
static int activeNodes = 0;
static SemaphoreHandle_t lock;

static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

// ---------- radio ----------

static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (!swaCheck(data, len)) return;
    const SwaPacket *p = (const SwaPacket *)data;
    uint32_t now = millis();

    xSemaphoreTake(lock, portMAX_DELAY);
    Patient &s = pts[p->bedId];
    s.seen = true;
    s.lastRx = now;
    s.hr = p->hr;
    s.spo2 = p->spo2;
    s.battery = p->battery;

    bool finger = p->flags & SWA_FLAG_FINGER;
    if (!finger) {
        if (s.finger || s.offAt == 0) s.offAt = now;
    } else {
        // sensor was off for ages, probably a different patient now
        if (s.offAt && now - s.offAt > 600000UL) s.engine.reset();
        s.offAt = 0;
    }
    s.finger = finger;

    if (finger && p->hr && p->spo2) s.engine.addReading(p->hr, p->spo2, now);
    xSemaphoreGive(lock);
}

// ---------- ack ----------

static void ackAll() {
    xSemaphoreTake(lock, portMAX_DELAY);
    for (int b = 1; b <= SWA_MAX_BEDS; b++) {
        if (pts[b].lvl != RISK_NORMAL) pts[b].ackLevel = pts[b].lvl;
    }
    xSemaphoreGive(lock);
    Serial.println("acknowledged");
}

// ---------- web ----------

static void onWs(AsyncWebSocket *srv, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type != WS_EVT_DATA) return;
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) return;
    if (len == 3 && memcmp(data, "ack", 3) == 0) ackAll();
}

static void pushJson() {
    if (ws.count() == 0) return;
    char buf[1400];
    int n = snprintf(buf, sizeof(buf), "{\"t\":%lu,\"beds\":[", (unsigned long)millis());
    bool firstItem = true;
    for (int b = 1; b <= SWA_MAX_BEDS; b++) {
        if (view[b].view == V_NODATA) continue;
        n += snprintf(buf + n, sizeof(buf) - n,
                      "%s{\"id\":%d,\"v\":%d,\"hr\":%d,\"spo2\":%d,\"bat\":%d,\"score\":%.1f,\"ack\":%d}",
                      firstItem ? "" : ",", b, view[b].view, view[b].hr, view[b].spo2,
                      view[b].battery, view[b].score, view[b].acked);
        firstItem = false;
        if (n > (int)sizeof(buf) - 120) break;
    }
    snprintf(buf + n, sizeof(buf) - n, "]}");
    ws.textAll(buf);
}

// ---------- tasks ----------

static void riskTask(void *) {
    for (;;) {
        uint32_t now = millis();
        int active = 0;

        xSemaphoreTake(lock, portMAX_DELAY);
        for (int b = 1; b <= SWA_MAX_BEDS; b++) {
            Patient &s = pts[b];
            BedInfo &v = view[b];
            if (!s.seen) { v.view = V_NODATA; continue; }
            active++;

            bool lost = (now - s.lastRx) > NODE_LOST_MS;
            RiskLevel lvl = RISK_NORMAL;
            BedView bv;

            if (lost) {
                lvl = RISK_WARNING;         // a silent node is a risk too, battery could be dead
                bv = V_LOST;
            } else if (s.finger) {
                lvl = s.engine.evaluate(now);
                bv = lvl == RISK_CRITICAL ? V_CRIT : lvl == RISK_WARNING ? V_WARN : V_OK;
            } else {
                // no finger: don't evaluate, but if it was already alarming keep it that way
                lvl = s.engine.level();
                if (s.offAt && now - s.offAt > FINGER_OFF_WARN_MS && lvl < RISK_WARNING) lvl = RISK_WARNING;
                bv = lvl == RISK_CRITICAL ? V_CRIT : lvl == RISK_WARNING ? V_WARN : V_NOFINGER;
            }
            s.lvl = lvl;

            if (lvl == RISK_NORMAL) {
                s.ackLevel = -1;
                s.lastSpoken = 0;
            } else if ((int)lvl > s.ackLevel) {
                uint32_t every = lvl == RISK_CRITICAL ? REPEAT_CRIT_MS : REPEAT_WARN_MS;
                if (s.lastSpoken == 0 || now - s.lastSpoken >= every) {
                    audioAnnounce(b, lvl);
                    s.lastSpoken = now;
                    Serial.printf("bed %d -> %s (score %.1f)\n", b, lvl == RISK_CRITICAL ? "CRITICAL" : "warning", s.engine.score());
                }
            }

            v.view = bv;
            v.hr = lost ? 0 : s.hr;
            v.spo2 = lost ? 0 : s.spo2;
            v.battery = s.battery;
            v.score = s.engine.score();
            v.acked = (lvl != RISK_NORMAL) && ((int)lvl <= s.ackLevel);
        }
        activeNodes = active;
        xSemaphoreGive(lock);

        pushJson();
        vTaskDelay(pdMS_TO_TICKS(EVAL_PERIOD_MS));
    }
}

static void uiTask(void *) {
    pinMode(PIN_ACK_BTN, INPUT_PULLUP);
    uint32_t lastPress = 0;
    for (;;) {
        if (digitalRead(PIN_ACK_BTN) == LOW && millis() - lastPress > 500) {
            lastPress = millis();
            ackAll();
        }

        BedInfo copy[SWA_MAX_BEDS + 1];
        int active;
        xSemaphoreTake(lock, portMAX_DELAY);
        memcpy(copy, view, sizeof(copy));
        active = activeNodes;
        xSemaphoreGive(lock);

        displayDraw(copy, active);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

// ---------- setup ----------

void setup() {
    Serial.begin(115200);
    delay(300);
    lock = xSemaphoreCreateMutex();
    memset(view, 0, sizeof(view));

    if (!LittleFS.begin(true)) Serial.println("littlefs mount failed");

    displayBegin();
    audioBegin();

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS, ESPNOW_CHANNEL);
    Serial.printf("ap %s up, ip %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());
    Serial.printf("hub mac (put this in the node config): %s\n", WiFi.macAddress().c_str());

    if (esp_now_init() != ESP_OK) {
        Serial.println("esp-now init failed, restarting");
        delay(1000);
        ESP.restart();
    }
    esp_now_register_recv_cb(onRecv);

    ws.onEvent(onWs);
    server.addHandler(&ws);
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
    server.begin();

    // wifi stack lives on core 0, our stuff goes on core 1
    xTaskCreatePinnedToCore(riskTask, "risk", 6144, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(uiTask, "ui", 6144, NULL, 1, NULL, 1);
}

void loop() {
    ws.cleanupClients();
    delay(1000);
}
