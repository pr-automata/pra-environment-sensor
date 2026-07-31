#pragma once
#include <Arduino.h>
#include "Config.h"
#include "NetworkManager.h"
#include "ISensor.h"

class CLIManager {
private:
    SystemConfig& config;
    NetworkManager& network;
    const ISensor& sensor;
    
    static constexpr size_t BUFFER_SIZE = 128;
    char buffer[BUFFER_SIZE];
    size_t buf_index = 0;
    
    enum CLIMode { MODE_NORMAL, MODE_SETUP };
    CLIMode mode = MODE_NORMAL;
    bool prompt_printed = false;

    char prev_c = 0;

    
    // --- Обработчики команд Нормального режима ---
    void cmdEnterSetup(const char* arg) {
        mode = MODE_SETUP;
        Serial.println("[CLI] Entered config mode. Type 'help' to see commands.");
    }
    
    void cmdOtaEnable(const char* arg) {
        network.enableOTA(); 
    }

    void cmdOtaDisable(const char* arg) {
        network.disableOTA(); 
    }

    void cmdStatus(const char* arg) {
        Serial.printf("ID: %s\nLoc: %s\nIP: %s\nManager: %s\nOTA Pass: %s\n", 
            config.system_id, config.system_location, 
            WiFi.localIP().toString().c_str(), config.manager_url, 
            (strlen(config.ota_password) == 0 ? "[NOT SET]" : "****"));
    }

    void cmdSysInfo(const char* arg) {
        unsigned long uptime_s = millis() / 1000;
        uint32_t days = uptime_s / 86400;
        uint8_t hours = (uptime_s % 86400) / 3600;
        uint8_t mins = (uptime_s % 3600) / 60;
        uint8_t secs = uptime_s % 60;

        Serial.println("\n--- System Hardware Info ---");
        Serial.printf("Chip ID:        %08X\n", ESP.getChipId());
        Serial.printf("Uptime:         %ud %02d:%02d:%02d\n", days, hours, mins, secs);
        Serial.printf("Reset Reason:   %s\n", ESP.getResetReason().c_str());
        Serial.printf("CPU Frequency:  %d MHz\n", ESP.getCpuFreqMHz());
        Serial.printf("Core Version:   %s\n", ESP.getCoreVersion().c_str());
        
        Serial.println("--- Power & Memory ---");
        Serial.printf("Internal VCC:   %.2f V\n", ESP.getVcc() / 1000.0);
        Serial.printf("Free Heap:      %u bytes\n", ESP.getFreeHeap());
        Serial.printf("Fragmentation:  %u%%\n", ESP.getHeapFragmentation());
        Serial.printf("Max Free Block: %u bytes\n", ESP.getMaxFreeBlockSize());
        Serial.printf("Contig. Stack:  %u bytes\n", ESP.getFreeContStack());
        
        Serial.println("--- Flash & OTA ---");
        Serial.printf("Real Flash:     %u KB\n", ESP.getFlashChipRealSize() / 1024);
        Serial.printf("Flash Speed:    %u MHz\n", ESP.getFlashChipSpeed() / 1000000);
        Serial.printf("Sketch Size:    %u KB\n", ESP.getSketchSize() / 1024);
        Serial.printf("Free OTA Space: %u KB\n", ESP.getFreeSketchSpace() / 1024);

        Serial.println("--- Network Interfaces ---");
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("Wi-Fi Status:   Connected\n");
            Serial.printf("Signal (RSSI):  %d dBm\n", WiFi.RSSI());
            Serial.printf("Channel:        %d\n", WiFi.channel());
            Serial.printf("BSSID:          %s\n", WiFi.BSSIDstr().c_str());
        } else {
            Serial.printf("Wi-Fi Status:   Disconnected\n");
        }
        Serial.println("----------------------------");
    }
    void cmdSensor(const char* arg) {
        // Получаем структуру данных напрямую от сенсора
        const SensorData& data = sensor.getData();
        
        Serial.println("\n--- Sensor Diagnostics ---");
        if (data.isValid) {
            Serial.printf("Status:      [OK]\n");
            Serial.printf("Temperature: %.2f *C\n", data.temperature);
            Serial.printf("Humidity:    %.2f %%\n", data.humidity);
            Serial.printf("Pressure:    %.2f hPa\n", data.pressure);
        } else {
            Serial.printf("Status:      [FAIL] (I2C Error / Offline)\n");
            Serial.printf("Temperature: NaN\n");
            Serial.printf("Humidity:    NaN\n");
            Serial.printf("Pressure:    NaN\n");
        }
        Serial.println("--------------------------");
    }

    void cmdFactoryReset(const char* arg) {
        Serial.println("\n[WARN] INITIATING FACTORY RESET...");
        config.factoryReset();
        Serial.println("[SYS] Device is wiped. Rebooting in 3 seconds...");
        delay(3000);
        ESP.restart();
    }

    void cmdReboot(const char* arg) {
        Serial.println("[SYS] Rebooting...");
        delay(100);
        ESP.restart();
    }

    void sanitizeInput(char* str) {
        for (size_t i = 0; i < strlen(str); i++) {
            if (str[i] == '"' || str[i] == '\\' || str[i] == ',') str[i] = '_'; 
        }
    }

    // --- Обработчики команд Режима Настройки ---
    void setSsid(const char* arg) { 
        strlcpy(config.ssid, arg, sizeof(config.ssid)); Serial.println("-> SSID set (RAM)"); 
    }
    void setPass(const char* arg) { 
        strlcpy(config.password, arg, sizeof(config.password)); Serial.println("-> Pass set (RAM)"); 
    }
    void setSysId(const char* arg) { 
        strlcpy(config.system_id, arg, sizeof(config.system_id)); 
        sanitizeInput(config.system_id);
        Serial.println("-> ID set (RAM)"); 
    }
    void setLoc(const char* arg) {
        strlcpy(config.system_location, arg, sizeof(config.system_location)); Serial.println("-> Location set (RAM)"); 
    }
    void setMgr(const char* arg) {
        strlcpy(config.manager_url, arg, sizeof(config.manager_url)); Serial.println("-> Manager URL set (RAM)"); 
    }
    void setOtaPass(const char* arg) {
        strlcpy(config.ota_password, arg, sizeof(config.ota_password)); Serial.println("-> OTA Password set (RAM)"); 
    }
    
    void cmdCommit(const char* arg) {
        config.save();
        mode = MODE_NORMAL;
        Serial.println("Saved. Rebooting to apply net config...");
        delay(500);
        ESP.restart();
    }
    
    void cmdExit(const char* arg) {
        config.load(); 
        mode = MODE_NORMAL;
        Serial.println("Discarded. Exited setup.");
    }

    // --- ЯДРО ДИСПЕТЧЕРИЗАЦИИ ---

    void processNormalMode(char* cmd) {
        // Структура для привязки строки к методу класса
        struct Command {
            const char* name;
            void (CLIManager::*handler)(const char*);
            const char* help;
        };

        // Таблица маршрутизации (создается на лету, компилятор уберет её в константы)
        const Command commands[] = {
            {"setup",       &CLIManager::cmdEnterSetup,  "Enter configuration mode"},
            {"ota enable",  &CLIManager::cmdOtaEnable,   "Open OTA window for 2 hours"},
            {"ota disable", &CLIManager::cmdOtaDisable,  "Close OTA window immediately"}, 
            {"status",      &CLIManager::cmdStatus,      "Show system status"},
            {"sysinfo",     &CLIManager::cmdSysInfo,     "Show deep hardware telemetry"},
            {"sensor",      &CLIManager::cmdSensor,      "Read current I2C sensor data"}, 
            {"factory reset", &CLIManager::cmdFactoryReset, "Wipe all settings and reboot"}, // <-- Кнопка судного дня
            {"reboot",      &CLIManager::cmdReboot,      "Restart the device"}
        };

        if (strcmp(cmd, "help") == 0) {
            Serial.println("\n--- Normal Mode Commands ---");
            for (const auto& c : commands) Serial.printf("  %-15s - %s\n", c.name, c.help);
            Serial.printf("  %-15s - %s\n", "help", "Show this message");
            return;
        }

        for (const auto& c : commands) {
            if (strcmp(cmd, c.name) == 0) {
                (this->*(c.handler))(nullptr); // Вызов метода по указателю
                return;
            }
        }
        Serial.println("[ERR] Unknown command. Type 'help'.");
    }

    void processSetupMode(char* cmd) {
        struct SetupCommand {
            const char* name;
            void (CLIManager::*handler)(const char*);
            bool requires_val;
            const char* help;
        };

        const SetupCommand commands[] = {
            {"ssid",      &CLIManager::setSsid,     true,  "Set WiFi SSID"},
            {"pass",      &CLIManager::setPass,     true,  "Set WiFi Password"},
            {"sys_id",    &CLIManager::setSysId,    true,  "Set System ID"},
            {"loc",       &CLIManager::setLoc,      true,  "Set Location"},
            {"mgr",       &CLIManager::setMgr,      true,  "Set Manager URL"},
            {"ota_pass",  &CLIManager::setOtaPass,  true,  "Set OTA Password"},
            {"commit",    &CLIManager::cmdCommit,   false, "Save to LittleFS and reboot"},
            {"exit",      &CLIManager::cmdExit,     false, "Discard changes and exit"}
        };

        char* key = strtok(cmd, " ");
        char* val = strtok(NULL, ""); 

        if (!key) return;

        if (strcmp(key, "help") == 0) {
            Serial.println("\n--- Setup Mode Commands ---");
            for (const auto& c : commands) {
                Serial.printf("  %-10s %-7s - %s\n", c.name, c.requires_val ? "<value>" : "", c.help);
            }
            Serial.printf("  %-10s %-7s - %s\n", "help", "", "Show this message");
            return;
        }

        for (const auto& c : commands) {
            if (strcmp(key, c.name) == 0) {
                if (c.requires_val && (!val || strlen(val) == 0)) {
                    Serial.printf("[ERR] Command '%s' requires a value.\n", c.name);
                } else {
                    (this->*(c.handler))(val);
                }
                return;
            }
        }
        Serial.println("[ERR] Unknown key. Type 'help'.");
    }

    void executeCommand() {
        while (buf_index > 0 && (buffer[buf_index - 1] == ' ' || buffer[buf_index - 1] == '\r' || buffer[buf_index - 1] == '\n')) {
            buffer[--buf_index] = '\0';
        }
        
        char* cmd = buffer;
        while (*cmd == ' ') cmd++;

        if (strlen(cmd) > 0) {
            Serial.println();
            if (mode == MODE_NORMAL) processNormalMode(cmd);
            else processSetupMode(cmd);
        }
        
        buf_index = 0;
        buffer[0] = '\0';
        printPrompt();
    }
    void printPrompt() {
        Serial.printf("%s%s", config.system_id, mode == MODE_NORMAL ? "> " : "(config)# ");
        prompt_printed = true;
    }
    
    public:
    CLIManager(SystemConfig& conf, NetworkManager& net, const ISensor& sens) 
        : config(conf), network(net), sensor(sens) {
        buffer[0] = '\0';
    }

    void update() {
        if (!prompt_printed) {
            Serial.println(); // Чистая строка только при самой первой загрузке терминала
            printPrompt();
        }

        while (Serial.available()) {
            char c = (char)Serial.read();
            char raw_c = c;

            // --- Бронебойный анти-CRLF фильтр ---
            // Если пришел \n сразу после \r (даже в следующем такте loop), убиваем его.
            if (c == '\n' && prev_c == '\r') {
                prev_c = raw_c;
                continue; 
            }
            prev_c = raw_c;
            
            // Нормализуем \r в \n для универсальной обработки
            if (c == '\r') c = '\n';

            if (c == '\b' || c == 0x7F) { 
                if (buf_index > 0) {
                    buf_index--;
                    buffer[buf_index] = '\0';
                    Serial.print("\b \b");
                }
            } else if (c == '\n') {
                if (buf_index > 0) {
                    executeCommand();
                } else {
                    Serial.println(); 
                    printPrompt(); 
                }
            } else if (c >= 32 && c <= 126) {
                if (buf_index < BUFFER_SIZE - 1) {
                    buffer[buf_index++] = c;
                    buffer[buf_index] = '\0';
                } else {
                    Serial.println("\n[ERR] Buffer overflow. Dropping input.");
                    buf_index = 0;
                    buffer[0] = '\0';
                    printPrompt();
                }
            }
        }
    }
};