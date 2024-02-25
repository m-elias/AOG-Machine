/*
   Attempt to create classes and put more code into functions for readability
*/

#include <Streaming.h>
#include <EEPROM.h>
#include <NativeEthernet.h>
#include <NativeEthernetUdp.h>
#include <IPAddress.h>
#include "clsPCA9555.h" // https://github.com/nicoverduin/PCA9555
#include "machine.h"

const uint8_t LONGER_UDP_PACKET_SIZE = 40; // currently the longest PGN is 30 (Section Dimension - 39 bytes), UDP_TX_PACKET_MAX_SIZE is only 24
uint8_t pgnData[LONGER_UDP_PACKET_SIZE];   // Buffer For Receiving UDP Data

// IP & MAC address of this module of this module
byte myip[4] = { 192, 168, 5, 123};                 // 123 is machine IP
byte mac[] = {0x00, 0x00, 0x56, 0x00, 0x00, 0x78};

const unsigned int AOG_PGN_PORT = 8888;   // port for receiving UDP data from AOG
const unsigned int DEST_PORT = 9999;    // port for sending UDP data to AOG

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Eth_PGNs;         // PGN In & Out Port 8888

IPAddress PGN_BROADCAST_IP = { myip[0], myip[1], myip[2], 255 };
bool Ethernet_running = false; //Auto set on in ethernet setup

extern "C" uint32_t set_arm_clock(uint32_t frequency);    // required prototype for setting CPU speed

PCA9555 pcaOutputs(0x20);          // for AiO v5.0a
MACHINE machine;

uint8_t arduinoOutputPinNumbers[] = { 31, 30, 22, 23, 1, 0 };    // all (3) can bus ports, using can bus comm LEDs on AiO v5.0a
uint8_t pcaOutputPinNumbers[8] = { 1, 0, 12, 15, 9, 8, 6, 7 };   // all 8 PCA9555 section/machine output pin numbers on AiO v5.0a
//const uint8_t pcaInputPinNumbers[]  = { 14, 13, 11, 10, 2, 3, 4, 5 }; // all 8 PCA9555 section/machine output "sensing" pin numbers on AiO v5.0a

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial << "\r\n************************************************\nStart setup\n\n";

  delay(500);                         //Small delay so serial can monitor start up
  //set_arm_clock(150000000);           //Set CPU speed to 150mhz
  Serial << "CPU speed set to: " << F_CPU_ACTUAL;

  Serial << "\r\nStarting Ethernet...";
  EthernetStart();

  if (pcaOutputs.begin()) {
    Serial << "\nSection outputs (PCA9555) detected (8 channels, low side switching)";
    machine.init(&pcaOutputs, pcaOutputPinNumbers, 100);   // for PCA9555, AiO v5.0a FET output control (low side)
  }
  
  // for regular "Arduino" pin control
  machine.init(arduinoOutputPinNumbers, sizeof(arduinoOutputPinNumbers), 100);


  Serial << "\r\nEnd setup\r\n";
}



void loop() {
  CheckPGNs();
  machine.watchdogCheck();
}



void CheckPGNs()
{
  //Serial << "\r\nchecking UDP";
  if (!Ethernet_running) {    // When ethernet is not running, return directly. parsePacket() will block when we don't
    Serial << "***Ethernet NOT running***";
    return;
  }

  uint16_t len = Eth_PGNs.parsePacket();
  if (len < 5) return;      // Check for len > 4, because we check byte 0, 1, 2 and 3

  Eth_PGNs.read(pgnData, LONGER_UDP_PACKET_SIZE);

  if (pgnData[0] != 0x80 && pgnData[1] != 0x81 && pgnData[2] != 0x7F) return;      // verify the first three bytes are AoG headers
  
  if (pgnData[3] == 0xFE)// && Autosteer_running)  // 0xFE (254) - Steer Data
  {
    /*Serial.print("\n0x"); Serial.print(pgnData[3], HEX); Serial.print((String)" (" + pgnData[3] + ") - ");
    //Serial.print("Steer Data");
    //Serial.print("\n0:"); machine.printBinary(pgnData[11]);// Serial.print(" "); Serial.print(relayLo,BIN);
    //Serial.print(" 1:"); machine.printBinary(pgnData[12]);// Serial.print(" "); Serial.print(relayHi,BIN);
    Serial.println();*/
  }

  else if (pgnData[3] == 0xFC)// && Autosteer_running)  // 0xFC (252) - Steer Settings
  {
    Serial.print("\n0x"); Serial.print(pgnData[3], HEX); Serial.print((String)" (" + pgnData[3] + ") - ");
    Serial.print("Steer Settings");
  }

  else if (pgnData[3] == 0xC8) // 0xC8 (200) - Hello from AgIO
  {
    //Serial.print("\n0x"); Serial.print(pgnData[3], HEX); Serial.print((String)" (" + pgnData[3] + ") - ");
    //Serial.print("Hello from AgIO");

    if (myip[3] == 126) // this is the steer module IP, reply as steer module
    {
      /*int16_t sa = (int16_t)(steerAngleActual * 100);

      helloFromAutoSteer[5] = (uint8_t)sa;
      helloFromAutoSteer[6] = sa >> 8;

      helloFromAutoSteer[7] = (uint8_t)helloSteerPosition;
      helloFromAutoSteer[8] = helloSteerPosition >> 8;
      helloFromAutoSteer[9] = switchByte;

      SendUdp(helloFromAutoSteer, sizeof(helloFromAutoSteer), Eth_ipDestination, portDestination);
      */
    }
    
    else if (myip[3] == 123) // this is the machine module IP, reply as machine module
    {
      /*if (udpData[7] == 1)  // not sure what this does if it's even implemented/used
      {
          relayLo -= 255;
          relayHi -= 255;
          watchdogTimer = 0;
      }*/
      uint8_t relayLo = 0;
      uint8_t relayHi = 0;
      uint8_t helloFromMachine[] = { 128, 129, 123, 123, 5, relayLo, relayHi, 0, 0, 0, 71 };
      SendUdp(helloFromMachine, sizeof(helloFromMachine), PGN_BROADCAST_IP, DEST_PORT);
    }
    
    else if (myip[3] == 121) // this is the IMU module IP, reply as IMU module
    {
      // should also reply even if other module but IMU is read by it
      /*if(useBNO08x || useCMPS)
        SendUdp(helloFromIMU, sizeof(helloFromIMU), Eth_ipDestination, portDestination); 
      */
    }
    
    else if (myip[3] == 120) // this is the GPS module IP, reply as GPS module
    {
      
    }
    
    else
    {
      Serial << "\r\nUnknown module IP: " << myip[3] << ", no reply sent to AgIO";
    }

  }


  else if (pgnData[3] == 202)  // 0xCA (202) - Scan Request
  {
    Serial.print("\n0x"); Serial.print(pgnData[3], HEX); Serial.print((String)" (" + pgnData[3] + ") - ");
    Serial.print("Scan Request");
    if (pgnData[4] == 3 && pgnData[5] == 202 && pgnData[6] == 202) {
      IPAddress rem_ip = Eth_PGNs.remoteIP();

      uint8_t scanReplyMachine[] = { 128, 129, 123, 203, 7,
                              myip[0], myip[1], myip[2], myip[3],
                              rem_ip[0], rem_ip[1], rem_ip[2], 23 };
      //checksum
      uint8_t CK_A = 0;
      for (uint8_t i = 2; i < sizeof(scanReplyMachine) - 1; i++) {
        CK_A = (CK_A + scanReplyMachine[i]);
      }
      scanReplyMachine[sizeof(scanReplyMachine) - 1] = CK_A;
      SendUdp(scanReplyMachine, sizeof(scanReplyMachine), PGN_BROADCAST_IP, DEST_PORT);
    }
  }  // 0xCA (202) - Scan Request


  /*else if ( udpData[3] == 235 ||  // 0xEB (235) - Section Dimensions
              udpData[3] == 236 ||  // 0xEC (236) - Machine Pin Config
              udpData[3] == 238 ||  // 0xEE (238) - Machine Config
              udpData[3] == 239 ||  // 0xEF (239) - Machine Data
              udpData[3] == 229 ){  // 0xE5 (229) - 64 Section Data*/
  else if (machine.parsePGN(pgnData, len))    // if no PGN matches yet, look for Machine PGNs
  {
    //Serial << "\nChecking machine PGNs";
  }


  // catch & alert to all other eth data
  else
  {
    Serial << "\r\n0x"; Serial.print(pgnData[3], HEX); Serial << "(" << pgnData[3] << ") - Unknown PGN data, len: " << len;
  }
}

void SendUdp(uint8_t *data, uint8_t datalen, IPAddress dip, uint16_t dport)
{
  Eth_PGNs.beginPacket(dip, dport);
  Eth_PGNs.write(data, datalen);
  Eth_PGNs.endPacket();
}
