// good plan:
// add eInk
// get weather
// get time




// SPDX-FileCopyrightText: 2025 Liz Clark for Adafruit Industries
//
// SPDX-License-Identifier: MIT

/*
 Web client

 This sketch connects to a website (http://wifitest.adafruit.com)
 using an Adafruit Wiz5500 Ethernet Breakout

 Circuit:
 * Ethernet breakout attached to Pico pins 16, 17, 18, 19

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe, based on work by Adrian McEwen
 modified 10 June 2025
 by Liz Clark

 */


// SPDX-FileCopyrightText: 2025 Liz Clark for Adafruit Industries
//
// SPDX-License-Identifier: MIT

/***************************************************
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

#include "Adafruit_ThinkInk.h"

#define EPD_DC 7
#define EPD_CS 8
#define EPD_BUSY 3 // can set to -1 to not use a pin (will wait a fixed delay)
#define SRAM_CS 6
#define EPD_RESET 4  // can set to -1 and share with microcontroller Reset!
#define EPD_SPI &SPI // primary SPI

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address for your controller below.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

char server[] = "wifitest.adafruit.com";

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

// Variables to measure the speed
unsigned long beginMicros, endMicros;
unsigned long byteCount = 0;
bool printWebData = true;  // set to false for better speed measurement

// 2.13" Quadcolor EPD with JD79661 chipset
ThinkInk_213_Quadcolor_AJHE5 display(EPD_DC, EPD_RESET, EPD_CS, SRAM_CS, EPD_BUSY,
                                     EPD_SPI);


void setup() {
  // You can use Ethernet.init(pin) to configure the CS pin
  // Here you're using pin 10 for CS
  // SCK: 13, MISO: 12, MOSI: 11
  Ethernet.init(17);  // Pico CSn pin

  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  Serial.println("Adafruit EPD full update test in red/yellow/black/white");
  display.begin(THINKINK_QUADCOLOR);
  }

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet breakout was not found.  Check your wiring.");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // try to configure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet a second to initialize:
  delay(1000);
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");

  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.print("connected to ");
    Serial.println(client.remoteIP());
    // Make a HTTP request:
    client.println("GET /testwifi/index.html HTTP/1.1");
    client.println("Host: wifitest.adafruit.com");
    client.println("Connection: close");
    client.println();
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
  beginMicros = micros();
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  int len = client.available();
  if (len > 0) {
    byte buffer[80];
    if (len > 80) len = 80;
    client.read(buffer, len);
    if (printWebData) {
      Serial.write(buffer, len); // show in the serial monitor (slows some boards)
    }
    byteCount = byteCount + len;
  }

  // if the server's disconnected, stop the client:
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

    // do nothing forevermore:
    while (true) {
      Serial.println("Banner demo");
      display.clearBuffer();
      display.setTextSize(3);
      display.setCursor((display.width() - 144) / 2, (display.height() - 24) / 2);
      String text = "QuadColor";
      uint16_t colors[] = {EPD_BLACK, EPD_RED, EPD_YELLOW};
      
      for (int i = 0; i < text.length(); i++) {
        // Change color for every character (0: BLACK, 1: RED, 2: YELLOW, 3: BLACK, etc.)
        display.setTextColor(colors[i % 3]);
        display.print(text.charAt(i));
      }
      display.display();
      
      delay(15000);
    
      Serial.println("Color quadrant demo");
      display.clearBuffer();
      // Top-left quadrant - EPD_BLACK
      display.fillRect(0, 0, display.width() / 2, display.height() / 2, EPD_BLACK);
      // Top-right quadrant - EPD_RED  
      display.fillRect(display.width() / 2, 0, display.width() / 2, display.height() / 2, EPD_RED);
      // Bottom-left quadrant - EPD_YELLOW
      display.fillRect(0, display.height() / 2, display.width() / 2, display.height() / 2, EPD_YELLOW);
      // Bottom-right quadrant - assume you have a 4th color like EPD_WHITE or another color
      display.fillRect(display.width() / 2, display.height() / 2, display.width() / 2, display.height() / 2, EPD_WHITE);
    
      display.display();
    
      delay(15000);
    
      Serial.println("Text demo");
      // large block of text
      display.clearBuffer();
      display.setTextSize(1);
      testdrawtext(
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur "
          "adipiscing ante sed nibh tincidunt feugiat. Maecenas enim massa, "
          "fringilla sed malesuada et, malesuada sit amet turpis. Sed porttitor "
          "neque ut ante pretium vitae malesuada nunc bibendum. Nullam aliquet "
          "ultrices massa eu hendrerit. Ut sed nisi lorem. In vestibulum purus a "
          "tortor imperdiet posuere. ",
          EPD_BLACK);
      display.display();
    
      delay(15000);
    
      display.clearBuffer();
      for (int16_t i = 0; i < display.width(); i += 4) {
        display.drawLine(0, 0, i, display.height() - 1, EPD_BLACK);
      }
      for (int16_t i = 0; i < display.height(); i += 4) {
        display.drawLine(display.width() - 1, 0, 0, i, EPD_RED);
      }
      for (int16_t i = 0; i < display.width(); i += 4) {
         display.drawLine(display.width()/2, display.height()-1, i, 0, 
                          EPD_YELLOW);
      }
      
      display.display();
    
      delay(15000);
    }
  }
}


void testdrawtext(const char *text, uint16_t color) {
  display.setCursor(0, 0);
  display.setTextColor(color);
  display.setTextWrap(true);
  display.print(text);
}
