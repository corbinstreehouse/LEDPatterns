
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

CDLazyBitmap::CDLazyBitmap(const char *filename) : m_colorTable(NULL), m_lineData(NULL) {
    DEBUG_PRINTF("Bitmap loading: %s\r\n", filename);
    m_filename = filename ? strdup(filename) : NULL;
    m_isValid = false;

    m_file = SdFile(filename, O_READ);
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
    if (m_bInfo.biCompression != 0 && /*(m_bInfo.biCompression != 1 && */m_bInfo.biCompression != 3) {
        CLOSE_AND_RETURN("Not supported type of compression");
    }
    
    if (m_bInfo.biBitCount != 32 && m_bInfo.biBitCount != 24 && m_bInfo.biBitCount != 8 && m_bInfo.biBitCount != 4 && m_bInfo.biBitCount != 1) {
        CLOSE_AND_RETURN("Not supported color depth");
    }
    
    if (m_bInfo.biBitCount <= 8) {
        DEBUG_PRINTF("reading palette: %s\r\n", filename);
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
    
    m_currentLineY = -1;
    m_dataOffset = fileHeader.bfOffBits;
    
    // Load the first line
    loadLineDataAtY(0);
    m_isValid = true;
}

void CDLazyBitmap::loadLineDataAtY(int y) {
    if (y >= getHeight() || y < 0) {
        DEBUG_PRINTLN("bad y offset requested");
        return;
    }
    if (y != m_currentLineY) {
        m_currentLineY = y;
        
        if (m_lineData == NULL) {
            m_lineData = (CRGB *)malloc(sizeof(CRGB)*getWidth());
        }
        
        // 4 byte aligned rows
        unsigned int lineWidth = ((getWidth() * m_bInfo.biBitCount / 8) + 3) & ~3;
        uint8_t *lineBuffer = (uint8_t *)malloc(sizeof(uint8_t) * lineWidth);
        
        // Seek to that line
        uint32_t lineOffset = m_dataOffset + y * lineWidth;

        m_file.seekSet(lineOffset);
        
        int Index = 0;
        if (m_bInfo.biCompression == 0) {
//            for (unsigned int i = 0; i < getHeight(); i++)
            {
                m_file.read((char*)lineBuffer, lineWidth);
                
                uint8_t *linePtr = lineBuffer;
                uint32_t width = getWidth();
                for (unsigned int j = 0; j < width; j++) {
                    if (m_bInfo.biBitCount == 1) {
                        uint32_t Color = *((uint8_t*) linePtr);
                        for (int k = 0; k < 8; k++) {
                            m_lineData[Index].red = m_colorTable[Color & 0x80 ? 1 : 0].red;
                            m_lineData[Index].green = m_colorTable[Color & 0x80 ? 1 : 0].green;
                            m_lineData[Index].blue = m_colorTable[Color & 0x80 ? 1 : 0].blue;
//                            m_lineData[Index].Alpha = m_colorTable[Color & 0x80 ? 1 : 0].Alpha;
                            Index++;
                            Color <<= 1;
                        }
                        linePtr++;
                        j += 7;
                    } else if (m_bInfo.biBitCount == 4) {
                        uint32_t Color = *((uint8_t*) linePtr);
                        m_lineData[Index].red = m_colorTable[(Color >> 4) & 0x0f].red;
                        m_lineData[Index].green = m_colorTable[(Color >> 4) & 0x0f].green;
                        m_lineData[Index].blue = m_colorTable[(Color >> 4) & 0x0f].blue;
//                        m_lineData[Index].Alpha = m_colorTable[(Color >> 4) & 0x0f].Alpha;
                        Index++;
                        m_lineData[Index].red = m_colorTable[Color & 0x0f].red;
                        m_lineData[Index].green = m_colorTable[Color & 0x0f].green;
                        m_lineData[Index].blue = m_colorTable[Color & 0x0f].blue;
//                        m_lineData[Index].Alpha = m_colorTable[Color & 0x0f].Alpha;
                        Index++;
                        linePtr++;
                        j++;
                    } else if (m_bInfo.biBitCount == 8) {
                        uint32_t Color = *((uint8_t*) linePtr);
                        ASSERT(m_bInfo.biClrUsed == 0 || Color < m_bInfo.biClrUsed);
                        m_lineData[Index].red = m_colorTable[Color].red;
                        m_lineData[Index].green = m_colorTable[Color].green;
                        m_lineData[Index].blue = m_colorTable[Color].blue;
//                        m_lineData[Index].Alpha = m_colorTable[Color].Alpha;
                        Index++;
                        linePtr++;
                    } else if (m_bInfo.biBitCount == 16) {
                        uint32_t Color = *((uint16_t*) linePtr);
                        m_lineData[Index].red = ((Color >> 10) & 0x1f) << 3;
                        m_lineData[Index].green = ((Color >> 5) & 0x1f) << 3;
                        m_lineData[Index].blue = (Color & 0x1f) << 3;
//                        m_lineData[Index].Alpha = 255;
                        Index++;
                        linePtr += 2;
                    } else if (m_bInfo.biBitCount == 24) {
                        uint32_t Color = *((uint32_t*) linePtr);
                        m_lineData[Index].blue = Color & 0xff;
                        m_lineData[Index].green = (Color >> 8) & 0xff;
                        m_lineData[Index].red = (Color >> 16) & 0xff;
//                        m_lineData[Index].Alpha = 255;
                        Index++;
                        linePtr += 3;
                    } else if (m_bInfo.biBitCount == 32) {
                        uint32_t Color = *((uint32_t*) linePtr);
                        m_lineData[Index].blue = Color & 0xff;
                        m_lineData[Index].green = (Color >> 8) & 0xff;
                        m_lineData[Index].red = (Color >> 16) & 0xff;
//                        m_lineData[Index].Alpha = Color >> 24;
                        Index++;
                        linePtr += 4;
                    }
                }
            }
        } else if (m_bInfo.biCompression == 1) { // RLE 8
#if 0
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
        
        
        free(lineBuffer);
        // done w/the file, so close it now
        if (getHeight() == 1) {
            m_file.close();
        }
    }
}

CRGB *CDLazyBitmap::getLineDataAtY(int y) {
    loadLineDataAtY(y);
    return m_lineData;
}

CDLazyBitmap::~CDLazyBitmap() {
    if (m_file.isOpen()) {
        m_file.close();
    }

    if (m_filename) {
        free(m_filename);
    }
    if (m_colorTable) {
        free(m_colorTable);
    }
    
    if (m_lineData) {
        free(m_lineData);
    }
}

