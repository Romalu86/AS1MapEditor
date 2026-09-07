#pragma once
// Global objects and free-function declarations. Included in ABI order by mapedit/runtime.hpp.

unsigned char CountByteGamma(int b1,int b2,int time);
int CountGamma(int g1,int g2,int time);
ENGINE* GetTrainEng(int army,int ordinal,int* physicalOrdinal);
const STRING GetTimeString();
const STRING GetDateString();
const STRING FGetTime(const STRING* file);

extern MAP* Map;
extern GRAPH* Graph;
extern REGISTRY* Registry;
extern PROFILE* Profile;
extern GRAPH_INIT GraphInit;
extern MOUSE* Mouse;
extern HASH_MAP* Hash;
extern R_MAP RailMap;
extern MYERROR* Error;
extern SOUND* Sound;
extern CONSTANT* Const;
extern float g_shiftSpeedX;      // original 0x004CE40C
extern float g_shiftSpeedY;      // original 0x004CE410
extern int g_unitArmyChangeScript; // original 0x004CE450; UNIT::Action army-change script function id.
extern int g_editorDeleteLatch; // original 0x004CE354; transient delete-mode latch used by MAP_EDIT::Tact.
extern int g_editorUnknownFlag4BCBE0; // exact original address 0x004BCBE0; semantic name not yet evidenced.
extern int g_previousSpriteType; // exact original address; proven to hold the previous MAP_EDIT::spriteType.
extern VID* EmptyVid;
extern int g_vidMemoryInUse; // original 0x004EE694
extern float g_controlPanelPreviousViewXMax; // original global at 0x004CE378
extern unsigned long PrevCurrentTime;
extern unsigned long PauseOldClock;
extern unsigned long CurrentTime;
extern float FSin[256];
extern unsigned long PrevRealCurrentTime;
extern unsigned long RealCurrentTime;
extern unsigned long prev_second_time;
extern int g_inputKeyLeftPrimary;   // original 0x004C01D8, retail init VK_LEFT.
extern int g_inputKeyLeftSecondary; // original 0x004C01DC, retail init VK_LEFT.
extern int g_inputKeyRightPrimary;  // original 0x004C01E0, retail init VK_RIGHT.
extern int g_inputKeyRightSecondary;// original 0x004C01E4, retail init VK_RIGHT.
extern int g_inputKeyUpPrimary;     // original 0x004C01E8, retail init VK_UP.
extern int g_inputKeyUpSecondary;   // original 0x004C01EC, retail init VK_UP.
extern int g_inputKeyDownPrimary;   // original 0x004C01F0, retail init VK_DOWN.
extern int g_inputKeyDownSecondary; // original 0x004C01F4, retail init VK_DOWN.
extern int g_inputFirstPrimary;     // original 0x004C01F8, retail init 1 (left mouse binding).
extern int g_inputFirstSecondary;   // original 0x004C01FC, retail init 1.
extern int g_inputSecondPrimary;    // original 0x004C0200, retail init 2 (right mouse binding).
extern int g_inputSecondSecondary;  // original 0x004C0204, retail init 2.
extern int g_inputPrevPrimary;       // original 0x004C0208.
extern int g_inputNextPrimary;       // original 0x004C020C.
extern int g_relativeControl;        // original 0x004F06D0.
extern int g_inputAllowFirst;       // original 0x004C0210, retail init 1.
extern int g_inputAllowSecond;      // original 0x004C0214, retail init 1.
extern int g_windowScreenX;         // original 0x004F06C8; updated from GetWindowRect.
extern int g_windowScreenY;         // original 0x004F06CC; updated from GetWindowRect.
extern int EvFunctionNumber[64];     // original 0x004CE414, script event IDs.


int Max(int arg1,int arg2);
int Min(int arg1,int arg2);
int NearBetween(float x,float x1,float x2,float err);
int Between(float x,float x1,float x2);
int Between(int x,int x1,int x2);
int Random(int interval);
int ActionNeedSpriteInVar1(int act);
int ActionReturnSprite(int act);
void WordSet(void* dest,int cword,int noword);
float Random(float interval);
int FExist(const STRING* name);
int InSegment(float x,float center,float halfsize);
int IsSpriteCorrectForGetSprite(SPRITE* sprite,int type);

FILE* FOpen(const STRING* name,const char* mode);
STRING FCurrentDirectory();
STRING FTempFile(const char* path,const char* prefix);
int FRemove(const STRING* name);
int FRename(const STRING* oldName,const STRING* newName);
STRING Int2Str(int value);
int __stdcall AppControlPanel(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppSelectVid(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppMapProperty(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppConvertSprite(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppTextProperty(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppOptions(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppRegionProperty(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppAbout(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppEditStackLine(HWND__*,unsigned int,unsigned int,long);
int __stdcall AppUnitProperty(HWND__*,unsigned int,unsigned int,long);
void UpdateScreenForDlg(HWND__* hwnd);
int __stdcall AppStart(HWND__*,unsigned int,unsigned int,long);
long __stdcall AppWndProc(HWND__*,unsigned int,unsigned int,long);
int __stdcall WinMain(HINSTANCE__*,HINSTANCE__*,char*,int);
