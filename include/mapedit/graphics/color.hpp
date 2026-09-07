#pragma once
// GRAPH_INIT/GAMMA/COLOR owners. Included in ABI order by mapedit/runtime.hpp.
struct GRAPH_INIT {
    char gameName[256];
    uint32_t screenModeX[32]; uint32_t screenModeY[32]; uint32_t screenModeColorDepth[8];
    uint32_t options; uint32_t defaultDevice;
    int defaultScreenX,defaultScreenY,defaultColorDepth,defaultFullScreen;
    int HaveMode(unsigned int size_x,unsigned int size_y,unsigned int color_depth);
};
static_assert(sizeof(GRAPH_INIT)==0x238, "MapEdit GRAPH_INIT size");

class COLOR;
class GAMMA {
public:
    enum GAMMA_CREATE { DECODE };
    uint32_t subtractive;
    uint32_t additive;

    GAMMA() : subtractive(0), additive(0) {}
    GAMMA(int red,int green,int blue);
    GAMMA(int alpha,int red,int green,int blue);
    GAMMA(GAMMA c1,GAMMA c2);
    GAMMA(COLOR diffuseColor,COLOR specularColor);
    GAMMA(const GAMMA* other);
    GAMMA(GAMMA_CREATE type,unsigned long gamma);
    const GAMMA* SetAlpha(int alpha);
    const GAMMA* SetRed(int red);
    const GAMMA* SetGreen(int green);
    const GAMMA* SetBlue(int blue);
    int Alpha();
    int Red();
    int Green();
    int Blue();
    unsigned long EncodeToDword();
    const GAMMA* operator=(const GAMMA* other);
    const GAMMA operator+(const GAMMA* other);
    const GAMMA* operator+=(const GAMMA* other);
    int operator==(const GAMMA* other);
    int IsDefault();
    unsigned long Diffuse();
    unsigned long Specular();
    int Write(STREAM* res);
    int Read(STREAM* res);
};
static_assert(sizeof(GAMMA)==8, "MapEdit GAMMA size");

GAMMA InterpolateGamma(const GAMMA* gamma1,const GAMMA* gamma2,float interpolation);

class RGB16;
struct RGB565 { uint16_t color; };
struct RGB555 {
    uint16_t color;
    RGB555(const COLOR* r);
    RGB555(const RGB565* r);
};
static_assert(sizeof(RGB555)==2, "debug metadata RGB555 size");
static_assert(sizeof(RGB565)==2, "debug metadata RGB565 size");
class COLOR {
public:
    uint32_t color;
    COLOR();
    COLOR(int red,int green,int blue);
    COLOR(int alpha,int red,int green,int blue);
    COLOR(const COLOR& r);
    COLOR(const RGB16* r);
    COLOR(const RGB555* r);
    COLOR(const GAMMA* gamma,const COLOR* col);
    unsigned int Red();
    unsigned int Green();
    unsigned int Blue();
    unsigned int Alpha();
    unsigned int ARGB32();
    unsigned int RGB24();
    unsigned int RGB565();
    int NearestInPalette(const COLOR* palette,int no_palette);
    int Diff(const COLOR* col2);
    int Sqr(int diff);
    COLOR AlphaAdd(COLOR r,unsigned int alpha);
    COLOR PrepareForOutputAlpha();
    COLOR SetAlpha(int alpha);
    void Write(STREAM* stream);
    void Read(STREAM* stream);
    const COLOR* operator=(const COLOR* r);
    int operator==(const COLOR* r);
};

class RGB16 {
public:
    uint16_t color;
    static uint16_t rMask;
    static uint16_t gMask;
    static int rShift;
    static int gShift;
    RGB16();
    RGB16(const COLOR* r);
    RGB16(const RGB16* r);
    RGB16(int red,int green,int blue);
    static void SetFormat(int rgb565);
};
static_assert(sizeof(RGB16)==2, "debug metadata RGB16 size");
static_assert(sizeof(COLOR)==4, "debug metadata COLOR size");

