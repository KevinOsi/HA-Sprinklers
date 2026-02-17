#ifndef ACTIONSQUEUE_H
#define ACTIONSQUEUE_H

#include <Arduino.h>
#include "Action.h"

class ActionsQueue {
public:
    static const int QueueLength = 20;
    Action ActionList[QueueLength];

    ActionsQueue();
    ~ActionsQueue();

    bool NewAct(Action Ins);
    bool Actins(int ID, Action Ins);
    bool Actdel(int ID);
    String GetList();
    bool PubList(PubSubClient& mqttClient, const char* topic);
    void ClearQueue();
};

#endif // ACTIONSQUEUE_H
