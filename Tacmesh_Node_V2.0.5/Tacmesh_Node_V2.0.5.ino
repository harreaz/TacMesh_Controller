//File:        TacMesh_Node_V2.0.6.ino
//Description: -
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
//              0003 - (16.nov.22) - added struct to variables in netconfig.h
//              0004 - (16.nov.22) - line cleanup in compare_time.ino and mqtt_setup.h
//              0005 - (15.dec.22) - replaced if else statement for operation mode with switch case
//              0006 - (15.dec.22) - added MQTT modeTopic "tacmesh/mode" and nodeSettingsTopic "tacmesh/nodesettings" in netconfig.h
//              0007 - (15.dec.22) - added MQTT rootTopic "tacmesh/logs" for node logging
//              0008 - (15.dec.22) - added MQTT queryTopic "tacmesh/query" for node to query settings
//              0009 - (15.dec.22) - re emit MQTT query to server for settings when timeout reached 5 seconds
//              0010 - (19.dec.22) - comment out default_starttime and default_stoptime
//              0011 - (20.dec.22) - created String jsonPayload(String payload, String Hostname), that converts the passed arguments to json format
//              0012 - (20.dec.22) - node will publish to mqtt on logs topic when connected and going to sleep
//              0013 - (21.dec.22) - node will publish to mqtt on logs topic when received an mqtt callback update from server
//              0014 - (26.jan.23) - changed in compare_time.ino, instead of using while loop replace with for statement to compare time with current index in an array
//             ----------------------------------------------------------------------------------------------
//
//Author          : Mohd Danial Hariz Bin Norazam (393)
//Supervisor      : Ahmad Bahari Nizam B. Abu Bakar.
//Current Version : Version 2.0.6
//
//-------------------------------------------------------------------------------------------------------------
//                                          History Version
//-------------------------------------------------------------------------------------------------------------
//Version - 2.0.1 = (0001,0002)
//Version - 2.0.2 = (0003,0004)
//Version - 2.0.3 = (0005,0006,0007,0008,0009)
//Version - 2.0.4 = (0010)
//Version - 2.0.5 = (0011,0012,0013)
//Version - 2.0.6 = (0014)

#include <ArduinoJson.h>
#include "netconfig.h"
#include "mqtt_setup.h"
#include <TimeLib.h>
#include "RTClib.h"

RTC_DS1307 rtc;

#define OPERATION 1
#define STANDBY 2
#define SCHEDULED 3

//************************************** Change the settings on the configuration file, do not change here *******************************//
const char* batterytopic = settings.batteryTopic;  // for nodes to publish battery voltage topic
const char* roottopic = settings.rootTopic;
const char* mqtt_server = settings.mqttServer;  // the address for MQTT Server
int mqtt_port = settings.mqttPort;              // port for MQTT Server
int relaypin = settings.relayPin;              // new board uses pin D5 (gpio14), old board uses D4 (gpio2)
int offset = settings.offSet;                  // set the correction offset value for voltage reading
int minvolt = settings.minVolt;                // MinVoltage x 100
int maxvolt = settings.maxVolt;                // MaxVoltage x 100
int updateinterval = settings.updateInterval;  // Interval to update battery percentage to MQTT server
bool relayon = settings.relayOn;    // Set LOW if relay is active low, Set HIGH if relay is active HIGH
bool relayoff = settings.relayOff;  // Opposite of relayon
const char* hostname =  settings.hostName;  // define hostname as node number
//****************************************************************************************************************************************//

boolean connecttoap = false;  // status flag for turning on radio
unsigned long timediff;       // time difference flag for sleep function
unsigned long lastmillis;
int checkinterval = 5000;
int updateonce = 0;
int voltavg;
int count;
int callbacktimeout;
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
  //  unsigned long tempVal;
  //  EEPROM.get(default_starttime_address, tempVal);
  //  if (tempVal != default_starttime) {
  //    Serial.println("Updated Default Start Time");
  //    EEPROM.put(default_starttime_address, default_starttime);
  //    EEPROM.commit();
  //  }
  //  delay(100);
  //  EEPROM.get(default_stoptime_address, tempVal);
  //  if (tempVal != default_stoptime) {
  //    Serial.println("Updated Default Stop Time");
  //    EEPROM.put(default_stoptime_address, default_stoptime);
  //    EEPROM.commit();
  //  }
  //  delay(100);

  // check if this is a wake from sleep, or wake by reset
  Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
  if (EEPROM[wakestatus_address] == 1) {
    Serial.println("This is a wake from sleep");
    EEPROM.write(wakestatus_address, false); //set wakestats_address to false
    EEPROM.commit();

    // check for operation mode
    EEPROM.get(operationmode_address, operationmode); // get the last operation mode stored in operationmode_address
    if (operationmode == STANDBY) { // operationmode == 2 is in standby mode
      Serial.println("Last mode was in standby");
      connecttoap = true;
      Serial.println("Connec to AP = True");
    }

    else {
      // call function to compare the time now to the scheduled time, value will return true or false
      boolean onradio = compareTime(arraysize_address, starttime_address, stoptime_address, timediff);
      if (onradio) {
        connecttoap = true;
        Serial.println("Time to turn on Radio");
      }
      else {
        connecttoap = false;
        Serial.println("Its not time to connect to radio");
        delay(1000);
        EEPROM.write(wakestatus_address, true);
        EEPROM.commit();
        delay(200);
        //        Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
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

  if (WiFi.status() == WL_CONNECTED) { // if wifi is connected, start handling ArduinoOTA
    ArduinoOTA.handle();

    if (!client.connected()) { // if not connected to mqtt server
      reconnect(); // this function calls the mqtt reconnect to server when disconnected
    }

    else if (client.connected()) { // if connected to mqtt server
      client.loop(); // this function handles the connection to mqtt server
      if (updateonce == 0) { // send an update on battery voltage and query for latest settings only once
        // construct json format for sending voltage and node name
        batteryvolt = jsonPayload(String (measureVolt()), String (hostname), "battery"); // get latest battery reading. Arguments (payload,hostname,type of message)
        emitMQTT(batterytopic, batteryvolt); // send mqtt message to battery topic with payload voltage and node name

        String querymessage = jsonPayload("query", String (hostname)); // format payload to json format. Arguments (payload, hostname,type of message)
        emitMQTT(settings.queryTopic, querymessage); // query latest update from server

        delay(300);
        updateonce++; // increment updateonce value by 1
      }
      if (updateonce == 1) {
        if (callbackflag) { // callbackflag is triggered at mqtt_setup.h after receiving query updates from server
          Serial.println("Updating RTC Time");
          updateTime(update_year, update_month, update_day, update_hour, update_minutes, update_seconds); // calculate latest unix timestamp receive from server to year,month,day,hour,minutes,seconds
          rtc.adjust(DateTime(update_year, update_month, update_day, update_hour, update_minutes, update_seconds)); // update the time on rtc with the calculated time received from server
          Serial.println("Call back flag is : " + String(callbackflag));
          updateonce++;
          delay(150);
        } else {
          Serial.println("Call back flag is : " + String(callbackflag));
          callbacktimeout++;
          delay(1000);
          if (callbacktimeout > 4) {
            String querymessage = jsonPayload("query", String (hostname));
            emitMQTT(settings.queryTopic, querymessage); // query latest update from server
            callbacktimeout = 0;
          }
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

    switch (operationmode) {
      case SCHEDULED: {
          Serial.println("Mode : Scheduled");
          //      Serial.println(millis() - lastmillis);
          boolean onradio = compareTime(arraysize_address, starttime_address, stoptime_address, timediff);
          emitMQTT(batterytopic, batteryvolt);
          if (onradio) {
            Serial.println("Still on Time");
          } else {
            Serial.println("All time schedule had passed, time to go to Sleep");
            EEPROM.write(wakestatus_address, true);
            EEPROM.commit();
            String logs = jsonPayload("Going to Sleep", String(settings.hostName));
            emitMQTT(roottopic, logs);
            delay(200);
            //            Serial.println("WAKE ADDRESS IS= " + String(EEPROM[wakestatus_address]));
            espSleep();
          }
        }
        break;

      case STANDBY: {
          sleepinterval = sleepinterval * 60 * 1e6;
          Serial.println("Mode in Standby");
          Serial.println("Controller will go to sleep for " + String(sleepinterval) + "seconds");
          EEPROM.write(wakestatus_address, true);
          EEPROM.commit();
          String logs = jsonPayload("Going to Sleep", String(settings.hostName));
          emitMQTT(roottopic, logs);
          delay(200);
          ESP.deepSleep(sleepinterval);
        }
        break;

      case OPERATION: {
          batteryvolt = jsonPayload(String (measureVolt()), String (hostname), "battery"); // get latest battery reading. Arguments (payload,hostname,type of message)
          emitMQTT(batterytopic, batteryvolt); // send mqtt message to battery topic with payload voltage and node name
          Serial.println("Mode in Operation");
        }
        break;
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
    //    ESP.deepSleep(ESP.deepSleepMax()); // deepsleepmax() function is not stable, esp does not wake up sometimes
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
