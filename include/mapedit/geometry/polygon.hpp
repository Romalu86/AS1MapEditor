#pragma once
// POINTLIST/POLYGON/CRC32 owners. Included in ABI order by mapedit/runtime.hpp.
class POINTLIST {
public:
    float x;
    float y;
    POINTLIST* next;
};
static_assert(sizeof(POINTLIST)==12, "debug metadata POINTLIST size");
static_assert(offsetof(POINTLIST,x)==0x0, "debug metadata POINTLIST::x offset");
static_assert(offsetof(POINTLIST,y)==0x4, "debug metadata POINTLIST::y offset");
static_assert(offsetof(POINTLIST,next)==0x8, "debug metadata POINTLIST::next offset");

class POLYGON {
public:
    POINTLIST* head;
    int closed;
    int noPoint;
    POLYGON();
    ~POLYGON();
    int NoPoint();
    int AskInside(float x,float y);
    POINTLIST* CreatePoint(float x,float y);
    void AddLinedPoint(float x,float y);
    void Closed();
    void CreateBox(float x0,float y0,float x1,float y1);
    void Draw(COLOR color);
    void Release();
};
static_assert(sizeof(POLYGON)==12, "debug metadata POLYGON size");

class CRC32 {
public:
    CRC32();
    CRC32(void* buf,unsigned int count);
    operator unsigned int();
    unsigned int Add(void* buf,unsigned int count);
private:
    unsigned int crc;
    static const unsigned int crc_table[256];
};
static_assert(sizeof(CRC32)==4, "debug metadata CRC32 size");

