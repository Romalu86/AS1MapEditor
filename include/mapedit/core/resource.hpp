#pragma once
// RESOURCE/RELATION/error type owners. Included in ABI order by mapedit/runtime.hpp.
class RESOURCE : public STREAM {
public:
    enum OPEN_MODE { OpenRead=0, OpenAppend=1, OpenWrite=2 };
    // Exact debug metadata/reference implementation layout for canonical MapEdit.exe (RESOURCE size 0x40).
    // STREAM contributes vfptr at +0x00.  Keeping STRING as a real member is
    // required: retail ctor/dtor invoke STRING ctor/dtor automatically at +0x0C.
    uint32_t flags;               // +0x04
    int state;                    // +0x08
    STRING filename;              // +0x0C
    uint32_t signature;           // +0x10
    int size;                     // +0x14
    int pos;                      // +0x18
    int begin;                    // +0x1C
    int end;                      // +0x20
    uint32_t subFlags;            // +0x24
    int noSubRes;                 // +0x28 (debug metadata: int sub_no)
    int subPos;                   // +0x2C
    int subSize;                  // +0x30 (debug metadata: int sub_size)
    int packedDiff;               // +0x34 (debug metadata: int packed_diff)
    FILE* file;                   // +0x38
    uint32_t type;                // +0x3C
    RESOURCE();
    RESOURCE(OPEN_MODE mode,const STRING* name,unsigned int resType);
    virtual ~RESOURCE();
    virtual int Read(void* data,unsigned int size);
    virtual int Write(const void* data,unsigned int size);
    int Open(FILE* file,unsigned int resType);
    int OpenForRead(const STRING* name,unsigned int resType);
    int OpenForWrite(const STRING* name,unsigned int resType);
    int OpenForAppend(const STRING* name,unsigned int resType);
    int GoBegin(unsigned int type);
    int GoNext(unsigned int type);
    int GoNextSub(unsigned int type);
    int Seek(int shift);
    void Error(int type,char* text,unsigned long err);
    int Shift(int shift);
    void Close();
    int IsOpen() const;
    int LoadIni(const STRING* filename);
    int WriteFile(const STRING* filename);
    int WritePacked(const void* data,unsigned int size,FILTER* packer);
    int ReadPacked(void* data,unsigned int size,FILTER* packer);
    int SubLoad(void** data,FILTER* packer);
    int Load(unsigned int type,void** data,int elem_size);
    int GetNoSubRes(unsigned int type);
    int SubSize();
    int PreAppend(unsigned int typ,FILTER* packer);
    void PostAppend();
    int Copy(RESOURCE* res,unsigned int typ);
    int Tell();
    int ResSize();
    STRING GetFileName() const;
};
class RELATION {
public:
    RELATION();
    LIST<SPRITE*> oldSprites;
    LIST<SPRITE*> newSprites;
    ~RELATION();
    void Release();
    SPRITE* Decode(SPRITE* old_sprite);
    void Insert(SPRITE* old_sprite,SPRITE* new_sprite);
};
static_assert(sizeof(RESOURCE)==0x40, "debug metadata RESOURCE size");
static_assert(sizeof(RELATION)==0x20, "debug metadata RELATION size");

enum TYPE_ERROR {
    E_LOCK=0, E_COPY=1, E_MEMORY=2, E_CREATE=3, E_INVALID=4,
    E_LOAD=5, E_SAVE=6, E_OPEN=7, E_SET=8, E_GET=9,
    E_ERROR=10, E_SECTION=11, E_INIT=12, E_MISSING=13, E_UNKNOWN=14
};

