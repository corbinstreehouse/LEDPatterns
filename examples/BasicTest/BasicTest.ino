#include "Arduino.h"
#include "LEDPatterns.h"
#include "FastLED.h"
#include "Adafruit_NeoPixel.h"

#include "NeoPixelLEDPatterns.h"

#define STRIP_LENGTH  60

NeoPixelLEDPatterns ledPatterns(STRIP_LENGTH);

void setup() {
  
  Serial.begin(9600);
  delay(3000);
  
  ledPatterns.begin();
//  ledPatterns.setBrightness(128); // optional
  ledPatterns.flashThreeTimesWithDelay(CRGB::Red, 150);
  
  // set the first pattern to show     
  ledPatterns.setDuration(2000); // 2 seconds
  ledPatterns.setPatternColor(CRGB::Green);
  ledPatterns.setPatternType(LEDPatternTypeRotatingRainbow);

  
}

uint32_t lastUpdate = 0;
void loop() {
  if (millis() - lastUpdate > 4000) {
    lastUpdate = millis();
    int type = ledPatterns.getPatternType();
    type++;
    if (type == LEDPatternTypeMax) {
      type = LEDPatternTypeMin;
    }
    ledPatterns.setPatternType((LEDPatternType)type);
  }
  ledPatterns.show();
}
