// good plan:
// make graph
// get time


// if Ethernet not working sudo systemctl restart NetworkManager


#include <Adafruit_ThinkInk.h>
#include <SPI.h>
#include <Ethernet.h>

#define EPD_DC 7
#define EPD_CS 8
#define EPD_BUSY 3
#define SRAM_CS 6
#define EPD_RESET 4
#define EPD_SPI &SPI

byte mac[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

char server[] = "api.open-meteo.com";

IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

EthernetClient client;

bool printWebData = true;
bool showWebData = true;
bool requireSerial = false;
String text = "";
String times[24] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};
float temps[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int precips[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float highTemp = 0; // maximum value on graph; higher than actual highest temperature in data set
float lowTemp = 0; // minimum value on graph; lower than actual lowest temperature in data set
int mappedTemps[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // mapped from 0 - 50 for display
int mappedPrecips[24] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // mapped from 0 - 50 for display
int indexOfLastSymbol = 0;
int indexOfCurrentSymbol = 0;
int indexOfNextSymbol = 0;

ThinkInk_213_Quadcolor_AJHE5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, EPD_SPI);

void setup() {
  Serial.begin(115200);
  if (requireSerial) {
    while (!Serial) {}
  }
  Ethernet.init(17);  // CS
  display.begin(THINKINK_QUADCOLOR);
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet breakout was not found.  Check your wiring.");
      while (true) {
        delay(1);
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  delay(1000);
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");
  if (client.connect(server, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
    client.println("GET /v1/forecast?latitude=45.6216&longitude=-94.2069&hourly=temperature_2m,precipitation_probability&timezone=America%2FChicago&wind_speed_unit=mph&temperature_unit=fahrenheit&precipitation_unit=inch&forecast_hours=24 HTTP/1.1");
    client.println("Host: api.open-meteo.com");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed");
  }
}

void loop() {
  int len = client.available();
  if (len > 0) {
    char c = client.read();
    if (printWebData) {
      Serial.write(c); // print to serial monitor
    }
    text += c;
  }
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    if (showWebData) {
      display.clearBuffer();
      fillArrays();
      mapArrays();
      display.display();
      delay(15000);
    }
    while (true) {}
  }
}

void fillArrays() {
  indexOfLastSymbol = text.indexOf("[");
  for (int i = 0; i < 24; i++) {
    indexOfCurrentSymbol = text.indexOf("T", indexOfLastSymbol + 1);
    times[i] = text.substring(indexOfCurrentSymbol + 1, indexOfCurrentSymbol + 3);
    indexOfLastSymbol = indexOfCurrentSymbol;
  }
  indexOfCurrentSymbol = text.indexOf("[", indexOfLastSymbol);
  for (int i = 0; i < 23; i++) {
    indexOfNextSymbol = text.indexOf(",", indexOfCurrentSymbol + 1);
    temps[i] = text.substring(indexOfCurrentSymbol + 1, indexOfNextSymbol).toFloat();
    indexOfLastSymbol = indexOfCurrentSymbol;
    indexOfCurrentSymbol = indexOfNextSymbol;
  }
  indexOfNextSymbol = text.indexOf("]", indexOfCurrentSymbol + 1);
  temps[23] = text.substring(indexOfCurrentSymbol + 1, indexOfNextSymbol).toFloat();
  indexOfLastSymbol = indexOfCurrentSymbol;
  indexOfCurrentSymbol = text.indexOf("[", indexOfLastSymbol);
  for (int i = 0; i < 23; i++) {
    indexOfNextSymbol = text.indexOf(",", indexOfCurrentSymbol + 1);
    precips[i] = text.substring(indexOfCurrentSymbol + 1, indexOfNextSymbol).toInt();
    indexOfLastSymbol = indexOfCurrentSymbol;
    indexOfCurrentSymbol = indexOfNextSymbol;
  }
  indexOfNextSymbol = text.indexOf("]", indexOfCurrentSymbol + 1);
  precips[23] = text.substring(indexOfCurrentSymbol + 1, indexOfNextSymbol).toInt();
}

void mapArrays() {
  highTemp = -7210111011412133;
  lowTemp = 7210111011412133;
  for (int i = 0; i < 23; i++) {
    if (temps[i] > highTemp) {
      highTemp = temps [i];
    }
    if (temps[i] < lowTemp) {
      lowTemp = temps [i];
    }
  }
  highTemp *= 10;
  int highTempInt = highTemp;
  while (highTempInt % 100 != 0) {
    highTempInt++;
  }
  highTemp = highTempInt;
  highTemp /= 10;
  lowTemp *= 10;
  int lowTempInt = lowTemp;
  while (lowTempInt % 100 != 0) {
    lowTempInt--;
  }
  lowTemp = lowTempInt;
  lowTemp /= 10;
  for (int i = 0; i < 23; i++) {
    mappedTemps[i] = map(temps[i], lowTemp, highTemp, 0, 50);
    mappedPrecips[i] = map(precips[i], 0, 100, 0, 50);
  }
}
