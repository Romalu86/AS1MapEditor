#pragma once
// HASH_MAP owner. Included in ABI order by mapedit/runtime.hpp.
class HASH_MAP {
public:
    HASH_MAP(float size_x,float size_y,MAP* owner,int no_vid);
    ~HASH_MAP();
    int begx,begy,endx,endy,curx,curindex,curUnit,shiftY,noX,noY;
    float scaleCellX,scaleCellY;
    SPRITE_LIST* map;
    SPRITE_LIST units;
    int ConvX(float x);
    int ConvY(float y);
    SPRITE* FirstUnit(int* index);
    SPRITE* NextUnit(int* index);
    SPRITE* FirstUnit();
    SPRITE* NextUnit();
    SPRITE* FirstInBox(float left,float top,float right,float bottom);
    SPRITE* NextInBox();
    SPRITE* CanPlace(const VID* vid,float x,float y,float z);
    int AskLine(const VID* vid,float x,float y,float z,float* endx,float* endy,float* endz);
    void Insert(SPRITE* spr);
    int Delete(SPRITE* spr);
    void ChangeCoor(SPRITE* spr,float x,float y);
};
static_assert(sizeof(HASH_MAP)==0x44, "debug metadata HASH_MAP size");

