//
//  LEDPatterns.cpp
//  LEDDigitalCyrWheel
//
//  Created by corbin dunn on 5/31/14.
//
//

#include "LEDPatterns.h"
#include "colorutils.h"
#include "hsv2rgb.h"
#include "colorpalettes.h"

#if SD_CARD_SUPPORT
    #include "SPI.h"
    #include "SdFat.h"
#endif

#if 1 // DEBUG
    #define DEBUG_PRINTLN(a) Serial.println(a)
    #define DEBUG_PRINTF(a, ...) Serial.printf(a, ##__VA_ARGS__)
#else
    #define DEBUG_PRINTLN(a)
    #define DEBUG_PRINTF(a, ...)
#endif

#ifndef byte
#define byte uint8_t
#endif

// OS X has MIN but not min, and Teensy has min but not MIN
#ifndef MIN
#define MIN min
#endif

class LEDStateInfo {
public:
    LEDStateInfo(int pos, int ledCount, int stateObjectCount, int velocity);
    void initForBounce(int pos, int stateObjectCount); // for bounce
    
    void setLife(unsigned long life); // in seconds
    void kill();
    unsigned long age();
    
    // color can be set
    void setColor(CRGB color) { m_color = color; }

    void update(CRGB *leds, int ledCount); // should be called on each iteration
    void updateBounceColor(CRGB *leds, int ledCount);
private:
	CRGB m_color;
	int m_acceleration;
	int m_velocity;
  	int m_pos;
	int m_LEDmm; // what is this for??
	unsigned long m_birth;
	unsigned long m_life;
	unsigned long m_death;
    
	unsigned long m_lastUpdate;
    float m_impact;
    float m_cycle;
    float m_cors;
    float m_height;
    bool alive();
};


LEDStateInfo::LEDStateInfo(int pos, int ledCount, int stateObjectCount, int velocity) {
    m_color = CHSV((255/stateObjectCount)*pos, 255, 255);
    m_LEDmm = 8; // TODO: what's this for?
    m_pos = ((m_LEDmm * ledCount) / stateObjectCount) * pos;
    m_velocity = velocity;
    m_acceleration = -1;
    m_lastUpdate = micros();
    m_birth = m_lastUpdate;
    m_life = 0;
    m_death = m_birth + m_life;

    if (velocity == 0) {
        randomSeed(micros());
        m_velocity = random(950, 1000);
        randomSeed(micros());
        m_acceleration = random(-1,-2);
        randomSeed(micros());
        if (random(2) == 1) {
            m_velocity *= -1;
            m_acceleration *= -1;
        }
    }
}

#define GRAVITY           -9.81              // Downward (negative) acceleration of gravity in m/s^2
#define BOUNCE_HEIGHT                5                  // Starting height, in meters, of the ball (strip length)
const float vImpact0 = sqrt( -2 * GRAVITY * BOUNCE_HEIGHT );  // Impact velocity of the ball when it hits the ground if "dropped" from the top of the strip

void LEDStateInfo::initForBounce(int pos, int stateObjectCount) {
    m_lastUpdate = millis();
    m_height = BOUNCE_HEIGHT;
    m_pos = 0;                              // Balls start on the ground
    m_impact = vImpact0;             // And "pop" up at vImpact0
    m_cors = 0.90 - float(pos)/pow(stateObjectCount,2);
    m_impact = m_cors * m_impact ;   // and recalculate its new upward velocity as it's old velocity * COR
    m_cycle = 0;
}

int circMod(int x, int m) {
    return (x%m + m)%m;
}

void LEDStateInfo::update(CRGB *leds, int ledCount) {
    if (alive()) {
        unsigned long now = micros();
        m_velocity += m_acceleration;
        m_pos += m_velocity * ((now-m_lastUpdate)/1000000.0);
        m_lastUpdate = now;
    }

    // was fireUpObj.
    // corbin: when does vel change to 0??? it starts out non zero..so I don't know when this would get executed..
    if (abs(m_velocity) == 0) {
        randomSeed(micros());
        m_velocity = random(950, 1000);
        randomSeed(micros());
        m_acceleration = random(-1,-2);
        randomSeed(micros());
        if (random(2) == 1) {
            m_velocity *= -1;
            m_acceleration *= -1;
        }
    }
    
    // was the iteration
    if (alive()) {
        int objectSize = abs(m_velocity/100);
        if (objectSize == 0) { objectSize = 1; }
        int ledPos = m_pos / m_LEDmm;
        int pos = ledPos;
        for (int x = 0; x <= objectSize; x++){
            if (m_velocity > 0) {
                leds[circMod(pos-x, ledCount)] += m_color % (uint8_t)(255-((255/objectSize)*x));
            } else {
                leds[circMod(pos+x, ledCount)] += m_color % (uint8_t)(255-((255/objectSize)*x));
            }
        }
    }
}

void LEDStateInfo::updateBounceColor(CRGB *leds, int ledCount) {
    m_cycle =  millis() - m_lastUpdate ;     // Calculate the time since the last time the ball was on the ground
    
    // A little kinematics equation calculates positon as a function of time, acceleration (gravity) and intial velocity
    m_height = 0.5 * GRAVITY * pow( m_cycle/1000 , 2.0 ) + m_impact * m_cycle/1000;
    
    if ( m_height < 0 ) {
        m_height = 0;                            // If the ball crossed the threshold of the "ground," put it back on the ground
        m_impact = m_cors * m_impact ;   // and recalculate its new upward velocity as it's old velocity * COR
        m_lastUpdate = millis();
        
        if ( m_impact < 0.01 ) m_impact = vImpact0;  // If the ball is barely moving, "pop" it back up at vImpact0
    }
    m_pos = round( m_height * (ledCount - 1) / BOUNCE_HEIGHT);
    leds[m_pos] = m_color;
}

// in seconds
void LEDStateInfo::setLife(unsigned long life) {
    m_life = life * 1000;
    m_death = m_birth + m_life;
}

bool LEDStateInfo::alive() {
    unsigned long now = micros();
    if (now < m_death || m_life == 0) {
        return true;
    } else {
        return false;
    }
}

void LEDStateInfo::kill() {
    m_death = micros();
}

// What do I mean by continuous??
// I mean to reset the percentage back to 0 once it goes past 1.0 IF this returns false. If it returns true, the value will go past 100% continuously forever.
static inline bool _PatternIsContinuous(LEDPatternType p) {
    switch (p) {
        case LEDPatternTypeRotatingMiniRainbows:
        case LEDPatternTypeRotatingRainbow:
        case LEDPatternTypeTheaterChase:
        case LEDPatternTypeGradient:
        case LEDPatternTypePluseGradientEffect:
        case LEDPatternTypeSolidRainbow:
        case LEDPatternTypeRainbowWithSpaces:
        case LEDPatternTypeRandomGradients:
        case LEDPatternTypeFadeOut: // After 100% the pattern just leaves it black (it doesn't reset to fade again)
        case LEDPatternTypeFadeIn: // After 100% it just leaves the color up (doesn't do another fade)
        case LEDPatternTypeFadeInFadeOut:
            return true;
        case LEDPatternTypeWarmWhiteShimmer:
        case LEDPatternTypeRandomColorWalk:
        case LEDPatternTypeTraditionalColors:
        case LEDPatternTypeColorExplosion:
        case LEDPatternTypeRWGradient:
        case LEDPatternTypeWhiteBrightTwinkle:
        case LEDPatternTypeWhiteRedBrightTwinkle:
        case LEDPatternTypeRedGreenBrightTwinkle:
        case LEDPatternTypeColorTwinkle:
            return false; // i think
            
        case LEDPatternTypeCollision:
            return false;
        case LEDPatternTypeColorWipe:
        case LEDPatternTypeWave:
            return false;
            
        case LEDPatternTypeDoNothing:
            return true;
#if SD_CARD_SUPPORT
        case LEDPatternTypeImageReferencedBitmap:
        case LEDPatternTypeImageEntireStrip_UNUSED:
            return true;
#endif
        case LEDPatternTypeBottomGlow:
            return true; // Doesn't do anything
        case LEDPatternTypeRotatingBottomGlow:
        case LEDPatternTypeSolidColor:
            return false; // repeats after a rotation
        case LEDPatternTypeCount:
        case LEDPatternTypeBlink:
            return false;
        case LEDPatternTypeFire:
        case LEDPatternTypeBlueFire:
        case LEDPatternTypeLavaFire:
        case LEDPatternTypeRainbowFire:
        case LEDPatternFlagEffect:
        case LEDPatternTypeFunkyClouds:
            return true;
        case LEDPatternTypeSinWave:
            // restarts every duration and generates a new seed/pattern
            return false;
        case LEDPatternTypeCrossfade:
            return false;
        case LEDPatternTypeLife:
        case LEDPatternTypeLifeDynamic:
        case LEDPatternTypeBouncingBall:
        case LEDPatternTypeBitmap:
            return true;
            
    }
}

// This always resets things, so only change it when necessary
void LEDPatterns::setPatternType(LEDPatternType type) {
    m_patternType = type;
    m_startTime = millis();
    m_firstTime = true;
    m_stateInfoCount = 0;
    m_loopCount = 0;
    m_count = 0;

}


void LEDPatterns::updateLEDsForPatternType(LEDPatternType patternType) {
//    DEBUG_PRINTF("updateLEDsForPatternType: %d\n", patternType);
    // for polulu
    unsigned int maxLoops = 0;  // go to next state when m_ledCount >= maxLoops
    // Only update based on the real type...
    if (m_patternType == LEDPatternTypeWarmWhiteShimmer || m_patternType == LEDPatternTypeRandomColorWalk)
    {
        // for these two patterns, we want to make sure we get the same
        // random sequence six times in a row (this provides smoother
        // random fluctuations in brightness/color)
        if (m_loopCount % 6 == 0)
        {
            m_seed = random(30000);
        }
        randomSeed(m_seed);
    }
    
    // Switch based ont he patternType
    switch (patternType) {
        case LEDPatternTypeRotatingRainbow: {
            rainbows(1);
            break;
        }
        case LEDPatternTypeRotatingMiniRainbows: {
            rainbows(4);
            break;
        }
        case LEDPatternTypeColorWipe: {
            colorWipe();
            break;
        }
        case LEDPatternTypeFadeOut: {
            fadeOut(getPercentagePassed());
            break;
        }
        case LEDPatternTypeFadeIn: {
            fadeIn(getPercentagePassed());
            break;
        }
        case LEDPatternTypeFadeInFadeOut: {
            float percentagePassed = getPercentagePassed();
            if (percentagePassed <= 0.5) {
                float fadeInPercentage = percentagePassed / 0.5;
                fadeIn(fadeInPercentage);
            } else {
                float fadeOutPercentage = (percentagePassed - 0.5) / 0.5;
                // first fade out marker
                if (m_stateInfoCount == 0) {
                    m_firstTime = true;
                    m_stateInfoCount = 1;
                    fadeOutPercentage = 0;
                }
                fadeOut(fadeOutPercentage);
            }
            break;
        }
        case LEDPatternTypeDoNothing: {
            m_needsInternalShow = false; // Doesn't do a show at all..leaves the last pixels shown on..
            break;
        }
        case LEDPatternTypeTheaterChase: {
            theaterChase();
            break;
        }
#if SD_CARD_SUPPORT
        case LEDPatternTypeImageReferencedBitmap: {
            linearImageFade();
            break;
        }
        case LEDPatternTypeImageEntireStrip_UNUSED: {
            patternImageEntireStrip();
            break;
        }
#endif
        case LEDPatternTypeGradient: {
            ledGradients();
            break;
        }
        case LEDPatternTypePluseGradientEffect: {
            pulseGradientEffect();
            break;
        }
        case LEDPatternTypeWarmWhiteShimmer:
            // warm white shimmer for 300 loopCounts, fading over last 70
            maxLoops = 300;
            warmWhiteShimmer();
            break;
            
        case LEDPatternTypeRandomColorWalk:
            // start with alternating red and green m_leds that randomly walk
            // to other colors for 400 loopCounts, fading over last 80
            maxLoops = 400;
            randomColorWalk(m_loopCount == 0 ? 1 : 0, m_loopCount > maxLoops - 80);
            
            break;
            
        case LEDPatternTypeTraditionalColors:
            // repeating pattern of red, green, orange, blue, magenta that
            // slowly moves for 400 loopCounts
            maxLoops = 400;
            traditionalColors();
            break;
        case LEDPatternTypeColorExplosion:
            // bursts of random color that radiate outwards from random points
            // for 630 loop counts; no burst generation for the last 70 counts
            // of every 200 count cycle or over the over final 100 counts
            // (this creates a repeating bloom/decay effect)
            maxLoops = 630;
            colorExplosion((m_loopCount % 200 > 130) || (m_loopCount > maxLoops - 100));
            
            break;
            
        case LEDPatternTypeRWGradient:
            // red -> white -> green -> white -> red ... gradiant that scrolls
            // across the strips for 250 counts; this pattern is overlaid with
            // waves of dimness that also scroll (at twice the speed)
            maxLoops = 250;
            pololuGradient();
            delay(6);  // add an extra 6ms delay to slow things down
            
            break;
            
        case LEDPatternTypeWhiteBrightTwinkle:
            brightTwinkle(0, 1, 0);
            break;
        case LEDPatternTypeWhiteRedBrightTwinkle:
            brightTwinkle(0, 2, 0);  // white and red for next 250 counts
            break;
        case LEDPatternTypeRedGreenBrightTwinkle:
            brightTwinkle(1, 2, 0);  // red, and green for next 250 counts
            break;
        case LEDPatternTypeColorTwinkle:
            // red, green, blue, cyan, magenta, yellow for the rest of the time
            brightTwinkle(1, 6, 0);
            break;
        case LEDPatternTypeCollision:
            // colors grow towards each other from the two ends of the strips,
            // accelerating until they collide and the whole strip flashes
            // white and fades; this repeats until the function indicates it
            // is done by returning 1, at which point we stop keeping maxLoops
            // just ahead of m_ledCount
            if (!collision())
            {
                maxLoops = m_loopCount + 2;
            }
            break;
        case LEDPatternTypeWave:
            wavePattern();
            break;
        case LEDPatternTypeBottomGlow:
            bottomGlow();
            break;
        case LEDPatternTypeRotatingBottomGlow:
            rotatingBottomGlow();
            break;
        case LEDPatternTypeSolidColor: {
            if (m_firstTime) {
                fill_solid(m_leds, m_ledCount, m_patternColor);
            } else {
                m_needsInternalShow = false; // Once set we don't need to do it again
            }
            break;
        }
        case LEDPatternTypeSolidRainbow: {
            //            m_strip->setBrightness(255);
            //            float percentageThrough = (float)timePassedInMS / 3000; //(float)itemHeader->duration;
            //            while (percentageThrough > 1.0) {
            //                percentageThrough -= 1.0;
            //            }
            //
            //            percentageThrough = percentageThrough*percentageThrough*percentageThrough*percentageThrough; // cubic?
            //
            //            m_strip->setBrightness(round(.5*255.0*percentageThrough));
            //            uint32_t c = m_strip->Color(255, 0, 0);
            //
            ////            uint32_t c = m_strip->Color(round(percentageThrough*.5*255), 0, 0);
            //            setAllPixelsToColor(c);
            //
            //
            //            break;
#warning corbin! don't call this multiple times...unless we want it to change
            if (m_firstTime) {
                solidRainbow(0, 1);
            } else {
                m_needsInternalShow = false; // Once set we don't need to do it again
            }
            break;
        }
        case LEDPatternTypeRainbowWithSpaces: {
            rainbowWithSpaces(1);
            
            break;
        }
        case LEDPatternTypeRandomGradients: {
            randomGradients();
            break;
        }
        case LEDPatternTypeBlink: {
            blinkPattern();
            break;
        }
        case LEDPatternTypeFire: {
            firePattern();
            break;
        }
        case LEDPatternTypeBlueFire: {
            blueFirePattern();
            break;
        }
        case LEDPatternTypeLavaFire: {
            fireColorWithPalette(LavaColors_p, 10, 220);
            break;
        }
        case LEDPatternTypeRainbowFire: {
            fireColorWithPalette(PartyColors_p, 40, 220);
            break;
        }
        case LEDPatternFlagEffect: {
            flagEffect();
            break;
        }
        case LEDPatternTypeCrossfade: {
            crossFadeToNextPattern();
            break;
        }case LEDPatternTypeSinWave: {
            sinWaveDemoEffect();
            break;
        }
        case LEDPatternTypeFunkyClouds: {
            funkyCloudsPattern();
            break;
        }
        case LEDPatternTypeLife: {
            lifePattern(false);
            break;
        }
        case LEDPatternTypeLifeDynamic: {
            lifePattern(true);
            break;
        }
        case LEDPatternTypeBouncingBall: {
            bouncingBallPattern();
            break;
        }
        case LEDPatternTypeBitmap: {
            bitmapPattern();
            break;
        }
    
    }
    
    if (m_patternType >= LEDPatternTypeWarmWhiteShimmer && m_patternType <= LEDPatternTypeCollision) {
        //        rgb_color *m_leds = (rgb_color *)m_strip.getPixels();
        // do a bit walk over to fix brightness, then show
        //        if (m_patternType != LEDPatternTypeWarmWhiteShimmer && m_patternType != LEDPatternTypeRandomColorWalk && m_patternType != LEDPatternTypeColorExplosion && m_patternType != LEDPatternTypeBrightTwinkle ) {
        //
        //            uint8_t brightness = g_strip.getBrightness();
        //            if (brightness > 0) {
        //                for (int i = 0; i < m_ledCount; i++) {
        //                    m_leds[i].red =  (m_leds[i].red * brightness) >> 8;
        //                    m_leds[i].green =  (m_leds[i].green * brightness) >> 8;
        //                    m_leds[i].blue =  (m_leds[i].blue * brightness) >> 8;
        //                }
        //            }
        //        }
        
        
        m_loopCount++;  // increment our loop counter/timer.
        if (m_loopCount >= maxLoops)
        {
            // if the time is up for the current pattern and the optional hold
            // switch is not grounding the AUTOCYCLE_SWITCH_PIN, clear the
            // loop counter and advance to the next pattern in the cycle
            m_loopCount = 0;  // reset timer
        }
    }

}

bool LEDPatterns::PatternIsContinuous(LEDPatternType p) {
    return _PatternIsContinuous(p);
}

bool LEDPatterns::PatternDurationShouldBeEqualToSegmentDuration(LEDPatternType p) {
    switch (p) {
        case LEDPatternTypeFadeIn:
        case LEDPatternTypeFadeInFadeOut:
        case LEDPatternTypeFadeOut:
        case LEDPatternTypeColorWipe: // maybe??
        case LEDPatternTypeCrossfade:
            return true;
        default:
            return false;
    }
}

bool LEDPatterns::PatternNeedsDuration(LEDPatternType p) {
    switch (p) {
        case LEDPatternTypeRotatingMiniRainbows:
        case LEDPatternTypeRotatingRainbow:
        case LEDPatternTypeTheaterChase:
        case LEDPatternTypeGradient:
        case LEDPatternTypePluseGradientEffect:
        case LEDPatternTypeSolidRainbow:
        case LEDPatternTypeRainbowWithSpaces:
        case LEDPatternTypeFadeOut:
        case LEDPatternTypeFadeIn:
        case LEDPatternTypeRandomGradients:
        case LEDPatternTypeColorWipe:
        case LEDPatternTypeWave:
        case LEDPatternTypeRotatingBottomGlow:
#if SD_CARD_SUPPORT
        case LEDPatternTypeImageReferencedBitmap:
        case LEDPatternTypeImageEntireStrip_UNUSED:
        case LEDPatternTypeBitmap:
#endif
        case LEDPatternTypeSinWave:
        case LEDPatternTypeBlink:
            return true;
        case LEDPatternTypeCrossfade:
        case LEDPatternTypeWarmWhiteShimmer:
        case LEDPatternTypeRandomColorWalk:
        case LEDPatternTypeTraditionalColors:
        case LEDPatternTypeColorExplosion:
        case LEDPatternTypeRWGradient:
        case LEDPatternTypeWhiteBrightTwinkle:
        case LEDPatternTypeWhiteRedBrightTwinkle:
        case LEDPatternTypeRedGreenBrightTwinkle:
        case LEDPatternTypeColorTwinkle:
        case LEDPatternTypeCollision:
        case LEDPatternTypeDoNothing:
        case LEDPatternTypeBottomGlow:
        case LEDPatternTypeSolidColor:
        case LEDPatternTypeFire:
        case LEDPatternTypeBlueFire:
        case LEDPatternTypeLavaFire:
        case LEDPatternTypeRainbowFire:
        case LEDPatternFlagEffect:
        case LEDPatternTypeLife:
        case LEDPatternTypeLifeDynamic:
        case LEDPatternTypeBouncingBall:
        case LEDPatternTypeFunkyClouds:
            return false;
        default:
            return false;
    }
}


void LEDPatterns::show() {
    if (m_pauseTime != 0) {
        return;
    }
    uint32_t now = millis();
    // The inital tick always starts with 0
    if (m_startTime > now) {
        m_startTime = now; // roll over..
    }
    m_timePassed = m_firstTime ? 0 : now - m_startTime;

    m_percentagePassedCache = m_firstTime ? 0 : m_duration != 0 ? (float)m_timePassed / (float)m_duration : 0.0;
    
//    NSLog(@"m_percentagePassedCache %g, m_firstTime: %d, m_timePassed: %d, m_duration: %d", m_percentagePassedCache, m_firstTime, m_timePassed, m_duration);
    
    if (!_PatternIsContinuous(m_patternType)) {
        // Since it isn't continuous,we have to reset the percentage when it goes past 1.0, and we reset it and the time back to 0 to start on the exact same 0 tick
        if (m_percentagePassedCache > 1.0) {
            m_firstTime = true;
            m_percentagePassedCache = 0; // Back to the start
            m_timePassed = 0;
            m_startTime = now;
//            NSLog(@" RESET: m_percentagePassedCache %g, m_firstTime: %d, m_timePassed: %d, m_duration: %d", m_percentagePassedCache, m_firstTime, m_timePassed, m_duration);
        }
    }

    m_needsInternalShow = true;
    updateLEDsForPatternType(m_patternType);
    // Some patterns may not need to do any more show work after doing it once.
    if (m_needsInternalShow) {
        internalShow();
    }
    // no longer the first time
    m_firstTime = false;
}

bool LEDPatterns::isPaused() {
    return m_pauseTime != 0;
}

void LEDPatterns::play() {
    if (m_pauseTime != 0) {
        // increase the start time by the time that has passed
        uint32_t now = millis();
        if (m_firstTime) {
            m_startTime = now;
        } else if (now > m_pauseTime) {
            uint32_t timePassed = now - m_pauseTime;
            m_startTime += timePassed;
            // handle the case of the pattern being reset.
            if (m_startTime > now) {
                m_startTime = now;
            }
        } else {
            // wrap; reset...
            m_firstTime = true;
        }
        m_pauseTime = 0;
    }
}

void LEDPatterns::pause() {
    if (m_pauseTime == 0) {
        m_pauseTime = millis(); // record when we paused so we can restart from that point
    }
}

// pololu... https://github.com/pololu/pololu-led-strip-arduino/blob/master/PololuLedStrip/examples/LedStripXmas/LedStripXmas.ino


// This function applies a random walk to val by increasing or
// decreasing it by changeAmount or by leaving it unchanged.
// val is a pointer to the byte to be randomly changed.
// The new value of val will always be within [0, maxVal].
// A walk direction of 0 decreases val and a walk direction of 1
// increases val.  The directions argument specifies the number of
// possible walk directions to choose from, so when directions is 1, val
// will always decrease; when directions is 2, val will have a 50% chance
// of increasing and a 50% chance of decreasing; when directions is 3,
// val has an equal chance of increasing, decreasing, or staying the same.
static void randomWalk(unsigned char *val, unsigned char maxVal, unsigned char changeAmount, unsigned char directions)
{
    unsigned char walk = random(directions);  // direction of random walk
    if (walk == 0)
    {
        // decrease val by changeAmount down to a min of 0
        if (*val >= changeAmount)
        {
            *val -= changeAmount;
        }
        else
        {
            *val = 0;
        }
    }
    else if (walk == 1)
    {
        // increase val by changeAmount up to a max of maxVal
        if (*val <= maxVal - changeAmount)
        {
            *val += changeAmount;
        }
        else
        {
            *val = maxVal;
        }
    }
}


// This function fades val by decreasing it by an amount proportional
// to its current value.  The fadeTime argument determines the
// how quickly the value fades.  The new value of val will be:
//   val = val - val*2^(-fadeTime)
// So a smaller fadeTime value leads to a quicker fade.
// If val is greater than zero, val will always be decreased by
// at least 1.
// val is a pointer to the byte to be faded.
static void fade(unsigned char *val, unsigned char fadeTime)
{
    if (*val != 0)
    {
        unsigned char subAmt = *val >> fadeTime;  // val * 2^-fadeTime
        if (subAmt < 1)
            subAmt = 1;  // make sure we always decrease by at least 1
        *val -= subAmt;  // decrease value of byte pointed to by val
    }
}

// ***** PATTERN RandomColorWalk *****
// This function randomly changes the color of every seventh LED by
// randomly increasing or decreasing the red, green, and blue components
// by changeAmount (capped at maxBrightness) or leaving them unchanged.
// The two preceding and following LEDs are set to progressively dimmer
// versions of the central color.  The initializeColors argument
// determines how the colors are initialized:
//   0: randomly walk the existing colors
//   1: set the LEDs to alternating red and green segments
//   2: set the LEDs to random colors
// When true, the dimOnly argument changes the random walk into a 100%
// chance of LEDs getting dimmer by changeAmount; this can be used for
// a fade-out effect.
void LEDPatterns::randomColorWalk(unsigned char initializeColors, unsigned char dimOnly)
{
    
    const unsigned char maxBrightness = 180;  // cap on LED brightness
    const unsigned char changeAmount = 3;  // size of random walk step
    
    // pick a good starting point for our pattern so the entire strip
    // is lit well (if we pick wrong, the last four LEDs could be off)
    unsigned char start;
    switch (m_ledCount % 7)
    {
        case 0:
            start = 3;
            break;
        case 1:
            start = 0;
            break;
        case 2:
            start = 1;
            break;
        default:
            start = 2;
    }
    
    for (int i = start; i < m_ledCount; i+=7)
    {
        if (initializeColors == 0)
        {
            // randomly walk existing colors of every seventh LED
            // (neighboring LEDs to these will be dimmer versions of the same color)
            randomWalk(&m_leds[i].red, maxBrightness, changeAmount, dimOnly ? 1 : 3);
            randomWalk(&m_leds[i].green, maxBrightness, changeAmount, dimOnly ? 1 : 3);
            randomWalk(&m_leds[i].blue, maxBrightness, changeAmount, dimOnly ? 1 : 3);
        }
        else if (initializeColors == 1)
        {
            CRGB c = { 0, 0, 0};
            // initialize LEDs to alternating red and green
            if (i % 2)
            {
                c.red = maxBrightness;
            }
            else
            {
                c.blue = maxBrightness;
            }
            m_leds[i] = c;
        }
        else
        {
            // initialize LEDs to a string of random m_leds
            m_leds[i] = CRGB((byte)random(maxBrightness), (byte)random(maxBrightness), (byte)random(maxBrightness));
        }
        
        // set neighboring LEDs to be progressively dimmer versions of the color we just set
        if (i >= 1)
        {
            CRGB c;
            c.red = (byte)(m_leds[i].red >> 2);
            c.green = (byte)(m_leds[i].green >> 2);
            c.blue = (byte)(m_leds[i].blue >> 2);
            m_leds[i-1] = c;
        }
        if (i >= 2)
        {
            CRGB c;
            c.red = (byte)(m_leds[i].red >> 3);
            c.green = (byte)(m_leds[i].green >> 3);
            c.blue = (byte)(m_leds[i].blue >> 3);
            m_leds
            [i-2] = c;
        }
        if (i + 1 < m_ledCount && i >= 1)
        {
            m_leds[i+1] = m_leds[i-1];
        }
        if (i + 2 < m_ledCount && i >= 2)
        {
            m_leds[i+2] = m_leds[i-2];
        }
    }
}


// ***** PATTERN TraditionalColors *****
// This function creates a repeating patern of traditional Christmas
// light m_leds: red, green, orange, blue, magenta.
// Every fourth LED is colored, and the pattern slowly moves by fading
// out the current set of lit LEDs while gradually brightening a new
// set shifted over one LED.
void LEDPatterns::traditionalColors()
{
    
#if PATTERN_EDITOR
    // slow it down..
    if (!shouldUpdatePattern()) {
        return;
    }
#endif
    // loop counts to leave strip initially dark
    const unsigned char initialDarkCycles = 10;
    // loop counts it takes to go from full off to fully bright
    const unsigned char brighteningCycles = 20;
    
    if (m_loopCount < initialDarkCycles)  // leave strip fully off for 20 cycles
    {
        return;
    }
    
    // if m_ledCount is not an exact multiple of our repeating pattern size,
    // it will not wrap around properly, so we pick the closest LED count
    // that is an exact multiple of the pattern period (20) and is not smaller
    // than the actual LED count.
    unsigned int extendedLEDCount = (((m_ledCount-1)/20)+1)*20;
    
    for (int i = 0; i < extendedLEDCount; i++)
    {
        unsigned char brightness = (m_loopCount - initialDarkCycles)%brighteningCycles + 1;
        unsigned char cycle = (m_loopCount - initialDarkCycles)/brighteningCycles;
        
        // transform i into a moving idx space that translates one step per
        // brightening cycle and wraps around
        unsigned int idx = (i + cycle)%extendedLEDCount;
        if (idx < m_ledCount)  // if our transformed index exists
        {
            if (i % 4 == 0)
            {
                // if this is an LED that we are coloring, set the color based
                // on the LED and the brightness based on where we are in the
                // brightening cycle
                switch ((i/4)%5)
                {
                    case 0:  // red
                        m_leds[idx].red = 200 * brightness/brighteningCycles;
                        m_leds[idx].green = 10 * brightness/brighteningCycles;
                        m_leds[idx].blue = 10 * brightness/brighteningCycles;
                        break;
                    case 1:  // green
                        m_leds[idx].red = 10 * brightness/brighteningCycles;
                        m_leds[idx].green = 200 * brightness/brighteningCycles;
                        m_leds[idx].blue = 10 * brightness/brighteningCycles;
                        break;
                    case 2:  // orange
                        m_leds[idx].red = 200 * brightness/brighteningCycles;
                        m_leds[idx].green = 120 * brightness/brighteningCycles;
                        m_leds[idx].blue = 0 * brightness/brighteningCycles;
                        break;
                    case 3:  // blue
                        m_leds[idx].red = 10 * brightness/brighteningCycles;
                        m_leds[idx].green = 10 * brightness/brighteningCycles;
                        m_leds[idx].blue = 200 * brightness/brighteningCycles;
                        break;
                    case 4:  // magenta
                        m_leds[idx].red = 200 * brightness/brighteningCycles;
                        m_leds[idx].green = 64 * brightness/brighteningCycles;
                        m_leds[idx].blue = 145 * brightness/brighteningCycles;
                        break;
                }
            }
            else
            {
                // fade the 3/4 of LEDs that we are not currently brightening
                fade(&m_leds[idx].red, 3);
                fade(&m_leds[idx].green, 3);
                fade(&m_leds[idx].blue, 3);
            }
        }
    }
}


// Helper function for adjusting the m_leds for the BrightTwinkle
// and ColorExplosion patterns.  Odd m_leds get brighter and even
// m_leds get dimmer.
static void brightTwinkleColorAdjust(unsigned char *color)
{
    if (*color == 255)
    {
        // if reached max brightness, set to an even value to start fade
        *color = 254;
    }
    else if (*color % 2)
    {
        // if odd, approximately double the brightness
        // you should only use odd values that are of the form 2^n-1,
        // which then gets a new value of 2^(n+1)-1
        // using other odd values will break things
        *color = *color * 2 + 1;
    }
    else if (*color > 0)
    {
        fade(color, 4);
        if (*color % 2)
        {
            (*color)--;  // if faded color is odd, subtract one to keep it even
        }
    }
}


// Helper function for adjusting the m_leds for the ColorExplosion
// pattern.  Odd m_leds get brighter and even m_leds get dimmer.
// The propChance argument determines the likelihood that neighboring
// LEDs are put into the brightening stage when the central LED color
// is 31 (chance is: 1 - 1/(propChance+1)).  The neighboring LED m_leds
// are pointed to by leftColor and rightColor (it is not important that
// the leftColor LED actually be on the "left" in your setup).
static void colorExplosionColorAdjust(unsigned char *color, unsigned char propChance,
                               unsigned char *leftColor, unsigned char *rightColor)
{
    if (*color == 31 && random(propChance+1) != 0)
    {
        if (leftColor != 0 && *leftColor == 0)
        {
            *leftColor = 1;  // if left LED exists and color is zero, propagate
        }
        if (rightColor != 0 && *rightColor == 0)
        {
            *rightColor = 1;  // if right LED exists and color is zero, propagate
        }
    }
    brightTwinkleColorAdjust(color);
}


// ***** PATTERN ColorExplosion *****
// This function creates bursts of expanding, overlapping m_leds by
// randomly picking LEDs to brighten and then fade away.  As these LEDs
// brighten, they have a chance to trigger the same process in
// neighboring LEDs.  The color of the burst is randomly chosen from
// among red, green, blue, and white.  If a red burst meets a green
// burst, for example, the overlapping portion will be a shade of yellow
// or orange.
// When true, the noNewBursts argument changes prevents the generation
// of new bursts; this can be used for a fade-out effect.
// This function uses a very similar algorithm to the BrightTwinkle
// pattern.  The main difference is that the random twinkling LEDs of
// the BrightTwinkle pattern do not propagate to neighboring LEDs.
void LEDPatterns::colorExplosion(bool noNewBursts)
{
    
#if PATTERN_EDITOR
    // slow it down..
    if (!shouldUpdatePattern()) {
        return;
    }
#endif
    // adjust the colors of the first LED
    colorExplosionColorAdjust(&m_leds[0].red, 9, (unsigned char*)0, &m_leds[1].red);
    colorExplosionColorAdjust(&m_leds[0].green, 9, (unsigned char*)0, &m_leds[1].green);
    colorExplosionColorAdjust(&m_leds[0].blue, 9, (unsigned char*)0, &m_leds[1].blue);
    
    for (int i = 1; i < m_ledCount - 1; i++)
    {
        // adjust the m_leds of second through second-to-last LEDs
        colorExplosionColorAdjust(&m_leds[i].red, 9, &m_leds[i-1].red, &m_leds[i+1].red);
        colorExplosionColorAdjust(&m_leds[i].green, 9, &m_leds[i-1].green, &m_leds[i+1].green);
        colorExplosionColorAdjust(&m_leds[i].blue, 9, &m_leds[i-1].blue, &m_leds[i+1].blue);
    }
    
    // adjust the m_leds of the last LED
    colorExplosionColorAdjust(&m_leds[m_ledCount-1].red, 9, &m_leds[m_ledCount-2].red, (unsigned char*)0);
    colorExplosionColorAdjust(&m_leds[m_ledCount-1].green, 9, &m_leds[m_ledCount-2].green, (unsigned char*)0);
    colorExplosionColorAdjust(&m_leds[m_ledCount-1].blue, 9, &m_leds[m_ledCount-2].blue, (unsigned char*)0);
    
    if (!noNewBursts)
    {
        // if we are generating new bursts, randomly pick one new LED
        // to light up
        for (int i = 0; i < 1; i++)
        {
            int j = random(m_ledCount);  // randomly pick an LED
            
            switch(random(7))  // randomly pick a color
            {
                    // 2/7 chance we will spawn a red burst here (if LED has no red component)
                case 0:
                case 1:
                    if (m_leds[j].red == 0)
                    {
                        m_leds[j].red = 1;
                    }
                    break;
                    
                    // 2/7 chance we will spawn a green burst here (if LED has no green component)
                case 2:
                case 3:
                    if (m_leds[j].green == 0)
                    {
                        m_leds[j].green = 1;
                    }
                    break;
                    
                    // 2/7 chance we will spawn a white burst here (if LED is all off)
                case 4:
                case 5:
                    if ((m_leds[j].red == 0) && (m_leds[j].green == 0) && (m_leds[j].blue == 0))
                    {
                        m_leds[j] =  CRGB(1,1,1);
                    }
                    break;
                    
                    // 1/7 chance we will spawn a blue burst here (if LED has no blue component)
                case 6:
                    if (m_leds[j].blue == 0)
                    {
                        m_leds[j].blue = 1;
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
}


// ***** PATTERN Gradient *****
// This function creates a scrolling color gradient that smoothly
// transforms from red to white to green back to white back to red.
// This pattern is overlaid with waves of brightness and dimness that
// scroll at twice the speed of the color gradient.
void LEDPatterns::pololuGradient()
{
    unsigned int j = 0;
    
    // populate colors array with full-brightness gradient colors
    // (since the array indices are a function of m_loopCount, the gradient
    // colors scroll over time)
    while (j < m_ledCount)
    {
        // transition from red to green over 8 LEDs
        for (int i = 0; i < 8; i++)
        {
            if (j >= m_ledCount){
                break;
            }
            CRGB c;
            c.red = (byte)(160 - 20*i);
            c.green = (byte)(20*i);
            c.blue = (byte)((160 - 20*i)*20*i/160);
            m_leds[(m_loopCount/2 + j + m_ledCount)%m_ledCount] = c;
            
            j++;
        }
        // transition from green to red over 8 LEDs
        for (int i = 0; i < 8; i++)
        {
            if (j >= m_ledCount){ break; }
            CRGB c;
            c.red = (byte)(20*i);
            c.green = (byte)(160 - 20*i);
            c.blue = (byte)((160 - 20*i)*20*i/160);
            m_leds[(m_loopCount/2 + j + m_ledCount)%m_ledCount] = c;
            j++;
        }
    }
    
    // modify the colors array to overlay the waves of dimness
    // (since the array indices are a function of m_loopCount, the waves
    // of dimness scroll over time)
    const unsigned char fullDarkLEDs = 10;  // number of LEDs to leave fully off
    const unsigned char fullBrightLEDs = 5;  // number of LEDs to leave fully bright
    const unsigned char cyclePeriod = 14 + fullDarkLEDs + fullBrightLEDs;
    
    // if m_ledCount is not an exact multiple of our repeating pattern size,
    // it will not wrap around properly, so we pick the closest LED count
    // that is an exact multiple of the pattern period (cyclePeriod) and is not
    // smaller than the actual LED count.
    unsigned int extendedLEDCount = (((m_ledCount-1)/cyclePeriod)+1)*cyclePeriod;
    
    j = 0;
    while (j < extendedLEDCount)
    {
        unsigned int idx;
        
        // progressively dim the LEDs
        for (int i = 1; i < 8; i++)
        {
            idx = (j + m_loopCount) % extendedLEDCount;
            if (j++ >= extendedLEDCount){ return; }
            if (idx >= m_ledCount){ continue; }
            
            m_leds[idx].red >>= i;
            m_leds[idx].green >>= i;
            m_leds[idx].blue >>= i;
        }
        
        // turn off these LEDs
        for (int i = 0; i < fullDarkLEDs; i++)
        {
            idx = (j + m_loopCount) % extendedLEDCount;
            if (j++ >= extendedLEDCount){ return; }
            if (idx >= m_ledCount){ continue; }
            
            m_leds[idx].red = 0;
            m_leds[idx].green = 0;
            m_leds[idx].blue = 0;
        }
        
        // progressively bring these LEDs back
        for (int i = 0; i < 7; i++)
        {
            idx = (j + m_loopCount) % extendedLEDCount;
            if (j++ >= extendedLEDCount){ return; }
            if (idx >= m_ledCount){ continue; }
            
            m_leds[idx].red >>= (7 - i);
            m_leds[idx].green >>= (7 - i);
            m_leds[idx].blue >>= (7 - i);
        }
        
        // skip over these LEDs to leave them at full brightness
        j += fullBrightLEDs;
    }
}


// ***** PATTERN BrightTwinkle *****
// This function creates a sparkling/twinkling effect by randomly
// picking LEDs to brighten and then fade away.  Possible m_leds are:
//   white, red, green, blue, yellow, cyan, and magenta
// numColors is the number of m_leds to generate, and minColor
// indicates the starting point (white is 0, red is 1, ..., and
// magenta is 6), so colors generated are all of those from minColor
// to minColor+numColors-1.  For example, calling brightTwinkle(2, 2, 0)
// will produce green and blue twinkles only.
// When true, the noNewBursts argument changes prevents the generation
// of new twinkles; this can be used for a fade-out effect.
// This function uses a very similar algorithm to the ColorExplosion
// pattern.  The main difference is that the random twinkling LEDs of
// this BrightTwinkle pattern do not propagate to neighboring LEDs.
void LEDPatterns::brightTwinkle(unsigned char minColor, unsigned char numColors, unsigned char noNewBursts)
{
#if PATTERN_EDITOR
    // slow it down..
    if (!shouldUpdatePattern()) {
        return;
    }
#endif

    
    // Note: the colors themselves are used to encode additional state
    // information.  If the color is one less than a power of two
    // (but not 255), the color will get approximately twice as bright.
    // If the color is even, it will fade.  The sequence goes as follows:
    // * Randomly pick an LED.
    // * Set the color(s) you want to flash to 1.
    // * It will automatically grow through 3, 7, 15, 31, 63, 127, 255.
    // * When it reaches 255, it gets set to 254, which starts the fade
    //   (the fade process always keeps the color even).
    for (int i = 0; i < m_ledCount; i++)
    {
        brightTwinkleColorAdjust(&m_leds[i].red);
        brightTwinkleColorAdjust(&m_leds[i].green);
        brightTwinkleColorAdjust(&m_leds[i].blue);
    }
    
    if (!noNewBursts)
    {
        // if we are generating new twinkles, randomly pick four new LEDs
        // to light up
        for (int i = 0; i < 4; i++)
        {
            int j = random(m_ledCount);
            if (m_leds[j].red == 0 && m_leds[j].green == 0 && m_leds[j].blue == 0)
            {
                // if the LED we picked is not already lit, pick a random
                // color for it and seed it so that it will start getting
                // brighter in that color
                CRGB c = CRGB(0, 0, 0);
                switch (random(numColors) + minColor)
                {
                    case 0:
                        c.red = 1;
                        c.green = 1;
                        c.blue = 1;
                        break;
                    case 1:
                        c.red = 1;
                        break;
                    case 2:
                        c.green = 1;
                        break;
                    case 3:
                        c.blue = 1;
                        break;
                    case 4:
                        c.red = 1;
                        c.green = 1;
                        break;
                    case 5:
                        c.green = 1;
                        c.blue = 1;
                        break;
                    case 6:
                        c.red = 1;
                        c.blue = 1;
                        //                        m_leds[j] = (rgb_color){1, 0, 1};  // magenta
                        break;
                    default:
                        c.red = 1;
                        c.green = 1;
                        c.blue = 1;
                        break;
                }
                m_leds[j] = c;
            }
        }
    }
}


// ***** PATTERN Collision *****
// This function spawns streams of color from each end of the strip
// that collide, at which point the entire strip flashes bright white
// briefly and then fades.  Unlike the other patterns, this function
// maintains a lot of complicated state data and tells the main loop
// when it is done by returning 1 (a return value of 0 means it is
// still in progress).
unsigned char LEDPatterns::collision()
{
#if PATTERN_EDITOR
    // slow it down..
    if (!shouldUpdatePattern()) {
        return 0;
    }
#endif
    const unsigned char maxBrightness = 180;  // max brightness for the colors
    const unsigned char numCollisions = 5;  // # of collisions before pattern ends
    
    if (m_loopCount == 0)
    {
        m_state = 0;
    }
    
    if (m_state % 3 == 0)
    {
        CRGB firstColor;
        // initialization state
        switch (m_state/3)
        {
            case 0:  // first collision: red streams
                firstColor.red = maxBrightness;
                firstColor.green = 0;
                firstColor.blue = 0;
                break;
            case 1:  // second collision: green streams
                firstColor.red = 0;
                firstColor.green = maxBrightness;
                firstColor.blue = 0;
                break;
            case 2:  // third collision: blue streams
                firstColor.red = 0;
                firstColor.green = 0;
                firstColor.blue = maxBrightness;
                break;
            case 3:  // fourth collision: warm white streams
                firstColor.red = maxBrightness;
                firstColor.green = maxBrightness*4/5;
                firstColor.blue = maxBrightness >> 3;
                break;
            default:  // fifth collision and beyond: random-color streams
                firstColor.red = static_cast<uint8_t>(random(maxBrightness));
                firstColor.green = static_cast<uint8_t>(random(maxBrightness));
                firstColor.blue = static_cast<uint8_t>(random(maxBrightness));
                break;
                
        }
        m_leds[0] = firstColor;
        
        // stream is led by two full-white LEDs
        m_leds[1] = m_leds[2] = CRGB::White;
        // make other side of the strip a mirror image of this side
        m_leds[m_ledCount - 1] = m_leds[0];
        m_leds[m_ledCount - 2] = m_leds[1];
        m_leds[m_ledCount - 3] = m_leds[2];
        
        m_state++;  // advance to next state
        m_count = 8;  // pick the first value of count that results in a startIdx of 1 (see below)
        return 0;
    }
    
    if (m_state % 3 == 1)
    {
        // stream-generation state; streams accelerate towards each other
        unsigned int startIdx = m_count*(m_count + 1) >> 6;
        unsigned int stopIdx = startIdx + (m_count >> 5);
        m_count++;
        if (startIdx < (m_ledCount + 1)/2)
        {
            // if streams have not crossed the half-way point, keep them growing
            for (int i = 0; i < startIdx-1; i++)
            {
                // start fading previously generated parts of the stream
                fade(&m_leds[i].red, 5);
                fade(&m_leds[i].green, 5);
                fade(&m_leds[i].blue, 5);
                fade(&m_leds[m_ledCount - i - 1].red, 5);
                fade(&m_leds[m_ledCount - i - 1].green, 5);
                fade(&m_leds[m_ledCount - i - 1].blue, 5);
            }
            for (int i = startIdx; i <= stopIdx; i++)
            {
                // generate new parts of the stream
                if (i >= (m_ledCount + 1) / 2)
                {
                    // anything past the halfway point is white
                    m_leds[i] = CRGB::White;
                }
                else
                {
                    m_leds[i] = m_leds[i-1];
                }
                // make other side of the strip a mirror image of this side
                m_leds[m_ledCount - i - 1] = m_leds[i];
            }
            // stream is led by two full-white LEDs
            m_leds[stopIdx + 1] = m_leds[stopIdx + 2] = CRGB::White;
            // make other side of the strip a mirror image of this side
            m_leds[m_ledCount - stopIdx - 2] = m_leds[stopIdx + 1];
            m_leds[m_ledCount - stopIdx - 3] = m_leds[stopIdx + 2];
        }
        else
        {
            // streams have crossed the half-way point of the strip;
            // flash the entire strip full-brightness white (ignores maxBrightness limits)
            for (int i = 0; i < m_ledCount; i++)
            {
                m_leds[i] = CRGB::White;
            }
            m_state++;  // advance to next state
        }
        return 0;
    }
    
    if (m_state % 3 == 2)
    {
        // fade state
        if (m_leds[0].red == 0 && m_leds[0].green == 0 && m_leds[0].blue == 0)
        {
            // if first LED is fully off, advance to next state
            m_state++;
            
            // after numCollisions collisions, this pattern is done
            return m_state == 3*numCollisions;
        }
        
        // fade the LEDs at different rates based on the state
        for (int i = 0; i < m_ledCount; i++)
        {
            switch (m_state/3)
            {
                case 0:  // fade through green
                    fade(&m_leds[i].red, 3);
                    fade(&m_leds[i].green, 4);
                    fade(&m_leds[i].blue, 2);
                    break;
                case 1:  // fade through red
                    fade(&m_leds[i].red, 4);
                    fade(&m_leds[i].green, 3);
                    fade(&m_leds[i].blue, 2);
                    break;
                case 2:  // fade through yellow
                    fade(&m_leds[i].red, 4);
                    fade(&m_leds[i].green, 4);
                    fade(&m_leds[i].blue, 3);
                    break;
                case 3:  // fade through blue
                    fade(&m_leds[i].red, 3);
                    fade(&m_leds[i].green, 2);
                    fade(&m_leds[i].blue, 4);
                    break;
                default:  // stay white through entire fade
                    fade(&m_leds[i].red, 4);
                    fade(&m_leds[i].green, 4);
                    fade(&m_leds[i].blue, 4);
            }
        }
    }

    return 0;
}


// ***** PATTERN WarmWhiteShimmer *****
// This function randomly increases or decreases the brightness of the
// even red LEDs by changeAmount, capped at maxBrightness.  The green
// and blue LED values are set proportional to the red value so that
// the LED color is warm white.  Each odd LED is set to a quarter the
// brightness of the preceding even LEDs.  The dimOnly argument
// disables the random increase option when it is true, causing
// all the LEDs to get dimmer by changeAmount; this can be used for a
// fade-out effect.
void LEDPatterns::warmWhiteShimmer()
{
    bool dimOnly = false;
    const unsigned char maxBrightness = 120;  // cap on LED brighness
    const unsigned char changeAmount = 2;   // size of random walk step
    
    for (int i = 0; i < m_ledCount; i += 2)
    {
        // randomly walk the brightness of every even LED
        // g r b
        randomWalk(&m_leds[i].red, maxBrightness, changeAmount, dimOnly ? 1 : 2);
        
        
        // warm white: red = x, green = 0.8x, blue = 0.125x
        m_leds[i].green = m_leds[i].red*4/5;  // green = 80% of red
        m_leds[i].blue = m_leds[i].red >> 3;  // blue = red/8
        
        // every odd LED gets set to a quarter the brighness of the preceding even LED
        if (i + 1 < m_ledCount)
        {
            CRGB c = m_leds[i];
            c.red = c.red >> 2;
            c.green = c.green >> 2;
            c.blue = c.blue >> 2;
            m_leds[i+1] = c;
        }
    }
}

#define WRAP_AROUND(pixel, count) if (pixel < 0) pixel += count; if (pixel >= count) pixel -= count;

float waveValueForTime(float ledCount, float time, float duration, int initialPixel) {
    float numberOfWaves = 2;
    float A = ledCount / 2.0;
    float x = time;
    float totalTime = duration;
    float waveLength = totalTime / numberOfWaves;
    float k = (2*M_PI)/waveLength;
    //    float a = (numberOfWaves/totalTime)*1.8;  // a is the rate of decay
    
    //    float y = A * exp(-a*x) *sin(k*x) + A;
    
    // direct fade with negative sloping line; linear drop off
    // y = (m*x+b)*sin(k*x)+A
    
    float b = ledCount / 2.0; // A
    float m = -b/totalTime;
    float y = (m*x+b)*sin(k*x)+A;
    
    // Offset randomly each iteration
    y = round(y); // round..floor, what should i do?
    
    //    y += initialPixel;
    //    if (y >= ledCount) {
    //        // wrap
    //        y -= ledCount;
    //    }
    return y;
}

void LEDPatterns::wavePatternWithColor(CRGB c, int initialPixel) {
    int mainPixel = waveValueForTime(m_ledCount, m_timePassed, m_duration, initialPixel);
    // consider the offset
    int tmp = mainPixel + initialPixel;
    WRAP_AROUND(tmp, m_ledCount);
    
    setPixelColor(tmp/*mainPixel*/, c);
    
    // Calculate the time a bit ago
    float step = m_duration * .043; // x% ago -- this determines the "tail length" 0.04 is good
    float timePassed = m_timePassed - step;
    if (timePassed < 0) {
        timePassed = 0;
    }
    
    int pixel = waveValueForTime(m_ledCount, timePassed, m_duration, initialPixel);
    float pixelsToFadeOver = mainPixel - pixel;
    // fade to that one..
    if (pixelsToFadeOver != 0) {
        // going up
        while (pixel != mainPixel) {
            float dist = mainPixel - pixel;
            float intensity = 1 - (dist / pixelsToFadeOver);
            // TODO: this could be made much faster and more effecient by using the CRGB stuff
            CRGB tmpColor = c;
            tmpColor.r *= intensity;
            tmpColor.g *= intensity;
            tmpColor.b *= intensity;
            
            int tmp = pixel + initialPixel;
            WRAP_AROUND(tmp, m_ledCount);
            
            setPixelColor(tmp /*pixel*/, tmpColor);
            if (pixel < mainPixel) {
                pixel++;
            } else {
                pixel--;
            }
        }
    }
}

void LEDPatterns::wavePattern() {
    // Reset on 0 time..
    if (m_firstTime) {
        m_initialPixel = random(m_ledCount);
        
        float inc = m_ledCount / 4.0;
        
        m_initialPixel1 = m_initialPixel + inc;
        while (m_initialPixel1 > m_ledCount) m_initialPixel1 -= m_ledCount;
        
        m_initialPixel2 = m_initialPixel1 + inc;
        while (m_initialPixel2 > m_ledCount) m_initialPixel2 -= m_ledCount;
        
        m_initialPixel3 = m_initialPixel2 + inc;
        while (m_initialPixel3 > m_ledCount) m_initialPixel3 -= m_ledCount;
        //        initialPixel1 = random(m_ledCount);
        //        initialPixel2 = random(m_ledCount);
        //        initialPixel3 = random(m_ledCount);
        
        //        randColor1 = (uint8_t)random(255) << 16 | (uint8_t)random(255) << 8 | (uint8_t)random(255); //        0xFF0000; // red
        //        randColor2 = (uint8_t)random(255) << 16 | (uint8_t)random(255) << 8 | (uint8_t)random(255); //        0xFF0000; // red
        //        randColor3 = (uint8_t)random(255) << 16 | (uint8_t)random(255) << 8 | (uint8_t)random(255); //        0xFF0000; // red
        CHSV hsv;
        if (m_stateInfoCount > 0) {
            // replace the original color
            hsv = CHSV(random(HUE_MAX_RAINBOW), 255, 255);
            hsv2rgb_rainbow(hsv, m_patternColor);

            int inc = HUE_MAX_RAINBOW / 4;
            
            hsv.hue += inc; // Wraps
            hsv2rgb_rainbow(hsv, m_randColor1);

            hsv.hue += inc; // Wraps
            hsv2rgb_rainbow(hsv, m_randColor2);

            hsv.hue += inc; // Wraps
            hsv2rgb_rainbow(hsv, m_randColor3);
        } else {
            m_randColor1 = m_randColor2 = m_randColor3 = m_patternColor;
        }
        // inc the m_stateInfoCount each "first time" to keep track of each reset and reset the color on each "first time"
        m_stateInfoCount++;
    }
    
    // reset to black
    fill_solid(m_leds, m_ledCount, CRGB::Black);
    
    wavePatternWithColor(m_patternColor, m_initialPixel);
    wavePatternWithColor(m_randColor1, m_initialPixel1);
    wavePatternWithColor(m_randColor2, m_initialPixel2);
    wavePatternWithColor(m_randColor3, m_initialPixel3);
}


void LEDPatterns::bottomGlowFromTopPixel(float topPixel) {
    // 60 degress of fading in and out
    int fadeCount = (float)m_ledCount * (60.0/360.0);
    // x degress of full on
    int fullCount = (float)m_ledCount * (90.0/360.0);
    // What is left is black
    int offCount = m_ledCount - fullCount - 2*fadeCount;
    // Start past the off count
    int halfOffCount = floor((float)offCount / 2.0);
    
    int wholeTopPixel = floor(topPixel);
    //    float fractionalToTheNextPixel = topPixel - (float)wholeTopPixel;
    
    int pixel = wholeTopPixel + halfOffCount;
    
    // Fade on
    for (int i = 0; i < fadeCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        float percentage = (float)(i + 1) / (float)fadeCount;
        CRGB c = m_patternColor;
        c.red *= percentage;
        c.green *= percentage;
        c.blue *= percentage;
        setPixelColor(pixel, c);
        pixel++;
    }
    
    // Full on
    for (int i = 0; i < fullCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        setPixelColor(pixel, m_patternColor);
        pixel++;
    }
    
    // Fade off
    for (int i = 0; i < fadeCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        float percentage = 1.0 - ((float)i / (float)fadeCount);
        CRGB c = m_patternColor;
        c.red *= percentage;
        c.green *= percentage;
        c.blue *= percentage;;
        setPixelColor(pixel, c);
        pixel++;
    }
    
    // off top portion
    for (int i = 0; i < offCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        setPixelColor(pixel, CRGB::Black);
        pixel++;
    }
    
}

void LEDPatterns::bottomGlow() {
    // Set once, and done. A cheap pattern.
    if (m_firstTime) {
        bottomGlowFromTopPixel(0); // glow from pixel 0..
    } else {
        m_needsInternalShow = false;
    }
}

void LEDPatterns::blinkPattern() {
    if (getPercentagePassed() < 0.5) {
        fill_solid(m_leds, m_ledCount, m_patternColor);
    } else {
        fill_solid(m_leds, m_ledCount, CRGB::Black);
    }
}


//static CRGB HeatColorBlue( uint8_t temperature)
//{
//    CRGB heatcolor;
//    
//    // Scale 'heat' down from 0-255 to 0-191,
//    // which can then be easily divided into three
//    // equal 'thirds' of 64 units each.
//    uint8_t t192 = scale8_video( temperature, 192);
//    
//    // calculate a value that ramps up from
//    // zero to 255 in each 'third' of the scale.
//    uint8_t heatramp = t192 & 0x3F; // 0..63
//    heatramp <<= 2; // scale up to 0..252
//    
//    // now figure out which third of the spectrum we're in:
//    if( t192 & 0x80) {
//        // we're in the hottest third
//        heatcolor.r = heatramp; // full red
//        heatcolor.g = 255; // full green
//        heatcolor.b = 255; // ramp up blue
//        
//    } else if( t192 & 0x40 ) {
//        // we're in the middle third
//        heatcolor.r = 0; // full red
//        heatcolor.g = heatramp; // ramp up green
//        heatcolor.b = 255; // no blue
//        
//    } else {
//        // we're in the coolest third
//        heatcolor.r = 0; //
//        heatcolor.g = 0; //
//        heatcolor.b = heatramp; //
//    }
//    
//    return heatcolor;
//}

// For 60-htz based patterns
bool LEDPatterns::shouldUpdatePattern() {
    if (m_firstTime) {
        m_timedPattern = millis(); // Use for timing
        m_needsInternalShow = false; // Avoids work..
        return true;
    } else {
        // Update every 1/60 second
        uint32_t now = millis();
        if (now - m_timedPattern >= ((1.0/60.0)*1000.0)) {
            // enough time passed!
            m_timedPattern = now;
            return true;
        } else {
            // not enough time passed;...
            return false;
        }
    }

}

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING  80

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 130

void LEDPatterns::fireColorWithPalette(const CRGBPalette16& pal, int cooling, int sparking) {
    if (!shouldUpdatePattern()) {
        return;
    }
    // Array of temperature readings at each simulation cell
    byte *heat = (byte *)getTempBuffer1();
    byte *heat2 = (byte *)getTempBuffer2();
    
    if (m_firstTime) {
        for (int i = 0; i < m_ledCount; i++) {
            heat[i] = 0; // random8(255);
            heat2[i] = 0;
        }
    }
    
    int count = m_ledCount / 2;
    int secondHalfCount = m_ledCount - count;
    if (secondHalfCount > count) {
        count = secondHalfCount;
    }
    
    // Step 1.  Cool down every cell a little
    for( int i = 0; i < count; i++) {
        heat[i] = qsub8( heat[i],  random(0, ((cooling * 10) / count) + 2));
        heat2[i] = qsub8( heat2[i],  random(0, ((cooling * 10) / count) + 2));
    }
    
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= count - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
        heat2[k] = (heat2[k - 1] + heat2[k - 2] + heat2[k - 2] ) / 3;
    }
//    for( int k= NUM_LEDS - 1; k >= 2; k--) {
//        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
//    }
    
    
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random(255) < sparking ) {
        int y = random(7);
        heat[y] = qadd8( heat[y], random(160,255) );
        y = random(7);
        heat2[y] = qadd8( heat2[y], random(160,255) );
    }
    
    // Step 4.  Map from heat cells to LED colors
    int k = m_ledCount - 1;
    for( int j = 0; j < count; j++) {
        byte colorindex = scale8( heat[j], 240);
        m_leds[j] = ColorFromPalette(pal, colorindex);
        
        byte colorindex2 = scale8( heat2[j], 240);
        m_leds[k] = ColorFromPalette(pal, colorindex2);
        k--;
    }
}

void LEDPatterns::firePatternWithColor(bool blue) {

    if (blue) {
        static const CRGBPalette16 bluePal = CRGBPalette16( CRGB::Black, CRGB::Blue, CRGB::Aqua,  CRGB::White);
        fireColorWithPalette(bluePal, COOLING, SPARKING);
    } else {
        fireColorWithPalette(HeatColors_p, COOLING, SPARKING);
    }
    
    return;
}

void LEDPatterns::firePattern() {
    firePatternWithColor(false);
}

void LEDPatterns::blueFirePattern() {
    firePatternWithColor(true);
}

void LEDPatterns::crossFadeToNextPattern() {
    CRGB *startingBuffer = getTempBuffer1();
    CRGB *endingBuffer = getTempBuffer2();
    
    if (m_firstTime) {
        // Copy whatever is there starting to our temp buffer
        // First pass, store off the initial state..
        memcpy(startingBuffer, m_leds, getBufferSize());
        
        // Run one tick of the next pattern...This won't work if the next pattern is another crossfade..
        if (m_nextPatternType != LEDPatternTypeCrossfade) {
            updateLEDsForPatternType(m_nextPatternType);
        }

        // Then do the same for the dest buffer
        memcpy(endingBuffer, m_leds, getBufferSize());
    }
    // Now smoothly crossfade the alpha of one to the other over our duration
    float percentage = getPercentagePassed();
    int alpha = 255 - percentage*255; // value from 0-255
    alpha++; // Value from 1-256, so we can shift instead of multiply or divide
    int inverse = 257 - alpha; // value for the fade in

    for (int i= 0; i < m_ledCount; i++) {
        m_leds[i].red = (startingBuffer[i].red * alpha + endingBuffer[i].red*inverse) >> 8;
        m_leds[i].green = (startingBuffer[i].green * alpha + endingBuffer[i].green*inverse) >> 8;
        m_leds[i].blue = (startingBuffer[i].blue * alpha + endingBuffer[i].blue*inverse) >> 8;
    }
}


// The fixed-point sine and cosine functions use marginally more
// conventional units, equal to 1/2 degree (720 units around full circle),
// chosen because this gives a reasonable resolution for the given output
// range (-127 to +127).  Sine table intentionally contains 181 (not 180)
// elements: 0 to 180 *inclusive*.  This is normal.

byte sineTable[181]  = {
    0,  1,  2,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 27, 28, 29, 30, 31, 32, 33, 34,
    35, 36, 37, 38, 39, 40, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67,
    67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 77, 78, 79, 80, 81,
    82, 83, 83, 84, 85, 86, 87, 88, 88, 89, 90, 91, 92, 92, 93, 94,
    95, 95, 96, 97, 97, 98, 99,100,100,101,102,102,103,104,104,105,
    105,106,107,107,108,108,109,110,110,111,111,112,112,113,113,114,
    114,115,115,116,116,117,117,117,118,118,119,119,120,120,120,121,
    121,121,122,122,122,123,123,123,123,124,124,124,124,125,125,125,
    125,125,126,126,126,126,126,126,126,127,127,127,127,127,127,127,
    127,127,127,127,127
};

char fixSin(int angle) {
    angle %= 720;               // -719 to +719
    if(angle < 0) angle += 720; //    0 to +719
    return (angle <= 360) ?
        sineTable[(angle <= 180) ?
                             angle          : // Quadrant 1
                             (360 - angle)] : // Quadrant 2
        -sineTable[(angle <= 540) ?
                              (angle - 360)   : // Quadrant 3
                              (720 - angle)] ; // Quadrant 4
}

char fixCos(int angle) {
    angle %= 720;               // -719 to +719
    if(angle < 0) angle += 720; //    0 to +719
    return (angle <= 360) ?
        ((angle <= 180) ?  (sineTable[180 - angle])  : // Quad 1
         -(sineTable[angle - 180])) : // Quad 2
        ((angle <= 540) ? -(sineTable[540 - angle])  : // Quad 3
         (sineTable[angle - 540])) ; // Quad 4
}


/*static double threeway_max(double a, double b, double c) {
    return MAX(a, MAX(b, c));
}

static  double threeway_min(double a, double b, double c) {
    return MIN(a, MIN(b, c));
}

// TODO: corbin, move to the HSV class stuff
static CHSV rgb2hsv(CRGB in)
{
    double rd = (double) in.r/255;
    double gd = (double) in.g/255;
    double bd = (double) in.b/255;
    double max = threeway_max(rd, gd, bd);
    double min = threeway_min(rd, gd, bd);
    double h, s, l = (max + min) / 2;
    
    if (max == min) {
        h = s = 0; // achromatic
    } else {
        double d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        if (max == rd) {
            h = (gd - bd) / d + (gd < bd ? 6 : 0);
        } else if (max == gd) {
            h = (bd - rd) / d + 2;
        } else if (max == bd) {
            h = (rd - gd) / d + 4;
        } else {
            h = 0;
        }
        h /= 6;
    }
    return CHSV(h*255, s*255, l*255);
}
*/

// https://github.com/adafruit/LPD8806/blob/master/examples/LEDbeltKit_alt/LEDbeltKit_alt.pde
// renderEffect03
void LEDPatterns::flagEffect() {
    if (!shouldUpdatePattern()) {
        return;
    }

    // Data for American-flag-like colors (20 pixels representing
    // blue field, stars and stripes).  This gets "stretched" as needed
    // to the full LED strip length in the flag effect code, below.
    // Can change this data to the colors of your own national flag,
    // favorite sports team colors, etc.  OK to change number of elements.
#define C_RED    CRGB(160,   0,   0)
#define C_WHITE CRGB::White
#define C_BLUE  CRGB(0,   0, 100)
     CRGB flagTable[]  = {
        C_BLUE , C_WHITE, C_BLUE , C_WHITE, C_BLUE , C_WHITE, C_BLUE, C_WHITE, C_BLUE, C_WHITE, C_BLUE,
        C_RED  , C_WHITE, C_RED  , C_WHITE, C_RED  , C_WHITE, C_RED , C_WHITE, C_RED ,
        C_WHITE, C_RED  , C_WHITE, C_RED  , C_WHITE, C_RED, C_WHITE, C_RED };
    
    // Wavy flag effect
    int i, sum, s;
    int  a, b;
    
    if (m_firstTime) {
        m_initialPixel = 720 + random(720); // Wavyness
        m_initialPixel1 = 4 + random(10);    // Wave speed
        m_initialPixel2 = 200 + random(200); // Wave 'puckeryness'
        m_initialPixel3 = 0;                 // Current  position
    }
    for(sum=0, i=0; i<m_ledCount-1; i++) {
        sum += m_initialPixel2 + fixCos(m_initialPixel3 + m_initialPixel * i / m_ledCount);
    }
    
    
    CRGB *pixel = &m_leds[0];
    int flagTableCount = (sizeof(flagTable)) / sizeof(CRGB);
    for(s=0, i=0; i<m_ledCount; i++) {
        int x = 256L * (flagTableCount - 1) * s / sum;
        int idx1 =  (x >> 8);
        int idx2 = ((x >> 8) + 1);
        if (idx2 >= flagTableCount) {
            idx2 = idx1;
        }
        
        // this mixture is wrong; I'm not sure why, or if it is something I'm doing funky..
        b    = (x & 255) + 1;
        a    = 257 - b;
#if HSV_blend
        CHSV firstColor = rgb2hsv(flagTable[idx1]);
        CHSV secondColor = rgb2hsv(flagTable[idx2]);

        CHSV combined;
        combined.hue = (((firstColor.hue) * a) +
                  ((secondColor.hue) * b)) >> 8;
        combined.sat = (((firstColor.sat) * a) +
                  ((secondColor.sat) * b)) >> 8;
        combined.value = (((firstColor.v) * a) +
                  ((secondColor.v) * b)) >> 8;
        
        *pixel = combined;
#endif

#if 1  // RGB_blend
        pixel->red = (((flagTable[idx1].red) * a) +
                  ((flagTable[idx2].red) * b)) >> 8;
        pixel->green = (((flagTable[idx1].green) * a) +
                  ((flagTable[idx2].green) * b)) >> 8;
        pixel->blue = (((flagTable[idx1].blue) * a) +
                  ((flagTable[idx2].blue) * b)) >> 8;
        
#endif
        
#if No_blend
        *pixel = flagTable[idx1];
#endif
        
        s += m_initialPixel2 + fixCos(m_initialPixel3 + m_initialPixel *
                                     i / m_ledCount);
        pixel++;
    }
    
    m_initialPixel3 += m_initialPixel1;
    if (m_initialPixel3 >= 720) m_initialPixel3 -= 720;
}

// finds the right index for a S shaped matrix
// ...maybe you need a different mapping for your setup
int XY(int x, int y, int WIDTH, int HEIGHT) {
    if(y > HEIGHT) { y = HEIGHT; }
    if(y < 0) { y = 0; }
    if(x > WIDTH) { x = WIDTH;}
    if(x < 0) { x = 0; }
    if(x % 2 == 1) {
        return (x * (WIDTH) + (HEIGHT - y -1));
    } else {
        // use that line only, if you have all rows beginning at the same side
        return (x * (WIDTH) + y);
    }
}

//  Bresenham line algorythm
static void Line(int x0, int y0, int x1, int y1, byte color, CRGB *buffer, int WIDTH, int HEIGHT)
{
    int dx =  abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = -abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = dx+dy, e2;
    for(;;){
        buffer[XY(x0, y0, WIDTH, HEIGHT)] += CHSV(color, 255, 255);
        if (x0==x1 && y0==y1) break;
        e2 = 2*err;
        if (e2 > dy) { err += dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

//  creates a little twister for softening
void Spiral(int x,int y, int r, byte dimm, CRGB *leds, int WIDTH, int HEIGHT) {
    for(int d = r; d >= 0; d--) {                // from the outside to the inside
        for(int i = x-d; i <= x+d; i++) {
            leds[XY(i,y-d, WIDTH, HEIGHT)] += leds[XY(i+1,y-d, WIDTH, HEIGHT)];   // lowest row to the right
            leds[XY(i,y-d, WIDTH, HEIGHT)].nscale8( dimm );}
        for(int i = y-d; i <= y+d; i++) {
            leds[XY(x+d,i, WIDTH, HEIGHT)] += leds[XY(x+d,i+1, WIDTH, HEIGHT)];   // right colum up
            leds[XY(x+d,i, WIDTH, HEIGHT)].nscale8( dimm );}
        for(int i = x+d; i >= x-d; i--) {
            leds[XY(i,y+d, WIDTH, HEIGHT)] += leds[XY(i-1,y+d, WIDTH, HEIGHT)];   // upper row to the left
            leds[XY(i,y+d, WIDTH, HEIGHT)].nscale8( dimm );}
        for(int i = y+d; i >= y-d; i--) {
            leds[XY(x-d,i, WIDTH, HEIGHT)] += leds[XY(x-d,i-1, WIDTH, HEIGHT)];   // left colum down
            leds[XY(x-d,i, WIDTH, HEIGHT)].nscale8( dimm );}
    }
}

#define NUMBER_LIFE_OBJECTS 0.10 // percentage
#define NUMBER_BOUNCE_OBJECTS 10

void blur(int amount, CRGB *leds, CRGB *temp, int count) {
    uint8_t t = 10;
    for (int b = 0; b < amount; b++) {
        for(int x = 0; x < count; x++) {
            temp[x] = leds[x] + (leds[(((x-1)%count)+count)%count]%t) + (leds[(x+1)%count]%t);
        }
        memcpy8(leds, temp, sizeof(CRGB)*count); // was sizeof(leds), which is ambiguous..sizeof a pointer???
    }
}

void LEDPatterns::commonInitForPattern() {
    // Initialize...
    // First, free any used memory so we can malloc our large array
    if (m_ledTempBuffer2) {
        free(m_ledTempBuffer2);
        m_ledTempBuffer2 = NULL;
    }
    
    if (m_stateInfo != NULL) {
        free(m_stateInfo);
    }
    
    // 10% of the led count...
    if (m_patternType == LEDPatternTypeBouncingBall) {
        m_stateInfoCount = NUMBER_BOUNCE_OBJECTS; //ceil(NUMBER_BOUNCE_OBJECTS * m_ledCount);
    } else {
        m_stateInfoCount = ceil(NUMBER_LIFE_OBJECTS * m_ledCount);
    }
    m_stateInfo = malloc(m_stateInfoCount * sizeof(LEDStateInfo));
    
    LEDStateInfo *stateInfo = (LEDStateInfo *)m_stateInfo;
    int velocity = m_patternType == LEDPatternTypeLifeDynamic ? 0 : 2000;
    // Go to the dynamic portion....
    for (int i = 0; i < m_stateInfoCount; i++) {
        stateInfo[i] = LEDStateInfo(i, m_ledCount, m_stateInfoCount, velocity); // initialize it
        if (m_patternType == LEDPatternTypeBouncingBall) {
            stateInfo[i].initForBounce(i, m_stateInfoCount);
        }
    }
}

void LEDPatterns::bouncingBallPattern() {
    if (!shouldUpdatePattern()) return;

    if (m_firstTime) {
        commonInitForPattern();
    }
    LEDStateInfo *stateInfo = (LEDStateInfo *)m_stateInfo;

    // Intialize all contents to black
    fill_solid(m_leds, m_ledCount, CRGB::Black);
    
    for (int i = 0; i < m_stateInfoCount; i++) {
        stateInfo[i].updateBounceColor(m_leds, m_ledCount);
    }
    blur(4, m_leds, getTempBuffer1(), m_ledCount);
}

void LEDPatterns::bitmapPatternFillPixels() {
    int xOffset = m_lazyBitmap->getXOffset();
    uint32_t imageWidth = m_lazyBitmap->getWidth();
    const CRGB *imageLineData = m_lazyBitmap->getFirstBuffer();
    
    // Pixel 0 is at this offset...loop through and update the leds
    int x = 0;
    while (x < m_ledCount) {
        int maxToCopy = m_ledCount - x;
        int availableToCopy = imageWidth - xOffset;
        int amountToCopy = MIN(maxToCopy, availableToCopy);
        memcpy(&m_leds[x], &imageLineData[xOffset], amountToCopy*sizeof(CRGB));
        x += amountToCopy;
        xOffset += amountToCopy;
        if (xOffset >= imageWidth) {
            xOffset = 0;
        }
    }
    ASSERT(x == m_ledCount);
}

void LEDPatterns::bitmapPatternStretchFillPixels() {
    // nearest neighbor, blocky-scaling
    uint32_t imageWidth = m_lazyBitmap->getWidth();
    const CRGB *imageLineData = m_lazyBitmap->getFirstBuffer();
    
    int wholeCopies = m_ledCount / imageWidth;
    int pixelsLeftOver = m_ledCount - (wholeCopies * imageWidth);
    int xOffset = 0;
    int i = 0;
    while (i < m_ledCount) {
        if (xOffset >= imageWidth) {
            // shouldn't hit this..it means we wrapped!
            DEBUG_PRINTLN("ERROR wrap 1");
            xOffset = 0;
        }
        // Repeat it for however many times we are stretching
        for (int j = 0; j < wholeCopies; j++) {
            if (i == m_ledCount) {
                DEBUG_PRINTLN("ERROR 2");
                break;// shouldn't hit this....
            }
            m_leds[i++] = imageLineData[xOffset];
        }
        if (i == m_ledCount && pixelsLeftOver > 0) {
            DEBUG_PRINTLN("ERROR 3");
            break;// shouldn't hit this....
        }
        // distribute the left over at the start; one pixel each before incrementing the x
        if (pixelsLeftOver > 0) {
            pixelsLeftOver--;
            m_leds[i++] = imageLineData[xOffset];
        }
        xOffset++;
    }
    
}

void LEDPatterns::bitmapPatternStretchInterpolFillPixels() {
    // nearest neighbor, blocky-scaling
    uint32_t imageWidth = m_lazyBitmap->getWidth();
    const CRGB *imageLineData = m_lazyBitmap->getFirstBuffer();
    
    int wholeCopies = m_ledCount / imageWidth;
    int pixelsLeftOver = m_ledCount - (wholeCopies * imageWidth);
    int xOffset = 0;
    int i = 0;
    while (i < m_ledCount) {
        if (xOffset >= imageWidth) {
            // shouldn't hit this..it means we wrapped!
            DEBUG_PRINTLN("ERROR wrap 1");
            xOffset = 0;
        }
        
        CRGB first = imageLineData[xOffset];
        CRGB second = imageLineData[xOffset < (imageWidth-1) ? xOffset + 1 : 0];
        
        int thisCount = wholeCopies;
        if (pixelsLeftOver > 0) {
            pixelsLeftOver--;
            thisCount++;
        }
        
        // Repeat it for however many times we are stretching
        for (int j = 0; j < thisCount; j++) {
            if (i == m_ledCount) {
                DEBUG_PRINTLN("ERROR 2");
                break;// shouldn't hit this....
            }
            float percentage = (float)j / (float)thisCount;
            m_leds[i].r = first.r + round(percentage * (float)(second.r - first.r));
            m_leds[i].g = first.g + round(percentage * (float)(second.g - first.g));
            m_leds[i].b = first.b + round(percentage * (float)(second.b - first.b));
            
            i++;
        }

        xOffset++;
    }
    
}

void LEDPatterns::bitmapPatternInterpolatePixels(float percentage, bool isChasingPattern) {
    uint32_t imageWidth = m_lazyBitmap->getWidth();
    CRGB *firstRow = m_lazyBitmap->getFirstBuffer();
    
//    NSLog(@"per: %g fraction: %d", percentage, fraction);
    
    int xOffset = m_lazyBitmap->getXOffset();
    if (isChasingPattern) {
        fract16 fraction = round(percentage*65536); // yeah, hardcoded..oh well, 2^16
        for (int i = 0; i < m_ledCount; i++) {
            int firstOffset = xOffset;
            xOffset++;
            if (xOffset >= imageWidth) {
                xOffset = 0;
            }
            m_leds[i] = firstRow[firstOffset].lerp16(firstRow[xOffset], fraction);
        }
    } else {
        CRGB *secondRow = m_lazyBitmap->getSecondBuffer();
        for (int i = 0; i < m_ledCount; i++) {
//            m_leds[i] = firstRow[xOffset].lerp16(secondRow[xOffset], fraction);
// ^ fast! but not what i wanted...
            CRGB first = firstRow[xOffset];
            CRGB second = secondRow[xOffset];
            m_leds[i].r = first.r + round(percentage * (float)(second.r - first.r));
            m_leds[i].g = first.g + round(percentage * (float)(second.g - first.g));
            m_leds[i].b = first.b + round(percentage * (float)(second.b - first.b));
            
            xOffset++;
            if (xOffset >= imageWidth) {
                xOffset = 0;
            }
        }
    }
}

void LEDPatterns::bitmapPattern() {
    ASSERT(m_lazyBitmap != NULL);
    float percentageThrough = 0;
    // Treat one line bitmaps as a chaser, and multi-line bitmaps as regulars..
    bool isChasingPattern = m_lazyBitmap->getHeight() == 1;
//    Serial.printf("m_duration: %d, m_timePassed: %d\r\n", m_duration, m_timePassed);
    // A chasing pattern; duration of 0 is to run as fast as it can
    if (m_duration == 0) {
        // FAST AS we can..
        if (isChasingPattern) {
            m_lazyBitmap->incXOffset();
        } else {
            m_lazyBitmap->incYOffsetBuffers();
        }
    } else if (m_timePassed >= m_duration) {
        // The sim is dropping frames; this simulates it going faster
        int count = floor(getPercentagePassed());
        if (count > 1) {
#ifndef PATTERN_EDITOR
            DEBUG_PRINTF("dropping %d frames\r\n", count - 1);
#endif
        }
        // Fixup dropped frames??? (I'm not sure if I want to do this..it might be smoother to NOT do it
#ifdef PATTERN_EDITOR
        while (count > 0)
#endif
        {
            if (isChasingPattern) {
                m_lazyBitmap->incXOffset();
            } else {
                m_lazyBitmap->incYOffsetBuffers();
            }
            count--;
        }
        m_startTime = millis(); // resets the clock
    } else if (!m_firstTime) {
        percentageThrough = getPercentagePassed();
    }

    if (!isChasingPattern && m_patternOptions.bitmapOptions.shouldStrechBitmap && m_lazyBitmap->getWidth() < m_ledCount) {
        if (m_patternOptions.bitmapOptions.shouldInterpolate) {
            bitmapPatternStretchInterpolFillPixels();
        } else {
            bitmapPatternStretchFillPixels();
        }
    } else if (m_patternOptions.bitmapOptions.shouldInterpolate && percentageThrough > 0) {
        bitmapPatternInterpolatePixels(percentageThrough, isChasingPattern);
    } else {
        bitmapPatternFillPixels();
    }
}

void LEDPatterns::lifePattern(bool dynamic) {
    if (!shouldUpdatePattern()) return;
    
    if (m_firstTime) {
        commonInitForPattern();
    }
    LEDStateInfo *stateInfo = (LEDStateInfo *)m_stateInfo;
    
    // Intialize all contents to black
    fill_solid(m_leds, m_ledCount, CRGB::Black);

    for (int i = 0; i < m_stateInfoCount; i++) {
        stateInfo[i].update(m_leds, m_ledCount);
    }

    //apply a blur to the LED array //3x = 220us, 16x = 1200us
    blur(3, m_leds, getTempBuffer1(), m_ledCount);
}


void LEDPatterns::funkyCloudsPattern() {
    if (!shouldUpdatePattern()) {
        return;
    }
    // since I found out that integers are casted impicit that part became easy...
    // so just have 4 sinewaves with differnt speeds running

//    m_initialPixel = (uint8_t)(m_initialPixel + 7);
//    m_initialPixel1 = (uint8_t)(m_initialPixel1 + 5);
//    m_initialPixel2 = (uint8_t)(m_initialPixel2 + 3);
//    m_initialPixel3 = (uint8_t)(m_initialPixel3 + 2);
    m_initialPixel = (uint8_t)(m_initialPixel + 4);
    m_initialPixel1 = (uint8_t)(m_initialPixel1 + 3);
    m_initialPixel2 = (uint8_t)(m_initialPixel2 + 2);
    m_initialPixel3 = (uint8_t)(m_initialPixel3 + 1);
    
    int width =  floor(sqrt(m_ledCount));
    int height = width;
    // first plant the seed into the buffer
    //buffer[XY(sin8(m_initialPixel)/20, cos8(m_initialPixel1)/17)] = CHSV (count[0] , 255, 255);
    
    //buffer[XY(sin8(m_initialPixel2)/17, cos8(m_initialPixel3)/17)] = CHSV (m_initialPixel1 , 255, 255);
//    for(int i = 0; i < m_ledCount ; i++) {
////        m_leds[i].fadeToBlackBy(1);
//
//    }
    Line(((sin8(m_initialPixel)/17)+(sin8(m_initialPixel2)/17))/2, ((sin8(m_initialPixel1)/17)+(sin8(m_initialPixel3)/17))/2,         // combine 2 wavefunctions for more variety of x
         ((sin8(m_initialPixel3)/17)+(sin8(m_initialPixel2)/17))/2, ((sin8(m_initialPixel1)/17)+(sin8(m_initialPixel2)/17))/2, m_initialPixel3,
         m_leds, width, height); // and y, color just simple linear ramp
    
    //   just some try and error:
    //Line(sin8(m_initialPixel3)/17, cos8(m_initialPixel1)/17, sin8(count[1])/17, cos8(m_initialPixel2)/17, m_initialPixel3);
//    m_leds[XY(quadwave8(m_ledCount)/17, 4, width, width)] = CHSV (0 , 255, 255); // lines following different wave fonctions
//    m_leds[XY(cubicwave8(m_ledCount)/17, 6, width, width)] = CHSV (40 , 255, 255);
//    m_leds[XY(triwave8(m_ledCount)/17, 8, width, width)] = CHSV (80 , 255, 255);
    
    // duplicate the seed in the buffer
    //Caleidoscope4();
    
    // add buffer to leds
//    for(int i = 0; i < m_ledCount ; i++) {
//        leds[i] += buffer[i];
//    }
    
    // clear buffer
//    ClearBuffer();
//    for(int i = 0; i < m_ledCount ; i++) {
//        m_leds[i] = 0;
//    }
    
    // rotate leds
    Spiral(7,7,10,120, m_leds, width, height);
    
    //DimmAll(230);
    
   // FastLED.show();
    
    // do not delete the current leds, just fade them down for the tail effect
    //DimmAll(220);
}

void LEDPatterns::sinWaveDemoEffect() {
    if (!shouldUpdatePattern()) {
        return;
    }

    if (m_firstTime) {
        m_initialPixel = random(1536); // Random hue
        // Number of repetitions (complete loops around color wheel);
        // any more than 4 per meter just looks too chaotic.
        // Store as distance around complete belt in half-degree units:
        m_initialPixel1 = (1 + random(4 * ((m_ledCount + 31) / 32))) * 720;
        // Frame-to-frame increment (speed) -- may be positive or negative,
        // but magnitude shouldn't be so small as to be boring.  It's generally
        // still less than a full pixel per frame, making motion very smooth.
        m_initialPixel2 = 4 + random(m_initialPixel) / m_ledCount;
        // Reverse direction half the time.
        if(random(2) == 0) m_initialPixel2 = -m_initialPixel2;
        m_initialPixel3 = 0; // Current position
    }
    
    for(int i=0; i<m_ledCount; i++) {
        int foo = fixSin(m_initialPixel3 + m_initialPixel1 * i / m_ledCount);
        // Peaks of sine wave are white, troughs are black, mid-range
        // values are pure hue (100% saturated).
        if (foo >= 0) {
            CHSV hsv = CHSV(m_initialPixel, 254 - (foo * 2), 255);
            hsv2rgb_rainbow(hsv, m_leds[i]);
        } else {
            CHSV hsv = CHSV(m_initialPixel, 255, 254 + foo * 2);
            hsv2rgb_rainbow(hsv, m_leds[i]);
        }
    }
    m_initialPixel3 += m_initialPixel2;
}

void LEDPatterns::rotatingBottomGlow() {
    float topPixel = getPercentagePassed()*m_ledCount;
    bottomGlowFromTopPixel(topPixel);
}

void LEDPatterns::fadeIn(float percentagePassed) {
    // slow fade in with:
    // y = x^2, where x == time
    if (percentagePassed >= 1.0) {
        fill_solid(m_leds, m_ledCount, m_patternColor);
    } else {
        float y = percentagePassed*percentagePassed;
        for (int i = 0; i < m_ledCount; i++) {
            fadePixel(i, m_patternColor, y);
        }
    }
}

CRGB *LEDPatterns::getTempBuffer1() {
    if (m_ledTempBuffer1 == NULL) {
        m_ledTempBuffer1 = (CRGB *)malloc(getBufferSize());
    }
    return m_ledTempBuffer1;
}

CRGB *LEDPatterns::getTempBuffer2() {
    if (m_ledTempBuffer2 == NULL) {
        m_ledTempBuffer2 = (CRGB *)malloc(getBufferSize());
    }
    return m_ledTempBuffer2;
}

void LEDPatterns::fadeOut(float percentagePassed) {
    // Opposite of fade in, and we store the inital value on the first pass
    // y = -x^2 + 1
    CRGB *tempBuffer = getTempBuffer1();
    if (m_firstTime) {
        // First pass, store off the initial state..
        memcpy(tempBuffer, m_leds, getBufferSize());
    }
    
    if (percentagePassed >= 1.0) {
        // All black
        fill_solid(m_leds, m_ledCount, CRGB::Black);
    } else {
        float y = -(percentagePassed*percentagePassed)+1;
        for (int i = 0; i < m_ledCount; i++) {
            // direct pixel access to avoid issues w/reading the already set brightness
            m_leds[i].green = tempBuffer[i].green*y;
            m_leds[i].red = tempBuffer[i].red*y;
            m_leds[i].blue = tempBuffer[i].blue*y;
        }
    }
}

#warning corbin - move to the other rainbow method from CRGB!
static CRGB hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = v * (255 - s) / 255;
    uint8_t q = v * (255 - f * s / 255) / 255;
    uint8_t t = v * (255 - (255 - f) * s / 255) / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch((h / 60) % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return CRGB(r, g, b);
}


void LEDPatterns::solidRainbow(int positionInWheel, int count) {
    int countPerSection = round(m_ledCount / count);
    for (int i = 0; i < m_ledCount; i++) {
//        uint16_t angle = 360.0 * ((float)((i + positionInWheel) / (float)countPerSection));
//        CRGB color = hsvToRgb(angle, 255, 255);
//        setPixelColor(i, color);
        
        uint16_t hue = HUE_MAX_RAINBOW * ((float)((i + positionInWheel) / (float)countPerSection));

        CHSV hsv = CHSV(hue, 255, 255);
        hsv2rgb_rainbow(hsv, m_leds[i]);
    }
}

void LEDPatterns::rainbows(int count) {
    int positionInWheel = round(getPercentagePassed()*m_ledCount);
    solidRainbow(positionInWheel, count);
}


void LEDPatterns::rainbowWithSpaces(int count) {
    int positionInWheel = round(getPercentagePassed()*m_ledCount);
    
    // 8 sections; 4 on, 4 off.
    int pixelsPerGlow = round((float)m_ledCount / 8.0) + 10;
    int pixelsOff = (m_ledCount - (pixelsPerGlow * 4)) / 4.0;
    
    int countPerSection = round(pixelsPerGlow / count);
    
    static const float rampCount = 7;
    
    int pixel = 0;
    for (int i = 0; i < 8; i++) {
        if (pixel == m_ledCount) break;
        for (int j = 0; j < pixelsPerGlow; j++) {
            uint16_t angle = 360.0 * ((float)((pixel + positionInWheel) / (float)countPerSection));
            CRGB color = hsvToRgb(angle, 255, 255);
            if (j < rampCount) {
                float rampUp = (float)(j+1) / rampCount;
                fadePixel(pixel, color, rampUp);
            } else {
                // close to the end?
                int howFarToEnd = pixelsPerGlow - j;
                if (howFarToEnd < rampCount) {
                    float rampUp = (float)(howFarToEnd+1) / rampCount;
                    fadePixel(pixel, color, rampUp);
                } else {
                    setPixelColor(pixel, color);
                }
            }
            pixel++;
            if (pixel == m_ledCount) break;
        }
        if (pixel == m_ledCount) break;
        for (int j = 0; j < pixelsOff; j++) {
            setPixelColor(pixel, CRGB::Black); // off
            pixel++;
            if (pixel == m_ledCount) break;
        }
    }
}


void LEDPatterns::theaterChase() {
    // The duration speeds up or slows down how fast we chase
    int swipePosition = round(getPercentagePassed()*6);
    
    for (int i = 0; i < m_ledCount; i++) {
        // TODO: add options for spacing.
        if ((swipePosition + i) % 5 == 0) {
            setPixelColor(i, m_patternColor);
        } else {
            setPixelColor(i, CRGB::Black);
        }
    }
}

// wipe a color on
void LEDPatterns::colorWipe() {
    int swipePosition = round(getPercentagePassed()*m_ledCount);
    
    for (int i = 0; i < m_ledCount; i++) {
        if (i < swipePosition) {
            setPixelColor(i, m_patternColor);
        } else {
            setPixelColor(i, CRGB::Black);
        }
    }
}

void LEDPatterns::ledGradients() {
    float t = getPercentagePassed();
    
    float numberOfGradients = 8.0;
    // See "Pattern Graphs" (Grapher document)
    float gradCnt = (float)m_ledCount / numberOfGradients;
    float gradRmp = gradCnt / 2.0;
    
    for (int x = 0; x < m_ledCount; x++) {
        float q = (float)x - t*gradCnt;
        float n = fmod(q+gradRmp, gradCnt);
        //        float v = (q+gradRmp) / gradCnt;
        //        double t; // not used
        //        float fractPart = modf(v, &t);
        //        float n = fractPart * gradCnt;
        n = fabs(n); // Needs to be positive
        
        float p = n - gradRmp;
        float s = p/gradRmp;
        float y = s*s; // Squared
        
        fadePixel(x, m_patternColor, y);
    }
}


void LEDPatterns::pulseGradientEffect() {
    
    byte time = millis();
    
    const int gradientUpPixelCount = 16;
    const int minV = 0;
    const int maxV = 255;
    for (int i = 0; i < m_ledCount; i++) {
        // ramp up, then down, gradientUpPixelCount per half
        int totalGradientPixels = gradientUpPixelCount*2;
        int v = i % totalGradientPixels;
        int currentR = (int)map(v, 0, totalGradientPixels-1, minV, maxV*2);
        
        // math mod...
        while (currentR > maxV*2) {
            currentR -= maxV*2;
        }
        
        if (currentR > maxV) {
            // Ramp down the value
            currentR = currentR - (maxV - minV); // This would give a small value..we want a big vaul going down to the minV
            currentR = maxV - currentR; // This gives the ramp down
            //            Serial.print(" ramped: ");
            //            Serial.print(currentR);
        }
        
        byte finalR = time - currentR;  // Vary for time and roll over (byte truncation)
        
        
        byte finalB = 0;
        int wholePasses = i / 16;
        if (wholePasses % 2 == 0) {
            finalB = finalR;
        }
        setPixelColor(i, CRGB(finalR, 0, finalB));
    }
}

int LEDPatterns::gradientOverXPixels(int pixel, int fullCount, int offCount, int fadeCount, CRGB color) {
    for (int i = 0; i < offCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        setPixelColor(pixel, 0);
        pixel++;
    }
    
    // Fade on
    for (int i = 0; i < fadeCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        float percentage = (float)(i + 1) / (float)fadeCount;
        CRGB c = color;
        c.red *= percentage;
        c.green *= percentage;
        c.blue *= percentage;
        setPixelColor(pixel, c);
        pixel++;
    }
    
    // Full on
    for (int i = 0; i < fullCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        setPixelColor(pixel, color);
        pixel++;
    }
    
    // Fade off
    for (int i = 0; i < fadeCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        float percentage = 1.0 - ((float)i / (float)fadeCount);
        CRGB c = color;
        c.red *= percentage;
        c.green *= percentage;
        c.blue *= percentage;
        setPixelColor(pixel, c);
        pixel++;
    }
    
    for (int i = 0; i < offCount; i++) {
        WRAP_AROUND(pixel, m_ledCount);
        setPixelColor(pixel, 0);
        pixel++;
    }
    return pixel;
}


void LEDPatterns::randomGradients() {
    float d = m_duration +4000; // slow it down.?
    float percentagePassed = (float)m_timePassed / d;
    
    CRGB *tmpBuffer = getTempBuffer1();
    
    float numberOfGradients = 8.0;
    
    if (m_firstTime) {
        // generate an initial random color for each gradient
        for (int i = 0; i < numberOfGradients; i++) {
            // Well, not really random..
            CHSV hsv = CHSV(HUE_MAX_RAINBOW*((float)i / (float)numberOfGradients), 255, 255);
            hsv2rgb_rainbow(hsv, tmpBuffer[i]);
        }
    }
    
    int offPixels = 6;
    
    float rampPixels = m_ledCount - (offPixels * numberOfGradients * 2);
    float rampCount = floor((float)rampPixels / numberOfGradients / 2.0);
    
    int pixel = round(percentagePassed*m_ledCount);
    pixel = pixel % m_ledCount;
    
    for (int i = 0; i < numberOfGradients; i++) {
        pixel = gradientOverXPixels(pixel, 0, offPixels, rampCount, tmpBuffer[i]);
    }
    //    // black on extras
    //    while (pixel < m_ledCount) {
    //        g_strip.setPixelColor(pixel, 0, 0, 0);
    //        pixel++;
    //    }
}

#if SD_CARD_SUPPORT

#define USE_TWO_BUFFERS 1


void LEDPatterns::patternImageEntireStrip() {
    bitmapPattern(); // For now..
    /*
    int totalPixelsInImage = m_dataLength / 3; // Assumes RGB encoding, 3 bytes per pixel...this should divide evenly..
    
    if (m_firstTime) {
        initImageDataForHeader();
    }
    
    uint32_t currentOffset = 0;
    uint32_t nextOffsetForBlending = 0;
    float percentageThrough = getPercentagePassed();
    if (!m_firstTime) {
        // Increase the offset by whole amounts once we have enough time passed
        if (totalPixelsInImage > m_ledCount) {
            int numOfCompleteImages = totalPixelsInImage / m_ledCount;
            float percentageThroughOfCompleteImagesThrough = percentageThrough * (float)numOfCompleteImages;
            // Floor it; we always go up from the previous to the next
            int totalWholeCompleteImagesThrough = floor(percentageThroughOfCompleteImagesThrough);
            int wholeCompleteImagesThrough = totalWholeCompleteImagesThrough;
            // wrap
            if (wholeCompleteImagesThrough > numOfCompleteImages) {
                wholeCompleteImagesThrough = wholeCompleteImagesThrough % numOfCompleteImages;
            }
            
            // again...3 bytes per pixel hardcoding!! I think I'd have to decode to this format.
            if (wholeCompleteImagesThrough > 0) {
                currentOffset = currentOffset + (3*m_ledCount*wholeCompleteImagesThrough);
                currentOffset = keepOffsetInDataBounds(currentOffset, m_dataLength); /// Shouldn't happen unless the image length/size is messed up
            }
            // Figure out the blending offset by adding one cycle through
            nextOffsetForBlending = currentOffset + (3*m_ledCount);
            nextOffsetForBlending = keepOffsetInDataBounds(nextOffsetForBlending, m_dataLength);
            // Adjust the percentageThrough to the amount we are through based on this current offset....
            //            percentageThrough = percentageThroughOfCompleteImagesThrough - wholeCompleteImagesThrough;
            percentageThrough = percentageThroughOfCompleteImagesThrough - totalWholeCompleteImagesThrough;
        }
    }
    
    for (int x = 0; x < m_ledCount; x++) {
        // First, bounds check each time... shouldn't be needed unless the size of the image isn't an integral value for the number of pixels.
        currentOffset = keepOffsetInDataBounds(currentOffset, m_dataLength);
        nextOffsetForBlending = keepOffsetInDataBounds(nextOffsetForBlending, m_dataLength);
        
        uint8_t *data = dataForOffset(currentOffset);
        uint8_t r = data[0];
        uint8_t g = data[1];
        uint8_t b = data[2];
        
#if USE_TWO_BUFFERS
        if (currentOffset != nextOffsetForBlending && percentageThrough > 0) {
            // Grab the next color and mix it...
            uint8_t *nextDataForBlending = dataForOffset(nextOffsetForBlending);
            
            uint8_t r2 = nextDataForBlending[0];
            uint8_t g2 = nextDataForBlending[1];
            uint8_t b2 = nextDataForBlending[2];
            
            //            Serial.print("percentage through");
            //            DEBUG_PRINTLN(percentageThrough);
            //            Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n", x, r, g, b);
            //            Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n", x, r2, g2, b2);
            //
            //            DEBUG_PRINTLN();
            
            // TODO: non floating point math.. ??
            r = r + round(percentageThrough * (float)(r2 - r));
            g = g + round(percentageThrough * (float)(g2 - g));
            b = b + round(percentageThrough * (float)(b2 - b));
            //            Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n\r\n", x, r, g, b);
        }
#endif
        
#if 0 // DEBUG
        Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n", x, r, g, b);
#endif
        // dude, this just re-packs it...
        setPixelColor(x, CRGB(r, g, b));
        
        // Again..assumes 3 bytes per pixel
        currentOffset += 3;
        nextOffsetForBlending += 3;
    }
     */
}


void LEDPatterns::linearImageFade() {
    bitmapPattern(); // For now..
/*
    uint32_t currentOffset = 0;
    float percentageThrough = 0;
    if (!m_firstTime) {
        int totalPixelsInImage = m_dataLength / 3; // Assumes RGB encoding, 3 bytes per pixel...this should divide evenly..
        percentageThrough = getPercentagePassed();
        float pixelsThrough = percentageThrough * totalPixelsInImage;
        int wholePixelsThrough = floor(pixelsThrough);
        currentOffset = currentOffset + 3*wholePixelsThrough;
        
        // Fixup percentageThrough to figure out how much to the next we are...
        percentageThrough = pixelsThrough - wholePixelsThrough;
        
    }
    //    DEBUG_PRINTF("LINEAR IMAGE: %.3f\r\n", percentageThrough);
    
    for (int x = 0; x < m_ledCount; x++) {
        // First, bounds check each time
        currentOffset = keepOffsetInDataBounds(currentOffset, m_dataLength);
        
        uint8_t *data = dataForOffset(currentOffset);
        uint8_t r = data[0];
        uint8_t g = data[1];
        uint8_t b = data[2];
        
        currentOffset += 3;
#if 0 // USE_TWO_BUFFERS
        if (percentageThrough > 0) {
            // TOO SLOW ?? this does a nice fade to the next
            // Grab the next color and mix it...
            uint32_t nextColorOffset = keepOffsetInDataBounds(currentOffset);
            uint8_t *nextColorData = dataForOffset(nextColorOffset, itemHeader);
            
            
            uint8_t r2 = nextColorData[0];
            uint8_t g2 = nextColorData[1];
            uint8_t b2 = nextColorData[2];
            
            //            Serial.print("percentage through");
            //            DEBUG_PRINTLN(percentageThrough);
            //            Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n", x, r, g, b);
            //            Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n", x, r2, g2, b2);
            //
            //            DEBUG_PRINTLN();
            
            // TODO: non floating point math.. ??
            r = r + round(percentageThrough * (float)(r2 - r));
            g = g + round(percentageThrough * (float)(g2 - g));
            b = b + round(percentageThrough * (float)(b2 - b));
            //            Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n\r\n", x, r, g, b);
        }
#endif
        
#if 0 // DEBUG
        Serial.printf("x:%d, r:%d, g:%d, b:%d\r\n", x, r, g, b);
#endif
        m_leds[x] = CRGB(r, g, b);
    }
 */
}

#endif  // SD_CARD_SUPPORT

void LEDPatterns::flashThreeTimes(CRGB color, uint32_t delayAmount) {
    for (int i = 0; i < 3; i++) {
        fill_solid(m_leds, m_ledCount, color);
        internalShow();
        delay(delayAmount);
        fill_solid(m_leds, m_ledCount, CRGB::Black);
        internalShow();
        delay(delayAmount);
    }
}

void LEDPatterns::flashOnce(CRGB color) {
    fill_solid(m_leds, m_ledCount, color);
    internalShow();
    delay(250);
}

void LEDPatterns::showProgress(float progress, CRGB color) {
    fill_solid(m_leds, m_ledCount, CRGB::Black);
    int max = round(progress * m_ledCount); // can be > 1.0
    if (max > m_ledCount) {
        max = max % m_ledCount;
    }
    
    for (int i = 0; i < max; i++) {
        setPixelColor(i, color);
    }
    internalShow();
}



