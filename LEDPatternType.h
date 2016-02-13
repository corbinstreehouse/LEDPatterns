//
//  LEDPatternType.h
//  LEDDigitalCyrWheel
//
//  Created by corbin dunn on 5/31/14.
//
//

#ifndef __LED_PATTERN_TYPE_H
#define __LED_PATTERN_TYPE_H

#include <stdint.h>

// Make this more portable with ARM compilers
#ifndef __has_feature
#define __has_feature(a) 0
#endif

#ifndef __has_extension
#define __has_extension(a) 0
#endif

#if __has_extension(cxx_strong_enums) || __has_feature(objc_fixed_enum)
    #define CD_ENUM(_type, _name)     enum _name : _type _name; enum _name : _type
    #define CD_OPTIONS(_type, _name) _type _name; enum : _type
#else
    #define CD_ENUM(_type, _name)     enum _name : _type
    #define CD_OPTIONS(_type, _name) _type _name; enum : _type
#endif

// NOTE: update g_patternTypeNames when this changes!!
typedef CD_ENUM(int32_t, LEDPatternType) {
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
    
    // NOTE: I'm making this a property of the image instead...
    LEDPatternTypeImageReferencedBitmap, // smooth traverse over pixels
    LEDPatternTypeImageEntireStrip_UNUSED, // one strip piece at a time defined
    
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
    
    LEDPatternTypeFire,
    LEDPatternTypeBlueFire,
    
    LEDPatternFlagEffect,
    
    LEDPatternTypeCrossfade,
    LEDPatternTypeSinWave, 
    LEDPatternTypeFunkyClouds,
    
    LEDPatternTypeLife,
    LEDPatternTypeLifeDynamic,
    
    LEDPatternTypeBouncingBall,
    LEDPatternTypeRainbowFire,
    LEDPatternTypeLavaFire,
    
    LEDPatternTypeBitmap,

    LEDPatternTypeFadeInFadeOut,
    
    LEDPatternTypeCount,
};
    
    
typedef struct __attribute__((__packed__)) LEDBitmapPatternOptions {
    uint32_t shouldInterpolateStretchedPixels:1;
    uint32_t shouldStrechBitmap:1;
    uint32_t shouldInterpolateToNextRow:1;
    uint32_t pov:1;
    uint32_t reserved:28;
    
#ifdef __cplusplus
    inline LEDBitmapPatternOptions(bool shouldInterpolateStretchedPixels, bool shouldStrechBitmap, bool shouldInterpolateToNextRow, bool pov=false) {
        this->shouldInterpolateStretchedPixels = shouldInterpolateStretchedPixels;
        this->shouldStrechBitmap = shouldStrechBitmap;
        this->shouldInterpolateToNextRow = shouldInterpolateToNextRow;
        this->pov = pov;
    }
#endif
    
} LEDBitmapPatternOptions;


// options that only apply to particular patterns, so I combine them all together. i could put the patternColor here as it only applies to certain patterns.
// warning: keep at 32-bits for now! Or I have to expand the header
    // NOTE: I'm going to drop using these, as they come across poorly in swift. I'll just use the main bitset for each option...and maybe put some specific extra data in here..
typedef struct __attribute__((__packed__)) LEDPatternOptions {
    union {
        LEDBitmapPatternOptions bitmapOptions;
        uint32_t raw;
    };
    
#ifdef __cplusplus

    inline LEDPatternOptions() __attribute__((always_inline))
    {
        //uninitialized
    }
    
    inline LEDPatternOptions(LEDBitmapPatternOptions bOptions) : bitmapOptions(bOptions) { }
    inline LEDPatternOptions(uint32_t raw) : raw(raw) { }
#endif
    
} LEDPatternOptions;


#endif