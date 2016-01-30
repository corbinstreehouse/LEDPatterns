
// corbin dunn, dec 6, 2015
#include "CDLazyBitmap.h"

// http://www.dragonwins.com/domains/getteched/bmp/bmpfileformat.htm

#define DEBUG 1

#if DEBUG
#warning "DEBUG CODE IS ON!!!! CDLazyBitmap.h"
    #define DEBUG_PRINTLN(a) Serial.println(a)
    #define DEBUG_PRINTF(a, ...) Serial.printf(a, ##__VA_ARGS__)
#else
    #define DEBUG_PRINTLN(a)
    #define DEBUG_PRINTF(a, ...)
#endif

#if DEBUG
#define CLOSE_AND_RETURN(error) {/* DumpFile(&file); */m_file.close(); Serial.println(error); return; }
#else
#define CLOSE_AND_RETURN(error) { file.close(); return; }
#endif

#if defined(__MK20DX128__) || defined(__MK20DX256__)
#ifndef __LITTLE_ENDIAN__
    #define __LITTLE_ENDIAN__
#endif
#endif

#ifdef __LITTLE_ENDIAN__
#define BITMAP_SIGNATURE 0x4d42
#else
#define BITMAP_SIGNATURE 0x424d
#endif

typedef struct  __attribute__((__packed__)) {
    uint16_t bfType;   /* the magic number used to identify the BMP file:
                          0x42 0x4D (Hex code points for B and M).
                          The following entries are possible:
                          BM - Windows 3.1x, 95, NT, ... etc
                          BA - OS/2 Bitmap Array
                          CI - OS/2 Color Icon
                          CP - OS/2 Color Pointer
                          IC - OS/2 Icon
                          PT - OS/2 Pointer. */
    uint32_t bfSize;    /* the size of the BMP file in bytes */
    uint16_t bfReserved1;  /* reserved. */
    uint16_t bfReserved2;  /* reserved. */
    uint32_t bfOffBits;    /* the offset, i.e. starting address,
                            of the byte where the bitmap data can be found. */
} CDBitmapFileHeader;

// 4 byte color palette..  byte palette would be a size of 12, which i'm not going to support. read:
// http://www.fileformat.info/format/bmp/egff.htm

struct  __attribute__((__packed__)) CDBitmapColorPaletteEntry {
    uint8_t blue, green, red, Alpha; // a not used..
};


#if DEBUGXX
void DumpFile(File *file) {
    file->seek(0);
    int i = 0;
    while (file->available()) {
        uint8_t byte;
        if (file->readBytes(&byte, 1) != 1) {
            break;
        }
        Serial.printf("%2x ", byte);
        i++;
        if (i == 30) {
            i = 0;
            Serial.println("");
        }
    }
}
#endif

CDLazyBitmap::CDLazyBitmap(const char *filename) : m_colorTable(NULL) {
    DEBUG_PRINTF("Bitmap loading: %s\r\n", filename);
    m_isValid = false;

    m_file = FatFile(filename, O_READ);
    if (!m_file.isFile()) {
        DEBUG_PRINTF(" not a bitmap file?: %s\r\n", filename);
        return;
    }
    
    CDBitmapFileHeader fileHeader;
    if (m_file.read((char*)&fileHeader, sizeof(CDBitmapFileHeader)) != sizeof(CDBitmapFileHeader)) {
        CLOSE_AND_RETURN("not a window bitmap image");
    }
    
    if (fileHeader.bfType != BITMAP_SIGNATURE) {
        CLOSE_AND_RETURN("not a window bitmap image");
    }
    
    if (m_file.read((char*)&m_bInfo, sizeof(CDBitmapInfoHeader)) != sizeof(CDBitmapInfoHeader)) {
        CLOSE_AND_RETURN("not an image??");
    }
    
    // 40 byte size is just CDBitmapInfoHeader
    if (m_bInfo.biSize == (sizeof(CDBitmapInfoHeader) + sizeof(CDBitmapInfoHeaderV4))) {
        // Read in v4 stuff
        if (m_file.read((char*)&m_bInfoV4, sizeof(CDBitmapInfoHeaderV4)) != sizeof(CDBitmapInfoHeaderV4)) {
            CLOSE_AND_RETURN("biSize should be at least v4 size!");
        }
    } else if (m_bInfo.biSize != sizeof(CDBitmapInfoHeader)) {
        CLOSE_AND_RETURN("biSize should be at least 40!");
    }
    
    // TODO: compression level 1..possible, but slow!!
    if (m_bInfo.biCompression != 0 /*&& (m_bInfo.biCompression != 1 && m_bInfo.biCompression != 3*/) {
        CLOSE_AND_RETURN("Not supported type of compression");
    }
    
    if (m_bInfo.biBitCount != 32 && m_bInfo.biBitCount != 24 && m_bInfo.biBitCount != 8 && m_bInfo.biBitCount != 4 && m_bInfo.biBitCount != 1) {
        CLOSE_AND_RETURN("Not supported color depth");
    }
    
    if (m_bInfo.biBitCount <= 8) {
//        DEBUG_PRINTF("reading palette: %s\r\n", filename);
        uint32_t paletteSize = m_bInfo.biClrUsed;
        if (paletteSize == 0) {
            paletteSize = 1 << m_bInfo.biBitCount; // 2^depth
        }
        paletteSize *= sizeof(CDBitmapColorPaletteEntry);
        
        // Read the palette.
        m_colorTable = (CDBitmapColorPaletteEntryRef)malloc(paletteSize);
        if (m_file.read((char*)m_colorTable, paletteSize) != paletteSize) {
            CLOSE_AND_RETURN("No palette?");
        }
    } else {
        m_colorTable = NULL;
    }
    
    m_dataOffset = fileHeader.bfOffBits;
    
    m_isValid = true;
}

void CDLazyBitmap::fillRGBBufferFromYOffset(CRGB *buffer, int y) {
    if (y >= getHeight() || y < 0) {
        DEBUG_PRINTLN("bad y offset requested");
        return;
    }
    
    if (m_bInfo.biCompression == 0) {
        uint32_t width = getWidth();
        // 4 byte aligned rows
        unsigned int lineWidth = ((width * m_bInfo.biBitCount / 8) + 3) & ~3;
        uint8_t *lineBuffer = (uint8_t *)malloc(sizeof(uint8_t) * lineWidth);

        int x = 0;
        // Seek to that line that was requested
        uint32_t lineOffset = m_dataOffset + y * lineWidth;
        m_file.seekSet(lineOffset);

        m_file.read((char*)lineBuffer, lineWidth);
        
        uint8_t *linePtr = lineBuffer;
        for (unsigned int j = 0; j < width; j++) {
            if (m_bInfo.biBitCount == 1) {
                uint32_t Color = *((uint8_t*) linePtr);
                for (int k = 0; k < 8; k++) {
                    buffer[x].red = m_colorTable[Color & 0x80 ? 1 : 0].red;
                    buffer[x].green = m_colorTable[Color & 0x80 ? 1 : 0].green;
                    buffer[x].blue = m_colorTable[Color & 0x80 ? 1 : 0].blue;
//                            m_lineData[Index].Alpha = m_colorTable[Color & 0x80 ? 1 : 0].Alpha;
                    x++;
                    Color <<= 1;
                }
                linePtr++;
                j += 7;
            } else if (m_bInfo.biBitCount == 4) {
                uint32_t Color = *((uint8_t*) linePtr);
                buffer[x].red = m_colorTable[(Color >> 4) & 0x0f].red;
                buffer[x].green = m_colorTable[(Color >> 4) & 0x0f].green;
                buffer[x].blue = m_colorTable[(Color >> 4) & 0x0f].blue;
//                        m_lineData[Index].Alpha = m_colorTable[(Color >> 4) & 0x0f].Alpha;
                x++;
                buffer[x].red = m_colorTable[Color & 0x0f].red;
                buffer[x].green = m_colorTable[Color & 0x0f].green;
                buffer[x].blue = m_colorTable[Color & 0x0f].blue;
//                        m_lineData[Index].Alpha = m_colorTable[Color & 0x0f].Alpha;
                x++;
                linePtr++;
                j++;
            } else if (m_bInfo.biBitCount == 8) {
                uint32_t Color = *((uint8_t*) linePtr);
                ASSERT(m_bInfo.biClrUsed == 0 || Color < m_bInfo.biClrUsed);
                buffer[x].red = m_colorTable[Color].red;
                buffer[x].green = m_colorTable[Color].green;
                buffer[x].blue = m_colorTable[Color].blue;
//                        m_lineData[Index].Alpha = m_colorTable[Color].Alpha;
                x++;
                linePtr++;
            } else if (m_bInfo.biBitCount == 16) {
                uint32_t Color = *((uint16_t*) linePtr);
                buffer[x].red = ((Color >> 10) & 0x1f) << 3;
                buffer[x].green = ((Color >> 5) & 0x1f) << 3;
                buffer[x].blue = (Color & 0x1f) << 3;
//                        m_lineData[Index].Alpha = 255;
                x++;
                linePtr += 2;
            } else if (m_bInfo.biBitCount == 24) {
                uint32_t Color = *((uint32_t*) linePtr);
                buffer[x].blue = Color & 0xff;
                buffer[x].green = (Color >> 8) & 0xff;
                buffer[x].red = (Color >> 16) & 0xff;
//                        m_lineData[Index].Alpha = 255;
                x++;
                linePtr += 3;
            } else if (m_bInfo.biBitCount == 32) {
                uint32_t Color = *((uint32_t*) linePtr);
                buffer[x].blue = Color & 0xff;
                buffer[x].green = (Color >> 8) & 0xff;
                buffer[x].red = (Color >> 16) & 0xff;
//                        m_lineData[Index].Alpha = Color >> 24;
                x++;
                linePtr += 4;
            }
        }
        free(lineBuffer);

    } else if (m_bInfo.biCompression == 1) { // RLE 8
#if 0
        m_file.seekSet(m_dataOffset);
        
        uint8_t Count = 0;
        uint8_t ColorIndex = 0;
        int x = 0, y = 0;
        
        while (file.eof() == false) {
            m_file.read((char*) &Count, sizeof(uint8_t));
            m_file.read((char*) &ColorIndex, sizeof(uint8_t));
            
            if (Count > 0) {
                Index = x + y * GetWidth();
                for (int k = 0; k < Count; k++) {
                    m_lineData[Index + k].red = m_colorTable[ColorIndex].red;
                    m_lineData[Index + k].green = m_colorTable[ColorIndex].green;
                    m_lineData[Index + k].blue = m_colorTable[ColorIndex].blue;
//                        m_lineData[Index + k].Alpha = m_colorTable[ColorIndex].Alpha;
                }
                x += Count;
            } else if (Count == 0) {
                int Flag = ColorIndex;
                if (Flag == 0) {
                    x = 0;
                    y++;
                } else if (Flag == 1) {
                    break;
                } else if (Flag == 2) {
                    char rx = 0;
                    char ry = 0;
                    file.read((chaesr*) &rx, sizeof(char));
                    file.read((char*) &ry, sizeof(char));
                    x += rx;
                    y += ry;
                } else {
                    Count = Flag;
                    Index = x + y * GetWidth();
                    for (int k = 0; k < Count; k++) {
                        file.read((char*) &ColorIndex, sizeof(uint8_t));
                        m_lineData[Index + k].red = m_colorTable[ColorIndex].red;
                        m_lineData[Index + k].green = m_colorTable[ColorIndex].green;
                        m_lineData[Index + k].blue = m_colorTable[ColorIndex].blue;
                        m_lineData[Index + k].Alpha = m_colorTable[ColorIndex].Alpha;
                    }
                    x += Count;
                    // Attention: Current Microsoft STL implementation seems to be buggy, tellg() always returns 0.
                    if (file.tellg() & 1) {
                        file.seekg(1, std::ios::cur);
                    }
                }
            }
        }
#endif
    } else if (m_bInfo.biCompression == 2) { // RLE 4
        /* RLE 4 is not supported */ // checked earlier
    } else if (m_bInfo.biCompression == 3) { // BITFIELDS
        
        /* We assumes that mask of each color component can be in any order */
#if 0
        uint32_t lineOffset = m_dataOffset + y * lineWidth;
        m_file.seekSet(lineOffset);
^^ is this right?
        
        uint32_t BitCountRed = CColor::BitCountByMask(m_BitmapHeader.RedMask);
        uint32_t BitCountGreen = CColor::BitCountByMask(m_BitmapHeader.GreenMask);
        uint32_t BitCountBlue = CColor::BitCountByMask(m_BitmapHeader.BlueMask);
        uint32_t BitCountAlpha = CColor::BitCountByMask(m_BitmapHeader.AlphaMask);
        
        for (unsigned int i = 0; i < GetHeight(); i++) {
            file.read((char*) Line, LineWidth);
            
            uint8_t *linePtr = Line;
            
            for (unsigned int j = 0; j < GetWidth(); j++) {
                
                uint32_t Color = 0;
                
                if (m_bInfo.biBitCount == 16) {
                    Color = *((uint16_t*) LinePtr);
                    LinePtr += 2;
                } else if (m_bInfo.biBitCount == 32) {
                    Color = *((uint32_t*) LinePtr);
                    LinePtr += 4;
                } else {
                    // Other formats are not valid
                }
                m_lineData[Index].red = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.RedMask), BitCountRed, 8);
                m_lineData[Index].green = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.GreenMask), BitCountGreen, 8);
                m_lineData[Index].blue = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.BlueMask), BitCountBlue, 8);
//                    m_lineData[Index].Alpha = CColor::Convert(CColor::ComponentByMask(Color, m_BitmapHeader.AlphaMask), BitCountAlpha, 8);
                
                Index++;
            }
        }
#endif
    }

    
}

CDLazyBitmap::~CDLazyBitmap() {
    if (m_file.isOpen()) {
        m_file.close();
    }

    if (m_colorTable) {
        free(m_colorTable);
    }
}



#define MAX_SIZE_SINGLE_BUFFER (10*1024) // 10kB

// Allocate the memory once for the patterns since we can share it; if we are in the sim, we may have multiple instances
static CRGB *g_sharedBuffer = NULL; // always NULL for the pattern editor

#ifdef PATTERN_EDITOR

static uint32_t FreeRam() {
    return MAX_SIZE_SINGLE_BUFFER + 1; // Doesn't matter
}

#else

static uint32_t FreeRam() { // for Teensy 3.0
    uint32_t stackTop;
    uint32_t heapTop;
    
    // current position of the stack.
    stackTop = (uint32_t) &stackTop;
    
    // current position of heap.
    void* hTop = malloc(1);
    heapTop = (uint32_t) hTop;
    free(hTop);
    
    // The difference is the free, available ram.
    return stackTop - heapTop;
}

#endif

// may return NULL if it couldn't be allocated
static inline CRGB *getSharedBuffer() {
    CRGB *result = g_sharedBuffer;
    if (result == NULL) {
        uint32_t freeRam = FreeRam();
        // Make sure we have extra room for the program to run (1kB enough?)
        if (freeRam > (MAX_SIZE_SINGLE_BUFFER + 1024)) {
            // allocate it
            result = (CRGB *)malloc(MAX_SIZE_SINGLE_BUFFER);
            // NOTE: I could probably get another 2KB! by using the other buffers passed in (if non NULL)
            // Save it if we aren't the pattern editor
#ifndef PATTERN_EDITOR
            // Save it only for the pattern editor
            g_sharedBuffer = result;
#endif
        }
    }
    return result;

}

CDPatternBitmap::CDPatternBitmap(const char *filename, CRGB *buffer1, CRGB *buffer2, size_t bufferSize) : CDLazyBitmap(filename), m_xOffset(0), m_yOffset(-1), m_buffer(NULL), m_bufferOwned(false), m_buffer1Owned(false)  {
    
    // Allocate an internal buffer if the passed in ones aren't large enough
    
    // In the end, this complexity may not be needed, as loading a line at a time from the SD card seems fast enough..
    if (getIsValid()) {
        size_t totalMemoryNeeded = sizeof(CRGB)*getWidth()*getHeight();
        
        // Try to allocate one big buffer to hold it all in RAM; up to 10kB
        if (totalMemoryNeeded <= MAX_SIZE_SINGLE_BUFFER) {
            m_buffer = getSharedBuffer();
#ifdef PATTERN_EDITOR
            m_bufferOwned = true; // Else we own it and free it..
#endif
        }
        
        // If we could allocate it, then fill it up!
        if (m_buffer != NULL) {
            CRGB *offset = m_buffer;
            for (int y = 0; y < getHeight(); y++ ) {
                fillRGBBufferFromYOffset(offset, y);
                offset += getWidth();
            }
        } else {
            // Do a line at a time..
            size_t requiredBufferSize = sizeof(CRGB)*getWidth();
            if (bufferSize >= requiredBufferSize) {
                m_buffer1 = buffer1;
                m_buffer2 = buffer2;
                m_buffer1Owned = false;
            } else {
                // Try the shared buffer for a single line
                m_buffer1 = getSharedBuffer();
                if (m_buffer1) {
#ifdef PATTERN_EDITOR
                    m_buffer1Owned = true; // It was allocated in this case
#endif
                } else {
                    // Allocate and own it; if we have enough space
                    if (requiredBufferSize < (FreeRam() + 1024)) {
                        m_buffer1 = (CRGB *)malloc(requiredBufferSize);
                        m_buffer1Owned = true; // It was allocated in this case
                    } else {
                        // Error condition...not enough RAM!
                        
                    }
                }
                
                m_buffer2 = NULL; // only one? sure..
            }
        }
        incYOffsetBuffers();
    }
}


CDPatternBitmap::~CDPatternBitmap() {
    if (m_buffer1Owned) {
        if (m_buffer1) {
            free(m_buffer1);
        }
        if (m_buffer2) {
            free(m_buffer2);
        }
    }
    if (m_bufferOwned && m_buffer) {
        free(m_buffer);
    }
}

void CDPatternBitmap::incYOffsetBuffers() {
    int height = getHeight();
    if (height == 1 && m_yOffset == 0) {
        return; // This means we already loaded the data; m_yOffset starts at -1, so we will run through this at least once to fill it up. We could close the file right now..but the abstraction probably isn't necessary
    }
    int oldOffset = m_yOffset;
    m_yOffset++;
    // wrap the values
    if (m_yOffset >= height) {
        m_yOffset = 0;
    }
    int secondBufferOffset = m_yOffset + 1;
    if (secondBufferOffset >= height) {
        secondBufferOffset = 0;
    }
    
    size_t width = getWidth();
    // If we loaded everything (non NULL buffer), then we can just compute the offset
    if (m_buffer != NULL) {
        m_buffer1 = m_buffer + (m_yOffset * width);
        m_buffer2 = m_buffer + (secondBufferOffset * width);
    } else {
        // If this was the first time... we fill up both buffers
        if (oldOffset == -1) {
            fillRGBBufferFromYOffset(m_buffer1, m_yOffset);
            if (m_buffer2) {
                if (height == 1) {
                    memcpy(m_buffer2, m_buffer1, sizeof(CRGB)*getWidth());
                } else {
                    fillRGBBufferFromYOffset(m_buffer2, secondBufferOffset);
                }
            }
        } else {
            // If only have one buffer, just fill up the first one
            if (m_buffer2 == NULL) {
                fillRGBBufferFromYOffset(m_buffer1, m_yOffset);
            } else {
                // Move the second to be the first
                CRGB *oldFirstBuffer = m_buffer1;
                m_buffer1 = m_buffer2;
                m_buffer2 = oldFirstBuffer;
                // well, we only have to fill it back up if there are more than 2 rows
                if (height > 2) {
                    fillRGBBufferFromYOffset(m_buffer2, secondBufferOffset);
                }
            }
        }
    }
    m_buffer1Valid = true;
}

CRGB *CDPatternBitmap::getFirstBuffer() {
    if (m_yOffset == -1) {
        incYOffsetBuffers();
    }
    if (!m_buffer1Valid) {
        fillRGBBufferFromYOffset(m_buffer1, m_yOffset);
        m_buffer1Valid = true;
    }
    return m_buffer1;
}

CRGB *CDPatternBitmap::getSecondBuffer() {
    if (m_buffer2) {
        return m_buffer2;
    }
    // we have to re-use the first buffer..
//    int secondBufferOffset = m_yOffset;
//    if (secondBufferOffset >= getHeight()) {
//        secondBufferOffset = 0;
//    }
    fillRGBBufferFromYOffset(m_buffer1, m_yOffset);
    m_buffer1Valid = false;
    return m_buffer1;
}

