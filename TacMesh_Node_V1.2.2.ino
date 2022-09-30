#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>

WiFiClient espClient;
PubSubClient client(espClient);

// Update these with values suitable for your network.

//************************************** Values yang kena tukar untuk specific node *******************************//
#define HOSTNAME "Node1"
const char* ssid = "tac01";                     // change which SSID of tacmesh radio controller would connect
const char* pswd = "tacmesh*!";                 // password of the SSID
const char* batterytopic = "tacmesh/battery1";  // battery topic number based on the node number

const char* mqtt_server = "172.16.0.154";  // the address for MQTT Server
int mqtt_port = 1883;                      // port for MQTT Server

int relaypin = 2;           // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
int offset = -28;            // set the correction offset value for voltage reading
int minvolt = 0;            // MinVoltage x 100
int maxvolt = 2000;         // MaxVoltage x 100
int updateInterval = 3000;  // Interval to update battery percentage to MQTT server

bool relayon = LOW;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
bool relayoff = HIGH;  // Opposite of relayon

String nodetype = "relaynode";  // if this is not for the last tacmesh node, replace with "relaynode"

IPAddress local_IP(172, 16, 0, 41); // Set controller ip address according to node hostname
IPAddress gateway(172, 16, 0, 254);  // Set your Gateway IP address
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);    //optional
IPAddress secondaryDNS(8, 8, 4, 4);  //optional

//****************************************************************************************************************//

const char* topic = "wemos";  // rhis is the [root topic]
unsigned long lastMsg = millis();
unsigned long triggertime2 = millis();
int triggerinterval = 30000;
int value = 0;
int voltavg;
float timestamp;
String message;
String offperiod;
String batteryvolt;
unsigned long offperiodmicros = 30e6;  //30seconds
bool updateonce = false;
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
  lastMsg = millis();
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
  if (!updateonce) {
    Serial.println("");
    Serial.println("First Time Updating");
    delay(50);

    emitMQTT(batterytopic, batteryvolt);
    delay(50);

    emitMQTT("tacmesh/query", nodetype);
    delay(50);
    updateonce = true;
    lastMsg = millis();
    triggertime2 = millis();
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

void setup() {
  Serial.begin(115200);
  pinMode(relaypin, OUTPUT);  // Initialize the Relay Pin as an output
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
    updateOnce();

    unsigned long triggertime1 = millis();
    if (triggertime1 - triggertime2 >= triggerinterval) {
      if (nodetype == "triggernode") {
        if (!operationmode) {
          Serial.println("Set sleep trigger to true");
          emitMQTT("tacmesh/trigger", HOSTNAME);
          triggerinterval = 2000;
          triggertime2 = millis();
        }
      }
    }
  }

  if (!operationmode && !schedulemode && sleepstatus) {
    Serial.println("Mode = Standby, I'm going into deep sleep mode for " + String(offperiodmicros / 1000000) + "seconds");
    if (nodetype != "triggernode") {
      delay(500);
    }
    ESP.deepSleep(offperiodmicros);
  }
  else if (operationmode || schedulemode || !sleepstatus) {
    if (now - lastMsg >= updateInterval) {
      if (operationmode) {
        Serial.println("Mode = Operation, updating Battery Voltage every " + String(updateInterval / 1000) + "seconds");
      } else if (schedulemode) {
        Serial.println("Mode = Scheduled, updating Battery Voltage every " + String(updateInterval / 1000) + "seconds");
      }
      if (client.connected()) {
        emitMQTT(batterytopic, batteryvolt);
        delay(50);
      }
      lastMsg = millis();
    }
  }
  delay(100);
}
