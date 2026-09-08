#include "mapedit/runtime.hpp"

DIALOG_ITEM::DIALOG_ITEM(HWND__* window, int resourceId)
    : hWnd(window), id(resourceId)
{
}

DIALOG_COMBO_BOX::DIALOG_COMBO_BOX(HWND__* window, int resourceId)
    : DIALOG_ITEM(window, resourceId)
{
}

DIALOG_BUTTON::DIALOG_BUTTON(HWND__* window, int resourceId)
    : DIALOG_ITEM(window, resourceId)
{
}

int DIALOG_ITEM::Message(unsigned int message, unsigned int wParam, long lParam)
{
    return static_cast<int>(SendDlgItemMessageA(hWnd, id, message, wParam, lParam));
}

unsigned int DIALOG_BUTTON::GetCheck()
{
    return Message(0xF0, 0, 0) == 1; // BM_GETCHECK
}

void DIALOG_BUTTON::SetCheck(int check)
{
    Message(0xF1, check != 0, 0); // BM_SETCHECK
}

int __stdcall AppControlPanel(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogControlPanel(hwnd,msg,wParam,lParam);
}


void DIALOG_ITEM::Enable(int flag)
{
    EnableWindow(GetDlgItem(hWnd,id),flag);
}

void DIALOG_ITEM::Disable()
{
    EnableWindow(GetDlgItem(hWnd,id),0);
}

void DIALOG_COMBO_BOX::Reset() { Message(0x14B,0,0); }
int DIALOG_COMBO_BOX::NoString() { return Message(0x146,0,0); }
int DIALOG_COMBO_BOX::AddString(const STRING* str)
{
    return Message(0x143,0,reinterpret_cast<long>(const_cast<STRING*>(str)->CharPtr()));
}
int DIALOG_COMBO_BOX::AddStringWithData(const STRING* str,int data)
{
    const int index=AddString(str);
    if (index != -1)
        Message(0x151,static_cast<unsigned int>(index),data);
    return index;
}
void DIALOG_COMBO_BOX::SetCurrent(int index)
{
    Message(0x14E,static_cast<unsigned int>(index),0);
}
void DIALOG_COMBO_BOX::SetCurrent(const STRING* str)
{
    Message(0x14D,0,reinterpret_cast<long>(const_cast<STRING*>(str)->CharPtr()));
}
int DIALOG_COMBO_BOX::GetCurrentStringIndex() { return Message(0x147,0,0); }
int DIALOG_COMBO_BOX::GetData(int index)
{
    return Message(0x150,static_cast<unsigned int>(index),0);
}
int DIALOG_COMBO_BOX::GetCurrentStringData()
{
    return GetData(GetCurrentStringIndex());
}

STRING ActionToText(int action)
{
    switch (action) {
    case 0x21: return STRING("ACT_MOVE");
    case 0x20: return STRING("ACT_ATTACK");
    case 0x25: return STRING("ACT_COOR_ATTACK");
    case 0x23: return STRING("ACT_BUILD_UNIT");
    case 0x86: return STRING("ACT_DESTROY_UNIT");
    case 0x27: return STRING("ACT_STOP");
    case 0x26: return STRING("ACT_RANDOM");
    case 0x24: return STRING("ACT_PATROL");
    case 0x22: return STRING("ACT_MOVE_TO");
    case 0x29: return STRING("ACT_ROTATE");
    case 0x28: return STRING("ACT_PAUSE");
    case 0x55: return STRING("ACT_DAMAGE");
    case 0x58: return STRING("ACT_SET_HP");
    case 0x62: return STRING("ACT_SET_INVISIBLE");
    case 0x5B: return STRING("ACT_SET_GOAL_COOR");
    case 0x49: return STRING("ACT_STOP_STACK");
    case 0x47: return STRING("ACT_GOTO_STACK");
    case 0x48: return STRING("ACT_CLEAR_STACK");
    case 0x2B: return STRING("ACT_FLAGMAN_TRIGGER");
    case 0x3F: return STRING("ACT_CHANGE_COOR");
    case 0x3E: return STRING("ACT_CHANGE_VID");
    case 0x87: return STRING("ACT_PLAY_SFX");
    case 0x00: return STRING("ANI_STAND");
    case 0x01: return STRING("ANI_STOP_MOVE/ANI_BUILD");
    case 0x02: return STRING("ANI_GO");
    case 0x03: return STRING("ANI_START_MOVE");
    case 0x04: return STRING("ANI_L_ROTATE");
    case 0x05: return STRING("ANI_R_ROTATE");
    case 0x06: return STRING("ANI_OPEN");
    case 0x07: return STRING("ANI_HIT");
    case 0x08: return STRING("ANI_FIGHT");
    case 0x09: return STRING("ANI_SALUT");
    case 0x0A: return STRING("ANI_STAND_OPEN");
    case 0x0B: return STRING("ANI_CLASH_VERT");
    case 0x0C: return STRING("ANI_LAZY/ANI_CLASH");
    case 0x0D: return STRING("ANI_WOUND");
    case 0x0E: return STRING("ANI_BIRTH");
    case 0x0F: return STRING("ANI_DEATH");
    case 0x10: return STRING("ANI_DEATH2");
    default: return Printf("UNKNOWN%i",action);
    }
}


STRING ActionComment(int action)
{
    switch (action) {
    case 0x21: return STRING("var1-x,var2-y,var3-z");
    case 0x20: return STRING("\xE0\xF2\xE0\xEA\xE0 (SPRITE*)var1");
    case 0x25: return STRING("\xEE\xE4\xE8\xED\xEE\xF7\xED\xFB\xE9 \xE2\xFB\xF1\xF2\xF0\xE5\xEB \xEF\xEE \xEA\xEE\xEE\xF0\xE4\xE8\xED\xE0\xF2\xE0\xEC var1,var2");
    case 0x23: return STRING("var1-nvid for building unit (\xE5\xF1\xEB\xE8 0 \xF2\xEE \xEE\xE4\xE8\xED \xE8\xE7 items), var2,var3 -\xEA\xEE\xEE\xF0\xE4\xE8\xED\xE0\xF2\xFB \xE3\xE4\xE5 \xF0\xEE\xE4\xE8\xF2\xF1\xFF \xFE\xED\xE8\xF2, \xE5\xF1\xEB\xE8 0 \xF2\xEE \xF0\xEE\xE4\xE8\xF2\xF1\xFF \xE2 \xEA\xEE\xEE\xF0\xE4\xE8\xED\xE0\xF2\xE0\xF5 \xF0\xEE\xE4\xE8\xF2\xE5\xEB\xFF, \xE5\xF1\xEB\xE8 <0 \xF2\xEE \xF0\xEE\xE4\xE8\xF2\xF1\xFF \xF1 \xF1\xEB\xF3\xF7\xE0\xE9\xED\xFB\xEC \xF1\xEC\xE5\xF9\xE5\xED\xE8\xE5\xEC \xEE\xF2 \xF0\xEE\xE4\xE8\xF2\xE5\xEB\xFF");
    case 0x86: return STRING("var1-nvid \xF3\xED\xE8\xF7\xF2\xEE\xE6\xE0\xE5\xEC\xEE\xE3\xEE \xEE\xE1\xFA\xE5\xEA\xF2\xE0, var2,var3 -\xEA\xEE\xEE\xF0\xE4\xE8\xED\xE0\xF2\xFB \xE3\xE4\xE5 \xF3\xED\xE8\xF7\xF2\xEE\xE6\xE8\xF2\xFC");
    case 0x27: return STRING("\xEF\xF0\xE5\xEA\xF0\xE0\xF2\xE8\xF2\xFC \xE4\xE2\xE8\xE6\xE5\xED\xE8\xE5 \xE8 \xE2\xFB\xEF\xEE\xEB\xED\xE5\xED\xE8\xE5 \xEB\xFE\xE1\xEE\xE9 \xEA\xEE\xEC\xE0\xED\xE4\xFB, \xE5\xF1\xEB\xE8 var1 \xED\xE5 \xF0\xE0\xE2\xED\xEE 0, \xF2\xEE \xED\xE5\xEC\xE5\xE4\xEB\xE5\xED\xED\xEE \xE7\xE0\xF2\xEE\xF0\xEC\xEE\xE7\xE8\xF2\xFC");
    case 0x26: return STRING("\xE2\xFB\xEF\xEE\xEB\xED\xE8\xF2\xFC \xEB\xE8\xE1\xEE \xEB\xFD\xE7\xE8, \xEB\xE8\xE1\xEE \xEF\xF0\xEE\xF1\xF2\xEE \xEF\xEE\xE2\xE5\xF0\xED\xF3\xF2\xFC \xE3\xEE\xEB\xEE\xE2\xF3, \xEB\xE8\xE1\xEE \xED\xE8\xF7\xE5\xE3\xEE \xED\xE5 \xF1\xE4\xE5\xEB\xE0\xF2\xFC");
    case 0x24: return STRING("?\xEF\xEE\xEA\xE0 \xED\xE5 \xEF\xEE\xE4\xE4\xE5\xF0\xE6\xE8\xE2\xE0\xE5\xF2\xF1\xFF?");
    case 0x22: return STRING("var1-(SPRITE*)goal");
    case 0x29: return STRING("\xEF\xEE\xE2\xE5\xF0\xED\xF3\xF2\xFC\xF1\xFF \xE2 \xED\xE0\xEF\xF0\xE0\xE2\xEB\xE5\xED\xE8\xE8 var1");
    case 0x28: return STRING("\xED\xE8\xF7\xE5\xE3\xEE \xED\xE5 \xE4\xE5\xEB\xE0\xF2\xFC var1 + Random(var2) ms");
    case 0x55: return STRING("var1-damage,var2-unit \xE2\xFB\xE7\xE2\xE0\xE2\xF8\xE8\xE9 damage");
    case 0x58: return STRING("var1 - \xED\xEE\xE2\xEE\xE5 \xEA\xEE\xEB\xE8\xF7\xE5\xF1\xF2\xE2\xEE hp, var2 - \xEA\xEE\xEB\xE8\xF7\xE5\xF1\xF2\xE2\xEE hp \xE2 \xEF\xF0\xEE\xF6\xE5\xED\xF2\xE0\xF5 \xEE\xF2 \xEC\xE0\xEA\xF1\xE8\xEC\xF3\xEC\xE0(\xE5\xF1\xEB\xE8 var1==0)");
    case 0x62: return STRING("var1 -  1-invisible, 0-visible");
    case 0x5B: return STRING("\xF1\xEE\xE7\xE4\xE0\xE5\xF2\xF1\xFF \xED\xEE\xE2\xE0\xFF goal \xF1 x=var1, y=var2, z=var3");
    case 0x49: return STRING("\xE4\xE0\xED\xED\xE0\xFF \xEA\xEE\xEC\xE0\xED\xE4\xE0 \xEE\xF1\xF2\xE0\xED\xE0\xE2\xEB\xE8\xE2\xE0\xE5\xF2 \xE2\xFB\xEF\xEE\xEB\xED\xE5\xED\xE8\xE5 \xE4\xE0\xEB\xFC\xED\xE5\xE9\xF8\xE8\xF5 \xEA\xEE\xEC\xE0\xED\xE4 \xE2 \xF1\xF2\xFD\xEA\xE5 (\xED\xF3\xE6\xED\xEE \xE4\xEB\xFF building)");
    case 0x47: return STRING("\xEF\xE5\xF0\xE5\xF5\xEE\xE4 \xE2 \xF1\xF2\xFD\xEA\xE5 \xEA\xEE\xEC\xEC\xE0\xED\xE4, var1-\xED\xEE\xEC\xE5\xF0 \xEA\xEE\xEC\xE0\xED\xE4\xFB \xEA \xEA\xEE\xF2\xEE\xF0\xEE\xE9 \xED\xF3\xE6\xED\xEE \xEF\xE5\xF0\xE5\xE9\xF2\xE8");
    case 0x48: return STRING("\xEE\xF7\xE8\xF1\xF2\xE8\xF2\xFC actionStack");
    case 0x2B: return STRING("\xED\xE5 \xEF\xF0\xEE\xE4\xEE\xEB\xE6\xE0\xF2\xFC \xE2\xFB\xEF\xEE\xEB\xED\xFF\xF2\xFC \xEA\xEE\xEC\xE0\xED\xE4\xFB \xF1\xF2\xE5\xEA\xE0 \xEF\xEE\xEA\xE0 Flagman(0)->NearestDistance(var1,var2) > var3");
    case 0x3F: return STRING("var1-x,var2-y,var3-z");
    case 0x3E: return STRING("\xE8\xE7\xEC\xE5\xED\xE8\xF2\xFC \xF2\xE5\xEA\xF3\xF9\xE8\xE9 vid \xE4\xEB\xFF \xF1\xEF\xF0\xE0\xE9\xF2\xE0, var1- new vid, var2 - new animation, if var2==-1 animation not changed");
    case 0x87: return STRING("var1 - \xED\xEE\xEC\xE5\xF0 \xE8\xE3\xF0\xE0\xE5\xEC\xEE\xE3\xEE sfx");
    case 0x00: return STRING("\xED\xEE\xF0\xEC\xE0\xEB\xFC\xED\xEE\xE5 \xF1\xEE\xF1\xF2\xEE\xFF\xED\xE8\xE5 \xEE\xE1\xFA\xE5\xEA\xF2\xE0");
    case 0x01: return STRING("\xEF\xF0\xEE\xF6\xE5\xF1\xF1 \xEE\xF1\xF2\xE0\xED\xEE\xE2\xEA\xE8 \xE4\xE2\xE8\xE6\xE5\xED\xE8\xFF/\xF1\xF2\xF0\xEE\xE8\xF2\xE5\xEB\xFC\xF1\xF2\xE2\xE0 \xE4\xEE\xF7\xE5\xF0\xED\xE5\xE3\xEE \xFE\xED\xE8\xF2\xE0");
    case 0x02: return STRING("\xE0\xED\xE8\xEC\xE0\xF6\xE8\xFF \xE4\xE2\xE8\xE6\xE5\xED\xE8\xFF \xFE\xED\xE8\xF2\xE0");
    case 0x03: return STRING("\xEF\xF0\xEE\xF6\xE5\xF1\xF1 \xED\xE0\xF7\xE0\xEB\xE0 \xE4\xE2\xE8\xE6\xE5\xED\xE8\xFF \xFE\xED\xE8\xF2\xE0");
    case 0x04: return STRING("ANI_L_ROTATE");
    case 0x05: return STRING("ANI_R_ROTATE");
    case 0x06: return STRING("ANI_OPEN");
    case 0x07: return STRING("\xEE\xE1\xFA\xE5\xEA\xF2 \xEF\xEE\xE2\xF0\xE5\xE6\xE4\xE5\xED(\xF3\xE4\xE0\xF0\xE5\xED)");
    case 0x08: return STRING("\xFE\xED\xE8\xF2 \xE0\xF2\xE0\xEA\xF3\xE5\xF2 \xEA\xEE\xE3\xEE \xEB\xE8\xE1\xEE");
    case 0x09: return STRING("\xE8\xE3\xF0\xEE\xEA \xEA\xEB\xE8\xEA\xED\xF3\xEB \xED\xE0 \xFE\xED\xE8\xF2\xE0");
    case 0x0A: return STRING("\xE8\xE7 \xFD\xF2\xEE\xE3\xEE \xF1\xEE\xF1\xF2\xEE\xFF\xED\xE8\xFF \xEE\xE1\xFA\xE5\xEA\xF2 \xED\xE5 \xE1\xF3\xE4\xE5\xF2 \xEF\xE5\xF0\xE5\xF5\xEE\xE4\xE8\xF2\xFC \xE0\xE2\xF2\xEE\xEC\xE0\xF2\xE8\xF7\xE5\xF1\xEA\xE8 \xE2 ANI_STAND \xEA\xE0\xEA \xE8\xE7 \xE4\xF0\xF3\xE3\xE8\xF5");
    case 0x0B: return STRING("\xF1\xF2\xEE\xEB\xEA\xED\xEE\xE2\xE5\xED\xE8\xE5 \xF1 \xE2\xE5\xF0\xF2\xE8\xEA\xE0\xEB\xFC\xED\xEE\xE9 \xF1\xF2\xE5\xED\xEA\xEE\xE9");
    case 0x0C: return STRING("\xEF\xE5\xF0\xE8\xEE\xE4\xE8\xF7\xE5\xF1\xEA\xE8 \xE2\xFB\xE7\xFB\xE2\xE0\xE5\xEC\xE0\xFF \xE0\xED\xE8\xEC\xE0\xF6\xE8\xFF \xED\xE0 \xED\xE8\xF7\xE5\xE3\xEE\xED\xE8\xE4\xE5\xEB\xE0\xED\xE8\xE5 \xE4\xEB\xFF B_CREATURE/\xF1\xF2\xEE\xEB\xEA\xED\xEE\xE2\xE5\xED\xE8\xE5 \xF1 \xE7\xE5\xEC\xEB\xE5\xE9");
    case 0x0D: return STRING("\xF1 \xFE\xED\xE8\xF2\xE0 \xF1\xED\xFF\xF2\xE0 \xEF\xEE\xEB\xEE\xE2\xE8\xED\xE0 Hp");
    case 0x0E: return STRING("\xE2\xFB\xE7\xFB\xE2\xE0\xE5\xF2\xF1\xFF \xEF\xF0\xE8 \xF0\xEE\xE6\xE4\xE5\xED\xE8\xE8 \xEE\xE1\xFA\xE5\xEA\xF2\xE0");
    case 0x0F: return STRING("\xF1\xEC\xE5\xF0\xF2\xFC \xFE\xED\xE8\xF2\xE0");
    case 0x10: return STRING("\xF1\xEC\xE5\xF0\xF2\xFC \xEE\xF2 \xEF\xEE\xE2\xF0\xE5\xE6\xE4\xE5\xED\xE8\xE9 \xE1\xEE\xEB\xFC\xF8\xE8\xF5 \xEF\xEE\xEB\xEE\xE2\xE8\xED\xFB maxHp \xE7\xE0 \xF0\xE0\xE7");
    default: return STRING("");
    }
}

namespace {
void StackToListBox(DIALOG_LIST_BOX* list,LIST<ACT>* stack)
{
    list->Reset();
    for (int i=stack->No()-1;i>=0;--i) {
        ACT* a=(*stack)[i];
        STRING action=ActionToText(a->act);
        STRING row=Printf("%3i: %-18s %5i %5i %5i",i,action.CharPtr(),a->var1,a->var2,a->var3);
        list->AddStringWithData(&row,i);
    }
    STRING empty;
    list->AddStringWithData(&empty,-1);
}

LIST<ACT> g_unitStack;
LIST<ACT> g_unitClipboard;
ACT g_unitEditAction;

}

namespace {
struct TAB_ITEM_OLD {
    uint32_t mask;
    uint32_t state;
    uint32_t stateMask;
    char* text;
    int textMax;
    int image;
    long data;
};
struct NMHDR_OLD {
    HWND__* hwndFrom;
    uint32_t idFrom;
    int code;
};
static_assert(sizeof(TAB_ITEM_OLD)==28, "retail x86 tab item ABI");
static_assert(sizeof(NMHDR_OLD)==12, "retail x86 NMHDR ABI");

int g_controlPanelCustomVid[10];
VID* g_controlPanelSavedVid;
}

int DIALOG_ITEM::IsNotify(unsigned int msg,unsigned int wParam,unsigned short notifyMessage)
{
    if (msg != 0x111u)
        return 0;
    if ((wParam & 0xFFFFu) != static_cast<unsigned int>(id))
        return 0;
    return ((wParam >> 16) & 0xFFFFu) == static_cast<unsigned int>(notifyMessage);
}

void DIALOG_ITEM::SetText(const STRING* str)
{
    SetDlgItemTextA(hWnd,id,const_cast<STRING*>(str)->CharPtr());
}

STRING DIALOG_ITEM::GetText()
{
    char buffer[512];
    GetDlgItemTextA(hWnd,id,buffer,512);
    return STRING(buffer);
}

int DIALOG_BUTTON::IsClicked(unsigned int msg,unsigned int wParam)
{
    return IsNotify(msg,wParam,0);
}

DIALOG_LIST_BOX::DIALOG_LIST_BOX(HWND__* window,int resourceId)
    : DIALOG_ITEM(window,resourceId)
{
}

int DIALOG_LIST_BOX::NoString() { return Message(0x18B,0,0); }
void DIALOG_LIST_BOX::Reset() { Message(0x184,0,0); }
void DIALOG_LIST_BOX::SetColumnWidth(int width) { Message(0x195,static_cast<unsigned int>(width),0); }
int DIALOG_LIST_BOX::AddString(const STRING* str)
{
    return Message(0x180,0,reinterpret_cast<long>(const_cast<STRING*>(str)->CharPtr()));
}
int DIALOG_LIST_BOX::AddStringWithData(const STRING* str,int data)
{
    const int index=AddString(str);
    if (index != -1)
        Message(0x19A,static_cast<unsigned int>(index),data);
    return index;
}
void DIALOG_LIST_BOX::SetCurrent(int index)
{
    Message(0x186,static_cast<unsigned int>(index),0);
}
int DIALOG_LIST_BOX::GetCurrentStringIndex() { return Message(0x188,0,0); }
int DIALOG_LIST_BOX::GetData(int index)
{
    return Message(0x199,static_cast<unsigned int>(index),0);
}
int DIALOG_LIST_BOX::GetCurrentStringData()
{
    return GetData(GetCurrentStringIndex());
}
int DIALOG_LIST_BOX::GetStringDataIndex(int data)
{
    for (int i=NoString()-1;i>=0;--i) {
        if (GetData(i)==data)
            return i;
    }
    return -1;
}
int DIALOG_LIST_BOX::IsSelChange(unsigned int msg,unsigned int wParam)
{
    return IsNotify(msg,wParam,1);
}
int DIALOG_LIST_BOX::IsDoubleClicked(unsigned int msg,unsigned int wParam)
{
    return IsNotify(msg,wParam,2);
}

void DIALOG_LIST_BOX::SetSelected(int index)
{
    Message(0x185u,1u,index);
}

int DIALOG_LIST_BOX::IsSelected(int index)
{
    return Message(0x187u,static_cast<unsigned int>(index),0) > 0;
}

void DIALOG_LIST_BOX::SetUnSelected(int index)
{
    Message(0x185u,0u,index);
}

DIALOG_TABS::DIALOG_TABS(HWND__* window,int resourceId)
    : DIALOG_ITEM(window,resourceId)
{
}
int DIALOG_TABS::NoItem() { return Message(0x1304,0,0); }
int DIALOG_TABS::SetMinWidth(int width)
{
    return Message(0x1331,0,width);
}
int DIALOG_TABS::AddItemWithData(const STRING* str,int data)
{
    TAB_ITEM_OLD item;
    item.mask=9;
    item.state=0;
    item.stateMask=0;
    item.text=const_cast<STRING*>(str)->CharPtr();
    item.textMax=const_cast<STRING*>(str)->Length();
    item.image=-1;
    item.data=data;
    return Message(0x1307,static_cast<unsigned int>(NoItem()),reinterpret_cast<long>(&item));
}
void DIALOG_TABS::SetCurrentWithData(int data)
{
    for (int i=NoItem()-1;i>=0;--i) {
        if (GetData(i)==data) {
            SetCurrent(i);
            break;
        }
    }
}
void DIALOG_TABS::SetCurrent(int index)
{
    Message(0x130C,static_cast<unsigned int>(index),0);
}
int DIALOG_TABS::GetData(int index)
{
    TAB_ITEM_OLD item;
    item.mask=8;
    item.state=0;
    item.stateMask=0;
    item.text=0;
    item.textMax=0;
    item.image=-1;
    item.data=0;
    Message(0x1305,static_cast<unsigned int>(index),reinterpret_cast<long>(&item));
    return item.data;
}
int DIALOG_TABS::GetCurrentItemData()
{
    return GetData(GetCurrentItemIndex());
}
int DIALOG_TABS::GetCurrentItemIndex() { return Message(0x130B,0,0); }
int DIALOG_TABS::IsSelChange(unsigned int msg,unsigned int,long lParam)
{
    if (msg != 0x4Eu)
        return 0;
    return reinterpret_cast<const NMHDR_OLD*>(lParam)->code == -551;
}

DIALOG_TEXT::DIALOG_TEXT(HWND__* window,int resourceId)
    : DIALOG_ITEM(window,resourceId)
{
}
const STRING* DIALOG_TEXT::operator=(const STRING* str)
{
    SetText(str);
    return str;
}

DIALOG_TEXT::operator STRING()
{
    return GetText();
}

DIALOG_RADIO::DIALOG_RADIO(HWND__* window,int firstId,int lastResourceId)
    : DIALOG_ITEM(window,firstId), lastId(lastResourceId)
{
}
void DIALOG_RADIO::Check(int index)
{
    CheckRadioButton(hWnd,id,lastId,id+index);
}
void DIALOG_RADIO::SetItem(int index,const STRING* str)
{
    SetDlgItemTextA(hWnd,id+index,const_cast<STRING*>(str)->CharPtr());
}
int DIALOG_RADIO::GetIndexClicked(unsigned int msg,unsigned int wParam)
{
    if (msg != 0x111u || ((wParam >> 16) & 0xFFFFu) != 0u)
        return -1;
    const int commandId=static_cast<int>(wParam & 0xFFFFu);
    if (commandId < id || commandId > lastId)
        return -1;
    return commandId-id;
}
int DIALOG_RADIO::GetIndexDblClicked(unsigned int msg,unsigned int wParam)
{
    if (msg != 0x111u || ((wParam >> 16) & 0xFFFFu) != 5u)
        return -1;
    const int commandId=static_cast<int>(wParam & 0xFFFFu);
    if (commandId < id || commandId > lastId)
        return -1;
    return commandId-id;
}

int MAP_EDIT::DialogControlPanel(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    DIALOG_TEXT coor(hwnd,0x3EF);
    DIALOG_LIST_BOX vids(hwnd,0x3F5);
    DIALOG_TABS tabs(hwnd,0x44E);
    DIALOG_BUTTON sort(hwnd,0x7D3);
    DIALOG_RADIO customVid(hwnd,0x444,0x44D);

    int index=customVid.GetIndexClicked(msg,wParam);
    if (index >= 0) {
        g_controlPanelSavedVid=Mouse->Vid();
        if (g_controlPanelCustomVid[index] >= 0) {
            VID* selected=Vid(g_controlPanelCustomVid[index]);
            ChangeMouseVid(selected,selected->m_unknown0C);
        }
    } else {
        index=customVid.GetIndexDblClicked(msg,wParam);
        if (index >= 0) {
            if (!g_controlPanelSavedVid)
                g_controlPanelSavedVid=Mouse->Vid();
            g_controlPanelCustomVid[index]=g_controlPanelSavedVid->m_idx;
            STRING label=g_controlPanelSavedVid->GetNumberName();
            customVid.SetItem(index,&label);
            STRING regName="CustomVid" + Int2Str(index);
            Registry->SetInt(regName,g_controlPanelCustomVid[index]);
            g_controlPanelSavedVid=0;
        } else if (vids.IsSelChange(msg,wParam)) {
            ChangeMouseVid(Vid(vids.GetCurrentStringData()),spriteType);
        } else if (sort.IsClicked(msg,wParam)) {
            optSortVid=!optSortVid;
            VidToListBox(&vids,static_cast<unsigned long>(spriteType),Mouse->Vid()->m_idx,optSortVid);
        } else if (tabs.IsSelChange(msg,wParam,lParam)) {
            ChangeMouseVid(Mouse->Vid(),static_cast<unsigned int>(tabs.GetCurrentItemData()));
            VidToListBox(&vids,static_cast<unsigned long>(spriteType),Mouse->Vid()->m_idx,optSortVid);
        }
    }

    switch (msg) {
    case 0:
        tabs.SetCurrentWithData(spriteType);
        VidToListBox(&vids,static_cast<unsigned long>(spriteType),Mouse->Vid()->m_idx,optSortVid);
        index=-1;
        for (int i=0;i<10;++i) {
            if (g_controlPanelCustomVid[i] == Mouse->Vid()->m_idx) {
                index=i;
                break;
            }
        }
        customVid.Check(index);
        return 0;

    case 1:
        vids.SetCurrent(vids.GetStringDataIndex(Mouse->Vid()->m_idx));
        index=-1;
        for (int i=0;i<10;++i) {
            if (g_controlPanelCustomVid[i] == Mouse->Vid()->m_idx) {
                index=i;
                break;
            }
        }
        customVid.Check(index);
        return 0;

    case 0x110:
        for (int i=0;i<10;++i) {
            STRING regName="CustomVid" + Int2Str(i);
            g_controlPanelCustomVid[i]=Registry->GetInt(regName,-1);
            STRING label;
            if (ValidateVid(g_controlPanelCustomVid[i]))
                label=Vid(g_controlPanelCustomVid[i])->GetNumberName();
            else
                label="unsetted";
            customVid.SetItem(i,&label);
        }

        VidToListBox(&vids,static_cast<unsigned long>(spriteType),Mouse->Vid()->m_idx,optSortVid);
        {
            STRING text("Terrain"); tabs.AddItemWithData(&text,1);
            text="Object";  tabs.AddItemWithData(&text,2);
            text="Unit";    tabs.AddItemWithData(&text,4);
            text="Avia";    tabs.AddItemWithData(&text,8);
            text="Menu";    tabs.AddItemWithData(&text,0x10);
            text="Railway"; tabs.AddItemWithData(&text,0x20);
            text="Region";  tabs.AddItemWithData(&text,0x40);
        }
        tabs.SetMinWidth(50);
        tabs.SetCurrentWithData(spriteType);
        sort.SetCheck(optSortVid);
        {
            STRING text=Printf("%-8i %-8i %6i+%-20i %i",
                static_cast<int>(m_input.mouseX),
                static_cast<int>(m_input.mouseY),
                static_cast<int>(insertZ),
                static_cast<int>(GetGroundZ(Mouse->Vid(),m_input.mouseX,m_input.mouseY)),
                Graph->RealZBuffer(m_input.screenMouseX,m_input.screenMouseY));
            coor=&text;
        }
        return 1;

    case 0x84: {
        RECT_OLD rect;
        GetWindowRect(m_hWnd,&rect);
        m_input.screenMouseX=static_cast<float>(static_cast<int>(static_cast<unsigned long>(lParam)&0xFFFFu)-rect.left);
        m_input.screenMouseY=static_cast<float>(static_cast<int>((static_cast<unsigned long>(lParam)>>16)&0xFFFFu)-rect.top);
        return 0;
    }
    default:
        return 0;
    }
}

DIALOG_UNSIGNED::DIALOG_UNSIGNED(HWND__* window,int resourceId)
    : DIALOG_ITEM(window,resourceId) {}
unsigned int DIALOG_UNSIGNED::operator=(unsigned int value) { SetUnsigned(value); return value; }
DIALOG_UNSIGNED::operator unsigned int() { return GetUnsigned(); }
void DIALOG_ITEM::SetUnsigned(unsigned int value) { SetDlgItemInt(hWnd,id,value,0); }
unsigned int DIALOG_ITEM::GetUnsigned() { return GetDlgItemInt(hWnd,id,0,0); }

DIALOG_INT::DIALOG_INT(HWND__* window,int resourceId)
    : DIALOG_ITEM(window,resourceId) {}
int DIALOG_INT::operator=(int value) { SetInt(value); return value; }
DIALOG_INT::operator int() { return GetInt(); }
void DIALOG_ITEM::SetInt(int value) { SetDlgItemInt(hWnd,id,static_cast<unsigned int>(value),1); }
int DIALOG_ITEM::GetInt() { return static_cast<int>(GetDlgItemInt(hWnd,id,0,1)); }

namespace {
unsigned int g_regionSavedProperty;
}

int __stdcall AppRegionProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogRegionProperty(hwnd,msg,wParam,lParam);
}

int MAP_EDIT::DialogRegionProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    if (curRegion)
        return curRegion->DialogRegionProperty(hwnd,msg,wParam,lParam);
    EndDialog(hwnd,0);
    return 0;
}

void UpdateScreenForDlg(HWND__* hwnd)
{
    COLOR black(0,0,0);
    Graph->ClearScreen(black);
    Graph->PreTact();
    Graph->Tact(1);
    Graph->PostTact(1);
    RedrawWindow(hwnd,0,0,0x485u);
}

int REGION::DialogRegionProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long)
{
    DIALOG_COMBO_BOX texture(hwnd,0x421);
    DIALOG_COMBO_BOX environment(hwnd,0x422);
    DIALOG_INT x(hwnd,0x3EF);
    DIALOG_INT y(hwnd,0x3F1);
    DIALOG_INT z(hwnd,0x3F3);
    DIALOG_UNSIGNED size_x(hwnd,0x3F0);
    DIALOG_UNSIGNED size_y(hwnd,0x3F2);
    DIALOG_INT fog_top(hwnd,0x3F4);
    DIALOG_INT fog_bottom(hwnd,0x3F5);
    DIALOG_UNSIGNED fog_red(hwnd,0x3F6);
    DIALOG_UNSIGNED fog_green(hwnd,0x3F7);
    DIALOG_UNSIGNED fog_blue(hwnd,0x3F8);
    DIALOG_BUTTON fog_inverse(hwnd,0x41B);
    DIALOG_BUTTON fog_slow_born(hwnd,0x41F);
    DIALOG_UNSIGNED source_vid[6]={
        DIALOG_UNSIGNED(hwnd,0x434),DIALOG_UNSIGNED(hwnd,0x436),DIALOG_UNSIGNED(hwnd,0x438),
        DIALOG_UNSIGNED(hwnd,0x43A),DIALOG_UNSIGNED(hwnd,0x43C),DIALOG_UNSIGNED(hwnd,0x43E)};
    DIALOG_UNSIGNED conversion_vid[6]={
        DIALOG_UNSIGNED(hwnd,0x435),DIALOG_UNSIGNED(hwnd,0x437),DIALOG_UNSIGNED(hwnd,0x439),
        DIALOG_UNSIGNED(hwnd,0x43B),DIALOG_UNSIGNED(hwnd,0x43D),DIALOG_UNSIGNED(hwnd,0x43F)};

    if (msg==3u) {
        UpdateScreenForDlg(hwnd);
        return 0;
    }
    if (msg==0x110u) {
        g_regionSavedProperty=property;
        Map->VidToControlBox(&texture,0x40u,Vid()->m_idx);
        Map->VidToControlBox(&environment,0x40u,environmentVid->m_idx);
        x=static_cast<int>(X());
        y=static_cast<int>(Y());
        z=static_cast<int>(Z());
        size_x=static_cast<unsigned int>(sizeX);
        size_y=static_cast<unsigned int>(sizeY);
        fog_top=fogTop;
        fog_bottom=fogBottom;
        fog_red=fogColor.Red();
        fog_green=fogColor.Green();
        fog_blue=fogColor.Blue();
        fog_inverse.SetCheck((property&2u)!=0u);
        fog_slow_born.SetCheck((property&1u)!=0u);
        for (int i=0;i<6;++i) {
            source_vid[i]=sourceVid[i] ? static_cast<unsigned int>(sourceVid[i]->m_idx) : 0u;
            conversion_vid[i]=conversionVid[i] ? static_cast<unsigned int>(conversionVid[i]->m_idx) : 0u;
        }
        SendDlgItemMessageA(hwnd,0x7D1,0xF1u,(property&8u)!=0u,0);
        SendDlgItemMessageA(hwnd,0x7D2,0xF1u,(property&4u)!=0u,0);
        SendDlgItemMessageA(hwnd,0x7D3,0xF1u,(property&0x10u)!=0u,0);
        const int enabled=(property&8u) ? 0 : 1;
        x.Enable(enabled); y.Enable(enabled); z.Enable(enabled); size_x.Enable(enabled); size_y.Enable(enabled);
        return 1;
    }
    if (msg!=0x111u)
        return 0;

    switch (wParam&0xFFFFu) {
    case 1: {
        Action(0x3E,texture.GetCurrentStringData(),0,0);
        environmentVid=Map->Vid(environment.GetCurrentStringData());
        COLOR fog(static_cast<int>(static_cast<unsigned int>(fog_red)),
                  static_cast<int>(static_cast<unsigned int>(fog_green)),
                  static_cast<int>(static_cast<unsigned int>(fog_blue)));
        SetFogParameters(static_cast<int>(fog_bottom),static_cast<int>(fog_top),fog);
        ChangeCoor(static_cast<float>(static_cast<int>(x)),
                   static_cast<float>(static_cast<int>(y)),
                   static_cast<float>(static_cast<int>(z)));
        SetSize(static_cast<float>(static_cast<unsigned int>(size_x)),
                static_cast<float>(static_cast<unsigned int>(size_y)));
        for (int i=0;i<6;++i) {
            sourceVid[i]=Map->Vid(static_cast<int>(static_cast<unsigned int>(source_vid[i])));
            if (sourceVid[i]->m_idx<=0) sourceVid[i]=0;
            conversionVid[i]=Map->Vid(static_cast<int>(static_cast<unsigned int>(conversion_vid[i])));
            if (conversionVid[i]->m_idx<=0) conversionVid[i]=0;
        }
        EndDialog(hwnd,1);
        break;
    }
    case 2:
        property=g_regionSavedProperty;
        EndDialog(hwnd,0);
        break;
    case 0x41B: property^=2u; break;
    case 0x41F: property^=1u; break;
    case 0x7D1: {
        property^=8u;
        const int enabled=(property&8u) ? 0 : 1;
        x.Enable(enabled); y.Enable(enabled); z.Enable(enabled); size_x.Enable(enabled); size_y.Enable(enabled);
        break;
    }
    case 0x7D2: property^=4u; break;
    case 0x7D3: property^=0x10u; break;
    default: break;
    }
    return 0;
}

void REGION::SetFogParameters(int newBottom,int newTop,COLOR newColor)
{
    fogTop=newTop;
    fogBottom=newBottom;
    fogColor=&newColor;
    if (fogTable)
        ::operator delete(fogTable);
    if (fogBottom>=fogTop)
        return;

    const int entries=(fogTop-fogBottom)*8+1;
    fogTable=static_cast<unsigned short*>(::operator new(static_cast<unsigned int>(entries*2)));
    if (!fogTable)
        MYERROR::LogExit(::Error,"Enough memory for DrawFog",entries);

    for (int i=entries-1;i>=0;--i) {
        int intensity=(i*255)/(fogTop-fogBottom);
        intensity/=8;
        if (Graph->CapsNotPalette()) {
            RGB16 color=Graph->ColorIntensity(intensity);
            fogTable[i]=color.color;
        } else {
            fogTable[i]=static_cast<unsigned short>(intensity);
        }
    }
}

DIALOG_SLIDER::DIALOG_SLIDER(HWND__* window,int resourceId) : DIALOG_ITEM(window,resourceId) {}
void DIALOG_SLIDER::SetRange(int left,int right)
{
    Message(0x406u,1u,static_cast<long>((left & 0xFFFF) | ((right & 0xFFFF) << 16)));
}
int DIALOG_SLIDER::operator=(int pos)
{
    Message(0x405u,1u,pos);
    return pos;
}
DIALOG_SLIDER::operator int()
{
    return Message(0x400u,0u,0);
}



int __stdcall AppAbout(HWND__* hwnd,unsigned int msg,unsigned int wParam,long)
{
    if (msg==0x110u)
        return 1;
    if (msg==0x111u && (wParam&0xFFFFu)==1u)
        EndDialog(hwnd,1);
    return 0;
}

// Retail action-stack line editor.  The action strings/comments and clipboard
// coordinate shortcuts are taken directly from the original dialog procedure.
int __stdcall AppEditStackLine(HWND__* hwnd,unsigned int msg,unsigned int wParam,long)
{
    DIALOG_COMBO_BOX action(hwnd,0x421);
    DIALOG_INT var1(hwnd,0x3EF);
    DIALOG_INT var2(hwnd,0x3F0);
    DIALOG_INT var3(hwnd,0x3F1);
    DIALOG_TEXT comment(hwnd,0x3F2);
    DIALOG_BUTTON pasteXYZ(hwnd,0x44F);
    DIALOG_BUTTON pasteYZ(hwnd,0x450);
    DIALOG_BUTTON mouseXYZ(hwnd,0x451);
    DIALOG_BUTTON mouseXY(hwnd,0x452);

    // Original convenience behavior: a comma pasted into var1/var2 is treated
    // as a coordinate tuple, exactly like the corresponding buttons.
    bool wantXYZ=pasteXYZ.IsClicked(msg,wParam)!=0;
    if (!wantXYZ && var1.IsNotify(msg,wParam,0x400)) {
        STRING text=var1.GetText();
        wantXYZ=strchr(text.CharPtr(),',')!=0;
    }
    if (wantXYZ) {
        STRING clip;
        clip.ReadFromClipboard(0);
        int a=0,b=0,c=0;
        sscanf(clip.CharPtr(),"%i,%i,%i",&a,&b,&c);
        var1=a; var2=b; var3=c;
    }

    if (mouseXYZ.IsClicked(msg,wParam)) {
        var1=static_cast<int>(Mouse->X());
        var2=static_cast<int>(Mouse->Y());
        var3=static_cast<int>(Mouse->Z());
    } else {
        bool wantYZ=pasteYZ.IsClicked(msg,wParam)!=0;
        if (!wantYZ && var2.IsNotify(msg,wParam,0x400)) {
            STRING text=var2.GetText();
            wantYZ=strchr(text.CharPtr(),',')!=0;
        }
        if (wantYZ) {
            STRING clip;
            clip.ReadFromClipboard(0);
            int b=0,c=0;
            sscanf(clip.CharPtr(),"%i,%i",&b,&c);
            var2=b; var3=c;
        } else if (mouseXY.IsClicked(msg,wParam)) {
            var2=static_cast<int>(Mouse->X());
            var3=static_cast<int>(Mouse->Y());
        }
    }

    if (msg==0x110u) {
        static const int kActions[]={
            0x21,0x20,0x25,0x23,0x86,0x27,0x26,0x29,0x28,0x49,0x47,0x48,
            0x2B,0x55,0x58,0x62,0x5B,0x3F,0x3E,0x87,
            0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,
            0x0C,0x0D,0x0E,0x0F,0x10
        };
        for (unsigned int i=0;i<sizeof(kActions)/sizeof(kActions[0]);++i) {
            STRING text=ActionToText(kActions[i]);
            action.AddStringWithData(&text,kActions[i]);
        }
        STRING currentAction=ActionToText(g_unitEditAction.act);
        action.SetCurrent(&currentAction);
        STRING help=ActionComment(g_unitEditAction.act);
        comment=&help;
        var1=g_unitEditAction.var1;
        var2=g_unitEditAction.var2;
        var3=g_unitEditAction.var3;
        return 1;
    }

    if (msg==0x111u) {
        const unsigned int id=wParam&0xFFFFu;
        if (id==0x421u) {
            STRING help=ActionComment(action.GetCurrentStringData());
            comment=&help;
        } else if (id==1u) {
            g_unitEditAction.act=action.GetCurrentStringData();
            g_unitEditAction.var1=static_cast<int>(var1);
            g_unitEditAction.var2=static_cast<int>(var2);
            g_unitEditAction.var3=static_cast<int>(var3);
            EndDialog(hwnd,1);
        } else if (id==2u) {
            EndDialog(hwnd,0);
        }
    }
    return 0;
}

int __stdcall AppUnitProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogUnitProperty(hwnd,msg,wParam,lParam);
}

int MAP_EDIT::DialogUnitProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    DIALOG_BUTTON aggressive(hwnd,0x41C);
    DIALOG_BUTTON careful(hwnd,0x41D);
    DIALOG_BUTTON behavior3(hwnd,0x41E);
    DIALOG_TEXT directionStatic(hwnd,0x429);
    DIALOG_UNSIGNED direction(hwnd,0x408);
    DIALOG_INT xCoord(hwnd,0x428);
    DIALOG_INT yCoord(hwnd,0x42A);
    DIALOG_INT zCoord(hwnd,0x42B);
    DIALOG_UNSIGNED army(hwnd,0x407);
    DIALOG_LIST_BOX items(hwnd,0x3F5);
    DIALOG_BUTTON itemFilter(hwnd,0x3FE);
    DIALOG_LIST_BOX stack(hwnd,0x3F6);
    DIALOG_BUTTON stackCut(hwnd,0x43B);
    DIALOG_BUTTON stackCopy(hwnd,0x43C);
    DIALOG_BUTTON stackPaste(hwnd,0x43D);
    DIALOG_BUTTON stackDelete(hwnd,0x43E);
    DIALOG_BUTTON stackEdit(hwnd,0x43F);
    DIALOG_BUTTON stackShiftUp(hwnd,0x440);
    DIALOG_BUTTON stackShiftDown(hwnd,0x441);
    DIALOG_BUTTON copyCoordinates(hwnd,0x450);
    DIALOG_BUTTON pasteCoordinates(hwnd,0x44F);
    DIALOG_BUTTON mouseCoordinates(hwnd,0x451);
    DIALOG_COMBO_BOX vids(hwnd,0x421);

    if (selectedSprites.No()) {
        const int current=stack.GetCurrentStringData();
        stackCut.Enable(g_unitStack.No()!=0);
        stackCopy.Enable(g_unitStack.No()!=0);
        stackPaste.Enable(g_unitClipboard.No()!=0);
        const int valid=current>=0 && current<g_unitStack.No();
        stackEdit.Enable(valid || !g_unitStack.No());
        stackDelete.Enable(valid);
        stackShiftUp.Enable(current>0 && current<g_unitStack.No());
        stackShiftDown.Enable(current>=0 && current<g_unitStack.No()-1);
    } else {
        stackEdit.Enable(1);
    }

    if (copyCoordinates.IsClicked(msg,wParam)) {
        STRING text=Printf("%i,%i,%i",static_cast<int>(xCoord),static_cast<int>(yCoord),static_cast<int>(zCoord));
        text.WriteToClipboard(0);
    }

    bool paste=false;
    if (xCoord.IsNotify(msg,wParam,0x400)) {
        STRING text=xCoord.GetText();
        paste=strchr(text.CharPtr(),',')!=0;
    }
    if (!paste)
        paste=pasteCoordinates.IsClicked(msg,wParam)!=0;
    if (paste) {
        STRING clip;
        clip.ReadFromClipboard(0);
        int x=0,y=0,z=0;
        sscanf(clip.CharPtr(),"%i,%i,%i",&x,&y,&z);
        xCoord=x; yCoord=y; zCoord=z;
    } else if (mouseCoordinates.IsClicked(msg,wParam) && Mouse) {
        xCoord=static_cast<int>(Mouse->X());
        yCoord=static_cast<int>(Mouse->Y());
    }

    if (msg==0x110u) {
        if (!selectedSprites.No()) {
            EndDialog(hwnd,0);
            return 0;
        }
        SPRITE* first=*selectedSprites.First();
        STRING title=STRING("Unit Property for '")+STRING(first->Vid()->m_name)+"'";
        SetWindowTextA(hwnd,title.CharPtr());
        SendDlgItemMessageA(hwnd,0x442,0x46Fu,0,3);
        SendDlgItemMessageA(hwnd,0x443,0x46Fu,0,static_cast<long>(first->Vid()->m_noDirections-1));

        g_unitStack.operator=(first->ActionStack());
        xCoord=static_cast<int>(first->X());
        yCoord=static_cast<int>(first->Y());
        zCoord=static_cast<int>(first->Z());
        army=static_cast<unsigned int>(first->Army());
        direction=static_cast<unsigned int>(first->RealDirection());
        ANGLE logical=first->Direction();
        STRING dirText=Printf("Direction=%i",logical.Int());
        directionStatic=&dirText;
        const long behave=first->Action(0x5E,0,0,0);
        aggressive.SetCheck((behave&1)!=0);
        careful.SetCheck((behave&2)!=0);
        behavior3.SetCheck((behave&4)!=0);

        items.Reset();
        vids.Reset();
        for (int i=0;i<m_noVid;++i) {
            VID* vid=VidSlot(i);
            if (!vid)
                continue;
            if (first->IsSpriteClass(vid->m_spriteClass)) {
                STRING label=vid->GetNumberName();
                const int row=vids.AddStringWithData(&label,i);
                if (first->Vid()->m_idx==i)
                    vids.SetCurrent(row);
            }
            if (vid->IsSpriteType(0xE)) {
                STRING label=vid->GetNumberName();
                const int row=items.AddStringWithData(&label,i);
                if (first->Action(0x38,i,0,0))
                    items.SetSelected(row);
            }
        }
        StackToListBox(&stack,&g_unitStack);
        itemFilter.SetCheck(1);
        return 1;
    }

    if (msg==0x2Eu) {
        if (reinterpret_cast<HWND__*>(lParam)!=GetDlgItem(hwnd,0x3F6))
            return -1;
        int current=stack.GetCurrentStringData();
        if (current<0) current=-1;
        if (current>=g_unitStack.No()) current=g_unitStack.No()-1;
        const int key=static_cast<int>(wParam&0xFFFFu);
        const bool shift=(GetKeyState(0x10)&0x8000)!=0;
        const bool ctrl=(GetKeyState(0x11)&0x8000)!=0;

        if (shift && key==0x26) {
            g_unitStack.ShiftDown(current);
            ++current;
        } else if (shift && key==0x28) {
            g_unitStack.ShiftUp(current);
            --current;
        } else if (ctrl && (key==0x2D || key==0x43)) {
            g_unitClipboard.operator=(&g_unitStack);
        } else if ((shift && key==0x2D) || (ctrl && key==0x56)) {
            g_unitStack.operator=(&g_unitClipboard);
        } else if ((shift && key==0x2E) || (ctrl && key==0x58)) {
            g_unitClipboard.operator=(&g_unitStack);
            g_unitStack.Release();
            current=-1;
        } else if (!shift && !ctrl && key==0x2E) {
            g_unitStack.DeleteNumberS(current);
            --current;
        } else if (!shift && !ctrl && key==0x2D) {
            if (current>=0 && current<g_unitStack.No())
                g_unitEditAction=*g_unitStack[current];
            if (DialogBoxParamA(m_instance,"EDIT_STACK_LINE",hwnd,AppEditStackLine,0))
                g_unitStack.InsertBefore(current+1,g_unitEditAction);
        } else {
            return -1;
        }
        StackToListBox(&stack,&g_unitStack);
        return g_unitStack.No()-1-current;
    }

    if (msg!=0x111u)
        return 0;

    const unsigned int id=wParam&0xFFFFu;
    if (id>=0x3FEu && id<=0x401u) {
        int mask=0xE;
        if (id==0x3FFu) mask=2;
        else if (id==0x400u) mask=4;
        else if (id==0x401u) mask=8;
        items.Reset();
        SPRITE* first=*selectedSprites.First();
        for (int i=0;i<m_noVid;++i) {
            VID* vid=VidSlot(i);
            if (!vid || !vid->IsSpriteType(mask))
                continue;
            STRING label=vid->GetNumberName();
            const int row=items.AddStringWithData(&label,i);
            if (first->Action(0x38,i,0,0))
                items.SetSelected(row);
        }
        return 0;
    }

    int current=stack.GetCurrentStringData();
    if (id==0x43B) {
        g_unitClipboard.operator=(&g_unitStack);
        g_unitStack.Release();
        StackToListBox(&stack,&g_unitStack);
        return 0;
    }
    if (id==0x43C) {
        g_unitClipboard.operator=(&g_unitStack);
        return 0;
    }
    if (id==0x43D) {
        g_unitStack.operator=(&g_unitClipboard);
        StackToListBox(&stack,&g_unitStack);
        return 0;
    }
    if (id==0x43E) {
        g_unitStack.DeleteNumberS(current);
        --current;
        StackToListBox(&stack,&g_unitStack);
        stack.SetCurrent(g_unitStack.No()-1-current);
        return 0;
    }
    if (id==0x43F) {
        if (current>=0 && current<g_unitStack.No())
            g_unitEditAction=*g_unitStack[current];
        if (DialogBoxParamA(m_instance,"EDIT_STACK_LINE",hwnd,AppEditStackLine,0))
            g_unitStack.InsertBefore(current+1,g_unitEditAction);
        StackToListBox(&stack,&g_unitStack);
        stack.SetCurrent(g_unitStack.No()-1-current);
        return 0;
    }
    if (id==0x440) {
        g_unitStack.ShiftUp(current);
        --current;
        StackToListBox(&stack,&g_unitStack);
        stack.SetCurrent(g_unitStack.No()-1-current);
        return 0;
    }
    if (id==0x441) {
        g_unitStack.ShiftDown(current);
        ++current;
        StackToListBox(&stack,&g_unitStack);
        stack.SetCurrent(g_unitStack.No()-1-current);
        return 0;
    }

    if (id==0x3F6u && ((wParam>>16)&0xFFFFu)==2u) {
        current=stack.GetCurrentStringData();
        if (current>=0 && current<g_unitStack.No()) {
            g_unitEditAction=*g_unitStack[current];
            if (DialogBoxParamA(m_instance,"EDIT_STACK_LINE",hwnd,AppEditStackLine,0))
                *g_unitStack[current]=g_unitEditAction;
            StackToListBox(&stack,&g_unitStack);
        }
        return 0;
    }

    if (id==2u) {
        EndDialog(hwnd,0);
        return 0;
    }
    if (id!=1u)
        return 0;

    SPRITE* first=*selectedSprites.First();
    int behave=0;
    if (aggressive.GetCheck()) behave|=1;
    if (careful.GetCheck()) behave|=2;
    if (behavior3.GetCheck()) behave|=4;

    bool stackChanged=g_unitStack.No()!=first->ActionStack()->No();
    if (!stackChanged) {
        for (int i=0;i<g_unitStack.No();++i) {
            if (g_unitStack[i]->operator!=((*first->ActionStack())[i])) {
                stackChanged=true;
                break;
            }
        }
    }
    const int newVid=vids.GetCurrentStringData();
    const bool vidChanged=newVid!=first->Vid()->m_idx;
    bool itemsChanged=false;
    for (int row=0;row<items.NoString();++row) {
        const int item=items.GetData(row);
        const bool selected=items.IsSelected(row)!=0;
        if (selected!=(first->HaveItem(item)!=0)) {
            itemsChanged=true;
            break;
        }
    }
    const int newArmy=static_cast<int>(static_cast<unsigned int>(army));
    const int newDirection=static_cast<int>(static_cast<unsigned int>(direction));
    const int newX=static_cast<int>(xCoord);
    const int newY=static_cast<int>(yCoord);
    const int newZ=static_cast<int>(zCoord);
    const bool coordChanged=first->X()!=static_cast<float>(newX) || first->Y()!=static_cast<float>(newY) || first->Z()!=static_cast<float>(newZ);

    for (int i=selectedSprites.No()-1;i>=0;--i) {
        SPRITE* sprite=*selectedSprites[i];
        if (vidChanged)
            sprite->Action(0x3E,newVid,0,0);
        if (stackChanged)
            sprite->ActionStack()->operator=(&g_unitStack);
        if (first->Action(0x5E,0,0,0)!=behave)
            sprite->Action(0x5F,behave,0,0);
        if (first->Army()!=newArmy)
            sprite->Action(0x61,newArmy,0,0);
        if (itemsChanged) {
            sprite->Action(0x39,0,0,0);
            for (int row=0;row<items.NoString();++row) {
                if (items.IsSelected(row))
                    sprite->InsertItem(items.GetData(row));
            }
        }
        if (first->RealDirection()!=newDirection)
            sprite->ChangeRealDirection(static_cast<unsigned int>(newDirection));
        if (coordChanged) {
            sprite->ChangeCoor(sprite->X()+static_cast<float>(newX)-first->X(),
                               sprite->Y()+static_cast<float>(newY)-first->Y(),
                               static_cast<float>(newZ));
        }
    }
    EndDialog(hwnd,1);
    return 0;
}


int __stdcall AppSelectVid(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogSelectVid(hwnd,msg,wParam,lParam);
}

int MAP_EDIT::DialogSelectVid(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    DIALOG_LIST_BOX vids(hwnd,0x3F5);
    DIALOG_BUTTON sort(hwnd,0x7D3);
    DIALOG_TABS tabs(hwnd,0x44D);

    if (tabs.IsSelChange(msg,wParam,lParam)) {
        ChangeMouseVid(Mouse->Vid(),static_cast<unsigned int>(tabs.GetCurrentItemData()));
        VidToListBox(&vids,static_cast<unsigned long>(spriteType),Mouse->Vid()->m_idx,optSortVid);
    } else if (vids.IsDoubleClicked(msg,wParam)) {
        ChangeMouseVid(Vid(vids.GetCurrentStringData()),static_cast<unsigned int>(spriteType));
        EndDialog(hwnd,1);
    } else if (sort.IsClicked(msg,wParam)) {
        optSortVid=!optSortVid;
        VidToListBox(&vids,static_cast<unsigned long>(spriteType),Mouse->Vid()->m_idx,optSortVid);
    }

    if (msg==0x110u) {
        STRING text("Terrain"); tabs.AddItemWithData(&text,1);
        text="Object";  tabs.AddItemWithData(&text,2);
        text="Unit";    tabs.AddItemWithData(&text,4);
        text="Avia";    tabs.AddItemWithData(&text,8);
        text="Menu";    tabs.AddItemWithData(&text,0x10);
        text="Railway"; tabs.AddItemWithData(&text,0x20);
        text="Region";  tabs.AddItemWithData(&text,0x40);
        tabs.SetMinWidth(50);
        tabs.SetCurrentWithData(spriteType);
        sort.SetCheck(optSortVid);
        vids.SetColumnWidth(210);
        VidToListBox(&vids,static_cast<unsigned long>(spriteType),Mouse->Vid()->m_idx,optSortVid);
        return 1;
    }
    if (msg==0x111u) {
        switch (wParam & 0xFFFFu) {
        case 1:
            ChangeMouseVid(Vid(vids.GetCurrentStringData()),static_cast<unsigned int>(spriteType));
            EndDialog(hwnd,1);
            break;
        case 2:
            EndDialog(hwnd,0);
            break;
        }
    }
    return 0;
}

namespace {
GAMMA g_mapPropertySavedGamma;
}

int __stdcall AppMapProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogMapProperty(hwnd,msg,wParam,lParam);
}

int MAP_EDIT::DialogMapProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long)
{
    DIALOG_UNSIGNED sizeX(hwnd,0x3F0);
    DIALOG_UNSIGNED sizeY(hwnd,0x3F1);
    DIALOG_UNSIGNED windDirection(hwnd,0x3F2);
    DIALOG_UNSIGNED windSpeed(hwnd,0x3F3);
    DIALOG_TEXT gammaText(hwnd,0x3EF);
    DIALOG_SLIDER gammaRed(hwnd,0x409);
    DIALOG_SLIDER gammaGreen(hwnd,0x408);
    DIALOG_SLIDER gammaBlue(hwnd,0x407);
    DIALOG_BUTTON environment(hwnd,0x41A);

    if (msg==0x110u) {
        GAMMA current=Graph->GetGamma();
        g_mapPropertySavedGamma=&current;
        sizeX=static_cast<unsigned int>(Map->SizeX());
        sizeY=static_cast<unsigned int>(Map->SizeY());
        windDirection=static_cast<unsigned int>(Graph->WindDirection().Int());
        windSpeed=static_cast<unsigned int>(Graph->WindSpeed()*1000.0f);
        environment.SetCheck(Graph->IsEnvironment(1));
        gammaRed.SetRange(-255,255);
        gammaGreen.SetRange(-255,255);
        gammaBlue.SetRange(-255,255);
        current=Graph->GetGamma(); gammaRed=-current.Red();
        current=Graph->GetGamma(); gammaGreen=-current.Green();
        current=Graph->GetGamma(); gammaBlue=-current.Blue();
        current=Graph->GetGamma();
        STRING text=Printf("%X",current.EncodeToDword());
        gammaText=&text;
        return 1;
    }

    if (msg==0x111u) {
        switch (wParam & 0xFFFFu) {
        case 1: {
            const unsigned int sx=sizeX;
            const unsigned int sy=sizeY;
            if (sx != static_cast<unsigned int>(Map->SizeX()) || sy != static_cast<unsigned int>(Map->SizeY()))
                Map->ChangeSizeXY(static_cast<float>(sx),static_cast<float>(sy));
            Map->SetScrollBox(-450.0f,-450.0f,Map->SizeX()+450.0f,Map->SizeY()+450.0f);
            Graph->SetWind(static_cast<int>(static_cast<unsigned int>(windSpeed)),ANGLE(static_cast<uint8_t>(static_cast<unsigned int>(windDirection))));
            GAMMA gamma(-static_cast<int>(gammaRed),-static_cast<int>(gammaGreen),-static_cast<int>(gammaBlue));
            Graph->SetGamma(&gamma);
            Graph->SetEnvironment(environment.GetCheck() ? 1u : 0x80000001u);
            EndDialog(hwnd,1);
            break;
        }
        case 2:
            Graph->SetGamma(&g_mapPropertySavedGamma);
            EndDialog(hwnd,0);
            break;
        case 0x437: {
            GAMMA gamma=Graph->GetGamma(); gamma.SetRed(0); Graph->SetGamma(&gamma); gammaRed=0;
            GAMMA now=Graph->GetGamma(); STRING text=Printf("%X",now.EncodeToDword()); gammaText=&text;
            UpdateScreenForDlg(hwnd);
            break;
        }
        case 0x438: {
            GAMMA gamma=Graph->GetGamma(); gamma.SetGreen(0); Graph->SetGamma(&gamma); gammaGreen=0;
            GAMMA now=Graph->GetGamma(); STRING text=Printf("%X",now.EncodeToDword()); gammaText=&text;
            UpdateScreenForDlg(hwnd);
            break;
        }
        case 0x439: {
            GAMMA gamma=Graph->GetGamma(); gamma.SetBlue(0); Graph->SetGamma(&gamma); gammaBlue=0;
            GAMMA now=Graph->GetGamma(); STRING text=Printf("%X",now.EncodeToDword()); gammaText=&text;
            UpdateScreenForDlg(hwnd);
            break;
        }
        }
        return 0;
    }

    if (msg==3u) {
        UpdateScreenForDlg(hwnd);
        return 0;
    }

    if (msg==0x115u) {
        const unsigned int code=wParam & 0xFFFFu;
        if (code==8u || code==5u) {
            GAMMA gamma(-static_cast<int>(gammaRed),-static_cast<int>(gammaGreen),-static_cast<int>(gammaBlue));
            Graph->SetGamma(&gamma);
            GAMMA now=Graph->GetGamma(); STRING text=Printf("%X",now.EncodeToDword()); gammaText=&text;
            if (code==8u)
                UpdateScreenForDlg(hwnd);
        }
    }
    return 0;
}

namespace {
int g_convertSourceVid=5;
int g_convertDestVid=5;
int g_convertDelete=1;
int g_convertRandomDirection=0;
unsigned int g_convertU0=0,g_convertU1=0,g_convertU2=0,g_convertU3=0;
int g_convertOffsetX=0,g_convertOffsetY=0,g_convertOffsetZ=0;
}

int __stdcall AppConvertSprite(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogConvertSprite(hwnd,msg,wParam,lParam);
}

int MAP_EDIT::DialogConvertSprite(HWND__* hwnd,unsigned int msg,unsigned int wParam,long)
{
    DIALOG_COMBO_BOX source(hwnd,0x421), dest(hwnd,0x422);
    DIALOG_BUTTON deleteOld(hwnd,0x7D1), randomDirection(hwnd,0x7D2);
    DIALOG_UNSIGNED u0(hwnd,0x3F0),u1(hwnd,0x3F1),u2(hwnd,0x3F3),u3(hwnd,0x3F4);
    DIALOG_INT offsetX(hwnd,0x3EF),offsetY(hwnd,0x3F2),offsetZ(hwnd,0x3F5);

    if (msg==0x110u) {
        VidToControlBox(&source,0x3Fu,g_convertSourceVid);
        VidToControlBox(&dest,0x3Fu,g_convertDestVid);
        deleteOld.SetCheck(g_convertDelete);
        randomDirection.SetCheck(g_convertRandomDirection);
        u0=g_convertU0; u1=g_convertU1; u2=g_convertU2; u3=g_convertU3;
        offsetX=g_convertOffsetX; offsetY=g_convertOffsetY; offsetZ=g_convertOffsetZ;
        return 1;
    }
    if (msg!=0x111u) return 0;
    switch (wParam & 0xFFFFu) {
    case 1: {
        g_convertSourceVid=source.GetCurrentStringData();
        g_convertDestVid=dest.GetCurrentStringData();
        g_convertDelete=static_cast<int>(deleteOld.GetCheck());
        g_convertRandomDirection=static_cast<int>(randomDirection.GetCheck());
        g_convertU0=u0; g_convertU1=u1; g_convertU2=u2; g_convertU3=u3;
        g_convertOffsetX=offsetX; g_convertOffsetY=offsetY; g_convertOffsetZ=offsetZ;

        int index=0;
        SPRITE* sprite=FirstSprite(Vid(g_convertSourceVid)->m_layer,&index);
        while (sprite) {
            if (sprite->Vid()->m_idx==g_convertSourceVid) {
                ANGLE direction = g_convertRandomDirection ? ANGLE(static_cast<uint8_t>(Random(255))) : sprite->Direction();
                const float x=sprite->X()+static_cast<float>(g_convertOffsetX);
                const float y=sprite->Y()+static_cast<float>(g_convertOffsetY);
                const float z=sprite->Z()+static_cast<float>(g_convertOffsetZ);
                if (g_convertDelete)
                    sprite->ScalarDeletingDestructor(1);
                CreateSprite(Vid(g_convertDestVid),x,y,z,direction,0);
            }
            sprite=NextSprite(Vid(g_convertSourceVid)->m_layer,&index);
        }
        EndDialog(hwnd,1);
        break;
    }
    case 2:
        EndDialog(hwnd,0);
        break;
    }
    return 0;
}


namespace {
unsigned long g_textPropertyFlags=0;   // MapEdit.exe .bss 0x004F0C10
int g_textPropertyResetActions=0;      // MapEdit.exe .bss 0x004F0C18
}

int __stdcall AppTextProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogTextProperty(hwnd,msg,wParam,lParam);
}

int MAP_EDIT::DialogTextProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long)
{
    DIALOG_TEXT directionStatic(hwnd,0x429);
    DIALOG_UNSIGNED direction(hwnd,0x408);
    DIALOG_INT zCoord(hwnd,0x428);
    DIALOG_INT yCoord(hwnd,0x42A);
    DIALOG_INT xCoord(hwnd,0x42B);
    DIALOG_UNSIGNED army(hwnd,0x407);
    DIALOG_COMBO_BOX alignX(hwnd,0x421);
    DIALOG_COMBO_BOX alignY(hwnd,0x422);
    DIALOG_TEXT name(hwnd,0x3EF);

    if (msg==0x110u) {
        if (!selectedSprites.No()) {
            EndDialog(hwnd,0);
            return 0;
        }

        SendDlgItemMessageA(hwnd,0x442,0x46Fu,0,3);
        SPRITE* first=*selectedSprites.First();
        SendDlgItemMessageA(hwnd,0x443,0x46Fu,0,static_cast<long>(first->Vid()->m_noDirections-1));
        g_textPropertyResetActions=0;

        const STRING* currentName=reinterpret_cast<const STRING*>(first->Action(0x79,0,0,0));
        name=currentName;
        g_textPropertyFlags=static_cast<unsigned long>(first->Action(0x5E,0,0,0));
        direction=static_cast<unsigned int>(first->RealDirection());
        zCoord=static_cast<int>(first->Z());
        yCoord=static_cast<int>(first->Y());
        xCoord=static_cast<int>(first->X());
        army=static_cast<unsigned int>(first->Army());

        ANGLE logicalDirection=first->Direction();
        STRING directionText=Printf("Direction=%i",logicalDirection.Int());
        directionStatic=&directionText;

        STRING left("Left"),centerX("Center"),right("Right");
        alignX.AddString(&left); alignX.AddString(&centerX); alignX.AddString(&right);
        STRING top("Top"),centerY("Center"),bottom("Bottom");
        alignY.AddString(&top); alignY.AddString(&centerY); alignY.AddString(&bottom);

        if (g_textPropertyFlags&1u) alignX.SetCurrent(1);
        else if (g_textPropertyFlags&2u) alignX.SetCurrent(2);
        else alignX.SetCurrent(0);
        if (g_textPropertyFlags&8u) alignY.SetCurrent(1);
        else if (g_textPropertyFlags&4u) alignY.SetCurrent(2);
        else alignY.SetCurrent(0);

        int radioId=0x449;
        switch (g_textPropertyFlags&0x70u) {
        case 0x10u: radioId=0x444; break;
        case 0x20u: radioId=0x445; break;
        case 0x30u: radioId=0x446; break;
        case 0x40u: radioId=0x447; break;
        case 0x50u: radioId=0x448; break;
        case 0x60u: radioId=0x44B; break;
        default: break;
        }
        SendDlgItemMessageA(hwnd,radioId,0xF1u,1,0);
        return 1;
    }

    if (msg!=0x111u)
        return 0;

    const unsigned int id=wParam&0xFFFFu;
    switch (id) {
    case 0x44A:
        g_textPropertyResetActions=1;
        return 0;
    case 0x449: g_textPropertyFlags&=~0x70u; return 0;
    case 0x444: g_textPropertyFlags=(g_textPropertyFlags&~0x70u)|0x10u; return 0;
    case 0x445: g_textPropertyFlags=(g_textPropertyFlags&~0x70u)|0x20u; return 0;
    case 0x446: g_textPropertyFlags=(g_textPropertyFlags&~0x70u)|0x30u; return 0;
    case 0x447: g_textPropertyFlags=(g_textPropertyFlags&~0x70u)|0x40u; return 0;
    case 0x448: g_textPropertyFlags=(g_textPropertyFlags&~0x70u)|0x50u; return 0;
    case 0x44B: g_textPropertyFlags=(g_textPropertyFlags&~0x70u)|0x60u; return 0;
    case 2:
        EndDialog(hwnd,0);
        return 0;
    case 1:
        break;
    default:
        return 0;
    }

    g_textPropertyFlags&=0xF0u;
    const int xAlign=alignX.GetCurrentStringIndex();
    if (xAlign==1) g_textPropertyFlags|=1u;
    else if (xAlign==2) g_textPropertyFlags|=2u;
    const int yAlign=alignY.GetCurrentStringIndex();
    if (yAlign==1) g_textPropertyFlags|=8u;
    else if (yAlign==2) g_textPropertyFlags|=4u;

    SPRITE* first=*selectedSprites.First();
    const int newDirection=static_cast<int>(static_cast<unsigned int>(direction));
    const int newArmy=static_cast<int>(static_cast<unsigned int>(army));
    const int newX=static_cast<int>(xCoord);
    const int newY=static_cast<int>(yCoord);
    const int newZ=static_cast<int>(zCoord);

    for (int i=selectedSprites.No()-1;i>=0;--i) {
        SPRITE* sprite=*selectedSprites[i];
        if (g_textPropertyResetActions)
            sprite->ResetActionStack();
        if (sprite->RealDirection()!=newDirection)
            sprite->ChangeRealDirection(static_cast<unsigned int>(newDirection));
        if (sprite->Army()!=newArmy)
            sprite->Action(0x61,newArmy,0,0);

        if (sprite->X()!=static_cast<float>(newX) || sprite->Y()!=static_cast<float>(newY) || sprite->Z()!=static_cast<float>(newZ)) {
            sprite->ChangeCoor(sprite->X()+static_cast<float>(newX)-first->X(),
                               sprite->Y()+static_cast<float>(newY)-first->Y(),
                               sprite->Z()+static_cast<float>(newZ)-first->Z());
        }

        if (static_cast<unsigned long>(sprite->Action(0x5E,0,0,0))!=g_textPropertyFlags)
            sprite->Action(0x5F,static_cast<long>(g_textPropertyFlags),0,0);

        STRING newName=static_cast<STRING>(name);
        const STRING* oldName=reinterpret_cast<const STRING*>(sprite->Action(0x79,0,0,0));
        bool updateName=!oldName || const_cast<STRING*>(oldName)->operator!=(&newName);
        if (!updateName)
            updateName=static_cast<unsigned long>(sprite->Action(0x5E,0,0,0))!=g_textPropertyFlags;
        if (updateName) {
            STRING copy(newName);
            sprite->Action(0x78,reinterpret_cast<long>(&copy),0,0);
        }
    }
    EndDialog(hwnd,1);
    return 0;
}

namespace {
int g_optionsShowHidden=0; // MapEdit.exe .bss 0x004F0C50
}

int __stdcall AppOptions(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam)
{
    return static_cast<MAP_EDIT*>(Map)->DialogOptions(hwnd,msg,wParam,lParam);
}

int MAP_EDIT::DialogOptions(HWND__* hwnd,unsigned int msg,unsigned int wParam,long)
{
    DIALOG_UNSIGNED airBrushSize(hwnd,0x3EF);
    DIALOG_UNSIGNED airBrushDensity(hwnd,0x3F0);
    DIALOG_UNSIGNED bigStepZ(hwnd,0x3F1);
    DIALOG_LIST_BOX vids(hwnd,0x3F5);
    DIALOG_BUTTON showHidden(hwnd,0x7D1);

    if (msg==0x110u) {
        airBrushSize=static_cast<unsigned int>(optAirBrushSize);
        airBrushDensity=optAirBrushDensity;
        bigStepZ=optBigStepZ;
        showHidden.SetCheck(g_optionsShowHidden);
        VidToListBox(&vids,0x6Fu,-2,g_optionsShowHidden);
        for (int i=vids.NoString()-1;i>=0;--i) {
            if (Vid(vids.GetData(i))->PropHide())
                vids.SetSelected(i);
        }
        return 1;
    }

    if (msg==0x111u) {
        const unsigned int id=wParam & 0xFFFFu;
        switch (id) {
        case 1:
            optAirBrushSize=static_cast<int>(static_cast<unsigned int>(airBrushSize));
            optAirBrushDensity=static_cast<unsigned int>(airBrushDensity);
            optBigStepZ=static_cast<unsigned int>(bigStepZ);
            for (int i=vids.NoString()-1;i>=0;--i)
                Vid(vids.GetData(i))->SetPropHide(vids.IsSelected(i));
            EndDialog(hwnd,1);
            break;
        case 2:
            EndDialog(hwnd,0);
            break;
        case 0x7D1:
            g_optionsShowHidden^=1;
            VidToListBox(&vids,0x6Fu,-2,g_optionsShowHidden);
            for (int i=vids.NoString()-1;i>=0;--i) {
                if (Vid(vids.GetData(i))->PropHide())
                    vids.SetSelected(i);
            }
            break;
        case 0x431:
            airBrushSize=5u;
            airBrushDensity=20u;
            break;
        case 0x432:
            airBrushSize=10u;
            airBrushDensity=20u;
            break;
        case 0x433:
            airBrushSize=50u;
            airBrushDensity=5u;
            break;
        case 0x3FE:
        case 0x42A:
        case 0x42B:
        case 0x400:
        case 0x401: {
            unsigned int typeMask=0;
            switch (id) {
            case 0x3FE: typeMask=0x6Fu; break;
            case 0x42A: typeMask=1u; break;
            case 0x42B: typeMask=2u; break;
            case 0x400: typeMask=4u; break;
            default:    typeMask=8u; break;
            }
            DIALOG_BUTTON typeButton(hwnd,static_cast<int>(id));
            const int checked=typeButton.GetCheck() ? 1 : 0;
            for (int i=vids.NoString()-1;i>=0;--i) {
                VID* vid=Vid(vids.GetData(i));
                if (vid->IsSpriteType(typeMask)) {
                    if (checked) vids.SetSelected(i);
                    else vids.SetUnSelected(i);
                }
                vid->SetPropHide(vids.IsSelected(i));
            }
            UpdateScreenForDlg(hwnd);
            break;
        }
        default:
            break;
        }
        return 0;
    }

    if (msg==3u) {
        for (int i=vids.NoString()-1;i>=0;--i)
            Vid(vids.GetData(i))->SetPropHide(vids.IsSelected(i));
        UpdateScreenForDlg(hwnd);
    }
    return 0;
}
