//
//  DotStarLEDPatterns.hpp
//  LEDDigitalCyrWheel
//
//  Created by Corbin Dunn on 12/21/15 .
//
//

#ifndef DotStarLEDPatterns_hpp
#define DotStarLEDPatterns_hpp


#include "LEDPatterns.h"
#include "Adafruit_DotStar.h"

#ifndef STRIP_PIN
#define STRIP_PIN 2 // corbin, pin 2 is for my LED cyr wheel!!!
#endif

#define CLOCK_PIN 3 // corbin! make a parameter..

class DotStarLEDPatterns : public LEDPatterns {
private:
    Adafruit_DotStar m_strip;
protected:
    virtual void internalShow() { m_strip.show(); }
public:
    virtual void setBrightness(uint8_t brightness) { m_strip.setBrightness(brightness); }
    uint8_t getBrightness() { return m_strip.getBrightness(); };

    DotStarLEDPatterns(uint32_t ledCount, int stripPin = STRIP_PIN, uint8_t type = DOTSTAR_BRG) : LEDPatterns(ledCount), m_strip(ledCount, stripPin, CLOCK_PIN, type, m_leds) {
    }
    
    virtual void begin() {
#if DEBUG
        Serial.print("dotstarLEDPatterns: using strip pin:\r\n");
#endif
        m_strip.begin();
    };
};


#endif /* DotStarLEDPatterns_hpp */
