#ifndef ACTION_H
#define ACTION_H

#include <Arduino.h>
#include <PubSubClient.h>

class Action {
public:
    String DeviceID;   // Micro-controller ID
    String SystemID;   // What relay or system is at work
    String SubTopic;   // MQTT Topic Sub
    String PubTopic;   // MQTT Topic Pub
    int PinNum;        // Pin number for output
    int RelayNum;      // Relay number
    bool DoAction;     // Action to be taken (true: ON, false: OFF)
    unsigned long TimeStamp;  // Time stamp for when action takes place

    static int numActions; // hold total actions in queue

    Action(String DeviceID, String SystemID, String SubTopic, String PubTopic, int PinNum, int RelayNum, bool DoAction, unsigned long TimeStamp);
    Action();
    ~Action();

    static int getNumActions() { return numActions; }
    String getJSON();
    bool publish(PubSubClient& mqttClient, String output);
    bool publish(PubSubClient& mqttClient);
};

#endif // ACTION_H
