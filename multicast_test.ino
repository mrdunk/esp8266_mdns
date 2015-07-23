/*
 * This sketch will display data mDNS (multicast DNS) data seen on the network.
 */

#include <ESP8266WiFi.h>

#include "mdns.h"

#include "secrets.h"  // Contains the following:
// const char* ssid = "Get off my wlan";  //  your network SSID (name)
// const char* pass = "secretwlanpass";       // your network password

void queryCallback(mdns::Query query){
  query.Display();
}

void answerCallback(mdns::Answer answer){
  answer.Display();
}

//mdns::MDns my_mdns(queryCallback, answerCallback);
mdns::MDns my_mdns;

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // setting up Station AP
  WiFi.begin(ssid, pass);

  // Wait for connect to AP
  Serial.print("[Connecting]");
  Serial.print(ssid);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tries++;
    if (tries > 30) {
      break;
    }
  }
  Serial.println();


  printWifiStatus();

  Serial.println("Connected to wifi");
  
}



void loop()
{
  my_mdns.Check();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
}
