#pragma once
#include <Arduino.h>

namespace SysConst {
    // --- Версионирование и Идентификация ---
    constexpr const char* SW_VERSION = "2.4";
    constexpr const char* HW_MODEL = "PR_Automata Environment Sensor";
    constexpr const char* SHORT_MODEL = "PR_Automata";
    
    constexpr const char* DEFAULT_SYS_ID = "PRA-EvS-default";
    constexpr const char* DEFAULT_LOC = "default_location";
    
    // --- Сетевые тайминги ---
    constexpr unsigned long OTA_TIMEOUT_MS = 2 * 60 * 60 * 1000UL;
    constexpr unsigned long HEARTBEAT_INTERVAL_MS = 300000;
    constexpr uint16_t HTTP_TIMEOUT_MS = 2000;
}