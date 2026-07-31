#pragma once
#include "ISensor.h"
#include <Adafruit_BME280.h>
#include <Wire.h>

class SensorBME280 : public ISensor {
private:
    Adafruit_BME280 bme;
    SensorData currentData;
    unsigned long lastRead = 0;
    static constexpr unsigned long INTERVAL_MS = 5000;
    uint8_t i2c_addr;

    bool isDataPossible(float t, float h, float p) const {
        return (t >= -40.0f && t <= 85.0f) && (h >= 0.0f && h <= 100.0f) && (p >= 300.0f && p <= 1100.0f);
    }

public:
    SensorBME280(uint8_t addr = 0x76) : i2c_addr(addr) {}

    bool begin() override {
        Wire.begin();
        bool ok = bme.begin(i2c_addr);
        currentData.isValid = ok;
        return ok;
    }

    void update() override {
        if (!currentData.isValid || (millis() - lastRead < INTERVAL_MS)) return;
        lastRead = millis();
        
        float t = bme.readTemperature();
        float h = bme.readHumidity();
        float p = bme.readPressure() / 100.0F;

        if (isDataPossible(t, h, p)) {
            currentData.temperature = t;
            currentData.humidity = h;
            currentData.pressure = p;
        }
    }

    const SensorData& getData() const override { 
        return currentData; 
    }
};