#include <EEPROM.h>

unsigned long starttimearray[20];
unsigned long stoptimearray[20];

void eepromGet(int startaddress, int arraysize, int datasize, String storearray) {
  // for lets say i = 0 , i < (3-1) * 4 + 0 or i < 8, i + 4
  for (int i = startaddress; i < arraysize * datasize + startaddress; i = i + datasize) {
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
