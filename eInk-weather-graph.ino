// good plan:
// add eInk
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

char server[] = "wifitest.adafruit.com";

IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

EthernetClient client;

unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
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
    client.println("GET /testwifi/index.html HTTP/1.1");
    client.println("Host: wifitest.adafruit.com");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed");
  }
  beginMicros = micros();
}

void loop() {
  int len = client.available();
  if (len > 0) {
    char c = client.read();
    if (printWebData) {
      Serial.write(c); // print to serial monitor
    }
    text = text + c;
    byteCount++;
  }

  if (!client.connected()) {
    endMicros = micros();
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    Serial.print("Received ");
    Serial.print(byteCount);
    Serial.print(" bytes in ");
    float seconds = (float)(endMicros - beginMicros) / 1000000.0;
    Serial.print(seconds, 4);
    float rate = (float)byteCount / seconds / 1000.0;
    Serial.print(", rate = ");
    Serial.print(rate);
    Serial.print(" kbytes/second");
    Serial.println();
    
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
