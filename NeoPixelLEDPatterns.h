//
//  NeoPixelLEDPatterns.h
//  LEDDigitalCyrWheel
//
//  Created by corbin dunn on 5/31/14.
//
//

#ifndef __LEDDigitalCyrWheel__NeoPixelLEDPatterns__
#define __LEDDigitalCyrWheel__NeoPixelLEDPatterns__

#include "LEDPatterns.h"
#include "Adafruit_NeoPixel.h"

#ifndef STRIP_PIN
    #define STRIP_PIN 14 // corbin, pin 2 is for my LED cyr wheel!!!
#endif

class NeoPixelLEDPatterns : public LEDPatterns {
private:
    Adafruit_NeoPixel m_strip;
protected:
    virtual void internalShow() { m_strip.show(); }
public:
    virtual void setBrightness(uint8_t brightness) { m_strip.setBrightness(brightness); }
    uint8_t getBrightness() { return m_strip.getBrightness(); };
    NeoPixelLEDPatterns(uint32_t ledCount, int stripPin = STRIP_PIN, uint8_t type = NEO_GRB + NEO_KHZ800) : LEDPatterns(ledCount), m_strip(ledCount, stripPin, type, m_leds) {
    }

    virtual void begin() {
#if DEBUG
        Serial.print("NeoPixelLEDPatterns: using strip pin:");
        Serial.println(m_strip.getPin());
#endif
        m_strip.begin();
    };
};


#endif /* defined(__LEDDigitalCyrWheel__NeoPixelLEDPatterns__) */
