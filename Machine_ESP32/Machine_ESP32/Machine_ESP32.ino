/*
  Copied from various examples/places
  Matt Elias 2024

  Developed on WEMOS D1 R32 (ESP32)

  to do:
    Add code to change IP & save to eeprom

*/
#include <EEPROM.h>

#include "WiFi.h"
// choose one option, set Wifi details accordingly below
//#define AP
#define STN

#ifdef AP
  const char* ssid = "AgOpenGPS_net";
  const char* password = "";
  IPAddress myIP = { 192, 168, 137, 1 }; // IP of ESP32 AccessPoint, default: 192.168.137.1 to match Windows Hotspot scheme

#elif defined(STN)
  const char* ssid = "bins";
  const char* password = "binsWifiPW";
  IPAddress myIP;                       // assigned by DHCP in stn/client mode
  //const char* ssid = "other";
  //const char* password = "PW";
  //const char* ssid = "AgOpenGPS_net";
  //const char* password = "";
  //IPAddress myIP = { 192, 168, 137, 79 }; // IP of ESP32 stn/client, default: 192.168.137.79 to match Windows Hotspot scheme

#endif

#include "AsyncUDP.h"
AsyncUDP udpServer;
uint16_t udpListenPort = 8888;         // UDP port to listen for AOG PGNs
uint16_t udpSendPort   = 9999;         // UDP port to send PGN data back to AgIO/AOG
IPAddress udpDestIP;              // assigned in wifi.ino, myIP.255

#include "machine.h"
MACHINE machine;
MACHINE::States machineStates;   

//const byte numMachineOutputs = 8;
//byte machineOutputPins[numMachineOutputs] = { 12, 13, 5, 23, 19, 18, 21, 22 };
const byte numMachineOutputs = 15;
byte machineOutputPins[numMachineOutputs] = { 12, 13, 5, 23, 19, 18, 21, 22, 14, 27, 16, 17, 25, 26, 4 };

void setup() {
  delay(250);           // for ESP, to settle boot up power surges
  Serial.begin(115200);
  Serial.print("\r\n*******************************************\r\nESP32 Async UDP Machine class demo\r\n");

  setupWifi();    // blocks further execution if connecting fails
  setupUDP();
  EEPROM.begin(150);    // enough for all needed EEPROM storage (only needed for ESP)

  machine.init(100);    // 100 is address for machine EEPROM storage (uses 33 bytes)
  machine.setSectionOutputsHandler(updateSectionOutputs);
  machine.setMachineOutputsHandler(updateMachineOutputs);
  machine.setUdpReplyHandler(pgnReplies);

  setOutputPinModes();
  Serial.print("\r\n\nSetup complete\r\n*******************************************\r\n");
}



void loop() {
  //delay(10);
  yield();

  machine.watchdogCheck();

  if (Serial.available()) parseSerial();
}


void parseSerial() {
  if (Serial.read() == 'm' && Serial.available()) {
    machine.debugLevel = Serial.read() - '0';
  }
}