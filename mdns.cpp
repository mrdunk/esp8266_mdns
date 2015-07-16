#include "mdns.h"

namespace mdns {


// A UDP instance to let us send and receive packets over UDP.
WiFiUDP Udp;

IPAddress ipMulti(224, 0, 0, 251);
unsigned int portMulti = 5353;      // local port to listen on

//buffer to hold incoming and outgoing packets
byte packetBuffer[4096];

int received_packet_size;

char fqdn_buffer[MAX_MDNS_NAME_LEN];


void PrintHex(unsigned char data) {
  char tmp[2];
  sprintf(tmp, "%02X", data);
  Serial.print(tmp);
  Serial.print(" ");
}


void setup() {
  Udp.beginMulticast(WiFi.localIP(),  mdns::ipMulti, mdns::portMulti);
}


int nameFromDnsPointer(char* p_fqdn_buffer, int fqdn_buffer_pos,
                       byte* p_packet_buffer, int packet_buffer_pos) {

  /*Serial.print("nameFromDnsPointer(");
  Serial.print(*(p_fqdn_buffer + fqdn_buffer_pos), HEX);
  Serial.print(", ");
  Serial.print(fqdn_buffer_pos, HEX);
  Serial.print(", ");
  Serial.print(*(p_packet_buffer + packet_buffer_pos), HEX);
  Serial.print(", ");
  Serial.print(packet_buffer_pos, HEX);
  Serial.println(")");*/

  if (fqdn_buffer_pos > 0 && *(p_fqdn_buffer - 1 + fqdn_buffer_pos) == '\0') {
    // Since we are adding more to an already populated buffer,
    // replace the trailing EOL with the FQDN seperator.
    *(p_fqdn_buffer - 1 + fqdn_buffer_pos) = '.';
  }

  if (*(p_packet_buffer + packet_buffer_pos) < 0xC0) {
    // Since the first 2 bits are not set,
    // this is the start of a name section.
    // http://www.tcpipguide.com/free/t_DNSNameNotationandMessageCompressionTechnique.htm
    int word_len = *(p_packet_buffer + packet_buffer_pos++);
    for (int l = 0; l < word_len; l++) {
      if (fqdn_buffer_pos >= MAX_MDNS_NAME_LEN) {
        return packet_buffer_pos;
      }
      *(p_fqdn_buffer + fqdn_buffer_pos++) = *(p_packet_buffer + packet_buffer_pos++);
    }
    *(p_fqdn_buffer + fqdn_buffer_pos++) = '\0';
    if (*(p_packet_buffer + packet_buffer_pos) > 0) {
      packet_buffer_pos =
        nameFromDnsPointer(p_fqdn_buffer, fqdn_buffer_pos, p_packet_buffer, packet_buffer_pos);
    } else {
      packet_buffer_pos++;
    }
  } else {
    // Message Compression used. Next 2 bytes are a pointer to the actual name section.
    int pointer = (*(p_packet_buffer + packet_buffer_pos++) - 0xC0) << 8;
    pointer += *(p_packet_buffer + packet_buffer_pos++);
    nameFromDnsPointer(p_fqdn_buffer, fqdn_buffer_pos, p_packet_buffer, pointer);
  }
  return packet_buffer_pos;
}


void GetMDnsPacket() {
  received_packet_size = Udp.parsePacket();
  if ( received_packet_size ) {
    Serial.println();
    Serial.print(millis() / 1000);
    Serial.print("ms : Packet of ");
    Serial.print(received_packet_size);
    Serial.print("bytes received from ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort());

    // We've received a packet, read the data from it
    Udp.read(packetBuffer, received_packet_size); // read the packet into the buffer

    if (received_packet_size <= 12) {
      // Packet too small to have any usefull data.
      return;
    }
    // TODO think about what to do if a packet is larger than received_packet_size.


    // packetBuffer[0] and packetBuffer[1] contain the Query ID field which is unused in mDNS.

    // packetBuffer[2] and packetBuffer[3] are DNS flags which are mostly unused in mDNS.
    bool query = !(packetBuffer[2] & 0b10000000);  // If it's not a query, it's an answer.
    bool truncated = packetBuffer[2] & 0b00000010;  // If it's truncated we can expect more data soon so we should wait for additional recods before deciding whether to respond.
    if (packetBuffer[3] & 0b00001111) {
      // Non zero Response code implies error.
      return;
    }

    // Number of incoming queries.
    unsigned int query_count = (packetBuffer[4] << 8) + packetBuffer[5];

    // Number of incoming answers.
    unsigned int answer_count = (packetBuffer[6] << 8) + packetBuffer[7];

    // Number of incoming Name Server resource records. (TODO is this used in mDNS?)
    unsigned int ns_count = (packetBuffer[8] << 8) + packetBuffer[9];

    // Number of incoming Additional resource records. (TODO is this used in mDNS?)
    unsigned int ar_count = (packetBuffer[10] << 8) + packetBuffer[11];


    // Start of Data section.
    int packet_buffer_pointer = 12;

    for (int question = 0; question < query_count; question++) {
      packet_buffer_pointer = Parse_Query(packet_buffer_pointer);
    }

    for (int answer = 0; answer < answer_count; answer++) {
      packet_buffer_pointer = Parse_Answer(packet_buffer_pointer);
    }

    DisplayRawPacket();

  } // end if
}

int Parse_Query(int packet_buffer_pointer) {
  Serial.print("question  0x");
  Serial.println(packet_buffer_pointer, HEX);

  packet_buffer_pointer = nameFromDnsPointer(fqdn_buffer, 0, packetBuffer, packet_buffer_pointer);
  Serial.print(" QNAME:    ");
  Serial.println(fqdn_buffer);

  byte qtype_0 = packetBuffer[packet_buffer_pointer++];
  byte qtype_1 = packetBuffer[packet_buffer_pointer++];
  byte qclass_0 = packetBuffer[packet_buffer_pointer++];
  byte qclass_1 = packetBuffer[packet_buffer_pointer++];

  unsigned int qtype = (qtype_0 << 8) + qtype_1;
  
  bool unicast_response = (0b10000000 & qclass_0);
  unsigned int qclass = ((qclass_0 & 0b01111111) << 8) + qclass_1;
  
  bool valid = true;
  if(qclass != 0xFF && qclass != 0x01){
    // QCLASS is not ANY (0xFF) or INternet (0x01).
    valid = false;
  }

  Serial.print(" QTYPE:  0x");
  Serial.print(qtype, HEX);
  Serial.print("      QCLASS: 0x");
  Serial.print(qclass, HEX);
  Serial.print("      Unicast Response: ");
  Serial.print(unicast_response);
  Serial.print("      Class valid: ");
  Serial.println(valid);
  
  return packet_buffer_pointer;
}

int Parse_Answer(int packet_buffer_pointer){
      Serial.print("answer  0x");
      Serial.println(packet_buffer_pointer, HEX);

      packet_buffer_pointer = nameFromDnsPointer(fqdn_buffer, 0, packetBuffer, packet_buffer_pointer);
      Serial.print(" NAME:     ");
      Serial.println(fqdn_buffer);

      Serial.print(" TYPE:     ");
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      Serial.println();

      Serial.print(" CLASS:    ");
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      Serial.println();

      Serial.print(" TTL:      ");
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      Serial.println();

      Serial.print(" RDLENGTH: ");
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      PrintHex(packetBuffer[packet_buffer_pointer++]);
      int rdlength = (packetBuffer[packet_buffer_pointer - 2] << 8) + packetBuffer[packet_buffer_pointer - 1];
      Serial.print("(");
      Serial.print(rdlength);
      Serial.println(")");

      Serial.print(" RDATA:    ");
      for (int j = 0; j < rdlength; ++j) {
        PrintHex(packetBuffer[packet_buffer_pointer++]);
      }
      Serial.println();

      return packet_buffer_pointer;
}

// Display packet contents in HEX.
void DisplayRawPacket() {

  // display the packet contents in HEX
  Serial.println("Raw packet");
  int i, j;

  for (i = 0; i <= received_packet_size; i += 16) {
    Serial.print("0x");
    PrintHex(i >> 8); PrintHex(i);
    Serial.print("   ");
    for (j = 0; j < 16; j++) {
      if (i + j >= received_packet_size) {
        break;
      }
      if (packetBuffer[i + j] > 31 and packetBuffer[i + j] < 128) {
        Serial.print((char)packetBuffer[i + j]);
      } else {
        Serial.print(".");
      }
    }
    Serial.print("    ");
    for (j = 0; j < 16; j++) {
      if (i + j >= received_packet_size) {
        break;
      }
      PrintHex(packetBuffer[i + j]);
      Serial.print(' ');
    }
    Serial.println();
  }
}

} // namespace mdns
