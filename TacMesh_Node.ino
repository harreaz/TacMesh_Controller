/*
  Simple wemos D1 mini  MQTT example
  This sketch demonstrates the capabilities of the pubsub library in combination
  with the ESP8266 board/library.
  It connects to the provided access point using dhcp, using ssid and pswd
  It connects to an MQTT server ( using mqtt_server ) then:
  - publishes "connected"+uniqueID to the [root topic] ( using topic )
  - subscribes to the topic "[root topic]/composeClientID()/in"  with a callback to handle
  - If the first character of the topic "[root topic]/composeClientID()/in" is an 1,
    switch ON the ESP Led, else switch it off
  - after a delay of "[root topic]/composeClientID()/in" minimum, it will publish
    a composed payload to
  It will reconnect to the server if the connection is lost using a blocking
  reconnect function.

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.

const char* ssid = "tac02";
const char* pswd = "tacmesh*!";
const char* mqtt_server = "172.16.0.154";
const char* topic = "wemos";    // rhis is the [root topic]
const char* batterytopic = "tacmesh/battery2";

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
int relaypin = 14;
int offset = -10  ; // set the correction offset value
int count;
bool relayon = HIGH;
bool relayoff = LOW;
int minvolt = 0; //in milivolt
int maxvolt = 2000; //in milivolt

int status = WL_IDLE_STATUS;     // the starting Wifi radio's status

IPAddress local_IP(172, 16, 0, 42);
// Set your Gateway IP address
IPAddress gateway(172, 16, 0, 254);
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
      Serial.println("Mode: Always ON");
      operationmode = true;
    }
    else if (message == "0") {
      Serial.println("Mode: Periodic every " + String(offperiod) + "minute");
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
      schedulemode = false;
    }
  }
  if (String(topic) == "tacmesh/offperiod") {
    offperiod = message;
    offperiodmicros = stringToLong(offperiod) * 1000000;
    Serial.println(offperiodmicros);
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
      // Once connected, publish an announcement...
      client.publish(topic, ("connected " + composeClientID()).c_str() , true );
      // ... and resubscribe
      // topic + clientID + in
      //      String subscription;
      //      subscription += topic;
      //      subscription += "/";
      //      subscription += composeClientID() ;
      //      subscription += "/in";
      //      client.subscribe(subscription.c_str() );
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

void setup() {
  Serial.begin(115200);

  pinMode(relaypin, OUTPUT);     // Initialize the Relay Pin as an output
  delay(10);
  digitalWrite(relaypin, relayon);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // confirm still connected to mqtt server
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

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
  String payload = "{\"Voltage\":";
  payload += String(voltage);
  payload += "}";

  if (updateonce == false) {
    Serial.println("I'm Awake  !!");
    delay(10);
    client.publish(batterytopic , payload.c_str());
    Serial.println("Sent Update to Node Red : {Voltage:" + String(voltage) + "V}");
    updateonce = true;
    lastMsg = millis();
  }
  //Serial.println(operationmode);
  unsigned long now = millis();
  if (operationmode == false && schedulemode == false) {
    if (now - lastMsg >= 5000) {
      Serial.println("I'm awake, but I'm going into deep sleep mode for " + String(offperiodmicros / 1000000) + "seconds");
      Serial.println(offperiodmicros);
      ESP.deepSleep(offperiodmicros);
      //Serial.println(operationmode);
    }
  }
  else if (operationmode == true || schedulemode == true) {
    if (now - lastMsg >= 5000) {
      client.publish(batterytopic , payload.c_str());
      Serial.println("Sent Update to Node Red : {Voltage:" + String(voltage) + "V}");
      lastMsg = millis();
      //Serial.println(operationmode);
    }
  }
  delay(100);
}
