//
//  FastLEDPatterns.h
//  LEDDigitalCyrWheel
//
//  Created by Corbin Dunn on 6/4/14 .
//
//

#ifndef LEDDigitalCyrWheel_FastLEDPatterns_h
#define LEDDigitalCyrWheel_FastLEDPatterns_h

#include "LEDPatterns.h"
#include "FastLED.h"

#define STRIP_PIN 2

class FastLEDPatterns : public LEDPatterns {
protected:
    virtual void internalShow() {
        FastLED.show();
    }
public:
    virtual void setBrightness(uint8_t brightness) {  FastLED.setBrightness(brightness); }
    uint8_t getBrightness() { return FastLED.getBrightness(); };
    FastLEDPatterns(uint32_t ledCount, int stripPin = STRIP_PIN) : LEDPatterns(ledCount) {
        // Change the parameters below as needed
        FastLED.addLeds<WS2812, STRIP_PIN, GRB>(m_leds, ledCount).setCorrection(TypicalLEDStrip);
    }
    virtual void begin() {
    };
};


#endif
