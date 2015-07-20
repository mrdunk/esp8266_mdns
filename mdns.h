#ifndef MDNS_H
#define MDNS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define MAX_MDNS_NAME_LEN 256


namespace mdns{

struct Query{
  unsigned int buffer_pointer;  
  char fqdn_buffer[MAX_MDNS_NAME_LEN];
  unsigned int qtype;
  unsigned int qclass;
  bool unicast_response;
  bool valid;

  void Display();
};

struct Answer{
  unsigned int buffer_pointer;
  char fqdn_buffer[MAX_MDNS_NAME_LEN];
  char result_buffer[MAX_MDNS_NAME_LEN];
  unsigned int rrtype;
  unsigned int rrclass;
  unsigned long int rrttl;
  bool rrset;

  void Display();
};

class MDns {
 public:
  MDns();
  bool Check();
 private:
  struct Query Parse_Query();
  struct Answer Parse_Answer();
  void DisplayRawPacket();

  bool init;
  
  unsigned int data_size;
  unsigned int buffer_pointer;  
  byte data_buffer[4096];
  bool type;
  bool truncated;
  unsigned int query_count;
  unsigned int answer_count;
  unsigned int ns_count;
  unsigned int ar_count;
};


// Display a byte on serial console in hexadecimal notation,
// padding with leading zero if necisary to provide evenly tabulated display data.
void PrintHex(unsigned char data);

// Extract Name from DNS data. Will follow pointers used by Message Compression.
// TODO Check for exceeding packet size.
int nameFromDnsPointer(char* p_fqdn_buffer, int fqdn_buffer_pos, byte* p_packet_buffer, int packet_buffer_pos);

int DisplayType(unsigned int type, int data_len, char* p_buffer,
      byte* p_packet_buffer, int packet_buffer_pos);

} // namespace mdns

#endif  // MDNS_H
