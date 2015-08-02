#include <ESP8266WiFi.h>
#include "mdns.h"


// When an mDNS packet gets parsed this callback gets called.
void packetCallback(const mdns::MDns* packet){
  packet->Display();
  packet->DisplayRawPacket();
}

// When an mDNS packet gets parsed this callback gets called once per Query.
// See mdns.h for definition of mdns::Query.
void queryCallback(const mdns::Query* query){
  query->Display();
}

// When an mDNS packet gets parsed this callback gets called once per Query.
// See mdns.h for definition of mdns::Answer.
void answerCallback(const mdns::Answer* answer){
  answer->Display();
}

// Initialise MDns. If you don't want the optioanl callbacks, just provide a NULL pointer.
mdns::MDns my_mdns(packetCallback, queryCallback, answerCallback);

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  // setting up Station AP
  WiFi.begin("your_wifi_ssid", "your_wifi_password");

  // Wait for connect to AP
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
}

void loop() {
  my_mdns.Check();
}

