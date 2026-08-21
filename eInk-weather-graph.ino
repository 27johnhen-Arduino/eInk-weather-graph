/* circuit:
 * Raspberry Pi Pico 2 (henceforth referred to as Pico) connected by Ethernet to Raspberry Pi
 * Pico VBUS connected to Raspberry Pi 5V
 * Pico GND connected to Raspberry Pi GND
 * Pico 3V3(OUT) connected to Adafruit WIZ5500 Ethernet Co-Processor Breakout Board (henceforth referred to as Ethernet) VIN
 * Pico 3V3(OUT) connected to Adafruit 2.13" 250x122 Quad-Color eInk (henceforth referred to as eInk) VIN
 * Pico GP19 connected to Ethernet MOSI
 * Pico GP19 connected to eInk MOSI
 * Pico GP18 connected to Ethernet SCK
 * Pico GP18 connected to eInk SCK
 * Pico GND connected to Ethernet GND
 * Pico GP17 connected to Ethernet CS
 * Pico GP16 connected to Ethernet MISO
 * Pico GP16 connected to eInk MISO
 * Pico GND connected to eInk GND
 * Pico GP8 connected to eInk ECS
 * Pico GP7 connected to eInk D/C
 * Pico GP6 connected to eInk SRCS
 * Pico GP4 connected to eInk RST
 * Pico GP3 connected to eInk BUSY
 */

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

bool requireSerial = false; // if true, will not run until serial monitor connected
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
int highTempX = 0; // x position on display; based on length of number
int lowTempX = 0; // x position on display; based on length of number

ThinkInk_213_Quadcolor_AJHE5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, EPD_SPI);

void setup() {
  Serial.begin(115200);
  if (requireSerial) {
    while (!Serial) {}
  }
  Ethernet.init(17);  // CS
  display.begin(THINKINK_QUADCOLOR);
  display.cp437(true);
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
    Serial.write(c);
    text += c;
  }
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    display.clearBuffer();
    fillArrays();
    mapArrays();
    drawLines();
    drawGraph();
    drawText();
    drawName();
    display.display();
    delay(3600000); // 1 hour
    rp2040.reboot();
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
  for (int i = 0; i < 24; i++) {
    mappedTemps[i] = map(temps[i], lowTemp, highTemp, 51, 1);
    mappedPrecips[i] = map(precips[i], 0, 100, 112, 62);
  }
}

void drawLines() {
  display.fillScreen(EPD_WHITE);
  display.drawFastHLine(20, 26, 230, EPD_BLACK);
  display.drawFastHLine(20, 87, 230, EPD_BLACK);
  display.drawFastVLine(20, 1, 50, EPD_BLACK);
  display.drawFastVLine(20, 62, 50, EPD_BLACK);
  display.drawFastVLine(249, 1, 50, EPD_BLACK);
  display.drawFastVLine(249, 62, 50, EPD_BLACK);
  display.drawFastHLine(20, 51, 230, EPD_BLACK);
  display.drawFastHLine(20, 112, 230, EPD_BLACK);
  display.drawFastHLine(20, 1, 230, EPD_BLACK);
  display.drawFastHLine(20, 62, 230, EPD_BLACK);
}

void drawGraph() {
  for (int i = 0; i < 23; i++) {
    display.drawLine((i * 10 + 20), mappedTemps[i], ((i + 1) * 10 + 20), mappedTemps[i + 1], EPD_RED);
  }
  for (int i = 0; i < 23; i++) {
    display.drawLine((i * 10 + 20), mappedPrecips[i], ((i + 1) * 10 + 20), mappedPrecips[i + 1], EPD_RED);
  }
}

void drawText() {
  display.setTextSize(1);
  display.setTextColor(EPD_RED);
  for (int i = 0; i < 24; i += 2) {
    display.setCursor(i * 10 + 15, 53);
    display.print(times[i]);
  }
  for (int i = 0; i < 24; i += 2) {
    display.setCursor(i * 10 + 15, 114);
    display.print(times[i]);
  }
  if (highTemp == 0) {
    highTempX = 13;
  } else if (highTemp < 100 && highTemp > -100) {
    highTempX = 7;
  } else {
    highTempX = 1;
  }
  if (lowTemp == 0) {
    lowTempX = 13;
  } else if (lowTemp < 100 && lowTemp > -100) {
    lowTempX = 7;
  } else {
    lowTempX = 1;
  }
  display.setCursor(highTempX, 1);
  display.print(highTemp, 0);
  display.setCursor(7, 23);
  display.write(0xF8);
  display.print("F");
  display.setCursor(lowTempX, 45);
  display.print(lowTemp, 0);
  display.setCursor(1, 62);
  display.print("100");
  display.setCursor(13, 84);
  display.print("%");
  display.setCursor(13, 106);
  display.print("0");
}
void drawName() {
  display.fillRect(0, 73, 11, 49, EPD_BLACK);
  display.setTextSize(1);
  display.setTextColor(EPD_YELLOW);
  display.setCursor(1, 74);
  display.print("H");
  display.setCursor(2, 82);
  display.print("E");
  display.setCursor(3, 90);
  display.print("N");
  display.setCursor(4, 98);
  display.print("R");
  display.setCursor(5, 106);
  display.print("Y");
  display.setCursor(6, 114);
  display.print("!");
}
