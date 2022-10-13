//netconfig.h
// node 3 as relaynode

int ipAddress[] = {172, 16, 0, 43}; // Set controller ip address according to node hostname
int gateWay[] = {172, 16, 0, 254}; //Set your Gateway IP address
int subNet[] = {255, 255, 255, 0}; //class c
int DNS[] = {8, 8, 8, 8}; //optional
int altDNS[] = {8, 8, 4, 4}; //optional

#define HostName "Node3"
const char* ssID = "tac03";                     // change which SSID of tacmesh radio controller would connect
const char* passWord = "tacmesh*!";                 // password of the SSID
const char* batteryTopic = "tacmesh/battery3";  // battery topic number based on the node number

const char* mqttServer = "172.16.0.154";  // the address for MQTT Server
int mqttPort = 1883;                      // port for MQTT Server

int relayPin = 14;           // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
int offSet = -20;            // set the correction offset value for voltage reading
int minVolt = 0;            // MinVoltage x 100
int maxVolt = 2000;         // MaxVoltage x 100
int updateInterval = 3000;  // Interval to update battery percentage to MQTT server

bool relayOn = HIGH;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
bool relayOff = LOW;  // Opposite of relayon

String nodeType = "relaynode";  // if this is not for the last tacmesh node, replace with "relaynode"
