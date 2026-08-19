// good plan:
// get weather
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
String text = "";

ThinkInk_213_Quadcolor_AJHE5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY, EPD_SPI);


void setup() {
  Ethernet.init(17);  // CS
  Serial.begin(115200);
  while (!Serial) {
    ;
  }
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
    text = text + c;
  }

  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    
    if (showWebData) {
      display.clearBuffer();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.setTextColor(EPD_RED);
      display.setTextWrap(true);
      display.print(text);
      display.display();
      delay(15000);
    }
  
    while (true) {}
  }
}
