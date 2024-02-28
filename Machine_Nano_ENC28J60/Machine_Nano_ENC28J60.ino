/*
  Arduino Nano/ENC28J60 example for AgOpenGPS machine & section control class
    Matt Elias Feb 2024
    Adapted from Machine_UDP_v5 in official AOG repo

*/

//the default network address
struct ConfigIP {
    uint8_t ipOne = 192;
    uint8_t ipTwo = 168;
    uint8_t ipThree = 5;
};  ConfigIP networkAddress;   //3 bytes
//-----------------------------------------------------------------------------------------------
#include <EEPROM.h>
#include "src\EtherCard_AOG.h"
#include <IPAddress.h>
#include "machine.h"

static uint8_t myIP[]  = { 0,0,0,123 };                  // ethernet interface ip address
static uint8_t gwIP[]  = { 0,0,0,1 };                    // gateway ip address
static uint8_t myDNS[] = { 8,8,8,8 };                   // DNS - you just need one anyway
static uint8_t netMask[] = { 255,255,255,0 };           // subnet
static uint8_t myMAC[] = { 0x0,0x0,0x56,0x0,0x0,0x7B }; // ethernet mac address - must be unique on your network
static uint8_t broadcastIP[] = { 0,0,0,255 };           // broadcast IP, back to AgIO
const uint16_t portFrom = 5123;                         // sending port of this module
const uint16_t portDestination = 9999;                  // port that AgIO listens on
uint8_t Ethernet::buffer[200];                          // udp send and receive buffer
#define CS_Pin 10       //ethercard 10,11,12,13, Nano = 10 depending how CS of ENC28J60 is Connected

void(*resetFunc) (void) = 0;      //Program counter reset
uint8_t serialResetTimer = 0;     //if serial buffer is getting full, empty it
int16_t temp, EEread = 0, EEP_Ident = 1234;

// arrange these Arduino pin numbers in order of desired output, (all available pins on "old" bootloader Nano listed below)
uint8_t arduinoOutputPinNumbers[] = { 2,3,4,5,6,7,8,9,A0,A1,A2,A3,A4,A5 };
MACHINE machine;

uint16_t outputPinStates;

void setup()
{
  Serial.begin(115200);
  Serial.print("\r\n\n************************************************\nStart setup\n\n");

  EEPROM.get(0, EEread);      // read identifier

  if (EEread != EEP_Ident) {  // check on first start and write EEPROM
    EEPROM.put(0, EEP_Ident);
    EEPROM.put(5, networkAddress);
  } else {
    EEPROM.get(5, networkAddress);
  }

  if (ether.begin(sizeof Ethernet::buffer, myMAC, CS_Pin) == 0)
      Serial.println(F("Failed to access Ethernet controller"));

  // grab the ip from EEPROM
  myIP[0] = networkAddress.ipOne;
  myIP[1] = networkAddress.ipTwo;
  myIP[2] = networkAddress.ipThree;

  gwIP[0] = networkAddress.ipOne;
  gwIP[1] = networkAddress.ipTwo;
  gwIP[2] = networkAddress.ipThree;

  broadcastIP[0] = networkAddress.ipOne;
  broadcastIP[1] = networkAddress.ipTwo;
  broadcastIP[2] = networkAddress.ipThree;

  // set up UDP connection
  ether.staticSetup(myIP, gwIP, myDNS, netMask);
  ether.udpServerListenOnPort(&parseUdpData, (uint16_t)8888);     //register to port 8888

  ether.printIp("_IP_: ", ether.myip);
  ether.printIp("GWay: ", ether.gwip);
  ether.printIp("AgIO: ", broadcastIP);

  machine.init(arduinoOutputPinNumbers, sizeof(arduinoOutputPinNumbers), 100);

  Serial.println("\r\n\nSetup complete, waiting for AgOpenGPS");
}

void loop()
{
  delay(1);

  //this must be called for ethercard functions to work. Calls parseUdpData() defined below.
  ether.packetLoop(ether.packetReceive());

  machine.watchdogCheck();      // used to check if UDP comms (PGN updates) have failed and turn outputs OFF

  readOutputPinStates();
}


void parseUdpData(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, uint8_t* udpData, uint16_t len)
{
  if (udpData[0] != 0x80 || udpData[1] != 0x81 || udpData[2] != 0x7F) return; // if these don't match, reject it
  /*IPAddress src(src_ip[0],src_ip[1],src_ip[2],src_ip[3]);
  Serial.print("dPort:");  Serial.print(dest_port);
  Serial.print("  sPort: ");  Serial.print(src_port);
  Serial.print("  sIP: ");  ether.printIp(src_ip);  Serial.print("  len:"); Serial.println(len);*/


  if (udpData[3] == 200)            // 0xC8 (200) - Hello from AgIO
  {
    Serial.print("\n0x"); Serial.print(udpData[3], HEX); Serial.print(" ("); Serial.print(udpData[3]); Serial.print(") - ");
    Serial.print("Hello from AgIO");

    const uint8_t helloFromMachine[] = { 128, 129, 123, 123, 5, 0, 0, 0, 0, 0, 71 };
    ether.sendUdp(helloFromMachine, 11, portFrom, broadcastIP, portDestination);
  }


  else if (udpData[3] == 201)       // 0xC9 (201) - Subnet Change
  {
    Serial.print("\n0x"); Serial.print(udpData[3], HEX); Serial.print(" ("); Serial.print(udpData[3]); Serial.print(") - ");
    Serial.print("Subnet Change");

    if (udpData[4] == 5 && udpData[5] == 201 && udpData[6] == 201)        // make really sure this is the subnet pgn
    {
      networkAddress.ipOne = udpData[7];
      networkAddress.ipTwo = udpData[8];
      networkAddress.ipThree = udpData[9];

      //save in EEPROM and restart
      EEPROM.put(5, networkAddress);
      Serial.print("\r\nRebooting for network IP change");
      delay(5);
      resetFunc();
    }
  }

  
  else if (udpData[3] == 202)       // 0xCA (202) - Scan Request
  {
    Serial.print("\n0x"); Serial.print(udpData[3], HEX); Serial.print(" ("); Serial.print(udpData[3]); Serial.print(") - ");
    Serial.print("Scan Request");

    if (udpData[4] == 3 && udpData[5] == 202 && udpData[6] == 202)   // make really sure this is the scan pgn
    {
      uint8_t scanReply[] = { 128, 129, 123, 203, 7, 
        networkAddress.ipOne, networkAddress.ipTwo, networkAddress.ipThree, 123,
        src_ip[0], src_ip[1], src_ip[2], 23   };

      //checksum
      int16_t CK_A = 0;
      for (uint8_t i = 2; i < sizeof(scanReply) - 1; i++)
      {
        CK_A = (CK_A + scanReply[i]);
      }
      scanReply[sizeof(scanReply)-1] = CK_A;

      static uint8_t superBroadcastIP[] = { 255,255,255,255 };

      ether.sendUdp(scanReply, sizeof(scanReply), portFrom, superBroadcastIP, portDestination);
    }
  }


  // just added here to suppress repeated "Unknown PGN data" msgs
  else if (udpData[3] == 0xFE)                // 0xFE (254) - Steer Data
  {
    //Serial.print("\n0x"); Serial.print(udpData[3], HEX); Serial.print(" ("); Serial.print(udpData[3]); Serial.print(") - ");
    //Serial.print("Steer Data");
  }


  else if (machine.parsePGN(udpData, len))    // if no PGN matches yet, look for Machine PGNs
  {
    //Serial.print("\r\nMachine/Section PGN matched");
  }


  else      // catch & alert to all other PGN data
  {
    Serial.print("\r\n0x"); Serial.print(udpData[3], HEX); Serial.print("("); Serial.print(udpData[3]); Serial.print(") - Unknown PGN, len: "); Serial.print(len);
  }
}

void readOutputPinStates()
{
  uint16_t prevPinStates = outputPinStates;
  for (uint8_t i = 0; i < sizeof(arduinoOutputPinNumbers); i++)
  {
    if (digitalRead(arduinoOutputPinNumbers[i])){
      bitSet(outputPinStates, i);
    } else {
      bitClear(outputPinStates, i);
    }
  }
  
  if (outputPinStates != prevPinStates) {
    Serial.println();
    machine.printBinaryByteLSB(outputPinStates, 8);
    machine.printBinaryByteLSB(outputPinStates >> 8, 8);
  }
}


