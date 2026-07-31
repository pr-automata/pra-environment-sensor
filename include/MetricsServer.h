#pragma once
#include <ESPAsyncWebServer.h>
#include "Config.h"
#include "ISensor.h"
#include "SystemConst.h"
class MetricsServer {
private:
    AsyncWebServer server;
    const SystemConfig& config;
    const ISensor& sensor;
    uint32_t baseline_heap = 0;

public:
    MetricsServer(const SystemConfig& cfg, const ISensor& sens) 
        : server(80), config(cfg), sensor(sens) {}

    void begin() {
        baseline_heap = ESP.getFreeHeap();
        server.on("/metrics", HTTP_GET, [this](AsyncWebServerRequest *request) {
            AsyncResponseStream *response = request->beginResponseStream("text/plain");
            const SensorData& data = sensor.getData();
            
            char tags[128];
            snprintf(tags, sizeof(tags), "system_id=\"%s\",system_location=\"%s\"", 
                     config.system_id, config.system_location);

            response->printf("pra_env_system_info{%s,system_version_sw=\"%s\",model=\"%s\"} 1\n", 
                        tags, SysConst::SW_VERSION, SysConst::SHORT_MODEL);

            if (data.isValid) {
                response->printf("pra_env_temperature_air{%s,sensor_type=\"temperature\"} %.2f\n", tags, data.temperature);
                response->printf("pra_env_pressure_air{%s,sensor_type=\"pressure\"} %.2f\n", tags, data.pressure);
                response->printf("pra_env_humidity_air{%s,sensor_type=\"humidity\"} %.2f\n", tags, data.humidity);
            } else {
                response->printf("pra_env_temperature_air{%s} NaN\n", tags);
            }
            response->printf("pra_env_system_signallevel_wifi{%s,sensor_type=\"network\"} %d\n", tags, WiFi.RSSI());

            response->printf("pra_system_uptime_seconds{%s} %lu\n", tags, millis() / 1000);
            response->printf("pra_system_heap_baseline_bytes{%s} %u\n", tags, baseline_heap);
            response->printf("pra_system_heap_free_bytes{%s} %u\n", tags, ESP.getFreeHeap());
            response->printf("pra_system_heap_fragmentation_percent{%s} %u\n", tags, ESP.getHeapFragmentation());

            
            // ESP.getVcc() возвращает милливольты. Делим на 1000 для вольтов.
            response->printf("pra_system_voltage_volts{%s} %.3f\n", tags, ESP.getVcc() / 1000.0);

            request->send(response);
        });
        server.begin();
    }
};