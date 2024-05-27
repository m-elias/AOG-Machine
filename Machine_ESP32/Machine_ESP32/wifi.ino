#ifdef AP

  void setupWifi()
  {
    Serial.print((String)"\r\nStarting Soft AP (" + ssid + ")...");
    WiFi.softAPConfig(myIP, myIP, netmask);   // for some reason setting "DHCP start" causes client ESP32s to fail wifi connection
    if (!WiFi.softAP(ssid, password)) {
      Serial.print("\r\nSoft AP creation failed.");
      while(1);
    }
    printWifiDetails(WiFi.softAPIP());
  }

#elif defined(STN)

  void setupWifi()
  {
    Serial.print((String)"\r\nConnecting to SSID (" + ssid + ")...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.print("\r\nWiFi Failed...");
      delay(5000);
    }
    myIP = WiFi.localIP();
    printWifiDetails(myIP);
  }

#endif

void printWifiDetails(IPAddress _ip)
{
  Serial.print("Success!!");
  Serial.print("\r\nMy IP address: ");
  Serial.print(_ip);

  udpDestIP = _ip;
  udpDestIP[3] = 255;
  Serial.print("\r\nSending UDP data to: ");
  Serial.print(udpDestIP);
}