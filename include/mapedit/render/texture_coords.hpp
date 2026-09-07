#pragma once
// VID_TEXCOOR owner. Included in ABI order by mapedit/runtime.hpp.
struct VID_TEXCOOR {
    int shadow_shift;      // +0x00
    int nsurf;             // +0x04
    int begx;              // +0x08
    int begy;              // +0x0C
    int sizex;             // +0x10
    int sizey;             // +0x14
    int shiftx;            // +0x18
    int shifty;            // +0x1C
    int next_fragment;     // +0x20
    void SetCoor(int begx,int begy,int endx,int endy,int next);
    int Intersection(int shift_x,int shift_y,int size_x,int size_y);
    void Read(STREAM* res,int new_version);
};
static_assert(sizeof(VID_TEXCOOR)==0x24, "debug metadata VID_TEXCOOR size");

int Distance(int d1,int d2);
float Distance(float d1,float d2);
int Distance(int d1,int d2,int d3);
float Distance(float d1,float d2,float d3);
int Square(int val);
int Sqrt(int val);

