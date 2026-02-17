#include "ActionsQueue.h"

// Note: QueueLength is static const in header, so we don't redefine it here unless it's integral constant which it is.

ActionsQueue::ActionsQueue(){
    // Initialize array with default constructor
    for(int i = 0; i < QueueLength; i++) {
        this->ActionList[i] = Action();
    }
}

ActionsQueue::~ActionsQueue(){
    // Destructor does nothing specific as array is member
}

bool ActionsQueue::NewAct(Action Ins){
    int inserted = 0;

    // Check if the first position is empty (timestamp 0)
    // The queue logic assumes that if position 0 is empty (timestamp 0), the queue is empty.
    // However, if we shift elements, empty spots will be at the end.

    if (ActionList[0].TimeStamp > 0) {
        // Queue is not empty, find position

        // Iterate through Array and find order
        // Start checking from index 0 because we might need to insert at the beginning
        // But the original code starts at 1 to compare with i-1.

        // Original logic:
        /*
        for (int i = 1; i < (length - 1); i++) {
            if (Ins.TimeStamp < this->ActionList[i-1].TimeStamp){
                this->Actins(i-1, Ins);
                inserted = 1;
                break;
            }
            if ((Ins.TimeStamp >= this->ActionList[i-1].TimeStamp) && ((Ins.TimeStamp <= this->ActionList[i].TimeStamp) || (!this->ActionList[i].TimeStamp) )){
                this->Actins(i, Ins);
                inserted = 1;
                break;
            }
        }
        */

        // Improved logic: find the first element that has a timestamp > new timestamp
        for (int i = 0; i < QueueLength; i++) {
             // If we find an empty slot (TimeStamp == 0), insert here
             if (this->ActionList[i].TimeStamp == 0) {
                 this->Actins(i, Ins);
                 inserted = 1;
                 break;
             }

             // If the current action is later than the new action, insert here (pushing current back)
             if (this->ActionList[i].TimeStamp > Ins.TimeStamp) {
                 this->Actins(i, Ins);
                 inserted = 1;
                 break;
             }
        }

        // If queue is full and we haven't inserted, we drop the last one?
        // The loop goes to QueueLength-1. If we reach the end, we can't insert.
        // The Actins shifts everything down, so the last element falls off.

    } else {
        // Queue is empty, insert at 0
        this->Actins(0, Ins);
        inserted = 1;
    }

    // Debug: print queue
    // Serial.println("Queue Updated");
    return inserted;
}

bool ActionsQueue::Actins(int ID, Action Ins){
    if (ID >= QueueLength) return false;

    // Shift elements down from end to ID
    for(int i = (QueueLength - 1); i > ID; i--) {
        this->ActionList[i] = this->ActionList[i-1];
    }

    ActionList[ID] = Ins;
    return true;
}

bool ActionsQueue::Actdel(int ID){
    if (ID >= QueueLength) return false;

    // Shift elements up from ID+1 to end
    for(int i = ID; i < (QueueLength - 1); i++){
        this->ActionList[i] = this->ActionList[i+1];
    }

    // Clear last item
    this->ActionList[QueueLength - 1] = Action();

    return true;
}

String ActionsQueue::GetList(){
    // Construct JSON array string
    String myString = "{\"Queue\":[";

    bool first = true;
    for(int i = 0; i < QueueLength; i++) {
        if (this->ActionList[i].TimeStamp > 0) {
            if (!first) myString += ",";
            // Simple representation: RelayNum and Action
            myString += "{\"R\":" + String(this->ActionList[i].RelayNum) + ",\"A\":" + String(this->ActionList[i].DoAction) + ",\"T\":" + String(this->ActionList[i].TimeStamp) + "}";
            first = false;
        }
    }
    myString += "]}";
    return myString;
}

bool ActionsQueue::PubList(PubSubClient& mqttClient, const char* topic){
    String myString = this->GetList();
    if (String(topic).length() == 0) return false;

    if(!mqttClient.publish(topic, myString.c_str())){
        return false;
    }
    return true;
}

void ActionsQueue::ClearQueue(){
    for(int i = 0; i < QueueLength; i++){
        this->ActionList[i] = Action();
    }
}
