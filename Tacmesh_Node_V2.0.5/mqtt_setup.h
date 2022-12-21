#include "wifi_setup.h"
#include "eeprom_get.h"
#include <PubSubClient.h>

PubSubClient client(espClient); // instantiate PubSubClient object

String message;
unsigned long starttime;
unsigned long stoptime;
unsigned long updatetime;
//unsigned long default_starttime = 1667016000;
//unsigned long default_stoptime = 1667016300;
byte operationmode; // 1 for operation mode, 2 for standby mode, 3 for scheduled mode
int sleepinterval;
boolean callbackflag = false;

//byte default_starttime_address = 0; // initialize a default start time value at 12.00pm
//byte default_stoptime_address = 50; // initialize a default stop time value at 12.05pm
//byte starttime_address = 4; // initial address for starttime array, every value stored will take up 4 bytes.
//byte stoptime_address = 54; // initial address for stoptime array, every value stored will take up 4 bytes.
byte starttime_address = 0; // initial address for starttime array, every value stored will take up 4 bytes.
byte stoptime_address = 50; // initial address for stoptime array, every value stored will take up 4 bytes.
byte wakestatus_address = 100; // address for wake from sleep status
byte arraysize_address = 105; // address for size of array
byte operationmode_address = 110; // address to store operation mode

String jsonPayload (String payload, String hostName, String topic = "else") {
  String Message;
  if (topic == "battery") {
    Message = "{\"Voltage\":";
    Message +=  "\"" + payload + "\"";
    Message += ",\"nodeName\":";
    Message += "\"" + hostName + "\"";
    Message += "}";
  } else {
    Message = "{\"payload\":";
    Message +=  "\"" + payload + "\"";
    Message += ",\"nodeName\":";
    Message += "\"" + hostName + "\"";
    Message += "}";
  }
  return Message;
}

void emitMQTT(const char* topics, String payload) {
  Serial.print("Emitting to " + String(topics) + " : ");
  Serial.println(payload);
  client.publish(topics, payload.c_str());
}

// callbak function handles the receiving of new mqtt messages, based on the subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("");
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] , Content = ");

  for (int i = 0; i < length; i++) {
    //Serial.print((char)payload[i]);
    message += (char)payload[i];
  }
  //  Serial.println(message);
  //  Serial.println(String(topic));
  Serial.println(message);

  // if the topic is for operation mode
  if (String(topic) == settings.modeTopic) {
    EEPROM.get(operationmode_address, operationmode); // get the latest operation mode stored in EEPROM
    if (operationmode != message.toInt()) { // if latest mode in EEPROM is not the same as current, update to EEPROM
      EEPROM.write(operationmode_address, message.toInt());
      EEPROM.commit();
    }
  }

  // if the topic is for node settings
  if (String(topic) == settings.nodeSettingsTopic) {
    // setup json format
    StaticJsonDocument<384> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    sleepinterval = doc["sleepinterval"]; // sleep interval during standby mode
    updatetime = doc["timestamp"]; // the received timestamp
    int arraysize = doc["arraysize"]; //the arraysize of the payload
    byte arraylength;
    //    Serial.println(sleepinterval);
    //    Serial.println(updatetime);

    EEPROM.get(arraysize_address, arraylength);
    if (arraysize != arraylength) {
      EEPROM.put(arraysize_address, arraysize);
      EEPROM.commit();
      Serial.println("Arraysize updated to : " + String(arraysize));
    }

    int datasize = sizeof(long);

    JsonArray starttimearray = doc["starttime"];
    for (int i = starttime_address; i < (arraysize * datasize) + starttime_address; i = i + datasize) {
      starttime = starttimearray[(i - starttime_address) / datasize];
      Serial.println("The value of Starttime " + String(starttime) + " is stored in index " + String(i));
      unsigned long tempvalue;
      EEPROM.get(i, tempvalue);
      if (starttime == tempvalue) {
        Serial.println("No Update since the starttime for index " + String(i) + " value is the same");
      }
      else {
        EEPROM.put(i, starttime);
        EEPROM.commit();
        Serial.println("Starttime for index " + String(i) + " updated to : " + String(starttime));
      }
      Serial.println("");
    }

    Serial.println("");
    JsonArray stoptimearray = doc["stoptime"];
    for (int i = stoptime_address; i < (arraysize * datasize) + stoptime_address; i = i + datasize) {
      stoptime = stoptimearray[(i - stoptime_address) / datasize]; // 1666624021
      Serial.println("The value of Stoptime " + String(stoptime) + " is stored in index " + String(i));
      unsigned long tempvalue;
      EEPROM.get(i, tempvalue);
      if (stoptime == tempvalue) {
        Serial.println("No Update since the stoptime for index " + String(i) + " value is the same");
      }
      else {
        EEPROM.put(i, stoptime);
        EEPROM.commit();
        Serial.println("Stoptime for index " + String(i) + " updated to : " + String(stoptime));
      }
      Serial.println("");
    }
  }
  String logs = jsonPayload("Received Settings from Server", String(settings.hostName));
  emitMQTT(settings.rootTopic, logs); 
  message = "";
  callbackflag = true;
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
      String logs = jsonPayload("Connected", String(settings.hostName));
      client.publish(settings.rootTopic, (logs).c_str(), true);
      String subscription1 = settings.nodeSettingsTopic;
      String subscription2 = settings.modeTopic;
      client.subscribe(subscription1.c_str());
      client.subscribe(subscription2.c_str());
      Serial.print("subscribed to : ");
      Serial.println(subscription1);
      Serial.print(" and ");
      Serial.println(subscription2);
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
