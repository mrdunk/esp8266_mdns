#ifndef MDNS_H
#define MDNS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define MAX_MDNS_NAME_LEN 256


namespace mdns{

// Display a byte on serial console in hexadecimal notation,
// padding with leading zero if necisary to provide evenly tabulated display data.
void PrintHex(unsigned char data);

// Initialise 
void setup();

// Extract Name from DNS data. Will follow pointers used by Message Compression.
// TODO Check for exceeding packet size.
int nameFromDnsPointer(char* p_fqdn_buffer, int fqdn_buffer_pos, byte* p_packet_buffer, int packet_buffer_pos);

void GetMDnsPacket();
int Parse_Query(int packet_buffer_pointer);
int Parse_Answer(int packet_buffer_pointer);
void DisplayRawPacket();



} // namespace mdns

#endif  // MDNS_H
