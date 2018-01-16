#include <Arduino.h>
#include <esp8266wifi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <esp8266webserver.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "webservdata.h" //stores the generic junk for the webserver
#include "default.h" //setup your defaults for local servers


// WIFI parameters
const char* SSID = MY_WIFI_SSID;  // Use your wifi SSID
const char* PASSWORD = MY_WIFI_PASSWORD; // Use your WIFI password
String NodeRedAlive = "CONNECTED";
String MyIP = "";

//NPT config
WiFiUDP ntpUDP;
NTPClient TimeClient(ntpUDP, "ca.pool.ntp.org", -21600 , 60000); // use canadian pool and set offset for local -6 hours

// MQTT Config
const char* BROKER_MQTT = MY_BROKER_MQTT_IP;  //setup your MQTT server settings
int BROKER_PORT = MY_BROKER_PORT;
const char* MQTT_USER = MY_MQTT_USER;
const char* MQTT_PASS = MY_MQTT_PASS;


WiFiClient espClient;
PubSubClient MQTT(espClient); // MQTT client setup
String MQTTConnected = "";

//Setup
String DeviceID = "ESP Irrigation";
String SystemOne = "Sprinkler Zone 1";
String SystemTwo = "Sprinkler Zone 2";
String SystemThree = "Sprinkler Zone 3";
String SystemFour = "Sprinkler Zone 4";


//pin config
int relayOne = 12;
int relayTwo = 13;
int relayThree = 14;
int relayFour = 16;


//MQTT topics
String TopicRelayOne = "Home/Irrigation/1/Relays/1"; //Relay1 MQTT sub
String TopicRelayTwo = "Home/Irrigation/1/Relays/2";  //Relay2 MQTT sub
String TopicRelayThree = "Home/Irrigation/1/Relays/3";  //Relay3 MQTT sub
String TopicRelayFour = "Home/Irrigation/1/Relays/4";  //Relay4 MQTT sub
String TopicStatusNR = "Home/Status/WatchDog/1";  //Node red system health sub
String TopicAllCmd = "Home/Irrigation/1/All"; // send commands to entire system

String TopicStatusOne = "Home/Irrigation/1/Status/1"; //Relay1 status Publish
String TopicStatusTwo = "Home/Irrigation/1/Status/2"; //Relay2 status Publish
String TopicStatusThree = "Home/Irrigation/1/Status/3";  //Relay3 status Publish
String TopicStatusFour = "Home/Irrigation/1/Status/4";  //Relay4 status Publish
String TopicStatusDevice = "Home/Irrigation/1/Status/WatchDog"; //node keep alive Publish
String TopicStatusQueue = "Home/Irrigation/1/Status/Queue"; //Publish actions queue

//Web server
ESP8266WebServer webServer(80);
String myPage = "";


//Timer Setup
double WDTimer = 0;  //watchdog timer for keep alive state
double UDTimer = 0;  //Relay status update timer
double QUTimer = 0;  //queue update timer

//classes
class Action {
  public:
	String DeviceID;   // Micro-controller ID
	String SystemID;   //What relay or system is at work
	String SubTopic;  //MQTT Topic Sub  maybe don't need???
	String PubTopic;  //MQTT Topic Pub
	int PinNum;    //Pin number for output
	int RelayNum;  //Relay number
	bool DoAction;   // what action is to be taken, open / close
	//double Timer;    // Delay function for execution  REMOVE
	unsigned long TimeStamp;  // Time stamp for when action takes place

	static int numActions; // hold total actions in queue

	Action(String DeviceID, String SystemID, String SubTopic, String PubTopic, int PinNum, int RelayNum, bool DoAction, unsigned long TimeStamp);
	Action();
	~Action();
	static int getNumActions() { return numActions; }
	String getMQTT();
	bool publish(String output);
	bool publish();

};

int Action::numActions = 0;

Action::Action(String DeviceID, String SystemID, String SubTopic, String PubTopic, int PinNum, int RelayNum, bool DoAction, unsigned long TimeStamp){

	this -> DeviceID = DeviceID;
	this -> SystemID = SystemID;
	this -> SubTopic = SubTopic;
	this -> PubTopic = PubTopic;
	this -> PinNum = PinNum;
	this -> RelayNum = RelayNum;
	this -> DoAction = DoAction;
	this -> TimeStamp = TimeStamp;


}

Action::Action(){

	this -> DeviceID = "";
	this -> SystemID = "";
	this -> SubTopic = "";
	this -> PubTopic = "";
	this -> PinNum = 0;
	this -> RelayNum = 0;
	this -> DoAction = 0;
	this -> TimeStamp = 0;


}

Action::~Action(){

	Action::numActions--;
}

String Action::getMQTT(){
	// construct the MQTT feedback based on the action.
	String myString;

	myString = "{\"Device\" : \""  + this -> DeviceID + "\" , \"System\" : \"" + this -> SystemID + "\" , \"Status\" : " + this -> DoAction + "}" ;

	return myString;
}

bool Action::publish(String output){
	//  publish MQTT message

	String Topic = this ->PubTopic;

	if(!MQTT.publish(Topic.c_str() , output.c_str(), sizeof(output))){

		return 0;

	}

	return 1;
}

bool Action::publish(){
	//  publish MQTT message
	String output = this->getMQTT();

	String Topic = this ->PubTopic;

	if(!MQTT.publish(Topic.c_str() , output.c_str(), sizeof(output))){

		return 0;

	}

	return 1;
}


//Actions QueueClass
class ActionsQueue
{
	public:
		static int QueueLength;
		Action ActionList[20];  // action list max 20 actions for now

		ActionsQueue();
		~ActionsQueue(){};
		bool NewAct(Action Ins);   //insert an action into list based on time
		bool Actins(int ID, Action Ins); // push action to start
		bool Actdel(int ID); // pop action at specific point, adjust list
		String GetList(); // get a JSON format listing of the actions queue
		bool PubList(); //publish list to the list topic
		bool ClearQueue();  //reset all relays to off, clear the queue
};

int ActionsQueue::QueueLength = 20;  //set queue length

ActionsQueue::ActionsQueue(){

	this->ActionList[0] = Action();

}

bool ActionsQueue::NewAct(Action Ins){

	//int length = sizeof(this ->ActionList);
	int length = this->QueueLength;
	int inserted = 0;

	//default, dump into first position if blank
	if (ActionList[0].TimeStamp > 0) {

		//itterate through Array and find order, start on position [1]
		for (int i = 1; i < (length - 1); i++) {


			if (Ins.TimeStamp < this->ActionList[i-1].TimeStamp){

				this->Actins(i-1, Ins);
				//Serial.println("inserting task in -> " + String(i-1));
				inserted = 1;
				break;

			}

			if ((Ins.TimeStamp >= this->ActionList[i-1].TimeStamp) && ((Ins.TimeStamp <= this->ActionList[i].TimeStamp) || (!this->ActionList[i].TimeStamp) )){
				// greater than fist position and less than second position

				this->Actins(i, Ins);
				//Serial.println("inserting task in -> " + String(i));
				inserted = 1;
				break;
			}


		}
	} else {


		this->Actins(0, Ins);

	}


	// Debug display queue, only top 10 actions will be listed
	for(int i = 0 ; i < 10 ; i++){

		Serial.println(String(i) + " - " + this->ActionList[i].PubTopic + " - " + this->ActionList[i].DoAction + " - " + this->ActionList[i].TimeStamp );
	}




	return 1;
}

bool ActionsQueue::Actins(int ID, Action Ins){

	//int length = sizeof(this ->ActionList);
	int length = this->QueueLength;


	for(int i = (length - 1); i > ID; i--)
	{
		this -> ActionList[i] = this -> ActionList[i-1];
	}


	ActionList[ID] = Ins;
	return 1;

}

bool ActionsQueue::Actdel(int ID){

	//int length = sizeof(this ->ActionList);
	int length = this->QueueLength;

	for( int i = ID; i < (length - 1); i++){
		this -> ActionList[i] = this -> ActionList[i+1];

	}

	// blank out last item in list
	this -> ActionList[length] = Action();

	return 1;
}

String ActionsQueue::GetList(){

	//String myString = "{\"Device\" : \""  + DeviceID + "\" , \"Queue\" : ["   ;

	int length = this->QueueLength;
	/*
	// FULL QUEUE AS JSON OBJ
	for( int i = 0; i < (length-1); i++){
		myString = myString + "{\"Pos\" : "  + i + " , \"Status\" : " + this->ActionList[i].DoAction + "}," ;

	}
	myString = myString + "{\"Pos\" : "  + length + " , \"Status\" : " + this->ActionList[length].DoAction + "}" ; //formating for last one

	myString = myString + "]}";

	*/

	String myString = "{\"Device\" : \""  + DeviceID + "\" , \"Queue\" : \""   ;
	for( int i = 0; i < (length-1); i++)
	{
			myString = myString + this->ActionList[i].RelayNum  + this->ActionList[i].DoAction ;

	}



	myString = myString + "\"}";







	return myString;
}

bool ActionsQueue::PubList(){

	String myString = this->GetList();

	if(!MQTT.publish(TopicStatusQueue.c_str() , myString.c_str(), sizeof(myString))){
		Serial.println("MQTT QUEUE LIST PUB FAIL");

		return 0;
	}
	else{
		myString = this->GetList();

		Serial.println("MQTT SENT -> " + myString);
	}


	return 1;
}

bool ActionsQueue::ClearQueue(){

	int length = this->QueueLength;

	//reset all relays directly
	digitalWrite(relayOne, 0);
	digitalWrite(relayTwo, 0);
	digitalWrite(relayThree, 0);
	digitalWrite(relayFour, 0);

	for( int i = 0; i < length; i++){
		this->ActionList[i] = Action();

	}


}


//Actions Queue
Action myActions;  // holds one action
ActionsQueue myQueue;  // holds actions in queue

//  functions and stuff
void initPins() {
	pinMode(relayOne, OUTPUT);
	pinMode(relayTwo, OUTPUT);
	pinMode(relayThree, OUTPUT);
	pinMode(relayFour, OUTPUT);
	digitalWrite(relayOne, 0);
	digitalWrite(relayTwo, 0);
	digitalWrite(relayThree, 0);
	digitalWrite(relayFour, 0);
}

void initSerial() {
	Serial.begin(115200);
}

void initWiFi() {
	delay(10);
	Serial.println();
	Serial.print("Connecting: ");
	Serial.println(SSID);

	WiFi.mode(WIFI_STA);
	WiFi.begin(SSID, PASSWORD); // Wifi Connect
	while (WiFi.status() != WL_CONNECTED) {
		delay(100);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print(SSID);
	Serial.print(" | IP | ");
	Serial.print(WiFi.localIP());
	Serial.print(" | MAC ");
	Serial.println(WiFi.macAddress());


	MyIP = String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);

}

// Receive messages
// format for JSON is {"Device":"Sprinklers Zone 1","Action":1, "Delay":2000, "Schedule":000}
void mqtt_callback(char* topic, byte* payload, unsigned int length) {

	String message;
	StaticJsonBuffer<600> jsonBuffer;
	double ActionTimeStamp;
	double ActionDelay;
	int action;

	//split out payload in message, parse into JSON buffer;
	for (int i = 0; i < length; i++) {
		char c = (char) payload[i];
		message += c;
	}

	// serial debug JSON msg and topic recieved
	Serial.print("Topic ");
	Serial.print(topic);
	Serial.print(" | ");
	Serial.println(message);

	// JSON parser
	JsonObject& parsed = jsonBuffer.parseObject(message);
	if (!parsed.success()) {
		Serial.println("parseObject() failed");
		return;
	}

	//const char* deviceID = parsed["Device"];
	//{"Device":"Sprinklers Zone 1","Action":2, "Schedule":0}
	action = parsed["Action"];
	ActionDelay = parsed["Delay"];

	//check if there is a timestamp included
	if(!parsed["TimeStamp"])	{
		ActionTimeStamp = TimeClient.getEpochTime();

	}
	else{
		ActionTimeStamp = parsed["TimeStamp"];
	}

	//debug
	//Serial.print("Hey the new action is at Epoch " + String(ActionTimeStamp) + " and a delay of ");


	//check if there is a delay on the JSON
	if(ActionDelay > 0 ){
		ActionTimeStamp = ActionTimeStamp + atof(parsed["Delay"]);
	}

	Serial.println( String(ActionDelay) + " New time = " + String(ActionTimeStamp));


	if(!strcmp(topic, TopicAllCmd.c_str())){  // Device commands check
		 // check for clear command
		 if(parsed["ClearQueue"]){

			 myQueue.ClearQueue();
			 Serial.println("Queue has been cleared");

			 myQueue.PubList();

		 }

	}

	if (!strcmp(topic, TopicRelayOne.c_str())) {
		Action AddMe(DeviceID, SystemOne, String(topic), TopicStatusOne,
				relayOne, 1, action, ActionTimeStamp);
		myQueue.NewAct(AddMe);
	}
	if (!strcmp(topic, TopicRelayTwo.c_str())) {
		Action AddMe(DeviceID, SystemTwo, String(topic), TopicStatusTwo,
				relayTwo, 2, action, ActionTimeStamp);
		myQueue.NewAct(AddMe);
	}
	if (!strcmp(topic, TopicRelayThree.c_str())) {
		Action AddMe(DeviceID, SystemThree, String(topic), TopicStatusThree,
				relayThree, 3, action, ActionTimeStamp);
		myQueue.NewAct(AddMe);
	}
	if (!strcmp(topic, TopicRelayFour.c_str())) {
		Action AddMe(DeviceID, SystemFour, String(topic), TopicStatusFour,
				relayFour, 4, action, ActionTimeStamp);
		myQueue.NewAct(AddMe);
	}



  message = "";
  Serial.println();
  Serial.flush();

}

// MQTT Broker connection
void initMQTT() {
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

//MQTT keep alive
void reconnectMQTT() {
	while (!MQTT.connected()) {
		Serial.print("Attempting MQTT Broker Connection: ");
		Serial.println(BROKER_MQTT);
		if (MQTT.connect(DeviceID.c_str(), MQTT_USER, MQTT_PASS)) {   // set unique name
			Serial.println("Connected");

			MQTT.subscribe(TopicRelayOne.c_str(), 1);
			MQTT.subscribe(TopicRelayTwo.c_str(), 1);
			MQTT.subscribe(TopicRelayThree.c_str(), 1);
			MQTT.subscribe(TopicRelayFour.c_str(), 1);
			MQTT.subscribe(TopicStatusNR.c_str(), 1);
			MQTT.subscribe(TopicAllCmd.c_str(), 1);

			MQTTConnected = "CONNECTED";
		} else {
			Serial.println("Connection Failed");
			Serial.println("Attempting Reconnect in 2 seconds");

			MQTTConnected = "DISABLED";
			delay(2000);
		}
	}
}

//Wifi Keep alive
void recconectWiFi() {
	while (WiFi.status() != WL_CONNECTED) {
		delay(100);
		Serial.print(".");
	}
}


//function to rebuild the page
void BuildPage()
{
  String Zone1 = "";
  String Zone2 = "";
  String Zone3 = "";
  String Zone4 = "";

  // get pin status

  if(digitalRead(relayOne)){
    Zone1 = "On";
    }else{
    Zone1 = "Off";
    }
  if(digitalRead(relayTwo)){
    Zone2 = "On";
    }else{
    Zone2 = "Off";
    }
  if(digitalRead(relayThree)){
    Zone3 = "On";
    }else{
    Zone3 = "Off";
    }
  if(digitalRead(relayFour)){
      Zone4 = "On";
      }else{
      Zone4 = "Off";
      }
  //read current temperature readings


  myPage = Header + MQTTConnected + Header2 + NodeRedAlive + Header3 + Zone1 + Header4 + Zone2 + Header5 + Zone3 + Header6 +  Zone4  +
		  Body1 + TopicRelayOne + Table1 + TopicRelayTwo + Table1 + TopicRelayThree + Table1 + TopicRelayFour + Table1 + TopicStatusNR +
		  Body2 + TopicStatusOne + Table1 + TopicStatusTwo + Table1 + TopicStatusThree + Table1 + TopicStatusFour + Table1 + TopicStatusDevice +
		  Body3 + Form;

}


void returnFail(String msg)
{
  webServer.sendHeader("Connection", "close");
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(500, "text/plain", msg + "\r\n");
}


//handle web submit
void handleSubmit(){


	int rebuild = 0;  // flag if changes then rebuild;

	//debug
	Serial.println("Web input received, Relay 1 " + webServer.arg("Relay1") + " Relay 2 "  + webServer.arg("Relay2") + " Relay 3 "  + webServer.arg("Relay3") + " Relay 4 "  + webServer.arg("Relay4"));


	// checks arguments and then sends in new commands

	if (!webServer.hasArg("Relay1")) {
		return returnFail("BAD ARGS");
	} else {
		String relay1Val = webServer.arg("Relay1");
		if (relay1Val == "1") {
				myQueue.NewAct(Action(DeviceID,SystemOne,TopicRelayOne,TopicStatusOne,relayOne,1,1,TimeClient.getEpochTime() ));
				rebuild = 1;
			} else if (relay1Val == "0") {
				myQueue.NewAct(Action(DeviceID,SystemOne,TopicRelayOne,TopicStatusOne,relayOne,0,1,TimeClient.getEpochTime() ));
				rebuild = 1;
			} else {
				returnFail("Bad Relay value on 1");
			}
	}

	if (!webServer.hasArg("Relay2")) {
			return returnFail("BAD ARGS");
		} else {
			String relay2Val = webServer.arg("Relay2");
			if (relay2Val == "1") {
					myQueue.NewAct(Action(DeviceID,SystemTwo,TopicRelayTwo,TopicStatusTwo,relayTwo,1,2,TimeClient.getEpochTime() ));
					rebuild = 1;
				} else if (relay2Val == "0") {
					//digitalWrite(relayOne, 0);
					myQueue.NewAct(Action(DeviceID,SystemTwo,TopicRelayTwo,TopicStatusTwo,relayTwo,0,2,TimeClient.getEpochTime() ));
					rebuild = 1;
				} else {
					returnFail("Bad Relay value on 2");
				}
		}

	if (!webServer.hasArg("Relay3")) {
			return returnFail("BAD ARGS");
		} else {
			String relay3Val = webServer.arg("Relay3");
			if (relay3Val == "1") {
					myQueue.NewAct(Action(DeviceID,SystemThree,TopicRelayThree,TopicStatusThree,relayThree,1,3,TimeClient.getEpochTime() ));
					rebuild = 1;
				} else if (relay3Val == "0") {
					myQueue.NewAct(Action(DeviceID,SystemThree,TopicRelayThree,TopicStatusThree,relayThree,0,3,TimeClient.getEpochTime() ));
					rebuild = 1;
				} else {
					returnFail("Bad Relay value on 3");
				}
		}

	if (!webServer.hasArg("Relay4")) {
			return returnFail("BAD ARGS");
		} else {
			String relay4Val = webServer.arg("Relay4");
			if (relay4Val == "1") {
					myQueue.NewAct(Action(DeviceID,SystemFour,TopicRelayFour,TopicStatusFour,relayFour,1,4,TimeClient.getEpochTime() ));
					rebuild = 1;
				} else if (relay4Val == "0") {
					myQueue.NewAct(Action(DeviceID,SystemFour,TopicRelayFour,TopicStatusFour,relayFour,0,4,TimeClient.getEpochTime() ));
					rebuild = 1;
				} else {
					returnFail("Bad Relay value on 4");
				}
		}








	//if there are changes and a rebuild is called activate
	if (rebuild) {
		BuildPage();
		webServer.send(200, "text/html", myPage);

	}


}



//handles the web at / root
void handle_root() {


  if(webServer.hasArg("Relay1") || webServer.hasArg("Relay2") || webServer.hasArg("Relay1") || webServer.hasArg("Rela4") ){

      handleSubmit();  //  turn off for now

  }
  else{
    BuildPage();
	webServer.send(200, "text/html", myPage);
  }


}

//Start webserver up
void startWebserver()
{


    webServer.on("/", handle_root);

    webServer.begin();
    Serial.println("setting up web server");
}

void WatchDogTimer() {


	String BufferTxt = "{\"Device\" : \"" + DeviceID
			+ "\" , \"Status\" : \"Alive\" ,  \"TimeStamp\" : "
			+ String(TimeClient.getEpochTime()) + " , \"IP\" : \"" + MyIP + "\"}";

	//Debug
	Serial.println(BufferTxt);

	if (!MQTT.publish(TopicStatusDevice.c_str(), BufferTxt.c_str(),
			sizeof(BufferTxt))) {

		Serial.println("MQTT pub error  !");
	}

}

//publish a regular update on the pin read for each relay
void SatusUpdates(){

	String myString;

	Serial.println("-- Relay status Updates --");

	myString = "{\"Device\" : \""  + DeviceID + "\" , \"System\" : \"" + SystemOne + "\" , \"Status\" : " + digitalRead(relayOne) + "}" ;
	if(!MQTT.publish(TopicStatusOne.c_str() , myString.c_str(), sizeof(myString))){
		Serial.println("MQTT PUB FAIL");
	}
	else{
		Serial.println(myString);
	}

	myString = "{\"Device\" : \""  + DeviceID + "\" , \"System\" : \"" + SystemTwo + "\" , \"Status\" : " + digitalRead(relayTwo) + "}" ;
	if(!MQTT.publish(TopicStatusTwo.c_str() , myString.c_str(), sizeof(myString))){
		Serial.println("MQTT PUB FAIL");
	}
	else{
		Serial.println(myString);
	}

	myString = "{\"Device\" : \""  + DeviceID + "\" , \"System\" : \"" + SystemThree + "\" , \"Status\" : " + digitalRead(relayThree) + "}" ;
	if(!MQTT.publish(TopicStatusThree.c_str() , myString.c_str(), sizeof(myString))){
		Serial.println("MQTT PUB FAIL");
	}
	else{
		Serial.println(myString);
	}

	myString = "{\"Device\" : \""  + DeviceID + "\" , \"System\" : \"" + SystemFour + "\" , \"Status\" : " + digitalRead(relayFour) + "}" ;
	if(!MQTT.publish(TopicStatusFour.c_str() , myString.c_str(), sizeof(myString))){
		Serial.println("MQTT PUB FAIL");
	}
	else{
		Serial.println(myString);
	}

	Serial.println("-- Relay status Updates END --");

}



void setup() {


	//run init functions
	initPins();
	initSerial();

	initWiFi();
	initMQTT();

	// start web server!
	startWebserver();

	//Start Timeclient
	TimeClient.begin();
	TimeClient.update();
	Serial.println("Starup time - Epoch time " + String(TimeClient.getEpochTime()) + " - Formated time - Day" + TimeClient.getDay() + " - " + TimeClient.getFormattedTime());

	//Setup Timers
	WDTimer = millis(); //watch dog cycle timer
	UDTimer = millis(); //status updates timer
	QUTimer = millis(); //setup queue status update timer

}

void loop() {


  webServer.handleClient();

	if (!MQTT.connected()) {
		reconnectMQTT(); // Retry Worker MQTT Server connection
	}
	recconectWiFi(); // Retry WiFi Network connection
	MQTT.loop();


	//check if time stamp is active on first in queue, else if empty skip
	if(myQueue.ActionList[0].TimeStamp > 0){
		// check if current time is greater than first action in queue
		if (TimeClient.getEpochTime() > (myQueue.ActionList[0].TimeStamp) ) {


			//execute task
			digitalWrite(myQueue.ActionList[0].PinNum, myQueue.ActionList[0].DoAction);
			//publish status update to MQTT topic
			if (!myQueue.ActionList[0].publish()) {
				Serial.println("MQTT post failed ");
			}


			//kill last message
			myQueue.Actdel(0);
		}

	}



	//send out watch dog timer update every 20 seconds
	if(millis() > (WDTimer + 20000)){

		WDTimer = millis();
		TimeClient.update();
		WatchDogTimer();
	}

	//refresh relays status on all relays every 5 minutes
	if(millis() > (UDTimer + 300000)){

		UDTimer = millis();
		SatusUpdates();



	}
	//ADD  queue Publish every minute
	if(millis() > (QUTimer + 60000)){

		QUTimer = millis();

		myQueue.PubList();



	}


}
