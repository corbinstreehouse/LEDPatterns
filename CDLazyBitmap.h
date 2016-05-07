// I wrote everythihg except the struct defines, which came from MS and other places.
// corbin

#ifndef _CD_LAZY_BITMAP_H_
#define _CD_LAZY_BITMAP_H_

#include <stdlib.h>
#include "FastLED.h"
#include "FatFile.h"

typedef struct CDBitmapColorPaletteEntry *CDBitmapColorPaletteEntryRef;


#if DEBUG
    #if PATTERN_EDITOR
        #define ASSERT(a) NSCAssert(a, @"Fail");
    #else
        #define ASSERT(a) if (!(a)) { \
            Serial.print("ASSERT ");  \
            Serial.print(__FILE__); Serial.print(" : "); \
            Serial.println(__LINE__); }
    #endif
#else
    #if PATTERN_EDITOR
        #define ASSERT(a) NSCAssert(a, @"Fail");
    #else
        #define ASSERT(a) ((void)0)
    #endif
#endif


typedef struct  __attribute__((__packed__)) {
    uint32_t biSize;     /* the size of this header (40 bytes) */
    int32_t biWidth;         /* the bitmap width in pixels */
    int32_t biHeight;        /* the bitmap height in pixels */
    uint16_t biPlanes;       /* the number of color planes being used.
                              Must be set to 1. */
    uint16_t biBitCount;         /* the number of bits per pixel,
                                  which is the color depth of the image.
                                  Typical values are 1, 4, 8, 16, 24 and 32. */
    uint32_t biCompression; /* the compression method being used.
                             See also bmp_compression_method_t. */
    uint32_t biSizeImage;    /* the image size. This is the size of the raw bitmap
                              data (see below), and should not be confused
                              with the file size. */
    uint32_t biXPelsPerMeter;          /* the horizontal resolution of the image.
                                        (pixel per meter) */
    uint32_t biYPelsPerMeter;          /* the vertical resolution of the image.
                                        (pixel per meter) */
    uint32_t biClrUsed;       /* the number of colors in the color palette,
                               or 0 to default to 2<sup><i>n</i></sup>. */
    uint32_t biClrImportant;    /* the number of important colors used,
                                 or 0 when every color is important;
                                 generally ignored. */
} CDBitmapInfoHeader;


typedef struct  __attribute__((__packed__)) {
    uint32_t RedMask;
    uint32_t GreenMask;
    uint32_t BlueMask;
    uint32_t AlphaMask;
    uint32_t CsType;
    uint32_t Endpoints[9]; // see http://msdn2.microsoft.com/en-us/library/ms536569.aspx
    uint32_t GammaRed;
    uint32_t GammaGreen;
    uint32_t GammaBlue;
} CDBitmapInfoHeaderV4;


class CDPatternBitmap {
private:
    CDBitmapInfoHeader m_bInfo;
    CDBitmapInfoHeaderV4 m_bInfoV4;
    CDBitmapColorPaletteEntryRef m_colorTable;
    uint32_t m_width, m_height;
private:
    bool m_isValid;
    uint32_t m_dataOffset;
    FatFile m_file;
    
    uint8_t *getLineBufferAtOffset(size_t size, uint32_t dataOffset, bool *owned);
    // fills the buffer from the image data, loading it as needed.
    // y can be from 0 to height-1.
    // buffer MUST be getWidth*sizeof(CRGB).
    void uncompressed_fillRGBBufferFromYOffset(CRGB *buffer, int y);
    
    void fillEntireBufferFromFile(CRGB *buffer);

    
    void fillEntireBufferFromFile_comp0(CRGB *buffer);
    // compression level 1, RLE 8
    void fillEntireBufferFromFile_comp1(CRGB *buffer);
    
    
public:
    
    inline bool getIsValid() { return m_isValid; }
    
    inline uint32_t getWidth() {
        return m_width; // m_bInfo.biWidth < 0 ? -m_bInfo.biWidth : m_bInfo.biWidth;
    }
    
    inline uint32_t getHeight() {
        return m_height; // m_bInfo.biHeight < 0 ? -m_bInfo.biHeight : m_bInfo.biHeight;
    }
    
    

private:
    CRGB *m_buffer; // When non NULL, we have fully loaded the bitmap. We need to free this if m_bufferOwned is true (only for the pattern editor)
    CRGB *m_buffer1;
    CRGB *m_buffer2;
    int m_yOffset; // y offset for the first buffer, second is the next line (or wrapped, the first line)
    int m_xOffset; // used by the pattern
    uint32_t m_buffer1Owned:1;
    uint32_t m_buffer2Owned:1;
    uint32_t m_bufferOwned:1;
    uint32_t m_bufferIsEntireFile:1;
    uint32_t m_bufferIsFullCRGBData:1;
public:
    // buffers MAY be REFERENCED, if large enough -- they are not owned by this class, and the memory must be kept alive outside of it
    CDPatternBitmap(const char *filename, CRGB *buffer1, CRGB *buffer2, size_t bufferSize);
    ~CDPatternBitmap();
    
    // Loads the next y offset, wrapping as needed
    inline void incYOffsetBuffers() {
        int oldOffset = m_yOffset;
        m_yOffset++;
        // wrap the values
        int height = getHeight();
        if (m_yOffset >= height) {
            m_yOffset = 0;
            if (height == 1 && oldOffset == 1) {
                return; // Already filled..
            }
        }
        updateBuffersWithYOffset(m_yOffset, oldOffset);
    }
    
    inline void incXOffset() {
        setXOffset(m_xOffset + 1);
    }
    
    inline void setXOffset(uint xOffset) {
        m_xOffset = xOffset;
        if (m_xOffset >= getWidth()) {
            m_xOffset = 0;
        }
    }
    
    void setYOffset(uint yOffset) {
        m_yOffset = yOffset;
        if (m_yOffset >= getHeight()) {
            m_yOffset = 0;
        }
    }
    
    // careful! no validation on offset is done
    // oldOffset can be -1 or the prior one that was loaded (offset is assumed to be oldOffset+1 in that case, minus wrapping)
    void updateBuffersWithYOffset(int offset, int oldOffset);
    
    inline void moveToStart() {
        m_yOffset = -1;
        incYOffsetBuffers();
    }
    inline int getXOffset() { return m_xOffset; }
    inline int getYOffset() { return m_yOffset; }
    
    inline CRGB *getFirstBuffer() {
#if DEBUG
        ASSERT(m_buffer1 != NULL)
#endif
        return m_buffer1;
    }
    inline CRGB *getSecondBuffer() {
#if DEBUG
        ASSERT(m_buffer2 != NULL)
#endif
        return m_buffer2;
    }
    
};




#endif