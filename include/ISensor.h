#pragma once

struct SensorData {
    float temperature = 0.0f;
    float humidity = 0.0f;
    float pressure = 0.0f;
    bool isValid = false;
};

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual const SensorData& getData() const = 0;
};