/*
 * This sketch will display data mDNS (multicast DNS) data seen on the network.
 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include "secrets.h"  // Contains the following:
// const char* ssid = "Get off my wlan";  //  your network SSID (name)
// const char* pass = "secretwlanpass";       // your network password


#define MAX_MDNS_NAME_LEN 256


int status = WL_IDLE_STATUS;

unsigned int localPort = 2390;      // local port to listen for UDP packets

byte packetBuffer[4096]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;

// Multicast declarations
IPAddress ipMulti(224, 0, 0, 251);
unsigned int portMulti = 5353;      // local port to listen on

void PrintHex(unsigned char data) {
  char tmp[2];
  sprintf(tmp, "%02X", data);
  Serial.print(tmp);
  Serial.print(" ");
}

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
  Serial.print("Udp Multicast Receiver listening at : ");
  Serial.print(ipMulti);
  Serial.print(":");
  Serial.println(portMulti);
  Udp.beginMulticast(WiFi.localIP(),  ipMulti, portMulti);
}

// Extract Name from DNS data. Will follow pointers used by Message Compression.
// TODO Check for exceeding packet size.
int nameFromDnsPointer(char* p_fqdn_buffer, int fqdn_buffer_pos, 
      byte* p_packet_buffer, int packet_buffer_pos){
  
  /*Serial.print("nameFromDnsPointer(");
  Serial.print(*(p_fqdn_buffer + fqdn_buffer_pos), HEX);
  Serial.print(", ");
  Serial.print(fqdn_buffer_pos, HEX);
  Serial.print(", ");
  Serial.print(*(p_packet_buffer + packet_buffer_pos), HEX);
  Serial.print(", ");
  Serial.print(packet_buffer_pos, HEX);
  Serial.println(")");*/
  
  if(fqdn_buffer_pos > 0 && *(p_fqdn_buffer -1 + fqdn_buffer_pos) == '\0'){
    // Since we are adding more to an already populated buffer,
    // replace the trailing EOL with the FQDN seperator.
    *(p_fqdn_buffer -1 + fqdn_buffer_pos) = '.';
  }
  
  if(*(p_packet_buffer + packet_buffer_pos) < 0xC0){
    // Since the first 2 bits are not set,
    // this is the start of a name section.
    // http://www.tcpipguide.com/free/t_DNSNameNotationandMessageCompressionTechnique.htm
    int word_len = *(p_packet_buffer + packet_buffer_pos++);
    for(int l = 0; l < word_len; l++){
      if(fqdn_buffer_pos >= MAX_MDNS_NAME_LEN){
        return packet_buffer_pos;
      }
      *(p_fqdn_buffer + fqdn_buffer_pos++) = *(p_packet_buffer + packet_buffer_pos++);
    }
    *(p_fqdn_buffer + fqdn_buffer_pos++) = '\0';
    if(*(p_packet_buffer + packet_buffer_pos) > 0){
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

void loop()
{
  int noBytes = Udp.parsePacket();
  if ( noBytes ) {
    Serial.println();
    Serial.print(millis() / 1000);
    Serial.print(":Packet of ");
    Serial.print(noBytes);
    Serial.print("bytes received from ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort());
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, noBytes); // read the packet into the buffer

    Serial.print(" ID:     ");
    PrintHex(packetBuffer[0]);
    PrintHex(packetBuffer[1]);
    Serial.println();

    Serial.print(" Flags:  ");
    PrintHex(packetBuffer[2]);
    PrintHex(packetBuffer[3]);
    Serial.println();

    Serial.print(" QDCOUNT: ");
    PrintHex(packetBuffer[4]);
    PrintHex(packetBuffer[5]);
    Serial.println();
    int questions = 0xFF * packetBuffer[4] + packetBuffer[5];
    //Serial.println(questions);
      
    Serial.print(" ANCOUNT:  ");
    PrintHex(packetBuffer[6]);
    PrintHex(packetBuffer[7]);
    Serial.println();
    int answers = 0xFF * packetBuffer[6] + packetBuffer[7];
    //Serial.println(answers);

    Serial.print(" NSCOUNT:  ");
    PrintHex(packetBuffer[8]);
    PrintHex(packetBuffer[9]);
    Serial.println();

    Serial.print(" ARCOUNT:  ");
    PrintHex(packetBuffer[10]);
    PrintHex(packetBuffer[11]);
    Serial.println();

    int i = 12;
    char fqdn_buffer[MAX_MDNS_NAME_LEN];
    
    for(int question=0; question < questions; question++){
      Serial.print("question  0x");
      Serial.println(i, HEX);
      
      i = nameFromDnsPointer(fqdn_buffer, 0, packetBuffer, i);
      Serial.print(" QNAME:    ");
      Serial.println(fqdn_buffer);

      Serial.print(" QTYPE:    ");
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      Serial.println();

      Serial.print(" QCLASS:   ");
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      Serial.println();
    }

    for(int answer=0; answer < answers; answer++){
      Serial.print("answer  0x");
      Serial.println(i, HEX);
      
      i = nameFromDnsPointer(fqdn_buffer, 0, packetBuffer, i);
      Serial.print(" NAME:     ");
      Serial.println(fqdn_buffer);

      Serial.print(" TYPE:     ");
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      Serial.println();

      Serial.print(" CLASS:    ");
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      Serial.println();

      Serial.print(" TTL:      ");
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      Serial.println();

      Serial.print(" RDLENGTH: ");
      PrintHex(packetBuffer[i++]);
      PrintHex(packetBuffer[i++]);
      int rdlength = 0xFF * packetBuffer[i -2] + packetBuffer[i -1];
      Serial.print("(");
      Serial.print(rdlength);
      Serial.println(")");

      Serial.print(" RDATA:    ");
      for(int j = 0; j < rdlength; ++j){
        PrintHex(packetBuffer[i++]);
      }
      Serial.println();      
    }
    
    // display the packet contents in HEX
    Serial.println("Raw packet");
    i = 12;
    int j;
    for (i = 12; i <= noBytes; i += 16) {
      Serial.print("0x");
      PrintHex(i >> 8); PrintHex(i);
      Serial.print("  ");
      for (j = 0; j < 16; j++) {
        if(i + j >= noBytes){
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
        if(i + j >= noBytes){
          break;
        }
        PrintHex(packetBuffer[i + j]);
        Serial.print(' ');
      }
      Serial.println();
    }
  } // end if
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
