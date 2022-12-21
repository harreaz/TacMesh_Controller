#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>


#define HOSTNAME settings.hostName
const char* ssid = settings.ssID;                     // change which SSID of tacmesh radio controller would connect
const char* pswd = settings.passWord;                 // password of the SSID

// Below parameters are to set for static ip, for DHCP just comment out the 5 lines
//IPAddress local_IP(ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3]); // Set controller ip address according to node hostname
//IPAddress gateway(gateWay[0], gateWay[1], gateWay[2], gateWay[3]);  // Set your Gateway IP address
//IPAddress subnet(subNet[0], subNet[1], subNet[2], subNet[3]); // class c
//IPAddress primaryDNS(DNS[0], DNS[1], DNS[2], DNS[3]);    //optional
//IPAddress secondaryDNS(altDNS[0], altDNS[1], altDNS[2], altDNS[3]);  //optional

WiFiClient espClient; // instantiate WiFiClient object
int status = WL_IDLE_STATUS;  // the starting Wifi radio's status

// setup wifi function handles the wifi connection
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  //  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
  //    Serial.println("STA Failed to configure");
  //  }
  Serial.print("Connecting to ");
  Serial.println(ssid); //ssid of wifi to connect
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pswd); // wemos will try to connect to the given ssid and password
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  ArduinoOTA.setHostname(HOSTNAME); // set over the air hostname

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
