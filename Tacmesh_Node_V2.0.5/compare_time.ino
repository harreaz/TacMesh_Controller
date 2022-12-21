//#include "eeprom_putget.h"
String a = "starttimearray";
String b = "stoptimearray";

void readingTimestamp(byte &arraysize, byte &starttime_address, byte &stoptime_address) { // get arraysize, starttime address, stoptime address
  starttimearray[arraysize]; // declare size of starttime array to the arraysize
  stoptimearray[arraysize]; // declare size of stoptime array to the arraysize

  int datasize = sizeof(long);
  // eepromGet function is in eeprom_get.h
  eepromGet(starttime_address, arraysize, datasize, a); // get value of starttime from eeprom array and store it into starttime array
  eepromGet(stoptime_address, arraysize, datasize, b); // get value of stoptime from eeprom array and store it into stoptime array
}

void updateTime(int &update_year,
                int &update_month,
                int &update_day,
                int &update_hour,
                int &update_minutes,
                int &update_seconds) {
  DateTime updateRTC = updatetime + 28800; // 28800 is a fixed value in seconds for GMT +8 time
  update_year = updateRTC.year();
  update_month = updateRTC.month();
  update_day = updateRTC.day();
  update_hour = updateRTC.hour();       // get hour from rtc
  update_minutes = updateRTC.minute();  // get minutes from rtc
  update_seconds = updateRTC.second();  // get seconds from rtc
  Serial.println(String(update_year) + "/" + String(update_month) + "/" + String(update_day) + "  " + String(updateRTC.hour()) + ":" + String(updateRTC.minute()) + ":" + String(updateRTC.second()));
  //  Serial.println(updatetime);
}

bool compareTime(byte &arraysize_address, byte &starttime_address, byte &stoptime_address, unsigned long &timediff) {
  bool scheduleontime = false;
  byte arraysize;
  Serial.println("");
  Serial.println("Comparing Time");

  DateTime timenow = rtc.now();
  int hour = timenow.hour();                                  // get hour from rtc
  int minutes = timenow.minute();                             // get minutes from rtc
  int seconds = timenow.second();                             // get seconds from rtc
  int minutestosecs = minutes * 60;                           // convert the minutes to seconds
  long hourtosecs = hour * 60 * 60;                           // convert the hours to seconds
  long timenowinsecs = hourtosecs + minutestosecs + seconds;  // seconds of the day
  Serial.println("Time now is : " + String(timenow.hour()) + ":" + String(timenow.minute()) + ":" + String(timenow.second()));

  // read the address, if its 255 meaning no data has ever been written at the address
  if (EEPROM.read(arraysize_address) == 255) {
    Serial.println("Arraysize has not been written yet");
    return false; // return value false, skip time compare
  }

  // get arraysize of starttime and stoptime address stored in eeprom
  EEPROM.get(arraysize_address, arraysize);
  // Serial.println("This is the arraysize : " + String(arraysize));

  // get the value of starttime stored in EEPROM based on arraysize, and store to starttimearray
  //  readingTimestamp(arraysize, default_starttime_address, default_stoptime_address);
  readingTimestamp(arraysize, starttime_address, stoptime_address);
  int n = 0;  // define n as counter

  while (true) {
    // attach a variable to the starttime array starting from the first array
    unsigned long starttime = starttimearray[n] + 28800;  // +28800 is a constant for +8 utc
    unsigned long stoptime = stoptimearray[n] + 28800;    // +28800 is a constant for +8 utc

    DateTime turnontime = starttime;
    hour = turnontime.hour();                                      // get hour from rtc
    minutes = turnontime.minute();                                 // get minutes from rtc
    seconds = turnontime.second();                                 // get seconds from rtc
    minutestosecs = minutes * 60;                                  // convert the minutes to seconds
    hourtosecs = hour * 60 * 60;                                   //convert the hours to seconds
    long turnontimeinsecs = hourtosecs + minutestosecs + seconds;  // seconds of the day

    DateTime turnofftime = stoptime;
    hour = turnofftime.hour();                                      // get hour from rtc
    minutes = turnofftime.minute();                                 // get minutes from rtc
    seconds = turnofftime.second();                                 // get seconds from rtc
    minutestosecs = minutes * 60;                                   // convert the minutes to seconds
    hourtosecs = hour * 60 * 60;                                    //convert the hours to seconds
    long turnofftimeinsecs = hourtosecs + minutestosecs + seconds;  // seconds of the day

    if (timenowinsecs >= turnontimeinsecs && timenowinsecs < turnofftimeinsecs) { // if time now has passed the starttime but has not passed the stoptime on current array index
      Serial.println("Its time to on");
      Serial.print("Time Now = ");
      Serial.println(String(timenow.hour()) + ":" + String(timenow.minute()) + ":" + String(timenow.second()));
      Serial.print("Time to ON = ");
      Serial.println(String(turnontime.hour()) + ":" + String(turnontime.minute()) + ":" + String(turnontime.second()));
      Serial.print("Time to OFF = ");
      Serial.println(String(turnofftime.hour()) + ":" + String(turnofftime.minute()) + ":" + String(turnofftime.second()));
      Serial.println("");
      scheduleontime = true;
      break;

    } else if (timenowinsecs < turnontimeinsecs) { // if time now has not pass the start time on current array index
      Serial.print("Waiting for trigger = ");
      Serial.print(turnontimeinsecs - timenowinsecs);
      Serial.println(" seconds left");
      Serial.print("Time Now = ");
      Serial.println(String(timenow.hour()) + ":" + String(timenow.minute()) + ":" + String(timenow.second()));
      Serial.print("Turn on Time = ");
      Serial.println(String(turnontime.hour()) + ":" + String(turnontime.minute()) + ":" + String(turnontime.second()));
      Serial.println("");
      timediff = -1 * (timenowinsecs - turnontimeinsecs);
      scheduleontime = false;
      break;

    } else if (timenowinsecs > turnofftimeinsecs) { // if time now has passed the stop time on current array index
      Serial.println ("Time has passed schedule time on index : " + String(n));
      Serial.print("Time Now = ");
      Serial.println(String(timenow.hour()) + ":" + String(timenow.minute()) + ":" + String(timenow.second()));
      Serial.print("Time to ON = ");
      Serial.println(String(turnontime.hour()) + ":" + String(turnontime.minute()) + ":" + String(turnontime.second()));
      Serial.print("Time to OFF = ");
      Serial.println(String(turnofftime.hour()) + ":" + String(turnofftime.minute()) + ":" + String(turnofftime.second()));
      Serial.println("");
      if (n < arraysize - 1) { // if the n number is still lower than the array size, then n + 1 and see the next index of the array
        n++;
      } else { // else if the n number is the same or larger than the arraysize, then time now has passed all timers in schedule
        timediff = 999999; // set timediff as maximum
        n = 0; // reset n value
        scheduleontime = false;
        break;
      }
    }
  }
  return scheduleontime; // return boolean
}
