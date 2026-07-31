#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct SystemConfig {
    char ssid[32] = "";
    char password[64] = "";
    char system_id[32] = "PRA-EvS-default";
    char system_location[32] = "default_location";
    char manager_url[128] = "";
    char ota_password[32] = "";

    void load() {
        if (!LittleFS.begin()) {
            Serial.println("[ERR] LittleFS mount failed!");
            return; 
        }
        if (LittleFS.exists("/config.json")) {
            File file = LittleFS.open("/config.json", "r");
            StaticJsonDocument<384> doc; // Память выделяется на стеке, никакой фрагментации
            if (!deserializeJson(doc, file)) {
                strlcpy(ssid, doc["ssid"] | "", sizeof(ssid));
                strlcpy(password, doc["password"] | "", sizeof(password));
                strlcpy(system_id, doc["system_id"] | "PRA-EvS-default", sizeof(system_id));
                strlcpy(system_location, doc["system_location"] | "default_location", sizeof(system_location));
                strlcpy(manager_url, doc["manager_url"] | "", sizeof(manager_url));
                strlcpy(ota_password, doc["ota_password"] | "", sizeof(ota_password));
            }
            file.close();
        }
    }

    void save() {
        File file = LittleFS.open("/config.tmp", "w");
        if (!file) return;
        
        StaticJsonDocument<384> doc;
        doc["ssid"] = ssid;
        doc["password"] = password;
        doc["system_id"] = system_id;
        doc["system_location"] = system_location;
        doc["manager_url"] = manager_url;
        doc["ota_password"] = ota_password;
        
        serializeJson(doc, file);
        file.close();
        LittleFS.rename("/config.tmp", "/config.json");
        Serial.println("[SYS] Config saved atomically.");
    }

    void factoryReset() {
        if (!LittleFS.begin()) {
            Serial.println("[ERR] LittleFS mount failed during reset!");
            return;
        }
        
        // Уничтожаем основной и временный файлы конфигурации
        if (LittleFS.exists("/config.json")) LittleFS.remove("/config.json");
        if (LittleFS.exists("/config.tmp")) LittleFS.remove("/config.tmp");
        
        Serial.println("[SYS] Configuration files erased from flash memory.");
    }
};