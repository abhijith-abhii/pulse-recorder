#include "recorder.hpp"
#include <Arduino.h>
#include <Preferences.h>
#include <esp_task_wdt.h>
#include <esp_timer.h>
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t edge_count = 0, last_edge = 0, rejected = 0;
volatile bool seen = false;
volatile uint64_t last_accepted_64 = 0;
void IRAM_ATTR pulse() {
    uint64_t stamp = uint64_t(esp_timer_get_time());
    uint32_t now = uint32_t(stamp);
    portENTER_CRITICAL_ISR(&mux);
    if (!seen || stamp - last_accepted_64 >= 500) {
        seen = true;
        last_edge = now;
        last_accepted_64 = stamp;
        ++edge_count;
    } else
        ++rejected;
    portEXIT_CRITICAL_ISR(&mux);
}
recorder::Config config;
recorder::Recorder recorder_state;
Preferences prefs;
bool storage_ready;
core::Line<32> line;
void setup() {
    Serial.begin(115200);
    pinMode(27, INPUT_PULLDOWN);
    pinMode(25, OUTPUT);
    pinMode(34, INPUT);
    analogReadResolution(12);
    storage_ready = prefs.begin("pulse", false);
    if (storage_ready && prefs.isKey("config")) {
        if (prefs.getBytesLength("config") != sizeof(config) ||
            prefs.getBytes("config", &config, sizeof(config)) != sizeof(config) ||
            !config.valid()) {
            config = recorder::Config{};
            Serial.println("# invalid config; default PPR=2");
        }
    }
    attachInterrupt(digitalPinToInterrupt(27), pulse, RISING);
    esp_task_wdt_init(5, true);
    esp_task_wdt_add(nullptr);
    Serial.println("us,count,rpm,stalled,runtime_us,adc_raw,rejected,jitter_us,ppr");
}
void loop() {
    static TickType_t last_tick = xTaskGetTickCount();
    static uint32_t last_report = micros(), last_save = 0;
    esp_task_wdt_reset();
    for (unsigned budget = 0; budget < 64 && Serial.available(); ++budget) {
        if (line.feed(char(Serial.read()))) {
            uint32_t value;
            if (recorder::parse_ppr(line.data, value)) {
                if (core::elapsed(millis(), last_save) < 1000) {
                    Serial.println("# ERR rate limit");
                    continue;
                }
                recorder::Config next = config;
                next.ppr = value;
                next.seal();
                if (storage_ready &&
                    prefs.putBytes("config", &next, sizeof(next)) == sizeof(next)) {
                    config = next;
                    Serial.println("# OK");
                } else
                    Serial.println("# ERR storage");
                last_save = millis();
            } else
                Serial.println("# ERR expected PPR 1..32");
        }
    }
    vTaskDelayUntil(&last_tick, pdMS_TO_TICKS(100));
    uint32_t now = micros();
#ifdef SIMULATED_SENSORS
    portENTER_CRITICAL(&mux);
    if ((millis() / 10000) % 2 == 0) {
        edge_count += 10;
        last_edge = now;
        seen = true;
    }
    portEXIT_CRITICAL(&mux);
#endif
    uint32_t count, last, noise;
    bool have;
    portENTER_CRITICAL(&mux);
    count = edge_count;
    last = last_edge;
    noise = rejected;
    have = seen;
    portEXIT_CRITICAL(&mux);
    auto r = recorder_state.update(now, count, last, have, config.ppr);
    digitalWrite(25, r.stalled ? HIGH : LOW);
    uint32_t dt = now - last_report;
    last_report = now;
    Serial.printf("%u,%u,%.2f,%d,%llu,%u,%u,%ld,%u\n", now, count, r.rpm, r.stalled,
                  (unsigned long long)r.runtime_us, analogRead(34), noise,
                  long(int64_t(dt) - 100000), config.ppr);
}
