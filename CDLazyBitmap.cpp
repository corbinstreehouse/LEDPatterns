
// corbin dunn, dec 6, 2015
#include "CDLazyBitmap.h"

// http://www.dragonwins.com/domains/getteched/bmp/bmpfileformat.htm

#undef DEBUG
#define DEBUG 0

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
    #define CLOSE_AND_RETURN(error) { m_file.close(); return; }
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

void CDPatternBitmap::fillEntireBufferFromFile(CRGB *buffer) {
    switch (m_bInfo.biCompression) {
        case 0: {
            fillEntireBufferFromFile_comp0(buffer);
            break;
        }
        case 1: {
            fillEntireBufferFromFile_comp1(buffer);
            break;
        }
        default: {
            m_isValid = false;
            ASSERT(NO); // unhandled compression!
        }
    }
}

void CDPatternBitmap::fillEntireBufferFromFile_comp0(CRGB *buffer) {
    m_file.seekSet(m_dataOffset);

    // 4 byte aligned rows
    unsigned int lineWidth = ((m_width * m_bInfo.biBitCount / 8) + 3) & ~3;
    size_t size = sizeof(uint8_t) * lineWidth;
    int x = 0;
    
    // read in one line at a time from the file
    // TODO: Maybe a stack buffer to try?
    uint8_t *lineBuffer = (uint8_t *)malloc(size);

    // unroll the loops a bit; this replicates code for speed
    if (m_bInfo.biBitCount == 1) {
        for (int y = 0; y < m_height; y++) {
            m_file.read((char*)lineBuffer, size);
            uint8_t *linePtr = lineBuffer;
            for (int j = 0; j < m_width; j++) {
                ///////////
                uint32_t color = *((uint8_t*) linePtr);
                for (int k = 0; k < 8; k++) {
                    buffer[x].red = m_colorTable[color & 0x80 ? 1 : 0].red;
                    buffer[x].green = m_colorTable[color & 0x80 ? 1 : 0].green;
                    buffer[x].blue = m_colorTable[color & 0x80 ? 1 : 0].blue;
                    //                            m_lineData[Index].Alpha = m_colorTable[Color & 0x80 ? 1 : 0].Alpha;
                    x++;
                    color <<= 1;
                }
                linePtr++;
                j += 7;
                ///////////
            }
        }
    } else if (m_bInfo.biBitCount == 4) {
        for (int y = 0; y < m_height; y++) {
            m_file.read((char*)lineBuffer, size);
            uint8_t *linePtr = lineBuffer;
            for (int j = 0; j < m_width; j++) {
                ///////////
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
                ///////////
            }
        }
    } else if (m_bInfo.biBitCount == 8) {
        for (int y = 0; y < m_height; y++) {
            m_file.read((char*)lineBuffer, size);
            uint8_t *linePtr = lineBuffer;
            for (int j = 0; j < m_width; j++) {
                ///////////
                uint32_t Color = *((uint8_t*) linePtr);
                ASSERT(m_bInfo.biClrUsed == 0 || Color < m_bInfo.biClrUsed);
                buffer[x].red = m_colorTable[Color].red;
                buffer[x].green = m_colorTable[Color].green;
                buffer[x].blue = m_colorTable[Color].blue;
                //                        m_lineData[Index].Alpha = m_colorTable[Color].Alpha;
                x++;
                linePtr++;
                ///////////
            }
        }
    } else if (m_bInfo.biBitCount == 16) {
        for (int y = 0; y < m_height; y++) {
            m_file.read((char*)lineBuffer, size);
            uint8_t *linePtr = lineBuffer;
            for (int j = 0; j < m_width; j++) {
                ///////////
                uint32_t Color = *((uint16_t*) linePtr);
                buffer[x].red = ((Color >> 10) & 0x1f) << 3;
                buffer[x].green = ((Color >> 5) & 0x1f) << 3;
                buffer[x].blue = (Color & 0x1f) << 3;
                //                        m_lineData[Index].Alpha = 255;
                x++;
                linePtr += 2;
                ///////////
            }
        }
    } else if (m_bInfo.biBitCount == 24) {
        for (int y = 0; y < m_height; y++) {
            m_file.read((char*)lineBuffer, size);
            uint8_t *linePtr = lineBuffer;
            for (int j = 0; j < m_width; j++) {
                ///////////
                
                uint32_t Color = *((uint32_t*) linePtr); // This trips up the address sanitizer because we are accessing 4 bytes, when there may only be 3, even with 4 byte alined data. Consider: width=12, 12*3=36. 36 is already 4 byte aligned (9 uint's go into it), but the last one trips this up. It is safe, unless we access the last byte of linePtr..but it is ugly the way this is done.
                buffer[x].blue = Color & 0xff;
                buffer[x].green = (Color >> 8) & 0xff;
                buffer[x].red = (Color >> 16) & 0xff;
                //                        m_lineData[Index].Alpha = 255;
                x++;
                linePtr += 3;
                ///////////
                
            }
        }
    } else if (m_bInfo.biBitCount == 32) {
        for (int y = 0; y < m_height; y++) {
            m_file.read((char*)lineBuffer, size);
            uint8_t *linePtr = lineBuffer;
            for (int j = 0; j < m_width; j++) {
                ///////////
                
                uint32_t Color = *((uint32_t*) linePtr);
                buffer[x].blue = Color & 0xff;
                buffer[x].green = (Color >> 8) & 0xff;
                buffer[x].red = (Color >> 16) & 0xff;
                //                        m_lineData[Index].Alpha = Color >> 24;
                x++;
                linePtr += 4;
                
                ///////////
                
            }
        }
    } else {
        m_isValid = false; // bad data
    }
    
    free(lineBuffer);
}

// Fills the entire buffer with CRGB data from the file
void CDPatternBitmap::fillEntireBufferFromFile_comp1(CRGB *buffer) {
    m_file.seekSet(m_dataOffset);
    size_t maxOffset = m_width*m_height;

    uint8_t Count = 0;
    uint8_t ColorIndex = 0;
    int x = 0, y = 0;
    int Index = 0;
    
    // assumes black..
    bzero(buffer, sizeof(CRGB)*maxOffset);

    bool stop = false;
    while (!stop && m_file.available() > 0) {
        m_file.read((char*) &Count, sizeof(uint8_t));
        m_file.read((char*) &ColorIndex, sizeof(uint8_t));
        
        if (Count > 0) {
            Index = x + y * m_width;
            for (int k = 0; k < Count; k++) {
                int offset = Index + k;
                ASSERT(offset < maxOffset);
                buffer[offset].red = m_colorTable[ColorIndex].red;
                buffer[offset].green = m_colorTable[ColorIndex].green;
                buffer[offset].blue = m_colorTable[ColorIndex].blue;
                //                        buffer[Index + k].Alpha = m_colorTable[ColorIndex].Alpha;
            }
            x += Count;
        } else if (Count == 0) {
            int Flag = ColorIndex;
            // https://ffmpeg.org/doxygen/0.6/msrledec_8c-source.html
            if (Flag == 0) { // end of line
                x = 0;
                y++;
            } else if (Flag == 1) {
                // end of picture code
                stop = true;
                // done!! return?
                break;
            } else if (Flag == 2) {
                char rx = 0;
                char ry = 0;
                m_file.read((char*) &rx, sizeof(char));
                m_file.read((char*) &ry, sizeof(char));
                x += rx;
                y += ry;
            } else {
                Count = Flag;
                Index = x + y * m_width;
                for (int k = 0; k < Count; k++) {
                    m_file.read((char*) &ColorIndex, sizeof(uint8_t));
                    int offset = Index + k;
                    ASSERT(offset < maxOffset);
                    buffer[offset].red = m_colorTable[ColorIndex].red;
                    buffer[offset].green = m_colorTable[ColorIndex].green;
                    buffer[offset].blue = m_colorTable[ColorIndex].blue;
//                    buffer[Index + k].Alpha = m_colorTable[ColorIndex].Alpha;
                }
                x += Count;
                if (m_file.curPosition() & 1) {
                    char tmp = 0;
                    m_file.read((char*) &tmp, sizeof(char));
                }

            }
        }
    }    
}

void CDPatternBitmap::uncompressed_fillRGBBufferFromYOffset(CRGB *buffer, int y) {
    if (y >= m_height || y < 0) {
        DEBUG_PRINTLN("bad y offset requested");
        return;
    }
    if (m_bInfo.biCompression != 0) {
        DEBUG_PRINTLN("bad compression, should be 0");
        return;
    }
    
    uint32_t width = m_width;
    
    // 4 byte aligned rows
    unsigned int lineWidth = ((width * m_bInfo.biBitCount / 8) + 3) & ~3;
    
    int x = 0;
    bool owned = false;
    size_t size = sizeof(uint8_t) * lineWidth;
    uint32_t offset = y *lineWidth;
    uint8_t *lineBuffer = getLineBufferAtOffset(size, offset, &owned);
    
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
            uint32_t Color = *((uint32_t*) linePtr); // This trips up the address sanitizer because we are accessing 4 bytes, when there may only be 3, even with 4 byte alined data. Consider: width=12, 12*3=36. 36 is already 4 byte aligned (9 uint's go into it), but the last one trips this up. It is safe, unless we access the last byte of linePtr..but it is ugly the way this is done.
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
    if (owned) {
        free(lineBuffer);
    }
}

#if 0
void CDPatternBitmap::uncompressed_fillRGBBufferFromYOffset(CRGB *buffer, int y) {
    if (y >= getHeight() || y < 0) {
        DEBUG_PRINTLN("bad y offset requested");
        return;
    }
    
    uint32_t width = getWidth();

    if (m_bInfo.biCompression == 0) {
        // see above..


    } else if (m_bInfo.biCompression == 1) { // RLE 8
        // not supported
        m_isValid = false;

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
#endif // disabled

//35k.. trying that
// with this, I still see: Free ram: 12071, total: 65536
// 18.4% free
#define MAX_SIZE_SINGLE_BUFFER (40*1024)

// Allocate the memory once for the patterns since we can share it; if we are in the sim, we may have multiple instances
static CRGB *g_sharedBuffer = NULL; // always NULL for the pattern editor

#ifdef PATTERN_EDITOR

static uint32_t heap_free() {
    return MAX_SIZE_SINGLE_BUFFER + 1025; // Doesn't matter
}

#else

#include "RamMonitor.h"
RamMonitor ram;
uint32_t heap_free() {
    return ram.free();
}

#endif

// may return NULL if it couldn't be allocated
static inline CRGB *getSharedBuffer() {
    CRGB *result = g_sharedBuffer;
    if (result == NULL) {
        uint32_t freeRam = heap_free();
        // Make sure we have extra room for the program to run (1kB enough?)
        if (freeRam > (MAX_SIZE_SINGLE_BUFFER + 1024)) {
            // allocate it
            result = (CRGB *)malloc(MAX_SIZE_SINGLE_BUFFER);
            // NOTE: I could probably get another 2KB! by using the other buffers passed in (if non NULL)
            // Save it if we aren't the pattern editor
#ifndef PATTERN_EDITOR
            // Save it only cached but NOT for the pattern editor... so we create it each time and own it
            g_sharedBuffer = result;
#endif
        } else {
            DEBUG_PRINTF("!!!!!!!!!!!! not enough free ram!: %d\r\n", freeRam);
        }
    }
    return result;
}


uint8_t *CDPatternBitmap::getLineBufferAtOffset(size_t size, uint32_t dataOffset, bool *owned) {
    if (m_bufferIsEntireFile) {
        uint8_t *result = (uint8_t *)m_buffer;
        *owned = false;
        return &result[dataOffset];
    } else {
        uint8_t *lineBuffer = (uint8_t *)malloc(size);     // corbin -- use shared buffer!!
        uint32_t lineOffset = m_dataOffset + dataOffset;
        m_file.seekSet(lineOffset);
#if DEBUG
        int amountRead = m_file.read((char*)lineBuffer, size);
        if (amountRead < size) {
            DEBUG_PRINTF("BITMAP ERROR: requested to read %d but read only %d\r\n", size, amountRead);
        }
#else
        m_file.read((char*)lineBuffer, size);
#endif
        *owned = true;
        return lineBuffer;
    }
}

CDPatternBitmap::CDPatternBitmap(const char *filename, CRGB *buffer1, CRGB *buffer2, size_t bufferSize) : m_xOffset(0), m_yOffset(-1), m_bufferOwned(false), m_buffer1Owned(false), m_buffer2Owned(false), m_bufferIsEntireFile(false), m_bufferIsFullCRGBData(false), m_colorTable(NULL)  {
    
    DEBUG_PRINTF("Bitmap loading: %s\r\n", filename);
    
#ifndef PATTERN_EDITOR
#if DEBUG
    Serial.printf("Free ram: %d, total: %d\r\n", ram.free(), ram.total());
    Serial.print((((float) ram.free()) / (float)ram.total()) * 100, 1);
    Serial.println("% <- free ram out of total ram");
#endif
#endif
    
    
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
    if (m_bInfo.biCompression != 0 && m_bInfo.biCompression != 1/*&& ( && m_bInfo.biCompression != 3*/) {
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
    
    // Cache hot spots
    m_width = m_bInfo.biWidth < 0 ? -m_bInfo.biWidth : m_bInfo.biWidth;
    m_height = m_bInfo.biHeight < 0 ? -m_bInfo.biHeight : m_bInfo.biHeight;
    
    
    m_dataOffset = fileHeader.bfOffBits;
    
    // buffer allocation and loading
    
    m_buffer = getSharedBuffer();
    if (m_buffer == NULL) {
        CLOSE_AND_RETURN("No shared buffer!?");
    }
    
#ifdef PATTERN_EDITOR
    m_bufferOwned = true; // The pattern editor owns it
#endif
    
    m_isValid = true;
    
    // This complexity IS needed, as loading a line at a time from the SD card IS TOO SLOW
    size_t totalMemoryNeeded = sizeof(CRGB)*m_width*m_height;
    
    // Try to allocate one big buffer to hold it all in RAM
    if (totalMemoryNeeded <= MAX_SIZE_SINGLE_BUFFER) {
        m_bufferIsFullCRGBData = true;
        fillEntireBufferFromFile(m_buffer);
    } else {
//            DEBUG_PRINTF("too large for a shared buffer when uncompressed; need %d, MAX_SIZE_SINGLE_BUFFER: %d, freeRam: %d\r\n", totalMemoryNeeded, MAX_SIZE_SINGLE_BUFFER, heap_free());
        
        // See if we can fit the SD card compressed data in the shared buffer so we don't load from the SD card a bunch
        uint32_t restDataSize = m_file.fileSize() - m_dataOffset;
        if (restDataSize <= MAX_SIZE_SINGLE_BUFFER) {
            m_file.seekSet(m_dataOffset);
            // suck it in
            m_file.read((char*)m_buffer, restDataSize);
            m_file.close();
            // The mark that we did this..
            m_bufferIsEntireFile = true;
        }
        
        // Try to use the passed in buffers
        size_t requiredBufferSize = sizeof(CRGB)*getWidth();
        if (bufferSize >= requiredBufferSize) {
            m_buffer1 = buffer1;
            m_buffer2 = buffer2;
            // Default is false
//                m_buffer1Owned = false;
//                m_buffer2Owned = false;
        } else {
            // Okay, we have to allocate
            if (m_bufferIsEntireFile) {
                // the shared buffer is in use
                if (2*requiredBufferSize < (heap_free() + 1024)) {
                    m_buffer1 = (CRGB *)malloc(requiredBufferSize);
                    m_buffer2 = (CRGB *)malloc(requiredBufferSize);
                    m_buffer1Owned = true; // It was allocated in this case
                    m_buffer2Owned = true;
                } else {
                    // Not enough RAM to allocate it
                    DEBUG_PRINTLN("not enough RAM for buffers");
                    m_isValid = false;
                }
            } else {
                // Use the shared buffer for one.
                m_buffer1 = m_buffer;
//                    m_buffer1Owned = false;
                // Allocate the second
                if (requiredBufferSize < (heap_free() + 1024)) {
                    m_buffer2 = (CRGB *)malloc(requiredBufferSize);
                    m_buffer2Owned = true; // It was allocated in this case
                } else {
                    // Not enough RAM to allocate it
                    DEBUG_PRINTLN("not enough RAM for buffers");
                    m_isValid = false;
                }
            }
        }
        
        // TODO: handle compression 1!!
        
        // if we are RLE, only do it if the entire thing can be in memory; otherwise it is going to be god awful slow disk access, and I want to know that is the issue by making it "invalid"
        if (m_bInfo.biCompression == 1) {
            m_isValid = false; // bad; not enough memory
            DEBUG_PRINTLN("not enough RAM for RLE 8 encoding");
        }
    }


    if (m_isValid) {
        incYOffsetBuffers();
    }
}


CDPatternBitmap::~CDPatternBitmap() {
    if (m_buffer1Owned) {
        if (m_buffer1) {
            free(m_buffer1);
        }
    }
    if (m_buffer2Owned) {
        if (m_buffer2) {
            free(m_buffer2);
        }
    }
    if (m_bufferOwned && m_buffer) {
        free(m_buffer);
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    
    if (m_colorTable) {
        free(m_colorTable);
    }
}

void CDPatternBitmap::updateBuffersWithYOffset(int offset, int oldOffset) {
    int height = getHeight();

    int secondBufferOffset = offset + 1;
    if (secondBufferOffset >= height) {
        secondBufferOffset = 0;
    }
    
    size_t width = getWidth();
    // If we loaded everything (non NULL buffer), then we can just compute the offset
    if (m_bufferIsFullCRGBData) {
        m_buffer1 = m_buffer + (offset * width);
        m_buffer2 = m_buffer + (secondBufferOffset * width);
    } else {
        // If this was the first time... we fill up both buffers
        if (oldOffset == -1) {
            uncompressed_fillRGBBufferFromYOffset(m_buffer1, offset);
            if (height == 1) {
                memcpy(m_buffer2, m_buffer1, sizeof(CRGB)*width);
            } else {
                uncompressed_fillRGBBufferFromYOffset(m_buffer2, secondBufferOffset);
            }
        } else {
            // Move the second to be the first
            CRGB *oldFirstBuffer = m_buffer1;
            m_buffer1 = m_buffer2;
            m_buffer2 = oldFirstBuffer;
            // well, we only have to fill it back up if there are more than 2 rows
            if (height > 2) {
                uncompressed_fillRGBBufferFromYOffset(m_buffer2, secondBufferOffset);
            }
        }
    }
}
