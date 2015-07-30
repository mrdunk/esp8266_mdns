#ifndef MDNS_H
#define MDNS_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#define DEBUG_OUTPUT          // Send packet summaries to Serial.
#define DEBUG_RAW             // Send HEX ans ASCII encoded raw packet to Serial.

#define MAX_PACKET_SIZE 4096   // Make this as big as memory limitations allow.
#define MAX_MDNS_NAME_LEN 256  // The mDNS spec says this should never be more than 256 (including trailing '\0').
//#define MAX_MDNS_NAME_LEN 10   // test small buffer size.

namespace mdns{

// A single mDNS Query.
struct Query{
  unsigned int buffer_pointer;            // Position of Answer in packet. (Used for debugging only.)
  char qname_buffer[MAX_MDNS_NAME_LEN];   // Question Name: Contains the object, domain or zone name.
  unsigned int qtype;                     // Question Type: Type of question being asked by client.
  unsigned int qclass;                    // Question Class: Normally the value 1 for Internet (“IN”)
  bool unicast_response;                  // 
  bool valid;                             // False if problems were encountered decoding packet.

  void Display();                         // Display a summary of this Answer on Serial port.
};

// A single mDNS Answer.
struct Answer{
  unsigned int buffer_pointer;          // Position of Answer in packet. (Used for debugging only.)
  char name_buffer[MAX_MDNS_NAME_LEN];  // object, domain or zone name.
  char rdata_buffer[MAX_MDNS_NAME_LEN]; // The data portion of the resource record.
  unsigned int rrtype;                  // ResourceRecord Type.
  unsigned int rrclass;                 // ResourceRecord Class: Normally the value 1 for Internet (“IN”)
  unsigned long int rrttl;              // ResourceRecord Time To Live: Number of seconds ths should be remembered.
  bool rrset;                           // Flush cache of records matching this name.
  bool valid;                           // False if problems were encountered decoding packet.

  // Get data from provided p_packet_buffer and populate rdata_buffer.
  // Formatting will be dependant on the Answer type (rrtype).
  // TODO: is this the best place to do this? It might fit better in MDns.
  int PopulateResultBuffer(byte* p_packet_buffer, int packet_buffer_pos, int result_data_len);
  
  void Display();                     // Display a summary of this Answer on Serial port.
};

class MDns {
 public:
  MDns() : init(false), p_query_function_(NULL), p_answer_function_(NULL) {};

  // Takes pointers to functions that will be passed Queries and Answers whenever they arrive.
  MDns(void(*p_query_function)(struct Query), void(*p_answer_function)(struct Answer)) :
      p_query_function_(p_query_function), p_answer_function_(p_answer_function) {};

  // Call this regularly to check for an incoming packet.
  bool Check();

  // Send this MDns packet.
  void Send();

  void Clear();
  void AddQuery(Query query);
  void AddAnswer(Answer answer);
  unsigned int PopulateName(char* name_buffer); // TODO private?
  
  // Display a summary of the packet on Serial port.
  void Display();
  
 private:
  struct Query Parse_Query();
  struct Answer Parse_Answer();
  void DisplayRawPacket();

  // Whether UDP socket has not yet been initialised. 
  bool init;

  // Pointer to function that gets called for every incoming query.
  void(*p_query_function_)(struct Query);

  // Pointer to function that gets called for every incoming answer.
  void(*p_answer_function_)(struct Answer);
  
  unsigned int data_size;
  unsigned int buffer_pointer;  
  byte data_buffer[MAX_PACKET_SIZE];
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
int nameFromDnsPointer(char* p_name_buffer, int name_buffer_pos, int name_buffer_len, 
    byte* p_packet_buffer, int packet_buffer_pos);

bool writeToBuffer(byte value, char* p_name_buffer, int* p_name_buffer_pos, int name_buffer_len);

int parseText(char* data_buffer, int data_buffer_len, int data_len,
    byte* p_packet_buffer, int packet_buffer_pos);

} // namespace mdns

#endif  // MDNS_H
