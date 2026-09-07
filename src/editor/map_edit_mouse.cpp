#include "mapedit/runtime.hpp"

// This intentionally preserves several odd original branches/thresholds instead of
// normalising them into a redesigned editor mode system.
void MAP_EDIT::ChangeMouseVid(VID* nvid,unsigned int new_type)
{
    optShiftSnapX=0;
    optShiftSnapY=0;
    if (!nvid)
        return;

    if (!nvid->IsSpriteType(new_type) && new_type<=0x40) {
        nvid=Vid(NextVid(nvid->m_idx,new_type));
        if (nvid==EmptyVid)
            return;
    }

    g_previousSpriteType=spriteType;
    spriteType=(int)new_type;

    if (!optTacticMode) {
        if (g_previousSpriteType!=spriteType)
            selectedSprites.Release();

        if (spriteType<0x40)
            Mouse->HardwareOff();
        else
            Mouse->HardwareOn();

        const int crossed20=((g_previousSpriteType>=0x20 && spriteType<0x20) ||
                             (g_previousSpriteType<0x20 && spriteType>=0x20));
        if (crossed20) {
            SendMessageA(hToolBar,0x401,0x9C59,spriteType<0x20);
            SendMessageA(hToolBar,0x401,0x9C62,spriteType<0x20);
            SendMessageA(hToolBar,0x401,0x9C75,spriteType<=0x40);
            SendMessageA(hToolBar,0x401,0xB01D,spriteType<0x40);
            SendMessageA(hToolBar,0x401,0xB029,spriteType<0x20);
            SendMessageA(hToolBar,0x401,0xB02B,spriteType<0x20);
        }
    }

    if (Mouse->Vid()!=nvid || spriteType!=g_previousSpriteType) {
        MENUITEMINFOA_OLD info;
        memset(&info,0,sizeof(info));
        STRING numberName;
        info.cbSize=sizeof(info);
        info.fMask=0x10;
        info.fType=0;

        const char* modeName;
        if (optTacticMode) {
            modeName="Tactic mode";
        } else if (spriteType<=0x40) {
            numberName=nvid->GetNumberName();
            modeName=numberName.CharPtr();
        } else if (spriteType==0x40) {
            // This branch is unreachable after the signed <=0x40 test in the original
            // machine code. It is retained because the original source/codegen contains it.
            modeName="Region edit mode";
        } else {
            modeName="Unknown mode";
        }
        info.dwTypeData=(char*)modeName;
        info.cch=(uint32_t)strlen(modeName);
        SetMenuItemInfoA(GetMenu(m_hWnd),6,1,&info);
        DrawMenuBar(m_hWnd);

        Mouse->Action(0x3E,nvid->m_idx,0,0);

        if (optControlPanel)
            DialogControlPanel(hControlPanel,(unsigned int)(spriteType==g_previousSpriteType),0,0);

        if (spriteType!=g_previousSpriteType) {
            unsigned int command=0;
            switch (spriteType) {
            case 1:    command=0x9C6F; break;
            case 2:    command=0x9C70; break;
            case 4:    command=0x9C71; break;
            case 8:    command=0x9C72; break;
            case 16:   command=0x9C73; break;
            case 32:   command=0xB023; break;
            case 64:   command=0xB022; break;
            default: break;
            }
            if (command)
                SendMessageA(hToolBar,0x402,command,1);
        }
    }

    if (new_type==0x40) {
        if (!curRegion)
            Right();
        if (curRegion)
            Mouse->Action(0x3E,curRegion->Vid()->m_idx,0,0);
    }
}
