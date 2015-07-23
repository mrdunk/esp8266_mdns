#include <Arduino.h>
#include "mdns.h"

namespace mdns {


// A UDP instance to let us send and receive packets over UDP.
WiFiUDP Udp;

//IPAddress ipMulti(224, 0, 0, 251);
//unsigned int portMulti = 5353;      // local port to listen on


void PrintHex(unsigned char data) {
  char tmp[2];
  sprintf(tmp, "%02X", data);
  Serial.print(tmp);
  Serial.print(" ");
}

bool MDns::Check() {
  if (!init) {
    init = true;
    Serial.println("Initilising Multicast.");
    Udp.beginMulticast(WiFi.localIP(), IPAddress(224, 0, 0, 251), 5353);
  }
  data_size = Udp.parsePacket();
  if ( data_size > 12) {
    // We've received a packet which is long enough to contain useful data so
    // read the data from it.
    // TODO think about what to do if a packet is larger than buffer.
    Udp.read(data_buffer, data_size); // read the packet into the buffer


    // data_buffer[0] and data_buffer[1] contain the Query ID field which is unused in mDNS.

    // data_buffer[2] and data_buffer[3] are DNS flags which are mostly unused in mDNS.
    type = !(data_buffer[2] & 0b10000000);  // If it's not a query, it's an answer.
    truncated = data_buffer[2] & 0b00000010;  // If it's truncated we can expect more data soon so we should wait for additional recods before deciding whether to respond.
    if (data_buffer[3] & 0b00001111) {
      // Non zero Response code implies error.
      return false;
    }

    // Number of incoming queries.
    query_count = (data_buffer[4] << 8) + data_buffer[5];

    // Number of incoming answers.
    answer_count = (data_buffer[6] << 8) + data_buffer[7];

    // Number of incoming Name Server resource records. (TODO is this used in mDNS?)
    ns_count = (data_buffer[8] << 8) + data_buffer[9];

    // Number of incoming Additional resource records. (TODO is this used in mDNS?)
    ar_count = (data_buffer[10] << 8) + data_buffer[11];

#ifdef DEBUG_OUTPUT
    Display();
#endif  // DEBUG_OUTPUT

    // Start of Data section.
    buffer_pointer = 12;

    for (int i_question = 0; i_question < query_count; i_question++) {
      struct Query query = Parse_Query();
      if (query.valid) {
        if (p_query_function_) {
          // Since a callback function has been registered, execute it.
          p_query_function_(query);
        }
      }
#ifdef DEBUG_OUTPUT
      query.Display();
#endif  // DEBUG_OUTPUT
    }

    for (int i_answer = 0; i_answer < (answer_count + ns_count + ar_count); i_answer++) {
      struct Answer answer = Parse_Answer();
      if (answer.valid) {
        if (p_answer_function_) {
          // Since a callback function has been registered, execute it.
          p_answer_function_(answer);
        }
      }
#ifdef DEBUG_OUTPUT
      answer.Display();
#endif  // DEBUG_OUTPUT
    }

#ifdef DEBUG_RAW
    DisplayRawPacket();
#endif  // DEBUG_RAW

    return true;
  }
  return false;
}

void MDns::Display() {
  Serial.println();
  Serial.print("query size: ");
  Serial.println(data_size);
  Serial.print(" TYPE: ");
  Serial.print(type);
  Serial.print("      QUERY_COUNT: ");
  Serial.print(query_count);
  Serial.print("      ANSWER_COUNT: ");
  Serial.print(answer_count);
  Serial.print("      NS_COUNT: ");
  Serial.print(ns_count);
  Serial.print("      AR_COUNT: ");
  Serial.println(ar_count);
}

struct Query MDns::Parse_Query() {
  struct Query return_value;
  return_value.buffer_pointer = buffer_pointer;

  buffer_pointer = nameFromDnsPointer(return_value.fqdn_buffer, 0, data_buffer, buffer_pointer);

  byte qtype_0 = data_buffer[buffer_pointer++];
  byte qtype_1 = data_buffer[buffer_pointer++];
  byte qclass_0 = data_buffer[buffer_pointer++];
  byte qclass_1 = data_buffer[buffer_pointer++];

  return_value.qtype = (qtype_0 << 8) + qtype_1;

  return_value.unicast_response = (0b10000000 & qclass_0);
  return_value.qclass = ((qclass_0 & 0b01111111) << 8) + qclass_1;

  return_value.valid = true;
  if (return_value.qclass != 0xFF && return_value.qclass != 0x01) {
    // QCLASS is not ANY (0xFF) or INternet (0x01).
    return_value.valid = false;
  }

  if (buffer_pointer > data_size) {
    // We've over-run the returned data.
    // Something has gone wrong receiving or parseing the data.
    return_value.valid = false;
  }
  return return_value;
}

struct Answer MDns::Parse_Answer() {
  struct Answer return_value;
  return_value.buffer_pointer = buffer_pointer;

  buffer_pointer = nameFromDnsPointer(return_value.fqdn_buffer, 0, data_buffer, buffer_pointer);

  return_value.rrtype = (data_buffer[buffer_pointer++] << 8) + data_buffer[buffer_pointer++];

  byte rrclass_0 = data_buffer[buffer_pointer++];
  byte rrclass_1 = data_buffer[buffer_pointer++];
  return_value.rrset = (0b10000000 & rrclass_0);
  return_value.rrclass = ((rrclass_0 & 0b01111111) << 8) + rrclass_1;

  return_value.rrttl = (data_buffer[buffer_pointer++] << 24) +
                       (data_buffer[buffer_pointer++] << 16) +
                       (data_buffer[buffer_pointer++] << 8) +
                       data_buffer[buffer_pointer++];

  int rdlength = (data_buffer[buffer_pointer++] << 8) + data_buffer[buffer_pointer++];

  buffer_pointer = DisplayType(return_value.rrtype, rdlength, return_value.result_buffer, data_buffer, buffer_pointer);

  if (buffer_pointer > data_size) {
    // We've over-run the returned data.
    // Something has gone wrong receiving or parseing the data.
    return_value.valid = false;
  }
  return return_value;
}

// Display packet contents in HEX.
void MDns::DisplayRawPacket() {
  // display the packet contents in HEX
  Serial.println("Raw packet");
  int i, j;

  for (i = 0; i <= data_size; i += 16) {
    Serial.print("0x");
    PrintHex(i >> 8); PrintHex(i);
    Serial.print("   ");
    for (j = 0; j < 16; j++) {
      if (i + j >= data_size) {
        break;
      }
      if (data_buffer[i + j] > 31 and data_buffer[i + j] < 128) {
        Serial.print((char)data_buffer[i + j]);
      } else {
        Serial.print(".");
      }
    }
    Serial.print("    ");
    for (j = 0; j < 16; j++) {
      if (i + j >= data_size) {
        break;
      }
      PrintHex(data_buffer[i + j]);
      Serial.print(' ');
    }
    Serial.println();
  }
}

int nameFromDnsPointer(char* p_fqdn_buffer, int fqdn_buffer_pos,
                       byte* p_packet_buffer, int packet_buffer_pos) {

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

int DisplayType(unsigned int type, int data_len, char* p_buffer,
                byte* p_packet_buffer, int packet_buffer_pos) {
  switch (type) {
    case 1:
      // A. Returns a 32-bit IPv4 address
      sprintf(p_buffer, "%u.%u.%u.%u", p_packet_buffer[packet_buffer_pos++], p_packet_buffer[packet_buffer_pos++]
              , p_packet_buffer[packet_buffer_pos++], p_packet_buffer[packet_buffer_pos++]);
      break;
    case 0xC:
      // PTR.  Pointer to a canonical name.
      packet_buffer_pos = nameFromDnsPointer(p_buffer, 0, p_packet_buffer, packet_buffer_pos);
      break;
    case 0xD:
      // HINFO. host information
      packet_buffer_pos = nameFromDnsPointer(p_buffer, 0, p_packet_buffer, packet_buffer_pos);
      break;
    case 0x10:
      // TXT.  Originally for arbitrary human-readable text in a DNS record.
      for (int i = 0; i < data_len; i++) {
        p_buffer[packet_buffer_pos++] = p_packet_buffer[packet_buffer_pos++];
      }
      break;
    case 0x1C:
      // AAAA.  Returns a 128-bit IPv6 address.
      for (int i = 0; i < data_len; i++) {
        sprintf(p_buffer, "%02X:", p_packet_buffer[packet_buffer_pos++]);
        p_buffer += 3;
      }
      break;
    case 0x21:
      // SRV. Server Selection.
    default:
      int buffer_pos = 0;
      for (int i = 0; i < data_len; i++) {
        if (buffer_pos < MAX_MDNS_NAME_LEN - 3) {
          sprintf(p_buffer + buffer_pos, "%02X ", p_packet_buffer[packet_buffer_pos++]);
        } else {
          packet_buffer_pos++;
        }
        buffer_pos += 3;
      }
      break;
  }
  return packet_buffer_pos;
}

void DisplayClass(unsigned int ip_class) {
  if (ip_class == 1) {
    Serial.print("Internet (IN)");
  } else {
    Serial.print("Unknown");
  }
}


void Query::Display() {
  Serial.print("question  0x");
  Serial.println(buffer_pointer, HEX);
  if (valid) {
    Serial.print(" QNAME:    ");
    Serial.print(fqdn_buffer);
    Serial.print("      QTYPE:  0x");
    Serial.print(qtype, HEX);
    Serial.print("      QCLASS: 0x");
    Serial.print(qclass, HEX);
    Serial.print("      Unicast Response: ");
    Serial.println(unicast_response);
  } else {
    Serial.println(" **ERROR**");
  }
}

void Answer::Display() {
  //if (strncmp(fqdn_buffer, "_mqtt.", 6) == 0 || strncmp(fqdn_buffer, "twinkle", 7) == 0) {
  Serial.print("answer  0x");
  Serial.println(buffer_pointer, HEX);
  if (valid) {
    Serial.print(" RRNAME:    ");
    Serial.print(fqdn_buffer);
    Serial.print("      RRTYPE:  0x");
    Serial.print(rrtype, HEX);
    Serial.print("      RRCLASS: 0x");
    Serial.print(rrclass, HEX);
    Serial.print("      RRTTL: ");
    Serial.print(rrttl);
    Serial.print("      RRSET: ");
    Serial.println(rrset);
    Serial.print(" RRDATA:    ");
    Serial.println(result_buffer);
  } else {
    Serial.println(" **ERROR**");
  }
  //}
}

} // namespace mdns
