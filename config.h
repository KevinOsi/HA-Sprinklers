#ifndef CONFIG_H
#define CONFIG_H

// --- WiFi Configuration ---
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// --- MQTT Configuration ---
const char* MQTT_BROKER = "YOUR_MQTT_BROKER_IP";
const int MQTT_PORT = 1883;
const char* MQTT_USER = "YOUR_MQTT_USER";
const char* MQTT_PASS = "YOUR_MQTT_PASS";

// --- Device Configuration ---
const char* DEVICE_ID = "ESP_Irrigation";
const char* SYSTEM_ONE_NAME = "Sprinkler Zone 1";
const char* SYSTEM_TWO_NAME = "Sprinkler Zone 2";
const char* SYSTEM_THREE_NAME = "Sprinkler Zone 3";
const char* SYSTEM_FOUR_NAME = "Sprinkler Zone 4";

// --- Pin Configuration (LinkNode R4) ---
const int RELAY_ONE_PIN = 12;
const int RELAY_TWO_PIN = 13;
const int RELAY_THREE_PIN = 14;
const int RELAY_FOUR_PIN = 16;

// --- MQTT Topics ---
const char* TOPIC_RELAY_ONE_SUB = "Home/Irrigation/1/Relays/1";
const char* TOPIC_RELAY_TWO_SUB = "Home/Irrigation/1/Relays/2";
const char* TOPIC_RELAY_THREE_SUB = "Home/Irrigation/1/Relays/3";
const char* TOPIC_RELAY_FOUR_SUB = "Home/Irrigation/1/Relays/4";
const char* TOPIC_ALL_CMD_SUB = "Home/Irrigation/1/All";

const char* TOPIC_STATUS_ONE_PUB = "Home/Irrigation/1/Status/1";
const char* TOPIC_STATUS_TWO_PUB = "Home/Irrigation/1/Status/2";
const char* TOPIC_STATUS_THREE_PUB = "Home/Irrigation/1/Status/3";
const char* TOPIC_STATUS_FOUR_PUB = "Home/Irrigation/1/Status/4";
const char* TOPIC_STATUS_DEVICE_PUB = "Home/Irrigation/1/Status/WatchDog";
const char* TOPIC_STATUS_QUEUE_PUB = "Home/Irrigation/1/Status/Queue";
const char* TOPIC_STATUS_NR_SUB = "Home/Status/WatchDog/1";

// --- NTP Configuration ---
const char* NTP_SERVER = "ca.pool.ntp.org";
const long GMT_OFFSET_SEC = -21600; // -6 hours
const int DAYLIGHT_OFFSET_SEC = 3600; // 1 hour

#endif // CONFIG_H
