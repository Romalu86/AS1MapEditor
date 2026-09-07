#pragma once
// REGION owner. Included in ABI order by mapedit/runtime.hpp.
class REGION : public SPRITE {
public:
    REGION(VID*,float,float,float,ANGLE,SPRITE*);
    ~REGION();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
    virtual void Draw();
    virtual void DrawSecondaryInfo();
    int fogEnd; unsigned int fogTick; unsigned short* fogTable; unsigned int property;
    int fogTop,fogBottom; COLOR fogColor; unsigned long gammaColor;
    float sizeX,sizeY; VID* environmentVid; int windScale;
    VID* sourceVid[6]; VID* conversionVid[6];
    float ScreenLeft(); float ScreenTop(); float ScreenRight(); float ScreenBottom();
    float SizeX(); float SizeY();
    int IsInsideScr(float x0,float y0);
    int IsInsideXY(float x0,float y0);
    static VID* ConvertVid(VID* oldVid,float x,float y,float z);
    void SetSize(float width,float height);
    void SetFogParameters(int newBottom,int newTop,COLOR newColor);
    int DialogSelectVid(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogMapProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogConvertSprite(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogUnitProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogTextProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogRegionProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogOptions(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
};
static_assert(sizeof(REGION)==0xD0, "debug metadata REGION size");

// Original MapEdit TRAIN_INFO debug type (engine.h / engine.cpp).
// debug metadata preserves every field name and the exact 0x40-byte layout.  The first
// dword intentionally remains a pair of bitfields: the retail constructor
// clears only bits 0 and 1 with read/modify/write operations.
