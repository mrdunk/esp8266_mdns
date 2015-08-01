# esp8266_mdns
mDNS queries and responses on esp8266.

This library aims to do the following:
 1. Give access to incoming mDNS packets and decode Question and Answer Records for commonly used record types.
 2. Allow Question and Answer Records for commonly used record types to be sent.

Future goals:
 1. Automatic replies to incoming Questions.
 2. Automatic retries when sending packets according to rfc6762.

Requirements
------------
- An Espressif [ESP8266](http://www.esp8266.com/) WiFi enabled SOC.
- The [ESP8266 Arduino](https://github.com/esp8266/Arduino) environment.
- ESP8266WiFi library.
- MDNS support in your operating system/client machines:
  - For Mac OSX support is built in through Bonjour already.
  - For Linux, install [Avahi](http://avahi.org/).
  - For Windows, install [Bonjour](http://www.apple.com/support/bonjour/).

Usage
-----
To add a simple mDNS listener to an Aruino sketch which will display all mDNS packets over the serial console try the following:
```
#include <ESP8266WiFi.h>
#include "mdns.h"

#define DEBUG_OUTPUT          // Send packet summaries to Serial.
#define DEBUG_RAW             // Send HEX ans ASCII encoded raw packet to Serial.

mdns::MDns my_mdns;

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
```
A more complete example is available int he ./examples/ folder.

Troubleshooting
---------------
Run [Wireshark](https://www.wireshark.org/) on a machine connected to your wireless network to confirm what is actually in flight.
The following filter will return only mDNS packets: ```udp.port == 5353```
