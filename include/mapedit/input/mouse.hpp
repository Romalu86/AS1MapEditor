#pragma once
// MOUSE owner. Included in ABI order by mapedit/runtime.hpp.
class MOUSE : public SPRITE {
public:
    MOUSE(VID* nvid,float xx,float yy,float zz,ANGLE dir,SPRITE* parent);
    ~MOUSE();
    void* ScalarDeletingDestructor(unsigned int flags) override;
    int Action(int action,int a,int b,int c) override;
    void Draw() override;
    int Hardware;
    void* hCursor[36];
    int Visible;
    void Enable();
    void Disable();
    void HardwareOn();
    void HardwareOff();
    void ChangeAnimation(int newAnimation);
    void ChangeDirection(ANGLE newDirection);
    int IsHardware();
    int IsDisable();
};
static_assert(sizeof(MOUSE)==0x108, "debug metadata MOUSE size");

