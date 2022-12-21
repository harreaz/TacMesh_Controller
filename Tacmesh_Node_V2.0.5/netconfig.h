//netconfig.h

struct {
  //int ipAddress[] = {192, 168, 1, 100}; // Set controller ip address according to node hostname
  //int gateWay[] = {192, 168, 1, 1}; //Set your Gateway IP address
  //int subNet[] = {255, 255, 255, 0}; //class c
  //int DNS[] = {8, 8, 8, 8}; //optional
  //int altDNS[] = {8, 8, 4, 4}; //optional

  const char* hostName =  "Node5";  // define hostname as node number
  const char* ssID = "tac05";                     // change which SSID of tacmesh radio controller would connect
  const char* passWord = "tacmesh*!";                 // password of the SSID
  const char* rootTopic = "tacmesh/logs";  // rhis is the [root topic]
  const char* batteryTopic = "tacmesh/battery";  // for nodes to publish battery voltage topic
  const char* modeTopic = "tacmesh/mode"; // for nodes to get the mode settings from server topic
  const char* nodeSettingsTopic = "tacmesh/nodesettings"; // for nodes to get the latest settings from server topic
  const char* queryTopic = "tacmesh/query"; // for nodes to get the latest settings from server topic

  const char* mqttServer = "172.16.0.154";  // the address for MQTT Server
  int mqttPort = 1883;                      // port for MQTT Server

  int relayPin = 14;           // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
  int offSet = -48;            // set the correction offset value for voltage reading
  int minVolt = 0;            // MinVoltage x 100
  int maxVolt = 2000;         // MaxVoltage x 100
  int updateInterval = 3000;  // Interval to update battery percentage to MQTT server

  bool relayOn = HIGH;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
  bool relayOff = LOW;  // Opposite of relayon
} settings;
