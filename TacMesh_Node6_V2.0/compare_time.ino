//#include "eeprom_putget.h"
String a = "starttimearray";
String b = "stoptimearray";

void readingTimestamp(byte &arraysize, byte &starttime_address, byte &stoptime_address) {
  starttimearray[arraysize];
  stoptimearray[arraysize];

  int datasize = sizeof(long);
  eepromGet(starttime_address, arraysize, datasize, a);
  eepromGet(stoptime_address, arraysize, datasize, b);
}

void updateTime(int &update_year,
                int &update_month,
                int &update_day,
                int &update_hour,
                int &update_minutes,
                int &update_seconds) {
  DateTime updateRTC = updatetime + 28800;
  update_year = updateRTC.year();
  update_month = updateRTC.month();
  update_day = updateRTC.day();
  update_hour = updateRTC.hour();       // get hour from rtc
  update_minutes = updateRTC.minute();  // get minutes from rtc
  update_seconds = updateRTC.second();  // get seconds from rtc
  Serial.println(String(update_year) + "/" + String(update_month) + "/" + String(update_day) + "  " + String(updateRTC.hour()) + ":" + String(updateRTC.minute()) + ":" + String(updateRTC.second()));
  Serial.println(updatetime);
}

bool compareTime(byte &arraysize_address, byte &starttime_address, byte &stoptime_address, unsigned long &timediff) {
  bool scheduleontime = false;
  byte arraysize;
  //  Serial.println("Does it enter here?");

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
    return false;
  }

  // get starttime array stored in eeprom
  EEPROM.get(arraysize_address, arraysize);
  // Serial.println("This is the arraysize : " + String(arraysize));

  // get the value of starttime stored in EEPROM based on arraysize, and store to starttimearray
  readingTimestamp(arraysize, default_starttime_address, default_stoptime_address);
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

    if (timenowinsecs >= turnontimeinsecs && timenowinsecs < turnofftimeinsecs) {
      //  if (timenowinsecs >= turnontimeinsecs) {
      Serial.println("Its time to on");
      Serial.println(String(timenow.hour()) + ":" + String(timenow.minute()) + ":" + String(timenow.second()));
      Serial.println(String(turnontime.hour()) + ":" + String(turnontime.minute()) + ":" + String(turnontime.second()));
      Serial.println(String(turnofftime.hour()) + ":" + String(turnofftime.minute()) + ":" + String(turnofftime.second()));
      // Serial.println(String(timenowinsecs) + " Masa Sekarang");
      // Serial.println(String(turnontimeinsecs) + " Masa ON");
      // Serial.println(String(turnofftimeinsecs) + " Masa ON");
      scheduleontime = true;
      break;

    } else if (timenowinsecs < turnontimeinsecs) {
      Serial.println("Waiting for trigger");
      // Serial.println(String(timenowinsecs) + " Masa Sekarang");
      // Serial.println(String(turnontimeinsecs) + " Masa ON");
      Serial.println(turnontimeinsecs - timenowinsecs);
      // Serial.println(n);
      Serial.println(String(turnontime.hour()) + ":" + String(turnontime.minute()) + ":" + String(turnontime.second()));
      timediff = -1 * (timenowinsecs - turnontimeinsecs);
      scheduleontime = false;
      break;

    } else if (timenowinsecs > turnofftimeinsecs) {
      Serial.println("Time has passed schedule time on index : " + String(n));
      // Serial.println(n);
      // Serial.println(String(timenowinsecs) + " Masa Sekarang");
      // Serial.println(String(turnontimeinsecs) + " Masa ON");
      Serial.println(String(turnofftime.hour()) + ":" + String(turnofftime.minute()) + ":" + String(turnofftime.second()));
      // int arraysize = (sizeof(starttimearray) / sizeof(unsigned long)) - 1; // total byte of values stored in the array will be divided by the type of the value in the array, in this case its 4 bytes (for unsgined long), -1 because array would start with index [0].
      if (n < arraysize) {
        n++;
      } else if (n >= arraysize) {
        timediff = 999999;
        n = 0;
        scheduleontime = false;
        break;
      }
    }
  }
  return scheduleontime;
}
