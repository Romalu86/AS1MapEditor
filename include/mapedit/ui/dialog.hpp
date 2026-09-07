#pragma once
// DIALOG owner family. Included in ABI order by mapedit/runtime.hpp.
class DIALOG_ITEM {
public:
    HWND__* hWnd; int id;
    DIALOG_ITEM(HWND__* wnd, int rc_id);
    int Message(unsigned int msg, unsigned int w, long l);
    int IsNotify(unsigned int msg,unsigned int wParam,unsigned short notifyMessage);
    void SetText(const STRING* str);
    STRING GetText();
    void SetUnsigned(unsigned int val);
    unsigned int GetUnsigned();
    void SetInt(int val);
    int GetInt();
    void Enable(int flag);
    void Disable();
};
class DIALOG_COMBO_BOX : public DIALOG_ITEM {
public:
    DIALOG_COMBO_BOX(HWND__*,int);
    void Reset();
    int NoString();
    int AddString(const STRING* str);
    int AddStringWithData(const STRING* str,int data);
    void SetCurrent(int index);
    void SetCurrent(const STRING* str);
    int GetCurrentStringIndex();
    int GetData(int index);
    int GetCurrentStringData();
};
class DIALOG_BUTTON : public DIALOG_ITEM {
public:
    DIALOG_BUTTON(HWND__*,int);
    unsigned int GetCheck();
    void SetCheck(int check);
    int IsClicked(unsigned int msg,unsigned int wParam);
};
class DIALOG_LIST_BOX : public DIALOG_ITEM {
public:
    DIALOG_LIST_BOX(HWND__*,int);
    int NoString();
    void Reset();
    void SetColumnWidth(int width);
    int AddString(const STRING* str);
    int AddStringWithData(const STRING* str,int data);
    void SetCurrent(int index);
    int GetCurrentStringIndex();
    int GetData(int index);
    int GetCurrentStringData();
    int GetStringDataIndex(int data);
    int IsSelChange(unsigned int msg,unsigned int wParam);
    int IsDoubleClicked(unsigned int msg,unsigned int wParam);
    void SetSelected(int index);
    int IsSelected(int index);
    void SetUnSelected(int index);
};
class DIALOG_TABS : public DIALOG_ITEM {
public:
    DIALOG_TABS(HWND__*,int);
    int NoItem();
    int SetMinWidth(int width);
    int AddItemWithData(const STRING* str,int data);
    void SetCurrentWithData(int data);
    void SetCurrent(int index);
    int GetData(int index);
    int GetCurrentItemData();
    int GetCurrentItemIndex();
    int IsSelChange(unsigned int msg,unsigned int wParam,long lParam);
};
class DIALOG_TEXT : public DIALOG_ITEM {
public:
    DIALOG_TEXT(HWND__*,int);
    const STRING* operator=(const STRING* str);
    operator STRING();
};
class DIALOG_UNSIGNED : public DIALOG_ITEM {
public:
    DIALOG_UNSIGNED(HWND__*,int);
    unsigned int operator=(unsigned int pos);
    operator unsigned int();
};
class DIALOG_INT : public DIALOG_ITEM {
public:
    DIALOG_INT(HWND__*,int);
    int operator=(int pos);
    operator int();
};
class DIALOG_SLIDER : public DIALOG_ITEM {
public:
    DIALOG_SLIDER(HWND__*,int);
    void SetRange(int left,int right);
    int operator=(int pos);
    operator int();
};
static_assert(sizeof(DIALOG_UNSIGNED)==8, "MapEdit DIALOG_UNSIGNED size");
static_assert(sizeof(DIALOG_INT)==8, "MapEdit DIALOG_INT size");
static_assert(sizeof(DIALOG_SLIDER)==8, "MapEdit DIALOG_SLIDER size");
class DIALOG_RADIO : public DIALOG_ITEM {
    int lastId;
public:
    DIALOG_RADIO(HWND__*,int firstId,int lastId);
    void Check(int index);
    void SetItem(int index,const STRING* str);
    int GetIndexClicked(unsigned int msg,unsigned int wParam);
    int GetIndexDblClicked(unsigned int msg,unsigned int wParam);
};
static_assert(sizeof(DIALOG_ITEM)==8, "MapEdit DIALOG_ITEM size");
static_assert(sizeof(DIALOG_LIST_BOX)==8, "MapEdit DIALOG_LIST_BOX size");
static_assert(sizeof(DIALOG_TABS)==8, "MapEdit DIALOG_TABS size");
static_assert(sizeof(DIALOG_TEXT)==8, "MapEdit DIALOG_TEXT size");
static_assert(sizeof(DIALOG_RADIO)==12, "MapEdit DIALOG_RADIO size");

