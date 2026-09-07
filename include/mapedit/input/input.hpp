#pragma once
// INPUT owner. Included in ABI order by mapedit/runtime.hpp.
class INPUT {
public:
    INPUT();
    union {
        uint32_t stateBits;
        struct {
            unsigned int lClick:1, mClick:1, rClick:1, lUp:1, rUp:1, lDown:1, rDown:1;
            unsigned int left:1, right:1, down:1, up:1, shift:1, ctrl:1, alt:1, first:1, second:1;
            unsigned int stateUnused:16;
        };
    };
    int mouseWheel;
    float mouseX,mouseY;
    float screenMouseX,screenMouseY;
    unsigned int key,vkKey;
    int WorkWndMessage(HWND__* hwnd,unsigned long msg,unsigned long wParam,unsigned long lParam);
    void ChangeCoor(float screen_x,float screen_y);
    void Tact();
    void Save(STREAM* res);
    void Load(STREAM* res);
    void ClearLClick();
    void ClearRClick();
    unsigned int GetState();
    static unsigned int StringToKey(STRING key);
};
static_assert(sizeof(INPUT)==0x20, "debug metadata INPUT size");
