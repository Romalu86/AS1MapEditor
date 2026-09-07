#include "mapedit/runtime.hpp"

namespace {
enum : unsigned long {
    WM_MOVE_OLD = 0x0003,
    WM_ACTIVATEAPP_OLD = 0x001C,
    WM_NCHITTEST_OLD = 0x0084,
    WM_KEYDOWN_OLD = 0x0100,
    WM_KEYUP_OLD = 0x0101,
    WM_CHAR_OLD = 0x0102,
    WM_LBUTTONDOWN_OLD = 0x0201,
    WM_LBUTTONUP_OLD = 0x0202,
    WM_RBUTTONDOWN_OLD = 0x0204,
    WM_RBUTTONUP_OLD = 0x0205,
    WM_MBUTTONDOWN_OLD = 0x0207,
    WM_MOUSEWHEEL_OLD = 0x020A
};

inline bool IsEitherKey(unsigned long key,int first,int second)
{
    return key==static_cast<unsigned long>(first) || key==static_cast<unsigned long>(second);
}
}

// This is the retail Win32 input router.  The key/mouse binding globals are
// writable configuration state in the original image, so they are kept as
// globals rather than folded to their retail startup constants.
int INPUT::WorkWndMessage(HWND__* hwnd,unsigned long msg,unsigned long wParam,unsigned long lParam)
{
    switch (msg) {
    case WM_MOVE_OLD: {
        RECT_OLD rect;
        GetWindowRect(hwnd,&rect);
        g_windowScreenX=static_cast<int>(rect.left);
        g_windowScreenY=static_cast<int>(rect.top);
        break;
    }
    case WM_NCHITTEST_OLD: {
        RECT_OLD rect;
        GetWindowRect(hwnd,&rect);

        // Retail masks both halves to 16 bits and does not sign-extend them.
        float sx=static_cast<float>(static_cast<unsigned int>(lParam)&0xFFFFu)-static_cast<float>(rect.left);
        float sy=static_cast<float>((static_cast<unsigned int>(lParam)>>16)&0xFFFFu)-static_cast<float>(rect.top);
        g_windowScreenX=static_cast<int>(rect.left);
        g_windowScreenY=static_cast<int>(rect.top);
        screenMouseX=sx;
        screenMouseY=sy;

        if (sx < Graph->ViewXMin())
            sx=Graph->ViewXMin();
        if (sx >= Graph->ViewXMax())
            sx=Graph->ViewXMax()-1.0f;
        if (sy < Graph->ViewYMin())
            sy=Graph->ViewYMin();
        if (sy >= Graph->ViewYMax())
            sy=Graph->ViewYMax()-1.0f;

        mouseX=Map->FromScreenX(sx);
        mouseY=Map->FromScreenY(sy);
        Mouse->ChangeCoor(mouseX,mouseY,Mouse->Z());
        if (Graph->InViewPort(screenMouseX,screenMouseY))
            return 1;
        break;
    }
    case WM_MOUSEWHEEL_OLD:
        mouseWheel=static_cast<short>((wParam>>16)&0xFFFFu)/120;
        break;

    case WM_LBUTTONDOWN_OLD:
        stateBits |= 0x20u; // lDown
        stateBits |= 0x01u; // lClick
        if (g_inputFirstPrimary==1)
            stateBits |= 0x4000u;
        if (g_inputSecondPrimary==1)
            stateBits |= 0x8000u;
        break;
    case WM_RBUTTONDOWN_OLD:
        stateBits |= 0x40u; // rDown
        stateBits |= 0x04u; // rClick
        if (g_inputFirstPrimary==2)
            stateBits |= 0x4000u;
        if (g_inputSecondPrimary==2)
            stateBits |= 0x8000u;
        break;
    case WM_MBUTTONDOWN_OLD:
        stateBits |= 0x02u;
        break;
    case WM_LBUTTONUP_OLD:
        stateBits &= ~0x20u;
        stateBits |= 0x08u;
        if (g_inputFirstPrimary==1 && g_inputAllowFirst)
            stateBits &= ~0x4000u;
        if (g_inputSecondPrimary==1 && g_inputAllowSecond)
            stateBits &= ~0x8000u;
        break;
    case WM_RBUTTONUP_OLD:
        stateBits &= ~0x40u;
        stateBits |= 0x10u;
        if (g_inputFirstPrimary==2 && g_inputAllowFirst)
            stateBits &= ~0x4000u;
        if (g_inputSecondPrimary==2 && g_inputAllowSecond)
            stateBits &= ~0x8000u;
        break;

    case WM_KEYDOWN_OLD:
        key=wParam<<8;
        vkKey=wParam;
        if (IsEitherKey(wParam,g_inputKeyLeftPrimary,g_inputKeyLeftSecondary))
            stateBits |= 0x0080u;
        else if (IsEitherKey(wParam,g_inputKeyRightPrimary,g_inputKeyRightSecondary))
            stateBits |= 0x0100u;
        else if (IsEitherKey(wParam,g_inputKeyUpPrimary,g_inputKeyUpSecondary))
            stateBits |= 0x0400u;
        else if (IsEitherKey(wParam,g_inputKeyDownPrimary,g_inputKeyDownSecondary))
            stateBits |= 0x0200u;
        else if (IsEitherKey(wParam,g_inputFirstPrimary,g_inputFirstSecondary))
            stateBits |= 0x4000u;
        else if (IsEitherKey(wParam,g_inputSecondPrimary,g_inputSecondSecondary))
            stateBits |= 0x8000u;
        else if (wParam==0x10u)
            stateBits |= 0x0800u;
        else if (wParam==0x11u)
            stateBits |= 0x1000u;
        break;
    case WM_CHAR_OLD:
        key=wParam&0xFFu;
        break;
    case WM_KEYUP_OLD:
        if (IsEitherKey(wParam,g_inputKeyLeftPrimary,g_inputKeyLeftSecondary))
            stateBits &= ~0x0080u;
        else if (IsEitherKey(wParam,g_inputKeyRightPrimary,g_inputKeyRightSecondary))
            stateBits &= ~0x0100u;
        else if (IsEitherKey(wParam,g_inputKeyUpPrimary,g_inputKeyUpSecondary))
            stateBits &= ~0x0400u;
        else if (IsEitherKey(wParam,g_inputKeyDownPrimary,g_inputKeyDownSecondary))
            stateBits &= ~0x0200u;
        else if (IsEitherKey(wParam,g_inputFirstPrimary,g_inputFirstSecondary))
            stateBits &= ~0x4000u;
        else if (IsEitherKey(wParam,g_inputSecondPrimary,g_inputSecondSecondary))
            stateBits &= ~0x8000u;
        else if (wParam==0x10u)
            stateBits &= ~0x0800u;
        else if (wParam==0x11u)
            stateBits &= ~0x1000u;
        break;
    case WM_ACTIVATEAPP_OLD:
        if (wParam==0) {
            stateBits &= ~0x0800u; // shift
            stateBits &= ~0x2000u; // alt
            stateBits &= ~0x1000u; // ctrl
        }
        break;
    default:
        break;
    }
    return 0;
}

void INPUT::Tact()
{
    stateBits &= ~0x10u; // rUp
    stateBits &= ~0x08u; // lUp
    stateBits &= ~0x02u; // mClick
    stateBits &= ~0x04u; // rClick
    stateBits &= ~0x01u; // lClick
    key = 0;
    vkKey = 0;
    mouseWheel = 0;
    if (!g_inputAllowFirst)
        stateBits &= ~0x4000u;
    if (!g_inputAllowSecond)
        stateBits &= ~0x8000u;
}

void INPUT::ChangeCoor(float screen_x,float screen_y)
{
    SetCursorPos(static_cast<int>(screen_x)+g_windowScreenX,
                 static_cast<int>(screen_y)+g_windowScreenY);
}

// VC6 initializes the declared bitfields one by one.  In particular, retail
// leaves alt (bit 13) and stateUnused (bits 16..31) untouched.
INPUT::INPUT()
{
    lClick=0;
    lDown=0;
    lUp=0;
    mClick=0;
    rClick=0;
    rDown=0;
    rUp=0;
    left=0;
    right=0;
    down=0;
    up=0;
    shift=0;
    ctrl=0;
    first=0;
    second=0;

    // Retail member-store order after the bitfield clears.
    mouseWheel=0;
    screenMouseX=0.0f;
    mouseX=0.0f;
    screenMouseY=0.0f;
    mouseY=0.0f;
    key=0;
    vkKey=0;
}

// Packs the retail input bitfield in the script-visible historical order.
unsigned int INPUT::GetState()
{
    const unsigned int b=stateBits;
    unsigned int result=(b>>0)&1u;
    result|=((b>>2)&1u)<<1;
    result|=((b>>5)&1u)<<2;
    result|=((b>>6)&1u)<<3;
    result|=((b>>11)&1u)<<4;
    result|=((b>>12)&1u)<<5;
    result|=((b>>7)&1u)<<6;
    result|=((b>>8)&1u)<<7;
    result|=((b>>10)&1u)<<8;
    result|=((b>>9)&1u)<<9;
    result|=((b>>14)&1u)<<10;
    result|=((b>>15)&1u)<<11;
    return result;
}

void INPUT::Save(STREAM* res)
{
    res->Write(this,0x20);
}

void INPUT::Load(STREAM* res)
{
    res->Read(this,0x20);
}

void INPUT::ClearLClick()
{
    stateBits&=~0x0001u;
    if (g_inputFirstPrimary==1)
        stateBits&=~0x4000u;
    if (g_inputSecondPrimary==1)
        stateBits&=~0x8000u;
}

void INPUT::ClearRClick()
{
    stateBits&=~0x0004u;
    if (g_inputFirstPrimary==2)
        stateBits&=~0x4000u;
    if (g_inputSecondPrimary==2)
        stateBits&=~0x8000u;
}

// Converts the textual control names accepted by the retail profile into the
// Win32 virtual-key/mouse codes stored in the global bindings.
unsigned int INPUT::StringToKey(STRING key)
{
    key = key.ToUpper();
    const char* s=key.CharPtr();
    if (!strcmp(s,"LBUTTON")) return 1;
    if (!strcmp(s,"RBUTTON")) return 2;
    if (!strcmp(s,"[")) return 0xDB;
    if (!strcmp(s,"]")) return 0xDD;
    if (!strcmp(s,"LEFT")) return 0x25;
    if (!strcmp(s,"RIGHT")) return 0x27;
    if (!strcmp(s,"UP")) return 0x26;
    if (!strcmp(s,"DOWN")) return 0x28;
    if (!strcmp(s,"INSERT")) return 0x2D;
    if (!strcmp(s,"DELETE")) return 0x2E;
    if (!strcmp(s,"HOME")) return 0x24;
    if (!strcmp(s,"END")) return 0x23;
    if (!strcmp(s,"PGUP")) return 0x21;
    if (!strcmp(s,"PGDN")) return 0x22;
    if (!strcmp(s,"SHIFT")) return 0x10;
    if (!strcmp(s,"CTRL")) return 0x11;
    if (s[0]=='F' && s[1]>='1' && s[1]<='9' && !s[2]) return 0x70u+(unsigned int)(s[1]-'1');
    if (!strcmp(s,"F10")) return 0x79;
    if (!strcmp(s,"F11")) return 0x7A;
    if (!strcmp(s,"F12")) return 0x7B;
    return static_cast<unsigned int>(static_cast<signed char>(key.FirstChar()));
}
