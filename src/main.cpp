#include <Arduino.h>
#include "Config.h"
#include "SensorBME280.h"
#include "MetricsServer.h"
#include "NetworkManager.h" 
#include "CLIManager.h"      
ADC_MODE(ADC_VCC);

SystemConfig config;
SensorBME280 bmeSensor(0x76);
MetricsServer metrics(config, bmeSensor);
NetworkManager network(config, bmeSensor); 
CLIManager terminal(config, network, bmeSensor);

void setup() {
    Serial.begin(115200);
    Serial.println("\n[CORE] Booting...");
    
    config.load();
    bmeSensor.begin();
    
    network.begin();
    metrics.begin();
}

void loop() {
    terminal.update(); 
    network.update();
    bmeSensor.update();

    // Отдаем 10 мс ядру ESP для сна модема и фоновых задач
    delay(10);
}