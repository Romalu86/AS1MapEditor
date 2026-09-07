#pragma once
struct IDirectDrawSurface7;
// SURFACE/TEXTURE owners. Included in ABI order by mapedit/runtime.hpp.
class SURFACE {
public:
    virtual ~SURFACE();
    void* m_surface;       // +0x04 IDirectDrawSurface7*
    int m_format;          // +0x08 D3DFORMAT
    int m_sizeX;           // +0x0C
    int m_sizeY;           // +0x10
    int m_reserved14;      // +0x14

    SURFACE();
    SURFACE(int size_x,int size_y,D3DFORMAT format);
    operator IDirectDrawSurface7*();
    int IsExist();
    int IsPaletted();
    int SizeX();
    int SizeY();
    unsigned char* Lock(int* pitch,RECT_OLD* rect);
    void UnLock();
    long CopyFromSurface(void* source_surface,RECT_OLD* source,POINT_OLD* dest);
    int MemorySize();
    int Format();
    void SetPalette(COLOR* palette);
    static void Error(int type,const char* text,unsigned long err);
    static int MemoryInUse();
};
static_assert(sizeof(SURFACE)==0x18, "debug metadata SURFACE size");

class TEXTURE : public SURFACE {
public:
    unsigned long property; // +0x18
    TEXTURE(int size_x,int size_y,int format,unsigned long property);
    virtual ~TEXTURE();
    void Draw(unsigned long vertex_shader,void* vertex,unsigned int size_one_vertex);
    void Draw(RECT_OLD* screen,RECT_OLD* tex,const GAMMA* gamma);
    void Draw(float z1,float z2,RECT_OLD* screen,RECT_OLD* tex,const GAMMA* gamma);
    void SetTexture(int stage);
    static void SetDeviceCaps(void* d3d_device);
};
static_assert(sizeof(TEXTURE)==0x1C, "debug metadata TEXTURE size");

char* GetPixelFormat(DDPIXELFORMAT_OLD* format);
char* GetPixelFormat(int format);

