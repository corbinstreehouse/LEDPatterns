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
#include "FastLED.h"

// Turn this off if you don't have an SD card. Needs to be set before the LEDPatternType include.
#define SD_CARD_SUPPORT 1

#include "LEDPatternType.h"
#include "CDLazyBitmap.h"


class LEDPatterns {
private:
    uint32_t m_startTime;
    LEDPatternType m_patternType;
    LEDPatternType m_nextPatternType; // For crossfade
    uint32_t m_ledCount;
    
    uint32_t m_firstTime:1;
    uint32_t m_needsInternalShow:1;
    uint32_t m_reserved:30;
    
    uint32_t m_duration;
    uint32_t m_timePassed;
    
    uint32_t m_timedPattern;
    // Stuff that applies to only certain patterns
    CRGB m_patternColor;
    LEDPatternOptions m_patternOptions;
    
    // Extra state for some patterns.... name is irrelevant
    uint32_t m_initialPixel;
    uint32_t m_initialPixel1;
    uint32_t m_initialPixel2;
    uint32_t m_initialPixel3;
    
    CRGB *m_ledTempBuffer1;
    CRGB *m_ledTempBuffer2;
    void *m_stateInfo;
    int m_stateInfoCount;
    
    CRGB m_randColor1; // These could be pointers into the temp buffer
    CRGB m_randColor2;
    CRGB m_randColor3;
    
    // These are just a direct port over of the pololu example code made up into a single class
    unsigned int m_loopCount;
    unsigned int m_seed;
    unsigned int m_state;
    unsigned int m_count;
    
    uint32_t m_pauseTime; // When non-0, we are paused
    
    
#if SD_CARD_SUPPORT
    // lazy bitmaps will replace my file format and file reading (soon!)
    CDPatternBitmap *m_lazyBitmap;
#endif
    
    inline int getBufferSize() { return sizeof(CRGB) * m_ledCount; }
    CRGB *getTempBuffer1();
    CRGB *getTempBuffer2();
    
    float m_percentagePassedCache;
    inline float getPercentagePassed() {
        return m_percentagePassedCache;
    };

    // TODO: corbin, use the other methods for fade to black as this is probably slow (but seems fast on a teensy)
    inline void fadePixel(int i, CRGB color, float amount) {
        color.red *= amount;
        color.green *= amount;
        color.blue *= amount;
        setPixelColor(i, color);
    }
    
    bool shouldUpdatePattern(); // for 60hz based patterns 
private: // Patterns
    // Pattern implementations by corbin
    void wavePattern();
    void wavePatternWithColor(CRGB c, int initialPixel);
    void bottomGlowFromTopPixel(float topPixel);
    void bottomGlow();
    void rotatingBottomGlow();
    void fadeIn(float percentagePassed);
    void fadeOut(float percentagePassed);
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
    void firePattern();
    void blueFirePattern();
    
    void firePatternWithColor(bool blue);
    
    void fireColorWithPalette(const CRGBPalette16& pal, int cooling, int sparking);
    void flagEffect();
    void sinWaveDemoEffect();
    void funkyCloudsPattern();
    
    void commonInitForPattern();
    void lifePattern(bool dynamic);
    void bouncingBallPattern();
    void bitmapPattern();
    
    void bitmapPatternFillPixels();
    void bitmapPatternInterpolatePixels(float percentage, bool isChasingPattern);
    
    // Fades smoothly to the next pattern from the current data shown over the duration of the pattern
    void crossFadeToNextPattern();
    
    void updateLEDsForPatternType(LEDPatternType patternType);
    
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
    inline uint32_t getLEDCount() { return m_ledCount; };
public:
    
    LEDPatterns(uint32_t ledCount) : m_ledCount(ledCount), m_duration(1000), m_pauseTime(0) {
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
    
    // a given pattern does NOT need a duration set if it is continuous
    static bool PatternIsContinuous(LEDPatternType p);
    static bool PatternNeedsDuration(LEDPatternType p);
    static bool PatternDurationShouldBeEqualToSegmentDuration(LEDPatternType p);


    // Call begin before doing anything
    virtual void begin() = 0;
    
    // Primary way to change patterns by calling setPatternType; this re-intializes things
    void setPatternType(LEDPatternType type);
    inline LEDPatternType getPatternType() { return m_patternType; }
    
    void setNextPatternType(LEDPatternType nextType) { m_nextPatternType = nextType; } // Only needed for crossfade pattern
    
    // A pattern's speed is based on its duration. Some patterns ignore this, and others adhere to it. After each duration "tick" happens, the interval count is increased.
    inline void setPatternDuration(uint32_t duration) { m_duration = duration; } // in ms; must be > 0
    
    // Some patterns are based off a primary color
    inline void setPatternColor(CRGB color) { m_patternColor = color; };
    inline void setPatternOptions(LEDPatternOptions patternOptions) { m_patternOptions = patternOptions; }
    

#if SD_CARD_SUPPORT
    // For LEDPatternTypeImage* and LEDPatternTypeBitmap, you MUST set the filename to read from. Calling this method loads the bitmap right at that moment.
    inline void setBitmapFilename(const char *filename) {
        if (m_lazyBitmap) {
            delete m_lazyBitmap;
            m_lazyBitmap = NULL;
        }
        if (filename != NULL) {
            if (m_lazyBitmap == NULL) {
//                wait, don't I use the buffers for crossfade? this won't work...
                m_lazyBitmap = new CDPatternBitmap(filename, getTempBuffer1(), getTempBuffer2(), getBufferSize());
            } else {
                m_lazyBitmap->moveToStart();
            }
        }
    }
    
    inline CDLazyBitmap *getBitmap() { return m_lazyBitmap; }
    
#endif
    
    // Abstract control over the overall LED options. Not all subclasses support all options.
    // 0 = off, 255 = on. Depends on the subclass for implementation
    virtual void setBrightness(uint8_t brightness) = 0;
    
    virtual void internalShow() = 0; // only updates the LEDs with current state; mainly for subclassing
    
    // Call show to make the update take
    void show();
    
    // The next methods are useful for showing state; they flash using "delay" and return after the flash has completed.
    void flashThreeTimes(CRGB color, uint32_t delay = 150);
    void flashOnce(CRGB color);
    
    void showProgress(float progress, CRGB color);
    
    // Implemented commands..
    void pause();
    void play();
    bool isPaused();
};






#endif /* defined(__LEDDigitalCyrWheel__LEDPatterns__) */
