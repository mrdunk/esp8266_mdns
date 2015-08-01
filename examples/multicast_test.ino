/*
 * This sketch will display  mDNS (multicast DNS) data seen on the network
 * and can be used to send mDNS queries.
 */

#include <ESP8266WiFi.h>
#include "mdns.h"

#include "secrets.h"  // Contains the following:
// const char* ssid = "Get off my wlan";      //  your network SSID (name)
// const char* pass = "secretwlanpass";       // your network password


//#define DEBUG_OUTPUT          // Send packet summaries to Serial.
#define DEBUG_RAW             // Send HEX ans ASCII encoded raw packet to Serial.


// When an mDNS packet gets parsed this callback gets called once per Query.
// See mdns.h for definition of mdns::Query.
void queryCallback(mdns::Query query){
  query.Display();
}

// When an mDNS packet gets parsed this callback gets called once per Query.
// See mdns.h for definition of mdns::Query.
void answerCallback(mdns::Answer answer){
  answer.Display();
}

mdns::MDns my_mdns(queryCallback, answerCallback);
//mdns::MDns my_mdns;

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


  // Query for all host information for a paticular service. ("_mqtt" in this case.)
  my_mdns.Clear();
  struct mdns::Query query_mqtt;
  strncpy(query_mqtt.qname_buffer, "_mqtt._tcp.local", MAX_MDNS_NAME_LEN);
  query_mqtt.qtype = MDNS_TYPE_PTR;
  query_mqtt.qclass = 1;    // "INternet"
  query_mqtt.unicast_response = 0;
  my_mdns.AddQuery(query_mqtt);
  my_mdns.Send();

  /*
  // Query for all service types on network.
  my_mdns.Clear();
  struct mdns::Query query_services;
  strncpy(query_services.qname_buffer, "_services._dns-sd._udp.local", MAX_MDNS_NAME_LEN);
  query_services.qtype = MDNS_TYPE_PTR;
  query_services.qclass = 1;    // "INternet"
  query_services.unicast_response = 0;
  my_mdns.AddQuery(query_services);
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
