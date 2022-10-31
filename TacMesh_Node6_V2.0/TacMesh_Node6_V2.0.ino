#include <ArduinoJson.h>
#include "netconfig.h"
#include "mqtt_setup.h"
//#include "eeprom_putget.h"
#include <TimeLib.h>
#include "RTClib.h"

RTC_DS1307 rtc;

//************************************** Change the settings on the configuration file, do not change here *******************************//
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

//**************************************************************************************************************************************//

boolean connecttoap = false; // status flag for turning on radio
unsigned long timediff; // time difference flag for sleep function
unsigned long lastmillis;
int checkinterval = 5000;
int updateonce = 0;
int voltavg;
int count;
String batteryvolt;

void setup() {
  Serial.begin(115200);
  Serial.println(" ");
  rtc.begin(); // start communicate with rtc
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.println("Retrying to reach rtc");
    while (1) {
      delay(1000);
      Serial.print(".");
      if (rtc.begin()) {
        Serial.println("\nRTC Established");
        break;
      }
    }
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // rtc.adjust(DateTime(2022, 9, 25, 17, 16, 0)); // year,month,day,hour,minute,second
  }

  EEPROM.begin(512);
  //  EEPROM.write(wakestatus_address, true);
  if (default_starttime_address != default_starttime) {
    EEPROM.put(default_starttime_address, default_starttime);
    EEPROM.commit();
  }
  if (default_stoptime_address != default_stoptime) {
    EEPROM.put(default_stoptime_address, default_stoptime);
    EEPROM.commit();
  }
  delay(200);
  Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
  if (EEPROM[wakestatus_address] == 1) {
    Serial.println("This is a wake from sleep");
    EEPROM.write(wakestatus_address, false);
    EEPROM.commit();

    EEPROM.get(operationmode_address, operationmode);
    if (operationmode == 2) {
      Serial.println("Last mode was in standby");
      connecttoap = true;
      Serial.println("Connec to AP = True");
    }
    else {
      boolean onradio = compareTime(arraysize_address, starttime_address, stoptime_address, timediff);

      if (onradio) {
        connecttoap = true;
        pinMode(relaypin, OUTPUT);
        digitalWrite(relaypin, relayon);
        Serial.println("Time to turn on Radio");
      }
      else {
        connecttoap = false;
        Serial.println("Its not time to connect to radio");
        delay(1000);
        Serial.println(timediff);
        EEPROM.write(wakestatus_address, true);
        EEPROM.commit();
        espSleep();
        //Serial.println(compareTime(arraysize_address, starttime_address, stoptime_address, timediff));
      }
    }
  }
  else {
    connecttoap = true;
    Serial.println("Connect to AP is True");
  }

  if (connecttoap) {
    pinMode(relaypin, OUTPUT);
    digitalWrite(relaypin, relayon);
    delay(1000);
    setup_wifi();
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    delay(1000);
  }
  lastmillis = millis();
}

void loop() {
  if (connecttoap) {
    ArduinoOTA.handle();

    if (!client.connected()) {
      reconnect();
    }

    else if (client.connected()) {
      client.loop();
      if (updateonce == 0) {
        batteryvolt = "{\"Voltage\":";
        batteryvolt += String(measureVolt());
        batteryvolt += ",\"nodeName\":";
        batteryvolt += "\"" + String(HostName) + "\"";
        batteryvolt += "}";
        emitMQTT(batterytopic, batteryvolt);
        String payloads = "{\"payload\"";
        payloads += ":\"query\"}";
        emitMQTT("tacmesh/nodes", payloads);
        delay(300);
        updateonce++;
      }
      if (updateonce == 1)
      {
        if (callbackflag) {
          updateTime();
          Serial.println("Call back flag is : " + String(callbackflag));
          updateonce++;
          delay(150);
        }
        else {
          Serial.println("Call back flag is : " + String(callbackflag));
          delay(1000);
        }
      }
    }
  }

  else {
    Serial.println("connecto to AP False");
  }
  //  Serial.println("Hello World");
  //  delay(2000);

  // if mode is in scheduled
  EEPROM.get(operationmode_address, operationmode);
  if (millis() - lastmillis >= checkinterval && updateonce == 2) {
    if (operationmode == 3) {
      Serial.println("Mode : Scheduled");
      //      Serial.println(millis() - lastmillis);
      boolean onradio = compareTime(arraysize_address, starttime_address, stoptime_address, timediff);
      if (onradio) {
        Serial.println("Still on Time");
      }
      else {
        emitMQTT(batterytopic, batteryvolt);
        delay(200);
        EEPROM.write(wakestatus_address, true);
        EEPROM.commit();
        espSleep();
      }
    }
    else if (operationmode == 2) {
      sleepinterval = sleepinterval * 60 * 1e6;
      Serial.println("Mode in Standby");
      Serial.println("Controller will go to sleep for " + String(sleepinterval) + "seconds");
      EEPROM.write(wakestatus_address, true);
      EEPROM.commit();
      delay(200);
      ESP.deepSleep(sleepinterval);
    }
    // if mode is in operation
    else if (operationmode == 1) {
      batteryvolt = "{\"Voltage\":";
      batteryvolt += String(measureVolt());
      batteryvolt += ",\"nodeName\":";
      batteryvolt += "\"" + String(HostName) + "\"";
      batteryvolt += "}";
      emitMQTT(batterytopic, batteryvolt);
      Serial.println("Mode in Operation");
    }
    lastmillis = millis();
  }
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

void espSleep() {
  if (timediff >= 4291) {
    delay(200);
    Serial.println("Going into Deep Sleep Max");
    //    ESP.deepSleep(ESP.deepSleepMax());
    //    ESP.deepSleep(4290e6);
    timediff = 4290 * 1e6;
    ESP.deepSleep(timediff);
    delay(200);
  }
  else if (timediff < 4290) {
    delay(200);
    Serial.println("Going into Sleep for : " + String(timediff) + " seconds" );
    timediff = timediff * 1e6;
    ESP.deepSleep(timediff);
    //ESP.deepSleep(3900e6);
    //delay(200);
  }
}
