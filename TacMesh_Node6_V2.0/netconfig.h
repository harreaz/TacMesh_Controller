//netconfig.h

//int ipAddress[] = {172, 16, 0, 46}; // Set controller ip address according to node hostname
//int gateWay[] = {172, 16, 0, 254}; //Set your Gateway IP address
//int subNet[] = {255, 255, 255, 0}; //class c
//int DNS[] = {8, 8, 8, 8}; //optional
//int altDNS[] = {8, 8, 4, 4}; //optional

// #define HostName "Node6"
// const char* ssID = "tac06";                     // change which SSID of tacmesh radio controller would connect
// const char* passWord = "tacmesh*!";                 // password of the SSID
// const char* batteryTopic = "tacmesh/battery";  // battery topic number based on the node number

// const char* mqttServer = "172.16.0.154";  // the address for MQTT Server
// int mqttPort = 1883;                      // port for MQTT Server

// int relayPin = 2;           // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
// int offSet = -20;            // set the correction offset value for voltage reading
// int minVolt = 0;            // MinVoltage x 100
// int maxVolt = 2000;         // MaxVoltage x 100
// int updateInterval = 3000;  // Interval to update battery percentage to MQTT server

// bool relayOn = LOW;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
// bool relayOff = HIGH;  // Opposite of relayon

//int ipAddress[] = {192, 168, 1, 100}; // Set controller ip address according to node hostname
//int gateWay[] = {192, 168, 1, 1}; //Set your Gateway IP address
//int subNet[] = {255, 255, 255, 0}; //class c
//int DNS[] = {8, 8, 8, 8}; //optional
//int altDNS[] = {8, 8, 4, 4}; //optional

#define HostName "Node6"
const char* ssID = "tac06";                     // change which SSID of tacmesh radio controller would connect
const char* passWord = "tacmesh*!";                 // password of the SSID
const char* batteryTopic = "tacmesh/battery";  // battery topic number based on the node number

const char* mqttServer = "broker.hivemq.com";  // the address for MQTT Server
int mqttPort = 1883;                      // port for MQTT Server

int relayPin = 2;           // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
int offSet = -20;            // set the correction offset value for voltage reading
int minVolt = 0;            // MinVoltage x 100
int maxVolt = 2000;         // MaxVoltage x 100
int updateInterval = 3000;  // Interval to update battery percentage to MQTT server

bool relayOn = LOW;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
bool relayOff = HIGH;  // Opposite of relayon

// String nodeType = "triggernode";  // if this is not for the last tacmesh node, replace with "relaynode"
