#pragma once
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <memory>
#include "Config.h"
#include "ISensor.h"

class NetworkManager {
private:
const SystemConfig& config;
    const ISensor& sensor;
    
    char macAddress[18] = {0};
    
    // --- Состояние сети и OTA ---
    bool networkReady = false;
    bool otaActive = false;
    unsigned long otaStartTime = 0;
    unsigned long lastHeartbeat = 0;
    
    // --- Константы ---
    static constexpr unsigned long HEARTBEAT_INTERVAL_MS = 300000;
    static constexpr uint16_t HTTP_TIMEOUT_MS = 2000;

    //Тайм аут для OTA. Через 2 часа OTA автоматически будет отключена. 
    static constexpr unsigned long OTA_TIMEOUT_MS = 2 * 60 * 60 * 1000UL;

    void sendHeartbeat() {
        if (strlen(config.manager_url) == 0 || strcmp(config.manager_url, "none") == 0) return;
        
        // Умные указатели: память освободится сама при выходе из области видимости
        std::unique_ptr<WiFiClient> client;
        
        if (strncmp(config.manager_url, "https://", 8) == 0) {
            auto sec_client = std::make_unique<WiFiClientSecure>();
            sec_client->setInsecure(); 
            client = std::move(sec_client);
        } else {
            client = std::make_unique<WiFiClient>();
        }
        
        HTTPClient http;
        http.setTimeout(HTTP_TIMEOUT_MS);
        
        if (http.begin(*client, config.manager_url)) {
            http.addHeader("Content-Type", "application/json");
            
            // Формируем полезную нагрузку без выделения памяти в куче
            StaticJsonDocument<256> doc;
            doc["system_id"] = config.system_id;
            doc["location"] = config.system_location;
            doc["ip"] = WiFi.localIP().toString(); // Это временный String от ядра, допустимо
            doc["mac"] = macAddress;
            doc["sw_ver"] = SysConst::SW_VERSION;
            doc["model"] = SysConst::HW_MODEL;
            doc["sensor"] = sensor.getData().isValid ? "ok" : "fail";
            
            char payload[256];
            serializeJson(doc, payload);
            
            int httpCode = http.POST(payload);
            if (httpCode > 0) {
                Serial.printf("[SWARM] Heartbeat delivered to %s (Code: %d)\n", config.manager_url, httpCode);
            } else {
                Serial.printf("[WARN] Heartbeat failed: %s\n", http.errorToString(httpCode).c_str());
            }
            http.end();
        } else {
            Serial.println("[ERR] Unable to parse or connect to manager URL");
        }
    }

public:
    NetworkManager(const SystemConfig& conf, const ISensor& sens) 
        : config(conf), sensor(sens) {}

    void begin() {
        WiFi.mode(WIFI_STA);
        WiFi.setSleepMode(WIFI_MODEM_SLEEP);
        WiFi.setAutoReconnect(true); 
        WiFi.setHostname(config.system_id);
        
        // Кэшируем MAC-адрес один раз
        strlcpy(macAddress, WiFi.macAddress().c_str(), sizeof(macAddress));
        
        if (strlen(config.ssid) > 0) {
            Serial.printf("[NET] Connecting to %s...\n", config.ssid);
            WiFi.begin(config.ssid, config.password);
        } else {
            Serial.println("[WARN] No SSID configured. Use CLI to setup network.");
        }
    }

    void enableOTA() {
        if (strlen(config.ota_password) == 0) {
            Serial.println("[ERR] OTA password is empty. Set it in 'setup' mode first.");
            return;
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[ERR] Wi-Fi not connected. Cannot start OTA.");
            return;
        }
        
        if (!otaActive) {
            ArduinoOTA.setHostname(config.system_id);
            ArduinoOTA.setPassword(config.ota_password);
            ArduinoOTA.begin();
        }
        
        otaActive = true;
        otaStartTime = millis();
        Serial.printf("[NET] OTA Window OPEN for %lu ms (2 hours).\n", OTA_TIMEOUT_MS);
    }

    void disableOTA() {
        if (otaActive) {
            otaActive = false;
            Serial.println("[NET] OTA Window CLOSED manually.");
        } else {
            Serial.println("[NET] OTA is already closed.");
        }
    }

    void update() {
        unsigned long now = millis();

        if (WiFi.status() == WL_CONNECTED) {
            if (!networkReady) {
                networkReady = true;
                Serial.println("[NET] Connected. OTA is SLEEPING. Use CLI 'ota enable' to wake it up.");
                sendHeartbeat(); 
                lastHeartbeat = now;
            }
            
            // Обрабатываем OTA только если окно открыто
            if (otaActive) {
                ArduinoOTA.handle();
                
                // Проверка таймаута
                if (now - otaStartTime >= OTA_TIMEOUT_MS) {
                    otaActive = false;
                    // Прекращение вызова handle() аппаратно замораживает процесс обновления,
                    // устройство перестанет принимать пакеты прошивки.
                    Serial.println("\n[NET] OTA Window EXPIRED. Port is now closed.");
                }
            }
            
            if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
                lastHeartbeat = now;
                sendHeartbeat();
            }
        } else {
            if (networkReady) {
                Serial.println("[NET] Connection lost.");
                networkReady = false; 
                otaActive = false;
            }
        }
    }
};