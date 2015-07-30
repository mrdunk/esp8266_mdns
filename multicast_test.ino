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


  my_mdns.Clear();

  struct mdns::Query query_mqtt;
  strncpy(query_mqtt.qname_buffer, "_mqtt._tcp.local", MAX_MDNS_NAME_LEN);
  query_mqtt.qtype = 0x0C;  // "PTR"
  query_mqtt.qclass = 1;    // "INternet"
  query_mqtt.unicast_response = 0;
  my_mdns.AddQuery(query_mqtt);

  my_mdns.Send();

  /*my_mdns.Clear();
  
  struct mdns::Query query_services;
  strncpy(query_services.qname_buffer, "_services._dns-sd._udp.local", MAX_MDNS_NAME_LEN);
  query_services.qtype = 0x0C;  // "PTR"
  query_services.qclass = 1;    // "INternet"
  query_services.unicast_response = 0;
  my_mdns.AddQuery(query_services);

  my_mdns.Send();*/

  /*my_mdns.Clear();

  struct mdns::Answer answer_esp;
  strncpy(answer_esp.name_buffer, "esp_test.local", MAX_MDNS_NAME_LEN);
  IPAddress ip = WiFi.localIP();
  answer_esp.rdata_buffer[0] = ip[0];
  answer_esp.rdata_buffer[1] = ip[1];
  answer_esp.rdata_buffer[2] = ip[2];
  answer_esp.rdata_buffer[3] = ip[3];
  answer_esp.rrtype = 0x1; // "A" record. 32-bit IPv4 address.
  answer_esp.rrclass = 1;  // "INternet"
  answer_esp.rrttl = 0xE10; // 1 hour
  answer_esp.rrset = 1;    // Cache flush.
  my_mdns.AddAnswer(answer_esp);
  
  my_mdns.Send();*/
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
