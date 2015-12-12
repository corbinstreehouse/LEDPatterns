// I wrote most this

#ifndef _CD_LAZY_BITMAP_H_
#define _CD_LAZY_BITMAP_H_

#include <stdlib.h>
#include "pixeltypes.h" // for CRGB
#include "SdFat.h"

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


class CDLazyBitmap {
private:
    CDBitmapInfoHeader m_bInfo;
    CDBitmapInfoHeaderV4 m_bInfoV4;
    char *m_filename;
    bool m_isValid;
    CDBitmapColorPaletteEntryRef m_colorTable;
    CRGB *m_lineData;
    int m_currentLineY;
    uint32_t m_dataOffset;
    uint32_t m_fileIsOpen:1;
    uint32_t m_reserved:31;
    SdFile m_file;
    
    void loadLineDataAtY(int y);
public:
    CDLazyBitmap(const char *filename);
    ~CDLazyBitmap();
    
    inline bool getIsValid() { return m_isValid; }
    inline const char *getFilename() { return m_filename; }
    
    inline uint32_t getWidth() {
		return m_bInfo.biWidth < 0 ? -m_bInfo.biWidth : m_bInfo.biWidth;
	}
	
	inline uint32_t getHeight() {
		return m_bInfo.biHeight < 0 ? -m_bInfo.biHeight : m_bInfo.biHeight;
	}
    
    // y can be from 0 to height
    CRGB *getLineDataAtY(int y);
};






#endif