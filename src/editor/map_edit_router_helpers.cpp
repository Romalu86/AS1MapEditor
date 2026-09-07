#include "mapedit/runtime.hpp"

int MAP_EDIT::IsLeftPossible()
{
    if (optTacticMode)
        return m_groups.First()!=0;
    if (spriteType==0x40)
        return 0;
    return 1;
}

// The non-tactic branch really returns 1 for both spriteType==0x40 and !=0x40 in
// the original machine code; do not "simplify" this into invented editor policy.
int MAP_EDIT::IsRightPossible()
{
    if (optTacticMode)
        return m_groups.First()!=0;
    if (spriteType==0x40)
        return 1;
    return 1;
}

int MAP_EDIT::CallDialogBox(const STRING* name,DLGPROC_OLD f)
{
    Graph->BeginPause();
    const int result=DialogBoxParamA(m_instance,const_cast<STRING*>(name)->CharPtr(),m_hWnd,f,0);
    Graph->EndPause();
    return result;
}
