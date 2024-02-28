void EthernetStart()
{
  // start the Ethernet connection:
  Serial.println("Initializing ethernet with static IP address");
  
  // try to congifure using IP:
  Ethernet.begin(mac, 0);          // Start Ethernet with IP 0.0.0.0
  
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) 
  {
    Serial.println("Ethernet shield was not found. GPS via USB only.");
    return;
  }
  
  /* linkStatus doesn't work see
  https://forum.pjrc.com/index.php?threads/teensy-4-1-nativeethernet-link-status.71143/
  https://forum.pjrc.com/index.php?threads/teensy-4-1-ethernet-link-status-wont-connect.68083/
  */
  /*if (Ethernet.linkStatus() == LinkOFF) 
  {
    Serial.println("Ethernet cable is not connected - Who cares we will start ethernet anyway.");
  }*/
  
  Ethernet.setLocalIP(myip);  // Change IP address to IP set by user
  //Serial.println("\r\nEthernet status OK"); // what does this help?
  Serial.print("IP set Manually: ");
  Serial.println(Ethernet.localIP());

  Ethernet_running = true;

  Serial.print("\r\nEthernet IP of module: "); Serial.println(Ethernet.localIP());
  Serial.print("Ethernet PGN broadcast IP: "); Serial.println(PGN_BROADCAST_IP);
  Serial.print("All data sending to port: "); Serial.println(DEST_PORT);

  // init UPD Port sending to AOG
  /*if (Eth_udpPAOGI.begin(portMy))
  {
    Serial.print("Ethernet GPS UDP sending from port: ");
    Serial.println(portMy);
  }*/

  // init UPD Port getting NTRIP from AOG
  /*if (Eth_udpNtrip.begin(AOGNtripPort)) // AOGNtripPort
  {
    Serial.print("Ethernet NTRIP UDP listening to port: ");
    Serial.println(AOGNtripPort);
  }*/

  // init UPD Port getting PGN data from AOG (autosteer, machine, IP settings, etc)
  if (Eth_PGNs.begin(AOG_PGN_PORT))
  {
    Serial.print("Ethernet PGN UDP listening to port: ");
    Serial.println(AOG_PGN_PORT);
  }
}
