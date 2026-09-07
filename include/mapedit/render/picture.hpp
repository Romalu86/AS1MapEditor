#pragma once
// PICTURE owner family. Included in ABI order by mapedit/runtime.hpp.
class PICTURE_BASE {
public:
    PICTURE_BASE();
    PICTURE_BASE(int size_x,int size_y,int bytes_per_pixel);
    virtual ~PICTURE_BASE();
    virtual void NextFrame();
    virtual void Rewind();
    virtual int Load(const STRING* filename);
    virtual void Close();
    uint8_t opaque[1064];
    void Error(TYPE_ERROR type,char* text,unsigned long err);
    void SetSize(int size_x,int size_y,int bytes_per_pixel);
    int SizeX();
    int SizeY();
    int IsPaletted();
    int IsOpened();
    int InViewPort(int x,int y);
    int BytesPerPixel();
    int FrameTime();
    int CurFrame();
    int NoFrame();
    const COLOR* GetPalette();
    void SetPalette(const COLOR* palette);
    COLOR GetPixel(int x,int y);
    unsigned int GetData(int x,int y);
    void PutData(int x,int y,unsigned int data);
    void PutPixel(int x,int y,COLOR color);
    void SaveTGA(const STRING* filename,int x,int y,int sizeX,int sizeY);
    STRING FileName();
};
static_assert(sizeof(PICTURE_BASE)==1068, "debug metadata PICTURE_BASE size");

class PICTURE {
public:
    enum PICTURE_TYPE { TYPE_UNKNOWN=0, TYPE_TGA=1, TYPE_BMP=2, TYPE_FLC=3, TYPE_VID=4, TYPE_Z=5 };
    PICTURE();
    PICTURE(int size_x,int size_y,PICTURE_TYPE create_type);
    virtual ~PICTURE();
    PICTURE_BASE* picture;
    int type;
    int Load(const STRING* filename);
    void Close();
    void Rewind();
    void NextFrame();
    int IsZ();
    int IsOpened();
    int InViewPort(int x,int y);
    int SizeX();
    int SizeY();
    int NoFrame();
    int CurFrame();
    int FrameTime();
    int IsPaletted();
    int BytesPerPixel();
    const COLOR* GetPalette();
    void SetPalette(const COLOR* palette);
    void SetSize(int size_x,int size_y,int bytes_in_pixel);
    COLOR GetPixel(int x,int y);
    unsigned int GetData(int x,int y);
    void PutData(int x,int y,unsigned int data);
    void PutPixel(int x,int y,COLOR color);
    void SaveTGA(const STRING* filename,int x,int y,int sizeX,int sizeY);
    STRING FileName();
};
static_assert(sizeof(PICTURE)==12, "debug metadata PICTURE size");

class PICTURE_MAKEVID {
public:
    PICTURE_MAKEVID();
    PICTURE_MAKEVID(int size_x,int size_y,unsigned long create_layer);
    virtual ~PICTURE_MAKEVID();                                  // vtable +0x00
    virtual void NextFrame();                                    // +0x04
    virtual void Rewind();                                       // +0x08
    virtual int Load(STRING file,STRING alphafile,STRING zfile); // +0x0C
    virtual void Close();                                        // +0x10
    PICTURE texture;                 // +0x004
    PICTURE alpha;                   // +0x010
    PICTURE zBuffer;                 // +0x01C
    COLOR commonPalette[256];        // +0x028
    unsigned char* paletteDecode;    // +0x428
    unsigned int vidType;            // +0x42C
    int CalcCRC32();
    int SizeX();
    int SizeY();
    int NoFrame();
    int CurFrame();
    int IsPaletted();
    int InViewPort(int x,int y);
    const COLOR* GetPalette();
    unsigned int GetData(int x,int y);
    void PutData(int x,int y,unsigned int data);
    COLOR GetPixel(int x,int y);
    unsigned int GetAlpha(int x,int y);
    short GetPixelZ(int x,int y);
    short GetPixelZInRect(int x,int y,int width,int height);
    unsigned long GetPixelT(int x,int y);
    void GetRectangle(int* begx,int* begy,int* endx,int* endy);
    int IsPixel(int x,int y);
    int IsPixelInRect(int x,int y,int width,int height);
    int WriteSurfaces(unsigned char* outbuf,unsigned short* buf,int surf_sizex,int surf_sizey);
    void WriteHardware(RESOURCE* res);
    int GetShadow(void* buffer);
    int GetSoftwareRectangle(void* buffer,int x0,int y0,int x1,int y1);
    void CreateOnePalette();
    void WriteSoftware(RESOURCE* res);
    void WritePseudo3d(RESOURCE* res);
    void WriteLight(RESOURCE* res);
    int MakeVid(unsigned int opt,STRING vidname);
    int GetPaletteNumber(COLOR color);
    void SetPaletteDecodeNumber(COLOR color,unsigned char number);
    void PutPixel(int x,int y,COLOR color);
    void PutPixelZ(int x,int y,int z);
    void Error(TYPE_ERROR type,char* text,unsigned long err);
};
static_assert(sizeof(PICTURE_MAKEVID)==1072, "debug metadata PICTURE_MAKEVID size");

// MapEdit debug metadata: PICTURE_FONT embeds a second complete PICTURE_MAKEVID at
// +0x430.  Its vtable overrides NextFrame/Rewind/Load and reuses base Close.
class PICTURE_FONT : public PICTURE_MAKEVID {
public:
    PICTURE_FONT();
    virtual ~PICTURE_FONT();
    virtual void NextFrame();
    virtual void Rewind();
    virtual int Load(STRING file,STRING alphafile,STRING zfile);

    PICTURE_MAKEVID font; // +0x430
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(PICTURE_FONT,font)==0x430,"debug metadata PICTURE_FONT embedded-source ABI");
static_assert(sizeof(PICTURE_FONT)==0x860,"debug metadata PICTURE_FONT size");
#ifdef __clang__
#pragma clang diagnostic pop
#endif

