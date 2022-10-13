#include "TickTwo.h"
#include "netconfig.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>

void updateOnce();// initiated on top so that ticker function can be refferenced to the function
void triggerSleep();// initiated on top so that ticker function can be refferenced to the function
void ledBlink();// initiated on top so that ticker function can be refferenced to the function
TickTwo ticker1(updateOnce, 0, 1); // create TickTwo object as ticker1, calling the function updateOnce, to start immediately without delay, run once
TickTwo trigger1(triggerSleep, 30000); // create TickTwo object as ticker1, calling the function triggerSleep, to start after 30s
TickTwo wificonnectblink(ledBlink,0);
TickTwo emitblink(ledBlink,0);

WiFiClient espClient;
PubSubClient client(espClient);

// Update these with values suitable for your network.

//************************************** Change the settings on the configuration file, do not change here *******************************//
#define HOSTNAME HostName
const char* ssid = ssID;                     // change which SSID of tacmesh radio controller would connect
const char* pswd = passWord;                 // password of the SSID
const char* batterytopic = batteryTopic;  // battery topic number based on the node number

const char* mqtt_server = mqttServer;  // the address for MQTT Server
int mqtt_port = mqttPort;                      // port for MQTT Server

int relaypin = relayPin;           // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
int offset = offSet;            // set the correction offset value for voltage reading
int minvolt = minVolt;            // MinVoltage x 100
int maxvolt = maxVolt;         // MaxVoltage x 100
int updateinterval = updateInterval;  // Interval to update battery percentage to MQTT server

bool relayon = relayOn;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
bool relayoff = relayOff;  // Opposite of relayon

String nodetype = nodeType;  // if this is not for the last tacmesh node, replace with "relaynode"

IPAddress local_IP(ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3]); // Set controller ip address according to node hostname
IPAddress gateway(gateWay[0], gateWay[1], gateWay[2], gateWay[3]);  // Set your Gateway IP address
IPAddress subnet(subNet[0], subNet[1], subNet[2], subNet[3]); // class c
IPAddress primaryDNS(DNS[0], DNS[1], DNS[2], DNS[3]);    //optional
IPAddress secondaryDNS(altDNS[0], altDNS[1], altDNS[2], altDNS[3]);  //optional

//**************************************************************************************************************************************//

const char* topic = "wemos";  // rhis is the [root topic]
unsigned long lastMsg = millis();
int voltavg;
float timestamp;
String message;
String batteryvolt;
unsigned long offperiodmicros = 30e6;  //30seconds
bool operationmode = false;
bool schedulemode = false;
bool relaystate = false;
bool sleepstatus = false;
int count;
int status = WL_IDLE_STATUS;  // the starting Wifi radio's status

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  wificonnectblink.update();
  }
  Serial.println("");
  Serial.println("WiFi connected");
  ArduinoOTA.setHostname(HOSTNAME);

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] , Content = ");
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  //  Serial.println(message);
  //  Serial.println(String(topic));

  if (String(topic) == "tacmesh/mode") {
    //onperiodmills = onperiod * 60 * 1000;
    // Switch on the LED if an 1 was received as first character
    String mode;
    if (message == "1") {
      operationmode = true;
      mode = "Operation";
    } else if (message == "0") {
      operationmode = false;
      mode = "Standby";
    }
    Serial.println("Mode: " + mode);
  }

  if (String(topic) == "tacmesh/schedule") {
    if (message == "true") {
      schedulemode = true;
    } else if (message == "false") {
      schedulemode = false;
    }
    Serial.println("Schedule Mode: " + String(schedulemode));
  }

  if (String(topic) == "tacmesh/offperiod") {
    String offperiod = message;
    offperiodmicros = stringToLong(offperiod) * 1000000;
    Serial.println("OFF Period: " + String(offperiod) + " seconds");
  }

  if (String(topic) == "tacmesh/timestamp") {
    String timestampmillis = message;
    timestamp = timestampmillis.toFloat() / 1000 + 28800;
    time_t t = int(timestamp);
    String actualtime = String(year(t)) + "/" + String(month(t)) + "/" + String(day(t));
    actualtime += " " + String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t));
    Serial.println("Actual Time: " + actualtime);
  }

  if (String(topic) == "tacmesh/sleep") {
    if (message == "true") {
      sleepstatus = true;
    } else if (message == "false") {
      sleepstatus = false;
    }
    Serial.println("Sleep Status : " + String(sleepstatus));
  }
  message = "";
}

String macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

String composeClientID() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientId;
  clientId += "esp-";
  clientId += macToStr(mac);
  return clientId;
}

long stringToLong(String s) {
  char arr[12];
  s.toCharArray(arr, sizeof(arr));
  return atol(arr);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("");
    Serial.print("Attempting MQTT connection...");

    String clientId = composeClientID();
    clientId += "-";
    clientId += String(micros() & 0xff, 16);  // to randomise. sort of

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish(topic, ("connected " + composeClientID()).c_str(), true);
      String subscription1 = "tacmesh/mode";
      String subscription2 = "tacmesh/offperiod";
      String subscription3 = "tacmesh/schedule";
      String subscription4 = "tacmesh/timestamp";
      String subscription5 = "tacmesh/sleep";
      client.subscribe(subscription1.c_str());
      client.subscribe(subscription2.c_str());
      client.subscribe(subscription3.c_str());
      client.subscribe(subscription4.c_str());
      client.subscribe(subscription5.c_str());
      Serial.print("subscribed to : ");
      Serial.print(subscription1);
      Serial.println(" and " + subscription2 + " and " + subscription3 + " and " + subscription4 + " and " + subscription5);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      break;
    }
  }
}

void updateOnce() {
  Serial.println("");
  Serial.println("First Time Updating");
  delay(50);

  emitMQTT(batterytopic, batteryvolt);
  delay(50);

  emitMQTT("tacmesh/query", nodetype);
  delay(50);

  lastMsg = millis();
}

void triggerSleep() {
  if (nodetype == "triggernode") {
    if (!operationmode) {
      Serial.println("Set sleep trigger to true");
      emitMQTT("tacmesh/trigger", HOSTNAME);
    }
  }
}

void turnOnRelay() {
  bool relaystate = relayon;
  digitalWrite(relaypin, relaystate);
  Serial.println("Relay Turned ON");
}

void turnOffRelay() {
  bool relaystate = relayoff;
  digitalWrite(relaypin, relaystate);
  Serial.println("Relay Turned OFF");
}

float measureVolt() {
  for (int i = 0; i < 1000; i++) {
    int volt = analogRead(A0);  // read the input
    voltavg = (voltavg + volt);
    count++;
  }
  int volt = voltavg / count;
  double voltage = map(volt, 0, 1023, minvolt, maxvolt) + offset;  // map 0-1023 to 0-2500 and add correction offset
  voltage /= 100;                                                  // divide by 100 to get the decimal values
  voltavg = 0;
  count = 0;

  return voltage;
}

void emitMQTT(const char* topics, String payload) {
  if (topics == "tacmesh/query") {
    Serial.println("Query update from Server");
  }
  else if (topics == "tacmesh/trigger") {
    Serial.println("Sending Sleep Trigger to Server");
  }
  else {
    String voltage = String(measureVolt());
    Serial.println("Sending battery update to Server : {Voltage:" + String(voltage) + "V}");
    Serial.println(" ");
  }
  client.publish(topics, payload.c_str());
}

void ledBlink() {
  int ledstate = digitalRead(LED_BUILTIN);
  digitalWrite(LED_BUILTIN, !ledstate);
  delay(100);
  digitalWrite(LED_BUILTIN, ledstate);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(relaypin, OUTPUT);  // Initialize the Relay Pin as an output
  ticker1.start();
  trigger1.start();
  wificonnectblink.start();
  emitblink.start();
  delay(10);
  turnOnRelay();
  setup_wifi();
  delay(10);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  lastMsg = millis();
}

void loop() {
  // confirm still connected to mqtt server
  unsigned long now = millis();
  batteryvolt = "{\"Voltage\":";
  batteryvolt += String(measureVolt());
  batteryvolt += "}";

  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }

  else if (client.connected()) {
    client.loop();
    ticker1.update();
    trigger1.update();
    if (trigger1.counter() == 1) trigger1.interval(2000);
  }

  if (!operationmode && !schedulemode && sleepstatus) {
    Serial.println("Mode = Standby, I'm going into deep sleep mode for " + String(offperiodmicros / 1000000) + "seconds");
    if (nodetype != "triggernode") {
      delay(500);
    }
    ESP.deepSleep(offperiodmicros);
  }
  else if (operationmode || schedulemode || !sleepstatus) {
    if (now - lastMsg >= updateinterval) {
      if (operationmode) {
        Serial.println("Mode = Operation, updating Battery Voltage every " + String(updateinterval / 1000) + "seconds");
      } else if (schedulemode) {
        Serial.println("Mode = Scheduled, updating Battery Voltage every " + String(updateinterval / 1000) + "seconds");
      }
      if (client.connected()) {
        emitMQTT(batterytopic, batteryvolt);
        emitblink.update();
        
      }
      lastMsg = millis();
    }
  }
  delay(100);
}
