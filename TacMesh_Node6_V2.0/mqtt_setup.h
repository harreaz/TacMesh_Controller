#include "wifi_setup.h"
#include "eeprom_get.h"
#include <PubSubClient.h>

PubSubClient client(espClient); // instantiate PubSubClient object

const char* topic = "wemos";  // rhis is the [root topic]
String message;
unsigned long starttime;
unsigned long stoptime;
unsigned long updatetime;
unsigned long default_starttime = 1667016900;
unsigned long default_stoptime = 1667017200;
byte operationmode; // 1 for operation mode, 2 for standby mode, 3 for scheduled mode
int sleepinterval;
boolean callbackflag = false;

byte default_starttime_address = 0; // initialize a default start time value at 12.00pm
byte default_stoptime_address = 50; // initialize a default stop time value at 12.05pm
byte starttime_address = 4; // initial address for starttime array, every value stored will take up 4 bytes.
byte stoptime_address = 54; // initial address for stoptime array, every value stored will take up 4 bytes.
byte wakestatus_address = 100; // address for wake from sleep status
byte arraysize_address = 105; // address for size of array
byte operationmode_address = 110; // address to store operation mode

void emitMQTT(const char* topics, String payload) {
  //  if (topics == "tacmesh/query") {
  //    Serial.println("Query update from Server");
  //  }
  //  else if (topics == "tacmesh/trigger") {
  //    Serial.println("Sending Sleep Trigger to Server");
  //  }
  //  else {
  //    String voltage = String(measureVolt());
  //    Serial.println("Sending battery update to Server : {Voltage:" + String(voltage) + "V}");
  //    Serial.println(" ");
  //  }
  Serial.print("Emitting : ");
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

  if (String(topic) == "tacmesh/mode") {
    EEPROM.get(operationmode_address, operationmode);
    if (operationmode != message.toInt()) {
      EEPROM.write(operationmode_address, message.toInt());
      EEPROM.commit();
    }
  }

  if (String(topic) == "tacmesh/test") {
    // String input;
    StaticJsonDocument<384> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    sleepinterval = doc["sleepinterval"]; // sleep interval during standby mode
    updatetime = doc["timestamp"]; // 1666928669
    int arraysize = doc["arraysize"];
    byte arraylength;
    Serial.println(sleepinterval);
    //    Serial.println(updatetime);

    EEPROM.get(arraysize_address, arraylength);
    if (arraysize == arraylength) {
      Serial.println("No Update since the arraysize value is the same");
    }
    else {
      EEPROM.put(arraysize_address, arraysize);
      EEPROM.commit();
      Serial.println("Arraysize updated to : " + String(arraysize));
    }
    int datasize = sizeof(long);

    JsonArray starttimearray = doc["starttime"];
    for (int i = starttime_address; i < (arraysize * datasize) + starttime_address; i = i + datasize) {
      starttime = starttimearray[(i - starttime_address) / datasize]; // 1666624021
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
      client.publish(topic, ("connected " + composeClientID()).c_str(), true);
      String subscription1 = "tacmesh/test";
      String subscription2 = "tacmesh/mode";
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
