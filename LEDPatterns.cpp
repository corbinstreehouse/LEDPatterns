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
#if SD_CARD_SUPPORT
    #include "SD.h"
#endif

#if DEBUG
    #define DEBUG_PRINTLN(a) Serial.println(a)
    #define DEBUG_PRINTF(a, ...) Serial.printf(a, ##__VA_ARGS__)
#else
    #define DEBUG_PRINTLN(a)
    #define DEBUG_PRINTF(a, ...)
#endif

#ifndef byte
#define byte uint8_t
#endif


static inline bool PatternIsContinuous(LEDPatternType p) {
    switch (p) {
        case LEDPatternTypeRotatingRainbow:
        case LEDPatternTypeRotatingMiniRainbows:
        case LEDPatternTypeTheaterChase:
        case LEDPatternTypeGradient:
        case LEDPatternTypePluseGradientEffect:
        case LEDPatternTypeSolidRainbow:
        case LEDPatternTypeRainbowWithSpaces:
        case LEDPatternTypeRandomGradients:
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
        case LEDPatternTypeFadeOut:
        case LEDPatternTypeFadeIn:
        case LEDPatternTypeColorWipe:
            return false;
            
        case LEDPatternTypeDoNothing:
            return true;
#if SD_CARD_SUPPORT
        case LEDPatternTypeImageLinearFade:
#endif
        case LEDPatternTypeWave:
            return false;
#if SD_CARD_SUPPORT
        case LEDPatternTypeImageEntireStrip:
            return true;
#endif
        case LEDPatternTypeBottomGlow:
            return true; // Doesn't do anything
        case LEDPatternTypeRotatingBottomGlow:
        case LEDPatternTypeSolidColor:
            return false; // repeats after a rotation
        case LEDPatternTypeMax:
        case LEDPatternTypeBlink:
            return false;
        case LEDPatternTypeFire:
        case LEDPatternTypeBlueFire:
            return true;
            
    }
}

// This always resets things, so only change it when necessary
void LEDPatterns::setPatternType(LEDPatternType type) {
    m_patternType = type;
    m_startTime = millis();
    m_firstTime = true;
    m_intervalCount = 0;
    m_loopCount = 0;
    m_count = 0;
}

void LEDPatterns::show() {
    uint32_t now = millis();
    // The inital tick always starts with 0
    m_timePassed = m_firstTime ? 0 : now - m_startTime;

    // How many intervals have passed?
    // TODO: corbin, I should only calculate these things for the patterns that need it!
    m_intervalCount = m_timePassed / m_duration;
    
    if (!PatternIsContinuous(m_patternType)) {
        // Since it isn't continuous, modify the timePassed here
        if (m_intervalCount > 0) {
            m_timePassed = m_timePassed - m_intervalCount*m_duration;
        }
    }
    
    
    // for polulu
    unsigned int maxLoops = 0;  // go to next state when m_ledCount >= maxLoops
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
    
    switch (m_patternType) {
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
            fadeOut();
            break;
        }
        case LEDPatternTypeFadeIn: {
            fadeIn();
            break;
        }
        case LEDPatternTypeDoNothing: {
            break;
        }
        case LEDPatternTypeTheaterChase: {
            theaterChase();
            break;
        }
#if SD_CARD_SUPPORT
        case LEDPatternTypeImageLinearFade: {
            linearImageFade();
            break;
        }
        case LEDPatternTypeImageEntireStrip: {
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
#warning corbin! don't call this multiple times...unless we want it to change
            fill_solid(m_leds, m_ledCount, m_patternColor);
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
//            if (isFirstPass || itemHeader->shouldSetBrightnessByRotationalVelocity)
            {
                solidRainbow(0, 1);
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
        case LEDPatternTypeAllOff: {
#if DEBUG
            //showOn();
#endif
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
    internalShow();
    // no longer the first time
    m_firstTime = false;
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
        if (i + 1 < m_ledCount)
        {
            m_leds[i+1] = m_leds[i-1];
        }
        if (i + 2 < m_ledCount)
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
    if (m_timePassed == 0) {
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
        if (m_intervalCount > 0) {
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
    }
}

void LEDPatterns::blinkPattern() {
    if (getPercentagePassed() < 0.5) {
        fill_solid(m_leds, m_ledCount, m_patternColor);
    } else {
        fill_solid(m_leds, m_ledCount, CRGB::Black);
    }
}


static CRGB HeatColorBlue( uint8_t temperature)
{
    CRGB heatcolor;
    
    // Scale 'heat' down from 0-255 to 0-191,
    // which can then be easily divided into three
    // equal 'thirds' of 64 units each.
    uint8_t t192 = scale8_video( temperature, 192);
    
    // calculate a value that ramps up from
    // zero to 255 in each 'third' of the scale.
    uint8_t heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2; // scale up to 0..252
    
    // now figure out which third of the spectrum we're in:
    if( t192 & 0x80) {
        // we're in the hottest third
        heatcolor.r = heatramp; // full red
        heatcolor.g = 255; // full green
        heatcolor.b = 255; // ramp up blue
        
    } else if( t192 & 0x40 ) {
        // we're in the middle third
        heatcolor.r = 0; // full red
        heatcolor.g = heatramp; // ramp up green
        heatcolor.b = 255; // no blue
        
    } else {
        // we're in the coolest third
        heatcolor.r = 0; //
        heatcolor.g = 0; //
        heatcolor.b = heatramp; //
    }
    
    return heatcolor;
}

void LEDPatterns::firePatternWithColor(bool blue) {
    // COOLING: How much does the air cool as it rises?
    // Less cooling = taller flames.  More cooling = shorter flames.
    // Default 50, suggested range 20-100
#define COOLING  80
    
    // SPARKING: What chance (out of 255) is there that a new spark will be lit?
    // Higher chance = more roaring fire.  Lower chance = more flickery fire.
    // Default 120, suggested range 50-200.
#define SPARKING 130

    // Array of temperature readings at each simulation cell
    byte *heat = (byte *)getTempBuffer1();
    byte *heat2 = (byte *)getTempBuffer2();
    
    if (m_firstTime) {
        m_initialPixel = millis(); // Use for timing
        for (int i = 0; i < m_ledCount; i++) {
            heat[i] = 0; // random8(255);
            heat2[i] = 0;
        }
    } else {
        // Update every 1/60 second
        uint32_t now = millis();
        if (now - m_initialPixel >= ((1.0/60.0)*1000.0)) {
            // enough time passed!
            m_initialPixel = now;
        } else {
            // not enough time passed;...
            return;
        }
    }

    int count = m_ledCount / 2;
    int secondHalfCount = m_ledCount - count;
    if (secondHalfCount > count) {
        count = secondHalfCount;
    }
    
    // Step 1.  Cool down every cell a little
    for( int i = 0; i < count; i++) {
        heat[i] = qsub8( heat[i],  random(0, ((COOLING * 10) / count) + 2));
        heat2[i] = qsub8( heat2[i],  random(0, ((COOLING * 10) / count) + 2));
    }
    
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= count - 3; k > 0; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
        heat2[k] = (heat2[k - 1] + heat2[k - 2] + heat2[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random(255) < SPARKING ) {
        int y = random(7);
        heat[y] = qadd8( heat[y], random(160,255) );
        y = random(7);
        heat2[y] = qadd8( heat2[y], random(160,255) );
    }
    
    // Step 4.  Map from heat cells to LED colors
    int k = m_ledCount - 1;
    for( int j = 0; j < count; j++) {
        if (blue) {
            m_leds[j] = HeatColorBlue( heat[j]);
            m_leds[k] = HeatColorBlue(heat2[j]);
        } else {
            m_leds[j] = HeatColor( heat[j]);
            m_leds[k] = HeatColor(heat2[j]);
        }
        k--;
    }
}

void LEDPatterns::firePattern() {
    firePatternWithColor(false);
}

void LEDPatterns::blueFirePattern() {
    firePatternWithColor(true);
}

void LEDPatterns::rotatingBottomGlow() {
    float topPixel = getPercentagePassed()*m_ledCount;
    bottomGlowFromTopPixel(topPixel);
}

void LEDPatterns::fadeIn() {
    // slow fade in with:
    // y = x^2, where x == time
    float t = getPercentagePassed();
    float y = t*t;
    
    for (int i = 0; i < m_ledCount; i++) {
        fadePixel(i, m_patternColor, y);
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

void LEDPatterns::fadeOut() {
    // Opposite of fade in, and we store the inital value on the first pass
    // y = -x^2 + 1
    CRGB *tempBuffer = getTempBuffer1();
    if (m_firstTime) {
        // First pass, store off the initial state..
        memcpy(tempBuffer, m_leds, getBufferSize());
    }
    
    float t = (float)m_timePassed / (float)m_duration;
    float y = -(t*t)+1;
    for (int i = 0; i < m_ledCount; i++) {
        // direct pixel access to avoid issues w/reading the already set brightness
        m_leds[i].green = tempBuffer[i].green*y;
        m_leds[i].red = tempBuffer[i].red*y;
        m_leds[i].blue = tempBuffer[i].blue*y;
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
        if ((swipePosition + i) % 3 == 0) {
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
void LEDPatterns::readDataIntoBufferStartingAtPosition(uint32_t position, uint8_t *buffer) {
    //    DEBUG_PRINTF("buffer load at: %d, of max: %d\r\n", position, imageHeader->dataLength);
    // Mark where we are in the file (relative)
    int bufferSize = getBufferSize();
    
    File f = SD.open(m_dataFilename);
    //    if (!f.available()) {
    //        DEBUG_PRINTLN("  _-----------------------what???");
    //        delay(1000);
    //        return;
    //    }
    
    uint32_t filePositionToReadFrom = m_dataOffset + position;
    //    DEBUG_PRINTLN("  seek");
    f.seek(filePositionToReadFrom);
    
    // We can read up to the end based on our current position
    int amountWeWantToRead = m_dataLength - position;
    if (amountWeWantToRead <= bufferSize) {
        // Easy, read all the rest in
        //        DEBUG_PRINTF("  read %d", amountWeWantToRead);
        f.readBytes((char*)buffer, amountWeWantToRead);
    } else {
        // Read up to the buffer size
        //        DEBUG_PRINTF("  read full %d", bufferSize);
        f.readBytes((char*)buffer, bufferSize);
    }
    f.close();
}

#define USE_TWO_BUFFERS 1

void LEDPatterns::initImageDataForHeader() {
    DEBUG_PRINTLN("------------------------------");
    DEBUG_PRINTF("playing image with length: %d\r\n", m_dataLength);
    //    Serial.printf("init image state; header: %x, data:%x, datavalue:%x\r\n", imageHeader, imageState->dataOffset, dataStart(imageHeader));
    DEBUG_PRINTLN("");
    
    m_dataOffsetReadIntoBuffer1 = 0;
    m_dataOffsetReadIntoBuffer2 = 0;
    readDataIntoBufferStartingAtPosition(m_dataOffsetReadIntoBuffer1, (uint8_t*)getTempBuffer1());
    // Fill in the second buffer, if we need to
#if USE_TWO_BUFFERS
    int bufferSize = getBufferSize();
    if (m_dataLength > bufferSize) {
        m_dataOffsetReadIntoBuffer2 = bufferSize;
        readDataIntoBufferStartingAtPosition(m_dataOffsetReadIntoBuffer2, (uint8_t*)getTempBuffer2());
    }
#endif
}

uint8_t *LEDPatterns::dataForOffset(uint32_t offset) {
    // If the offset is within our buffer size, then just return it directly from the buffer
    int bufferSize = getBufferSize();
    uint8_t *buffer1 = (uint8_t *)getTempBuffer1();
    // In the first buffer?
    if (offset >= m_dataOffsetReadIntoBuffer1 && offset < (m_dataOffsetReadIntoBuffer1 + bufferSize)) {
        int offsetInBuffer = offset - m_dataOffsetReadIntoBuffer1;
        return &buffer1[offsetInBuffer];
    }
#if USE_TWO_BUFFERS
    // second buffer?
    uint8_t *buffer2 = (uint8_t *)getTempBuffer2();
    if (offset >= m_dataOffsetReadIntoBuffer2 && offset < (m_dataOffsetReadIntoBuffer2 + bufferSize)) {
        int offsetInBuffer = offset - m_dataOffsetReadIntoBuffer2;
        return &buffer2[offsetInBuffer];
    }
#endif
    
    // Nope...have to do more work...reset the buffers
    m_dataOffsetReadIntoBuffer1 = offset;
    readDataIntoBufferStartingAtPosition(m_dataOffsetReadIntoBuffer1, buffer1);
    
    // read into buffer two also
#if USE_TWO_BUFFERS
    m_dataOffsetReadIntoBuffer2 = offset + bufferSize;
    if (m_dataOffsetReadIntoBuffer2 >= m_dataLength) {
        m_dataOffsetReadIntoBuffer2 = 0; // Back to the start
    }
    readDataIntoBufferStartingAtPosition(m_dataOffsetReadIntoBuffer2, buffer2);
#endif
    
    // now the buffer 1 is full, starting at pos 0, and the second is ready for the next set...
    return &buffer1[0];
}

static inline uint32_t keepOffsetInDataBounds(uint32_t offset, uint32_t length) {
    int amountOver = offset - length;
    // wrap
    if (amountOver > 0) {
        offset = amountOver;
        if (offset > length) {
            offset = 0; // way too far..i could mod or something..
        }
    }
    return offset;
}


void LEDPatterns::patternImageEntireStrip() {
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
        setPixelColor(x, CRGB(r, g, b));
        // Again..assumes 3 bytes per pixel
        currentOffset += 3;
        nextOffsetForBlending += 3;
    }
}


void LEDPatterns::linearImageFade() {
    if (m_firstTime) {
        initImageDataForHeader();
    }
    
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
}

#endif  // SD_CARD_SUPPORT

void LEDPatterns::flashThreeTimesWithDelay(CRGB color, uint32_t delayAmount) {
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