#include "mapedit/runtime.hpp"

// Original MapEdit.exe 0x0040EE6D..0x0040F20E, map_edit.cpp:1560.
//
// This restores the retail MAP_EDIT vtable owner and its exact editor-class
// dispatch.  The derived constructors are deliberately real dependencies:
// hiding them behind MAP::CreateSprite is what allowed the earlier false
// zero-linker-frontier state.
SPRITE* MAP_EDIT::CreateSprite(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
{
    if (!vid)
        return 0;

    if ((vid->m_propertyBits>>4)&1u)
        vid=REGION::ConvertVid(vid,x,y,z);

    if (vid->m_limit394>=0 && vid->NoSprites()>=vid->m_limit394)
        return 0;

    switch (vid->m_spriteClass) {
    case 0:
    case 1:
        return new TERRAIN(vid,x,y,z,direction,parent);
    case 3:
        return new BUILDING(vid,x,y,z,direction,parent);
    case 20:
        return new CIV_ROBOT(vid,x,y,z,direction,parent);
    case 21:
        return new ENGINE(vid,x,y,z,direction,parent);
    case 22:
        return new RAIL(vid,x,y,z,direction,parent);
    case 24:
        return new DEPO(vid,x,y,z,direction,parent);
    case 25:
        return new CREATURE(vid,x,y,z,direction,parent);
    case 26:
        return new BALLOON(vid,x,y,z,direction,parent);
    default:
        return MAP::CreateSprite(vid,x,y,z,direction,parent);
    }
}
