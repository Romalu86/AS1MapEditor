#include "mapedit/runtime.hpp"

const unsigned int CRC32::crc_table[256] = {
    0x00000000u,
    0x77073096u,
    0xEE0E612Cu,
    0x990951BAu,
    0x076DC419u,
    0x706AF48Fu,
    0xE963A535u,
    0x9E6495A3u,
    0x0EDB8832u,
    0x79DCB8A4u,
    0xE0D5E91Eu,
    0x97D2D988u,
    0x09B64C2Bu,
    0x7EB17CBDu,
    0xE7B82D07u,
    0x90BF1D91u,
    0x1DB71064u,
    0x6AB020F2u,
    0xF3B97148u,
    0x84BE41DEu,
    0x1ADAD47Du,
    0x6DDDE4EBu,
    0xF4D4B551u,
    0x83D385C7u,
    0x136C9856u,
    0x646BA8C0u,
    0xFD62F97Au,
    0x8A65C9ECu,
    0x14015C4Fu,
    0x63066CD9u,
    0xFA0F3D63u,
    0x8D080DF5u,
    0x3B6E20C8u,
    0x4C69105Eu,
    0xD56041E4u,
    0xA2677172u,
    0x3C03E4D1u,
    0x4B04D447u,
    0xD20D85FDu,
    0xA50AB56Bu,
    0x35B5A8FAu,
    0x42B2986Cu,
    0xDBBBC9D6u,
    0xACBCF940u,
    0x32D86CE3u,
    0x45DF5C75u,
    0xDCD60DCFu,
    0xABD13D59u,
    0x26D930ACu,
    0x51DE003Au,
    0xC8D75180u,
    0xBFD06116u,
    0x21B4F4B5u,
    0x56B3C423u,
    0xCFBA9599u,
    0xB8BDA50Fu,
    0x2802B89Eu,
    0x5F058808u,
    0xC60CD9B2u,
    0xB10BE924u,
    0x2F6F7C87u,
    0x58684C11u,
    0xC1611DABu,
    0xB6662D3Du,
    0x76DC4190u,
    0x01DB7106u,
    0x98D220BCu,
    0xEFD5102Au,
    0x71B18589u,
    0x06B6B51Fu,
    0x9FBFE4A5u,
    0xE8B8D433u,
    0x7807C9A2u,
    0x0F00F934u,
    0x9609A88Eu,
    0xE10E9818u,
    0x7F6A0DBBu,
    0x086D3D2Du,
    0x91646C97u,
    0xE6635C01u,
    0x6B6B51F4u,
    0x1C6C6162u,
    0x856530D8u,
    0xF262004Eu,
    0x6C0695EDu,
    0x1B01A57Bu,
    0x8208F4C1u,
    0xF50FC457u,
    0x65B0D9C6u,
    0x12B7E950u,
    0x8BBEB8EAu,
    0xFCB9887Cu,
    0x62DD1DDFu,
    0x15DA2D49u,
    0x8CD37CF3u,
    0xFBD44C65u,
    0x4DB26158u,
    0x3AB551CEu,
    0xA3BC0074u,
    0xD4BB30E2u,
    0x4ADFA541u,
    0x3DD895D7u,
    0xA4D1C46Du,
    0xD3D6F4FBu,
    0x4369E96Au,
    0x346ED9FCu,
    0xAD678846u,
    0xDA60B8D0u,
    0x44042D73u,
    0x33031DE5u,
    0xAA0A4C5Fu,
    0xDD0D7CC9u,
    0x5005713Cu,
    0x270241AAu,
    0xBE0B1010u,
    0xC90C2086u,
    0x5768B525u,
    0x206F85B3u,
    0xB966D409u,
    0xCE61E49Fu,
    0x5EDEF90Eu,
    0x29D9C998u,
    0xB0D09822u,
    0xC7D7A8B4u,
    0x59B33D17u,
    0x2EB40D81u,
    0xB7BD5C3Bu,
    0xC0BA6CADu,
    0xEDB88320u,
    0x9ABFB3B6u,
    0x03B6E20Cu,
    0x74B1D29Au,
    0xEAD54739u,
    0x9DD277AFu,
    0x04DB2615u,
    0x73DC1683u,
    0xE3630B12u,
    0x94643B84u,
    0x0D6D6A3Eu,
    0x7A6A5AA8u,
    0xE40ECF0Bu,
    0x9309FF9Du,
    0x0A00AE27u,
    0x7D079EB1u,
    0xF00F9344u,
    0x8708A3D2u,
    0x1E01F268u,
    0x6906C2FEu,
    0xF762575Du,
    0x806567CBu,
    0x196C3671u,
    0x6E6B06E7u,
    0xFED41B76u,
    0x89D32BE0u,
    0x10DA7A5Au,
    0x67DD4ACCu,
    0xF9B9DF6Fu,
    0x8EBEEFF9u,
    0x17B7BE43u,
    0x60B08ED5u,
    0xD6D6A3E8u,
    0xA1D1937Eu,
    0x38D8C2C4u,
    0x4FDFF252u,
    0xD1BB67F1u,
    0xA6BC5767u,
    0x3FB506DDu,
    0x48B2364Bu,
    0xD80D2BDAu,
    0xAF0A1B4Cu,
    0x36034AF6u,
    0x41047A60u,
    0xDF60EFC3u,
    0xA867DF55u,
    0x316E8EEFu,
    0x4669BE79u,
    0xCB61B38Cu,
    0xBC66831Au,
    0x256FD2A0u,
    0x5268E236u,
    0xCC0C7795u,
    0xBB0B4703u,
    0x220216B9u,
    0x5505262Fu,
    0xC5BA3BBEu,
    0xB2BD0B28u,
    0x2BB45A92u,
    0x5CB36A04u,
    0xC2D7FFA7u,
    0xB5D0CF31u,
    0x2CD99E8Bu,
    0x5BDEAE1Du,
    0x9B64C2B0u,
    0xEC63F226u,
    0x756AA39Cu,
    0x026D930Au,
    0x9C0906A9u,
    0xEB0E363Fu,
    0x72076785u,
    0x05005713u,
    0x95BF4A82u,
    0xE2B87A14u,
    0x7BB12BAEu,
    0x0CB61B38u,
    0x92D28E9Bu,
    0xE5D5BE0Du,
    0x7CDCEFB7u,
    0x0BDBDF21u,
    0x86D3D2D4u,
    0xF1D4E242u,
    0x68DDB3F8u,
    0x1FDA836Eu,
    0x81BE16CDu,
    0xF6B9265Bu,
    0x6FB077E1u,
    0x18B74777u,
    0x88085AE6u,
    0xFF0F6A70u,
    0x66063BCAu,
    0x11010B5Cu,
    0x8F659EFFu,
    0xF862AE69u,
    0x616BFFD3u,
    0x166CCF45u,
    0xA00AE278u,
    0xD70DD2EEu,
    0x4E048354u,
    0x3903B3C2u,
    0xA7672661u,
    0xD06016F7u,
    0x4969474Du,
    0x3E6E77DBu,
    0xAED16A4Au,
    0xD9D65ADCu,
    0x40DF0B66u,
    0x37D83BF0u,
    0xA9BCAE53u,
    0xDEBB9EC5u,
    0x47B2CF7Fu,
    0x30B5FFE9u,
    0xBDBDF21Cu,
    0xCABAC28Au,
    0x53B39330u,
    0x24B4A3A6u,
    0xBAD03605u,
    0xCDD70693u,
    0x54DE5729u,
    0x23D967BFu,
    0xB3667A2Eu,
    0xC4614AB8u,
    0x5D681B02u,
    0x2A6F2B94u,
    0xB40BBE37u,
    0xC30C8EA1u,
    0x5A05DF1Bu,
    0x2D02EF8Du
};

CRC32::CRC32() : crc(0) {}

CRC32::CRC32(void* buf,unsigned int count) : crc(0) { Add(buf,count); }

CRC32::operator unsigned int() { return crc; }

unsigned int CRC32::Add(void* buf,unsigned int count)
{
    unsigned char* pos=static_cast<unsigned char*>(buf);
    unsigned char* end=pos+count;
    if (!pos) { crc=0; return 0; }
    while (pos<end) {
        crc=((crc>>8)&0x00FFFFFFu)^crc_table[(crc^*pos)&0xFFu];
        ++pos;
    }
    return crc;
}


void* __cdecl operator new(unsigned int, void* place) { return place; }

namespace {
struct PictureBaseLayout {
    uint32_t vptr;
    int noFrame;             // +0x004
    int curFrame;            // +0x008
    int frameTime;           // +0x00C
    int sizeX;               // +0x010
    int sizeY;               // +0x014
    int bytesPerPixel;       // +0x018
    STRING filename;         // +0x01C
    FILE* file;              // +0x020
    COLOR palette[256];      // +0x024
    void* data;              // +0x424
    long dataOffset;         // +0x428
};
static_assert(sizeof(PictureBaseLayout)==0x42C, "PICTURE_BASE ABI");

PictureBaseLayout& Fields(PICTURE_BASE* picture)
{
    return *reinterpret_cast<PictureBaseLayout*>(picture);
}
 }

PICTURE_BASE::PICTURE_BASE()
{
    PictureBaseLayout& f=Fields(this);
    f.noFrame=0;
    f.curFrame=0;
    f.bytesPerPixel=0;
    f.frameTime=0x47;
    f.sizeX=0;
    f.sizeY=0;
    f.filename.m_buf=STRING::EMPTY;
    f.file=0;
    f.data=0;
    // Retail does not initialize +0x428 here.
}

PICTURE_BASE::PICTURE_BASE(int size_x,int size_y,int bytes_per_pixel)
{
    PictureBaseLayout& f=Fields(this);
    f.noFrame=0;
    f.curFrame=0;
    f.bytesPerPixel=0;
    f.frameTime=0x47;
    f.sizeX=0;
    f.sizeY=0;
    f.filename.m_buf=STRING::EMPTY;
    f.file=0;
    f.data=0;
    f.noFrame=1;
    SetSize(size_x,size_y,bytes_per_pixel);
}

void PICTURE_BASE::SetSize(int size_x,int size_y,int bytes_per_pixel)
{
    PictureBaseLayout& f=Fields(this);
    f.sizeX=size_x;
    f.sizeY=size_y;
    f.bytesPerPixel=bytes_per_pixel;
    if (f.data)
        ::operator delete(f.data);
    const unsigned int bytes=static_cast<unsigned int>(size_x*size_y*bytes_per_pixel);
    f.data=::operator new(bytes);
    if (f.data)
        memset(f.data,0,bytes);
    else
        MYERROR::Error(::Error,"PICTURE '%s'",2,"picture buffer",static_cast<int>(bytes),f.filename.CharPtr());
}

void PICTURE_BASE::Close()
{
    PictureBaseLayout& f=Fields(this);
    if (f.data) {
        ::operator delete(f.data);
        f.data=0;
    }
    if (f.file) {
        fclose(f.file);
        f.file=0;
    }
    f.noFrame=0;
    f.sizeX=0;
    f.sizeY=0;
}

PICTURE_BASE::~PICTURE_BASE()
{
    Close();
    Fields(this).filename.~STRING();
}

PICTURE::PICTURE(int size_x,int size_y,PICTURE_TYPE create_type)
    : picture(0), type(static_cast<int>(create_type))
{
    if (type>0 && type<=2)
        picture=new PICTURE_BASE(size_x,size_y,3);
    else if (type==5)
        picture=new PICTURE_BASE(size_x,size_y,2);
}

PICTURE::PICTURE() : picture(new PICTURE_BASE()), type(0) {}

void PICTURE_BASE::PutData(int x,int y,unsigned int value)
{
    PictureBaseLayout& f=Fields(this);
    if (x<0 || y<0 || x>=f.sizeX || y>=f.sizeY || !f.data)
        return;
    const unsigned int index=static_cast<unsigned int>(x+y*f.sizeX);
    unsigned char* data=static_cast<unsigned char*>(f.data);
    switch (f.bytesPerPixel) {
    case 4: reinterpret_cast<unsigned int*>(data)[index]=value; break;
    case 3: {
        unsigned char* pixel=data+index*3u;
        pixel[0]=static_cast<unsigned char>(value);
        pixel[1]=static_cast<unsigned char>(value>>8);
        pixel[2]=static_cast<unsigned char>(value>>16);
        break;
    }
    case 2: reinterpret_cast<unsigned short*>(data)[index]=static_cast<unsigned short>(value); break;
    case 1: data[index]=static_cast<unsigned char>(value); break;
    default: break;
    }
}

void PICTURE_BASE::PutPixel(int x,int y,COLOR value)
{
    PictureBaseLayout& f=Fields(this);
    if (x<0 || y<0 || x>=f.sizeX || y>=f.sizeY || !f.data)
        return;
    const unsigned int index=static_cast<unsigned int>(x+y*f.sizeX);
    unsigned char* data=static_cast<unsigned char*>(f.data);
    switch (f.bytesPerPixel) {
    case 4: reinterpret_cast<unsigned int*>(data)[index]=value.color; break;
    case 3: {
        unsigned char* pixel=data+index*3u;
        pixel[0]=static_cast<unsigned char>(value.color);
        pixel[1]=static_cast<unsigned char>(value.color>>8);
        pixel[2]=static_cast<unsigned char>(value.color>>16);
        break;
    }
    case 2:
        reinterpret_cast<unsigned short*>(data)[index]=static_cast<unsigned short>(
            ((value.color>>9)&0x7C00u)|((value.color>>6)&0x03E0u)|((value.color>>3)&0x001Fu));
        break;
    case 1: data[index]=static_cast<unsigned char>(value.color); break;
    default: break;
    }
}

void PICTURE::PutPixel(int x,int y,COLOR value) { picture->PutPixel(x,y,value); }

void PICTURE::PutData(int x,int y,unsigned int value) { picture->PutData(x,y,value); }

void PICTURE_MAKEVID::PutPixel(int x,int y,COLOR value)
{
    texture.PutPixel(x,y,value);
}

void PICTURE_MAKEVID::PutPixelZ(int x,int y,int z)
{
    zBuffer.PutData(x,y,static_cast<unsigned int>(z-0x400));
}

// Exact default converter layout used by MAP::CreateVid.
PICTURE_MAKEVID::PICTURE_MAKEVID()
    : texture(), alpha(), zBuffer(), paletteDecode(0), vidType(0)
{
}

PICTURE_MAKEVID::PICTURE_MAKEVID(int size_x,int size_y,unsigned long create_layer)
    : texture(size_x,size_y,PICTURE::TYPE_TGA),
      alpha(),
      zBuffer(size_x,size_y,PICTURE::TYPE_Z),
      paletteDecode(0),
      vidType(0)
{
    if (create_layer&2u) vidType|=2u;
    if (create_layer&4u) vidType|=4u;
    if (create_layer&1u) vidType|=1u;
}

void PICTURE::Close()
{
    // Retail dereferences picture directly and dispatches vtable slot +0x10;
    // there is no null/safety guard on this failure path.
    void** vtable=*reinterpret_cast<void***>(picture);
    typedef void (__thiscall *CloseMethod)(PICTURE_BASE*);
    reinterpret_cast<CloseMethod>(vtable[0x10/4])(picture);
}

int PICTURE_MAKEVID::Load(STRING file,STRING alphafile,STRING zfile)
{
    if (paletteDecode)
        operator delete(paletteDecode);
    paletteDecode=0;

    if (strcmp(alphafile.m_buf,STRING::EMPTY)) vidType|=2u;
    if (strcmp(zfile.m_buf,STRING::EMPTY)) vidType|=4u;
    if (strcmp(file.m_buf,STRING::EMPTY)) {
        vidType|=1u;
    } else {
        file=alphafile;
        alphafile=STRING::EMPTY;
    }

    int anyOk=(texture.Load(&file)==0);
    anyOk|=(alpha.Load(&alphafile)==0);
    zBuffer.Load(&zfile);

    if (texture.BytesPerPixel()==4)
        vidType|=2u;
    if (texture.SizeX()==2 && texture.SizeY()==2)
        vidType=0x80u;
    return anyOk==0;
}

void PICTURE_MAKEVID::Close()
{
    texture.Close();
    alpha.Close();
    zBuffer.Close();
}

PICTURE::~PICTURE()
{
    delete picture;
    picture=0;
}

int PICTURE::IsZ() { return type==5; }
int PICTURE::SizeX() { return picture->SizeX(); }
int PICTURE::SizeY() { return picture->SizeY(); }
COLOR PICTURE::GetPixel(int x,int y) { return picture->GetPixel(x,y); }
unsigned int PICTURE::GetData(int x,int y) { return picture->GetData(x,y); }
void PICTURE::SaveTGA(const STRING* filename,int x,int y,int sizeX,int sizeY)
{
    picture->SaveTGA(filename,x,y,sizeX,sizeY);
}

int PICTURE_BASE::SizeX() { return Fields(this).sizeX; }
int PICTURE_BASE::SizeY() { return Fields(this).sizeY; }

// debug metadata layout; the compiler emits zBuffer/alpha/texture destruction after
// this body, matching the original member-dtor order.
PICTURE_MAKEVID::~PICTURE_MAKEVID()
{
    if (paletteDecode) {
        operator delete(paletteDecode);
        paletteDecode=0;
    }
}

int PICTURE_MAKEVID::CalcCRC32()
{
    CRC32 crc;
    for (int x=0;x<SizeX();++x) {
        for (int y=0;y<SizeY();++y) {
            unsigned int pixel;
            if (vidType&4u) {
                pixel=static_cast<unsigned short>(GetPixelZ(x,y));
                crc.Add(&pixel,2);
            }
            const unsigned int flags=vidType;
            if (flags&8u) {
                vidType=flags&~8u;
                pixel=GetPixelT(x,y);
                vidType|=8u;
            } else {
                pixel=GetPixelT(x,y);
            }
            crc.Add(&pixel,2);
        }
    }
    return static_cast<int>(static_cast<unsigned int>(crc));
}

void PICTURE_MAKEVID::Rewind()
{
    texture.Rewind();
    alpha.Rewind();
    zBuffer.Rewind();
}

void PICTURE_MAKEVID::NextFrame()
{
    texture.NextFrame();
    alpha.NextFrame();
    zBuffer.NextFrame();
}

COLOR PICTURE_MAKEVID::GetPixel(int x,int y)
{
    if (!InViewPort(x,y))
        return COLOR(0,0,0);

    if (alpha.IsOpened()) {
        // The retail body performs three independent texture reads before
        // composing A/R/G/B; keep that ordering instead of folding them.
        COLOR blue=texture.GetPixel(x,y);
        COLOR green=texture.GetPixel(x,y);
        COLOR red=texture.GetPixel(x,y);
        COLOR a=alpha.GetPixel(x,y);
        return COLOR(static_cast<int>(a.Blue()),
                     static_cast<int>(red.Red()),
                     static_cast<int>(green.Green()),
                     static_cast<int>(blue.Blue()));
    }
    return texture.GetPixel(x,y);
}

unsigned int PICTURE_MAKEVID::GetAlpha(int x,int y)
{
    if (!InViewPort(x,y)) return 0;
    if (alpha.IsOpened()) return alpha.GetPixel(x,y).Blue();
    if (texture.BytesPerPixel()==4) return texture.GetData(x,y)>>24;
    return 255;
}

short PICTURE_MAKEVID::GetPixelZ(int x,int y)
{
    if (!InViewPort(x,y)) return 0x400;
    short z=0;
    if (zBuffer.IsOpened() && zBuffer.IsZ())
        z=static_cast<short>(zBuffer.GetData(x,y));
    if (z==static_cast<short>(0x8000)) return 0;
    return static_cast<short>(z+0x400);
}

short PICTURE_MAKEVID::GetPixelZInRect(int x,int y,int width,int height)
{
    if (IsPixel(x+width-1,y+height-1))
        return GetPixelZ(x+width-1,y+height-1);

    const int begX=x;
    width+=x;
    height+=y;
    for (;y<height;++y) {
        for (x=begX;x<width;++x) {
            if (IsPixel(x,y))
                return GetPixelZ(x,y);
        }
    }
    return 0;
}

int PICTURE_MAKEVID::GetPaletteNumber(COLOR color)
{
    if (paletteDecode)
        return paletteDecode[color.RGB565()*16u+(color.Alpha()>>4)];
    return color.NearestInPalette(commonPalette,256);
}

void PICTURE_MAKEVID::SetPaletteDecodeNumber(COLOR color,unsigned char number)
{
    if (!paletteDecode) return;
    const unsigned int c=color.color;
    unsigned char* slot=paletteDecode+16u*((((c&0xF8u)|(((c&0xFC00u)|((c>>3)&0x1F0000u))>>2))>>3))+(c>>28);
    const unsigned char old=*slot;
    if (!old || old==number) {
        *slot=number;
        return;
    }
    for (unsigned int i=0;i<0x100000u;++i)
        if (paletteDecode[i]==old) paletteDecode[i]=number;
}

unsigned long PICTURE_MAKEVID::GetPixelT(int x,int y)
{
    if (!InViewPort(x,y))
        return 0;

    if (vidType&0x1000u) {
        if (vidType&0x8u)
            return texture.GetData(x,y);

        COLOR argb=GetPixel(x,y);
        int a=static_cast<int>(argb.Alpha());
        int r=static_cast<int>(argb.Red());
        int g=static_cast<int>(argb.Green());
        int b=static_cast<int>(argb.Blue());
        if (!IsPixel(x,y))
            a=0;
        if ((vidType&1u) && (vidType&2u)) {
            // Retail has no zero-alpha guard here (0x44B875..0x44B8A2).
            r=255*r/a;
            g=255*g/a;
            b=255*b/a;
            if (r>255) r=255;
            if (g>255) g=255;
            if (b>255) b=255;
            return static_cast<unsigned long>((a<<24)|(r<<16)|(g<<8)|b);
        }
        return static_cast<unsigned long>(((a>>7)<<15)|((r>>3)<<10)|((g>>3)<<5)|(b>>3));
    }

    if (!(vidType&0x8u)) {
        COLOR argb=GetPixel(x,y);
        unsigned char a=static_cast<unsigned char>(argb.Alpha());
        unsigned char r=static_cast<unsigned char>(argb.Red());
        unsigned char g=static_cast<unsigned char>(argb.Green());
        unsigned char b=static_cast<unsigned char>(argb.Blue());
        if ((vidType&1u) && (vidType&2u)) {
            if ((a>>4)!=0) {
                r=static_cast<unsigned char>((255*static_cast<int>(r)/static_cast<int>(a))>>4);
                g=static_cast<unsigned char>((255*static_cast<int>(g)/static_cast<int>(a))>>4);
                b=static_cast<unsigned char>((255*static_cast<int>(b)/static_cast<int>(a))>>4);
                if (r>15) r=15;
                if (g>15) g=15;
                if (b>15) b=15;
                return static_cast<unsigned long>(((a>>4)<<12)|(r<<8)|(g<<4)|b);
            }
            return 0;
        }
        if ((vidType&4u) && (vidType&2u))
            return static_cast<unsigned long>((r>>4)<<8 | (g>>4)<<4 | (b>>4));
        return static_cast<unsigned long>((r>>3)<<11 | (g>>2)<<5 | (b>>3));
    }

    if (vidType&2u)
        return static_cast<unsigned long>(GetPaletteNumber(GetPixel(x,y)));
    return texture.GetData(x,y);
}

void PICTURE_MAKEVID::GetRectangle(int* begx,int* begy,int* endx,int* endy)
{
    if (begy && endy) {
        int y=0;
        *begy=-1;
        while (y<SizeY() && *begy==-1) {
            for (int x=0;x<SizeX();++x)
                if (IsPixel(x,y)) *begy=y;
            ++y;
        }
        y=SizeY()-1;
        *endy=-1;
        while (y>=0 && *endy==-1) {
            for (int x=0;x<SizeX();++x)
                if (IsPixel(x,y)) *endy=y;
            --y;
        }
        if (*endy>=0) ++*endy;
        if (*endy<=*begy || *begy<0 || *endy<0) {
            *begy=0;
            *endy=0;
        }
    }

    if (begx && endx) {
        int x=0;
        *begx=-1;
        while (x<SizeX() && *begx==-1) {
            for (int y=0;y<SizeY();++y)
                if (IsPixel(x,y)) *begx=x;
            ++x;
        }
        x=SizeX()-1;
        *endx=-1;
        while (x>=0 && *endx==-1) {
            for (int y=0;y<SizeY();++y)
                if (IsPixel(x,y)) *endx=x;
            --x;
        }
        if (*endx>=0) ++*endx;
        if (*endx<=*begx || *begx<0 || *endx<0) {
            // This odd retail write is intentional and confirmed in the reference implementation
            // at 0x44BDAA..0x44BDB6.
            if (begy) *begy=0;
            if (endy) *endy=0;
        }
    }
}

int PICTURE_MAKEVID::IsPixel(int x,int y)
{
    if (!InViewPort(x,y))
        return 0;

    if (zBuffer.IsOpened() && zBuffer.IsZ()) {
        if (static_cast<short>(zBuffer.GetData(x,y))==static_cast<short>(0x8000))
            return 0;
        if ((vidType&0x20u) && (vidType&2u) && (vidType&4u)) {
            if (vidType&1u)
                return static_cast<int>(GetAlpha(x,y)>>4);
            return static_cast<int>(GetPixelT(x,y)&0xFFFu);
        }
        return 1;
    }

    if ((vidType&2u) && (vidType&1u))
        return static_cast<int>(GetAlpha(x,y)>>4);
    return static_cast<int>(GetPixel(x,y).color&0x00FFFFFFu);
}

int PICTURE_MAKEVID::IsPixelInRect(int x,int y,int width,int height)
{
    const int begX=x;
    width+=x;
    height+=y;
    for (;y<height;++y) {
        for (x=begX;x<width;++x)
            if (IsPixel(x,y)) return 1;
    }
    return 0;
}


namespace {
template <class T>
void PutPacked(void* dst,const T& value)
{
    memcpy(dst,&value,static_cast<unsigned int>(sizeof(T)));
}
}

// This preserves both the direct 8/16-bit copy path and the retail
// DirectDraw DXT1/DXT3 conversion path. The latter intentionally uses raw
// COM vtable calls because the original binary does so and this keeps the
// DirectDraw ABI independent of modern SDK wrapper declarations.
int PICTURE_MAKEVID::WriteSurfaces(unsigned char* outbuf,unsigned short* buf,int surf_sizex,int surf_sizey)
{
    int outPos=0;

    if ((vidType&8u) || !(vidType&0x800u)) {
        for (int y=0;y<surf_sizey;++y) {
            unsigned short* const srcRow=buf+(y<<8);
            if (vidType&8u) {
                for (int x=0;x<surf_sizex;++x)
                    outbuf[outPos++]=static_cast<unsigned char>(srcRow[x]);
            } else {
                const unsigned int bytes=static_cast<unsigned int>(surf_sizex*2);
                memcpy(outbuf+outPos,srcRow,bytes);
                outPos+=static_cast<int>(bytes);
            }
        }
        return outPos;
    }

    unsigned char desc[0x6c];
    memset(desc,0,sizeof(desc));
    *reinterpret_cast<unsigned int*>(desc+0x00)=0x6cu;
    *reinterpret_cast<unsigned int*>(desc+0x04)=0x1007u;
    if (!(vidType&4u) && !(vidType&2u))
        *reinterpret_cast<unsigned int*>(desc+0x04)|=0x10000u;
    *reinterpret_cast<int*>(desc+0x0c)=surf_sizex;
    *reinterpret_cast<int*>(desc+0x08)=surf_sizey;
    *reinterpret_cast<unsigned int*>(desc+0x68)=0x840u;

    DDPIXELFORMAT_OLD* const pf=reinterpret_cast<DDPIXELFORMAT_OLD*>(desc+0x48);
    pf->dwSize=0x20u;
    pf->dwFlags=0x40u;
    pf->dwRGBBitCount=16u;
    if ((vidType&2u) && (vidType&1u)) {
        pf->dwFlags|=1u;
        pf->dwRBitMask=0x0f00u;
        pf->dwGBitMask=0x00f0u;
        pf->dwBBitMask=0x000fu;
        pf->dwRGBAlphaBitMask=0xf000u;
    } else {
        pf->dwRBitMask=0xf800u;
        pf->dwGBitMask=0x07e0u;
        pf->dwBBitMask=0x001fu;
    }

    void* dd=Graph->DDraw();
    void** const ddvt=*reinterpret_cast<void***>(dd);
    typedef long (__stdcall *CreateSurfaceFn)(void*,void*,void**,void*);
    CreateSurfaceFn const createSurface=reinterpret_cast<CreateSurfaceFn>(ddvt[0x18/4]);

    void* srcSurface=0;
    long hr=createSurface(dd,desc,&srcSurface,0);
    if (hr) {
        Error(E_CREATE,const_cast<char*>("DD surface"),0);
        return 0;
    }

    *reinterpret_cast<unsigned int*>(desc+0x04)=0x1007u;
    *reinterpret_cast<unsigned int*>(desc+0x68)=0x1800u;
    const DDPIXELFORMAT_OLD dxt=((vidType&2u) && (vidType&1u))
        ? DDPIXELFORMAT_OLD{0x20u,0x04u,0x33545844u,0,0,0,0,0}
        : DDPIXELFORMAT_OLD{0x20u,0x04u,0x31545844u,0,0,0,0,0};
    memcpy(desc+0x48,&dxt,sizeof(dxt));

    void* dxtSurface=0;
    hr=createSurface(dd,desc,&dxtSurface,0);
    if (hr) {
        Error(E_CREATE,const_cast<char*>("DXT surface"),0);
        return 0;
    }

    typedef long (__stdcall *LockFn)(void*,void*,void*,unsigned long,void*);
    typedef long (__stdcall *UnlockFn)(void*,void*);
    typedef long (__stdcall *BltFastFn)(void*,unsigned long,unsigned long,void*,void*,unsigned long);
    typedef unsigned long (__stdcall *ReleaseFn)(void*);

    void** const srcvt=*reinterpret_cast<void***>(srcSurface);
    hr=reinterpret_cast<LockFn>(srcvt[0x64/4])(srcSurface,0,desc,1u,0);
    if (hr) {
        Error(E_LOCK,const_cast<char*>("surface"),0);
        return 0;
    }

    unsigned char* const locked=static_cast<unsigned char*>(*reinterpret_cast<void**>(desc+0x24));
    for (int y=0;y<surf_sizey;++y) {
        memcpy(locked+2*surf_sizex*y,buf+(y<<8),static_cast<unsigned int>(2*surf_sizex));
    }
    reinterpret_cast<UnlockFn>(srcvt[0x80/4])(srcSurface,0);

    void** const dxtvt=*reinterpret_cast<void***>(dxtSurface);
    hr=reinterpret_cast<BltFastFn>(dxtvt[0x1c/4])(dxtSurface,0,0,srcSurface,0,0x10u);
    if (hr) {
        Error(E_COPY,const_cast<char*>("surface"),0);
        return 0;
    }

    hr=reinterpret_cast<LockFn>(dxtvt[0x64/4])(dxtSurface,0,desc,1u,0);
    if (hr) {
        Error(E_LOCK,const_cast<char*>("DXT surface"),0);
        return 0;
    }
    if (*reinterpret_cast<unsigned int*>(desc+0x04)&0x80000u) {
        const unsigned int linearSize=*reinterpret_cast<unsigned int*>(desc+0x10);
        memcpy(outbuf,*reinterpret_cast<void**>(desc+0x24),linearSize);
        outPos+=static_cast<int>(linearSize);
    } else {
        Error(E_ERROR,const_cast<char*>("not DDSD_LINEARSIZE"),0);
    }
    reinterpret_cast<UnlockFn>(dxtvt[0x80/4])(dxtSurface,0);

    reinterpret_cast<ReleaseFn>(dxtvt[0x08/4])(dxtSurface);
    reinterpret_cast<ReleaseFn>(srcvt[0x08/4])(srcSurface);
    return outPos;
}


int __cdecl SortCallBack(const void* arg1,const void* arg2)
{
    const VID_TEXCOOR* const first=*static_cast<VID_TEXCOOR* const*>(arg1);
    const VID_TEXCOOR* const second=*static_cast<VID_TEXCOOR* const*>(arg2);
    if (Max(first->sizex,second->sizex)>Max(first->sizey,second->sizey))
        return second->sizex-first->sizex;
    return second->sizey-first->sizey;
}

// Hardware VID writer: frame CRC folding, 256x256 atlas placement, SURF/DATA
// section emission and optional QS1 packing.  The ordering, page numbering and
// duplicate-frame replacement follow the original x86 routine.
void PICTURE_MAKEVID::WriteHardware(RESOURCE* res)
{
    QS1_CODER* colorCoder=0;
    QS1_CODER* zCoder=0;
    if (vidType&0x100u) {
        colorCoder=new QS1_CODER((vidType&0x800u)?1:2);
        zCoder=new QS1_CODER(2);
    }

    const int frames=NoFrame();
    const int maxCoor=frames*(SizeX()/256+1)*(SizeY()/256+1)+1;
    unsigned int* const control=static_cast<unsigned int*>(::operator new(static_cast<unsigned int>(frames*4)));
    if (!control) {
        Error(E_MEMORY,const_cast<char*>("cntrl"),0);
        exit(1);
    }
    unsigned char* const shadow=static_cast<unsigned char*>(::operator new(0x200000u));
    if (!shadow) {
        Error(E_MEMORY,const_cast<char*>("shadow"),0);
        exit(1);
    }
    VID_TEXCOOR* const coor=static_cast<VID_TEXCOOR*>(
        ::operator new(static_cast<unsigned int>((maxCoor+0x384)*sizeof(VID_TEXCOOR))));
    if (!coor) {
        Error(E_MEMORY,const_cast<char*>("texcoor"),static_cast<unsigned long>(maxCoor+0x384));
        exit(1);
    }

    int noCoor=frames;
    for (int frame=0;frame<frames;++frame) {
        VID_TEXCOOR* const head=coor+frame;
        control[frame]=static_cast<unsigned int>(CalcCRC32());
        int duplicate=0;
        for (;duplicate<frame;++duplicate) {
            if (control[duplicate]==control[frame]) {
                head->nsurf=-1;
                head->sizex=duplicate;
                break;
            }
        }
        if (duplicate>=frame) {
            head->nsurf=0;
            int left,top,right,bottom;
            GetRectangle(&left,&top,&right,&bottom);
            if (right-left<=256 && bottom-top<=256) {
                head->SetCoor(left,top,right,bottom,0);
            } else {
                int first=1;
                for (int x=left;x<right;x+=256) {
                    for (int y=top;y<bottom;y+=256) {
                        VID_TEXCOOR* cell;
                        if (first) {
                            cell=head;
                            cell->SetCoor(x,y,right,bottom,noCoor);
                            first=0;
                        } else {
                            cell=coor+noCoor;
                            cell->SetCoor(x,y,right,bottom,noCoor+1);
                            ++noCoor;
                        }
                    }
                }
                if (noCoor>0)
                    coor[noCoor-1].next_fragment=0;
            }
        }
        NextFrame();
    }
    ::operator delete(control);

    VID_TEXCOOR** const indices=static_cast<VID_TEXCOOR**>(
        ::operator new(static_cast<unsigned int>(noCoor*sizeof(VID_TEXCOOR*))));
    if (!indices) {
        Error(E_MEMORY,const_cast<char*>("indices for sort"),static_cast<unsigned long>(noCoor));
        exit(1);
    }
    for (int i=0;i<noCoor;++i)
        indices[i]=coor+i;
    qsort(indices,static_cast<size_t>(noCoor),sizeof(VID_TEXCOOR*),SortCallBack);

    int pos=0;
    int pages=0;
    for (int sortedIndex=0;sortedIndex<noCoor;++sortedIndex) {
        VID_TEXCOOR* const cell=indices[sortedIndex];
        if (cell->nsurf<0 || !cell->sizey)
            continue;
        if (!sortedIndex || cell->sizex<indices[sortedIndex-1]->sizex || cell->sizey<indices[sortedIndex-1]->sizey)
            pos=0;

        int x=0;
        int placed=0;
        while (pos<=(maxCoor<<8)-cell->sizey) {
            if (pos>(pos&~255)+256-cell->sizey)
                pos=(pos&~255)+256;
            x=0;
            while (x<=256-cell->sizex) {
                placed=1;
                for (int previous=sortedIndex-1;previous>=0;--previous) {
                    VID_TEXCOOR* const other=indices[previous];
                    if (other->nsurf<0)
                        continue;
                    if (other->Intersection(x,pos,cell->sizex,cell->sizey)) {
                        placed=0;
                        // Retail stores end-1 then performs the common x increment.
                        // This is equivalent to beginning the next probe at end.
                        x=other->begx+other->sizex;
                        break;
                    }
                }
                if (placed)
                    break;
            }
            if (placed)
                break;
            ++pos;
        }
        if (!placed) {
            Error(E_ERROR,const_cast<char*>("Can't replace rectangle"),0);
            exit(1);
        }

        const int page=pos/256;
        cell->nsurf=(vidType&4u)?page*2:page;
        if (pages<page+1)
            pages=page+1;
        cell->begx=x;
        cell->begy=pos;
    }
    ::operator delete(indices);
    Rewind();

    unsigned char* const stream=static_cast<unsigned char*>(::operator new(0x200000u));
    if (!stream) {
        Error(E_MEMORY,const_cast<char*>("2097152"),0);
        exit(1);
    }
    unsigned short* const colorPages=static_cast<unsigned short*>(::operator new(static_cast<unsigned int>(pages<<17)));
    if (!colorPages) {
        Error(E_MEMORY,const_cast<char*>("Buf"),static_cast<unsigned long>(pages));
        exit(1);
    }
    unsigned short* const zPages=static_cast<unsigned short*>(::operator new(static_cast<unsigned int>(pages<<17)));
    if (!zPages) {
        Error(E_MEMORY,const_cast<char*>("ZBuf"),static_cast<unsigned long>(pages));
        exit(1);
    }
    memset(colorPages,0,static_cast<unsigned int>(pages<<17));
    memset(zPages,0,static_cast<unsigned int>(pages<<17));

    for (int frame=0;frame<frames;++frame) {
        if (coor[frame].nsurf>=0) {
            VID_TEXCOOR* cell=coor+frame;
            for (;;) {
                for (int x0=cell->shiftx;x0<cell->shiftx+cell->sizex;++x0) {
                    for (int y0=cell->shifty;y0<cell->shifty+cell->sizey;++y0) {
                        const int at=cell->begx+x0-cell->shiftx+((y0+cell->begy-cell->shifty)<<8);
                        if (vidType&4u)
                            zPages[at]=static_cast<unsigned short>(GetPixelZ(x0,y0));
                        colorPages[at]=static_cast<unsigned short>(GetPixelT(x0,y0));
                    }
                }
                if (!cell->next_fragment)
                    break;
                cell=coor+cell->next_fragment;
            }
        }
        NextFrame();
    }

    for (int i=0;i<noCoor;++i)
        coor[i].begy%=256;
    for (int i=0;i<noCoor;++i) {
        if (coor[i].nsurf<0)
            memcpy(coor+i,coor+coor[i].sizex,sizeof(VID_TEXCOOR));
    }

    res->PreAppend(0x46525553u,0); // SURF
    short noSurf=static_cast<short>((vidType&4u)?pages*2:pages);
    res->Write(&noSurf,2);
    for (int page=0;page<pages;++page) {
        int usedX=0;
        int usedY=0;
        const int surfaceNo=(vidType&4u)?page*2:page;
        for (int i=0;i<noCoor;++i) {
            if (coor[i].nsurf!=surfaceNo)
                continue;
            const int right=coor[i].begx+coor[i].sizex;
            const int bottom=coor[i].begy+coor[i].sizey;
            if (usedX<right) usedX=right;
            if (usedY<bottom) usedY=bottom;
        }
        int surfaceX=32;
        int surfaceY=32;
        while (surfaceX<usedX) surfaceX<<=1;
        while (surfaceY<usedY) surfaceY<<=1;
        if (surfaceX>256)
            Error(E_INVALID,const_cast<char*>("SurfSizeX"),static_cast<unsigned long>(surfaceX));
        if (surfaceY>256)
            Error(E_INVALID,const_cast<char*>("SurfSizeY"),static_cast<unsigned long>(surfaceY));

        short word=static_cast<short>(surfaceX);
        res->Write(&word,2);
        word=static_cast<short>(surfaceY);
        res->Write(&word,2);

        int size=WriteSurfaces(stream,colorPages+(page<<16),surfaceX,surfaceY);
        res->Write(&size,4);
        res->WritePacked(stream,static_cast<unsigned int>(size),colorCoder);
        if (vidType&4u) {
            size=0;
            for (int y=0;y<surfaceY;++y) {
                memcpy(stream+size,zPages+((page<<16)+(y<<8)),static_cast<unsigned int>(2*surfaceX));
                size+=2*surfaceX;
            }
            res->Write(&size,4);
            res->WritePacked(stream,static_cast<unsigned int>(size),zCoder);
        }
    }
    res->PostAppend();

    res->PreAppend(0x41544144u,0); // DATA
    res->Write(coor,static_cast<unsigned int>(noCoor*sizeof(VID_TEXCOOR)));
    res->PostAppend();

    // The retail local shadowsize is explicitly zeroed immediately before
    // its SHAD conditional; therefore that section is unreachable here.
    if (colorCoder) delete colorCoder;
    if (zCoder) delete zCoder;
    ::operator delete(colorPages);
    ::operator delete(zPages);
    ::operator delete(coor);
    ::operator delete(stream);
    ::operator delete(shadow);
}

int PICTURE_MAKEVID::GetShadow(void* buffer)
{
    static const short dx[8]={0,1,1,1,0,-1,-1,-1};
    static const short dy[8]={-1,-1,0,1,1,1,0,-1};

    short* const output=static_cast<short*>(buffer);
    int outWords=0;
    output[outWords++]=0;
    if (!(vidType&0x20000u))
        return outWords*2;

    short* const dots=static_cast<short*>(::operator new(static_cast<unsigned int>(4*SizeY()*SizeX())));
    if (!dots) {
        Error(E_MEMORY,const_cast<char*>("shadow dot"),0);
        return outWords*2;
    }
    unsigned char* const visited=static_cast<unsigned char*>(::operator new(static_cast<unsigned int>(SizeX()*SizeY())));
    if (!visited) {
        Error(E_MEMORY,const_cast<char*>("dot without shadow"),0);
        return outWords*2;
    }
    memset(visited,0,static_cast<unsigned int>(SizeX()*SizeY()));

    for (;;) {
        int noDot=0;
        outWords=1;

        int x=0;
        int y=0;
        for (;x<SizeX();++x) {
            for (y=0;y<=x;++y) {
                if (IsPixel(x,y) && !visited[x+y*SizeX()])
                    break;
            }
            if (y<=x)
                break;

            int k=0;
            for (;k<x;++k) {
                if (IsPixel(k,x) && !visited[k+x*SizeX()])
                    break;
            }
            if (k<x) {
                y=x;
                x=k;
                break;
            }
        }

        // Retail exits here without releasing the two temporary allocations.
        // Preserve that observable failure-path behavior rather than adding a
        // cleanup that is absent at 0x44D387..0x44D38C.
        if (x>=SizeX())
            return outWords*2;

        dots[2*noDot]=static_cast<short>(x);
        const int begx=static_cast<short>(x);
        dots[2*noDot+1]=static_cast<short>(y);
        const int begy=static_cast<short>(y);
        ++noDot;

        int direction=0;
        do {
            int probe=(direction+1)&7;
            while (probe!=direction) {
                const int nx=x+dx[probe];
                const int ny=y+dy[probe];
                if (IsPixel(nx,ny) && !visited[nx+ny*SizeX()]) {
                    x=nx;
                    y=ny;
                    dots[2*noDot]=static_cast<short>(x);
                    dots[2*noDot+1]=static_cast<short>(y);
                    ++noDot;
                    direction=probe^4;
                    break;
                }
                probe=(probe+1)&7;
            }

            if (probe==direction) {
                --noDot;
                int back=0;
                for (;back<8;++back) {
                    if (static_cast<int>(dots[2*noDot])==static_cast<int>(dots[2*(noDot-1)])+dx[back] &&
                        static_cast<int>(dots[2*noDot+1])==static_cast<int>(dots[2*(noDot-1)+1])+dy[back])
                        break;
                }
                direction=back^4;
            }
        } while (static_cast<int>(dots[0])!=x || static_cast<int>(dots[1])!=y);

        int noDot1=1;
        int base=0;
        while (base<noDot) {
            int probe=base+3;
            for (;probe<noDot;++probe) {
                if ((static_cast<int>(dots[2*probe+1])-static_cast<int>(dots[2*base+1]))==0 &&
                    (static_cast<int>(dots[2*probe])-static_cast<int>(dots[2*base]))==0)
                    continue;

                const int retailCheck=
                    static_cast<int>(dots[2*base+1])-
                    static_cast<int>(dots[2*probe-1])-
                    static_cast<int>(dots[2*probe-2])+
                    static_cast<int>(dots[2*base]);
                if (abs(retailCheck)<1)
                    continue;

                int mid=base+1;
                for (;mid<probe;++mid) {
                    const int ddx=static_cast<int>(dots[2*base])-static_cast<int>(dots[2*probe]);
                    const int ddy=static_cast<int>(dots[2*base+1])-static_cast<int>(dots[2*probe+1]);
                    const int length=Sqrt(ddx*ddx+ddy*ddy);
                    const int numerator=
                        (static_cast<int>(dots[2*probe+1])-static_cast<int>(dots[2*base+1]))*static_cast<int>(dots[2*mid])+
                        (static_cast<int>(dots[2*base])-static_cast<int>(dots[2*probe]))*static_cast<int>(dots[2*mid+1])-
                        static_cast<int>(dots[2*base])*static_cast<int>(dots[2*probe+1])+
                        static_cast<int>(dots[2*probe])*static_cast<int>(dots[2*base+1]);
                    const int distance=abs(numerator)/length;
                    const int limit=(vidType&0x80000u)?3:0;
                    if (distance>limit)
                        break;
                }
                if (mid<probe)
                    break;
            }

            base=probe-1;
            if (probe<noDot) {
                const unsigned int packed=*reinterpret_cast<const unsigned int*>(dots+2*(probe-1));
                *reinterpret_cast<unsigned int*>(dots+2*noDot1)=packed;
                ++noDot1;
            }
        }

        if (noDot1<3) {
            visited[begx+begy*SizeX()]=1;
            continue;
        }

        noDot=noDot1;
        while (noDot>255) {
            noDot/=2;
            for (int i=0;i<noDot;++i) {
                const unsigned int packed=*reinterpret_cast<const unsigned int*>(dots+4*i);
                *reinterpret_cast<unsigned int*>(dots+2*i)=packed;
            }
        }

        output[0]=static_cast<short>(noDot);
        for (int i=0;i<noDot;++i) {
            const int px=static_cast<int>(dots[2*i]);
            const int py=static_cast<int>(dots[2*i+1]);
            const int lift=static_cast<int>(GetPixelZ(px,py))/8-128;
            output[outWords++]=static_cast<short>(px);
            output[outWords++]=static_cast<short>(py+lift);
            output[outWords++]=static_cast<short>(lift);
        }

        ::operator delete(dots);
        ::operator delete(visited);
        return outWords*2;
    }
}

void PICTURE_MAKEVID::CreateOnePalette()
{
    int distRow[256];
    int matrix[256][256];

    if (paletteDecode)
        ::operator delete(paletteDecode);
    paletteDecode=static_cast<unsigned char*>(::operator new(0x100000u));
    memset(paletteDecode,0,0x100000u);

    unsigned int* const pal=reinterpret_cast<unsigned int*>(commonPalette);
    pal[0]=0;
    int count=1;
    matrix[0][0]=0;

    for (int frame=0;frame<NoFrame();++frame) {
        for (int y=0;y<SizeY();++y) {
            for (int x=0;x<SizeX();++x) {
                if (!IsPixel(x,y))
                    continue;
                COLOR c=GetPixel(x,y);
                const unsigned int raw=c.color;
                const unsigned int hash=((raw&0xF8u)|(((raw&0xFC00u)|((raw>>3)&0x1F0000u))>>2))>>3;
                if (paletteDecode[16u*hash+(raw>>28)])
                    continue;

                int bestIdx=0;
                int k=0;
                for (;k<count;++k) {
                    distRow[k]=commonPalette[k].Diff(&c);
                    if (!distRow[k])
                        break;
                    if (distRow[k]<distRow[bestIdx])
                        bestIdx=k;
                }
                if (k<count) {
                    SetPaletteDecodeNumber(c,static_cast<unsigned char>(k));
                    continue;
                }
                if (count<256) {
                    for (k=0;k<count;++k) {
                        matrix[count][k]=distRow[k];
                        matrix[k][count]=distRow[k];
                    }
                    commonPalette[count]=c;
                    SetPaletteDecodeNumber(c,static_cast<unsigned char>(count));
                    ++count;
                    continue;
                }

                int pairJ=1;
                int pairI=0;
                for (int j=1;j<256;++j) {
                    for (int i=0;i<j;++i) {
                        if (matrix[j][i]<matrix[pairJ][pairI]) {
                            pairJ=j;
                            pairI=i;
                        }
                    }
                }
                if (matrix[pairJ][pairI]<distRow[bestIdx]) {
                    COLOR oldColor;
                    oldColor.color=pal[pairI];
                    SetPaletteDecodeNumber(oldColor,static_cast<unsigned char>(bestIdx));
                    pal[pairI]=raw;
                    SetPaletteDecodeNumber(c,static_cast<unsigned char>(pairI));
                    for (k=0;k<256;++k) {
                        matrix[pairI][k]=distRow[k];
                        matrix[k][pairI]=distRow[k];
                    }
                } else {
                    SetPaletteDecodeNumber(c,static_cast<unsigned char>(bestIdx));
                }
            }
        }
        NextFrame();
    }
    Rewind();

    int cnt[256];
    int sumR[256];
    int sumB[256];
    int sumG[256];
    int sumA[256];
    memset(cnt,0,sizeof(cnt));
    memset(sumR,0,sizeof(sumR));
    memset(sumG,0,sizeof(sumG));
    memset(sumB,0,sizeof(sumB));
    memset(sumA,0,sizeof(sumA));

    int i=0;
    for (;i<0x100000;++i) {
        const unsigned char index=paletteDecode[i];
        if (!index)
            continue;
        ++cnt[index];
        sumR[index]+=((i/16)>>8)&0xF8;
        sumG[index]+=((i/16)>>3)&0xFC;
        sumB[index]+=8*((i/16)&0x1F);
        sumA[index]+=16*(i&0xF)+15;
    }
    for (i=0;i<256;++i) {
        if (!cnt[i])
            continue;
        int b=sumB[i]/cnt[i];
        int g=sumG[i]/cnt[i];
        int r=sumR[i]/cnt[i];
        int a=sumA[i]/cnt[i];
        if (a<0) a=0; else if (a>255) a=255;
        if (r<0) r=0; else if (r>255) r=255;
        if (g<0) g=0; else if (g>255) g=255;
        if (b<0) b=0; else if (b>255) b=255;
        pal[i]=static_cast<unsigned int>(b|(g<<8)|(r<<16)|(a<<24));
    }

    if ((vidType&2u) && count>0) {
        for (i=0;i<count;++i) {
            COLOR* const col=reinterpret_cast<COLOR*>(pal+i);
            if ((pal[i]&0xFF000000u)>0xEF000000u)
                col->SetAlpha(255);
            else
                col->SetAlpha(static_cast<int>(16u*(pal[i]>>28)));
            const unsigned int c=pal[i];
            if (c&0xFF000000u) {
                int a=static_cast<int>(c>>24);
                int b=255*static_cast<int>(c&0xFFu)/a;
                int g=255*static_cast<int>((c>>8)&0xFFu)/a;
                int r=255*static_cast<int>((c>>16)&0xFFu)/a;
                if (a<0) a=0; else if (a>255) a=255;
                if (r<0) r=0; else if (r>255) r=255;
                if (g<0) g=0; else if (g>255) g=255;
                if (b<0) b=0; else if (b>255) b=255;
                pal[i]=static_cast<unsigned int>(b|(g<<8)|(r<<16)|(a<<24));
            }
        }
    }
}

int PICTURE_MAKEVID::GetSoftwareRectangle(void* buffer,int x0,int y0,int x1,int y1)
{
    char* const out=static_cast<char*>(buffer);
    int pos=0;
    for (int y=y0;y<y1;++y) {
        int x=x0;
        for (;;) {
            int runStart=x;
            if (!IsPixel(x,y)) {
                do {
                    ++x;
                    if (x>=x1)
                        break;
                } while (!IsPixel(x,y));
            }

            // Retail tests the physical picture width here, not the rectangle x1.
            if (x>=SizeX()) {
                out[pos++]=0;
                out[pos++]=0;
                break;
            }

            while (x-runStart>255) {
                out[pos++]=static_cast<char>(255);
                out[pos++]=0;
                runStart+=255;
            }
            out[pos++]=static_cast<char>(x-runStart);

            runStart=x;
            while (x-runStart<255) {
                if (vidType&4u) {
                    if (!IsPixel(x,y) && !IsPixel(x+1,y) && !IsPixel(x+2,y))
                        break;
                } else if (!IsPixel(x,y)) {
                    break;
                }
                if (x>=x1)
                    break;
                ++x;
            }
            out[pos++]=static_cast<char>(x-runStart);

            if (vidType&4u) {
                for (int zx=runStart;zx<x;++zx) {
                    const short z=GetPixelZ(zx,y);
                    PutPacked(out+pos,z);
                    pos+=2;
                }
            }
            for (int px=runStart;px<x;++px) {
                if (vidType&8u)
                    out[pos++]=static_cast<char>(GetPixelT(px,y));
                else {
                    const short pixel=static_cast<short>(GetPixelT(px,y));
                    PutPacked(out+pos,pixel);
                    pos+=2;
                }
            }
        }
    }
    return pos;
}

void PICTURE_MAKEVID::WriteSoftware(RESOURCE* res)
{
    QS1_CODER* coder=0;
    if (vidType&0x100u)
        coder=new QS1_CODER(1);

    char* const buf=static_cast<char*>(::operator new(0x200000u));
    if (!buf) {
        STRING name=texture.FileName();
        MYERROR::Error(::Error,"PICTURE '%s'",2,"cdr_s",0,name.CharPtr());
        exit(1);
    }
    int* const crcs=static_cast<int*>(::operator new(static_cast<unsigned int>(4*NoFrame())));
    if (!crcs) {
        STRING name=texture.FileName();
        MYERROR::Error(::Error,"PICTURE '%s'",2,"cntrl_s",0,name.CharPtr());
        exit(1);
    }

    if (vidType&8u) {
        res->PreAppend(0x204C4150u,0); // PAL 
        if (vidType&0x10u) {
            if (vidType&2u) {
                CreateOnePalette();
                res->Write(commonPalette,1024u);
            } else {
                res->Write(texture.GetPalette(),1024u);
            }
            res->PostAppend();
        } else {
            unsigned char triplets[768];
            const COLOR* const palette=texture.GetPalette();
            for (int e=0;e<256;++e) {
                triplets[3*e]=static_cast<unsigned char>(palette[e].color>>16);
                triplets[3*e+1]=static_cast<unsigned char>(palette[e].color>>8);
                triplets[3*e+2]=static_cast<unsigned char>(palette[e].color);
            }
            res->Write(triplets,768u);
            res->PostAppend();
        }
    }

    for (int frame=0;frame<NoFrame();++frame) {
        res->PreAppend(0x41544144u,0); // DATA
        int pos=0;
        const int crc=CalcCRC32();
        crcs[frame]=crc;
        int dup=0;
        for (;dup<frame;++dup) {
            if (crc==crcs[dup]) {
                pos=2;
                const short d=static_cast<short>(dup);
                PutPacked(buf,d);
                break;
            }
        }
        if (dup>=frame) {
            pos+=GetShadow(buf+pos);
            int top=0,bottom=0;
            GetRectangle(0,&top,0,&bottom);
            if (vidType&0x100000u) {
                bottom=0;
                top=0;
            }
            const short topWord=static_cast<short>(top);
            const short heightWord=static_cast<short>(bottom-top);
            PutPacked(buf+pos,topWord); pos+=2;
            PutPacked(buf+pos,heightWord); pos+=2;
            pos+=GetSoftwareRectangle(buf+pos,0,top,SizeX(),bottom);
        }
        res->Write(&pos,4u);
        res->WritePacked(buf,static_cast<unsigned int>(pos),coder);
        res->PostAppend();
        NextFrame();
    }

    delete coder;
    ::operator delete(crcs);
    ::operator delete(buf);
}

void PICTURE_MAKEVID::WritePseudo3d(RESOURCE* res)
{
    char* const buf=static_cast<char*>(::operator new(0x200000u));
    if (!buf) {
        STRING name=texture.FileName();
        MYERROR::Error(::Error,"PICTURE '%s'",2,"cdr_s",0,name.CharPtr());
        exit(1);
    }
    int* const crcs=static_cast<int*>(::operator new(static_cast<unsigned int>(4*NoFrame())));
    if (!crcs) {
        STRING name=texture.FileName();
        MYERROR::Error(::Error,"PICTURE '%s'",2,"cntrl_s",0,name.CharPtr());
        exit(1);
    }
    QS1_CODER* coder=(vidType&0x100u) ? new QS1_CODER(1) : 0;

    if (vidType&2u)
        vidType&=~8u;
    if (vidType&8u) {
        res->PreAppend(0x204C4150u,0);
        res->Write(texture.GetPalette(),1024u);
        res->PostAppend();
    }

    for (int frame=0;frame<NoFrame();++frame) {
        res->PreAppend(0x41544144u,0);
        int pos=0;
        const int crc=CalcCRC32();
        crcs[frame]=crc;
        int dup=0;
        for (;dup<frame;++dup) {
            if (crc==crcs[dup]) {
                pos=4;
                PutPacked(buf,dup);
                break;
            }
        }
        if (dup>=frame) {
            int left=0,top=0,right=0,bottom=0;
            GetRectangle(&left,&top,&right,&bottom);
            const int format=(vidType&8u) ? 41 : ((vidType&2u) ? 21 : 25);
            PutPacked(buf+pos,format); pos+=4;
            const short width=static_cast<short>(right-left);
            const short height=static_cast<short>(bottom-top);
            PutPacked(buf+pos,width); pos+=2;
            PutPacked(buf+pos,height); pos+=2;
            for (int y=top;y<bottom;++y) {
                for (int x=left;x<right;++x) {
                    if (vidType&8u)
                        buf[pos++]=static_cast<char>(GetPixelT(x,y));
                    else if (vidType&2u) {
                        const unsigned int pixel=static_cast<unsigned int>(GetPixelT(x,y));
                        PutPacked(buf+pos,pixel); pos+=4;
                    } else {
                        const short pixel=static_cast<short>(GetPixelT(x,y));
                        PutPacked(buf+pos,pixel); pos+=2;
                    }
                }
            }

            const int countPos=pos;
            int zero=0;
            PutPacked(buf+pos,zero); pos+=4;
            PutPacked(buf+pos,zero); pos+=4;
            int vertices=0;
            int indices=0;
            const int gridW=right-left+9;
            const int gridH=bottom-top+9;
            short* const grid=static_cast<short*>(::operator new(static_cast<unsigned int>(2*gridW*gridH)));
            for (int y=top;y<bottom+8;y+=8) {
                for (int x=left;x<right+8;x+=8) {
                    if (IsPixelInRect(x-8,y-8,9,9)) {
                        const short z=GetPixelZInRect(x-8,y-8,9,9);
                        const short sx=static_cast<short>(x);
                        const short sy=static_cast<short>(y);
                        const short tx=static_cast<short>(x-left);
                        const short ty=static_cast<short>(y-top);
                        PutPacked(buf+pos,sx); pos+=2;
                        PutPacked(buf+pos,sy); pos+=2;
                        PutPacked(buf+pos,z); pos+=2;
                        PutPacked(buf+pos,tx); pos+=2;
                        PutPacked(buf+pos,ty); pos+=2;
                        grid[x-left+gridW*(y-top)]=static_cast<short>(vertices);
                        ++vertices;
                    }
                }
            }
            PutPacked(buf+countPos,vertices);
            for (int y=top;y<bottom+8;y+=8) {
                for (int x=left;x<right+8;x+=8) {
                    if (IsPixelInRect(x,y,8,8)) {
                        short v;
                        v=grid[x-left+gridW*(y-top)]; PutPacked(buf+pos,v); pos+=2;
                        v=grid[x+8-left+gridW*(y-top)]; PutPacked(buf+pos,v); pos+=2;
                        v=grid[x-left+gridW*(y-top+8)]; PutPacked(buf+pos,v); pos+=2;
                        v=grid[x+8-left+gridW*(y-top+8)]; PutPacked(buf+pos,v); pos+=2;
                        v=grid[x+8-left+gridW*(y-top)]; PutPacked(buf+pos,v); pos+=2;
                        v=grid[x-left+gridW*(y-top+8)]; PutPacked(buf+pos,v); pos+=2;
                        indices+=6;
                    }
                }
            }
            PutPacked(buf+countPos+4,indices);
            ::operator delete(grid);
        }
        res->Write(&pos,4u);
        res->WritePacked(buf,static_cast<unsigned int>(pos),coder);
        res->PostAppend();
        NextFrame();
    }

    delete coder;
    ::operator delete(crcs);
    ::operator delete(buf);
}

void PICTURE_MAKEVID::WriteLight(RESOURCE* res)
{
    res->PreAppend(0x41544144u,0);
    for (int i=0;i<NoFrame();++i) {
        COLOR color=GetPixel(0,0);
        color.Write(res);
        NextFrame();
    }
    res->PostAppend();
}

int PICTURE_MAKEVID::SizeX() { return texture.SizeX(); }
int PICTURE_MAKEVID::SizeY() { return texture.SizeY(); }

int PICTURE_MAKEVID::InViewPort(int x,int y) { return texture.InViewPort(x,y); }

int PICTURE_MAKEVID::NoFrame() { return texture.NoFrame(); }

int PICTURE_BASE::IsPaletted()
{
    return Fields(this).bytesPerPixel == 1;
}

unsigned int PICTURE_BASE::GetData(int x,int y)
{
    PictureBaseLayout& f = Fields(this);
    if (x < 0 || y < 0 || x >= f.sizeX || y >= f.sizeY || !f.data)
        return 0;

    const unsigned int index = static_cast<unsigned int>(y * f.sizeX + x);
    const uint8_t* data = static_cast<const uint8_t*>(f.data);
    switch (f.bytesPerPixel) {
    case 4:
        return reinterpret_cast<const uint32_t*>(data)[index];
    case 3: {
        const uint8_t* pixel = data + index * 3u;
        return static_cast<unsigned int>(pixel[0]) |
               (static_cast<unsigned int>(pixel[1]) << 8) |
               (static_cast<unsigned int>(pixel[2]) << 16);
    }
    case 2:
        return reinterpret_cast<const uint16_t*>(data)[index];
    case 1:
        return data[index];
    default:
        return 0;
    }
}

COLOR PICTURE_BASE::GetPixel(int x,int y)
{
    PictureBaseLayout& f = Fields(this);
    if (x < 0 || y < 0 || x >= f.sizeX || y >= f.sizeY || !f.data)
        return COLOR(0,0,0);

    const unsigned int index = static_cast<unsigned int>(y * f.sizeX + x);
    const uint8_t* data = static_cast<const uint8_t*>(f.data);

    if (IsPaletted()) {
        const unsigned int paletteIndex = data[index];
        return COLOR(f.palette[paletteIndex]);
    }

    COLOR result;
    switch (f.bytesPerPixel) {
    case 4:
        result.color = reinterpret_cast<const uint32_t*>(data)[index];
        return result;
    case 3: {
        const uint8_t* pixel = data + index * 3u;
        result.color = static_cast<uint32_t>(pixel[0]) |
                       (static_cast<uint32_t>(pixel[1]) << 8) |
                       (static_cast<uint32_t>(pixel[2]) << 16);
        return result;
    }
    case 2: {
        const uint32_t value = reinterpret_cast<const uint16_t*>(data)[index];
        result.color = 0xFF000000u |
                       ((value << 9) & 0x00FF0000u) |
                       ((value << 6) & 0x0000FF00u) |
                       ((value << 3) & 0x000000FFu);
        return result;
    }
    case 1: {
        const int v = data[index];
        return COLOR(v,v,v);
    }
    default:
        return COLOR(0,0,0);
    }
}


void PICTURE_BASE::SaveTGA(const STRING* filename,int x,int y,int sizeX,int sizeY)
{
    PictureBaseLayout& f=Fields(this);
    unsigned char header[18]={0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,24,1};
    if (!f.data) {
        MYERROR::Error(::Error,"PICTURE '%s'",10,"SaveTGA-not picture",0,f.filename.CharPtr());
        return;
    }
    if (sizeY==-1) sizeY=f.sizeY;
    if (sizeX==-1) sizeX=f.sizeX;
    if (x+sizeX>f.sizeX || y+sizeY>f.sizeY) {
        MYERROR::Error(::Error,"PICTURE '%s'",4,"size in SaveTGA",0,f.filename.CharPtr());
        return;
    }

    void* file=0;
    if (filename && filename->m_buf && *filename->m_buf)
        file=fopen(filename->m_buf,"wb");
    if (!file) {
        MYERROR::Error(::Error,"PICTURE '%s'",7,filename && filename->m_buf ? filename->m_buf : "",0,f.filename.CharPtr());
        return;
    }

    if (f.bytesPerPixel==4)
        header[16]=static_cast<unsigned char>(f.bytesPerPixel*8);
    header[12]=static_cast<unsigned char>(sizeX);
    header[13]=static_cast<unsigned char>(static_cast<unsigned int>(sizeX)>>8);
    header[14]=static_cast<unsigned char>(sizeY);
    header[15]=static_cast<unsigned char>(static_cast<unsigned int>(sizeY)>>8);
    header[0]=0;
    header[2]&=static_cast<unsigned char>(~8u);
    fwrite(header,18u,1u,file);

    for (int py=sizeY-1;py>=0;--py) {
        for (int px=0;px<sizeX;++px) {
            COLOR pixel=GetPixel(px+x,py+y);
            unsigned int value=pixel.color;
            if (f.bytesPerPixel==4)
                fwrite(&value,1u,4u,file);
            else {
                value&=0x00ffffffu;
                fwrite(&value,1u,3u,file);
            }
        }
    }
    fclose(file);
}


#pragma pack(push,1)
struct FLIC_HEADER {
    unsigned long Size;
    unsigned short Type;
    unsigned short FrameCount;
    unsigned short Width;
    unsigned short Height;
    unsigned short BitsInPixel;
    unsigned short Flags;
    unsigned long Speed;
    unsigned short Unused;
    unsigned long Created;
    unsigned long Creator;
    unsigned long Updated;
    unsigned long Updator;
    unsigned short AspectX;
    unsigned short AspectY;
    unsigned char Reserved1[38];
    unsigned long Offset1;
    unsigned long Offset2;
    unsigned char Reserved2[40];
};
struct TGA_HEADER {
    unsigned char IdLength;
    unsigned char ColorMapType;
    unsigned char ImageType;
    unsigned short ColorMapFirst;
    unsigned short ColorMapLength;
    unsigned char ColorMapUnitLength;
    unsigned short ImageXOrigin;
    unsigned short ImageYOrigin;
    unsigned short Width;
    unsigned short Height;
    unsigned char ImageBitsPerPixel;
    unsigned char ImageDescriptor;
};
struct BMP_HEADER {
    unsigned short Type;
    unsigned long Size;
    unsigned long Reserved;
    unsigned long OffBits;
    unsigned long SizeStruct;
    unsigned long Width;
    unsigned long Height;
    unsigned short Planes;
    unsigned short BitCount;
    unsigned long Compression;
    unsigned long SizeImage;
    unsigned long XPelsPerMeter;
    unsigned long YPelsPerMeter;
    unsigned long ClrUsed;
    unsigned long ClrImportant;
};
struct Z_HEADER {
    unsigned long Type;
    unsigned long Width;
    unsigned long Height;
    unsigned long NoFrame;
};
#pragma pack(pop)
static_assert(sizeof(FLIC_HEADER)==128, "debug metadata FLIC_HEADER size");
static_assert(sizeof(TGA_HEADER)==18, "debug metadata TGA_HEADER size");
static_assert(sizeof(BMP_HEADER)==54, "debug metadata BMP_HEADER size");
static_assert(sizeof(Z_HEADER)==16, "debug metadata Z_HEADER size");

class PICTURE_FLIC : public PICTURE_BASE {
public:
    unsigned short flicType;
    virtual void NextFrame();
    virtual int Load(const STRING* filename);
};
static_assert(sizeof(PICTURE_FLIC)==1072, "debug metadata PICTURE_FLIC size");

class PICTURE_TGA : public PICTURE_BASE {
public:
    unsigned char imageType;
    virtual void NextFrame();
    virtual void Rewind();
    virtual int Load(const STRING* filename);
    int LoadHeader(TGA_HEADER* header);
};
static_assert(sizeof(PICTURE_TGA)==1072, "debug metadata PICTURE_TGA size");

class PICTURE_BMP : public PICTURE_BASE {
public:
    virtual void NextFrame();
    virtual void Rewind();
    virtual int Load(const STRING* filename);
    int LoadHeader(BMP_HEADER* header);
};
static_assert(sizeof(PICTURE_BMP)==1068, "debug metadata PICTURE_BMP size");

class PICTURE_Z : public PICTURE_BASE {
public:
    virtual void NextFrame();
    virtual int Load(const STRING* filename);
};
static_assert(sizeof(PICTURE_Z)==1068, "debug metadata PICTURE_Z size");

void PICTURE_BASE::Error(TYPE_ERROR type,char* text,unsigned long err)
{
    PictureBaseLayout& f=Fields(this);
    MYERROR::Error(::Error,"PICTURE '%s'",static_cast<int>(type),text,err,f.filename.CharPtr());
}

int PICTURE_BASE::IsOpened() { return Fields(this).data!=0; }
int PICTURE_BASE::BytesPerPixel() { return Fields(this).bytesPerPixel; }
int PICTURE_BASE::FrameTime() { return Fields(this).frameTime; }
int PICTURE_BASE::CurFrame() { return Fields(this).curFrame; }
int PICTURE_BASE::NoFrame() { return Fields(this).noFrame; }
const COLOR* PICTURE_BASE::GetPalette() { return Fields(this).palette; }
void PICTURE_BASE::SetPalette(const COLOR* palette)
{
    if (palette)
        memcpy(Fields(this).palette,palette,sizeof(Fields(this).palette));
}

void PICTURE_BASE::Rewind()
{
    PictureBaseLayout& f=Fields(this);
    if (!IsOpened()) return;
    if (f.file) fseek(f.file,f.dataOffset,0);
    f.curFrame=-1;
    NextFrame();
}

void PICTURE_BASE::NextFrame()
{
    PictureBaseLayout& f=Fields(this);
    if (!f.noFrame) return;
    ++f.curFrame;
    if (f.curFrame>=f.noFrame)
        Rewind();
}

int PICTURE_BASE::Load(const STRING* filename)
{
    Close();
    // Retail calls STRING::operator==(EMPTY) directly on filename; a null STRING pointer is not guarded.
    if (filename->m_buf[0]==0) {
        Error(E_INVALID,const_cast<char*>("filename"),0);
        return 1;
    }
    STRING lower=const_cast<STRING*>(filename)->ToLower();
    *reinterpret_cast<STRING*>(reinterpret_cast<unsigned char*>(this)+0x1C)=lower;
    PictureBaseLayout& f=Fields(this);
    f.file=FOpen(filename,"rb");
    if (!f.file) {
        Error(E_OPEN,const_cast<char*>(""),0);
        return 1;
    }
    return 0;
}

int PICTURE_FLIC::Load(const STRING* filename)
{
    PictureBaseLayout& f=Fields(this);
    FLIC_HEADER header;
    if (PICTURE_BASE::Load(filename)) return 1;
    fread(&header,sizeof(header),1,f.file);
    flicType=header.Type;
    f.noFrame=header.FrameCount;
    if (header.Type==0xAF11u) {
        f.dataOffset=static_cast<long>(sizeof(header));
        f.frameTime=static_cast<int>((1000ul*header.Speed)/70ul);
    } else if (header.Type==0xAF12u) {
        f.dataOffset=static_cast<long>(header.Offset1);
        f.frameTime=static_cast<int>(header.Speed);
    } else {
        Close();
        Error(E_INVALID,const_cast<char*>("flic type"),0);
        return 1;
    }
    SetSize(static_cast<int>(header.Width),static_cast<int>(header.Height),1);
    Rewind();
    return 0;
}

void PICTURE_FLIC::NextFrame()
{
    PictureBaseLayout& f=Fields(this);
    if (!IsOpened()) {
        Error(E_ERROR,const_cast<char*>("Picture has not opened"),static_cast<unsigned long>(f.curFrame));
        return;
    }
    ++f.curFrame;
    if (f.curFrame>=f.noFrame) {
        Rewind();
        return;
    }

    unsigned long frameSize=0;
    fread(&frameSize,4,1,f.file);
    unsigned char* frame=static_cast<unsigned char*>(malloc(frameSize));
    if (!frame) {
        Error(E_MEMORY,const_cast<char*>("cadr"),static_cast<unsigned long>(f.curFrame));
        return;
    }
    fread(frame,frameSize-4u,1,f.file);
    if (*reinterpret_cast<unsigned short*>(frame)!=0xF1FAu) {
        free(frame);
        Error(E_INVALID,const_cast<char*>("chunk mark"),static_cast<unsigned long>(f.curFrame));
        return;
    }

    unsigned int offset=12u;
    const unsigned int chunkCount=*reinterpret_cast<unsigned short*>(frame+2);
    for (unsigned int chunk=0;chunk<chunkCount;++chunk) {
        const unsigned long chunkSize=*reinterpret_cast<unsigned long*>(frame+offset);
        unsigned char* p=frame+offset+4u;
        offset+=chunkSize;
        const unsigned int chunkType=p[0];
        p+=2;

        switch (chunkType) {
        case 4:  // FLI_COLOR256
        case 11: { // FLI_COLOR
            int packets=static_cast<short>(*reinterpret_cast<unsigned short*>(p));
            p+=2;
            int paletteIndex=0;
            while (--packets>=0) {
                paletteIndex+=*p++;
                int count=*p++;
                if (!count) count=256;
                while (--count>=0) {
                    const int red=(chunkType==11) ? static_cast<int>(p[0])*4 : p[0];
                    const int green=(chunkType==11) ? static_cast<int>(p[1])*4 : p[1];
                    const int blue=(chunkType==11) ? static_cast<int>(p[2])*4 : p[2];
                    COLOR c(red,green,blue);
                    f.palette[paletteIndex].color=c.color;
                    p+=3;
                    ++paletteIndex;
                }
            }
            break;
        }
        case 7: { // FLC SS2 word delta
            int endLine=*reinterpret_cast<unsigned short*>(p);
            p+=2;
            int line=0;
            int dstOffset=0;
            while (line<endLine) {
                int code=static_cast<short>(*reinterpret_cast<unsigned short*>(p));
                p+=2;
                if ((code&0xC000)==0xC000) {
                    const int skip=-code;
                    dstOffset+=skip*f.sizeX;
                    line+=skip;
                    endLine+=skip;
                    continue;
                }
                if ((code&0xC000)==0x8000) {
                    static_cast<unsigned char*>(f.data)[dstOffset+f.sizeX-1]=static_cast<unsigned char>(code&0xFF);
                    continue;
                }
                const int lineStart=dstOffset;
                int packets=code;
                while (--packets>=0) {
                    dstOffset+=*p++;
                    int count=static_cast<signed char>(*p++);
                    if (count<0) {
                        while (++count<=0) {
                            *reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(f.data)+dstOffset)=
                                *reinterpret_cast<unsigned short*>(p);
                            dstOffset+=2;
                        }
                        p+=2;
                    } else {
                        memcpy(static_cast<unsigned char*>(f.data)+dstOffset,p,static_cast<unsigned int>(count*2));
                        p+=count*2;
                        dstOffset+=count*2;
                    }
                }
                dstOffset=lineStart+f.sizeX;
                ++line;
            }
            break;
        }
        case 12: { // FLI_LC byte delta
            int line=*reinterpret_cast<unsigned short*>(p); p+=2;
            const int endLine=line+*reinterpret_cast<unsigned short*>(p); p+=2;
            for (;line<endLine;++line) {
                int x=0;
                int packets=*p++;
                while (--packets>=0) {
                    x+=*p++;
                    int count=static_cast<signed char>(*p++);
                    if (count<0) {
                        const int run=-count;
                        memset(static_cast<unsigned char*>(f.data)+line*f.sizeX+x,*p++,static_cast<unsigned int>(run));
                        x+=run;
                    } else {
                        memcpy(static_cast<unsigned char*>(f.data)+line*f.sizeX+x,p,static_cast<unsigned int>(count));
                        x+=count;
                        p+=count;
                    }
                }
            }
            break;
        }
        case 13: // FLI_BLACK
            memset(f.data,0,static_cast<unsigned int>(f.sizeX*f.sizeY));
            break;
        case 15: // FLI_BRUN
            if (flicType==0xAF11u) {
                for (int y=0;y<f.sizeY;++y) {
                    int x=0;
                    int packets=*p++;
                    while (--packets>=0) {
                        const int count=static_cast<signed char>(*p);
                        if (count<0) {
                            const int end=x-count;
                            while (x<end) {
                                ++p;
                                static_cast<unsigned char*>(f.data)[y*f.sizeX+x]=*p;
                                ++x;
                            }
                        } else {
                            const int end=x+static_cast<unsigned char>(*p);
                            ++p;
                            while (x<end) {
                                static_cast<unsigned char*>(f.data)[y*f.sizeX+x]=*p;
                                ++x;
                            }
                        }
                    }
                }
            } else {
                for (int y=0;y<f.sizeY;++y) {
                    int x=0;
                    ++p; // packet count is present but retail decodes until width.
                    while (x<f.sizeX) {
                        const int count=static_cast<signed char>(*p++);
                        if (count<0) {
                            const int literal=-count;
                            memcpy(static_cast<unsigned char*>(f.data)+y*f.sizeX+x,p,static_cast<unsigned int>(literal));
                            x+=literal;
                            p+=literal;
                        } else {
                            memset(static_cast<unsigned char*>(f.data)+y*f.sizeX+x,*p++,static_cast<unsigned int>(count));
                            x+=count;
                        }
                    }
                }
            }
            break;
        case 16: // FLI_COPY
            memcpy(f.data,p,static_cast<unsigned int>(f.sizeX*f.sizeY));
            break;
        default:
            break;
        }
    }
    free(frame);
}

int PICTURE_TGA::LoadHeader(TGA_HEADER* header)
{
    PictureBaseLayout& f=Fields(this);
    if (!f.file) {
        Error(E_OPEN,const_cast<char*>("tga cadr"),static_cast<unsigned long>(f.curFrame));
        return 1;
    }
    fread(header,sizeof(*header),1,f.file);
    imageType=header->ImageType;
    f.dataOffset=static_cast<long>(header->IdLength+sizeof(*header));
    if (!imageType) {
        Close();
        Error(E_ERROR,const_cast<char*>("not image in tga"),0);
        return 1;
    }
    if (header->ColorMapType==1) {
        Close();
        Error(E_ERROR,const_cast<char*>("not supported ColorMapType in tga"),0);
        return 1;
    }
    if ((imageType&3u)==1u) {
        Close();
        Error(E_ERROR,const_cast<char*>("not supported ColorMap in tga"),0);
        return 1;
    }
    if (IsOpened()) {
        const int bpp=(static_cast<int>(header->ImageBitsPerPixel)+7)/8;
        if (f.sizeX!=header->Width || f.sizeY!=header->Height || f.bytesPerPixel!=bpp) {
            Error(E_ERROR,const_cast<char*>("TGA parameters different from first cadr"),static_cast<unsigned long>(f.curFrame));
            return 1;
        }
    }
    return 0;
}

int PICTURE_TGA::Load(const STRING* filename)
{
    TGA_HEADER header;
    PictureBaseLayout& f=Fields(this);
    if (PICTURE_BASE::Load(filename)) return 1;
    if (LoadHeader(&header)) return 1;
    f.noFrame=1;
    STRING before=f.filename.Before(".tga");
    const char last=before.m_buf[0] ? before.m_buf[strlen(before.m_buf)-1] : 0;
    if (isdigit(static_cast<unsigned char>(last))) {
        for (;;) {
            STRING numbered=f.filename.Add(f.noFrame);
            if (!FExist(&numbered)) break;
            ++f.noFrame;
        }
    }
    SetSize(static_cast<int>(header.Width),static_cast<int>(header.Height),static_cast<int>(header.ImageBitsPerPixel)/8);
    Rewind();
    return 0;
}

void PICTURE_TGA::Rewind()
{
    PictureBaseLayout& f=Fields(this);
    if (!IsOpened()) return;
    if (f.file) fclose(f.file);
    f.file=FOpen(&f.filename,"rb");
    TGA_HEADER header;
    if (LoadHeader(&header)) return;
    fseek(f.file,f.dataOffset,0);
    f.curFrame=-1;
    NextFrame();
}

void PICTURE_TGA::NextFrame()
{
    PictureBaseLayout& f=Fields(this);
    if (!IsOpened()) {
        Error(E_ERROR,const_cast<char*>("Picture has not opened"),static_cast<unsigned long>(f.curFrame));
        return;
    }
    ++f.curFrame;
    if (f.curFrame>=f.noFrame) {
        Rewind();
        return;
    }
    if (f.file) fclose(f.file);
    STRING numbered=f.filename.Add(f.curFrame);
    f.file=FOpen(&numbered,"rb");
    TGA_HEADER header;
    if (LoadHeader(&header)) return;
    fseek(f.file,f.dataOffset,0);

    if (imageType&8u) {
        const unsigned int bufferSize=static_cast<unsigned int>(f.sizeX*f.sizeY*f.bytesPerPixel);
        unsigned char* encoded=static_cast<unsigned char*>(malloc(bufferSize));
        if (!encoded) {
            Error(E_MEMORY,const_cast<char*>("cadr2"),static_cast<unsigned long>(f.curFrame));
            return;
        }
        fread(encoded,bufferSize,1,f.file);
        int source=0;
        for (int y=f.sizeY-1;y>=0;--y) {
            for (int x=0;x<f.sizeX;) {
                const int run=encoded[source]&0x80;
                int count=(encoded[source]&0x7F)+1;
                ++source;
                while (--count>=0) {
                    memcpy(static_cast<unsigned char*>(f.data)+(x+y*f.sizeX)*f.bytesPerPixel,
                           encoded+source,static_cast<unsigned int>(f.bytesPerPixel));
                    if (!run) source+=f.bytesPerPixel;
                    ++x;
                }
                if (run) source+=f.bytesPerPixel;
            }
        }
        free(encoded);
    } else {
        for (int y=f.sizeY-1;y>=0;--y) {
            fread(static_cast<unsigned char*>(f.data)+y*f.sizeX*f.bytesPerPixel,
                  static_cast<unsigned int>(f.sizeX*f.bytesPerPixel),1,f.file);
        }
    }
}

int PICTURE_BMP::LoadHeader(BMP_HEADER* header)
{
    PictureBaseLayout& f=Fields(this);
    if (!f.file) {
        Error(E_OPEN,const_cast<char*>("bmp cadr"),static_cast<unsigned long>(f.curFrame));
        return 1;
    }
    fread(header,sizeof(*header),1,f.file);
    if (header->Compression) {
        Close();
        Error(E_ERROR,const_cast<char*>("not supported bmp compression type"),0);
        return 1;
    }
    if (header->BitCount<8) {
        Close();
        Error(E_ERROR,const_cast<char*>("not supported bmp 2 and 4 bit type"),0);
        return 1;
    }
    f.dataOffset=static_cast<long>(header->OffBits);
    if (IsOpened()) {
        const int bpp=(static_cast<int>(header->BitCount)+7)/8;
        if (f.sizeX!=static_cast<int>(header->Width) || f.sizeY!=static_cast<int>(header->Height) || f.bytesPerPixel!=bpp) {
            Error(E_ERROR,const_cast<char*>("BMP parameters different from first cadr"),static_cast<unsigned long>(f.curFrame));
            return 1;
        }
    }
    return 0;
}

int PICTURE_BMP::Load(const STRING* filename)
{
    BMP_HEADER header;
    PictureBaseLayout& f=Fields(this);
    if (PICTURE_BASE::Load(filename)) return 1;
    if (LoadHeader(&header)) return 1;
    f.noFrame=1;
    STRING before=filename->Before(".bmp");
    const char last=before.m_buf[0] ? before.m_buf[strlen(before.m_buf)-1] : 0;
    if (isdigit(static_cast<unsigned char>(last))) {
        for (;;) {
            STRING numbered=filename->Add(f.noFrame);
            if (!FExist(&numbered)) break;
            ++f.noFrame;
        }
    }
    SetSize(static_cast<int>(header.Width),static_cast<int>(header.Height),static_cast<int>(header.BitCount)/8);
    Rewind();
    return 0;
}

void PICTURE_BMP::Rewind()
{
    PictureBaseLayout& f=Fields(this);
    if (!IsOpened()) return;
    if (f.file) fclose(f.file);
    f.file=FOpen(&f.filename,"rb");
    BMP_HEADER header;
    if (LoadHeader(&header)) return;
    fseek(f.file,f.dataOffset,0);
    f.curFrame=-1;
    NextFrame();
}

void PICTURE_BMP::NextFrame()
{
    PictureBaseLayout& f=Fields(this);
    if (!IsOpened()) {
        Error(E_ERROR,const_cast<char*>("Picture has not opened"),static_cast<unsigned long>(f.curFrame));
        return;
    }
    ++f.curFrame;
    if (f.curFrame>=f.noFrame) {
        Rewind();
        return;
    }
    if (f.file) fclose(f.file);
    STRING numbered=f.filename.Add(f.curFrame);
    f.file=FOpen(&numbered,"rb");
    BMP_HEADER header;
    if (LoadHeader(&header)) return;
    fseek(f.file,f.dataOffset,0);
    if (f.bytesPerPixel==1)
        fread(f.palette,0x400u,1,f.file);

    const int lineSize=f.sizeX*f.bytesPerPixel;
    unsigned char* line=static_cast<unsigned char*>(f.data)+f.sizeY*lineSize;
    const int padding=(4-lineSize)&3;
    fseek(f.file,f.dataOffset,0);
    int y=f.sizeY;
    while (y>0) {
        line-=lineSize;
        const int readResult=static_cast<int>(fread(line,static_cast<unsigned int>(lineSize),1,f.file));
        if (readResult<0) break;
        if (padding) fseek(f.file,padding,1);
        --y;
    }
}

int PICTURE_Z::Load(const STRING* filename)
{
    PictureBaseLayout& f=Fields(this);
    Z_HEADER header;
    if (PICTURE_BASE::Load(filename)) return 1;
    fread(&header,sizeof(header),1,f.file);
    if (header.Type!=0x6675425Au) {
        Close();
        Error(E_INVALID,const_cast<char*>("Z format file"),0);
        return 1;
    }
    f.noFrame=static_cast<int>(header.NoFrame);
    f.dataOffset=static_cast<long>(sizeof(header));
    f.frameTime=0x47;
    SetSize(static_cast<int>(header.Width),static_cast<int>(header.Height),2);
    Rewind();
    return 0;
}

void PICTURE_Z::NextFrame()
{
    PictureBaseLayout& f=Fields(this);
    if (!IsOpened()) {
        Error(E_ERROR,const_cast<char*>("Picture has not opened"),static_cast<unsigned long>(f.curFrame));
        return;
    }
    ++f.curFrame;
    if (f.curFrame>=f.noFrame) {
        Rewind();
        return;
    }
    for (int y=0;y<f.sizeY;++y) {
        int x=0;
        while (x<f.sizeX) {
            unsigned short count=0;
            fread(&count,2,1,f.file);
            const int run=count&0x7FFF;
            unsigned short* dest=static_cast<unsigned short*>(f.data)+y*f.sizeX+x;
            if (count&0x8000u) {
                for (int i=0;i<run;++i) dest[i]=0x8000u;
            } else {
                fread(dest,2,static_cast<unsigned int>(run),f.file);
            }
            x+=run;
        }
    }
}

int PICTURE::Load(const STRING* filename)
{
    // Retail assumes a valid STRING object, destroys the current picture first,
    // and lets an empty string fall through to TYPE_UNKNOWN/PICTURE_BASE.
    if (picture) {
        delete picture;
        picture=0;
    }
    if (strstr(filename->m_buf,".tga") || strstr(filename->m_buf,".TGA")) {
        type=TYPE_TGA;
        picture=new PICTURE_TGA;
    } else if (strstr(filename->m_buf,".z") || strstr(filename->m_buf,".Z")) {
        type=TYPE_Z;
        picture=new PICTURE_Z;
    } else if (strstr(filename->m_buf,".flc") || strstr(filename->m_buf,".FLC")) {
        type=TYPE_FLC;
        picture=new PICTURE_FLIC;
    } else if (strstr(filename->m_buf,".bmp") || strstr(filename->m_buf,".BMP")) {
        type=TYPE_BMP;
        picture=new PICTURE_BMP;
    } else {
        if (strcmp(filename->m_buf,STRING::EMPTY))
            MYERROR::Log(::Error,"!!!ERROR!!!PICTURE '%s': Unknown format file",filename->m_buf);
        type=TYPE_UNKNOWN;
        picture=new PICTURE_BASE;
    }
    if (type!=TYPE_UNKNOWN)
        return picture->Load(filename);
    return 1;
}

int PICTURE::IsPaletted() { return picture->IsPaletted(); }


int PICTURE_MAKEVID::MakeVid(unsigned int opt,STRING vidname)
{
    RESOURCE out;
    int word;

    if (opt&0x10u)
        vidType|=0x1000u;
    else if (opt&1u)
        vidType|=0x20u;
    if (opt&2u)
        vidType|=0x100u;
    if (opt&4u)
        vidType|=0x20000u;
    if (opt&8u)
        vidType|=0x800u;

    if (texture.IsOpened() && texture.BytesPerPixel()==1)
        vidType|=8u;
    if (vidType&0x20u)
        vidType&=~8u;

    if (!texture.IsOpened() && !alpha.IsOpened()) {
        Error(E_INVALID,const_cast<char*>("not picture3"),0);
        return 1;
    }
    if (!(vidType&4u) && (vidType&0x1000u)) {
        Error(E_ERROR,const_cast<char*>("Unsupported files combination"),0);
        return 1;
    }

    if (!strcmp(vidname.CharPtr(),STRING::EMPTY)) {
        const STRING source=texture.FileName();
        vidname=source.Before(".")+".vid";
    }

    if (out.OpenForWrite(&vidname,0x20444956u)) { // 'VID '
        Error(E_CREATE,const_cast<char*>("file (.vid)"),0);
        return 1;
    }

    out.PreAppend(0x44414548u,0); // HEAD
    vidType|=0x10u;
    word=static_cast<int>(vidType);
    out.Write(&word,2);
    word=texture.FrameTime();
    out.Write(&word,2);
    word=texture.NoFrame();
    out.Write(&word,2);
    word=texture.SizeX();
    out.Write(&word,2);
    word=texture.SizeY();
    out.Write(&word,2);
    out.PostAppend();

    if (opt&0x20u)
        vidType|=0x80000u;
    if (opt&0x40u)
        vidType|=0x100000u;

    if (vidType&0x1000u)
        WritePseudo3d(&out);
    else if (vidType&0x80u)
        WriteLight(&out);
    else if ((vidType&0x20u) && (vidType&2u) && (vidType&4u))
        WriteSoftware(&out);
    else if (vidType&0x20u)
        WriteHardware(&out);
    else
        WriteSoftware(&out);

    out.Close();
    return 0;
}

void PICTURE::NextFrame() { picture->NextFrame(); }
void PICTURE::Rewind() { picture->Rewind(); }

int PICTURE::IsOpened() { return picture->IsOpened(); }

int PICTURE::InViewPort(int x,int y) { return picture->InViewPort(x,y); }

int PICTURE::BytesPerPixel() { return picture->BytesPerPixel(); }
int PICTURE::NoFrame() { return picture->NoFrame(); }

int PICTURE_BASE::InViewPort(int x,int y)
{
    PictureBaseLayout& f=Fields(this);
    return x>=0 && y>=0 && x<f.sizeX && y<f.sizeY;
}


const COLOR* PICTURE::GetPalette() { return picture->GetPalette(); }
int PICTURE::FrameTime() { return picture->FrameTime(); }
void PICTURE::SetPalette(const COLOR* palette) { picture->SetPalette(palette); }
void PICTURE::SetSize(int size_x,int size_y,int bytes_in_pixel) { picture->SetSize(size_x,size_y,bytes_in_pixel); }
int PICTURE::CurFrame() { return picture->CurFrame(); }

const COLOR* PICTURE_MAKEVID::GetPalette() { return texture.GetPalette(); }
int PICTURE_MAKEVID::IsPaletted() { return texture.IsPaletted(); }
int PICTURE_MAKEVID::CurFrame() { return texture.CurFrame(); }
unsigned int PICTURE_MAKEVID::GetData(int x,int y) { return texture.GetData(x,y); }
void PICTURE_MAKEVID::PutData(int x,int y,unsigned int data) { texture.PutData(x,y,data); }

void PICTURE_MAKEVID::Error(TYPE_ERROR type,char* text,unsigned long err)
{
    STRING file=texture.FileName();
    MYERROR::Error(::Error,"PICTURE '%s'",static_cast<int>(type),text,err,file.CharPtr());
}

STRING PICTURE::FileName()
{
    return picture->FileName();
}

STRING PICTURE_BASE::FileName()
{
    return Fields(this).filename;
}

void VID_TEXCOOR::SetCoor(int x0,int y0,int x1,int y1,int next)
{
    nsurf=0;
    next_fragment=next;
    shiftx=x0;
    const int width=x1-x0;
    sizex=width>256?256:width;
    shifty=y0;
    const int height=y1-y0;
    sizey=height>256?256:height;
}

int VID_TEXCOOR::Intersection(int shift_x,int shift_y,int size_x,int size_y)
{
    const int dx=abs((sizex+2*begx)/2-(size_x+2*shift_x)/2);
    if (dx>=(size_x+sizex)/2)
        return 0;
    const int dy=abs((sizey+2*begy)/2-(size_y+2*shift_y)/2);
    return dy<(size_y+sizey)/2;
}

int Distance(int d1,int d2)
{
    return Sqrt(d1*d1+d2*d2);
}

int Sqrt(int val)
{
    int result=0;
    int bit=0x40000000;
    while (bit) {
        if (val>=result+bit) {
            val-=result+bit;
            result=(result>>1)|bit;
        } else {
            result>>=1;
        }
        bit>>=2;
    }
    return result;
}


// constructed before MSVC installs the derived vtable.
PICTURE_FONT::PICTURE_FONT()
    : PICTURE_MAKEVID(), font()
{
}

PICTURE_FONT::~PICTURE_FONT()
{
}

int PICTURE_FONT::Load(STRING file,STRING alphafile,STRING zfile)
{
    Close();
    const int ret=font.Load(file,alphafile,zfile);
    if (ret)
        return ret;

    texture.SetSize(font.SizeX()/16-1,font.SizeY()/16-1,font.texture.BytesPerPixel());
    // Retail writes 256 directly into PICTURE_BASE::noFrame (+0x04).
    *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(texture.picture)+0x04)=256;
    vidType=1;
    if (font.IsPaletted()) {
        texture.SetPalette(font.GetPalette());
        vidType|=8u;
    }
    Rewind();
    *reinterpret_cast<STRING*>(reinterpret_cast<unsigned char*>(texture.picture)+0x1C)=file;
    return ret;
}

void PICTURE_FONT::NextFrame()
{
    PICTURE_MAKEVID::NextFrame();
    const int begx=(CurFrame()%16)*(SizeX()+1);
    const int begy=(CurFrame()/16)*(SizeY()+1);
    for (int y=0;y<SizeY();++y)
        for (int x=0;x<SizeX();++x)
            PutData(x,y,font.GetData(x+begx,y+begy));
}

void PICTURE_FONT::Rewind()
{
    PICTURE_MAKEVID::Rewind();
    for (int y=0;y<SizeY();++y)
        for (int x=0;x<SizeX();++x)
            PutData(x,y,font.GetData(x,y));
}
