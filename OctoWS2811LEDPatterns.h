//
//  OctoWS2811LEDPatterns.h
//  LEDDigitalCyrWheel
//
//  Created by Corbin Dunn on 6/4/14 .
//
//

#ifndef LEDDigitalCyrWheel_OctoWS2811LEDPatterns_h
#define LEDDigitalCyrWheel_OctoWS2811LEDPatterns_h

#include "LEDPatterns.h"
//#include "OctoWS2811.h" // For f's sake, we can't include this because it doesn't have a header guard! The client has to include it first..how sad.

#ifndef STRIP_PIN
    #define STRIP_PIN 2
#endif

class OctoWS2811WithMemory : public OctoWS2811 {
public:
    OctoWS2811WithMemory(uint32_t numPerStrip, uint8_t config = WS2811_GRB) : OctoWS2811(numPerStrip, NULL, NULL, config) {
        frameBuffer = malloc(sizeof(int) * numPerStrip * 6);
    };
    ~OctoWS2811WithMemory() {
        free(frameBuffer);
    }
};


class OctoWS2811LEDPatterns : public LEDPatterns {
private:
    OctoWS2811WithMemory m_strip;
protected:
    virtual void internalShow() {
        // This is our double-buffer copy; the drawBuffer is NULL, which is okay with the class
        for (int i = 0; i < getLEDCount(); i++) {
            //m_strip.setPixel(i, m_leds[i]);
            m_strip.setPixel(i, m_strip.color(m_leds[i].red, m_leds[i].green, m_leds[i].blue));
        }
        m_strip.show();
    }
public:
    virtual void setBrightness(uint8_t brightness) {  } // not supported
    uint8_t getBrightness() { return 255; }; // not supported
    OctoWS2811LEDPatterns(uint32_t ledCount, int stripPin = STRIP_PIN, uint8_t config = WS2811_GRB) : LEDPatterns(ledCount), m_strip(ledCount, config) { }
    virtual void begin() {
        m_strip.begin();
        m_strip.show();// all off
    };
};


#endif
