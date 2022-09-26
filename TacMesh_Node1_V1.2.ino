#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#define HOSTNAME "Node1"

// Update these with values suitable for your network.

const char* ssid = "tac01";
const char* pswd = "tacmesh*!";
const char* mqtt_server = "172.16.0.154";
int mqtt_port = 1883;
const char* topic = "wemos";    // rhis is the [root topic]
const char* batterytopic = "tacmesh/battery1";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
int value = 0;
int voltavg;
String message;
String offperiod;
unsigned long offperiodmicros = 30e6; //30seconds
bool operationmode = false;
bool schedulemode = false;
bool relaystate = false;
bool updateonce = false;
//int relaypin = 14;
int relaypin = 2;
int offset = -26  ; // set the correction offset value
int count;
bool relayon = LOW;
bool relayoff = HIGH;
int minvolt = 0; //in milivolt
int maxvolt = 2000; //in milivolt
int updateInterval = 2500;

int status = WL_IDLE_STATUS;     // the starting Wifi radio's status

IPAddress local_IP(172, 16, 0, 41);
IPAddress gateway(172, 16, 0, 254); // Set your Gateway IP address
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  Serial.print("Connecting to ");
  Serial.println(ssid);
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
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  Serial.println(message);
  Serial.println(String(topic));

  if (String(topic) == "tacmesh/mode") {
    //onperiodmills = onperiod * 60 * 1000;
    // Switch on the LED if an 1 was received as first character
    if (message == "1") {
      Serial.println("Mode: Operation");
      operationmode = true;
    }
    else if (message == "0") {
      Serial.println("Mode: Standby, Periodic every " + String(offperiod) + "minute");
      operationmode = false;
    }
  }
  if (String(topic) == "tacmesh/schedule") {
    //onperiodmills = onperiod * 60 * 1000;
    // Switch on the LED if an 1 was received as first character
    if (message == "true") {
      Serial.println("Mode: Scheduled ON");
      schedulemode = true;

    }
    else if (message == "false") {
      Serial.println("Mode: Scheduled OFF");
      schedulemode = false;
    }
  }
  if (String(topic) == "tacmesh/offperiod") {
    offperiod = message;
    offperiodmicros = stringToLong(offperiod) * 1000000;
    Serial.println("OFF Period: " + String(offperiod) + " seconds");
  }
  message = "";
}

String macToStr(const uint8_t* mac)
{
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

long stringToLong(String s)
{
  char arr[12];
  s.toCharArray(arr, sizeof(arr));
  return atol(arr);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    String clientId = composeClientID() ;
    clientId += "-";
    clientId += String(micros() & 0xff, 16); // to randomise. sort of

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.publish(topic, ("connected " + composeClientID()).c_str() , true );
      String subscription1 = "tacmesh/mode";
      String subscription2 = "tacmesh/offperiod";
      String subscription3 = "tacmesh/schedule";
      client.subscribe(subscription1.c_str());
      client.subscribe(subscription2.c_str());
      client.subscribe(subscription3.c_str());
      Serial.print("subscribed to : ");
      Serial.print(subscription1);
      Serial.println(" and " + subscription2 + " and " + subscription3);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" wifi=");
      Serial.print(WiFi.status());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
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
  for (int i = 0; i < 100; i++)
  {
    int volt = analogRead(A0);// read the input
    voltavg = (voltavg + volt);
    count++;
  }
  int volt = voltavg / count;
  double voltage = map(volt, 0, 1023, minvolt, maxvolt) + offset; // map 0-1023 to 0-2500 and add correction offset
  voltage /= 100; // divide by 100 to get the decimal values
  voltavg = 0;
  count = 0;

  return voltage;
}

void emitMQTT(const char* topics, String payload) {
  String voltage = String(measureVolt());
  client.publish(topics , payload.c_str());
  Serial.println("Sent Update to Node Red : {Voltage:" + String(voltage) + "V}");
}

void setup() {
  Serial.begin(115200);
  pinMode(relaypin, OUTPUT);     // Initialize the Relay Pin as an output
  delay(10);
  turnOnRelay();
  setup_wifi();
  delay(10);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // confirm still connected to mqtt server
  ArduinoOTA.handle();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  String payload = "{\"Voltage\":";
  payload += String(measureVolt());
  payload += "}";

  if (!updateonce) {
    Serial.println("I'm Awake  !!");
    delay(10);
    emitMQTT(batterytopic, payload);
    updateonce = true;
    emitMQTT("tacmesh/query", "Node1");
    Serial.println("Asking query for updates");
    lastMsg = millis();
  }
  unsigned long now = millis();

  if (!operationmode && !schedulemode) {
    if (now - lastMsg >= updateInterval) {
      Serial.println("Mode = Standby, I'm going into deep sleep mode for " + String(offperiodmicros / 1000000) + "seconds");
      Serial.println(operationmode);
      Serial.println(schedulemode);
      ESP.deepSleep(offperiodmicros);
      //Serial.println(operationmode);
    }
  }
  else if (operationmode || schedulemode) {
    if (now - lastMsg >= updateInterval) {
      if (operationmode) {
        Serial.println("Mode = Operation, updating Battery Voltage every " + String(updateInterval / 1000) + "seconds");
      }
      else if (schedulemode) {
        Serial.println("Mode = Scheduled, updating Battery Voltage every " + String(updateInterval / 1000) + "seconds");
      }
      emitMQTT(batterytopic, payload);
      lastMsg = millis();
    }
  }
  delay(100);
}
