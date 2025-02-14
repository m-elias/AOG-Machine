/*

  Parse PGNs sent from AgIO - sent to port 8888
  a lot of this code taken from v4.x AiO I2C firmware

*/

//#define UDP_MAX_PACKET_SIZE 40         // Buffer For Receiving 8888 UDP PGN Data
uint32_t pgn254Time, pgn254MaxDelay, pgn254AveDelay, pgn254MinDelay = 99999;

void checkForPGNs(AsyncUDPPacket packet)
{
  if (packet.remotePort() != 9999 || packet.length() < 5) return;  //make sure from AgIO

  if (packet.data()[0] != 0x80 || packet.data()[1] != 0x81 || packet.data()[2] != 0x7F) return;  // verify first 3 PGN header bytes



  #ifdef MACHINE_H
    // 0xE5 (229) - 64 Section Data
    // 0xEB (235) - Section Dimensions
    // 0xEC (236) - Machine Pin Config
    // 0xEE (238) - Machine Config
    // 0xEF (239) - Machine Data
    if (machine.parsePGN(packet.data(), packet.length(), packet.remoteIP(), myIP))    // look for Machine PGNs, return TRUE if machine specific PGN was found
    {
      return;   // abort further PGN processing if machine specific PGN as received/parsed
    }
  #endif



  if (packet.data()[3] == 100 && packet.length() == 30)         // 0x64 (100) - Corrected Position
  {
    //printPgnAnnoucement(packet, (char*)"Corrected Position");
    return;                    // no other processing needed
  }



  if (packet.data()[3] == 200 && packet.length() == 9)          // 0xC8 (200) - Hello from AgIO
  {
    printPgnAnnoucement(packet, (char*)"Hello from AgIO");
    return;
  } // 0xC8 (200) - Hello from AgIO



  if (packet.data()[3] == 201 && packet.length() == 11)         // 0xC9 (201) - Subnet Change
  {
    printPgnAnnoucement(packet, (char*)"Subnet Change");
    if (packet.data()[4] == 5 && packet.data()[5] == 201 && packet.data()[6] == 201)
    {
      Serial.print("\r\n- IP changed from "); Serial.print(myIP);
      myIP[0] = packet.data()[7];
      myIP[1] = packet.data()[8];
      myIP[2] = packet.data()[9];

      Serial.print(" to "); Serial.print(myIP);

      //Serial.print("\r\n- Saving to EEPROM and restarting Teensy");
      //UDP.SaveModuleIP();  //save in EEPROM and restart
      delay(10);
      // reboot ESP here?
    }
    return; // no other processing needed
  }  // 0xC9 (201) - Subnet Change



  if (packet.data()[3] == 202 && packet.length() == 9)          // 0xCA (202) - Scan Request
  {
    printPgnAnnoucement(packet, (char*)"Scan Request");
    Serial.print("\r\nAgIO   "); Serial.print(packet.remoteIP());
    Serial.print(":"); Serial.print(packet.remotePort());
    Serial.print("\r\nModule "); Serial.print(myIP);   // packet.localIP() returns the dest IP of 255.255.255.255.255 for Scan Request
    Serial.print(":"); Serial.print(packet.localPort());
    return;
  } // 0xCA (202) - Scan Request



  if (packet.data()[3] == 251 && packet.length() == 14)         // 0xFB (251) - SteerConfig
  {
    //printPgnAnnoucement(packet, (char*)"Steer Config");
    return; // no other processing needed
  }  // 0xFB (251) - SteerConfig



  if (packet.data()[3] == 252 && packet.length() == 14)         // 0xFC (252) - Steer Settings
  {
    //printPgnAnnoucement(packet, (char*)"Steer Settings");
    return; // no other processing needed
  }  // 0xFC (252) - Steer Settings



  if (packet.data()[3] == 254 && packet.length() == 14)        // 0xFE (254) - Steer Data (sent at GPS freq, ie 10hz (100ms))
  {
    //printPgnAnnoucement(packet, (char*)"Steer Data");
    return; // no other processing needed
  }  // 0xFE (254) - Steer Data



  printPgnAnnoucement(packet, (char*)"Unprocessed/unrecognized PGN");
}

void printPgnAnnoucement(AsyncUDPPacket packet, char* _pgnName)
{
  printPgnAnnoucement(packet.data(), packet.length(), _pgnName);
}

void printPgnAnnoucement(uint8_t* _data, uint8_t _len, char* _pgnName)
{
  // One line PGN data display
  Serial.print("\r\n0x"); Serial.print(_data[3], HEX);
  Serial.print("("); Serial.print(_data[3]); Serial.print(")-");
  Serial.print(_pgnName);
  for (uint8_t i = strlen(_pgnName); i < 20; i++) Serial.print(" ");  // to align PGN name length for PGN data bytes dump
  Serial.printf(" %2i Data>", _len);
  //Serial.print(millis());

  //Serial.print(" Data: ");
  for (byte i = 4; i < _len - 1; i++) {      // -1 skips printing CRC
    Serial.printf("%3i", _data[i]); Serial.print(" ");    // aligns all data bytes using empty/leading whitespace
    //Serial.print(packet.data()[i]); Serial.print(" ");          // no data byte alignment
  }
  
  // Two line PGN data display
  /*Serial.print("\r\n0x"); Serial.print(_data[3], HEX);
  Serial.print(" ("); Serial.print(_data[3]); Serial.print(") - ");
  Serial.print(_pgnName); Serial.print(", "); Serial.print(_len); Serial.print(" bytes ");
  Serial.print(millis());

  Serial.print("\r\nData: ");
  for (byte i = 0; i < _len; i++) {
    Serial.print(_data[i]); Serial.print(" ");
  }
  Serial.print(" Length: ");
  Serial.print(_len);

  Serial.println();*/
}
