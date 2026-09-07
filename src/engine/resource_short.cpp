#include "mapedit/runtime.hpp"

namespace {
int& ResourceState(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x08);
}
void*& ResourceFile(RESOURCE* resource)
{
    return *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(resource) + 0x38);
}
void* ResourceFile(const RESOURCE* resource)
{
    return *reinterpret_cast<void* const*>(reinterpret_cast<const uint8_t*>(resource) + 0x38);
}
uint32_t& ResourceFlags(RESOURCE* resource)
{
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource) + 0x04);
}
STRING& ResourceFilename(RESOURCE* resource)
{
    return *reinterpret_cast<STRING*>(reinterpret_cast<uint8_t*>(resource) + 0x0C);
}
int& ResourceSize(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x14);
}
int& ResourcePos(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x18);
}
int& ResourceBegin(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x1C);
}
int& ResourceEnd(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x20);
}
}

int RESOURCE::IsOpen() const
{
    return ResourceFile(this) != 0;
}

STRING RESOURCE::GetFileName() const
{
    return *reinterpret_cast<const STRING*>(reinterpret_cast<const uint8_t*>(this)+0x0C);
}

int RESOURCE::Shift(int shift)
{
    ResourceState(this) = 2;
    return fseek(ResourceFile(this), shift, 1);
}

int RESOURCE::Read(void* data,unsigned int size)
{
    if (!IsOpen() || !size)
        return static_cast<int>(size);
    if (ResourceState(this) == 0)
        Shift(0);
    ResourceState(this) = 1;
    const unsigned int read = fread(data, 1u, size, ResourceFile(this));
    return static_cast<int>(size - read);
}


RESOURCE::RESOURCE()
{
    ResourceFile(this) = 0;
    ResourcePos(this) = 0;
    ResourceSize(this) = 0;
    ResourceBegin(this) = 0;
    ResourceEnd(this) = 0;
    ResourceFlags(this) &= ~1u;
    ResourceState(this) = 2;
}

void RESOURCE::Close()
{
    if (IsOpen()) {
        if (ResourceFlags(this) & 1u) {
            Seek(ResourceBegin(this)+4);
            ResourceEnd(this)-=ResourceBegin(this)+8;
            Write(&ResourceEnd(this),4u);
        }
        fclose(ResourceFile(this));
    }
    ResourceFile(this) = 0;
    ResourceEnd(this) = 0;
    ResourceFilename(this) = "Not opened";
}

RESOURCE::~RESOURCE()
{
    Close();
}

namespace {
uint32_t& ResourceSignature(RESOURCE* resource)
{
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource) + 0x10);
}
uint32_t& ResourceType(RESOURCE* resource)
{
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource) + 0x3C);
}
uint32_t& ResourceSubFlags(RESOURCE* resource)
{
    return *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(resource) + 0x24);
}
int& ResourceNoSubRes(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x28);
}
int& ResourceSubPos(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x2C);
}
int& ResourceSubSize(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x30);
}
int& ResourcePackedPos(RESOURCE* resource)
{
    return *reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(resource) + 0x34);
}
long FileLengthRetail(FILE* file)
{
    // directly.  Do not emulate it with fseek/ftell: fseek mutates CRT stream
    // state (including EOF) and is not retail-equivalent.
    if (!file)
        return 0;
    // VC6 exposed FILE::_file directly; UCRT does not.  _fileno(FILE*) is the
    // modern ABI boundary equivalent and, unlike fseek/ftell, preserves stream
    // position/state exactly as retail's _filelength(file->_file) path.
    return _filelength(_fileno(file));
}
}

int RESOURCE::Write(const void* data,unsigned int size)
{
    if (!IsOpen() || !size)
        return static_cast<int>(size);
    if (ResourceState(this)==1)
        Shift(0);
    ResourceState(this)=0;
    const unsigned int written=fwrite(data,1u,size,static_cast<FILE*>(ResourceFile(this)));
    return static_cast<int>(size-written);
}

int RESOURCE::WritePacked(const void* data,unsigned int size,FILTER* packer)
{
    if (!IsOpen() || !size)
        return static_cast<int>(size);

    if (!packer)
        return Write(data,size);

    const int written=packer->Encode(data,size,static_cast<FILE*>(ResourceFile(this)));
    ResourcePackedPos(this)+=static_cast<int>(size)-written;
    return 0;
}

void RESOURCE::Error(int type,char* text,unsigned long err)
{
    if (::Error)
        MYERROR::Error(::Error,"RES '%s' '%.4s'",type,text,err,
                       ResourceFilename(this).m_buf,&ResourceType(this));
}

int RESOURCE::GoBegin(unsigned int type)
{
    if (!IsOpen())
        return 1;
    ResourcePos(this)=ResourceBegin(this)+4;
    ResourceSize(this)=0;
    return GoNext(type);
}

int RESOURCE::Seek(int shift)
{
    ResourceState(this)=2;
    return fseek(ResourceFile(this),shift,0);
}

int RESOURCE::GoNext(unsigned int type)
{
    if (!IsOpen())
        return 1;

    for (;;) {
        const int advance=((ResourceSize(this)+1)&~1)+8;
        ResourcePos(this)+=advance;
        Seek(ResourcePos(this));

        if (ResourcePos(this)>=ResourceEnd(this)) {
            ResourcePos(this)-=advance;
            if (ResourceSignature(this)==0x20534552u)
                Seek(ResourcePos(this)+0x18);
            else
                Seek(ResourcePos(this)+8);
            return 2;
        }

        Read(&ResourceType(this),4u);
        Read(&ResourceSize(this),4u);

        if (ResourceSignature(this)==0x20534552u) {
            Read(&ResourceSubFlags(this),4u);
            if (ResourceSubFlags(this)&0x80000000u) {
                Read(&ResourcePackedPos(this),4u);
                Read(&ResourceNoSubRes(this),4u);
            } else {
                ResourceNoSubRes(this)=ResourceSubFlags(this);
                ResourceSubFlags(this)=0;
            }
            ResourceSubPos(this)=static_cast<int>(ftell(static_cast<FILE*>(ResourceFile(this))));
            Read(&ResourceSubSize(this),4u);
        }

        if (type==ResourceType(this) || type==0x20594E41u)
            return 0;
    }
}

int RESOURCE::Open(FILE* file,unsigned int resType)
{
    if (IsOpen())
        Close();
    ResourceFile(this)=file;
    if (!file) {
        Error(7,const_cast<char*>("file is NULL"),0);
        return 1;
    }

    ResourceBegin(this)=static_cast<int>(ftell(file));
    uint32_t signature=0;
    if (Read(&signature,4u)) {
        Error(5,const_cast<char*>("empty file"),0);
        Close();
        return 4;
    }
    ResourceSignature(this)=signature;
    if (signature!=0x20534552u && signature!=0x46464952u) {
        Error(4,const_cast<char*>("resource signature"),0);
        Close();
        return 2;
    }

    Read(&ResourceEnd(this),4u);
    ResourceEnd(this)+=ResourceBegin(this)+8;
    if (FileLengthRetail(file)<ResourceEnd(this))
        Error(10,const_cast<char*>("Invalid filelength"),
              static_cast<unsigned long>(FileLengthRetail(file)-ResourceEnd(this)));

    uint32_t actualType=0;
    Read(&actualType,4u);
    if (actualType!=resType && resType!=0x20594E41u) {
        ResourceType(this)=actualType;
        Error(4,const_cast<char*>("resource type"),0);
        Close();
        return 3;
    }
    GoBegin(0x20594E41u);
    return 0;
}

int RESOURCE::OpenForRead(const STRING* name,unsigned int resType)
{
    if (IsOpen())
        Close();
    FILE* file=FOpen(name,"rb");
    if (!file) {
        Error(7,const_cast<STRING*>(name)->CharPtr(),0);
        return 1;
    }
    ResourceFilename(this)=name;
    return Open(file,resType);
}

int RESOURCE::OpenForWrite(const STRING* name,unsigned int resType)
{
    if (IsOpen())
        Close();
    FILE* file=FOpen(name,"w+b");
    ResourceFile(this)=file;
    if (!file) {
        Error(3,const_cast<STRING*>(name)->CharPtr(),0);
        return 1;
    }
    ResourceBegin(this)=0;
    ResourceEnd(this)=12;
    ResourceType(this)=0;
    ResourceFilename(this)=name;
    ResourceSignature(this)=0x20534552u;
    Write(&ResourceSignature(this),4u);
    uint32_t size=4;
    Write(&size,4u);
    Write(&resType,4u);
    GoBegin(0x20594E41u);
    return 0;
}

int RESOURCE::OpenForAppend(const STRING* name,unsigned int resType)
{
    if (IsOpen())
        Close();
    FILE* file=FOpen(name,"r+b");
    ResourceFilename(this)=name;
    if (file)
        return Open(file,resType);
    return OpenForWrite(name,resType);
}

RESOURCE::RESOURCE(OPEN_MODE mode,const STRING* name,unsigned int resType)
{
    ResourceFile(this)=0;
    ResourcePos(this)=0;
    ResourceSize(this)=0;
    ResourceBegin(this)=0;
    ResourceEnd(this)=0;
    ResourceFlags(this)&=~1u;
    ResourceState(this)=2;

    if (mode==OpenAppend)
        OpenForAppend(name,resType);
    else if (mode==OpenWrite)
        OpenForWrite(name,resType);
    else
        OpenForRead(name,resType);
}

void RESOURCE::PostAppend()
{
    ResourceFlags(this)|=1u;
    ResourceSubFlags(this)|=0x80000000u;
    ++ResourceNoSubRes(this);

    ResourceSubSize(this)=static_cast<int>(ftell(static_cast<FILE*>(ResourceFile(this)))) - ResourceSubPos(this) - 4;
    ResourceSize(this)+=ResourceSubSize(this)+4;
    ResourceEnd(this)+=ResourceSubSize(this)+4;

    Seek(ResourceSubPos(this));
    ResourceSubPos(this)+=ResourceSubSize(this)+4;
    Write(&ResourceSubSize(this),4u);

    ResourceState(this)=2;
    Seek(ResourcePos(this));
    Write(&ResourceType(this),4u);
    Write(&ResourceSize(this),4u);
    Write(&ResourceSubFlags(this),4u);
    Write(&ResourcePackedPos(this),4u);
    Write(&ResourceNoSubRes(this),4u);
    ResourceSubFlags(this)&=~0x100u;
}

STREAM::STREAM() {}

STREAM::~STREAM() {}

int RESOURCE::PreAppend(unsigned int typ,FILTER* packer)
{
    if (!IsOpen())
        return -1;

    if (packer)
        ResourceSubFlags(this)|=0x100u;

    while (GoNext(0x20594E41u)==0) {
    }

    if (ResourceType(this)!=typ) {
        ResourceEnd(this)+=0x14;
        ResourceType(this)=typ;
        ResourceNoSubRes(this)=0;
        ResourceSubFlags(this)=0;
        // Retail 0x004119A2..0x004119B8 advances from the current section
        // position (+0x18), not from RESOURCE::m_begin (+0x1C).  Using m_begin
        // corrupts every generated container after the first append.
        ResourcePos(this)=ResourcePos(this)+((ResourceSize(this)+1)&~1)+8;
        ResourceSize(this)=0x0C;
    }

    ResourceSubPos(this)=ResourcePos(this)+ResourceSize(this)+8;
    ResourceSubSize(this)=0;
    ResourcePackedPos(this)=0;
    Seek(ResourceSubPos(this)+4);
    return 0;
}

extern "C" void* __cdecl malloc(unsigned int);
extern "C" void __cdecl free(void*);

int RESOURCE::Copy(RESOURCE* res,unsigned int typ)
{
    if (!IsOpen() || !res->IsOpen())
        return 1;

    if (res->GoBegin(typ)!=0)
        return 0;

    do {
        PreAppend(typ,0);
        ResourceNoSubRes(this)=0;
        const unsigned int size=static_cast<unsigned int>(ResourceSize(res));
        ResourceNoSubRes(this)+=ResourceNoSubRes(res);

        void* data=malloc(size);
        if (!data)
            return 1;
        res->Read(data,size);
        Write(data,size);
        free(data);
    } while (res->GoNext(typ)==0);

    --ResourceNoSubRes(this);
    PostAppend();
    return 0;
}

int RESOURCE::Tell()
{
    return static_cast<int>(ftell(ResourceFile(this)));
}

int RESOURCE::ResSize()
{
    return ResourceSize(this);
}

int RESOURCE::GoNextSub(unsigned int type)
{
    if (!IsOpen())
        return -1;
    ResourceSubPos(this) += ResourceSubSize(this) + 4;
    if (ResourceSubPos(this) >= ResourcePos(this) + ResourceSize(this) + 8)
        return GoNext(type);
    Seek(ResourceSubPos(this));
    Read(&ResourceSubSize(this),4u);
    return 0;
}


// Exact VC6 RESOURCE::ReadPacked control flow. FILTER::Decode is virtual slot +0x24.
int RESOURCE::ReadPacked(void* data,unsigned int size,FILTER* packer)
{
    if (!IsOpen() || size == 0u)
        return static_cast<int>(size);
    if (!packer)
        return Read(data,size);
    return static_cast<int>(size) - packer->Decode(data,size,static_cast<FILE*>(ResourceFile(this)));
}

// The original MapEdit body does not consult the FILTER* argument: it allocates
// exactly the current subresource payload and dispatches RESOURCE::Read directly.
int RESOURCE::SubLoad(void** data,FILTER* packer)
{
    (void)packer;
    if (!IsOpen()) {
        Error(5,const_cast<char*>("file not opened"),0);
        return 0;
    }
    if (ResourceSubSize(this) <= 0) {
        Error(11,const_cast<char*>("SubLoad"),0);
        return 0;
    }

    void* const block = malloc(static_cast<unsigned int>(ResourceSubSize(this)));
    *data = block;
    if (!block) {
        Error(2,const_cast<char*>("Subload data"),static_cast<unsigned long>(ResourceSubSize(this)));
        return 0;
    }
    if (Read(block,static_cast<unsigned int>(ResourceSubSize(this))) != 0)
        Error(5,const_cast<char*>("Subload"),0);
    return ResourceSubSize(this);
}

int RESOURCE::SubSize()
{
    return ResourceSubSize(this);
}

int RESOURCE::GetNoSubRes(unsigned int type)
{
    // RESOURCE x86 layout: compiler vfptr +0x00, file +0x38, sub_no +0x28.
    FILE* const file=*reinterpret_cast<FILE**>(reinterpret_cast<unsigned char*>(this)+0x38);
    if (!file)
        return 0;
    int count=0;
    if (GoBegin(type)==0) {
        do {
            count+=*reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(this)+0x28);
        } while (GoNext(type)==0);
    }
    return count;
}

int RESOURCE::Load(unsigned int type,void** data,int elem_size)
{
    if (!IsOpen()) {
        Error(5,const_cast<char*>("file not opened"),0);
        exit(1);
    }
    const int count=GetNoSubRes(type);
    if (!count) {
        Error(11,const_cast<char*>("Load"),type);
        exit(1);
    }
    GoBegin(type);
    if (!*data)
        *data=::operator new(static_cast<unsigned int>(elem_size*count));
    else
        Error(5,const_cast<char*>("Already loaded"),0);
    if (!*data)
        MYERROR::LogExit(::Error,"ResLoad::type=%.4s no_sub=%i Not enough Memory",
                         reinterpret_cast<const char*>(&type),count);
    for (int i=0;i<count;++i) {
        Read(reinterpret_cast<unsigned char*>(*data)+i*elem_size,static_cast<unsigned int>(elem_size));
        GoNextSub(type);
    }
    return count;
}
