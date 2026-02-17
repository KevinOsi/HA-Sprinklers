#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#include "config.h"
#include "Action.h"
#include "ActionsQueue.h"
#include "index_html.h"

// --- Global Objects ---
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, GMT_OFFSET_SEC, 60000);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

ESP8266WebServer server(80);

ActionsQueue actionQueue;

// --- Global Variables ---
unsigned long lastWatchDogTime = 0;
unsigned long lastStatusUpdateTime = 0;
unsigned long lastQueueUpdateTime = 0;
unsigned long lastReconnectAttempt = 0;

// --- Function Prototypes ---
void setupWiFi();
void setupMQTT();
void reconnectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void handleRoot();
void handleStatus();
void handleControl();
void handleQueue();
void handleNotFound();
void checkQueue();
void sendWatchDog();
void sendStatusUpdates();

// --- Setup ---
void setup() {
    Serial.begin(115200);
    delay(10);
    Serial.println("\nStarting ESP Irrigation System...");

    // Pin Setup
    pinMode(RELAY_ONE_PIN, OUTPUT);
    pinMode(RELAY_TWO_PIN, OUTPUT);
    pinMode(RELAY_THREE_PIN, OUTPUT);
    pinMode(RELAY_FOUR_PIN, OUTPUT);

    // Ensure relays are OFF (active HIGH or LOW depending on relay module, usually LOW is OFF for active HIGH relays)
    // The original code used digitalWrite(relay, 0) for OFF.
    digitalWrite(RELAY_ONE_PIN, LOW);
    digitalWrite(RELAY_TWO_PIN, LOW);
    digitalWrite(RELAY_THREE_PIN, LOW);
    digitalWrite(RELAY_FOUR_PIN, LOW);

    // WiFi Setup
    setupWiFi();

    // OTA Setup
    ArduinoOTA.setHostname(DEVICE_ID);
    ArduinoOTA.onStart([]() { Serial.println("Start updating..."); });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
    ArduinoOTA.begin();

    // Time Setup
    timeClient.begin();

    // MQTT Setup
    setupMQTT();

    // Web Server Setup
    server.on("/", HTTP_GET, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/control", HTTP_POST, handleControl);
    server.on("/queue", HTTP_GET, handleQueue);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("HTTP server started");
}

// --- Loop ---
void loop() {
    // Handle OTA
    ArduinoOTA.handle();

    // Handle WiFi and MQTT Connection
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost, reconnecting...");
        setupWiFi();
    }

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnectMQTT();
        }
    } else {
        mqttClient.loop();
    }

    // Handle Web Server
    server.handleClient();

    // Handle Time
    timeClient.update();

    // Handle Queue Execution
    checkQueue();

    // Periodic Tasks
    unsigned long now = millis();

    // Watchdog (Every 20s)
    if (now - lastWatchDogTime > 20000) {
        lastWatchDogTime = now;
        sendWatchDog();
    }

    // Status Updates (Every 5 min)
    if (now - lastStatusUpdateTime > 300000) {
        lastStatusUpdateTime = now;
        sendStatusUpdates();
    }

    // Queue List Update (Every 1 min)
    if (now - lastQueueUpdateTime > 60000) {
        lastQueueUpdateTime = now;
        if (mqttClient.connected()) {
            actionQueue.PubList(mqttClient, TOPIC_STATUS_QUEUE_PUB);
        }
    }
}

// --- Implementations ---

void setupWiFi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // Setup mDNS
        if (MDNS.begin("sprinklers")) { // http://sprinklers.local
            Serial.println("mDNS responder started");
        }
    } else {
        Serial.println("\nWiFi connection failed. Will retry in loop.");
    }
}

void setupMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
}

void reconnectMQTT() {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = DEVICE_ID;
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
        Serial.println("connected");
        // Subscribe to topics
        mqttClient.subscribe(TOPIC_RELAY_ONE_SUB);
        mqttClient.subscribe(TOPIC_RELAY_TWO_SUB);
        mqttClient.subscribe(TOPIC_RELAY_THREE_SUB);
        mqttClient.subscribe(TOPIC_RELAY_FOUR_SUB);
        mqttClient.subscribe(TOPIC_ALL_CMD_SUB);
        mqttClient.subscribe(TOPIC_STATUS_NR_SUB);
    } else {
        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 5 seconds");
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(message);

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    // Check for ClearQueue command
    if (doc.containsKey("ClearQueue") && doc["ClearQueue"]) {
        actionQueue.ClearQueue();
        Serial.println("Queue cleared via MQTT");
        actionQueue.PubList(mqttClient, TOPIC_STATUS_QUEUE_PUB);
        return;
    }

    int action = doc["Action"];
    float delaySec = doc["Delay"] | 0;
    unsigned long actionTimeStamp;

    if (doc.containsKey("TimeStamp")) {
        actionTimeStamp = doc["TimeStamp"];
    } else {
        actionTimeStamp = timeClient.getEpochTime();
    }

    if (delaySec > 0) {
        actionTimeStamp += (unsigned long)delaySec;
    }

    Serial.print("New action time: ");
    Serial.println(actionTimeStamp);

    String systemID = "";
    String pubTopic = "";
    int pin = 0;
    int relayNum = 0;
    bool valid = false;

    if (strcmp(topic, TOPIC_RELAY_ONE_SUB) == 0) {
        systemID = SYSTEM_ONE_NAME; pubTopic = TOPIC_STATUS_ONE_PUB; pin = RELAY_ONE_PIN; relayNum = 1; valid = true;
    } else if (strcmp(topic, TOPIC_RELAY_TWO_SUB) == 0) {
        systemID = SYSTEM_TWO_NAME; pubTopic = TOPIC_STATUS_TWO_PUB; pin = RELAY_TWO_PIN; relayNum = 2; valid = true;
    } else if (strcmp(topic, TOPIC_RELAY_THREE_SUB) == 0) {
        systemID = SYSTEM_THREE_NAME; pubTopic = TOPIC_STATUS_THREE_PUB; pin = RELAY_THREE_PIN; relayNum = 3; valid = true;
    } else if (strcmp(topic, TOPIC_RELAY_FOUR_SUB) == 0) {
        systemID = SYSTEM_FOUR_NAME; pubTopic = TOPIC_STATUS_FOUR_PUB; pin = RELAY_FOUR_PIN; relayNum = 4; valid = true;
    }

    if (valid) {
        Action newAction(DEVICE_ID, systemID, String(topic), pubTopic, pin, relayNum, (bool)action, actionTimeStamp);
        actionQueue.NewAct(newAction);
    }
}

void handleRoot() {
    server.send(200, "text/html", INDEX_HTML);
}

void handleStatus() {
    // Increased buffer size for queue
    DynamicJsonDocument doc(2048);

    // System Info
    JsonObject system = doc.createNestedObject("system");
    system["deviceID"] = DEVICE_ID;
    system["ip"] = WiFi.localIP().toString();
    system["time"] = timeClient.getFormattedTime();
    system["rssi"] = WiFi.RSSI();

    // Relay Status
    JsonArray relays = doc.createNestedArray("relays");
    relays.add(digitalRead(RELAY_ONE_PIN));
    relays.add(digitalRead(RELAY_TWO_PIN));
    relays.add(digitalRead(RELAY_THREE_PIN));
    relays.add(digitalRead(RELAY_FOUR_PIN));

    // Queue
    String queueJson = actionQueue.GetList();
    // Parse queue JSON string into the main doc
    // Use a temporary doc to parse, then add to main doc?
    // Or just manually add the string? But doc["queue"] = string adds escaped string.
    // doc["queue"] = serialized(queueJson) might work if library supports it, but ArduinoJson v6 uses deserializeJson.

    DynamicJsonDocument queueDoc(1024);
    DeserializationError error = deserializeJson(queueDoc, queueJson);
    if (!error) {
        doc["queue"] = queueDoc;
    } else {
        doc["queue"] = "Error parsing queue";
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleControl() {
    if (server.hasArg("plain") == false) {
        server.send(400, "application/json", "{\"error\":\"Body not received\"}");
        return;
    }

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));

    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    int relay = doc["relay"];
    int action = doc["action"];

    // Validate
    if (relay < 1 || relay > 4) {
        server.send(400, "application/json", "{\"error\":\"Invalid Relay\"}");
        return;
    }

    String systemID = "";
    String subTopic = "";
    String pubTopic = "";
    int pin = 0;

    switch(relay) {
        case 1: systemID = SYSTEM_ONE_NAME; subTopic = TOPIC_RELAY_ONE_SUB; pubTopic = TOPIC_STATUS_ONE_PUB; pin = RELAY_ONE_PIN; break;
        case 2: systemID = SYSTEM_TWO_NAME; subTopic = TOPIC_RELAY_TWO_SUB; pubTopic = TOPIC_STATUS_TWO_PUB; pin = RELAY_TWO_PIN; break;
        case 3: systemID = SYSTEM_THREE_NAME; subTopic = TOPIC_RELAY_THREE_SUB; pubTopic = TOPIC_STATUS_THREE_PUB; pin = RELAY_THREE_PIN; break;
        case 4: systemID = SYSTEM_FOUR_NAME; subTopic = TOPIC_RELAY_FOUR_SUB; pubTopic = TOPIC_STATUS_FOUR_PUB; pin = RELAY_FOUR_PIN; break;
    }

    // Immediate action (current time)
    unsigned long timestamp = timeClient.getEpochTime();
    Action newAction(DEVICE_ID, systemID, subTopic, pubTopic, pin, relay, (bool)action, timestamp);

    actionQueue.NewAct(newAction);

    server.send(200, "application/json", "{\"success\":true}");
}

void handleQueue() {
    server.send(200, "application/json", actionQueue.GetList());
}

void handleNotFound() {
    server.send(404, "text/plain", "Not Found");
}

void checkQueue() {
    // Check if first item in queue is valid (TimeStamp > 0)
    if (actionQueue.ActionList[0].TimeStamp > 0) {
        // Check if current time >= action time
        if (timeClient.getEpochTime() >= actionQueue.ActionList[0].TimeStamp) {

            Serial.print("Executing action for Relay ");
            Serial.println(actionQueue.ActionList[0].RelayNum);

            // Execute Action
            digitalWrite(actionQueue.ActionList[0].PinNum, actionQueue.ActionList[0].DoAction);

            // Publish Status
            if (mqttClient.connected()) {
                if (!actionQueue.ActionList[0].publish(mqttClient)) {
                    Serial.println("MQTT Publish Failed");
                }
            }

            // Remove from queue
            actionQueue.Actdel(0);
        }
    }
}

void sendWatchDog() {
    StaticJsonDocument<256> doc;
    doc["Device"] = DEVICE_ID;
    doc["Status"] = "Alive";
    doc["TimeStamp"] = timeClient.getEpochTime();
    doc["IP"] = WiFi.localIP().toString();

    String output;
    serializeJson(doc, output);

    if (mqttClient.connected()) {
        mqttClient.publish(TOPIC_STATUS_DEVICE_PUB, output.c_str());
        Serial.println("WatchDog sent");
    }
}

void sendStatusUpdates() {
    Serial.println("Sending Status Updates");

    if (!mqttClient.connected()) return;

    // Relay 1
    StaticJsonDocument<128> doc1;
    doc1["Device"] = DEVICE_ID;
    doc1["System"] = SYSTEM_ONE_NAME;
    doc1["Status"] = digitalRead(RELAY_ONE_PIN);
    String out1;
    serializeJson(doc1, out1);
    mqttClient.publish(TOPIC_STATUS_ONE_PUB, out1.c_str());

    // Relay 2
    StaticJsonDocument<128> doc2;
    doc2["Device"] = DEVICE_ID;
    doc2["System"] = SYSTEM_TWO_NAME;
    doc2["Status"] = digitalRead(RELAY_TWO_PIN);
    String out2;
    serializeJson(doc2, out2);
    mqttClient.publish(TOPIC_STATUS_TWO_PUB, out2.c_str());

    // Relay 3
    StaticJsonDocument<128> doc3;
    doc3["Device"] = DEVICE_ID;
    doc3["System"] = SYSTEM_THREE_NAME;
    doc3["Status"] = digitalRead(RELAY_THREE_PIN);
    String out3;
    serializeJson(doc3, out3);
    mqttClient.publish(TOPIC_STATUS_THREE_PUB, out3.c_str());

    // Relay 4
    StaticJsonDocument<128> doc4;
    doc4["Device"] = DEVICE_ID;
    doc4["System"] = SYSTEM_FOUR_NAME;
    doc4["Status"] = digitalRead(RELAY_FOUR_PIN);
    String out4;
    serializeJson(doc4, out4);
    mqttClient.publish(TOPIC_STATUS_FOUR_PUB, out4.c_str());
}
