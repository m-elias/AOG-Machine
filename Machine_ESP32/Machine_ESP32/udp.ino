void setupUDP()
{
  if (udpServer.listen(udpListenPort)) {
    Serial.print("\r\nUDP Listening on: "); Serial.print(myIP);
    Serial.print(":"); Serial.print(udpListenPort);

    // this function is triggered asynchronously(?) by the AsyncUDP library
    udpServer.onPacket([](AsyncUDPPacket packet) {
      checkForPGNs(packet);
    }); // all the brackets and ending ; are necessary!
  }
}


// callback function for Machine class to send data back to AgIO/AOG
void pgnReplies(const uint8_t *pgnData, uint8_t len, IPAddress destIP)
{
  udpServer.writeTo(pgnData, len, destIP, udpSendPort);
}