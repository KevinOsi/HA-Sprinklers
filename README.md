# ESP8266 LinkNode R4 Sprinkler Controller

This project implements a smart sprinkler controller using an ESP8266-based LinkNode R4 board. It features a modern web interface, MQTT integration for home automation (e.g., Home Assistant, Node-RED), and an action queue system.

## Features

- **Web Interface:** Responsive, single-page application to control relays and view status.
- **MQTT Support:** Full integration with MQTT for remote control and status updates.
- **Action Queue:** Schedule actions with timestamps (e.g., turn on for X seconds).
- **OTA Updates:** Update firmware wirelessly.
- **mDNS Support:** Access the device via `http://sprinklers.local`.
- **JSON API:** RESTful API for integration with other systems.

## Hardware

- **LinkNode R4:** An ESP8266 development board with 4 relays.
- **Power Supply:** 5V DC (via micro USB or terminal).
- **Sprinkler Solenoids:** 24V AC (typically) connected to the relays.

## Software Requirements

- **Arduino IDE:** with ESP8266 board support installed.
- **Libraries:**
  - `ESP8266WiFi`
  - `ESP8266WebServer`
  - `PubSubClient` (by Nick O'Leary)
  - `ArduinoJson` (v6)
  - `NTPClient` (by Fabrice Weinberg)
  - `ArduinoOTA`

## Configuration

1.  **WiFi & MQTT:** Open `config.h` and update the following:
    ```cpp
    const char* WIFI_SSID = "YOUR_WIFI_SSID";
    const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
    const char* MQTT_BROKER = "YOUR_MQTT_BROKER_IP";
    const char* MQTT_USER = "YOUR_MQTT_USER";
    const char* MQTT_PASS = "YOUR_MQTT_PASS";
    ```

2.  **Pins:** The default pins for LinkNode R4 are set in `config.h`:
    - Relay 1: GPIO 12
    - Relay 2: GPIO 13
    - Relay 3: GPIO 14
    - Relay 4: GPIO 16

## Usage

### Web Interface
Navigate to `http://sprinklers.local` (or the device IP) in your browser. You can:
- View current relay status.
- Manually toggle relays ON/OFF.
- View the pending action queue.
- See system information (WiFi signal, uptime, etc).

### MQTT Topics
The device subscribes to:
- `Home/Irrigation/1/Relays/{1-4}`: Send JSON `{"Action": 1}` (ON) or `{"Action": 0}` (OFF).
  - Optional: `{"Delay": 60}` (execute after 60s), `{"TimeStamp": 1700000000}` (execute at specific epoch).
- `Home/Irrigation/1/All`: Send `{"ClearQueue": true}` to clear pending actions.

The device publishes to:
- `Home/Irrigation/1/Status/{1-4}`: Relay status `{"Status": 1}`.
- `Home/Irrigation/1/Status/WatchDog`: Periodic heartbeat.
- `Home/Irrigation/1/Status/Queue`: Current queue list.

### API Endpoints
- `GET /status`: Returns system status, relay states, and queue.
- `POST /control`: Control a relay.
  - Body: `{"relay": 1, "action": 1}`
- `GET /queue`: Returns the raw queue list.

## OTA Updates
The device supports Over-The-Air updates. In Arduino IDE, select the network port `ESP_Irrigation` (or `sprinklers`) to upload new firmware without USB.

## License
MIT
