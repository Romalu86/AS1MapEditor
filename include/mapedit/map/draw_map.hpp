#pragma once
// DRAW_MAP owner. Included in ABI order by mapedit/runtime.hpp.
class DRAW_MAP {
public:
    union {
        uint32_t optionBits;
        struct {
            unsigned int optDrawBackGround:1;
            unsigned int optionUnused:31;
        };
    };
    float begx;
    float begy;
    float sizex;
    float sizey;

    explicit DRAW_MAP(SPRITE* map);
    DRAW_MAP(float bx,float by,float sx,float sy);
    int IsInside(float x,float y);
    float FromScreenX(float x);
    float FromScreenY(float y);
    float X(float x);
    float Y(float y);
    void DrawDot(float x,float y,COLOR color);
    void DrawDot2x2(float x,float y,COLOR color);
    void DrawDot4x4(float x,float y,COLOR color);
    int Control(INPUT* input);
    int IsClickOnMap(const INPUT* input);
    void ClickOnMap(const INPUT* input);
    void Draw();
};
static_assert(sizeof(DRAW_MAP)==0x14, "debug metadata DRAW_MAP size");

