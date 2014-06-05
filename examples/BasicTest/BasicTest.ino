#include "Arduino.h"
#include "LEDPatterns.h"
#include "FastLED.h"

#define STRIP_LENGTH  60
#define STRIP_PIN 2

// Pick your style. NeoPixel, requires the corbinstreehouse library and teensy due to my changes to it
#define USE_ADAFRUIT 0
#define USE_OCTO 0
#define USE_FAST_LED 1

#if USE_ADAFRUIT
#include "Adafruit_NeoPixel.h"
#include "NeoPixelLEDPatterns.h"
NeoPixelLEDPatterns ledPatterns(STRIP_LENGTH, STRIP_PIN);
#endif

#if USE_OCTO
// Oco style
#include "OctoWS2811.h"
#include "OctoWS2811LEDPatterns.h"
OctoWS2811LEDPatterns ledPatterns(STRIP_LENGTH, STRIP_PIN);
#endif

#if USE_FAST_LED
#include "FastLED.h"
#include "FastLEDPatterns.h"
FastLEDPatterns ledPatterns(STRIP_LENGTH, STRIP_PIN);
#endif

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
