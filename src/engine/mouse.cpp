#include "mapedit/runtime.hpp"

// Hardware cursor transitions from mouse.cpp:87..150. Enable/Disable and the
// general SPRITE animation/direction routines remain their own derived owners.
void MOUSE::HardwareOn()
{
    if (Hardware)
        return;
    Disable();
    m_vid = EmptyVid;
    Hardware = 1;
    Enable();
}

void MOUSE::HardwareOff()
{
    if (!Hardware)
        return;
    Disable();
    m_vid = Map->Vid(1);
    const int animation = Animation();
    SPRITE::ChangeAnimation(animation == 0);
    SPRITE::ChangeAnimation(animation);
    Hardware = 0;
    Enable();
}

void MOUSE::ChangeAnimation(int newAnimation)
{
    if (Hardware) {
        if (Animation() != newAnimation && Visible)
            SetCursor(hCursor[newAnimation]);
        if (newAnimation < 17)
            SPRITE::ChangeAnimation(newAnimation);
        else
            m_ani = newAnimation;
        return;
    }
    SPRITE::ChangeAnimation(newAnimation);
}

void MOUSE::ChangeDirection(ANGLE newDirection)
{
    SPRITE::ChangeDirection(newDirection);
    if (HaveLink())
        Link()->ChangeDirection(newDirection);
}

void MOUSE::Disable()
{
    if (!Visible)
        return;
    Visible = 0;
    if (!Hardware)
        return;

    SetCursor(0);
    for (int i=0; i<36; ++i) {
        if (hCursor[i]) {
            DestroyCursor(hCursor[i]);
            hCursor[i] = 0;
        }
    }
}

void MOUSE::Enable()
{
    static const char* const cursorName[36] = {
        "arrow", "noammo", "move", "clash", "repair", "attack",
        "farattack", "select", "nomove", "cycle", "link", "unlink",
        "cantmove", "patrol", "delete", "capture", "mine", "unmine",
        "small-arrow", "small-noammo", "small-move", "small-taran",
        "small-repair", "small-attack", "small-farattack", "small-select",
        "small-nomove", "cycle", "small-link", "small-unlink",
        "small-cantmove", "small-patrol", "delete", "small-capture",
        "small-mine", "small-unmine"
    };

    if (Visible)
        return;
    Visible=1;
    SetCursor(0);
    if (!Hardware)
        return;

    for (int i=0; i<36; ++i) {
        if (hCursor[i]) {
            DestroyCursor(hCursor[i]);
            hCursor[i]=0;
        }

        if (cursorName[i] && cursorName[i][0]) {
            STRING filename("cursores\\");
            filename += cursorName[i];
            filename += ".ani";
            hCursor[i]=LoadCursorFromFileA(filename.CharPtr());
        }
        if (!hCursor[i])
            hCursor[i]=LoadCursorA(0,reinterpret_cast<const char*>(0x7F00));
    }
    SetCursor(hCursor[Animation()]);
}

int MOUSE::IsHardware()
{
    return Hardware;
}


// Retail constructor installs the MOUSE vtable, starts disabled/hidden, keeps
// hardware-cursor mode selected and removes the sprite/link from the hash
// ownership path until Enable() is called by MAP startup.
MOUSE::MOUSE(VID* nvid,float xx,float yy,float zz,ANGLE dir,SPRITE* parent)
    : SPRITE(nvid,xx,yy,zz,dir,parent), Hardware(1), Visible(0)
{
    for (int i=0; i<36; ++i)
        hCursor[i]=0;

    if (m_vid!=EmptyVid)
        Remove();
    else if (HaveLink())
        Link()->Remove();

    if (m_vid==EmptyVid)
        AddRef();
}

MOUSE::~MOUSE()
{
    MYERROR::Log(::Error,"Mouse  release");
}

// Retail slot +0x00 @ 0x00465420. Kept explicit because SPRITE models the
// deleting-destructor slot directly rather than as a C++ virtual destructor.
void* MOUSE::ScalarDeletingDestructor(unsigned int flags)
{
    MOUSE* result=this;
    this->MOUSE::~MOUSE();
    if (flags&1u)
        ::operator delete(result);
    return result;
}

int MOUSE::Action(int action,int a,int b,int c)
{
    switch (action) {
    case 0x3d:
        ChangeAnimation(a);
        return 0;

    case 0x3f: {
        RECT_OLD rect={0,0,0,0};
        GetWindowRect(Graph->Wnd(),&rect);
        SetCursorPos(a+rect.left,b+rect.top);
        return 0;
    }

    case 0x3e:
        if (!Map->ValidateVid(a))
            return 0;
        if (m_vid && a==m_vid->m_idx)
            return 0;

        Insert();
        SPRITE::Action(0x3e,a,b,c);
        for (SPRITE* child=this; child; child=child->Link())
            Hash->Delete(child);
        Remove();
        return 0;

    default:
        return SPRITE::Action(action,a,b,c);
    }
}

void MOUSE::Draw()
{
    if (!Hardware && Visible && m_vid) {
        void** vtable=*reinterpret_cast<void***>(m_vid);
        typedef void (__thiscall *DrawMethod)(VID*,const SPRITE*);
        reinterpret_cast<DrawMethod>(vtable[0x0c/4])(m_vid,this);
    }
}

int MOUSE::IsDisable()
{
    return Visible==0;
}
