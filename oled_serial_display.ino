// Arduino OLED Display with Serial Communication
// For Arduino MKR WiFi 1010 with SSD1306 OLED Display
// Receives text commands from computer via USB serial and displays on OLED

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}  // Wait for USB connection on MKR
  
  Serial.println("=== OLED Serial Display Starting ===");
  Serial.println("Initializing display...");
  
  // Initialize display at I2C address 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("ERROR: display.begin() failed!");
    Serial.println("Check: 1) Wiring 2) Library installed 3) I2C address");
    while (true) {
      delay(1000);
    }
  }
  
  Serial.println("Display initialized successfully!");
  
  // Show "Ready" message on startup
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Ready"));
  display.display();
  
  Serial.println("ready");
  Serial.println("=== Waiting for commands ===");
}

void loop() {
  static String line;
  
  // Read incoming serial data
  while (Serial.available()) {
    char c = Serial.read();
    
    if (c == '\n') {
      line.trim();
      Serial.print("Received command: '");
      Serial.print(line);
      Serial.println("'");
      
      // Special command: CLEAR - clears the display
      if (line.equalsIgnoreCase("CLEAR")) {
        display.clearDisplay();
        display.display();
        Serial.println("ok:cleared");
      } 
      // Any other text: display it on OLED
      else if (line.length() > 0) {
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println(line);
        display.display();
        Serial.print("ok:shown '");
        Serial.print(line);
        Serial.println("'");
      }
      
      // Clear the line buffer for next command
      line = "";
    } 
    else {
      // Accumulate characters until newline
      line += c;
    }
  }
}

