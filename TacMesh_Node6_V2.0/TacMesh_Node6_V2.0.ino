//File:        TacMesh_Node6_V2.0.ino
//Description: Program for handling asterisk call termination from remote site. After each bridging and call
//             dispatching, operator can request to terminate current connected asterisk call remotely.
//             Remote communication request are done via RestAPI.
//             ----------------------------------------------------------------------------------------------
//Notes      : Major, Minor and Revision notes:
//             ----------------------------------------------------------------------------------------------
//             Major    - Software major number will counting up each time there is a major changes on the
//                        software features. Minor number will reset to '0' and revision number will reset
//                        to '1' (on each major changes). Initially major number will be set to '1'
//             Minor    - Software minor number will counting up each time there is a minor changes on the
//                        software. Revision number will reset to '1' (on each minor changes).
//             Revision - Software revision number will counting up each time there is a bug fixing on the
//                        on the current major and minor number.
//             ----------------------------------------------------------------------------------------------
//             Current Features & Bug Fixing Information
//             ----------------------------------------------------------------------------------------------
//              0001 - (9.nov.22) - added condition to monitor the changes on default start time / stop time
//                                  on eeprom address.
//              0002 - (9.nov.22) - added condition to update RTC 1307 date & time when controller receives
//                                  time update from server
//             ----------------------------------------------------------------------------------------------
//
//Author          : Mohd Danial Hariz Bin Norazam (393)
//Supervisor      : Ahmad Bahari Nizam B. Abu Bakar.
//Current Version : Version 2.0.1
//
//-------------------------------------------------------------------------------------------------------------
//                                          History Version
//-------------------------------------------------------------------------------------------------------------
//Version - 2.0.1 = (0001,0002)

#include <ArduinoJson.h>
#include "netconfig.h"
#include "mqtt_setup.h"
#include <TimeLib.h>
#include "RTClib.h"

RTC_DS1307 rtc;

//************************************** Change the settings on the configuration file, do not change here *******************************//
const char* batterytopic = batteryTopic;  // battery topic number based on the node number
const char* mqtt_server = mqttServer;  // the address for MQTT Server
int mqtt_port = mqttPort;              // port for MQTT Server
int relaypin = relayPin;              // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
int offset = offSet;                  // set the correction offset value for voltage reading
int minvolt = minVolt;                // MinVoltage x 100
int maxvolt = maxVolt;                // MaxVoltage x 100
int updateinterval = updateInterval;  // Interval to update battery percentage to MQTT server
bool relayon = relayOn;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
bool relayoff = relayOff;  // Opposite of relayon
//****************************************************************************************************************************************//

boolean connecttoap = false;  // status flag for turning on radio
unsigned long timediff;       // time difference flag for sleep function
unsigned long lastmillis;
int checkinterval = 5000;
int updateonce = 0;
int voltavg;
int count;
int update_year;
int update_month;
int update_day;
int update_hour;
int update_minutes;
int update_seconds;
String batteryvolt;

void setup() {
  Serial.begin(115200);
  Serial.println(" ");
  rtc.begin();  // start communicate with rtc
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //wait for rtc to start
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
  // if this is the first time rtc is on, configure by adjusting the date time
  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // rtc.adjust(DateTime(2022, 9, 25, 17, 16, 0)); // year,month,day,hour,minute,second
  }

  EEPROM.begin(512); // start EEPROM

  //Register Default Starttime on default starttime address, if the value maintains the same, it would not be updated
  unsigned long tempVal;
  EEPROM.get(default_starttime_address, tempVal);
  if (tempVal != default_starttime) {
    Serial.println("Updated Default Start Time");
    EEPROM.put(default_starttime_address, default_starttime);
    EEPROM.commit();
  }
  delay(100);
  EEPROM.get(default_stoptime_address, tempVal);
  if (tempVal != default_stoptime) {
    Serial.println("Updated Default Stop Time");
    EEPROM.put(default_stoptime_address, default_stoptime);
    EEPROM.commit();
  }
  delay(100);

  // check if this is a wake from sleep, or wake by reset
  Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
  if (EEPROM[wakestatus_address] == 1) {
    Serial.println("This is a wake from sleep");
    EEPROM.write(wakestatus_address, false); //set wakestats_address to false
    EEPROM.commit();

    // check for operation mode
    EEPROM.get(operationmode_address, operationmode); // get the last operation mode stored in operationmode_address
    if (operationmode == 2) { // operationmode == 2 is in standby mode 
      Serial.println("Last mode was in standby");
      connecttoap = true; 
      Serial.println("Connec to AP = True");
    } else {
      // call function to compare the time now to the scheduled time, value will return true or false
      boolean onradio = compareTime(arraysize_address, starttime_address, stoptime_address, timediff);
      if (onradio) {
        connecttoap = true;
        Serial.println("Time to turn on Radio");
      } else {
        connecttoap = false;
        Serial.println("Its not time to connect to radio");
        delay(1000);
        Serial.println(timediff);
        EEPROM.write(wakestatus_address, true);
        EEPROM.commit();
        delay(200);
        Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
        espSleep(); // to deep sleep microcontroller 
      }
    }
  } else {
    connecttoap = true;
    Serial.println("Connect to AP is True");
  }

  if (connecttoap) {
    pinMode(relaypin, OUTPUT);
    digitalWrite(relaypin, relayon); // turn on radio
    setup_wifi(); // start connect to ssid sequence
    client.setServer(mqtt_server, mqtt_port); // start connect to mqtt server and port
    client.setCallback(callback); // start mqtt callback function
    delay(1000);
  }
  lastmillis = millis(); // start counter flag
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
        // construct json format for sending voltage and node name
        batteryvolt = "{\"Voltage\":";
        batteryvolt += String(measureVolt());
        batteryvolt += ",\"nodeName\":";
        batteryvolt += "\"" + String(HostName) + "\"";
        batteryvolt += "}";
        emitMQTT(batterytopic, batteryvolt); // send mqtt message to battery topic with payload voltage and node name
        String payloads = "{\"payload\"";
        payloads += ":\"query\"}";
        emitMQTT("tacmesh/nodes", payloads); // query latest update from server
        delay(300);
        updateonce++;
      }
      if (updateonce == 1) {
        // callbackflag is triggered at mqtt_setup.h after receiving query updates from server
        if (callbackflag) {
          Serial.println("Updating RTC Time");
          updateTime(update_year, update_month, update_day, update_hour, update_minutes, update_seconds); // calculate latest unix timestamp receive from server to year,month,day,hour,minutes,seconds
          rtc.adjust(DateTime(update_year, update_month, update_day, update_hour, update_minutes, update_seconds)); // update the time on rtc with the calculated time received from server
          Serial.println("Call back flag is : " + String(callbackflag));
          updateonce++;
          delay(150);
        } else {
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
      emitMQTT(batterytopic, batteryvolt);
      if (onradio) {
        Serial.println("Still on Time");
      } else {
        EEPROM.write(wakestatus_address, true);
        EEPROM.commit();
        delay(200);
        Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
        espSleep();
      }
    } else if (operationmode == 2) {
      sleepinterval = sleepinterval * 60 * 1e6;
      Serial.println("Mode in Standby");
      Serial.println("Controller will go to sleep for " + String(sleepinterval) + "seconds");
      EEPROM.write(wakestatus_address, true);
      EEPROM.commit();
      delay(200);
      Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
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
  } else if (timediff < 4290) {
    delay(200);
    Serial.println("Going into Sleep for : " + String(timediff) + " seconds");
    timediff = timediff * 1e6;
    ESP.deepSleep(timediff);
    //ESP.deepSleep(3900e6);
    //delay(200);
  }
}
