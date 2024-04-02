#include <stdint.h>
/*
  My attempt at putting the Machine/Section control code all into a class
    by Matt Elias Feb 2024

  Supports control of regular Arduino pins as well as PCA9555 I2C port expanders as used on the AiO v5.0a Proto
    - two init() functions, one for Arduino pins and one for PCA9555/v5.0a control
    - currently only properly supports 24 output pins
      - to use all 64 would probably need a mode to only do sections and not other functions like hyd lift, trams, geo stop, etc
    - 


  To to:
    - add section switch code, maybe in it's own class?
    - add GPS speed output options/code
    - maybe split up machine functions and sections into seperate classes or seperate output groups
    - support 64 section only init/functions (no other machine controls)
    - split Arduino pin inversion from PCA pin inversion or add setting to either match or invert PCA from Arduino pins
    
*/

#ifndef MACHINE_H
#define MACHINE_H

#include "EEPROM.h"
#include "IPAddress.h"
#include <stdint.h>
#include "elapsedMillis.h"
#ifdef CLSPCA9555_H_
  #include "clsPCA9555.h"
#endif

class MACHINE
{
private:
  int16_t eeAddr = -1;                            // -1 defaults to no EEPROM saving/loading
  #define EE_IDENT 11                             // change to force EE to reset to defaults
  bool eeLoadedAtStartup;

  struct Config {
    uint8_t raiseTime = 2;
    uint8_t lowerTime = 4;
    uint8_t hydLiftEnable = false;
    uint8_t isPinActiveHigh = 0;          // if zero, active low (default)
  
    uint8_t user1;                        //user defined values set in machine tab
    uint8_t user2;
    uint8_t user3;
    uint8_t user4;

    uint8_t pinFunction[1 + 24] = { 0,1,2,3,4,5,6,7,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
  };  Config config;   // 33 bytes
  /* pin function numbers as below, assigned to pinFunction[]
  1-16   Section 1-16
  17,18  Hyd Up, Hyd Down,
  19,20  Tramline Right, Left
  21     Geo Stop  */

  // 24 is max, function selectable, pins currently supported in AoG
  // all the '1 +' are to eliminate 0 indexing so that outputPinNumber[1] is pin 1 (the first pin)
  // all variables store states as 1 - ON & 0 - OFF and outputs are inverted according to isPinActiveHigh only at digitalWrite

  uint8_t numOutputPins = 0;                      // 0 defaults to no direct Arduino pin control
  const uint8_t maxOutputPins = 24;               // 24 pins can be configured in AoG (Machine Pin Config PGN), 64 sections currently the max supported by AoG
  uint8_t* outputPinNumbers;                      // store Arduino output pin numbers
  bool forceOutputUpdate;

#ifdef CLSPCA9555_H_
  PCA9555* pcaOutputs = NULL;
  uint8_t* pcaOutputPinNumbers;                  // the AiO v5.0a uses 8 PCA9555 IO for outputs
#endif

  struct States {
    uint8_t uTurn;                // 
    uint8_t gpsSpeed;             // x0.1 to get real speed in km/hr
    uint8_t hydLift;              // 0 - off, 1 - down, 2 - up
    uint8_t tramline;             // bit0: right, bit1: left
    uint8_t geoStop;              // 0 - inside boundary, 1 - outside boundary

    // read value from Machine Data and set 1 or zero according to list, 21 different function types
    bool functions[1 + 21] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

    // store state of up to 64 sections, from AOG, but only currently using section 1-16 as per the pin functions above
    bool sections[1 + 64];
  }; States states;

  const uint8_t LOOP_TIME = 200;                  // 5hz
  const uint16_t watchdogTimeoutPeriod = 4000;    // ms, originally was 20 update cycles (4 secs)
  const uint16_t watchdogAlertPeriod = 1000;      // ms, how long after UDP comms lost to alert
  bool watchdogAlertTriggered;

  const String functionNames[1 + 21] = { "",
      "S01", "S02", "S03", "S04", "S05", "S06", "S07", "S08",
      "S09", "S10", "S11", "S12", "S13", "S14", "S15", "S16",
      "HydUp", "HydDn", "TramR", "TramL", "GeoStop"
  };

public:

  bool isInit;
  elapsedMillis watchdogTimer;

  uint8_t debugLevel = 3;
    // 0 - debug prints OFF
    // 1 - alerts/errors only
    // 2 - init info
    // 3 - config PGN announcements & data (only sent when changing settings in AOG)
    // 4 - all PGN announcements
    // 5 - all PGN data

  uint8_t debugMask = B0000000;     // not yet implemented
    // bit 0: alerts/errors
    // bit 1: init info
    // bit 2: 
    // bit 3: 64 Sections PGN
    // bit 4: Section Dimensions PGN
    // bit 5: Pin Config PGN
    // bit 6: Machine Config PGN
    // bit 7: Machine Data PGN



  MACHINE(void) {}
  ~MACHINE(void) {}

  // init function for regular "Arduino" pins
  void init(uint8_t* _outputPinNumbers, uint8_t _numOutputPins = 0, int16_t _eeAddr = -1, const uint8_t _eeSize = 33)
  {
    numOutputPins = min(_numOutputPins, maxOutputPins);   // limit to maxOutputPins (24 or 64)
    
    //Serial.print("\r\nnumOutputPins:"); Serial.print(numOutputPins);
    eeAddr = _eeAddr;
    loadFromEeprom();

    outputPinNumbers = _outputPinNumbers;

    for (uint8_t i = 0; i < numOutputPins; i++) {
      pinMode(outputPinNumbers[i], OUTPUT);
      digitalWrite(outputPinNumbers[i], !config.isPinActiveHigh);
    }
    isInit = true;
  }

#ifdef CLSPCA9555_H_
  // init function for PCA9555 IO expander pins (AiO v5.0a)
  void init(PCA9555* _pcaOutputs, uint8_t* _outputPins, int16_t _eeAddr = -1, const uint8_t _eeSize = 33)
  {
    pcaOutputs = _pcaOutputs;
    eeAddr = _eeAddr;
    loadFromEeprom();

    pcaOutputPinNumbers = _outputPins;

    for (uint8_t i = 0; i < 8; i++){
      //pcaOutputPinNumbers[i+1] = _outputPins[i];
      //pcaOutputs->pinMode(pcaOutputPinNumbers[i], OUTPUT);                         // calling digitalWrite already sets the pins to OUTPUT
      pcaOutputs->digitalWrite(pcaOutputPinNumbers[i], config.isPinActiveHigh);      // PCA9555 outputs on AiO v5.0a are inverted from other test LEDs
    }
    isInit = true;
  }
#endif

  void watchdogCheck()
  {
    if (!isInit) return;

    if (watchdogTimer > watchdogTimeoutPeriod)    // watchdogTimer reset with Machine Data PGN, should be 64 Section instead or both?
    {
      if (debugLevel > 0) Serial.print("\r\n*** UDP Machine Comms lost for 4s, setting all outputs OFF! ***");
      for (uint8_t i = 1; i <= 21; i++) {
        states.functions[i] = 0;            // set all functions OFF
      }

      updateOutputPins();
      watchdogTimer = 0;            // only output timed out OFF every watchdogTimeoutPeriod
    }
    else if (watchdogTimer > watchdogAlertPeriod)
    {
      if (debugLevel > 0 && !watchdogAlertTriggered) {
        Serial.print("\r\n** UDP Machine Comms lost for 1s **");
        watchdogAlertTriggered = true;
      }
    }
  }

  // update triggered by PGN from AOG for quicker section response, watchdogCheck() looks for comms timeout
  // updating outputs from PGN should be slightly quicker response then waiting for old update loop to trigger, at times the delay was almost 200ms
  void updateStates()
  {
    static uint8_t lastTrigger;          // "static" means this var is accessible only to this function but its value persists (it's not destroyed)
    static uint8_t raiseTimer = 0;
    static uint8_t lowerTimer = 0;
    bool isRaise, isLower;

    bool prevFunctions[sizeof(states.functions)];// = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
    memcpy(prevFunctions, states.functions, sizeof(states.functions));

    if (config.hydLiftEnable)       // hydraulic lift
    {
      if (states.hydLift != lastTrigger && (states.hydLift == 1 || states.hydLift == 2))
      {
        lastTrigger = states.hydLift;
        lowerTimer = 0;
        raiseTimer = 0;

        switch (states.hydLift) {
          case 1:   //lower
            lowerTimer = config.lowerTime * 5;      // secs * 5 update cycles per second (5hz)
            break;
          case 2:   //raise
            raiseTimer = config.raiseTime * 5;
            break;
        }
      }

      if (raiseTimer) {
        raiseTimer--;
        lowerTimer = 0;
      }
      if (lowerTimer) lowerTimer--;

      //if anything wrong, shut off hydraulics, reset last
      if (states.hydLift != 1 && states.hydLift != 2) { //|| gpsSpeed < 2)
        lowerTimer = 0;
        raiseTimer = 0;
        lastTrigger = 0;
      }

      isLower = isRaise = false;          // set both OFF
      if (lowerTimer) isLower = true;     // if lower is active, set ON
      if (raiseTimer) isRaise = true;     // if raise is active, set ON
    }
    else {      // hyd lift is disabled, make sure any hyd lift pins are OFF
      isLower = false;
      isRaise = false;
    }

    /*float speedPulse = gpsSpeed * 36.1111;
    if (speedPulse > 2.0) {                    // less then 2 hz (0.055 km/hr) doesn't work well with tone()
      tone(speedPulsePin, uint16_t(speedPulse));
    } else {
      noTone(speedPulsePin);
    }*/

    //Load the current output states for section 1-16 only from 64 Sections PGN
    for (uint8_t i = 1; i <= 16; i++) {
        states.functions[i] = states.sections[i];    // copy from 64 Sections PGN
    }

    // Hydraulics
    states.functions[17] = isLower;
    states.functions[18] = isRaise;

    //Tram
    states.functions[19] = bitRead(states.tramline, 0); //right
    states.functions[20] = bitRead(states.tramline, 1); //left

    //GeoStop
    states.functions[21] = states.geoStop;

    if (forceOutputUpdate || memcmp(states.functions, prevFunctions, sizeof(states.functions))) {
      //Serial.print("\r\nOutputs updated");
      updateOutputPins();
    }

    if (debugLevel > 0 && watchdogAlertTriggered) { //watchdogTimer > watchdogAlertPeriod) {
      Serial.print("\r\n*** UDP Machine Comms resumed ***");
    }
    watchdogTimer = 0;   //reset watchdog timer
    watchdogAlertTriggered = false;

    // *** Sending PGN_237 isn't necessary/doesn't do anything?

    // generic "from machine" PGN template
    //uint8_t PGN_237[14] = { 0x80, 0x81, 0x7B, 237, 8, 1, 2, 3, 4, 0, 0, 0, 0, 0xCC };

    //checksum
    /*int16_t CK_A = 0;
    for (uint8_t i = 2; i < sizeof(PGN_237) - 1; i++) {
      CK_A = (CK_A + PGN_237[i]);
    }
    PGN_237[sizeof(PGN_237) - 1] = CK_A;*/

    //off to AOG
    //sendUdp(PGN_237, sizeof(PGN_237), _dIP, _dPort, _udp);
    //_udp->beginPacket(_dIP, _dPort);
    //_udp->write(PGN_237, sizeof(PGN_237));
    //_udp->endPacket();
  }


  void updateOutputPins()
  {
    // set pins according to states.functions unless watchdog has timed out, then set to !isPinActiveHigh

    if (numOutputPins > 0)
    {
      //if (debugLevel > 3) Serial.print("\r\nPin outputs ");
      for (uint8_t i = 1; i <= numOutputPins; i++) {
        digitalWrite(outputPinNumbers[i - 1], states.functions[config.pinFunction[i]] == config.isPinActiveHigh);                     // ==, XOR
        //if (debugLevel > 3) Serial.print(i); Serial.print(":"); Serial.print(states.functions[config.pinFunction[i]] == config.isPinActiveHigh); Serial.print(" ");
      }
    }

#ifdef CLSPCA9555_H_
    if (pcaOutputs != NULL)
    {
      Serial.print("\r\nPCA outputs ");
      for (uint8_t i = 1; i <= 8; i++) {       // AiO v5.0a has 8 PCA9555 outputs
        if (config.pinFunction[i] > 0) {
          pcaOutputs->digitalWrite(pcaOutputPinNumbers[i - 1], !(states.functions[config.pinFunction[i]] == config.isPinActiveHigh));   // NXOR
          Serial.print(i); Serial.print(":"); Serial.print(!(states.functions[config.pinFunction[i]] == config.isPinActiveHigh)); Serial.print(" ");
        }
      }
    }
#endif
    /*Serial.println();
    for (byte i = 0; i < 24; i++){
      Serial.printf("%2i ", config.pinFunction[i]);
    }
    Serial.println();
    for (byte i = 0; i < 24; i++){
      Serial.printf("%2i ", (config.pinFunction[i] > 0 ? states.functions[config.pinFunction[i]] : -1));
    }
    Serial.println();
    for (byte i = 0; i < 22; i++){
      Serial.printf("%2i ", states.functions[i]);
    }*/
    //Serial << "\r\noutput update delay: " << updateDelayTimer;
    //if (config.pinFunction[0]) digitalWrite(4, states.functions[config.pinFunction[0]]);
    forceOutputUpdate = false;
  }

  // ***************************************************************************************************************************************************
  // ****************************************************** PGN PARSING ********************************************************************************
  // ***************************************************************************************************************************************************
  bool parsePGN(uint8_t *pgnData, uint8_t len)
  {
    if (len < 5) return false;
    // should check CRC here too???
    if (pgnData[0] != 0x80 || pgnData[1] != 0x81 || pgnData[2] != 0x7F) return false;    // skip the rest if the first three bytes are NOT AoG headers


    if (pgnData[3] == 229 && len == 16)                    // 0xE5 (229) - 64 Section Data, len: 16
    {                                                      // use this instead of relayLo/Hi from other PGNs because it works for zones/groups too
      
      if (debugLevel > 3) { Serial.print("\r\n0x"); Serial.print(pgnData[3], HEX); Serial.print(" ("); Serial.print(pgnData[3]); Serial.print(") - "); }
      if (debugLevel > 3) Serial.print("64 Section Data");

      if (debugLevel > 3) Serial.println();
      for (uint8_t j = 0; j < 8; j++) {
        if (debugLevel > 3){ Serial.print(j + 1); Serial.print(":"); }
        for (uint8_t i = 0; i < 8; i++) {
          states.sections[1 + i + j * 8] = bitRead(pgnData[5 + j], i);
          if (debugLevel > 3) Serial.print(states.sections[1 + i + j * 8]);
        }
        if (debugLevel > 3) Serial.print(" ");
      }

      if (debugLevel > 3) Serial.println(); 
      updateStates();
      return true;
    }


    else if (pgnData[3] == 235 && len == 39)               // 0xEB (235) - Section Dimensions, len: 39
    {
      if (debugLevel > 2) { Serial.print("\r\n0x"); Serial.print(pgnData[3], HEX); Serial.print(" ("); Serial.print(pgnData[3]); Serial.print(") - "); }
      if (debugLevel > 2) Serial.print("Section Dimensions");
      
      // parse section dims here
      
      if (debugLevel > 2) Serial.println(); 
      return true;
    }


    else if (pgnData[3] == 236 && len == 30)               // 0xEC (236) - Machine Pin Config, len: 30
    {
      if (debugLevel > 2) { Serial.print("\r\n0x"); Serial.print(pgnData[3], HEX); Serial.print(" ("); Serial.print(pgnData[3]); Serial.print(") - "); }
      if (debugLevel > 2) Serial.print("Machine Pin Config");
      for (uint8_t i = 5; i < min(len - 1, uint8_t(sizeof(config.pinFunction) + 5)); i++) {
        config.pinFunction[i - 4] = pgnData[i];
      }
      if (debugLevel > 2) printPinConfig();
      saveToEeprom();
      forceOutputUpdate = true;

      if (debugLevel > 2) Serial.println();
      return true;
    }


    else if (pgnData[3] == 238 && len == 14)                // 0xEE (238) - Machine Config, len: 14
    {
      if (debugLevel > 2) { Serial.print("\r\n0x"); Serial.print(pgnData[3], HEX); Serial.print(" ("); Serial.print(pgnData[3]); Serial.print(") - "); }
      if (debugLevel > 2) Serial.print("Machine Config");
      config.raiseTime = pgnData[5];
      config.lowerTime = pgnData[6];
      //config.hydLiftEnable = pgnData[7];    // not used?

      uint8_t set0 = pgnData[8];  // setting0
                                  // bit 0: relayActiveHigh
                                  // bit 1: hydLiftEnable
      config.isPinActiveHigh = bitRead(set0, 0) ? 1 : 0;
      config.hydLiftEnable   = bitRead(set0, 1) ? 1 : 0;

      config.user1 = pgnData[9];
      config.user2 = pgnData[10];
      config.user3 = pgnData[11];
      config.user4 = pgnData[12];

      //Serial << "\r\n- set0: " << set0 << " " << (bitRead(set0, 3) ? 1 : 0) << (bitRead(set0, 2) ? 1 : 0) << (bitRead(set0, 1) ? 1 : 0) << (bitRead(set0, 0) ? 1 : 0);
      if (debugLevel > 2) printConfig();
      saveToEeprom();
      forceOutputUpdate = true;
      //rebootFunc();    // from old code, is there any reason to reboot?

      if (debugLevel > 2) Serial.println();
      return true;
    }


    else if (pgnData[3] == 239 && len == 14)                // 0xEF (239) - Machine Data, len: 14
    {
      if (debugLevel > 3) { Serial.print("\r\n0x"); Serial.print(pgnData[3], HEX); Serial.print(" ("); Serial.print(pgnData[3]); Serial.print(") - "); }
      if (debugLevel > 3) Serial.print("Machine Data");
      
      states.uTurn = pgnData[5];

      //gpsSpeed = (float)pgnData[6] * 0.1;     // gpsSpeed is 10x actual speed
      states.gpsSpeed = pgnData[6];
      //speedPulseUpdate();

      states.hydLift = pgnData[7];
      states.tramline = pgnData[8];  // bit 0 is right bit 1 is left
      states.geoStop = (pgnData[9] > 0 ? 1 : 0);

      //relayLo = pgnData[11];  // use 64 Sections PGN data instead
      //relayHi = pgnData[12];

      if (debugLevel > 4) {
        Serial.print("\r\nuTurn: "); Serial.print(states.uTurn);
        Serial.print("\r\ngpsSpeed: "); Serial.print(pgnData[6]); Serial.print(" > "); Serial.print(states.gpsSpeed);
        Serial.print("\r\nhydLift(1:D 2:U): "); Serial.print(states.hydLift); Serial.print((states.hydLift == 1 ? ":DOWN" : states.hydLift == 2 ? ":UP" : ":OFF"));
        Serial.print("\r\ntramline(1:R 2:L): "); Serial.print(states.tramline); Serial.print((states.tramline == 1 ? ":RIGHT" : states.tramline == 2 ? ":LEFT" : ":OFF"));
        Serial.print("\r\ngeoStop(0:OK 1:STOP): "); Serial.print(states.geoStop); Serial.print((states.geoStop == 1 ? ":STOP!!!" : ":GOOD"));
        Serial.print("\r\nsec 1-8: "); printBinaryByteLSB(pgnData[11], 8);
        Serial.print(" sec 9-16: "); printBinaryByteLSB(pgnData[12], 8);
      } else if (debugLevel > 3) {
        Serial.print("\r\n0:"); printBinaryByteLSB(pgnData[11], 8);
        Serial.print(" 1:"); printBinaryByteLSB(pgnData[12], 8);
      }

      //updateStates();   // 64 Section PGN comes after Machine Data, so states/outputs are updated there
      if (debugLevel > 3) Serial.println();
      return true;
    }

    else
    {
      return false;   // no matching PGN, return false for further PGN processing
    }

    return false;     // should never reach this
  }


  // ***************************************************************************************************************************************************
  // ****************************************************** OTHER FUNCTIONS*****************************************************************************
  // ***************************************************************************************************************************************************
  void loadFromEeprom()
  {
    if (eeLoadedAtStartup) return;
    if (eeAddr < 0) return;

    uint8_t EEread;
    EEPROM.get(eeAddr + 0, EEread);        // read version identifier
    if (EEread != EE_IDENT) {              // check on first start and write EEPROM
      EEPROM.put(eeAddr + 0, EE_IDENT);
      EEPROM.put(eeAddr + 2, config);      // +2 to leave room for EE_IDENT
      Serial.print("\r\n\n* Machine config reset to default (new EEPROM version) *");
    } else {
      EEPROM.get(eeAddr + 2, config);
      Serial.print("\r\n\nMachine config loaded from EEPROM");
    }
    printConfig();
    printPinConfig();
    eeLoadedAtStartup = true;
  }

  void saveToEeprom()
  {
    if (eeAddr < 0) return;
    EEPROM.put(eeAddr + 2, config);
    if (debugLevel > 1) Serial.print("\r\nNew Machine config saved to EEPROM");
  }

  void printConfig()
  {
    Serial.print("\r\n- raiseTime: "); Serial.print(config.raiseTime);
    Serial.print("\r\n- lowerTime: "); Serial.print(config.lowerTime);
    Serial.print("\r\n- relayActiveHigh: "); Serial.print(config.isPinActiveHigh);
    Serial.print("\r\n- hydLiftEnable: "); Serial.print(config.hydLiftEnable);
    Serial.print("\r\n- user1: "); Serial.print(config.user1);
    Serial.print("\r\n- user2: "); Serial.print(config.user2);
    Serial.print("\r\n- user3: "); Serial.print(config.user3);
    Serial.print("\r\n- user4: "); Serial.print(config.user4);
  }

  void printPinConfig()
  {
    for (uint8_t i = 1; i < uint8_t(sizeof(config.pinFunction)); i++) {
      Serial.print("\r\n- Pin ");
      Serial.print((i < 10 ? " " : ""));
      Serial.print(i); Serial.print(": ");
      Serial.print((config.pinFunction[i] < 10 ? " " : ""));
      Serial.print(config.pinFunction[i]);
      Serial.print(" ");
      Serial.print(functionNames[config.pinFunction[i]]);
    }
  }

  

  void printBinaryByteLSB(uint8_t var, uint8_t len) {
    for (uint8_t bit = 0; bit < len; bit++){
      Serial.print(bitRead(var, bit));
    }
  }

  void calculateAndSetCRC(uint8_t myMessage[], uint8_t myLen) {
    if (myLen <= 2 ) return;

    uint8_t crc = 0;
    for (byte i = 2; i < myLen - 1; i++) {
      crc = (crc + myMessage[i]);
    }
    myMessage[myLen - 1] = crc;
  }


};
#endif