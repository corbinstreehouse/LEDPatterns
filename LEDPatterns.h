//
//  LEDPatterns.h
//  LEDDigitalCyrWheel
//
//  Created by corbin dunn on 5/31/14.
//
//

#ifndef __LEDDigitalCyrWheel__LEDPatterns__
#define __LEDDigitalCyrWheel__LEDPatterns__

#include "Arduino.h"
#include "pixeltypes.h"

// Turn this off if you don't have an SD card
#define SD_CARD_SUPPORT 0

// NOTE: update g_patternTypeNames when this changes!!
typedef enum : int16_t {
    LEDPatternTypeMin = 0,
    
    LEDPatternTypeRotatingRainbow = 0,
    LEDPatternTypeRotatingMiniRainbows,
    
    LEDPatternTypeFadeOut,
    LEDPatternTypeFadeIn,
    LEDPatternTypeColorWipe,
    LEDPatternTypeDoNothing,
    LEDPatternTypeTheaterChase,
    
    LEDPatternTypeGradient,
    LEDPatternTypePluseGradientEffect,
    LEDPatternTypeRandomGradients,
    
#if SD_CARD_SUPPORT
    // Patterns defined by an image
    LEDPatternTypeImageLinearFade, // smooth traverse over pixels
    LEDPatternTypeImageEntireStrip, // one strip piece at a time defined
#endif
    
    // the next set is ordered specifically
    LEDPatternTypeWarmWhiteShimmer,
    LEDPatternTypeRandomColorWalk,
    LEDPatternTypeTraditionalColors,
    LEDPatternTypeColorExplosion,
    LEDPatternTypeRWGradient,
    
    LEDPatternTypeWhiteBrightTwinkle,
    LEDPatternTypeWhiteRedBrightTwinkle,
    LEDPatternTypeRedGreenBrightTwinkle,
    LEDPatternTypeColorTwinkle,
    
    LEDPatternTypeCollision,
    
    LEDPatternTypeWave, // 4 wave
    LEDPatternTypeBottomGlow,
    LEDPatternTypeRotatingBottomGlow,
    
    LEDPatternTypeSolidColor,
    LEDPatternTypeSolidRainbow,
    LEDPatternTypeRainbowWithSpaces,
    
    LEDPatternTypeBlink,
    
    LEDPatternTypeMax,
    LEDPatternTypeAllOff = LEDPatternTypeMax,
} LEDPatternType;


class LEDPatterns {
private:
    uint32_t m_startTime;
    LEDPatternType m_patternType;
    uint32_t m_ledCount;
    
    bool m_firstTime;
    uint32_t m_duration;
    uint32_t m_timePassed;
    int m_intervalCount;
    
    // Stuff that applies to only certain patterns
    CRGB m_patternColor;
    
    // Extra state/info for some patterns
    int m_initialPixel;
    int m_initialPixel1;
    int m_initialPixel2;
    int m_initialPixel3;
    
    CRGB *m_ledTempBuffer1;
    CRGB *m_ledTempBuffer2;
    
    CRGB m_randColor1; // These could be pointers into the temp buffer
    CRGB m_randColor2;
    CRGB m_randColor3;
    
    // These are just a direct port over of the pololu example code made up into a single class
    unsigned int m_loopCount;
    unsigned int m_seed;
    unsigned int m_state;
    unsigned int m_count;
    
#if SD_CARD_SUPPORT
    uint32_t m_dataOffsetReadIntoBuffer1;
    uint32_t m_dataOffsetReadIntoBuffer2;
    
    char *m_dataFilename; // a reference! not copied
    uint32_t m_dataLength;
    uint32_t m_dataOffset;
    
    void initImageDataForHeader();
    void readDataIntoBufferStartingAtPosition(uint32_t position, uint8_t *buffer);
    uint8_t *dataForOffset(uint32_t offset);
#endif
    
    inline int getBufferSize() { return sizeof(CRGB) * m_ledCount; }
    CRGB *getTempBuffer1();
    CRGB *getTempBuffer2();
    inline float getPercentagePassed() { return (float)m_timePassed / (float)m_duration; };

    // TODO: corbin, use the other methods for fade to black as this is probably slow (but seems fast on a teensy)
    inline void fadePixel(int i, CRGB color, float amount) {
        color.red *= amount;
        color.green *= amount;
        color.blue *= amount;
        setPixelColor(i, color);
    }
private: // Patterns
    // Pattern implementations by corbin
    void wavePattern();
    void wavePatternWithColor(CRGB c, int initialPixel);
    void bottomGlowFromTopPixel(float topPixel);
    void bottomGlow();
    void rotatingBottomGlow();
    void fadeIn();
    void fadeOut();
    void solidRainbow(int positionInWheel, int count);
    void rainbows(int count);
    void rainbowWithSpaces(int count);
    void colorWipe();
    void theaterChase();
    void ledGradients();
    void pulseGradientEffect();
    void randomGradients();
    int gradientOverXPixels(int pixel, int fullCount, int offCount, int fadeCount, CRGB color);
    void blinkPattern();

#if SD_CARD_SUPPORT
    // Image based patterns from an SD card
    void linearImageFade();
    void patternImageEntireStrip();
#endif
    
    // Patterns taken from pololu demo
    void warmWhiteShimmer();
    void randomColorWalk(unsigned char initializeColors, unsigned char dimOnly);
    void traditionalColors();
    void colorExplosion(bool noNewBursts);
    void pololuGradient();
    void brightTwinkle(unsigned char minColor, unsigned char numColors, unsigned char noNewBursts);
    unsigned char collision();
protected:
    CRGB *m_leds;
    inline void setPixelColor(int pixel, CRGB color) { m_leds[pixel] = color; };
    virtual void internalShow() = 0;
    inline uint32_t getLEDCount() { return m_ledCount; };
public:
    
    LEDPatterns(uint32_t ledCount) : m_ledCount(ledCount), m_duration(1000) {
        int byteCount = sizeof(CRGB) * ledCount;
        m_leds = (CRGB *)malloc(byteCount);
        bzero(m_leds, byteCount);
        randomSeed(m_seed);
    };
    
    ~LEDPatterns() {
        free(m_leds);
        if (m_ledTempBuffer1) {
            free(m_ledTempBuffer1);
        }
        if (m_ledTempBuffer2) {
            free(m_ledTempBuffer2);
        }
    }

    // Call begin before doing anyting
    virtual void begin() = 0;
    
    // Primary way to change patterns by calling setPatternType; this re-intializes things
    void setPatternType(LEDPatternType type);
    inline LEDPatternType getPatternType() { return m_patternType; }
    // A pattern's speed is based on its duration. Some patterns ignore this, and others adhere to it. After each duration "tick" happens, the interval count is increased.
    inline void setDuration(uint32_t duration) { m_duration = duration; } // in ms; must be > 0
    // Some patterns are based off a primary color
    inline void setPatternColor(CRGB color) { m_patternColor = color; };

    
    // The number of intervals that have passed. 0 based counting (0 is the first, and after a call to show() it will be 1)
    inline int getIntervalCount() { return m_intervalCount; };
    
#if SD_CARD_SUPPORT
    // For LEDPatternTypeImage, you MUST set the data info. A file can have the image data anywhere inside of it, and dataOffset indicates the offset starting into the file.
    inline void setDataInfo(char *filename, uint32_t dataLength, uint32_t dataOffset = 0) {
        m_dataFilename = filename; m_dataLength = dataLength; m_dataOffset = dataOffset;
    };
#endif
    
    // Abstract control over the overall LED options. Not all subclasses support all options.
    // 0 = off, 255 = on. Depends on the subclass for implementation
    virtual void setBrightness(uint8_t brightness) = 0;
    
    // Call show to make the update take
    void show();
    
    // The next methods are useful for showing state; they flash using "delay" and return after the flash has completed.
    void flashThreeTimesWithDelay(CRGB color, uint32_t delay);
    void flashOnce(CRGB color);
};






#endif /* defined(__LEDDigitalCyrWheel__LEDPatterns__) */
