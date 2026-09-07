#pragma once
// DirectDraw ABI records. Included in ABI order by mapedit/runtime.hpp.
struct DD_DRIVER {
    char description[40];                 // +0x000
    void* pDeviceGUID;                    // +0x028
    uint8_t deviceGUID[16];               // +0x02C
    int videoMemory;                      // +0x03C
    int noModes;                          // +0x040
    int sizeX[16];                        // +0x044
    int sizeY[16];                        // +0x084
    int modesPixel[16];                   // +0x0C4 (D3DFORMAT)
    int desktopPixel;                     // +0x104 (D3DFORMAT)
    uint32_t flags;                       // +0x108, bit0=windowed
    DD_DRIVER();
    int GetMode(int size_x,int size_y,int bits_per_pixel);
    STRING GetModeDesctription(int mode);
};
static_assert(sizeof(DD_DRIVER)==0x10C, "debug metadata DD_DRIVER size");

struct DDPIXELFORMAT_OLD {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwRGBAlphaBitMask;
};
static_assert(sizeof(DDPIXELFORMAT_OLD)==0x20, "DX7 DDPIXELFORMAT ABI");

