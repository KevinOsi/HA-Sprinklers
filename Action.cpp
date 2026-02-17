#include "Action.h"

int Action::numActions = 0;

Action::Action(String DeviceID, String SystemID, String SubTopic, String PubTopic, int PinNum, int RelayNum, bool DoAction, unsigned long TimeStamp){
    this->DeviceID = DeviceID;
    this->SystemID = SystemID;
    this->SubTopic = SubTopic;
    this->PubTopic = PubTopic;
    this->PinNum = PinNum;
    this->RelayNum = RelayNum;
    this->DoAction = DoAction;
    this->TimeStamp = TimeStamp;

    // Increment total actions count if you want to track live objects,
    // but in the original code it was incremented on construction and decremented on destruction.
    // However, since we are copying objects into the array, this counter might be tricky.
    // I'll keep the original logic but it might not be accurate if copy constructor is not handled.
    // For now, I'll just follow the original pattern.
    Action::numActions++;
}

Action::Action(){
    this->DeviceID = "";
    this->SystemID = "";
    this->SubTopic = "";
    this->PubTopic = "";
    this->PinNum = 0;
    this->RelayNum = 0;
    this->DoAction = false;
    this->TimeStamp = 0;
    Action::numActions++;
}

Action::~Action(){
    Action::numActions--;
}

String Action::getJSON(){
    // construct the JSON feedback based on the action.
    String myString;
    // Simple JSON construction
    myString = "{\"Device\":\"" + this->DeviceID + "\",\"System\":\"" + this->SystemID + "\",\"Status\":" + (this->DoAction ? "1" : "0") + "}";
    return myString;
}

bool Action::publish(PubSubClient& mqttClient, String output){
    // publish MQTT message
    if (this->PubTopic.length() == 0) return false;

    String Topic = this->PubTopic;

    // Note: PubSubClient::publish returns boolean
    if(!mqttClient.publish(Topic.c_str(), output.c_str())){
        return false;
    }
    return true;
}

bool Action::publish(PubSubClient& mqttClient){
    // publish MQTT message
    String output = this->getJSON();
    return this->publish(mqttClient, output);
}
