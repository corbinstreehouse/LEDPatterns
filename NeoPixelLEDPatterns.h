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


#define STRIP_PIN 2


class NeoPixelLEDPatterns : public LEDPatterns {
private:
    Adafruit_NeoPixel m_strip;
protected:
    virtual void internalShow() { m_strip.show(); }
public:
    virtual void setBrightness(uint8_t brightness) { m_strip.setBrightness(brightness); }
    uint8_t getBrightness() { return m_strip.getBrightness(); };
    NeoPixelLEDPatterns(uint32_t ledCount) : LEDPatterns(ledCount), m_strip(ledCount, STRIP_PIN, NEO_GRB + NEO_KHZ800, m_leds) {
        
    }

    virtual void begin() { m_strip.begin(); };
};


#endif /* defined(__LEDDigitalCyrWheel__NeoPixelLEDPatterns__) */
