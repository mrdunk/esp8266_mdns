/*
 *  This sketch is a simple MDNS responder for the local hostname
 *  It demonstrates how to use unicast vs multicast answers
 *  
 *  When a query packet is received, the queryCallback function is called
 *  If the query is for our hostname, then an answer is populated and a unicast or multicast response is sent
 */

#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

#include <mdns.h>

#include "secrets.h"  // Contains the following:
// const char* ssid = "Get off my wlan";      //  your network SSID (name)
// const char* pass = "secretwlanpass";       // your network password

char hostname[] = "test.local"; // local hostname

void queryCallback(const mdns::Query* query);

bool queryRecv;
IPAddress queryIP;
bool queryUnicast;

// Initialise MDns. We only need the query callback
mdns::MDns my_mdns(NULL, queryCallback, NULL);

// When an mDNS packet gets parsed this callback gets called once per Query.
// This callback flags the query for an answer if the name matches the local hostname
void queryCallback(const mdns::Query* query){
  if((query->qclass==1)&&(query->qtype==MDNS_TYPE_A)) {
    Serial.println("A Query received");
    // received a host type query
    if(0==strcmp(query->qname_buffer,hostname)) {
      // query is for us
      Serial.println("Query matches hostname");
      queryRecv = true;
      queryIP = my_mdns.getRemoteIP();
      queryUnicast = query->unicast_response;
    }
  }
}

void setup() {
  Serial.begin(115200); // open serial port for messages

  WiFi.begin(ssid,pass); // start WiFi connection

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

  my_mdns.begin(); // call to startUdpMulticast
}

void loop() {
  queryRecv = false; queryIP = IPAddress(0,0,0,0); queryUnicast = false;
  my_mdns.loop();
  if(queryRecv) {
    SendQueryAnswer();
  }
}

void SendQueryAnswer() {
  mdns::Answer answer;

  answer.rrtype = MDNS_TYPE_A;
  answer.rrclass = 1; // INternet
  answer.rrttl = 255;
  answer.rrset = false;
  answer.valid = true;
  strcpy(answer.name_buffer,hostname);
  answer.rdata_buffer[0] = WiFi.localIP()[0];
    answer.rdata_buffer[1] = WiFi.localIP()[1];
    answer.rdata_buffer[2] = WiFi.localIP()[2];
    answer.rdata_buffer[3] = WiFi.localIP()[3];
  my_mdns.Clear();
  if(!my_mdns.AddAnswer(answer)) {
    Serial.println("AddAnswer returned false");
  }
  if(queryUnicast) {
    Serial.println("Sending unicast response");
    my_mdns.SendUnicast(my_mdns.getRemoteIP());
  } else {
    Serial.println("Sending multicast response");
    my_mdns.Send();
  }
  queryRecv = false;
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
