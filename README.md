# 🌡️ PR//Automata Environment Sensor (PRA-EvS)

Lightweight, optimized environment monitoring node based on ESP8266 and the BME280 sensor. Designed with code quality, reliability, and enterprise-grade telemetry in mind. 

This project completely ditches hardcoded Wi-Fi credentials in favor of an elegant, built-in Serial CLI and exposes native Prometheus metrics out of the box.

## ✨ Features
* **📊 Native Prometheus Endpoint:** Exposes metrics (`/metrics`) natively without intermediate MQTT brokers or Node-RED formatting. Includes deep system telemetry (VCC, Heap Fragmentation, Uptime, Wi-Fi RSSI).
* **💻 Interactive CLI Terminal:** No hardcoded config. Connect via USB and configure SSID, passwords, and node metadata right in the serial monitor.
* **☁️ Secure OTA Updates:** Time-windowed OTA. Updates are disabled by default and can be activated via CLI for a 2-hour secure window.
* **💾 Fail-Safe Configuration:** Uses LittleFS with atomic file renaming. Configuration won't corrupt even on sudden power loss.
* **🔋 Hardware Optimization:** Implements `MODEM_SLEEP` and `StaticJsonDocument` stack-allocation to prevent heap fragmentation and thermal throttling.
* **🌐 Swarm Heartbeat / Auto-Discovery:** Nodes can periodically POST their health, IP, and MAC address to a central HTTP/HTTPS manager. Perfect for dynamic Prometheus target generation (Service Discovery).

## 🛠️ Hardware Requirements
* ESP8266 (e.g., Wemos D1 Mini)
* BME280 I2C Sensor (Default address: `0x76`)

## 🚀 Installation & First Boot

1. **Clone & Build:**
   Open the project in VS Code with the PlatformIO extension. Compile and flash the device via USB.

2. **Enter Configuration Mode:**
   Open the PlatformIO Serial Monitor (Baud rate: `115200`). You will be greeted by the CLI. Type `setup` to enter configuration mode:
```text
   PRA-EvS-default> setup
   [CLI] Entered config mode. Type 'help' to see commands.
   PRA-EvS-default(config)#

```

3. **Set Parameters:**
Use the CLI to configure your node. 
```text
ssid <Your_WiFi_SSID>
pass <Your_WiFi_Password>
sys_id <Unique_Node_Name>
loc <Room/Location>
ota_pass <Your_Secret_OTA_Password>
mgr <http://your-manager-url:port/api/register>
```


4. **Commit & Reboot:**
Type `commit` to atomically save to LittleFS. The device will automatically reboot and connect to your network.
```text
PRA-EvS-default(config)# commit
Saved. Rebooting to apply net config...
```



## 📈 Grafana Dashboard

A ready-to-use Grafana v2 dashboard template (`dashboard.json`) is included in the repository.
Simply import it into your Grafana instance and select your Prometheus data source. It includes dynamic cascaded variables (`location` -> `system_id`), automatic baseline heap calculation, and threshold colorizations.

## 🔄 OTA (Over-The-Air) Updates

To prevent unauthorized access, the OTA port is closed during normal operation. To flash a new firmware over Wi-Fi:

1. Connect via Serial (USB) and type `ota enable`.
2. The OTA window will open for 2 hours.
3. Uncomment the OTA block in `platformio.ini`, enter your node's IP and password.
4. Upload via PlatformIO.
5. The OTA window closes automatically after the timeout or network loss.

## 🤖 PR_Automata Env Swarm Manager (Coming Soon)
The `mgr` endpoint parameter is designed to work with our upcoming **Swarm Manager**. It's a lightweight centralized registry that listens to node heartbeats, tracks their IPs, and automatically generates `targets.json` files for Prometheus HTTP Service Discovery. 

EvS repo https://github.com/pr-automata/pra-evs-manager
## 📜 License

MIT License. Feel free to fork, build, and deploy.
