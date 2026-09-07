#include "mapedit/runtime.hpp"

extern "C" long __stdcall DirectDrawCreateEx(void* guid,void** directDraw,const void* iid,void* outerUnknown);
extern "C" long __stdcall DirectDrawEnumerateExA(void* callback,void* context,unsigned long flags);
void* __cdecl operator new(unsigned int,void* place);


namespace {
struct MovieGuid32 {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
};
static_assert(sizeof(MovieGuid32)==16,"Win32 GUID ABI");
const MovieGuid32 kMovieClsidFilterGraph={0xE436EBB3u,0x524Fu,0x11CEu,{0x9F,0x53,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
const MovieGuid32 kMovieIidGraphBuilder ={0x56A868A9u,0x0AD4u,0x11CEu,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
const MovieGuid32 kMovieIidMediaControl ={0x56A868B1u,0x0AD4u,0x11CEu,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
const MovieGuid32 kMovieIidMediaEvent   ={0x56A868B6u,0x0AD4u,0x11CEu,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
const MovieGuid32 kMovieIidVideoWindow  ={0x56A868B4u,0x0AD4u,0x11CEu,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
extern "C" long __stdcall CoCreateInstance(const MovieGuid32& rclsid,void* outer,unsigned long clsContext,
                                             const MovieGuid32& riid,void** object);

long MovieQueryInterface(void* object,const MovieGuid32& iid,void** out)
{
    void** vtable=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *Method)(void*,const MovieGuid32&,void**);
    return reinterpret_cast<Method>(vtable[0])(object,iid,out);
}

long MovieRenderFile(void* graph,const unsigned short* filename)
{
    void** vtable=*reinterpret_cast<void***>(graph);
    typedef long (__stdcall *Method)(void*,const unsigned short*,const unsigned short*);
    return reinterpret_cast<Method>(vtable[0x34/4])(graph,filename,0);
}
}


namespace {
// DirectDraw7 IID embedded at MapEdit.exe 0x004B7B3C.
const unsigned char kIidDirectDraw7[16] = {
    0xC0,0x5E,0xE6,0x15,0x9C,0x3B,0xD2,0x11,
    0xB9,0x2F,0x00,0x60,0x97,0x97,0xEA,0x5B
};

int g_driverEnumeratedCount=0; // retail 0x004CE200
void ResetEnumeratedPixelFormatsRetail();

int ConvertPixelFormat(const DDPIXELFORMAT_OLD* p)
{
    const unsigned long flags=p->dwFlags;
    if (flags&0x20u) return 0x29;
    if (flags&0x400u) {
        if (flags&0x4000u) {
            if (p->dwRGBBitCount==16u && p->dwRBitMask==1u) return 0x49;
            if (p->dwRGBBitCount==24u && p->dwRBitMask==8u) return 0x4B;
            return 0;
        }
        if (p->dwRGBBitCount==16u) return 0x50;
        if (p->dwRGBBitCount==32u) return 0x47;
        if (p->dwRGBBitCount==24u) return 0x4D;
        return 0;
    }
    if (flags&0x2u) {
        if (p->dwRGBBitCount==32u && p->dwRGBAlphaBitMask==0xFF000000u) return 0x15;
        if (p->dwRGBBitCount==16u && p->dwRGBAlphaBitMask==0xF000u) return 0x1A;
        if (p->dwRGBBitCount==16u && p->dwRGBAlphaBitMask==0x8000u) return 0x19;
        return 0;
    }
    if (p->dwRGBBitCount==32u && p->dwBBitMask==0xFFu) return 0x16;
    if (p->dwRGBBitCount==16u && p->dwGBitMask==0x7E0u) return 0x17;
    if (p->dwRGBBitCount==24u) return 0x14;
    if (p->dwGBitMask==0x3E0u) return 0x18;
    return 0;
}

__declspec(noinline) int FormatBPP(const DDPIXELFORMAT_OLD* p)
{
    return p->dwBBitMask<=0xFFu ? static_cast<int>(p->dwRGBBitCount) : 0;
}

struct DdSurfaceDescRetail {
    unsigned char bytes[0x7C];
    int Width() const { return *reinterpret_cast<const int*>(bytes+0x0C); }
    int Height() const { return *reinterpret_cast<const int*>(bytes+0x08); }
    DDPIXELFORMAT_OLD* PixelFormat() { return reinterpret_cast<DDPIXELFORMAT_OLD*>(bytes+0x48); }
    const DDPIXELFORMAT_OLD* PixelFormat() const { return reinterpret_cast<const DDPIXELFORMAT_OLD*>(bytes+0x48); }
};
static_assert(sizeof(DdSurfaceDescRetail)==0x7C,"DDSURFACEDESC2 retail size used by MapEdit");

long __stdcall ModeEnumCallback(DdSurfaceDescRetail* pdds,void* data)
{
    DD_DRIVER* driver=static_cast<DD_DRIVER*>(data);
    const int bpp=FormatBPP(pdds->PixelFormat());
    const int selected=GraphInit.HaveMode(static_cast<unsigned int>(pdds->Width()),
                                          static_cast<unsigned int>(pdds->Height()),
                                          static_cast<unsigned int>(bpp));
    if (::Error) {
        // Retail ModeEnumCallback converts DDPIXELFORMAT to D3DFORMAT first
        // (MapEdit.exe 0x00401B28/0x00401B65) and then uses GetPixelFormat(D3DFORMAT).
        // Calling the DDPIXELFORMAT overload here produced "RGB 888" instead of
        // the retail "X8R8G8B8" log token.
        const int pixelFormat=ConvertPixelFormat(pdds->PixelFormat());
        MYERROR::Log(::Error,
                     selected ? "    Enum display modes %ix%i %s - selected"
                              : "    Enum display modes %ix%i %s",
                     pdds->Width(),pdds->Height(),GetPixelFormat(pixelFormat));
    }
    if (!GraphInit.HaveMode(static_cast<unsigned int>(pdds->Width()),
                            static_cast<unsigned int>(pdds->Height()),
                            static_cast<unsigned int>(bpp)))
        return 1;

    const int n=driver->noModes;
    driver->sizeX[n]=pdds->Width();
    driver->sizeY[n]=pdds->Height();
    driver->modesPixel[n]=ConvertPixelFormat(pdds->PixelFormat());
    driver->noModes=n+1;
    return 1;
}

unsigned long ReleaseDirectDrawRetail(void* object)
{
    void** vt=*reinterpret_cast<void***>(object);
    typedef unsigned long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vt[0x08/4])(object);
}

long GetDirectDrawCapsRetail(void* object,void* caps)
{
    void** vt=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *Method)(void*,void*,void*);
    return reinterpret_cast<Method>(vt[0x2C/4])(object,caps,0);
}

long GetDirectDrawDisplayModeRetail(void* object,void* desc)
{
    void** vt=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *Method)(void*,void*);
    return reinterpret_cast<Method>(vt[0x30/4])(object,desc);
}

long EnumDirectDrawModesRetail(void* object,void* context)
{
    void** vt=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *Method)(void*,unsigned long,void*,void*,void*);
    return reinterpret_cast<Method>(vt[0x20/4])(object,0,0,context,reinterpret_cast<void*>(&ModeEnumCallback));
}

int __stdcall DirectDrawEnumCallback(void* guid,char* description,char* name,void* data,void*)
{
    DD_DRIVER* const drivers=static_cast<DD_DRIVER*>(data);
    DD_DRIVER* const driver=&drivers[g_driverEnumeratedCount];

    unsigned char caps[0x17C];
    *reinterpret_cast<unsigned long*>(caps)=0x17Cu;
    void* dd=0;
    const long createResult=DirectDrawCreateEx(guid,&dd,kIidDirectDraw7,0);
    if (createResult) {
        Graph->Error(3,"DDraw during enumeration",static_cast<unsigned long>(createResult));
        return 1;
    }

    GetDirectDrawCapsRetail(dd,caps);
    if (::Error)
        MYERROR::Log(::Error,"Enum Driver: %s - %s VideoMemory=%i",
                     description,name,*reinterpret_cast<int*>(caps+0x3C));

    int descriptionEnded=0;
    for (int i=0;i<0x27;++i) {
        const char c=descriptionEnded ? 0 : description[i];
        driver->description[i]=c;
        if (!c) descriptionEnded=1;
    }
    if (guid) {
        memcpy(driver->deviceGUID,guid,16u);
        driver->pDeviceGUID=driver->deviceGUID;
    } else {
        driver->pDeviceGUID=0;
    }

    if ((*reinterpret_cast<unsigned long*>(caps+0x08)&0x80000u) && !guid)
        driver->flags|=1u;
    if (!(*reinterpret_cast<unsigned long*>(caps+0x04)&0x400u))
        driver->flags|=2u;
    driver->videoMemory=*reinterpret_cast<int*>(caps+0x3C);

    DdSurfaceDescRetail desc;
    memset(&desc,0,sizeof(desc));
    *reinterpret_cast<unsigned long*>(desc.bytes)=0x7Cu;
    GetDirectDrawDisplayModeRetail(dd,&desc);
    driver->desktopPixel=ConvertPixelFormat(desc.PixelFormat());
    EnumDirectDrawModesRetail(dd,driver);

    if (!(*reinterpret_cast<unsigned long*>(caps+0x04)&1u) ||
        !(*reinterpret_cast<unsigned long*>(caps+0x16C)&0x20000u))
        return 1;

    ReleaseDirectDrawRetail(dd);
    ++g_driverEnumeratedCount;
    return 1;
}
}

int GRAPH_INIT::HaveMode(unsigned int size_x,unsigned int size_y,unsigned int color_depth)
{
    int color=0;
    while (screenModeColorDepth[color] && screenModeColorDepth[color]!=color_depth)
        ++color;
    if (!screenModeColorDepth[color]) return 0;
    int mode=0;
    while (screenModeX[mode]) {
        if (screenModeX[mode]==size_x && screenModeY[mode]==size_y) return 1;
        ++mode;
    }
    return 0;
}

// Exact MapEdit static COLOR owners.  BLACK is initialized at 0x004263AA;
// WHITE/GRAY/BLUE/LIGHTBLUE/GREEN/RED/LIGHTRED/YELLOW follow in the
// 0x00426426..0x0042656D static-initializer chain.
COLOR GRAPH::BLACK(0,0,0);
COLOR GRAPH::BLUE(0,0,255);
COLOR GRAPH::LIGHTBLUE(128,128,255);
COLOR GRAPH::WHITE(255,255,255);
COLOR GRAPH::GRAY(128,128,128);
COLOR GRAPH::GREEN(0,255,0);
COLOR GRAPH::RED(255,0,0);
COLOR GRAPH::LIGHTRED(255,128,128);
COLOR GRAPH::YELLOW(255,255,0);

namespace {
struct GraphAbiFields {
    uint32_t unknown00;
    uint32_t stateFlags;                 // +0x004, bit 0 = paused
    int pixelShader;                     // +0x008, logged by GRAPH::Init
    uint16_t snowRamp[256];              // +0x00C..+0x20B
    void* colorBuffer;                   // +0x20C
    float sizeX;                         // +0x210
    float sizeY;                         // +0x214
    int colorPitch;                      // +0x218
    unsigned short* zBuffer;             // +0x21C
    int zPitch;                          // +0x220
    float viewXMin;                      // +0x224
    float viewXMax;                      // +0x228
    float viewYMin;                      // +0x22C
    float viewYMax;                      // +0x230
    int driverCount;                     // +0x234
    DD_DRIVER drivers[8];                // +0x238, stride 0x10C
    int currentDriver;                   // +0xA98
    float effectShiftX;                  // +0xA9C
    float effectShiftY;                  // +0xAA0
    int effectVar1[16];                  // +0xAA4
    int effectVar2[16];                  // +0xAE4
    unsigned long effectStart[16];       // +0xB24
    int effectDuration[16];              // +0xB64
    GAMMA effectGamma;                   // +0xBA4
    GAMMA gamma;                         // +0xBAC
    uint32_t environment;                 // +0xBB4
    ANGLE windDirection;                  // +0xBB8
    float windForce;                      // +0xBBC
    uint8_t reservedBC0[0xBCC - 0xBC0];
    MOVIE movie;                         // +0xBCC
    HWND__* hwnd;                        // +0xBDC
    SURFACE* paletteSurface;              // +0xBE0
    SURFACE* surfaceBE4;                 // +0xBE4
    SURFACE* surfaceBE8;                 // +0xBE8
    uint8_t reservedBEC[0xBF0 - 0xBEC];
    int debugFontHeight;                 // +0xBF0
    uint8_t reservedBF4[0xBF8 - 0xBF4];
    void* directDraw;                    // +0xBF8
    void* direct3D7;                    // +0xBFC
    void* d3dDevice7;                    // +0xC00
    void* zSurface;                      // +0xC04; LockZ target
    void* realZSurface;                  // +0xC08; RealZBuffer target
    void* captureSourceSurface;          // +0xC0C; Effect(5) source
    void* colorSurface;                  // +0xC10
    void* effectSurface;                 // +0xC14; Effect(5) destination
};
static_assert(offsetof(GraphAbiFields, pixelShader) == 0x008, "GRAPH pixel-shader ABI");
static_assert(offsetof(GraphAbiFields, snowRamp) == 0x00C, "GRAPH snow-ramp ABI");
static_assert(offsetof(GraphAbiFields, colorBuffer) == 0x20C, "GRAPH color-buffer ABI");
static_assert(offsetof(GraphAbiFields, sizeX) == 0x210, "GRAPH sizeX ABI");
static_assert(offsetof(GraphAbiFields, zBuffer) == 0x21C, "GRAPH z-buffer ABI");
static_assert(offsetof(GraphAbiFields, viewXMin) == 0x224, "GRAPH viewport ABI");
static_assert(offsetof(GraphAbiFields, driverCount) == 0x234, "GRAPH driver-count ABI");
static_assert(offsetof(GraphAbiFields, drivers) == 0x238, "GRAPH drivers ABI");
static_assert(offsetof(GraphAbiFields, currentDriver) == 0xA98, "GRAPH current-driver ABI");
static_assert(offsetof(GraphAbiFields, effectShiftX) == 0xA9C, "GRAPH effect shift ABI");
static_assert(offsetof(GraphAbiFields, effectVar1) == 0xAA4, "GRAPH effect var1 ABI");
static_assert(offsetof(GraphAbiFields, effectVar2) == 0xAE4, "GRAPH effect var2 ABI");
static_assert(offsetof(GraphAbiFields, effectStart) == 0xB24, "GRAPH effect start ABI");
static_assert(offsetof(GraphAbiFields, effectDuration) == 0xB64, "GRAPH effect duration ABI");
static_assert(offsetof(GraphAbiFields, effectGamma) == 0xBA4, "GRAPH effect gamma ABI");
static_assert(offsetof(GraphAbiFields, gamma) == 0xBAC, "GRAPH gamma ABI");
static_assert(offsetof(GraphAbiFields, environment) == 0xBB4, "GRAPH environment ABI");
static_assert(offsetof(GraphAbiFields, windDirection) == 0xBB8, "GRAPH wind-direction ABI");
static_assert(offsetof(GraphAbiFields, windForce) == 0xBBC, "GRAPH wind-force ABI");
static_assert(offsetof(GraphAbiFields, movie) == 0xBCC, "GRAPH movie ABI");
static_assert(offsetof(GraphAbiFields, hwnd) == 0xBDC, "GRAPH hwnd ABI");
static_assert(offsetof(GraphAbiFields, paletteSurface) == 0xBE0, "GRAPH palette surface ABI");
static_assert(offsetof(GraphAbiFields, surfaceBE4) == 0xBE4, "GRAPH surface BE4 ABI");
static_assert(offsetof(GraphAbiFields, surfaceBE8) == 0xBE8, "GRAPH surface BE8 ABI");
static_assert(offsetof(GraphAbiFields, debugFontHeight) == 0xBF0, "GRAPH debug-font-height ABI");
static_assert(offsetof(GraphAbiFields, directDraw) == 0xBF8, "GRAPH DirectDraw ABI");
static_assert(offsetof(GraphAbiFields, direct3D7) == 0xBFC, "GRAPH Direct3D ABI");
static_assert(offsetof(GraphAbiFields, d3dDevice7) == 0xC00, "GRAPH D3D device ABI");
static_assert(offsetof(GraphAbiFields, zSurface) == 0xC04, "GRAPH z-surface ABI");
static_assert(offsetof(GraphAbiFields, realZSurface) == 0xC08, "GRAPH real-z surface ABI");
static_assert(offsetof(GraphAbiFields, captureSourceSurface) == 0xC0C, "GRAPH capture-source ABI");
static_assert(offsetof(GraphAbiFields, colorSurface) == 0xC10, "GRAPH surface ABI");
static_assert(offsetof(GraphAbiFields, effectSurface) == 0xC14, "GRAPH effect-surface ABI");

GraphAbiFields& Fields(GRAPH* graph)
{
    return *reinterpret_cast<GraphAbiFields*>(graph);
}

long DeviceSetTextureStageState(void* device,unsigned long stage,unsigned long state,unsigned long value)
{
    void** const vtable=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long,unsigned long);
    return reinterpret_cast<Method>(vtable[0x94/4])(device,stage,state,value);
}

long DeviceSetRenderState(void* device,unsigned long state,unsigned long value)
{
    void** const vtable=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long);
    return reinterpret_cast<Method>(vtable[0x50/4])(device,state,value);
}


unsigned long g_retailSquallStart=0;
float g_retailOldWindSpeed=-1.0f;
}

DD_DRIVER::DD_DRIVER()
{
    flags &= ~0x0Fu;
    noModes=0;
    description[0]=0;
}

float GRAPH::ViewXMin() { return Fields(this).viewXMin; }
float GRAPH::ViewXMax() { return Fields(this).viewXMax; }
float GRAPH::ViewYMin() { return Fields(this).viewYMin; }
float GRAPH::ViewYMax() { return Fields(this).viewYMax; }

int GRAPH::BoxInViewPort(float left,float top,float right,float bottom)
{
    const GraphAbiFields& f=Fields(this);
    return right>=f.viewXMin && left<f.viewXMax &&
           bottom>=f.viewYMin && top<f.viewYMax;
}

float GRAPH::SizeX() { return Fields(this).sizeX; }
float GRAPH::SizeY() { return Fields(this).sizeY; }
int GRAPH::IsPaused() { return (Fields(this).stateFlags & 1u) != 0; }

int GRAPH::Is15Bit()
{
    return Fields(this).surfaceBE4->Format()==24;
}

void GRAPH::LockZ()
{
    GRAPH_CORE::LockZ();
}

unsigned short* GRAPH::LockZ(int* pitch)
{
    LockZ();
    *pitch=Fields(this).zPitch;
    return Fields(this).zBuffer;
}

GAMMA GRAPH::GetGamma()
{
    return GAMMA(&Fields(this).gamma);
}

TEXTURE* GRAPH::AlphaBuffer()
{
    return reinterpret_cast<TEXTURE*>(Fields(this).surfaceBE8);
}

int GRAPH::CapsFullScreen()
{
    return (Fields(this).stateFlags >> 7) & 1u;
}

int GRAPH_CORE::InViewPort(float x,float y)
{
    GraphAbiFields& fields = Fields(reinterpret_cast<GRAPH*>(this));
    return x >= fields.viewXMin && x < fields.viewXMax &&
           y >= fields.viewYMin && y < fields.viewYMax;
}


COLOR GRAPH::GetPixel(float x,float y)
{
    Lock();
    GraphAbiFields& f=Fields(this);
    if (!InViewPort(x,y))
        return COLOR(0,0,0);
    const int ix=static_cast<int>(x);
    const int iy=static_cast<int>(y);
    const int index=ix+iy*f.colorPitch;
    if (f.stateFlags&2u)
        return reinterpret_cast<COLOR*>(f.colorBuffer)[index];
    return COLOR(&reinterpret_cast<RGB16*>(f.colorBuffer)[index]);
}

void GRAPH::SavePict(PICTURE* pict,int pict_x,int pict_y,int x,int y,int sizex,int sizey)
{
    Lock();
    for (int py=sizey-1;py>=0;--py) {
        for (int px=0;px<sizex;++px)
            pict->PutPixel(pict_x+px,pict_y+py,GetPixel(static_cast<float>(x+px),static_cast<float>(y+py)));
    }
}

void GRAPH::SavePictAndZ(PICTURE_MAKEVID* pict,int pict_x,int pict_y,int x,int y,int sizex,int sizey)
{
    Lock();
    LockZ();
    GraphAbiFields& f=Fields(this);
    for (int py=sizey-1;py>=0;--py) {
        for (int px=0;px<sizex;++px) {
            const float sx=static_cast<float>(x+px);
            const float sy=static_cast<float>(y+py);
            pict->PutPixel(pict_x+px,pict_y+py,GetPixel(sx,sy));
            const int z=InViewPort(sx,sy)
                ? static_cast<int>(f.zBuffer[(x+px)+(y+py)*f.zPitch])
                : 0x400;
            pict->PutPixelZ(pict_x+px,pict_y+py,z);
        }
    }
}

void GRAPH::SaveTGA(const STRING* file,int x,int y,int sizex,int sizey)
{
    PICTURE picture(sizex,sizey,PICTURE::TYPE_TGA);
    for (int py=0;py<sizey;++py) {
        for (int px=0;px<sizex;++px)
            picture.PutPixel(px,py,GetPixel(static_cast<float>(x+px),static_cast<float>(y+py)));
    }
    picture.SaveTGA(file,0,0,-1,-1);
}

void GRAPH::SaveZ(const STRING* fileName,int x,int y,int sizex,int sizey)
{
    FILE* file=FOpen(fileName,"r+b");
    int appendCount;
    if (!file) {
        file=FOpen(fileName,"wb");
        appendCount=1;
    } else {
        appendCount=0;
    }

    if (!file) {
        Error(7,fileName->m_buf,0);
        return;
    }

    GraphAbiFields& fields=Fields(this);
    int unlockAfter=0;
    if (!fields.zBuffer) {
        unlockAfter=1;
        LockZ();
    }

    const unsigned int id=0x6675425Au; // "ZBuf" in retail little-endian storage.
    fwrite(&id,4u,1u,file);
    fwrite(&sizex,4u,1u,file);
    fwrite(&sizey,4u,1u,file);

    if (appendCount==1) {
        fwrite(&appendCount,4u,1u,file);
    } else {
        fseek(file,12,0);
        fread(&appendCount,1u,4u,file);
        ++appendCount;
        fseek(file,12,0);
        fwrite(&appendCount,4u,1u,file);
        fseek(file,0,2);
    }

    for (int py=0;py<sizey;++py) {
        fwrite(&sizex,2u,1u,file);
        for (int px=0;px<sizex;++px) {
            int z=static_cast<int>(fields.zBuffer[(x+px)+(y+py)*fields.zPitch])-0x400;
            fwrite(&z,2u,1u,file);
        }
    }

    fclose(file);
    if (unlockAfter)
        UnLockZ();
}

void GRAPH::Box(float x,float y,float x1,float y1,COLOR color)
{
    Line(x,y,x1,y,color);
    Line(x,y1,x1,y1,color);
    Line(x,y,x,y1,color);
    Line(x1,y,x1,y1,color);
}



int MOVIE::IsOpen()
{
    return pGraph != 0;
}

int MOVIE::Update()
{
    if (!pEvent)
        return 1;
    long eventCode=0;
    void** vtable=*reinterpret_cast<void***>(pEvent);
    typedef long (__stdcall *WaitForCompletionMethod)(void*,long,long*);
    reinterpret_cast<WaitForCompletionMethod>(vtable[0x24/4])(pEvent,0,&eventCode);
    return eventCode==1;
}

namespace {
void* g_retailCachedCursor=0;
ICONINFO_OLD g_retailCursorInfo={0,0,0,0,0};
}

// The original presents through IDirectDrawSurface7 directly.  +0xC0C is the
// primary surface and +0xC10 is the back surface in this owner.
void GRAPH_CORE::PostTact(int draw)
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    UnLock();

    void** deviceVtable=*reinterpret_cast<void***>(fields.d3dDevice7);
    typedef long (__stdcall *DeviceNoArg)(void*);
    const long endScene=reinterpret_cast<DeviceNoArg>(deviceVtable[0x18/4])(fields.d3dDevice7);
    if (endScene)
        Error(10,"3dEndScene for PostTact",static_cast<unsigned long>(endScene));

    if (fields.movie.IsOpen())
        return;

    if (fields.stateFlags & 0x10u) {
        void* dc=0;
        void** backVtable=*reinterpret_cast<void***>(fields.colorSurface);
        typedef long (__stdcall *GetDcMethod)(void*,void**);
        reinterpret_cast<GetDcMethod>(backVtable[0x44/4])(fields.colorSurface,&dc);

        void* cursor=GetCursor();
        if (cursor != g_retailCachedCursor) {
            g_retailCachedCursor=cursor;
            GetIconInfo(cursor,&g_retailCursorInfo);
            if (g_retailCursorInfo.hbmMask)
                DeleteObject(g_retailCursorInfo.hbmMask);
            if (g_retailCursorInfo.hbmColor)
                DeleteObject(g_retailCursorInfo.hbmColor);
        }

        POINT_OLD point={0,0};
        GetCursorPos(&point);
        point.x-=static_cast<long>(g_retailCursorInfo.xHotspot);
        point.y-=static_cast<long>(g_retailCursorInfo.yHotspot);
        DrawIcon(dc,static_cast<int>(point.x),static_cast<int>(point.y),cursor);

        typedef long (__stdcall *ReleaseDcMethod)(void*,void*);
        reinterpret_cast<ReleaseDcMethod>(backVtable[0x68/4])(fields.colorSurface,dc);
    }

    if (!draw || fields.effectStart[6] || fields.effectStart[7])
        return;

    void** primaryVtable=*reinterpret_cast<void***>(fields.captureSourceSurface);
    // Retail 0x0040333A..0x00403439:
    //   windowed (!bit 0x80) OR option bit 0x400 -> Blt backbuffer to primary;
    //   true fullscreen (bit 0x80 && !bit 0x400) -> Flip primary chain.
    // A15 had this branch reversed, so the normal windowed editor attempted
    // IDirectDrawSurface7::Flip() and left the render window black.
    if (!(fields.stateFlags & 0x80u) || (fields.stateFlags & 0x400u)) {
        RECT_OLD windowRect={0,0,0,0};
        GetWindowRect(fields.hwnd,&windowRect);

        RECT_OLD sourceRect;
        sourceRect.left=static_cast<long>(fields.viewXMin);
        sourceRect.top=static_cast<long>(fields.viewYMin);
        sourceRect.right=static_cast<long>(fields.viewXMax);
        sourceRect.bottom=static_cast<long>(fields.viewYMax);

        RECT_OLD destRect;
        destRect.left=windowRect.left+sourceRect.left;
        destRect.top=windowRect.top+sourceRect.top;
        destRect.right=windowRect.left+sourceRect.right;
        destRect.bottom=windowRect.top+sourceRect.bottom;

        typedef long (__stdcall *BltMethod)(void*,RECT_OLD*,void*,RECT_OLD*,unsigned long,void*);
        const long presentHr=reinterpret_cast<BltMethod>(primaryVtable[0x14/4])(
            fields.captureSourceSurface,&destRect,fields.colorSurface,&sourceRect,0x01000000u,0);
        (void)presentHr;
    } else {
        typedef long (__stdcall *FlipMethod)(void*,void*,unsigned long);
        const unsigned long flipFlags=(fields.stateFlags & 0x100u) ? 0x1u : 0x9u;
        const long presentHr=reinterpret_cast<FlipMethod>(primaryVtable[0x2C/4])(
            fields.captureSourceSurface,0,flipFlags);
        (void)presentHr;
    }
}

void MOVIE::Pause()
{
    if (!pMediaControl)
        return;
    void** vtable=*reinterpret_cast<void***>(pMediaControl);
    typedef long (__stdcall *Method)(void*);
    reinterpret_cast<Method>(vtable[8])(pMediaControl);
}

void MOVIE::Resume()
{
    if (!pMediaControl)
        return;
    void** vtable=*reinterpret_cast<void***>(pMediaControl);
    typedef long (__stdcall *Method)(void*);
    reinterpret_cast<Method>(vtable[7])(pMediaControl);
}

void GRAPH_CORE::UnLock()
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    if (!fields.colorBuffer)
        return;
    void** vtable=*reinterpret_cast<void***>(fields.colorSurface);
    typedef long (__stdcall *UnlockMethod)(void*,void*);
    reinterpret_cast<UnlockMethod>(vtable[0x80/4])(fields.colorSurface,0);
    fields.colorBuffer=0;
}

void GRAPH_CORE::FlipToGDISurface()
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    void** vtable=*reinterpret_cast<void***>(fields.directDraw);
    typedef long (__stdcall *FlipMethod)(void*);
    reinterpret_cast<FlipMethod>(vtable[0x28/4])(fields.directDraw);
}

void GRAPH::DrawVid(VID* vid,int ncadr,float shiftx,float shifty,float shiftz)
{
    if (!vid || vid==EmptyVid)
        return;
    if (ncadr<0 || ncadr>=vid->m_dotFrameCount) {
        Error(4,"ncadr in DrawVid",static_cast<unsigned long>(ncadr));
        return;
    }

    GraphAbiFields& fields=Fields(this);
    const bool hadColor=fields.colorBuffer!=0;
    const bool hadZ=fields.zBuffer!=0;

    if (vid->m_layer==5 || vid->m_layer==6 || vid->m_layer==7) {
        LockZ();
        Lock();
    } else {
        UnLock();
        UnLockZ();
    }

    if (vid->m_layer<=8) {
        SetRenderState(0x0Eu,0u); // D3DRS_ZWRITEENABLE
        SetRenderState(0x1Bu,1u); // D3DRS_ALPHABLENDENABLE
    } else {
        SetRenderState(0x1Bu,0u);
        SetRenderState(0x0Eu,1u);
        SetRenderState(0x17u,7u); // D3DRS_ZFUNC, D3DCMP_GREATEREQUAL
    }

    SPRITE sprite(vid,Map->FromScreenX(shiftx),Map->FromScreenY(shifty),shiftz,
                  ANGLE(static_cast<unsigned char>(0)),0);
    sprite.m_noCadr=ncadr;
    void** const vtable=*reinterpret_cast<void***>(vid);
    typedef void (__thiscall *DrawMethod)(VID*,const SPRITE*);
    reinterpret_cast<DrawMethod>(vtable[0x0C/4])(vid,&sprite);

    if (hadColor) Lock(); else UnLock();
    if (hadZ) LockZ(); else UnLockZ();
}

void GRAPH::BeginPause()
{
    GraphAbiFields& fields=Fields(this);
    fields.stateFlags |= 1u;
    GRAPH_CORE::UnLock();
    GRAPH_CORE::FlipToGDISurface();
    DrawMenuBar(fields.hwnd);
    RedrawWindow(fields.hwnd,0,0,0x400u);
    fields.movie.Pause();
}

void GRAPH::EndPause()
{
    GraphAbiFields& fields=Fields(this);
    fields.stateFlags &= ~1u;
    fields.movie.Resume();
}


void GRAPH_CORE::Error(int type,const char* text,unsigned long err)
{
    if (::Error)
        MYERROR::Error(::Error,"GRAPH",type,text,err);
}

void* GRAPH_CORE::LockSurface(void* surface,int* pitch,RECT_OLD* rect)
{
    // DDSURFACEDESC2 is 0x7c bytes in DirectDraw7.  Retail initializes only
    // dwSize before IDirectDrawSurface7::Lock and then consumes lPitch/lpSurface.
    uint8_t desc[0x7c];
    *reinterpret_cast<uint32_t*>(desc)=0x7cu;
    void** vtable=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *LockMethod)(void*,RECT_OLD*,void*,unsigned long,void*);
    const long hr=reinterpret_cast<LockMethod>(vtable[0x64/4])(surface,rect,desc,0,0);
    if (hr) {
        Error(0,"LockSurface",static_cast<unsigned long>(hr));
        return 0;
    }
    *pitch=*reinterpret_cast<int*>(desc+0x10);
    return *reinterpret_cast<void**>(desc+0x24);
}

int GRAPH_CORE::BytesPerPixel()
{
    return (Fields(reinterpret_cast<GRAPH*>(this)).stateFlags & 2u) ? 4 : 2;
}

int GRAPH::BytesPerPixel()
{
    return GRAPH_CORE::BytesPerPixel();
}

void GRAPH_CORE::Lock()
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    if (fields.colorBuffer)
        return;
    fields.colorBuffer=LockSurface(fields.colorSurface,&fields.colorPitch,0);
    fields.colorPitch/=BytesPerPixel();
}

void GRAPH::Lock()
{
    GRAPH_CORE::Lock();
}

void* GRAPH::Lock(int* pitch)
{
    GRAPH_CORE::Lock();
    GraphAbiFields& f=Fields(this);
    *pitch=f.colorPitch;
    return f.colorBuffer;
}

void GRAPH_CORE::LockZ()
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    if (fields.zBuffer)
        return;
    fields.zBuffer=static_cast<unsigned short*>(LockSurface(fields.zSurface,&fields.zPitch,0));
    fields.zPitch/=2;
}

int GRAPH_CORE::RealZBuffer(float x,float y)
{
    if (!InViewPort(x,y))
        return 0;

    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    int result=0;
    int bytePitch=0;
    short* data=static_cast<short*>(LockSurface(fields.realZSurface,&bytePitch,0));
    if (data) {
        const int ix=static_cast<int>(x);
        const int iy=static_cast<int>(y);
        const int pitchWords=bytePitch/2;
        result=(static_cast<int>(data[ix+iy*pitchWords])-0x3ff)/8;

        void** vtable=*reinterpret_cast<void***>(fields.realZSurface);
        typedef long (__stdcall *UnlockMethod)(void*,void*);
        reinterpret_cast<UnlockMethod>(vtable[0x80/4])(fields.realZSurface,0);
    }
    return result;
}

void GRAPH::PutPixel(float x,float y,COLOR color)
{
    if (!InViewPort(x,y))
        return;
    Lock();
    GraphAbiFields& fields=Fields(this);
    const int ix=static_cast<int>(x);
    const int iy=static_cast<int>(y);
    const int index=ix+iy*fields.colorPitch;
    if (fields.stateFlags & 2u)
        reinterpret_cast<COLOR*>(fields.colorBuffer)[index]=&color;
    else
        reinterpret_cast<RGB16*>(fields.colorBuffer)[index]=RGB16(&color);
}

void GRAPH::PutBigPixel(float x,float y,COLOR color)
{
    Lock();
    GraphAbiFields& fields=Fields(this);
    if (x<fields.viewXMin || y<fields.viewYMin ||
        x>=fields.viewXMax-1.0f || y>=fields.viewYMax-1.0f)
        return;

    const int ix=static_cast<int>(x);
    const int iy=static_cast<int>(y);
    const int p=fields.colorPitch;
    const int i=ix+iy*p;
    if (fields.stateFlags & 2u) {
        COLOR* dst=reinterpret_cast<COLOR*>(fields.colorBuffer);
        dst[i+p+1]=&color;
        dst[i+1]=&color;
        dst[i+p]=&color;
        dst[i]=&color;
    } else {
        RGB16* dst=reinterpret_cast<RGB16*>(fields.colorBuffer);
        const RGB16 c(&color);
        dst[i+p+1].color=c.color;
        dst[i+1].color=c.color;
        dst[i+p].color=c.color;
        dst[i].color=c.color;
    }
}

long GRAPH_CORE::SetRenderState(unsigned long state,unsigned long value)
{
    if (state==0x17u || state==0x0Eu || state==0x07u)
        return 0;
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    void** vtable=*reinterpret_cast<void***>(fields.d3dDevice7);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long);
    return reinterpret_cast<Method>(vtable[0x50/4])(fields.d3dDevice7,state,value);
}

long GRAPH::SetAlphaBlend(unsigned long srcBlend,unsigned long destBlend)
{
    GraphAbiFields& fields=Fields(this);
    void** vtable=*reinterpret_cast<void***>(fields.d3dDevice7);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long);
    Method set=reinterpret_cast<Method>(vtable[0x50/4]);
    set(fields.d3dDevice7,0x1Bu,1u);
    set(fields.d3dDevice7,0x13u,srcBlend);
    return set(fields.d3dDevice7,0x14u,destBlend);
}

void GRAPH_CORE::DrawPrimitive(unsigned long type,unsigned long vertexShader,void* vertex,
                               unsigned int /*sizeOneVertex*/,int noVertex)
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    void** vtable=*reinterpret_cast<void***>(fields.d3dDevice7);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long,void*,unsigned int,unsigned long);
    const long hr=reinterpret_cast<Method>(vtable[0x64/4])(
        fields.d3dDevice7,type,vertexShader,vertex,static_cast<unsigned int>(noVertex),0u);
    if (hr)
        Error(10,"DrawPrimitive in DrawPrimitive",static_cast<unsigned long>(hr));
}

namespace {
struct TLVertexRetail {
    float x,y,z,rhw;
    unsigned int diffuse;
    unsigned int specular;
};
static_assert(sizeof(TLVertexRetail)==0x18,"retail TL vertex stride");
struct TLLineVertexRetail {
    float x,y,z,rhw;
    unsigned int diffuse;
};
static_assert(sizeof(TLLineVertexRetail)==0x14,"retail TL line vertex stride");

void ClearTexture0(GraphAbiFields& fields)
{
    void** vtable=*reinterpret_cast<void***>(fields.d3dDevice7);
    typedef long (__stdcall *Method)(void*,unsigned long,void*);
    reinterpret_cast<Method>(vtable[0x8C/4])(fields.d3dDevice7,0u,0);
}
}

void GRAPH::SetGamma(const GAMMA* gamma)
{
    GraphAbiFields& fields=Fields(this);
    if (fields.gamma.operator==(gamma))
        return;
    fields.gamma=gamma;
    for (int i=0;i<0x800;++i) {
        VID* vid=Map->Vid(i);
        if (vid!=EmptyVid)
            Map->Vid(i)->SetGamma(gamma,4);
    }
}

void GRAPH::DrawSquall()
{
    if (Map->IsPaused())
        return;

    GraphAbiFields& fields=Fields(this);
    if (fields.environment & 0x80u) {
        if (g_retailOldWindSpeed==-1.0f)
            g_retailOldWindSpeed=fields.windForce;

        const unsigned long elapsed=CurrentTime-g_retailSquallStart;
        if (elapsed<=0x800u) {
            fields.windForce=(static_cast<float>(elapsed)*g_retailOldWindSpeed/512.0f)+g_retailOldWindSpeed;
            return;
        }
        if (elapsed<=0x1000u) {
            fields.windForce=(static_cast<float>(0x1000u-elapsed)*g_retailOldWindSpeed/512.0f)+g_retailOldWindSpeed;
            return;
        }

        fields.environment &= ~0x80u;
        fields.windForce=g_retailOldWindSpeed;
        g_retailOldWindSpeed=-1.0f;
        return;
    }

    g_retailSquallStart=CurrentTime;
    if (g_retailOldWindSpeed!=-1.0f)
        fields.windForce=g_retailOldWindSpeed;
    g_retailOldWindSpeed=-1.0f;
}

int GRAPH::DrawPicture(const STRING* filename,float x,float y)
{
    PICTURE picture;
    if (picture.Load(filename))
        return 1;
    for (int px=0;px<picture.SizeX();++px) {
        for (int py=0;py<picture.SizeY();++py)
            PutPixel(x+static_cast<float>(px),y+static_cast<float>(py),picture.GetPixel(px,py));
    }
    return 0;
}

void GRAPH::ShadowLine(float x,float y,float x1,float y1,int shadow)
{
    TLLineVertexRetail v[2]={
        {x,y,0.99999988f,1.0f,static_cast<unsigned int>(shadow)},
        {x1,y1,0.99999988f,1.0f,static_cast<unsigned int>(shadow)}
    };
    UnLock();
    GraphAbiFields& fields=Fields(this);
    ClearTexture0(fields);
    SetAlphaBlend(1u,4u);
    SetRenderState(0x0Eu,0u);
    DrawPrimitive(2u,0x44u,v,sizeof(TLLineVertexRetail),2);
    SetRenderState(0x0Eu,1u);
}

void GRAPH::ShadowBar(float x,float y,float x1,float y1,int shadow)
{
    TLVertexRetail v[4]={
        {x,y,0.99999988f,1.0f,static_cast<unsigned int>(shadow),0xffffffffu},
        {x1,y,0.99999988f,1.0f,static_cast<unsigned int>(shadow),0xffffffffu},
        {x1,y1,0.99999988f,1.0f,static_cast<unsigned int>(shadow),0xffffffffu},
        {x,y1,0.99999988f,1.0f,static_cast<unsigned int>(shadow),0xffffffffu}
    };
    UnLock();
    GraphAbiFields& fields=Fields(this);
    ClearTexture0(fields);
    SetAlphaBlend(1u,4u);
    SetRenderState(0x0Eu,0u);
    DrawPrimitive(6u,0xC4u,v,sizeof(TLVertexRetail),4);
    SetRenderState(0x0Eu,1u);
}

void GRAPH::LightLine(float x,float y,float x1,float y1,unsigned int bright)
{
    TLLineVertexRetail v[2]={
        {x,y,0.99999988f,1.0f,bright},
        {x1,y1,0.99999988f,1.0f,bright}
    };
    UnLock();
    GraphAbiFields& fields=Fields(this);
    ClearTexture0(fields);
    SetAlphaBlend(9u,2u);
    SetRenderState(0x0Eu,0u);
    DrawPrimitive(2u,0x44u,v,sizeof(TLLineVertexRetail),2);
    SetRenderState(0x0Eu,1u);
}

void GRAPH::LightBar(float x,float y,float x1,float y1,unsigned int bright)
{
    TLVertexRetail v[4]={
        {x,y,0.99999988f,1.0f,bright,0xffffffffu},
        {x1,y,0.99999988f,1.0f,bright,0xffffffffu},
        {x1,y1,0.99999988f,1.0f,bright,0xffffffffu},
        {x,y1,0.99999988f,1.0f,bright,0xffffffffu}
    };
    UnLock();
    GraphAbiFields& fields=Fields(this);
    ClearTexture0(fields);
    SetRenderState(0x1Du,0u);
    SetAlphaBlend(9u,2u);
    SetRenderState(0x0Eu,0u);
    DrawPrimitive(6u,0xC4u,v,sizeof(TLVertexRetail),4);
    SetRenderState(0x0Eu,1u);
}

void GRAPH::Bar(float x,float y,float x1,float y1,COLOR color)
{
    const unsigned int packed=color.ARGB32();
    TLVertexRetail v[4]={
        {x,y,0.99999988f,1.0f,packed,0xffffffffu},
        {x1,y,0.99999988f,1.0f,packed,0xffffffffu},
        {x1,y1,0.99999988f,1.0f,packed,0xffffffffu},
        {x,y1,0.99999988f,1.0f,packed,0xffffffffu}
    };
    UnLock();
    GraphAbiFields& fields=Fields(this);
    ClearTexture0(fields);
    if (color.Alpha()==0xffu)
        SetRenderState(0x1Bu,0u);
    else
        SetAlphaBlend(5u,6u);
    SetRenderState(0x0Eu,0u);
    DrawPrimitive(6u,0xC4u,v,sizeof(TLVertexRetail),4);
    SetRenderState(0x0Eu,1u);
}

void GRAPH::Line(float x,float y,float x1,float y1,COLOR color)
{
    if (fabsf(x)>10000.0f) {
        x=0.0f;
        if (::Error) MYERROR::Error(::Error,"GRAPH",4,"x in Line",0);
    }
    if (fabsf(x1)>10000.0f) {
        x1=0.0f;
        if (::Error) MYERROR::Error(::Error,"GRAPH",4,"x1 in Line",0);
    }
    if (fabsf(y)>10000.0f) {
        y=0.0f;
        if (::Error) MYERROR::Error(::Error,"GRAPH",4,"y in Line",0);
    }
    if (fabsf(y1)>10000.0f) {
        y1=0.0f;
        if (::Error) MYERROR::Error(::Error,"GRAPH",4,"y1 in Line",0);
    }

    int ix=static_cast<int>(x);
    int iy=static_cast<int>(y);
    int stepY=-1;
    int steep=0;
    int stepX=(x1>x) ? 1 : -1;
    if (y1>y) stepY=1;
    unsigned int dx=static_cast<unsigned int>(static_cast<int>(x1-x) < 0 ? -static_cast<int>(x1-x) : static_cast<int>(x1-x));
    unsigned int dy=static_cast<unsigned int>(static_cast<int>(y1-y) < 0 ? -static_cast<int>(y1-y) : static_cast<int>(y1-y));
    if (dy>dx) {
        steep=1;
        int t=ix; ix=iy; iy=t;
        unsigned int ut=dx; dx=dy; dy=ut;
        t=stepX; stepX=stepY; stepY=t;
    }
    int d=static_cast<int>(2u*dy-dx);
    const int incr=static_cast<int>(2u*dy);
    for (unsigned int count=0;count<dx;++count) {
        if (steep) PutPixel(static_cast<float>(iy),static_cast<float>(ix),color);
        else PutPixel(static_cast<float>(ix),static_cast<float>(iy),color);
        while (d>=0) {
            iy+=stepY;
            d-=static_cast<int>(2u*dx);
        }
        ix+=stepX;
        d+=incr;
    }
    PutPixel(x1,y1,color);
}

void GRAPH_CORE::SetViewPort(float x0,float y0,float x1,float y1)
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    fields.viewXMin=x0;
    fields.viewXMax=x1;
    fields.viewYMin=y0;
    fields.viewYMax=y1;
    VID::SetViewPort(static_cast<int>(x0),static_cast<int>(y0),static_cast<int>(x1),static_cast<int>(y1));
    if (!fields.d3dDevice7)
        return;

    struct Viewport7Retail {
        uint32_t x,y,width,height;
        float minZ,maxZ;
    } viewport;
    viewport.x=static_cast<uint32_t>(static_cast<int>(x0));
    viewport.y=static_cast<uint32_t>(static_cast<int>(y0));
    viewport.width=static_cast<uint32_t>(static_cast<int>(x1-x0));
    viewport.height=static_cast<uint32_t>(static_cast<int>(y1-y0));
    viewport.minZ=0.0f;
    viewport.maxZ=1.0f;

    void** deviceVtable=*reinterpret_cast<void***>(fields.d3dDevice7);
    typedef long (__stdcall *SetViewportMethod)(void*,const Viewport7Retail*);
    long hr=reinterpret_cast<SetViewportMethod>(deviceVtable[0x34/4])(fields.d3dDevice7,&viewport);
    if (hr)
        Error(8,"viewport",static_cast<unsigned long>(hr));

    RECT_OLD rect={static_cast<long>(static_cast<int>(x0)),
                   static_cast<long>(static_cast<int>(y0)),
                   static_cast<long>(static_cast<int>(x1)),
                   static_cast<long>(static_cast<int>(y1))};
    struct RegionDataRetail {
        uint32_t headerSize;
        uint32_t type;
        uint32_t count;
        uint32_t regionSize;
        RECT_OLD bound;
        RECT_OLD dataRect;
    } region;
    region.headerSize=0x20u;
    region.type=1u;
    region.count=1u;
    region.regionSize=0u;
    region.bound=rect;
    region.dataRect=rect;

    void* clipper=0;
    void** ddVtable=*reinterpret_cast<void***>(fields.directDraw);
    typedef long (__stdcall *CreateClipperMethod)(void*,unsigned long,void**,void*);
    hr=reinterpret_cast<CreateClipperMethod>(ddVtable[0x10/4])(fields.directDraw,0u,&clipper,0);
    if (hr)
        Error(3,"clipper to zbuffer",static_cast<unsigned long>(hr));
    if (!clipper)
        return;

    void** clipVtable=*reinterpret_cast<void***>(clipper);
    typedef long (__stdcall *SetClipListMethod)(void*,void*,unsigned long);
    reinterpret_cast<SetClipListMethod>(clipVtable[0x1C/4])(clipper,&region,0u);

    typedef long (__stdcall *SetClipperMethod)(void*,void*);
    void** zVtable=*reinterpret_cast<void***>(fields.zSurface);
    reinterpret_cast<SetClipperMethod>(zVtable[0x70/4])(fields.zSurface,0);
    void** backVtable=*reinterpret_cast<void***>(fields.colorSurface);
    reinterpret_cast<SetClipperMethod>(backVtable[0x70/4])(fields.colorSurface,0);

    hr=reinterpret_cast<SetClipperMethod>(zVtable[0x70/4])(fields.zSurface,clipper);
    if (hr)
        Error(8,"clipper to zbuffer",static_cast<unsigned long>(hr));
    if (fields.stateFlags & 0x20u) {
        hr=reinterpret_cast<SetClipperMethod>(backVtable[0x70/4])(fields.colorSurface,clipper);
        if (hr)
            Error(8,"clipper to backbuffer",static_cast<unsigned long>(hr));
    }

    typedef unsigned long (__stdcall *ReleaseMethod)(void*);
    reinterpret_cast<ReleaseMethod>(clipVtable[0x08/4])(clipper);
}

void GRAPH_CORE::PutsXY(float x,float y,const char* str,COLOR color)
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    UnLock();

    void* dc=0;
    void** vtable=*reinterpret_cast<void***>(fields.colorSurface);
    typedef long (__stdcall *GetDCMethod)(void*,void**);
    if (reinterpret_cast<GetDCMethod>(vtable[17])(fields.colorSurface,&dc) < 0)
        return;

    SetTextColor(dc,color.ARGB32() & 0x00FFFFFFu);
    SetBkMode(dc,1);
    ExtTextOutA(dc,static_cast<int>(x),static_cast<int>(y),0,0,str,strlen(str),0);

    typedef long (__stdcall *ReleaseDCMethod)(void*,void*);
    reinterpret_cast<ReleaseDCMethod>(vtable[26])(fields.colorSurface,dc);
}

void __cdecl GRAPH::PrintfXY(float x,float y,const char* format,...)
{
    char buffer[1024];
    va_list args;
    va_start(args,format);
    vsprintf(buffer,format,args);
    va_end(args);
    PutsXY(x,y,buffer,GREEN);
}




int FormatBPP(D3DFORMAT format)
{
    switch (format) {
    case 20: return 24; // D3DFMT_R8G8B8
    case 21: // D3DFMT_A8R8G8B8
    case 22: return 32; // D3DFMT_X8R8G8B8
    case 23: // D3DFMT_R5G6B5
    case 24: // D3DFMT_X1R5G5B5
    case 25: // D3DFMT_A1R5G5B5
    case 26: return 16; // D3DFMT_A4R4G4B4
    case 41: return 8;  // D3DFMT_P8
    case 0x31545844: return 4; // DXT1
    case 0x33545844: // DXT3
    case 0x35545844: return 8; // DXT5
    default: return 0;
    }
}

int DD_DRIVER::GetMode(int size_x,int size_y,int bits_per_pixel)
{
    for (int i=0;i<noModes;++i)
        if (sizeX[i]==size_x && sizeY[i]==size_y && FormatBPP(modesPixel[i])==bits_per_pixel)
            return i;
    return -1;
}

STRING DD_DRIVER::GetModeDesctription(int mode)
{
    for (int i=noModes-1;i>=1;--i) {
        if (modesPixel[i] != modesPixel[0])
            return Printf("%i x %i x %ibpp",sizeX[mode],sizeY[mode],FormatBPP(modesPixel[mode]));
    }
    return Printf("%i x %i",sizeX[mode],sizeY[mode]);
}

void GRAPH::UpdateStartupDialog(DIALOG_COMBO_BOX* driversBox,DIALOG_COMBO_BOX* modes,DIALOG_BUTTON* fullscreen)
{
    GraphAbiFields& fields=Fields(this);

    if (driversBox->NoString()) {
        fields.currentDriver=driversBox->GetCurrentStringData();
    } else {
        if (fullscreen)
            fullscreen->SetCheck((fields.stateFlags >> 7) & 1u);
        driversBox->Reset();
        for (int i=0;i<fields.driverCount;++i) {
            STRING description(fields.drivers[i].description);
            const int index=driversBox->AddStringWithData(&description,i);
            if (i==fields.currentDriver)
                driversBox->SetCurrent(index);
        }
    }

    if (modes->NoString()) {
        const int data=modes->GetCurrentStringData();
        fields.sizeX=static_cast<float>(data & 0x7FFF);
        fields.sizeY=static_cast<float>(data >> 16);
        fields.stateFlags=(fields.stateFlags & ~2u) | ((data & 0x8000) ? 2u : 0u);
    }

    DD_DRIVER& driver=fields.drivers[fields.currentDriver];
    const int bpp=(fields.stateFlags & 2u) ? 32 : 16;
    if (driver.GetMode(static_cast<int>(fields.sizeX),static_cast<int>(fields.sizeY),bpp) < 0) {
        fields.sizeX=static_cast<float>(driver.sizeX[0]);
        fields.sizeY=static_cast<float>(driver.sizeY[0]);
        fields.stateFlags=(fields.stateFlags & ~2u) | (FormatBPP(driver.modesPixel[0])==32 ? 2u : 0u);
    }

    if (fullscreen) {
        int windowed=0;
        if ((driver.flags & 1u) &&
            (((fields.stateFlags >> 1) & 1u) == (FormatBPP(driver.desktopPixel)==32 ? 1u : 0u)) &&
            static_cast<float>(GetSystemMetrics(0)) >= fields.sizeX &&
            static_cast<float>(GetSystemMetrics(1)) >= fields.sizeY) {
            windowed=1;
        }

        if (!windowed) {
            fullscreen->SetCheck(1);
            fullscreen->Disable();
        } else {
            fullscreen->Enable(1);
        }
        fields.stateFlags=(fields.stateFlags & ~0x80u) | ((fullscreen->GetCheck() & 1u) << 7);
    }

    modes->Reset();
    for (int i=0;i<driver.noModes;++i) {
        int data=driver.sizeX[i] | (driver.sizeY[i] << 16);
        if (FormatBPP(driver.modesPixel[i])==32)
            data|=0x8000;
        STRING description=driver.GetModeDesctription(i);
        const int index=modes->AddStringWithData(&description,data);
        if (static_cast<float>(driver.sizeX[i])==fields.sizeX &&
            static_cast<float>(driver.sizeY[i])==fields.sizeY &&
            (((fields.stateFlags >> 1) & 1u) == (FormatBPP(driver.modesPixel[i])==32 ? 1u : 0u))) {
            modes->SetCurrent(index);
        }
    }
}

// This is the editor's complete effect-state owner.  In particular effect 5
// preserves the original DirectDraw surface capture path and effect 2 records
// the current map center used by MAP::SetShiftCoor; no newer-engine camera
// branch is present in this executable.
void GRAPH_CORE::Effect(int eff,int var1,int var2,int duration)
{
    GraphAbiFields& fields=Fields(reinterpret_cast<GRAPH*>(this));
    if (eff < 0 || eff >= 16)
        return;

    if (eff == 3 || eff == 9 || eff == 10) {
        fields.effectStart[10]=0;
        fields.effectStart[9]=0;
        fields.effectStart[3]=0;
    }

    if (duration == 0) {
        switch (eff) {
        case 1: duration=0x900; break;
        case 2: duration=0x200; break;
        case 3: duration=0x900; break;
        case 5: duration=0x400; break;
        case 9: duration=0x500; break;
        case 10: duration=0x400; break;
        default: break;
        }
    }

    fields.effectStart[eff]=RealCurrentTime;
    fields.effectDuration[eff]=duration;
    fields.effectVar1[eff]=var1;
    fields.effectVar2[eff]=var2;

    if (eff == 0) {
        for (int i=0;i<16;++i)
            fields.effectStart[i]=0;
        return;
    }

    if (eff == 5 && fields.effectSurface) {
        RECT_OLD windowRect;
        RECT_OLD rect;
        GetWindowRect(fields.hwnd,&windowRect);
        rect.left=windowRect.left;
        rect.top=windowRect.top;
        rect.right=windowRect.left+static_cast<int>(fields.sizeX);
        rect.bottom=windowRect.top+static_cast<int>(fields.sizeY);

        fields.effectStart[10]=0;
        fields.effectStart[9]=0;
        fields.effectStart[3]=0;

        if ((fields.stateFlags & 2u) == 0) {
            // IDirectDrawSurface7::BltFast(0,0,source,&rect,DDBLTFAST_WAIT).
            void** vtable=*reinterpret_cast<void***>(fields.effectSurface);
            typedef long (__stdcall *BltFastMethod)(void*,unsigned long,unsigned long,void*,RECT_OLD*,unsigned long);
            const long hr=reinterpret_cast<BltFastMethod>(vtable[0x1C/4])(
                fields.effectSurface,0,0,fields.captureSourceSurface,&rect,0x10u);
            if (hr)
                Error(1,"for eff_alphaappear",static_cast<unsigned long>(hr));
        } else {
            int destinationPitch=0;
            int sourcePitch=0;
            RGB16* destination=static_cast<RGB16*>(LockSurface(fields.effectSurface,&destinationPitch,0));
            COLOR* source=static_cast<COLOR*>(LockSurface(fields.captureSourceSurface,&sourcePitch,&rect));

            if (destination && source) {
                destinationPitch=destinationPitch/2-static_cast<int>(fields.sizeX);
                sourcePitch=sourcePitch/4-static_cast<int>(fields.sizeX);
                for (int y=0; static_cast<float>(y)<fields.sizeY; ++y) {
                    for (int x=0; static_cast<float>(x)<fields.sizeX; ++x) {
                        RGB16 converted(source);
                        destination->color=converted.color;
                        ++destination;
                        ++source;
                    }
                    destination+=destinationPitch;
                    source+=sourcePitch;
                }
            }

            typedef long (__stdcall *UnlockMethod)(void*,void*);
            void** srcVtable=*reinterpret_cast<void***>(fields.captureSourceSurface);
            reinterpret_cast<UnlockMethod>(srcVtable[0x80/4])(fields.captureSourceSurface,0);
            void** dstVtable=*reinterpret_cast<void***>(fields.effectSurface);
            reinterpret_cast<UnlockMethod>(dstVtable[0x80/4])(fields.effectSurface,0);
        }
        return;
    }

    if (eff == 2) {
        fields.effectShiftX=Map->FromScreenX(0.0f)+fields.sizeX/2.0f;
        fields.effectShiftY=Map->FromScreenY(0.0f)+fields.sizeY/2.0f;
        return;
    }

    if (eff == 11)
        fields.effectGamma.operator=(&fields.gamma);
}

void GRAPH::PutPixel(int x,int y,COLOR color)
{
    if (!InViewPort(static_cast<float>(x),static_cast<float>(y)))
        return;
    Lock();
    GraphAbiFields& fields=Fields(this);
    const int index=x+y*fields.colorPitch;
    if (fields.stateFlags & 2u)
        reinterpret_cast<COLOR*>(fields.colorBuffer)[index]=&color;
    else
        reinterpret_cast<RGB16*>(fields.colorBuffer)[index]=RGB16(&color);
}

void GRAPH::PutAlphaPixel(float x,float y,COLOR color,unsigned int alpha)
{
    alpha &= 0xFFu;
    if (!InViewPort(x,y) || alpha==0u)
        return;

    Lock();
    GraphAbiFields& fields=Fields(this);
    const int ix=static_cast<int>(x);
    const int iy=static_cast<int>(y);
    const int index=ix+iy*fields.colorPitch;
    if (fields.stateFlags & 2u) {
        reinterpret_cast<COLOR*>(fields.colorBuffer)[index].AlphaAdd(color,alpha);
    } else {
        RGB16* dst=reinterpret_cast<RGB16*>(fields.colorBuffer)+index;
        COLOR current(dst);
        COLOR blended=current.AlphaAdd(color,alpha);
        *dst=RGB16(&blended);
    }
}

void GRAPH::Circle(float xx,float yy,float radius,COLOR color)
{
    const int x=static_cast<int>(xx);
    const int y=static_cast<int>(yy);
    int cx=0;
    int cy=static_cast<int>(radius);
    int df=1-static_cast<int>(radius);
    int d_e=3;
    int d_se=5-2*static_cast<int>(radius);

    do {
        PutPixel(x+cx,y+cy,color);
        if (cx!=0)
            PutPixel(x-cx,y+cy,color);
        if (cy!=0)
            PutPixel(x+cx,y-cy,color);
        if (cx!=0 && cy!=0)
            PutPixel(x-cx,y-cy,color);

        if (cx!=cy) {
            PutPixel(x+cy,y+cx,color);
            if (cx!=0)
                PutPixel(x+cy,y-cx,color);
            if (cy!=0)
                PutPixel(x-cy,y+cx,color);
            if (cx!=0 && cy!=0)
                PutPixel(x-cy,y-cx,color);
        }

        if (df<0) {
            df+=d_e;
            d_e+=2;
            d_se+=2;
        } else {
            df+=d_se;
            d_e+=2;
            d_se+=4;
            --cy;
        }
        ++cx;
    } while (cx<=cy);
}

void GRAPH::WuLine(float x0,float y0,float x1,float y1,COLOR color)
{
    if (y0>y1) {
        float t=y0; y0=y1; y1=t;
        t=x0; x0=x1; x1=t;
    }

    PutPixel(x0,y0,color);

    int deltaX=static_cast<int>(x1-x0);
    int xDir;
    if (deltaX>=0) {
        xDir=1;
    } else {
        xDir=-1;
        deltaX=-deltaX;
    }
    int deltaY=static_cast<int>(y1-y0);

    if (deltaY==0 || deltaX==0) {
        Line(x0,y0,x1,y1,color);
        return;
    }

    if (deltaX==deltaY) {
        do {
            x0+=static_cast<float>(xDir);
            y0+=1.0f;
            PutPixel(x0,y0,color);
            --deltaY;
        } while (deltaY!=0);
        return;
    }

    unsigned short errAcc=0;
    unsigned short errAdj;
    if (deltaY>deltaX) {
        errAdj=static_cast<unsigned short>((deltaX<<16)/deltaY);
        --deltaY;
        if (deltaY!=0) {
            do {
                const unsigned short old=errAcc;
                errAcc=static_cast<unsigned short>(errAcc+errAdj);
                if (static_cast<unsigned int>(errAcc)<=static_cast<unsigned int>(old))
                    x0+=static_cast<float>(xDir);
                y0+=1.0f;
                const unsigned int weight=(static_cast<unsigned int>(errAcc)&0xFFFFu)>>8;
                PutAlphaPixel(x0,y0,color,0xFFu-weight);
                PutAlphaPixel(x0+static_cast<float>(xDir),y0,color,weight);
                --deltaY;
            } while (deltaY!=0);
        }
    } else {
        errAdj=static_cast<unsigned short>((deltaY<<16)/deltaX);
        --deltaX;
        if (deltaX!=0) {
            do {
                const unsigned short old=errAcc;
                errAcc=static_cast<unsigned short>(errAcc+errAdj);
                if (static_cast<unsigned int>(errAcc)<=static_cast<unsigned int>(old))
                    y0+=1.0f;
                x0+=static_cast<float>(xDir);
                const unsigned int weight=(static_cast<unsigned int>(errAcc)&0xFFFFu)>>8;
                PutAlphaPixel(x0,y0,color,0xFFu-weight);
                PutAlphaPixel(x0,y0+1.0f,color,weight);
                --deltaX;
            } while (deltaX!=0);
        }
    }

    PutPixel(x1,y1,color);
}

void GRAPH_CORE::PutcXY(float x,float y,char c,COLOR color)
{
    char text[2];
    text[0]=c;
    text[1]=0;
    PutsXY(x,y,text,color);
}



MOVIE::MOVIE()
    : pGraph(0), pMediaControl(0), pEvent(0), pVidWin(0)
{
}

void MOVIE::Error(TYPE_ERROR type,char* text,unsigned long err)
{
    if (::Error)
        MYERROR::Error(::Error,"MOVIE",static_cast<int>(type),text,err);
}

// center_x/center_y are present in the retail signature but are not read by
// the original function; cursor placement is always the graph centre.
void MOVIE::Open(const STRING* file,int center_x,int center_y)
{
    (void)center_x;
    (void)center_y;

    Release();
    Graph->FlipToGDISurface();

    long hr=CoCreateInstance(kMovieClsidFilterGraph,0,1,kMovieIidGraphBuilder,&pGraph);
    if (hr<0) {
        Error(E_CREATE,const_cast<char*>("GraphBuilder"),static_cast<unsigned long>(hr));
        return;
    }

    hr=MovieQueryInterface(pGraph,kMovieIidMediaControl,&pMediaControl);
    if (hr<0) {
        Error(E_CREATE,const_cast<char*>("MediaControl"),static_cast<unsigned long>(hr));
        Release();
        return;
    }

    MovieQueryInterface(pGraph,kMovieIidMediaEvent,&pEvent);

    unsigned short wideName[0x400];
    const_cast<STRING*>(file)->ToWideChar(wideName,0x400);
    hr=MovieRenderFile(pGraph,wideName);
    if (hr<0) {
        Error(E_INVALID,const_cast<char*>("RenderFile"),static_cast<unsigned long>(hr));
        Release();
        return;
    }

    MovieQueryInterface(pGraph,kMovieIidVideoWindow,&pVidWin);

    {
        void** vtable=*reinterpret_cast<void***>(pVidWin);
        typedef long (__stdcall *OwnerMethod)(void*,long);
        reinterpret_cast<OwnerMethod>(vtable[0x74/4])(pVidWin,reinterpret_cast<long>(Map->m_hWnd));
        reinterpret_cast<OwnerMethod>(vtable[0x24/4])(pVidWin,0x44000000L);
    }

    Graph->FlipToGDISurface();

    const int width=static_cast<int>(Graph->ViewXMax())-static_cast<int>(Graph->ViewXMin())+1;
    const int height=static_cast<int>(Graph->ViewYMax())-static_cast<int>(Graph->ViewYMin())+1;
    const int left=static_cast<int>(Graph->ViewXMin());
    const int top=static_cast<int>(Graph->ViewYMin());
    {
        void** vtable=*reinterpret_cast<void***>(pVidWin);
        typedef long (__stdcall *PositionMethod)(void*,long,long,long,long);
        reinterpret_cast<PositionMethod>(vtable[0x9C/4])(pVidWin,left,top,width,height);
    }

    {
        void** vtable=*reinterpret_cast<void***>(pMediaControl);
        typedef long (__stdcall *RunMethod)(void*);
        reinterpret_cast<RunMethod>(vtable[0x1C/4])(pMediaControl);
    }

    SetCapture(Map->m_hWnd);
    SetCursorPos(static_cast<int>(Graph->SizeX()),static_cast<int>(Graph->SizeY()));
}

// Retail releases mouse capture first, then releases the four DirectShow interfaces.
void MOVIE::Release()
{
    ReleaseCapture();
    void** slots[4] = { &pVidWin, &pMediaControl, &pEvent, &pGraph };
    for (int i=0;i<4;++i) {
        void*& object=*slots[i];
        if (object) {
            void** vtable=*reinterpret_cast<void***>(object);
            typedef unsigned long (__stdcall *ReleaseMethod)(void*);
            reinterpret_cast<ReleaseMethod>(vtable[2])(object);
        }
        object=0;
    }
}

void GRAPH::StopMovie()
{
    Fields(this).movie.Release();
}

void GRAPH::SetWind(int speed,ANGLE direction)
{
    GraphAbiFields& fields=Fields(this);
    fields.windForce=static_cast<float>(speed)/1000.0f;
    fields.windDirection=direction;
}

void GRAPH::SetEnvironment(unsigned int env)
{
    GraphAbiFields& fields=Fields(this);
    if ((env & 0x80000000u)==0u) {
        if (env==1u || env==2u)
            fields.environment&=~3u;
        if (env & 0x0C00u)
            fields.environment&=~0x0C00u;
        if (env & 0xC000u)
            fields.environment&=~0xC000u;
        fields.environment|=env;
    } else {
        fields.environment&=~env;
    }
}

int GAMMA::Write(STREAM* res)
{
    return res->Write(this,8u);
}

void ANGLE::Write(STREAM* res)
{
    const int direction=value;
    res->Write(&direction,4u);
}

void GRAPH::SaveParameters(STREAM* res)
{
    GraphAbiFields& f=Fields(this);
    res->Write(&f.environment,4u);
    f.gamma.Write(res);
    f.windDirection.Write(res);
    res->Write(&f.windForce,4u);
}

namespace { int g_surfaceMemoryInUse = 0; } // original 0x004CE208

int SURFACE::MemoryInUse()
{
    return g_surfaceMemoryInUse;
}

namespace {
unsigned long ReleaseComObject(void*& object)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef unsigned long (__stdcall *ReleaseMethod)(void*);
    const unsigned long result=reinterpret_cast<ReleaseMethod>(vtable[0x08/4])(object);
    object=0;
    return result;
}

void DeleteVirtualGraphObject(void*& object)
{
    if (!object)
        return;
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void* (__thiscall *DeletingDestructor)(void*,unsigned int);
    reinterpret_cast<DeletingDestructor>(vtable[0])(object,1u);
    object=0;
}
}

void GRAPH_CORE::SetDefaultState()
{
    // Intentionally empty in the retail base class.
}

void GRAPH::SetDefaultState()
{
    GraphAbiFields& f=Fields(this);
    DeviceSetTextureStageState(f.d3dDevice7,0,2,2);
    DeviceSetTextureStageState(f.d3dDevice7,0,3,0);
    DeviceSetTextureStageState(f.d3dDevice7,0,1,4);
    DeviceSetTextureStageState(f.d3dDevice7,0,5,2);
    DeviceSetTextureStageState(f.d3dDevice7,0,6,0);
    DeviceSetTextureStageState(f.d3dDevice7,0,4,4);
    DeviceSetRenderState(f.d3dDevice7,0x11,1);
    DeviceSetRenderState(f.d3dDevice7,0x10,1);
    DeviceSetRenderState(f.d3dDevice7,0x1D,0);
    DeviceSetRenderState(f.d3dDevice7,0x1B,0);
    SetRenderState(0x1A,1);
    SetRenderState(0x8E,0);
    SetRenderState(0x89,0);
    SetRenderState(0x0F,0);
    DeviceSetRenderState(f.d3dDevice7,0x17,8);
    DeviceSetRenderState(f.d3dDevice7,0x0E,0);
    SetRenderState(7,1);
    SetRenderState(0x17,7);
}

MOVIE::~MOVIE()
{
    Release();
}

GRAPH_CORE::~GRAPH_CORE()
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));

    f.movie.Release();
    UnLock();
    UnLockZ();

    if (f.d3dDevice7) {
        void** const vtable=*reinterpret_cast<void***>(f.d3dDevice7);
        typedef long (__stdcall *SetTextureMethod)(void*,unsigned long,void*);
        reinterpret_cast<SetTextureMethod>(vtable[0x8C/4])(f.d3dDevice7,0,0);
    }

    if (f.effectSurface)
        ReleaseComObject(f.effectSurface);

    DeleteVirtualGraphObject(reinterpret_cast<void*&>(f.surfaceBE8));
    DeleteVirtualGraphObject(reinterpret_cast<void*&>(f.paletteSurface));
    DeleteVirtualGraphObject(reinterpret_cast<void*&>(f.surfaceBE4));

    if (f.d3dDevice7) {
        const unsigned long result=ReleaseComObject(f.d3dDevice7);
        MYERROR::Log(::Error,"d3dDevice release %i",result);
    }
    if (f.realZSurface) {
        const unsigned long result=ReleaseComObject(f.realZSurface);
        MYERROR::LogTmp(::Error,"ZBufferHard release %i",result);
    }
    if (f.colorSurface) {
        const unsigned long result=ReleaseComObject(f.colorSurface);
        MYERROR::LogTmp(::Error,"backBuffer release %i",result);
    }
    if (f.zSurface) {
        const unsigned long result=ReleaseComObject(f.zSurface);
        MYERROR::LogTmp(::Error,"ZBuffer release %X",result);
    }
    if (f.captureSourceSurface) {
        const unsigned long result=ReleaseComObject(f.captureSourceSurface);
        MYERROR::LogTmp(::Error,"frontBuffer release %i",result);
    }
    if (f.direct3D7) {
        const unsigned long result=ReleaseComObject(f.direct3D7);
        MYERROR::Log(::Error,"d3d release %i",result);
    }
    if (f.directDraw) {
        const unsigned long result=ReleaseComObject(f.directDraw);
        MYERROR::Log(::Error,"DDraw  release %i",result);
    }

    f.movie.~MOVIE();
    reinterpret_cast<STRING*>(reinterpret_cast<uint8_t*>(this)+0xBC0)->~STRING();
}


GRAPH_CORE::GRAPH_CORE(const GRAPH_INIT* init)
{
    (void)opaque;
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));

    // RGB16[256] default construction at +0x0C is a retail no-op.
    // DD_DRIVER[8] is non-trivial and must execute its exact ctor on the
    // opaque debug metadata storage.
    for (int i=0;i<8;++i)
        new (&f.drivers[i]) DD_DRIVER();

    new (&f.effectGamma) GAMMA();
    new (&f.gamma) GAMMA();
    new (&f.windDirection) ANGLE();
    new (reinterpret_cast<unsigned char*>(this)+0xBC0) STRING();
    new (&f.movie) MOVIE();

    if (init)
        memcpy(&GraphInit,init,sizeof(GraphInit));

    // Retail graph_dx7.cpp @ 0x00401E33 explicitly zeroes the pixel-shader
    // field before reading GRAPH_INIT options.  This is observable in the
    // startup caps log and must not depend on allocator contents.
    f.pixelShader=0;

    // The original constructor dereferences init after the conditional copy;
    // MAP always passes a valid GRAPH_INIT.
    const unsigned long options=init->options;
    f.stateFlags=(f.stateFlags&~0x400u)|((options&1u)?0x400u:0u);
    f.stateFlags=(f.stateFlags&~0x100u)|((options&2u)?0x100u:0u);
    f.stateFlags&=~1u;

    f.direct3D7=0;
    f.d3dDevice7=0;
    f.directDraw=0;
    f.paletteSurface=0;
    f.surfaceBE4=0;
    f.surfaceBE8=0;
    f.effectSurface=0;
    f.colorSurface=0;
    f.captureSourceSurface=0;
    f.realZSurface=0;
    f.zSurface=0;
    f.zBuffer=0;
    f.colorBuffer=0;
    f.environment=0;
    f.windDirection.value=0xDCu;
    f.windForce=0.02f;
    *reinterpret_cast<unsigned long*>(reinterpret_cast<unsigned char*>(this)+0xBF0)=0;
    *reinterpret_cast<unsigned long*>(reinterpret_cast<unsigned char*>(this)+0xBEC)=0;

    ResetEnumeratedPixelFormatsRetail();

    Effect(0,0,0,0);

    g_driverEnumeratedCount=0;
    DirectDrawEnumerateExA(reinterpret_cast<void*>(&DirectDrawEnumCallback),
                           f.drivers,7u);
    f.driverCount=g_driverEnumeratedCount;
    f.currentDriver=static_cast<int>(init->defaultDevice);
    f.sizeX=static_cast<float>(static_cast<unsigned int>(init->defaultScreenX));
    f.sizeY=static_cast<float>(static_cast<unsigned int>(init->defaultScreenY));
    f.stateFlags=(f.stateFlags&~2u)|(init->defaultColorDepth==32 ? 2u : 0u);
    f.stateFlags=(f.stateFlags&~0x80u)|(init->defaultFullScreen ? 0x80u : 0u);
    if (f.currentDriver>=f.driverCount)
        f.currentDriver=0;
}


namespace {
// Retail GUIDs read byte-for-byte from MapEdit.exe .rdata.
const unsigned char kIidDirectDraw[16] = {
    0x80,0xDB,0x14,0x6C,0x33,0xA7,0xCE,0x11,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60
};
const unsigned char kIidDirect3D7[16] = {
    0x77,0x9E,0x04,0xF5,0x61,0x48,0xD2,0x11,0xA4,0x07,0x00,0xA0,0xC9,0x06,0x29,0xA8
};
const unsigned char kIidD3DHalDevice[16] = {
    0xE0,0x3D,0xE6,0x84,0xAA,0x46,0xCF,0x11,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E
};
const unsigned char kIidD3DRgbDevice[16] = {
    0x60,0x5C,0x66,0xA4,0x73,0x26,0xCF,0x11,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56
};

void* g_retailDirectDrawLegacy=0; // original BSS owner 0x004F06D4

long ComQueryInterface(void* object,const void* iid,void** out)
{
    void** vt=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *Method)(void*,const void*,void**);
    return reinterpret_cast<Method>(vt[0])(object,iid,out);
}
unsigned long ComAddRef(void* object)
{
    void** vt=*reinterpret_cast<void***>(object);
    typedef unsigned long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vt[1])(object);
}
long DDrawSetCooperativeLevel(void* dd,HWND__* hwnd,unsigned long flags)
{
    void** vt=*reinterpret_cast<void***>(dd);
    typedef long (__stdcall *Method)(void*,HWND__*,unsigned long);
    return reinterpret_cast<Method>(vt[0x50/4])(dd,hwnd,flags);
}
long DDrawSetDisplayMode(void* dd,unsigned long w,unsigned long h,unsigned long bpp,unsigned long refresh,unsigned long flags)
{
    void** vt=*reinterpret_cast<void***>(dd);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long,unsigned long,unsigned long,unsigned long);
    return reinterpret_cast<Method>(vt[0x54/4])(dd,w,h,bpp,refresh,flags);
}
long DDrawCreateClipper(void* dd,void** clipper)
{
    void** vt=*reinterpret_cast<void***>(dd);
    typedef long (__stdcall *Method)(void*,unsigned long,void**,void*);
    return reinterpret_cast<Method>(vt[0x10/4])(dd,0,clipper,0);
}
long ClipperSetHWnd(void* clipper,HWND__* hwnd)
{
    void** vt=*reinterpret_cast<void***>(clipper);
    typedef long (__stdcall *Method)(void*,unsigned long,HWND__*);
    return reinterpret_cast<Method>(vt[0x20/4])(clipper,0,hwnd);
}
long SurfaceSetClipper(void* surface,void* clipper)
{
    void** vt=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *Method)(void*,void*);
    return reinterpret_cast<Method>(vt[0x70/4])(surface,clipper);
}
long SurfaceGetAttached(void* surface,unsigned long caps,void** out)
{
    struct Caps2 { unsigned long caps,caps2,caps3,caps4; } c={caps,0,0,0};
    void** vt=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *Method)(void*,Caps2*,void**);
    return reinterpret_cast<Method>(vt[0x30/4])(surface,&c,out);
}
long SurfaceGetDesc(void* surface,void* desc)
{
    void** vt=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *Method)(void*,void*);
    return reinterpret_cast<Method>(vt[0x58/4])(surface,desc);
}
long SurfaceAddAttached(void* surface,void* attached)
{
    void** vt=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *Method)(void*,void*);
    return reinterpret_cast<Method>(vt[0x0C/4])(surface,attached);
}
long SurfaceUnlockRaw(void* surface,void* ptr)
{
    void** vt=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *Method)(void*,void*);
    return reinterpret_cast<Method>(vt[0x80/4])(surface,ptr);
}
long SurfaceBlt(void* surface,RECT_OLD* dst,void* source,RECT_OLD* src,unsigned long flags)
{
    void** vt=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *Method)(void*,RECT_OLD*,void*,RECT_OLD*,unsigned long,void*);
    return reinterpret_cast<Method>(vt[0x14/4])(surface,dst,source,src,flags,0);
}
long SurfaceFlip(void* surface)
{
    void** vt=*reinterpret_cast<void***>(surface);
    typedef long (__stdcall *Method)(void*,void*,unsigned long);
    return reinterpret_cast<Method>(vt[0x2C/4])(surface,0,1);
}
long D3DEnumZFormats(void* d3d,const void* deviceGuid,void* callback,void* context)
{
    void** vt=*reinterpret_cast<void***>(d3d);
    typedef long (__stdcall *Method)(void*,const void*,void*,void*);
    return reinterpret_cast<Method>(vt[0x18/4])(d3d,deviceGuid,callback,context);
}
long D3DCreateDevice(void* d3d,const void* guid,void* target,void** device)
{
    void** vt=*reinterpret_cast<void***>(d3d);
    typedef long (__stdcall *Method)(void*,const void*,void*,void**);
    return reinterpret_cast<Method>(vt[0x10/4])(d3d,guid,target,device);
}
long DeviceEnumTextureFormats(void* device,void* callback,void* context)
{
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*,void*,void*);
    return reinterpret_cast<Method>(vt[0x10/4])(device,callback,context);
}
long DeviceGetCaps(void* device,void* caps)
{
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*,void*);
    return reinterpret_cast<Method>(vt[0x0C/4])(device,caps);
}

long DeviceGetTextureStageState(void* device,unsigned long stage,unsigned long state,unsigned long* value)
{
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long,unsigned long*);
    return reinterpret_cast<Method>(vt[0x90/4])(device,stage,state,value);
}

extern DDPIXELFORMAT_OLD g_pf565;
extern DDPIXELFORMAT_OLD g_pf555;
extern DDPIXELFORMAT_OLD g_pf4444;
extern DDPIXELFORMAT_OLD g_pfPal8;
extern DDPIXELFORMAT_OLD g_pfZ16;

int __stdcall TextureSearchCallback(DDPIXELFORMAT_OLD* pf,void*)
{
    char* const text=const_cast<char*>(GetPixelFormat(pf));
    if (pf->dwRGBBitCount==8u && (pf->dwFlags&0x20u)) {
        memcpy(&g_pfPal8,pf,sizeof(g_pfPal8));
        // graph_dx7.cpp retail 0x00401C6D: strcat(text," selected").
        strcat(text," selected");
    } else if (pf->dwRGBAlphaBitMask==0xF000u) {
        memcpy(&g_pf4444,pf,sizeof(g_pf4444));
        // graph_dx7.cpp retail 0x00401C9B: same selected marker.
        strcat(text," selected");
    } else if (pf->dwRGBBitCount==16u && (pf->dwFlags&0x40u) && !(pf->dwFlags&3u)) {
        // Retail TextureSearchCallback @ 0x00401CD1 reads DDPIXELFORMAT +0x14,
        // i.e. dwGBitMask.  0x07E0 identifies RGB565.
        if (pf->dwGBitMask==0x7E0u) memcpy(&g_pf565,pf,sizeof(g_pf565));
        else memcpy(&g_pf555,pf,sizeof(g_pf555));
    }
    if (::Error) MYERROR::Log(::Error,"Enum texture format %s",text);
    return 1;
}
int __stdcall ZBufferSearchCallback(DDPIXELFORMAT_OLD* pf,void*)
{
    char* const text=const_cast<char*>(GetPixelFormat(pf));
    if (pf->dwFlags==0x400u && pf->dwRGBBitCount==16u) {
        memcpy(&g_pfZ16,pf,sizeof(g_pfZ16));
        // graph_dx7.cpp retail 0x00401D5B: strcat(text," selected").
        strcat(text," selected");
    }
    if (::Error) MYERROR::Log(::Error,"Enum zbuffer format %s",text);
    return 1;
}
}

int GRAPH_CORE::Init(HWND__* hwnd)
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    const DD_DRIVER& drv=f.drivers[f.currentDriver];
    long err=DirectDrawCreateEx(drv.pDeviceGUID,&f.directDraw,kIidDirectDraw7,0);
    if (err) { Error(3,"DDraw device",static_cast<unsigned long>(err)); return 1; }

    err=ComQueryInterface(f.directDraw,kIidDirectDraw,&g_retailDirectDrawLegacy);
    if (err<0) Error(10,"Can't query interface",0);

    const int fullscreen=((f.stateFlags>>7)&1u)!=0;
    err=DDrawSetCooperativeLevel(f.directDraw,hwnd,fullscreen?0x13u:0x08u);
    if (err) {
        Error(8,"cooperative level",static_cast<unsigned long>(err));
        ReleaseComObject(f.directDraw); f.directDraw=0; return 1;
    }

    DDPIXELFORMAT_OLD emptyPf={0};
    unsigned char descPrimary[0x7C];
    memset(descPrimary,0,sizeof(descPrimary));
    *reinterpret_cast<unsigned long*>(descPrimary)=0x7Cu;
    if (fullscreen) {
        const unsigned long bpp=(f.stateFlags&2u)?32u:16u;
        err=DDrawSetDisplayMode(f.directDraw,static_cast<unsigned long>(f.sizeX),static_cast<unsigned long>(f.sizeY),bpp,0,1);
        if (err) { Error(8,"display mode",static_cast<unsigned long>(err)); return 1; }
        f.captureSourceSurface=CreateSurface(static_cast<int>(f.sizeX),static_cast<int>(f.sizeY),S_PRIMARY,&emptyPf);
        if (!f.captureSourceSurface) return 1;
        err=SurfaceGetAttached(f.captureSourceSurface,4u,&f.colorSurface);
        if (err) { Error(9,"attached surface",static_cast<unsigned long>(err)); return 1; }
    } else {
        f.captureSourceSurface=CreateSurface(static_cast<int>(f.sizeX),static_cast<int>(f.sizeY),S_PRIMARY,&emptyPf);
        if (!f.captureSourceSurface) return 1;
        void* clipper=0;
        err=DDrawCreateClipper(f.directDraw,&clipper);
        if (err<0) { Error(3,"clipper",static_cast<unsigned long>(err)); return 1; }
        ClipperSetHWnd(clipper,hwnd);
        SurfaceSetClipper(f.captureSourceSurface,clipper);
        ReleaseComObject(clipper);
        SurfaceGetDesc(f.captureSourceSurface,descPrimary);
        f.colorSurface=CreateSurface(static_cast<int>(f.sizeX),static_cast<int>(f.sizeY),S_BACKBUFFER,
            reinterpret_cast<DDPIXELFORMAT_OLD*>(descPrimary+0x48));
        if (!f.colorSurface) return 1;
    }

    unsigned char backDesc[0x7C];
    memset(backDesc,0,sizeof(backDesc));
    *reinterpret_cast<unsigned long*>(backDesc)=0x7Cu;
    SurfaceGetDesc(f.colorSurface,backDesc);
    const unsigned long backCaps=*reinterpret_cast<unsigned long*>(backDesc+0x68);
    if (!(backCaps&0x4000u)) Error(10,"back buffer in system memory",backCaps);
    if (::Error) {
        DDPIXELFORMAT_OLD* const pf=reinterpret_cast<DDPIXELFORMAT_OLD*>(backDesc+0x48);
        MYERROR::Log(::Error,"Selected display mode %ix%i %s desktop %s",
                     static_cast<int>(f.sizeX),static_cast<int>(f.sizeY),GetPixelFormat(ConvertPixelFormat(pf)),
                     GetPixelFormat(drv.desktopPixel));
    }

    err=ComQueryInterface(f.directDraw,kIidDirect3D7,&f.direct3D7);
    if (err) { Error(3,"d3d",static_cast<unsigned long>(err)); return 1; }

    D3DEnumZFormats(f.direct3D7,(f.stateFlags&0x20u)?kIidD3DRgbDevice:kIidD3DHalDevice,
                    reinterpret_cast<void*>(&ZBufferSearchCallback),this);
    if (!g_pfZ16.dwSize) { Error(9,"enum zbuffer",0); return 1; }

    f.realZSurface=CreateSurface(static_cast<int>(f.sizeX),static_cast<int>(f.sizeY),S_ZBUFFER,&g_pfZ16);
    if (!f.realZSurface) { Error(3,"zBufferHard",0); return 1; }
    err=SurfaceAddAttached(f.colorSurface,f.realZSurface);
    if (err<0) { Error(8,"attach zbuffer",static_cast<unsigned long>(err)); return 1; }

    if (f.stateFlags&0x20u) {
        ComAddRef(f.realZSurface);
        f.zSurface=f.realZSurface;
    } else {
        f.zSurface=CreateSurface(static_cast<int>(f.sizeX),static_cast<int>(f.sizeY),S_ZBUFFER_SOFT,&g_pfZ16);
        if (!f.zSurface) return 1;
    }

    err=D3DCreateDevice(f.direct3D7,(f.stateFlags&0x20u)?kIidD3DRgbDevice:kIidD3DHalDevice,f.colorSurface,&f.d3dDevice7);
    if (err) Error(3,"3d Device",static_cast<unsigned long>(err));

    DeviceEnumTextureFormats(f.d3dDevice7,reinterpret_cast<void*>(&TextureSearchCallback),this);
    unsigned char caps[0xEC];
    memset(caps,0,sizeof(caps));
    err=DeviceGetCaps(f.d3dDevice7,caps);
    if (err) { Error(9,"Caps",static_cast<unsigned long>(err)); return 1; }

    const unsigned long devCaps=*reinterpret_cast<unsigned long*>(caps+0x00);
    const unsigned long texCaps=*reinterpret_cast<unsigned long*>(caps+0xB4);
    const unsigned long rasterCaps=*reinterpret_cast<unsigned long*>(caps+0x5C);
    f.stateFlags=(f.stateFlags&~0x08u)|((devCaps&0x1000u)?0x08u:0u);
    f.stateFlags=(f.stateFlags&~0x800u)|((texCaps&0x800000u)?0x800u:0u);
    f.stateFlags=(f.stateFlags&~0x1000u)|((texCaps&0x10u)?0x1000u:0u);
    f.stateFlags=(f.stateFlags&~0x04u)|((rasterCaps&0x80u)?0x04u:0u);

    const DDPIXELFORMAT_OLD* const effectPf=g_pf565.dwSize?&g_pf565:&g_pf555;
    f.effectSurface=CreateSurface(static_cast<int>(f.sizeX),static_cast<int>(f.sizeY),S_OFFSCREEN,effectPf);
    return f.effectSurface?0:1;
}

void GRAPH_CORE::CheckDriverBugs()
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    f.stateFlags|=0x2000u;
    if (f.stateFlags&0x20u) return;
    LockZ();
    const int x=static_cast<int>(f.viewXMin);
    const int y=static_cast<int>(f.viewYMin);
    unsigned short* p=f.zBuffer+x+y*f.zPitch;
    *reinterpret_cast<unsigned long*>(p)=0x1234u;
    UnLockZ();
    if (BlitZBuffer()) f.stateFlags&=~0x2000u;
    int pitch=0;
    unsigned char* hard=static_cast<unsigned char*>(LockSurface(f.realZSurface,&pitch,0));
    if (hard) {
        unsigned char* at=hard+2*x+y*pitch;
        if (*reinterpret_cast<unsigned long*>(at)!=0x1234u) f.stateFlags&=~0x2000u;
        SurfaceUnlockRaw(f.realZSurface,0);
    } else {
        Error(0,"ZBufferHard",0);
    }
}

void GRAPH_CORE::RefreshBuffers()
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    RECT_OLD wrect={0,0,0,0};
    ::GetWindowRect(f.hwnd,&wrect);
    RECT_OLD src={static_cast<long>(f.viewXMin),static_cast<long>(f.viewYMin),
                  static_cast<long>(f.viewXMax),static_cast<long>(f.viewYMax)};
    RECT_OLD dst={src.left+wrect.left,src.top+wrect.top,src.right+wrect.left,src.bottom+wrect.top};
    UnLock();
    SurfaceBlt(f.captureSourceSurface,&dst,f.colorSurface,&src,0x01000000u);
    if (f.stateFlags&0x40u) {
        SurfaceFlip(f.captureSourceSurface);
        SurfaceBlt(f.colorSurface,&src,f.captureSourceSurface,&dst,0x01000000u);
    }
}

char* GetTextureState();

GRAPH::GRAPH(const GRAPH_INIT* init)
    : GRAPH_CORE(init)
{
    // debugText is a real member at +0xC18 and is constructed automatically;
    // the compiler writes GRAPH's one-slot vptr after the base constructor.
}

int GRAPH::Init(HWND__* hwnd)
{
    GraphAbiFields& f=Fields(this);
    DD_DRIVER& drv=f.drivers[f.currentDriver];

    Registry->SetInt(STRING("ScreenX"),static_cast<int>(SizeX()));
    Registry->SetInt(STRING("ScreenY"),static_cast<int>(SizeY()));
    Registry->SetInt(STRING("BPP"),BytesPerPixel()*8);
    Registry->SetInt(STRING("Device"),f.currentDriver);
    Registry->SetInt(STRING("FullScreen"),(f.stateFlags&0x80u)?1:0);

    const int lowDetail=Registry->GetInt(STRING("LowDetail"),0);
    f.stateFlags=(f.stateFlags&~0x200u)|((lowDetail&1)?0x200u:0u);

    // Retail explicitly disables the software/reference path here.
    f.stateFlags&=~0x20u;

    f.stateFlags=(f.stateFlags&~0x10u)|((drv.flags&2u)?0x10u:0u);
    if (drv.flags&2u) {
        if (::Error) MYERROR::Log(::Error,"zm_nongdi");
    }
    // The old renderer immediately clears NONGDI again before startup.
    f.stateFlags&=~0x10u;

    const int triple=Registry->GetInt(STRING("TripleBuffer"),1);
    const int enableTriple=(triple && (f.stateFlags&0x80u) && !(f.stateFlags&0x400u))?1:0;
    f.stateFlags=(f.stateFlags&~0x40u)|(enableTriple?0x40u:0u);

    // Adapter bit0 means windowed operation is supported.
    if (!(drv.flags&1u)) f.stateFlags|=0x80u;

    if (!(f.stateFlags&0x80u)) {
        const int desktopX=GetSystemMetrics(0);
        if (f.sizeX>static_cast<float>(desktopX)) f.sizeX=static_cast<float>(desktopX);
        const int desktopY=GetSystemMetrics(1);
        if (f.sizeY>static_cast<float>(desktopY)) f.sizeY=static_cast<float>(desktopY);

        const int desktopBpp=FormatBPP(drv.desktopPixel);
        const int want32=(f.stateFlags&2u)?1:0;
        const int desktop32=(desktopBpp==32)?1:0;
        if (want32!=desktop32)
            f.stateFlags=(f.stateFlags&~2u)|(desktop32?2u:0u);
    }

    // Retail has a VRAM/triple-buffer test here, then unconditionally disables
    // triple buffering. Preserve that final state exactly.
    const int halfVideoMemory=drv.videoMemory/2;
    const float tripleNeed=f.sizeX*f.sizeY*6.0f;
    if (static_cast<float>(halfVideoMemory)>=tripleNeed) f.stateFlags&=~0x40u;
    f.stateFlags&=~0x40u;

    f.hwnd=hwnd;
    RECT_OLD windowRect={0,0,0,0};
    RECT_OLD clientRect={0,0,0,0};
    GetWindowRect(hwnd,&windowRect);
    GetClientRect(hwnd,&clientRect);
    POINT_OLD clientTL={clientRect.left,clientRect.top};
    POINT_OLD clientBR={clientRect.right,clientRect.bottom};
    ClientToScreen(hwnd,&clientTL);
    ClientToScreen(hwnd,&clientBR);
    f.debugFontHeight=23;

    const int initResult=GRAPH_CORE::Init(hwnd);
    if (initResult) return initResult;

    if (!(f.stateFlags&0x80u) || (f.stateFlags&0x400u)) {
        SetViewPort(static_cast<float>(clientTL.x-windowRect.left),
                    static_cast<float>(clientTL.y-windowRect.top),
                    static_cast<float>(clientBR.x-windowRect.left),
                    static_cast<float>(clientBR.y-windowRect.top));
    } else {
        SetViewPort(0.0f,0.0f,f.sizeX,f.sizeY);
    }

    if (::Error) {
        MYERROR::Log(::Error,"SetViewPort (%.0f,%.0f) - (%.0f,%.0f)",
                     static_cast<double>(ViewXMin()),static_cast<double>(ViewYMin()),
                     static_cast<double>(ViewXMax()),static_cast<double>(ViewYMax()));
        MYERROR::Log(::Error,"%s",GetTextureState());
    }
    SetDefaultState();
    if (::Error) MYERROR::Log(::Error,"%s",GetTextureState());

    TEXTURE::SetDeviceCaps(f.d3dDevice7);

    f.paletteSurface=new TEXTURE(256,256,0x29,0);
    if (!f.paletteSurface || !f.paletteSurface->IsExist()) {
        Error(3,"light buffer",0);
        return 1;
    }
    if (f.paletteSurface->IsPaletted()) {
        COLOR palette[256];
        for (int i=0;i<256;++i) {
            COLOR c(i,i,i);
            palette[i].color=c.color;
        }
        f.paletteSurface->SetPalette(palette);
    }

    f.surfaceBE4=new TEXTURE(256,256,0x17,0);
    if (!f.surfaceBE4 || !f.surfaceBE4->IsExist()) {
        Error(3,"hiBuffer",0);
        return 1;
    }
    f.surfaceBE8=new TEXTURE(256,256,0x1A,0);
    if (!f.surfaceBE8 || !f.surfaceBE8->IsExist()) {
        Error(3,"alphaBuffer",0);
        return 1;
    }

    CheckDriverBugs();
    ClearScreen(COLOR(0,0,0));
    RefreshBuffers();
    STRING courier("Courier");
    SetFont(&courier,7,8);

    RGB16::SetFormat(f.surfaceBE4->Format()==0x17);
    for (int i=0;i<256;i+=8) {
        f.snowRamp[i+0]=RGB16(i+0,i+0,i+0).color;
        f.snowRamp[i+1]=RGB16(i+1,i+1,i+8).color;
        f.snowRamp[i+2]=RGB16(i+8,i+2,i+2).color;
        f.snowRamp[i+3]=RGB16(i+8,i+3,i+8).color;
        f.snowRamp[i+4]=RGB16(i+4,i+8,i+4).color;
        f.snowRamp[i+5]=RGB16(i+5,i+8,i+8).color;
        f.snowRamp[i+6]=RGB16(i+8,i+8,i+6).color;
        f.snowRamp[i+7]=RGB16(i+8,i+8,i+8).color;
    }

    STRING caps;
    if (f.stateFlags&0x04u) caps+="ALPHAPALETTE ";
    if (f.stateFlags&0x10u) caps+="NONGDI ";
    if (f.stateFlags&0x20u) caps+="SOFTWARE "; else caps+="HARDWARE ";
    if (f.stateFlags&0x200u) caps+="LOWDETAIL ";
    if (f.stateFlags&0x08u) caps+="AGP ";
    if (!(f.stateFlags&0x80u)) caps+="WINDOWED ";
    if (f.stateFlags&0x40u) caps+="TRIPLEBUFFER "; else caps+="DOUBLEBUFFER ";
    if (f.stateFlags&0x800u) caps+="DOTPRODUCT3 ";
    if (!(f.stateFlags&0x1000u)) caps+="NOTMODULATE2X ";
    if (!(f.stateFlags&0x2000u)) caps+="CAN'T_Z_BLT ";
    if (f.stateFlags&0x02u) caps+="COLOR32 ";
    if (f.stateFlags&0x100u) caps+="VSYNC ";
    STRING shader=Printf("PIXELSHADER=%i",f.pixelShader);
    caps+=shader.CharPtr();
    if (::Error) MYERROR::Log(::Error,"caps=%s",caps.CharPtr());

    Lock();
    UnLock();
    LockZ();
    UnLockZ();
    if (::Error) MYERROR::Log(::Error,"Pitch=%i zPitch=%i",f.colorPitch*BytesPerPixel(),f.zPitch*2);
    ReLoadPalettes();
    return initResult;
}

GRAPH::~GRAPH()
{
    // debugText and GRAPH_CORE are destroyed automatically in retail order.
}

// --- accelerated GRAPH_CORE::PreTact dependency closure -------------------
namespace {
// MapEdit.exe BSS/data owners populated by DirectDraw pixel-format enumeration
// during GRAPH_CORE initialization.  They are zero-initialized in the retail
// image and copied from enumerated DDPIXELFORMAT records later.
DDPIXELFORMAT_OLD g_pf565 = {0};       // original 0x004CE140
DDPIXELFORMAT_OLD g_pf555 = {0};       // original 0x004CE160
DDPIXELFORMAT_OLD g_pf4444 = {0};      // original 0x004CE180
DDPIXELFORMAT_OLD g_pfPal8 = {0};      // original 0x004CE1A8
DDPIXELFORMAT_OLD g_pfZ16 = {0};       // original 0x004CE1E0
void ResetEnumeratedPixelFormatsRetail()
{
    // Retail resets only the leading dword of each global descriptor here.
    g_pf4444.dwSize=0;
    g_pf555.dwSize=g_pf4444.dwSize;
    g_pf565.dwSize=g_pf555.dwSize;
    g_pfPal8.dwSize=g_pf565.dwSize;
    g_pfZ16.dwSize=g_pfPal8.dwSize;
}
const DDPIXELFORMAT_OLD g_pfDXT1 = {0x20u,0x04u,0x31545844u,0,0,0,0,0};
const DDPIXELFORMAT_OLD g_pfDXT3 = {0x20u,0x04u,0x33545844u,0,0,0,0,0};
int g_capsSquareTextures = 0;          // original 0x004CE20C
int g_capsPowerOfTwoTextures = 1;      // original 0x004BC2B8
int g_capsMaxTextureX = 256;           // original 0x004BC2BC
int g_capsMaxTextureY = 256;           // original 0x004BC2C0
char g_textureCreateSurfaceText[1] = {0}; // original 0x004CE214, initialized later

long CallCreateSurface(void* dd,void* desc,void** surface)
{
    void** vt=*reinterpret_cast<void***>(dd);
    typedef long (__stdcall *Method)(void*,void*,void**,void*);
    return reinterpret_cast<Method>(vt[0x18/4])(dd,desc,surface,0);
}
}

void TEXTURE::SetDeviceCaps(void* d3d_device)
{
    unsigned char caps[0xEC];
    memset(caps,0,sizeof(caps));
    if (DeviceGetCaps(d3d_device,caps)<0)
        SURFACE::Error(9,"Caps",0);

    g_capsMaxTextureX=*reinterpret_cast<int*>(caps+0x84);
    g_capsMaxTextureY=*reinterpret_cast<int*>(caps+0x88);
    const unsigned long textureCaps=*reinterpret_cast<unsigned long*>(caps+0x5C);
    g_capsSquareTextures=static_cast<int>(textureCaps&0x20u);
    g_capsPowerOfTwoTextures=static_cast<int>(textureCaps&0x02u);

    STRING text;
    if (g_capsSquareTextures) text+="SQUARE ";
    if (g_capsPowerOfTwoTextures) text+="POWER2 ";
    else text+="NOTPOWER2 ";
    STRING maxSize=Printf("MAXSIZE=%i,%i",g_capsMaxTextureX,g_capsMaxTextureY);
    text+=maxSize.CharPtr();
    if (::Error) MYERROR::Log(::Error,"TextureCaps=%s",text.CharPtr());

    // Retail forces this compatibility switch on after logging the hardware bit.
    g_capsPowerOfTwoTextures=1;
}

namespace {
void SetTextArg(char* buf,unsigned int value)
{
    if (value&0x20u) strcat(buf,"alp-");
    if (value&0x10u) strcat(buf,"inv-");
    switch (value&0x0Fu) {
    case 2u: strcat(buf,"tex"); break;
    case 0u: strcat(buf,"dif"); break;
    case 4u: strcat(buf,"spec"); break;
    case 1u: strcat(buf,"cur"); break;
    case 3u: strcat(buf,"tfac"); break;
    default: break;
    }
}
const char* TextureOpNameRetail(unsigned long value)
{
    switch (value) {
    case 1u: return "dis";
    case 2u: return "sel1";
    case 3u: return "sel2";
    case 4u: return "mod";
    case 13u: return "tex_alpha";
    default: return "unknown";
    }
}
}

char* GetTextureState()
{
    static char buf[512]; // retail static text owner 0x004EE250
    strcpy(buf,"Op=");
    unsigned long value=0;
    DeviceGetTextureStageState(Graph->D3DDevice(),0,1,&value);
    strcat(buf,TextureOpNameRetail(value));
    strcat(buf," Arg1=");
    DeviceGetTextureStageState(Graph->D3DDevice(),0,2,&value);
    SetTextArg(buf,value);
    strcat(buf," Arg2=");
    DeviceGetTextureStageState(Graph->D3DDevice(),0,3,&value);
    SetTextArg(buf,value);
    strcat(buf," AOp=");
    DeviceGetTextureStageState(Graph->D3DDevice(),0,4,&value);
    strcat(buf,TextureOpNameRetail(value));
    strcat(buf," Arg1=");
    DeviceGetTextureStageState(Graph->D3DDevice(),0,5,&value);
    SetTextArg(buf,value);
    strcat(buf," Arg2=");
    DeviceGetTextureStageState(Graph->D3DDevice(),0,6,&value);
    SetTextArg(buf,value);
    return buf;
}

// Retail D3DFORMAT-name helper. Unknown values intentionally leave the
// shared buffer unchanged, matching the original switch default.
char* GetPixelFormat(int format)
{
    static char out[256] = "Unknown"; // original shared text buffer at 0x004BC2C4
    const char* text=0;
    switch (static_cast<unsigned int>(format)) {
    case 20u: text="R8G8B8"; break;
    case 21u: text="A8R8G8B8"; break;
    case 22u: text="X8R8G8B8"; break;
    case 23u: text="R5G6B5"; break;
    case 24u: text="X1R5G5B5"; break;
    case 25u: text="A1R5G5B5"; break;
    case 26u: text="A4R4G4B4"; break;
    case 28u: text="A8"; break;
    case 40u: text="A8P8"; break;
    case 41u: text="P8"; break;
    case 50u: text="L8"; break;
    case 51u: text="A8L8"; break;
    case 52u: text="A4L4"; break;
    case 60u: text="V8U8"; break;
    case 70u: text="D16_LOCKABLE"; break;
    case 71u: text="D32"; break;
    case 73u: text="D15S1"; break;
    case 75u: text="D24S8"; break;
    case 77u: text="D24X8"; break;
    case 80u: text="D16"; break;
    case 100u: text="VERTEXDATA"; break;
    case 101u: text="INDEX16"; break;
    case 102u: text="INDEX32"; break;
    case 0x31545844u: text="DXT1"; break;
    case 0x32545844u: text="DXT2"; break;
    case 0x33545844u: text="DXT3"; break;
    case 0x34545844u: text="DXT4"; break;
    case 0x35545844u: text="DXT5"; break;
    default: break;
    }
    if (text) strcpy(out,text);
    return out;
}

// This exact owner is used only on CreateSurface failure paths in this closure.
char* GetPixelFormat(DDPIXELFORMAT_OLD* p)
{
    static char out[256];
    int hasAlphaOrZ=0;
    out[0]=0;
    const unsigned int f=p->dwFlags;
    if (f & 0x80u) strcat(out,"COMPRESSED ");
    if (f & 0x100u) strcat(out,"RGBTOYUV ");
    if (f & 0x2u) strcat(out,"ALPHA ");
    if (p->dwFourCC) {
        strncat(out,reinterpret_cast<const char*>(&p->dwFourCC),4u);
        strcat(out," ");
    }
    if (f & 0x20u) { strcpy(out,"PAL8"); return out; }
    if (f & 0x08u) { strcpy(out,"PAL4"); return out; }
    if (f & 0x400u) {
        if (f & 0x4000u)
            sprintf(out,"STENCILBUFFER Z%i %X  S%i %X",
                    static_cast<int>(p->dwRGBBitCount),p->dwGBitMask,
                    static_cast<int>(p->dwRBitMask),p->dwBBitMask);
        else
            sprintf(out,"ZBUFFER%i %X",static_cast<int>(p->dwRGBBitCount),p->dwGBitMask);
        return out;
    }
    if (f & 0x8000u) { strcat(out,"ALPHAPREMULT "); hasAlphaOrZ=1; }
    if (f & 0x20000u) strcat(out,"LUMINANCE ");
    if (f & 0x80000u) strcat(out,"BUMP ");
    if (f & 0x1u) { strcat(out,"A"); hasAlphaOrZ=1; }
    if (f & 0x2000u) { strcat(out,"Z"); hasAlphaOrZ=1; }
    if (f & 0x20000u) strcat(out,"L ");
    if (f & 0x80000u) strcat(out,"UV");
    if (f & 0x40000u) strcat(out,"L ");
    if (f & 0x40u) {
        if (p->dwBBitMask < p->dwRBitMask) strcat(out,"RGB ");
        else strcat(out,"BGR ");
    }
    if (f & 0x200u) strcat(out,"YUV ");
    char countText[16];
    if (hasAlphaOrZ) {
        unsigned int count=0;
        for (unsigned int bit=0;bit<p->dwRGBBitCount;++bit)
            if (p->dwRGBAlphaBitMask&(1u<<bit)) ++count;
        _itoa(static_cast<int>(count),countText,10);
        strcat(out,countText);
        if (count>9u) strcat(out," ");
    }
    {
        unsigned int count=0;
        for (unsigned int bit=0;bit<p->dwRGBBitCount;++bit)
            if (p->dwRBitMask&(1u<<bit)) ++count;
        _itoa(static_cast<int>(count),countText,10);
        strcat(out,countText);
        if (count>9u) strcat(out," ");
    }
    {
        unsigned int count=0;
        for (unsigned int bit=0;bit<p->dwRGBBitCount;++bit)
            if (p->dwGBitMask&(1u<<bit)) ++count;
        _itoa(static_cast<int>(count),countText,10);
        strcat(out,countText);
        if (count>9u) strcat(out," ");
    }
    {
        unsigned int count=0;
        for (unsigned int bit=0;bit<p->dwRGBBitCount;++bit)
            if (p->dwBBitMask&(1u<<bit)) ++count;
        _itoa(static_cast<int>(count),countText,10);
        strcat(out,countText);
    }
    return out;
}

SURFACE::SURFACE() : m_surface(0) {}

SURFACE::SURFACE(int size_x,int size_y,D3DFORMAT format)
    : m_surface(0)
{
    for (;;) {
        const DDPIXELFORMAT_OLD* pf=0;
        if (format==0x29) pf=g_pfPal8.dwSize ? &g_pfPal8 : 0;
        else if (format==0x50) pf=&g_pfZ16;
        else if (static_cast<unsigned int>(format)==0x31545844u) pf=&g_pfDXT1;
        else if (static_cast<unsigned int>(format)==0x33545844u) pf=&g_pfDXT3;
        else if (format==0x1A) pf=&g_pf4444;
        else if (format==0x17) pf=g_pf565.dwSize ? &g_pf565 : 0;
        else if (format==0x18 || format==0x19) pf=g_pf555.dwSize ? &g_pf555 : 0;
        if (pf) {
            GRAPH_CORE::TYPE_SURFACE type=(format==0x50)
                ? GRAPH_CORE::S_ZBUFFER_SOFT : GRAPH_CORE::S_TEXTURE;
            m_surface=Graph->CreateSurface(size_x,size_y,type,pf);
        }
        if (m_surface) break;
        if (format==0x29 || static_cast<unsigned int>(format)==0x31545844u) format=0x17;
        else if (static_cast<unsigned int>(format)==0x33545844u) format=0x1A;
        else if (format==0x17) format=0x18;
        else if (format==0x18) format=0x19;
        else break;
    }
    m_sizeX=size_x;
    m_sizeY=size_y;
    m_format=format;
    if (!m_surface) Error(3,g_textureCreateSurfaceText,0);
    else g_surfaceMemoryInUse+=MemorySize();
}

SURFACE::~SURFACE()
{
    if (!m_surface) return;
    void* const device=Graph->D3DDevice();
    void** dvt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *SetTextureMethod)(void*,unsigned long,void*);
    reinterpret_cast<SetTextureMethod>(dvt[0x8C/4])(device,0,0);
    void** svt=*reinterpret_cast<void***>(m_surface);
    typedef unsigned long (__stdcall *ReleaseMethod)(void*);
    const unsigned long hr=reinterpret_cast<ReleaseMethod>(svt[0x08/4])(m_surface);
    if (hr)
        Error(10,"Release count !=0",hr);
    else
        g_surfaceMemoryInUse-=MemorySize();
}

void SURFACE::Error(int type,const char* text,unsigned long err)
{
    if (::Error) MYERROR::Error(::Error,"TEXTURE",type,text,err);
}

int SURFACE::MemorySize() { return m_sizeX*m_sizeY*(IsPaletted()?1:2); }
int SURFACE::IsPaletted() { return m_format==0x29; }
int SURFACE::Format() { return m_format; }
int SURFACE::IsExist() { return m_surface!=0; }

SURFACE::operator IDirectDrawSurface7*()
{
    return reinterpret_cast<IDirectDrawSurface7*>(m_surface);
}

void* GRAPH_CORE::D3DDevice() { return Fields(reinterpret_cast<GRAPH*>(this)).d3dDevice7; }

IDirectDrawSurface7* GRAPH_CORE::ZBufferSoft()
{
    return reinterpret_cast<IDirectDrawSurface7*>(Fields(reinterpret_cast<GRAPH*>(this)).zSurface);
}

void* GRAPH_CORE::DDraw() { return Fields(reinterpret_cast<GRAPH*>(this)).directDraw; }

void SURFACE::SetPalette(COLOR* palette)
{
    if (!m_surface) { Error(8,"palette for non initialized texture",0); return; }
    if (Format()!=0x29) { Error(8,"palette for non palette texture",0); return; }
    void* pal=0;
    void* dd=Graph->DDraw();
    void** dvt=*reinterpret_cast<void***>(dd);
    typedef long (__stdcall *CreatePaletteMethod)(void*,unsigned long,COLOR*,void**,void*);
    reinterpret_cast<CreatePaletteMethod>(dvt[0x14/4])(dd,4u,palette,&pal,0);
    void** svt=*reinterpret_cast<void***>(m_surface);
    typedef long (__stdcall *SetPaletteMethod)(void*,void*);
    reinterpret_cast<SetPaletteMethod>(svt[0x7C/4])(m_surface,pal);
    void** pvt=*reinterpret_cast<void***>(pal);
    typedef unsigned long (__stdcall *ReleaseMethod)(void*);
    reinterpret_cast<ReleaseMethod>(pvt[0x08/4])(pal);
}

void* GRAPH_CORE::CreateSurface(int width,int height,TYPE_SURFACE type,const DDPIXELFORMAT_OLD* format)
{
    unsigned char desc[0x7C];
    memset(desc,0,sizeof(desc));
    *reinterpret_cast<uint32_t*>(desc+0x00)=0x7Cu;
    *reinterpret_cast<uint32_t*>(desc+0x04)=0x1007u;
    *reinterpret_cast<int*>(desc+0x08)=height;
    *reinterpret_cast<int*>(desc+0x0C)=width;
    memcpy(desc+0x48,format,0x20);
    void* surf=0;
    long hr=0;
    uint32_t& flags=*reinterpret_cast<uint32_t*>(desc+0x04);
    uint32_t& backCount=*reinterpret_cast<uint32_t*>(desc+0x14);
    uint32_t& caps=*reinterpret_cast<uint32_t*>(desc+0x68);
    uint32_t& caps2=*reinterpret_cast<uint32_t*>(desc+0x6C);
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));

    if (type==S_PRIMARY) {
        flags=1u; caps=0x200u;
        if ((f.stateFlags>>7)&1u) {
            flags|=0x20u;
            backCount=((f.stateFlags>>6)&1u) ? 2u : 1u;
            caps|=0x6018u;
        }
        hr=CallCreateSurface(f.directDraw,desc,&surf);
        if (hr) {
            backCount=1u;
            f.stateFlags&=~0x40u;
            hr=CallCreateSurface(f.directDraw,desc,&surf);
            if (hr) Error(3,"primary surface",static_cast<unsigned long>(hr));
        }
        return surf;
    }
    if (type==S_BACKBUFFER) {
        caps=0x6040u;
        hr=CallCreateSurface(f.directDraw,desc,&surf);
        if (hr) {
            Error(3,"backbuffer",static_cast<unsigned long>(hr));
            flags=7u;
            hr=CallCreateSurface(f.directDraw,desc,&surf);
            if (hr) Error(3,"backbuffer(try2)",static_cast<unsigned long>(hr));
        }
        return surf;
    }
    if (type==S_ZBUFFER && !((f.stateFlags>>5)&1u)) caps=0x24000u;
    else if (type==S_ZBUFFER || type==S_ZBUFFER_SOFT) caps=0x20800u;
    else if (type==S_OFFSCREEN) caps=0x840u;
    else {
        caps=0x1000u;
        if (((f.stateFlags>>5)&1u) || type==S_TEXTURE_SYSTEM) caps|=0x800u;
        else if (type==S_TEXTURE_DYNAMIC) caps|=0x4000u;
        else caps2=0x10u;
    }
    hr=CallCreateSurface(f.directDraw,desc,&surf);
    if (hr) Error(3,GetPixelFormat(reinterpret_cast<DDPIXELFORMAT_OLD*>(desc+0x48)),static_cast<unsigned long>(hr));
    return surf;
}

TEXTURE::TEXTURE(int size_x,int size_y,int format,unsigned long p)
{
    m_surface=0;
    if (format!=0x50 && g_capsSquareTextures) {
        if (size_x>size_y) size_y=size_x;
        else if (size_x<size_y) size_x=size_y;
    }
    if (g_capsPowerOfTwoTextures) {
        int x=1; while (size_x>x) x<<=1;
        int y=1; while (size_y>y) y<<=1;
        size_x=x; size_y=y;
    }
    if (size_x>g_capsMaxTextureX || size_x<=0) { Error(4,"initial sizeX",static_cast<unsigned long>(size_x)); return; }
    if (size_y>g_capsMaxTextureY || size_y<=0) { Error(4,"initial sizeY",static_cast<unsigned long>(size_y)); return; }

    for (;;) {
        const DDPIXELFORMAT_OLD* pf=0;
        if (format==0x29) pf=g_pfPal8.dwSize ? &g_pfPal8 : 0;
        else if (format==0x50) pf=&g_pfZ16;
        else if (static_cast<unsigned int>(format)==0x31545844u) pf=&g_pfDXT1;
        else if (static_cast<unsigned int>(format)==0x33545844u) pf=&g_pfDXT3;
        else if (format==0x1A) pf=&g_pf4444;
        else if (format==0x17) pf=g_pf565.dwSize ? &g_pf565 : 0;
        else if (format==0x18 || format==0x19) pf=g_pf555.dwSize ? &g_pf555 : 0;
        if (pf) {
            GRAPH_CORE::TYPE_SURFACE type=(p&1u)?GRAPH_CORE::S_TEXTURE_DYNAMIC:GRAPH_CORE::S_TEXTURE;
            if (format==0x50) type=GRAPH_CORE::S_ZBUFFER_SOFT;
            m_surface=Graph->CreateSurface(size_x,size_y,type,pf);
        }
        if (m_surface) break;
        if (format==0x29 || static_cast<unsigned int>(format)==0x31545844u) format=0x17;
        else if (static_cast<unsigned int>(format)==0x33545844u) format=0x1A;
        else if (format==0x17) format=0x18;
        else if (format==0x18) format=0x19;
        else break;
    }
    m_sizeX=size_x; m_sizeY=size_y; m_format=format;
    if (!m_surface) Error(3,g_textureCreateSurfaceText,0);
    else g_surfaceMemoryInUse+=MemorySize();
}

TEXTURE::~TEXTURE() {}

void GRAPH_CORE::ReLoadPalettes()
{
    TEXTURE*& paletteTexture=*reinterpret_cast<TEXTURE**>(reinterpret_cast<uint8_t*>(this)+0xBE0);
    if (!paletteTexture || !paletteTexture->IsPaletted()) return;
    delete paletteTexture;
    paletteTexture=new TEXTURE(256,256,0x29,0);
    if (!paletteTexture->IsExist()) Error(3,"Light at RelodPalette()",0);
    if (paletteTexture->IsPaletted()) {
        COLOR palette[256];
        for (int i=0;i<256;i++) {
            COLOR c(i,i,i);
            palette[i]=&c;
        }
        paletteTexture->SetPalette(palette);
    }
    MYERROR::Log(::Error,"ReloadPalettes");
}

int GRAPH_CORE::PreTact()
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    typedef long (__stdcall *SurfaceIsLost)(void*);
    void** pvt=*reinterpret_cast<void***>(f.captureSourceSurface);
    long lost=reinterpret_cast<SurfaceIsLost>(pvt[0x60/4])(f.captureSourceSurface);
    if (lost!=static_cast<long>(0x887601C2u)) {
        void** bvt=*reinterpret_cast<void***>(f.colorSurface);
        lost=reinterpret_cast<SurfaceIsLost>(bvt[0x60/4])(f.colorSurface);
    }
    if (lost==static_cast<long>(0x887601C2u)) {
        MYERROR::Log(::Error,"Restore surface");
        void** dvt=*reinterpret_cast<void***>(f.directDraw);
        typedef long (__stdcall *RestoreAll)(void*);
        reinterpret_cast<RestoreAll>(dvt[0x64/4])(f.directDraw);
        ReLoadPalettes();
    }
    void** vt=*reinterpret_cast<void***>(f.d3dDevice7);
    typedef long (__stdcall *BeginScene)(void*);
    const long hr=reinterpret_cast<BeginScene>(vt[0x14/4])(f.d3dDevice7);
    if (hr) Error(10,"3dBeginScene for PreTact",static_cast<unsigned long>(hr));
    return 0;
}

long GRAPH_CORE::CopyToZBuffer(RECT_OLD* screen_rect,RECT_OLD* tex_rect,TEXTURE* tex)
{
    UnLockZ();
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    void** vt=*reinterpret_cast<void***>(f.zSurface);
    typedef long (__stdcall *BltMethod)(void*,RECT_OLD*,void*,RECT_OLD*,unsigned long,void*);
    return reinterpret_cast<BltMethod>(vt[0x14/4])(f.zSurface,screen_rect,tex->m_surface,tex_rect,0x01000000u,0);
}

void GRAPH_CORE::UnLockZ()
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    if (f.zBuffer) {
        void** vt=*reinterpret_cast<void***>(f.zSurface);
        typedef long (__stdcall *UnlockMethod)(void*,void*);
        reinterpret_cast<UnlockMethod>(vt[0x80/4])(f.zSurface,0);
        f.zBuffer=0;
    }
}

void GRAPH_CORE::ClearScreen(COLOR fill)
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    RECT_OLD rect;
    rect.left=static_cast<long>(f.viewXMin);
    rect.top=static_cast<long>(f.viewYMin);
    rect.right=static_cast<long>(f.viewXMax);
    rect.bottom=static_cast<long>(f.viewYMax);

    UnLock();
    unsigned char fx[100];
    memset(fx,0,sizeof(fx));
    *reinterpret_cast<unsigned long*>(fx)=100u;
    *reinterpret_cast<unsigned long*>(fx+80)=fill.ARGB32();
    void** cvt=*reinterpret_cast<void***>(f.colorSurface);
    typedef long (__stdcall *BltMethod)(void*,RECT_OLD*,void*,RECT_OLD*,unsigned long,void*);
    reinterpret_cast<BltMethod>(cvt[0x14/4])(f.colorSurface,&rect,0,0,0x01000400u,fx);

    UnLockZ();
    *reinterpret_cast<unsigned long*>(fx+80)=0x3FFu;
    void** zvt=*reinterpret_cast<void***>(f.zSurface);
    reinterpret_cast<BltMethod>(zvt[0x14/4])(f.zSurface,0,0,0,0x03000000u,fx);
}

int GRAPH::CapsNotPalette()
{
    return Fields(this).paletteSurface->Format()!=0x29;
}

RGB16 GRAPH::ColorIntensity(int intensity)
{
    const RGB16* value=reinterpret_cast<const RGB16*>(reinterpret_cast<const uint8_t*>(this)+0x0C+intensity*2);
    return RGB16(value);
}

void SURFACE::UnLock()
{
    // Retail dispatches Unlock directly through m_surface; no null guard is present.
    void** vtable=*reinterpret_cast<void***>(m_surface);
    typedef long (__stdcall *UnlockMethod)(void*,void*);
    reinterpret_cast<UnlockMethod>(vtable[0x80/4])(m_surface,0);
}

unsigned char* SURFACE::Lock(int* pitch,RECT_OLD* rect)
{
    // Retail calls the surface Lock vtable entry directly; failure handling starts at the HRESULT.
    uint8_t desc[0x7c];
    *reinterpret_cast<uint32_t*>(desc)=0x7cu;
    void** vtable=*reinterpret_cast<void***>(m_surface);
    typedef long (__stdcall *LockMethod)(void*,RECT_OLD*,void*,unsigned long,void*);
    const long hr=reinterpret_cast<LockMethod>(vtable[0x64/4])(m_surface,rect,desc,0,0);
    if (hr) {
        Error(0,"SURFACE::Lock",static_cast<unsigned long>(hr));
        return 0;
    }
    *pitch=*reinterpret_cast<int*>(desc+0x10);
    return *reinterpret_cast<unsigned char**>(desc+0x24);
}

int SURFACE::SizeX() { return m_sizeX; }
int SURFACE::SizeY() { return m_sizeY; }

long SURFACE::CopyFromSurface(void* source_surface,RECT_OLD* source,POINT_OLD* dest)
{
    RECT_OLD destRect;
    destRect.left=dest->x;
    destRect.top=dest->y;
    destRect.right=dest->x + source->right - source->left;
    destRect.bottom=dest->y + source->bottom - source->top;
    void** vtable=*reinterpret_cast<void***>(m_surface);
    typedef long (__stdcall *BltMethod)(void*,RECT_OLD*,void*,RECT_OLD*,unsigned long,void*);
    const long hr=reinterpret_cast<BltMethod>(vtable[0x14/4])(
        m_surface,&destRect,source_surface,source,0x01000000u,0);
    if (hr)
        Error(1,"from surface",static_cast<unsigned long>(hr));
    return hr;
}

namespace {
struct TextureVertexRetail {
    float x,y,z,rhw;
    unsigned int diffuse;
    unsigned int specular;
    float tu,tv;
};
static_assert(sizeof(TextureVertexRetail)==0x20,"retail texture vertex stride");
}

void TEXTURE::Draw(unsigned long vertex_shader,void* vertex,unsigned int size_one_vertex)
{
    (void)size_one_vertex; // Retail owner does not read its third argument.
    void* device=Graph->D3DDevice();
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *SetTextureMethod)(void*,unsigned long,void*);
    long hr=reinterpret_cast<SetTextureMethod>(vt[0x8C/4])(device,0,m_surface);
    if (hr) Error(8,"texture",static_cast<unsigned long>(hr));

    typedef long (__stdcall *DrawPrimitiveMethod)(void*,unsigned long,unsigned long,void*,unsigned long,unsigned long);
    hr=reinterpret_cast<DrawPrimitiveMethod>(vt[0x64/4])(device,6u,vertex_shader,vertex,4u,0u);
    if (hr) Error(10,"DrawPrimitiveUP",static_cast<unsigned long>(hr));
}

void TEXTURE::SetTexture(int stage)
{
    void* device=Graph->D3DDevice();
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *SetTextureMethod)(void*,unsigned long,void*);
    reinterpret_cast<SetTextureMethod>(vt[0x8C/4])(device,static_cast<unsigned long>(stage),m_surface);
}

void TEXTURE::Draw(RECT_OLD* screen,RECT_OLD* tex,const GAMMA* gamma)
{
    // Retail has no synthetic Graph/surface early-return; it enters the GAMMA/render path directly.
    const unsigned long diffuse=const_cast<GAMMA*>(gamma)->Diffuse();
    const unsigned long specular=const_cast<GAMMA*>(gamma)->Specular();
    const float invW=1.0f/static_cast<float>(SizeX());
    const float invH=1.0f/static_cast<float>(SizeY());
    const float u0=(static_cast<float>(tex->left)+0.5f)*invW;
    const float u1=(static_cast<float>(tex->right)+0.5f)*invW;
    const float v0=(static_cast<float>(tex->top)+0.5f)*invH;
    const float v1=(static_cast<float>(tex->bottom)+0.5f)*invH;
    const float z=0.99999988f;
    TextureVertexRetail p[4]={
        {static_cast<float>(screen->left),static_cast<float>(screen->top),z,1.0f,diffuse,specular,u0,v0},
        {static_cast<float>(screen->right),static_cast<float>(screen->top),z,1.0f,diffuse,specular,u1,v0},
        {static_cast<float>(screen->right),static_cast<float>(screen->bottom),z,1.0f,diffuse,specular,u1,v1},
        {static_cast<float>(screen->left),static_cast<float>(screen->bottom),z,1.0f,diffuse,specular,u0,v1}
    };
    void* dev=Graph->D3DDevice();
    void** vt=*reinterpret_cast<void***>(dev);
    typedef long (__stdcall *SetTextureMethod)(void*,unsigned long,void*);
    typedef long (__stdcall *StageMethod)(void*,unsigned long,unsigned long,unsigned long);
    reinterpret_cast<SetTextureMethod>(vt[0x8C/4])(dev,0,m_surface);
    const int exact=(screen->right-screen->left)==(tex->right-tex->left) &&
                    (screen->bottom-screen->top)==(tex->bottom-tex->top);
    StageMethod stage=reinterpret_cast<StageMethod>(vt[0x94/4]);
    stage(dev,0,0x10u,exact?1u:2u);
    stage(dev,0,0x11u,exact?1u:2u);
    Graph->SetRenderState(0x1Du,specular!=0u);
    Graph->DrawPrimitive(6u,0x1C4u,p,sizeof(TextureVertexRetail),4);
}

void TEXTURE::Draw(float z1,float z2,RECT_OLD* screen,RECT_OLD* tex,const GAMMA* gamma)
{
    // Retail has no synthetic Graph/surface early-return; it enters the GAMMA/render path directly.
    const unsigned long diffuse=const_cast<GAMMA*>(gamma)->Diffuse();
    const unsigned long specular=const_cast<GAMMA*>(gamma)->Specular();
    const float invW=1.0f/static_cast<float>(SizeX());
    const float invH=1.0f/static_cast<float>(SizeY());
    const float u0=(static_cast<float>(tex->left)+0.5f)*invW;
    const float u1=(static_cast<float>(tex->right)+0.5f)*invW;
    const float v0=(static_cast<float>(tex->top)+0.5f)*invH;
    const float v1=(static_cast<float>(tex->bottom)+0.5f)*invH;
    TextureVertexRetail p[4]={
        {static_cast<float>(screen->left),static_cast<float>(screen->top),z1,1.0f,diffuse,specular,u0,v0},
        {static_cast<float>(screen->right),static_cast<float>(screen->top),z1,1.0f,diffuse,specular,u1,v0},
        {static_cast<float>(screen->right),static_cast<float>(screen->bottom),z2,1.0f,diffuse,specular,u1,v1},
        {static_cast<float>(screen->left),static_cast<float>(screen->bottom),z2,1.0f,diffuse,specular,u0,v1}
    };
    void* dev=Graph->D3DDevice();
    void** vt=*reinterpret_cast<void***>(dev);
    typedef long (__stdcall *SetTextureMethod)(void*,unsigned long,void*);
    typedef long (__stdcall *StageMethod)(void*,unsigned long,unsigned long,unsigned long);
    reinterpret_cast<SetTextureMethod>(vt[0x8C/4])(dev,0,m_surface);
    const int exact=(screen->right-screen->left)==(tex->right-tex->left) &&
                    (screen->bottom-screen->top)==(tex->bottom-tex->top);
    StageMethod stage=reinterpret_cast<StageMethod>(vt[0x94/4]);
    stage(dev,0,0x10u,exact?1u:2u);
    stage(dev,0,0x11u,exact?1u:2u);
    Graph->SetRenderState(0x1Du,specular!=0u);
    Graph->DrawPrimitive(6u,0x1C4u,p,sizeof(TextureVertexRetail),4);
}

int GRAPH_CORE::BlitZBuffer()
{
    GraphAbiFields& f=Fields(reinterpret_cast<GRAPH*>(this));
    if ((f.stateFlags>>5)&1u)
        return 0;
    if ((f.stateFlags>>13)&1u) {
        RECT_OLD rect={static_cast<long>(f.viewXMin),static_cast<long>(f.viewYMin),
                       static_cast<long>(f.viewXMax),static_cast<long>(f.viewYMax)};
        UnLockZ();
        void** vt=*reinterpret_cast<void***>(f.realZSurface);
        typedef long (__stdcall *BltFastMethod)(void*,unsigned long,unsigned long,void*,RECT_OLD*,unsigned long);
        const long hr=reinterpret_cast<BltFastMethod>(vt[0x1C/4])(
            f.realZSurface,static_cast<unsigned long>(f.viewXMin),static_cast<unsigned long>(f.viewYMin),
            f.zSurface,&rect,0x10u);
        if (hr) {
            Error(1,"zbuffer to zbufferhard",static_cast<unsigned long>(hr));
            return 1;
        }
        return 0;
    }

    LockZ();
    unsigned char* src=reinterpret_cast<unsigned char*>(f.zBuffer);
    int pitch=0;
    unsigned char* dst=static_cast<unsigned char*>(LockSurface(f.realZSurface,&pitch,0));
    if (!dst) {
        Error(0,"ZBufferHard",0);
        return 1;
    }
    const int rowBytes=f.zPitch*2;
    const int rows=static_cast<int>(f.sizeY);
    if (rowBytes==pitch) {
        memcpy(dst,src,static_cast<unsigned int>(rows*pitch));
    } else {
        for (int y=0;y<rows;++y) {
            memcpy(dst,src,static_cast<unsigned int>(rowBytes));
            dst+=pitch;
            src+=rowBytes;
        }
    }
    void** vt=*reinterpret_cast<void***>(f.realZSurface);
    typedef long (__stdcall *UnlockMethod)(void*,void*);
    reinterpret_cast<UnlockMethod>(vt[0x80/4])(f.realZSurface,0);
    return 0;
}

namespace {
struct WeatherVertexRetail {
    float x;
    float y;
    float z;
    float rhw;
    unsigned long color;
};
static_assert(sizeof(WeatherVertexRetail)==0x14,"MapEdit weather vertex stride");

WeatherVertexRetail g_rainVertices[500] = {};
int g_rainCount = 0;
ANGLE g_rainWindAngle;
float g_rainWindForce = 99999.0f;
float g_rainWindDrift = 0.0f;

WeatherVertexRetail g_snowVertices[6000] = {};
int g_snowCount = 0;
ANGLE g_snowWindAngle;
float g_snowWindForce = 99999.0f;
float g_snowWindDrift = 0.0f;
float g_snowPrevScreenX = 0.0f;
float g_snowPrevScreenY = 0.0f;

unsigned long g_groundSnowStart = 0;
int g_groundSnowAmount = 0;

void SetTexture0(void* device,void* texture)
{
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*,unsigned long,void*);
    reinterpret_cast<Method>(vt[0x8C/4])(device,0,texture);
}

void SetTextureStage(void* device,unsigned long state,unsigned long value)
{
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long,unsigned long);
    reinterpret_cast<Method>(vt[0x94/4])(device,0,state,value);
}

long SceneCall(void* device,unsigned int vtableByteOffset)
{
    void** vt=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vt[vtableByteOffset/4])(device);
}

void DrawSpriteVirtual(SPRITE* sprite)
{
    void** vt=*reinterpret_cast<void***>(sprite);
    typedef void (__thiscall *Method)(SPRITE*);
    reinterpret_cast<Method>(vt[0x14/4])(sprite);
}

}


// Direct DX7 rain-streak path.  This is the original 0x14-byte particle
// representation, not the newer WEATHER_RASTER implementation.
void GRAPH::DrawRain()
{
    GraphAbiFields& f=Fields(this);
    const unsigned int mode=f.environment&0x0C00u;
    if (!mode) { g_rainCount=0; return; }
    if ((f.stateFlags&0x20u) || Map->IsPaused()) return;

    if (g_rainWindAngle.operator!=(&f.windDirection) || g_rainWindForce!=f.windForce) {
        const float old=g_rainWindDrift;
        g_rainWindDrift=f.windDirection.Sin()*f.windForce*1000.0f;
        if (old!=g_rainWindDrift) {
            for (int i=0;i<g_rainCount;++i) {
                WeatherVertexRetail& a=g_rainVertices[i*2];
                WeatherVertexRetail& b=g_rainVertices[i*2+1];
                b.x+=(b.y-a.y)*(g_rainWindDrift-old)/200.0f;
            }
        }
        g_rainWindAngle.operator=(&f.windDirection);
        g_rainWindForce=f.windForce;
    }

    if (mode==0x0C00u) g_rainCount=250;
    else if (mode==0x0800u) {
        if (g_rainCount<250) ++g_rainCount;
        else { g_rainCount=250; SetEnvironment(0x0C00u); }
    } else if (mode==0x0400u) {
        if (g_rainCount>0) --g_rainCount;
        else f.environment&=~0x0C00u;
    }

    const unsigned long dt=CurrentTime-PrevCurrentTime;
    int created=0;
    for (int i=0;i<g_rainCount;++i) {
        WeatherVertexRetail& a=g_rainVertices[i*2];
        WeatherVertexRetail& b=g_rainVertices[i*2+1];
        const bool reset=(a.color==0u || a.y>f.viewYMax || a.z<0.015625f);
        if (reset) {
            ++created;
            const float dy=static_cast<float>(Random(50)+15);
            const float dx=-dy*g_rainWindDrift/200.0f;
            const float x=Random(f.viewXMax+100.0f)-50.0f;
            const float y=Random(created<50?40.0f:f.viewYMax);
            const float z=((dy+10.0f)*10.0f)*(1.0f/8192.0f)+(1.0f/64.0f);
            a.x=x; a.y=y; a.z=z; a.rhw=1.0f; a.color=0x70E0E0FFu;
            b.x=x+dx; b.y=y-dy; b.z=z+dy*(1.0f/8192.0f); b.rhw=1.0f; b.color=0x308080FFu;
            continue;
        }

        const float dy=static_cast<float>(dt)*(b.z-a.z)*200.0f;
        float dx=dy*g_rainWindDrift/200.0f;
        if (a.x+dx<0.0f) dx+=f.sizeX;
        if (a.x+dx>f.sizeX) dx-=f.sizeX;
        a.x+=dx; b.x+=dx;
        a.y+=dy; b.y+=dy;
        a.z-=dy*(1.0f/8192.0f); b.z-=dy*(1.0f/8192.0f);
    }

    SetTexture0(f.d3dDevice7,0);
    SetAlphaBlend(5u,6u);
    DrawPrimitive(2u,0x44u,g_rainVertices,sizeof(WeatherVertexRetail),g_rainCount*2);
}

// Retail snow path: six 0x14-byte vertices (three line segments) are maintained
// for each logical flake.  The final DrawPrimitive count is intentionally only
// snowCount*2, exactly as in the original executable.
void GRAPH::DrawSnow()
{
    GraphAbiFields& f=Fields(this);
    const unsigned int mode=f.environment&0xC000u;
    if (!mode) {
        g_snowCount=0;
        return;
    }
    if ((f.stateFlags&0x20u) || Map->IsPaused())
        return;

    if (g_snowWindAngle.operator!=(&f.windDirection) || g_snowWindForce!=f.windForce) {
        g_snowWindDrift=f.windDirection.Sin()*f.windForce*1000.0f;
        g_snowWindAngle.operator=(&f.windDirection);
        g_snowWindForce=f.windForce;
    }

    if (mode==0xC000u) {
        g_snowCount=1000;
    } else if (mode==0x8000u) {
        if (g_snowCount<1000)
            ++g_snowCount;
        else {
            g_snowCount=1000;
            SetEnvironment(0xC000u);
        }
    } else if (mode==0x4000u) {
        if (g_snowCount>0)
            --g_snowCount;
        else
            f.environment&=~0xC000u;
    }

    const unsigned long dt=CurrentTime-PrevCurrentTime;
    int created=0;
    int vertexIndex=0;
    for (int i=0;i<g_snowCount;++i) {
        WeatherVertexRetail& first=g_snowVertices[vertexIndex];
        if (first.color==0u || first.z<0.015625f) {
            ++created;
            const float half=static_cast<float>(Random(2)+2);
            const float x=Random(f.viewXMax-1.0f);
            const float y=Random(created<50 ? 40.0f : f.viewYMax);
            const float z=Random(f.viewYMax+50.0f)*(1.0f/8192.0f)+(1.0f/64.0f);
            const float halfZ=half*(1.0f/8192.0f);

            WeatherVertexRetail& v0=g_snowVertices[vertexIndex++];
            v0.x=x-half;      v0.y=y;      v0.z=z;       v0.rhw=1.0f; v0.color=0xFFFFFFFFu;
            WeatherVertexRetail& v1=g_snowVertices[vertexIndex++];
            v1.x=x+half;      v1.y=y;      v1.z=z+halfZ; v1.rhw=1.0f; v1.color=0xFFFFFFFFu;
            WeatherVertexRetail& v2=g_snowVertices[vertexIndex++];
            v2.x=x-half*0.5f; v2.y=y-half; v2.z=z;       v2.rhw=1.0f; v2.color=0xFFFFFFFFu;
            WeatherVertexRetail& v3=g_snowVertices[vertexIndex++];
            v3.x=x+half*0.5f; v3.y=y+half; v3.z=z;       v3.rhw=1.0f; v3.color=0xFFFFFFFFu;
            WeatherVertexRetail& v4=g_snowVertices[vertexIndex++];
            v4.x=x-half*0.5f; v4.y=y+half; v4.z=z;       v4.rhw=1.0f; v4.color=0xFFFFFFFFu;
            WeatherVertexRetail& v5=g_snowVertices[vertexIndex++];
            v5.x=x+half*0.5f; v5.y=y-half; v5.z=z;       v5.rhw=1.0f; v5.color=0xFFFFFFFFu;
            continue;
        }

        const WeatherVertexRetail& second=g_snowVertices[vertexIndex+1];
        const float fall=static_cast<float>(dt)*(second.z-first.z-(1.0f/8192.0f))*150.0f;
        const float windJitter=(fall*g_snowWindDrift+50.0f-static_cast<float>(Random(100)))/50.0f;
        const float currentScreenX=Map->ToScreenX(0.0f);
        float dx=windJitter-(g_snowPrevScreenX-currentScreenX);
        const float dz=fall*(1.0f/8192.0f);
        const float currentScreenY=Map->ToScreenY(0.0f);
        float dy=fall-(g_snowPrevScreenY-currentScreenY);

        if (first.x+dx<f.viewXMin-30.0f)
            dx+=f.viewXMax-f.viewXMin;
        if (first.x+dx>f.viewXMax+30.0f)
            dx-=f.viewXMax-f.viewXMin;
        if (first.y+dy<f.viewYMin-30.0f)
            dy+=f.viewYMax-f.viewYMin;
        if (first.y+dy>f.viewYMax+30.0f)
            dy-=f.viewYMax-f.viewYMin;

        for (int n=0;n<6;++n) {
            WeatherVertexRetail& v=g_snowVertices[vertexIndex++];
            v.x+=dx;
            v.y+=dy;
            v.z-=dz;
        }
    }

    g_snowPrevScreenX=Map->ToScreenX(0.0f);
    g_snowPrevScreenY=Map->ToScreenY(0.0f);

    // The retail loop advances through the vertex array only when Random(4)
    // returns zero.  This unusual behavior is preserved verbatim.
    vertexIndex=0;
    for (int i=0;i<g_snowCount;++i) {
        if (Random(4)!=0)
            continue;

        const float centerX=(g_snowVertices[vertexIndex+1].x+g_snowVertices[vertexIndex].x)*0.5f;
        const float centerY=(g_snowVertices[vertexIndex+1].y+g_snowVertices[vertexIndex].y)*0.5f;
        for (int n=0;n<6;++n) {
            WeatherVertexRetail& v=g_snowVertices[vertexIndex++];
            const float oldX=v.x;
            v.x=centerX+v.y-centerY;
            v.y=centerY+oldX-centerX;
        }
    }

    SetTexture0(f.d3dDevice7,0);
    SetAlphaBlend(5u,6u);
    DrawPrimitive(2u,0x44u,g_snowVertices,sizeof(WeatherVertexRetail),g_snowCount*2);
}

void GRAPH::DrawFog(float fx0,float fy0,float fx1,float fy1,int fogBottom,int fogTop,
                    COLOR fogColor,unsigned short* table,int end,int invert)
{
    int x0=static_cast<int>(fx0);
    int y0=static_cast<int>(fy0);
    int x1=static_cast<int>(fx1);
    int y1=static_cast<int>(fy1);
    GraphAbiFields& f=Fields(this);
    if (f.stateFlags&0x20u)
        return;
    if (!table || !BoxInViewPort(fx0,fy0,fx1,fy1))
        return;

    if (fx0<f.viewXMin) x0=static_cast<int>(f.viewXMin);
    if (fy0<f.viewYMin) y0=static_cast<int>(f.viewYMin);
    if (fx1>=f.viewXMax) x1=static_cast<int>(f.viewXMax);
    if (fy1>=f.viewYMax) y1=static_cast<int>(f.viewYMax);

    const int sizeX=x1-x0;
    const int sizeY=y1-y0;
    if (sizeX<4 || sizeY<4)
        return;

    const int begin=end-((fogTop-fogBottom)<<3);
    LockZ();

    RECT_OLD screen={x0,y0,x1,y1};
    RECT_OLD tex={0,0,sizeX/4,sizeY/4};
    int pitch=0;
    TEXTURE* fogTexture=reinterpret_cast<TEXTURE*>(f.paletteSurface);
    unsigned char* shadow=fogTexture ? fogTexture->Lock(&pitch,&tex) : 0;
    if (!shadow) {
        Error(0,"fog buffer",0);
        return;
    }

    if (!fogTexture->IsPaletted()) {
        unsigned short old=0;
        int y=y0;
        int zAddr=y0*f.zPitch+x0;
        unsigned char* out=shadow;
        while (y<y1) {
            old=0;
            int x=x0;
            while (x<x1) {
                const unsigned int z0=f.zBuffer[zAddr];
                const unsigned int z3=f.zBuffer[zAddr+3];
                const int z=static_cast<int>(z0<z3?z0:z3)-0x400;
                if (z<=end) {
                    if (z<=begin)
                        old=table[end-begin];
                    else
                        old=table[end-z];
                } else if (z<=end+10) {
                    old=0;
                }
                *reinterpret_cast<unsigned short*>(out)=old;
                out+=2;
                x+=4;
                zAddr+=4;
            }
            y+=4;
            zAddr+=f.zPitch*4-((sizeX+3)/4)*4;
            out+=pitch-((sizeX+3)/4)*2;
        }
    } else {
        unsigned char old=0;
        int y=y0;
        int zAddr=y0*f.zPitch+x0;
        unsigned char* out=shadow;
        while (y<y1) {
            old=0;
            int x=x0;
            while (x<x1) {
                const unsigned int z0=f.zBuffer[zAddr];
                const unsigned int z3=f.zBuffer[zAddr+3];
                const int z=static_cast<int>(z0<z3?z0:z3)-0x400;
                if (z<=end) {
                    if (z<=begin)
                        old=0xffu;
                    else
                        old=static_cast<unsigned char>(table[end-z]);
                } else if (z<=end+10) {
                    old=0;
                }
                *out++=old;
                x+=4;
                zAddr+=4;
            }
            y+=4;
            zAddr+=f.zPitch*4-((sizeX+3)/4)*4;
            out+=pitch-((sizeX+3)/4);
        }
    }

    fogTexture->UnLock();
    SetAlphaBlend(invert?1u:2u,4u);
    const float zFog=static_cast<float>(end+0x3FE)/65536.0f;
    GAMMA gamma(fogColor,COLOR(0,0,0));
    fogTexture->Draw(zFog,zFog,&screen,&tex,&gamma);
}

void GRAPH::DrawGroundSnow()
{
    GraphAbiFields& f=Fields(this);

    // Retail samples the map shift before either early-return condition and
    // aligns the quarter-resolution snow texture to that screen-space phase.
    const int screenX0=static_cast<int>(Map->ToScreenX(0.0f));
    const int alignX=(4-((-screenX0)&3))&3;
    const int screenY0=static_cast<int>(Map->ToScreenY(0.0f));
    const int alignY=(4-((-screenY0)&3))&3;

    if ((f.stateFlags&0x20u) || Map->IsPaused())
        return;

    if ((f.environment&0x40u)==0u) {
        g_groundSnowStart=CurrentTime;
        g_groundSnowAmount=0;
        return;
    }

    // Retail only checks the old value before updating it.  There is no
    // post-update clamp to 256 in MapEdit.exe.
    if (static_cast<unsigned int>(g_groundSnowAmount)<256u)
        g_groundSnowAmount=static_cast<int>((CurrentTime-g_groundSnowStart)>>7);

    LockZ();

    RECT_OLD textureRect={0,0,
        static_cast<long>(static_cast<int>(f.sizeX)/4),
        static_cast<long>(static_cast<int>(f.sizeY)/4)};
    RECT_OLD screenRect={0,0,
        static_cast<long>(static_cast<int>(f.sizeX)),
        static_cast<long>(static_cast<int>(f.sizeY))};

    int pitch=0;
    unsigned char* const dst=f.paletteSurface->Lock(&pitch,&textureRect);
    if (!dst) {
        Error(0,"snow buffer",0);
        return;
    }

    if (!f.paletteSurface->IsPaletted()) {
        // Lock returns byte pitch; the 16-bit branch indexes WORDs.
        pitch/=2;
        int y=alignY;
        int zIndex=alignY*f.zPitch+alignX;
        while (y<f.sizeY) {
            int x=alignX;
            while (x<f.sizeX) {
                bool written=false;
                if (y>3) {
                    const int slope=abs(static_cast<int>(f.zBuffer[zIndex])-
                                        static_cast<int>(f.zBuffer[zIndex-f.zPitch*4]));
                    if (slope<=6) {
                        const unsigned int level=(static_cast<unsigned int>(6-slope)*
                                                  static_cast<unsigned int>(g_groundSnowAmount))>>3;
                        reinterpret_cast<unsigned short*>(dst)[x/4+(y/4)*pitch]=f.snowRamp[level];
                        written=true;
                    }
                }
                if (!written) {
                    const int slope=abs(static_cast<int>(f.zBuffer[zIndex])-
                                        static_cast<int>(f.zBuffer[zIndex+f.zPitch*4]));
                    if (slope<=6) {
                        const unsigned int level=(static_cast<unsigned int>(6-slope)*
                                                  static_cast<unsigned int>(g_groundSnowAmount))>>3;
                        reinterpret_cast<unsigned short*>(dst)[x/4+(y/4)*pitch]=f.snowRamp[level];
                    } else {
                        reinterpret_cast<unsigned short*>(dst)[x/4+(y/4)*pitch]=0;
                    }
                }
                x+=4;
                zIndex+=4;
            }
            y+=4;
            zIndex+=f.zPitch*3;
        }
    } else {
        int y=alignY;
        int zIndex=alignY*f.zPitch+alignX;
        while (y<f.sizeY) {
            int x=alignX;
            while (x<f.sizeX) {
                bool written=false;
                if (y>3) {
                    const int slope=abs(static_cast<int>(f.zBuffer[zIndex])-
                                        static_cast<int>(f.zBuffer[zIndex-f.zPitch*4]));
                    if (slope<=6) {
                        const unsigned int level=(static_cast<unsigned int>(6-slope)*
                                                  static_cast<unsigned int>(g_groundSnowAmount))>>3;
                        dst[x/4+(y/4)*pitch]=static_cast<unsigned char>(level);
                        written=true;
                    }
                }
                if (!written) {
                    const int slope=abs(static_cast<int>(f.zBuffer[zIndex])-
                                        static_cast<int>(f.zBuffer[zIndex+f.zPitch*4]));
                    if (slope<=6) {
                        const unsigned int level=(static_cast<unsigned int>(6-slope)*
                                                  static_cast<unsigned int>(g_groundSnowAmount))>>3;
                        dst[x/4+(y/4)*pitch]=static_cast<unsigned char>(level);
                    } else {
                        dst[x/4+(y/4)*pitch]=0;
                    }
                }
                x+=4;
                zIndex+=4;
            }
            y+=4;
            zIndex+=f.zPitch*3;
        }
    }

    f.paletteSurface->UnLock();
    SetRenderState(0x1Du,0u);
    SetAlphaBlend(2u,4u);
    GAMMA gamma;
    reinterpret_cast<TEXTURE*>(f.paletteSurface)->Draw(&screenRect,&textureRect,&gamma);
}

void GRAPH::DrawEffect(int draw)
{
    GraphAbiFields& f=Fields(this);
    unsigned int t=RealCurrentTime;

    if (f.effectStart[5]) {
        const unsigned int elapsed=t-f.effectStart[5];
        const unsigned int duration=static_cast<unsigned int>(f.effectDuration[5]);
        if (duration && elapsed<duration) {
            SetAlphaBlend(6u,5u);
            const unsigned int alpha=(elapsed<<8)/duration;
            if (draw && f.surfaceBE4 && f.effectSurface) {
                for (float y=f.viewYMin;y<f.viewYMax;y+=256.0f) {
                    for (float x=f.viewXMin;x<f.viewXMax;x+=256.0f) {
                        const int w=static_cast<int>((f.viewXMax-x)<256.0f?(f.viewXMax-x):256.0f);
                        const int h=static_cast<int>((f.viewYMax-y)<256.0f?(f.viewYMax-y):256.0f);
                        RECT_OLD src={static_cast<long>(x-f.viewXMin),static_cast<long>(y-f.viewYMin),
                                      static_cast<long>(x-f.viewXMin)+w,static_cast<long>(y-f.viewYMin)+h};
                        POINT_OLD dest={0,0};
                        if (f.surfaceBE4->CopyFromSurface(f.effectSurface,&src,&dest)==0) {
                            RECT_OLD screen={static_cast<long>(x),static_cast<long>(y),static_cast<long>(x)+w,static_cast<long>(y)+h};
                            RECT_OLD tex={0,0,w,h};
                            GAMMA gamma(COLOR(static_cast<int>(alpha),255,255,255),COLOR(0,0,0));
                            reinterpret_cast<TEXTURE*>(f.surfaceBE4)->Draw(&screen,&tex,&gamma);
                        }
                    }
                }
            }
        } else f.effectStart[5]=0;
    }

    t=RealCurrentTime;
    if (f.effectStart[2]) {
        const unsigned int elapsed=t-f.effectStart[2];
        const unsigned int duration=static_cast<unsigned int>(f.effectDuration[2]);
        if (duration && elapsed<duration) {
            const float k=static_cast<float>(elapsed)/static_cast<float>(duration);
            const float tx=static_cast<float>(f.effectVar1[2]);
            const float ty=static_cast<float>(f.effectVar2[2]);
            Map->SetShiftCoor(f.effectShiftX+(tx-f.effectShiftX)*k,
                              f.effectShiftY+(ty-f.effectShiftY)*k,0);
        } else {
            Map->SetShiftCoor(static_cast<float>(f.effectVar1[2]),static_cast<float>(f.effectVar2[2]),0);
            f.effectStart[2]=0;
        }
    }

    t=RealCurrentTime;
    if (f.effectStart[1]) {
        const unsigned int elapsed=t-f.effectStart[1];
        const unsigned int duration=static_cast<unsigned int>(f.effectDuration[1]);
        if (duration && elapsed<duration) {
            if (draw) {
                unsigned int phase=(elapsed*0x300u)/duration;
                unsigned int light=phase<0x100u?phase:(phase<0x200u?0x1FFu-phase:phase-0x200u);
                if (light>255u) light=255u;
                LightBar(0,0,f.sizeX,f.sizeY,light);
                LightBar(0,0,f.sizeX,f.sizeY,light);
                LightBar(0,0,f.sizeX,f.sizeY,light);
            }
        } else f.effectStart[1]=0;
    }

    t=RealCurrentTime;
    if (f.effectStart[3]) {
        const unsigned int elapsed=t-f.effectStart[3];
        const unsigned int duration=static_cast<unsigned int>(f.effectDuration[3]);
        if (duration && elapsed<duration) {
            if (draw) {
                const unsigned int third=duration/3u?duration/3u:1u;
                unsigned int shade=255u;
                if (elapsed<third) shade=(elapsed*255u)/third;
                else if (elapsed>=2u*third) shade=((duration-elapsed)*255u)/third;
                ShadowBar(0,0,f.sizeX,f.sizeY,static_cast<int>(shade*0x010101u));
            }
        } else f.effectStart[3]=0;
    }

    t=RealCurrentTime;
    if (f.effectStart[9]) {
        const unsigned int elapsed=t-f.effectStart[9];
        const unsigned int duration=static_cast<unsigned int>(f.effectDuration[9]);
        if (!duration || elapsed>=duration) f.effectStart[9]=0;
        else if (draw) {
            const unsigned int ramp=(duration*4u)/5u;
            const unsigned int shade=(elapsed<ramp && ramp)?(elapsed*255u/ramp):255u;
            ShadowBar(0,0,f.sizeX,f.sizeY,static_cast<int>(shade*0x010101u));
        }
    }

    t=RealCurrentTime;
    if (f.effectStart[10]) {
        const unsigned int elapsed=t-f.effectStart[10];
        const unsigned int duration=static_cast<unsigned int>(f.effectDuration[10]);
        if (!duration || elapsed>=duration) f.effectStart[10]=0;
        else if (draw) {
            const unsigned int shade=((duration-elapsed)*256u)/duration;
            ShadowBar(0,0,f.sizeX,f.sizeY,static_cast<int>((shade>255u?255u:shade)*0x010101u));
        }
    }

    t=RealCurrentTime;
    if (f.effectStart[11]) {
        const unsigned int elapsed=t-f.effectStart[11];
        const unsigned int duration=static_cast<unsigned int>(f.effectDuration[11]);
        if (elapsed>=duration) {
            GAMMA target(GAMMA::DECODE,static_cast<unsigned long>(f.effectVar1[11]));
            SetGamma(&target);
            f.effectStart[11]=0;
        } else if (draw) {
            const float interpolation=static_cast<float>(elapsed)/static_cast<float>(duration);
            GAMMA target(GAMMA::DECODE,static_cast<unsigned long>(f.effectVar1[11]));
            GAMMA current=InterpolateGamma(&f.effectGamma,&target,interpolation);
            SetGamma(&current);
        }
    }
}

void GRAPH::Tact(int draw)
{
    GraphAbiFields& f=Fields(this);
    float oldX=0.0f, oldY=0.0f;
    const bool squallShift=(f.environment&4u)!=0u && !Map->IsPaused();
    if (squallShift) {
        oldX=Map->FromScreenX(0.0f);
        oldY=Map->FromScreenY(0.0f);
        Map->SetShiftCoor(SizeX()*0.5f+oldX+4.0f-static_cast<float>(Random(8)),
                          SizeY()*0.5f+oldY+4.0f-static_cast<float>(Random(8)),0);
    }

    if (draw) {
        bool clear=!Map->ValidateVid(0x400);
        if (!clear) {
            const float x0=Map->FromScreenX(0.0f);
            const float y0=Map->FromScreenY(0.0f);
            const float worldW=x0+f.viewXMax-f.viewXMin;
            const float worldH=y0+f.viewYMax-f.viewYMin;
            clear=(x0<0.0f || y0<0.0f || Map->SizeX()<worldW || Map->SizeY()<worldH);
        }
        if (clear) { COLOR black(0,0,0); ClearScreen(black); }

        SetRenderState(0x1Bu,0u);
        SetRenderState(0x17u,8u);
        SetRenderState(0x0Eu,1u);
        SetTextureStage(f.d3dDevice7,0x10u,1u);
        UnLock();
        Map->DrawLayer(0);
        SetTexture0(f.d3dDevice7,0);
        Lock(); LockZ();
        Map->DrawLayer(1); Map->DrawLayer(2); Map->DrawLayer(3);
        UnLock();
        Map->DrawLayer(4);
        Lock(); LockZ();
        Map->DrawLayer(5); Map->DrawLayer(6); Map->DrawLayer(7);
        UnLock();
        const long endHr=SceneCall(f.d3dDevice7,0x18u);
        if (endHr) Error(10,"3dEndScene2",static_cast<unsigned long>(endHr));
        BlitZBuffer();
        const long beginHr=SceneCall(f.d3dDevice7,0x14u);
        if (beginHr) Error(10,"3dBeginScene2",static_cast<unsigned long>(beginHr));
        LockZ();
        SetRenderState(0x17u,7u);
        Map->DrawLayer(8);
        SetRenderState(0x0Eu,0u);
        SetRenderState(0x1Bu,1u);
        SetTextureStage(f.d3dDevice7,0x10u,1u);
        Map->DrawLayer(9); Map->DrawLayer(10);

        const int cx=static_cast<int>(Map->FromScreenX(f.sizeX*0.5f));
        const int cy=static_cast<int>(Map->FromScreenY(f.sizeY*0.5f));
        if ((f.environment&3u) && !(f.stateFlags&0x20u)) {
            for (int layer=5;layer<=6;++layer) {
                int it=0;
                for (SPRITE* s=Map->FirstSprite(layer,&it);s;s=Map->NextSprite(layer,&it)) {
                    const int sx=static_cast<int>(s->X());
                    const int sy=static_cast<int>(s->Y());
                    const int sz=static_cast<int>(s->Z());
                    const bool xNear=(((sx-cx+512)&~1023)==0);
                    const bool yzNear=((((sy-sz)-cy+512)&~1023)==0);
                    if ((xNear&&yzNear) || static_cast<int>(s->Y())-cy>=512)
                        s->DrawShadow();
                }
            }
        }

        LockZ();
        SetTextureStage(f.d3dDevice7,0x10u,2u);
        Map->DrawLayer(11);
        DrawGroundSnow();
        DrawSnow();
        SetTextureStage(f.d3dDevice7,0x10u,1u);
        Map->DrawLayer(12);
        DrawRain();
        UnLockZ();
        SetRenderState(0x0Eu,1u);
        SetTextureStage(f.d3dDevice7,0x10u,1u);
        Lock(); LockZ();
        Map->DrawLayer(13);
        UnLock(); UnLockZ();
        Map->DrawLayer(14);
        Lock(); LockZ();
        Map->DrawLayer(15);

        if (!Mouse->IsHardware()) {
            VID* const mouseVid=Mouse->Vid();
            if (mouseVid->PropAlwaysTop()) {
                for (SPRITE* s=Mouse;s;s=s->Link())
                    if (!s->IsInvisible()) DrawSpriteVirtual(s);
            }
        }
    }

    DrawSquall();
    if (squallShift)
        Map->SetShiftCoor(SizeX()*0.5f+oldX,SizeY()*0.5f+oldY,0);
    UnLock();
    if (f.movie.IsOpen() && f.movie.Update()) f.movie.Release();
    SetRenderState(0x1Du,0u);
    DrawEffect(draw);
    UnLock();
}

int GRAPH::GetEffectState(int effect)
{
    GraphAbiFields& fields=Fields(this);
    if(effect>0 && effect<16 && fields.effectStart[effect])
        return static_cast<int>((RealCurrentTime-fields.effectStart[effect])*100u/static_cast<unsigned int>(fields.effectDuration[effect]));
    return -1;
}


float GRAPH::WindSpeed()
{
    struct Layout { unsigned char pad[0xBBC]; float speed; };
    return reinterpret_cast<const Layout*>(this)->speed;
}

ANGLE GRAPH::WindDirection()
{
    struct Layout { unsigned char pad[0xBB8]; ANGLE direction; };
    return reinterpret_cast<const Layout*>(this)->direction;
}

int GRAPH::IsEnvironment(unsigned int environment)
{
    return static_cast<int>(Fields(this).environment & environment);
}


void GRAPH_CORE::PutsXY(float x,float y,const STRING* str,COLOR color)
{
    PutsXY(x,y,str ? str->m_buf : 0,color);
}


int GRAPH::DrawLoadBar(VID* draw_vid)
{
    static unsigned long loadTime=0;
    static int loadFrame=0;

    RealCurrentTime=timeGetTime();
    if (!draw_vid || RealCurrentTime-loadTime<=draw_vid->m_phaseRandomInterval)
        return 0;

    PreTact();
    ClearScreen(COLOR(0,0,0));
    DrawVid(draw_vid,loadFrame,SizeX()/2.0f,SizeY()/2.0f+1.0f,1.0f);
    DrawEffect(1);

    const STRING* const loadText=reinterpret_cast<const STRING*>(reinterpret_cast<const unsigned char*>(this)+0xC18u);
    PutsXY(200.0f,ViewYMin()+5.0f,loadText,GREEN);
    PostTact(1);

    loadTime=RealCurrentTime;
    ++loadFrame;
    if (loadFrame>=draw_vid->m_dotFrameCount)
        loadFrame=0;
    return 1;
}

void GRAPH_CORE::SetFont(const STRING* font_name,int width,int height)
{
    GRAPH* graph=reinterpret_cast<GRAPH*>(this);
    STRING* const font=reinterpret_cast<STRING*>(reinterpret_cast<unsigned char*>(graph)+0xBC0);
    *font=font_name;
    *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(graph)+0xBC4)=width;
    *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(graph)+0xBC8)=height;
}

void GRAPH::OldLoadParameters(STREAM* res)
{
    GraphAbiFields& fields=Fields(this);
    res->Read(&fields.environment,4u);
    unsigned long packed=0;
    res->Read(&packed,4u);
    GAMMA gamma(GAMMA::DECODE,packed);
    SetGamma(&gamma);

    // Legacy GRPH (MapEdit.exe 0x0042BAF7) stores direction as a single byte,
    // unlike the modern GRAPH::LoadParameters path which uses ANGLE::Read and
    // therefore consumes a DWORD.  Keep the legacy byte layout explicit.
    unsigned char directionByte=0;
    short force=0;
    res->Read(&directionByte,1u);
    res->Read(&force,2u);
    SetWind(force,ANGLE(directionByte));
}

void GRAPH::LoadParameters(STREAM* res)
{
    GraphAbiFields& fields=Fields(this);
    GAMMA gamma;
    res->Read(&fields.environment,4u);
    gamma.Read(res);
    SetGamma(&gamma);
    fields.windDirection.Read(res);
    res->Read(&fields.windForce,4u);
}

void __cdecl GRAPH::DrawDebugText(const char* str,...)
{
    STRING* const debugText=reinterpret_cast<STRING*>(reinterpret_cast<unsigned char*>(this)+0xC18);
    *debugText=str;
    PreTact();
    Bar(200.0f,ViewYMin()+5.0f,800.0f,ViewYMin()+30.0f,GRAPH::BLACK);
    PutsXY(200.0f,ViewYMin()+5.0f,debugText,GRAPH::GREEN);
    PostTact(1);
}

// Retail dynamic light raster.  This is the editor's original CPU light-map
// path: it samples the software Z buffer at a 4x4 cadence, writes a temporary
// light texture, then blends that texture back into the scene.
void GRAPH::DrawLightSource(float shiftx,float shifty,float shiftz,float size_x,float size_y,COLOR color)
{
    static int lightBufferToggle=0; // original global 0x004EE684
    lightBufferToggle^=1;

    if ((color.color&0x00FFFFFFu)==0)
        return;

    LockZ();
    GraphAbiFields& f=Fields(this);

    int sizex2=(static_cast<int>(size_x)/2)*3;
    sizex2&=~3;
    int sizey2=(static_cast<int>(size_y)/2)*3;
    sizey2&=~3;
    if (sizex2>0x200) sizex2=0x200;
    if (sizey2>0x200) sizey2=0x200;

    const int sx=static_cast<int>(size_x);
    const int sy=static_cast<int>(size_y);
    const int size=(sx*sy)/500;
    const int z=static_cast<int>(shiftz);
    const int z1=(z*2)/3+8;
    shifty-=(static_cast<float>(z1)-shiftz);

    if (!BoxInViewPort(shiftx-static_cast<float>(sizex2),
                       shifty-static_cast<float>(sizey2),
                       shiftx+static_cast<float>(sizex2),
                       shifty+static_cast<float>(sizey2)))
        return;

    const float zFloat=(shiftz+size_x+50.0f)*0.0001220703125f+0.015625f;

    RECT_OLD scrRect;
    scrRect.left=static_cast<long>(shiftx-static_cast<float>(sizex2));
    scrRect.top=static_cast<long>(shifty-static_cast<float>(sizey2));
    scrRect.right=static_cast<long>(shiftx+static_cast<float>(sizex2));
    scrRect.bottom=static_cast<long>(shifty+static_cast<float>(sizey2));

    RECT_OLD texRect;
    texRect.left=0;
    texRect.top=0;
    texRect.right=sizex2/2;
    texRect.bottom=sizey2/2;

    SURFACE* const buffer=lightBufferToggle ? f.surfaceBE4 : f.paletteSurface;
    int pitch=0;
    unsigned char* const light=buffer->Lock(&pitch,&texRect);
    if (!light) {
        GRAPH_CORE::Error(10,"light buffer",0);
        return;
    }

    const bool paletted=buffer->IsPaletted()!=0;
    if (!paletted)
        pitch/=2;

    const unsigned short* const ramp=reinterpret_cast<const unsigned short*>(
        reinterpret_cast<const unsigned char*>(this)+0x0C);

    for (int y=-sizey2;y<sizey2;y+=4) {
        for (int x=-sizex2;x<sizex2;x+=4) {
            int zz;
            const float sampleX=static_cast<float>(x)+shiftx;
            const float sampleY=static_cast<float>(y)+shifty;
            if (!InViewPort(sampleX,sampleY)) {
                zz=0x7FFF;
            } else {
                const int ix=x+static_cast<int>(shiftx);
                const int iy=y+static_cast<int>(shifty);
                const unsigned short raw=f.zBuffer[iy*f.zPitch+ix];
                zz=(static_cast<int>(raw)/8)-128;
            }

            int zz2;
            if (!InViewPort(sampleX+3.0f,sampleY+3.0f)) {
                zz2=0x7FFF;
            } else {
                const int ix=x+static_cast<int>(shiftx)+3;
                const int iy=y+static_cast<int>(shifty)+3;
                const unsigned short raw=f.zBuffer[iy*f.zPitch+ix];
                zz2=(static_cast<int>(raw)/8)-128;
            }
            if (zz2<zz)
                zz=zz2;
            if (zz==0x7FFF)
                continue;

            int d=y+zz-z1;
            const int dz=zz-z;
            d=(9*(d*d))/4;
            d+=(dz*dz)/4;
            d+=x*x;

            int scale=size ? 0x100-(d/size) : 0x100-d;
            if (scale<0) scale=0;
            if (scale>0xFF) scale=0xFF;

            const int outY=(y+sizey2)/4;
            const int outX=(x+sizex2)/4;
            if (!paletted)
                reinterpret_cast<unsigned short*>(light)[outY*pitch+outX]=ramp[scale];
            else
                light[outY*pitch+outX]=static_cast<unsigned char>(scale);
        }
    }

    buffer->UnLock();
    SetRenderState(0x1Du,0u);
    SetAlphaBlend(9u,2u);

    ++texRect.left;
    --texRect.right;
    ++texRect.top;
    --texRect.bottom;
    COLOR black(0,0,0);
    GAMMA gamma(color,black);
    reinterpret_cast<TEXTURE*>(buffer)->Draw(zFloat,zFloat,&scrRect,&texRect,&gamma);
}

HWND__* GRAPH_CORE::Wnd()
{
    return Fields(reinterpret_cast<GRAPH*>(this)).hwnd;
}

void GRAPH::PlayMovie(const STRING* file)
{
    GraphAbiFields& fields=Fields(this);
    fields.movie.Open(file,(int)fields.sizeX/2,(int)fields.sizeY/2);
}

int GRAPH::IsPlayMovie()
{
    return Fields(this).movie.IsOpen();
}
