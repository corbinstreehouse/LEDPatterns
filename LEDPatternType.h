//
//  LEDPatternType.h
//  LEDDigitalCyrWheel
//
//  Created by corbin dunn on 5/31/14.
//
//

#ifndef __LED_PATTERN_TYPE_H
#define __LED_PATTERN_TYPE_H

// NOTE: update g_patternTypeNames when this changes!!

#if (__cplusplus && __cplusplus >= 201103L && (__has_extension(cxx_strong_enums) || __has_feature(objc_fixed_enum))) || (!__cplusplus && __has_feature(objc_fixed_enum))
    #define CD_ENUM(_type, _name)     enum _name : _type _name; enum _name : _type
#else
    #define CD_ENUM(_type, _name)     enum _name : _type
#endif

typedef CD_ENUM(int16_t, LEDPatternType) {
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
    
    LEDPatternTypeMax,
    LEDPatternTypeAllOff = LEDPatternTypeMax,
};
    
    
typedef struct __attribute__((__packed__)) LEDBitmapPatternOptions {
    uint32_t shouldInterpolate:1;
    uint32_t stretchToFill:1; // corbin -- would be cool to implement!
    uint32_t reserved:30;
    
#ifdef __cplusplus
    inline LEDBitmapPatternOptions(bool shouldInterpolate, bool stretchToFill) {
        this->shouldInterpolate = shouldInterpolate;
        this->stretchToFill = stretchToFill;
    }
#endif
    
} LEDBitmapPatternOptions;


// options that only apply to particular patterns, so I combine them all together. i could put the patternColor here as it only applies to certain patterns.
// warning: keep at 32-bits for now! Or I have to expand the header
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