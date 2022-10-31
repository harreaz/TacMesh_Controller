#include <EEPROM.h>

unsigned long starttimearray[20];
unsigned long stoptimearray[20];

//************ EEPROM ADDRESS VARIABLES ************//
//byte starttime_address = 0; // initial address for starttime array, every value stored will take up 4 bytes.
//byte stoptime_address = 50; // initial address for stoptime array, every value stored will take up 4 bytes.
//byte wakestatus_address = 100; // address for wake from sleep status
//byte arraysize_address = 105; // address for size of array

void eepromGet(int startaddress, int arraysize, int datasize, String storearray) {
  for (int i = startaddress; i <= arraysize * datasize + startaddress; i = i + datasize) {
    if (storearray == "starttimearray") {
      EEPROM.get(i, starttimearray[(i - startaddress) / datasize]);
      Serial.println("The value Starttime " + String(starttimearray[(i - startaddress) / datasize]) + " is from index " + String(i));
    }
    else if (storearray == "stoptimearray") {
      EEPROM.get(i, stoptimearray[(i - startaddress) / datasize]);
      Serial.println("The value of Stoptime " + String(stoptimearray[(i - startaddress) / datasize]) + " is from index " + String(i));
    }
  }
  Serial.println("");
}
