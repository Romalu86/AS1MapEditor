#include "mapedit/runtime.hpp"

// Original MapEdit global storage owners. Most pointers/counters are zero-initialized
// in the retail image; GRAPH_INIT below is a notable initialized .data owner.
MAP* Map = 0;
GRAPH* Graph = 0;
REGISTRY* Registry = 0;
PROFILE* Profile = 0;
// FUNCTION/DATA OWNER: MAPEDIT 0x004BC978
// Retail MapEdit.exe stores GRAPH_INIT as initialized .data, not zero-filled BSS.
// Exact 0x238-byte semantic contents derived from original/MapEdit.exe:
//   modes: 640x480, 800x600, 1024x768, 1280x1024
//   bpp:   16, 32
//   options=1, defaultDevice=0, default=640x480x16 fullscreen.
// Leaving this object zero-initialized makes GRAPH_INIT::HaveMode reject every
// DirectDraw mode and leaves START_DIALOG's resolution combo empty.
GRAPH_INIT GraphInit = {
    "",
    { 640u, 800u, 1024u, 1280u },
    { 480u, 600u,  768u, 1024u },
    { 16u, 32u },
    1u, 0u,
    640, 480, 16, 1
};
MOUSE* Mouse = 0;
HASH_MAP* Hash = 0;
MYERROR* Error = 0;
SOUND* Sound = 0;
CONSTANT* Const = 0;

int ENGINE::globaldeleting = 0;
int EvFunctionNumber[64] = {};
float g_shiftSpeedX = 0.0f;
float g_shiftSpeedY = 0.0f;
int g_unitArmyChangeScript = 0; // MapEdit.exe 0x004CE450; retail initial value is zero.
int g_vidMemoryInUse = 0;

// Retail .data @ VA 0x004BCBE0 contains 01 00 00 00.  MAP_EDIT::Load
// consumes this one-shot flag to run maps\default.lgc and then clears it.
int g_editorUnknownFlag4BCBE0 = 1;
int g_previousSpriteType = 0;
int g_inputKeyLeftPrimary = 0x25;
int g_inputKeyLeftSecondary = 0x25;
int g_inputKeyRightPrimary = 0x27;
int g_inputKeyRightSecondary = 0x27;
int g_inputKeyUpPrimary = 0x26;
int g_inputKeyUpSecondary = 0x26;
int g_inputKeyDownPrimary = 0x28;
int g_inputKeyDownSecondary = 0x28;
int g_inputFirstPrimary = 1;
int g_inputFirstSecondary = 1;
int g_inputSecondPrimary = 2;
int g_inputSecondSecondary = 2;
int g_inputPrevPrimary = 0xDB;
int g_inputNextPrimary = 0xDD;
int g_relativeControl = 0;
int g_inputAllowFirst = 1;
int g_inputAllowSecond = 1;
int g_windowScreenX = 0;
int g_windowScreenY = 0;

unsigned long PrevCurrentTime = 0;
unsigned long CurrentTime = 0;
unsigned long PrevRealCurrentTime = 0;
unsigned long RealCurrentTime = 0;
unsigned long prev_second_time = 0;

// R_MAP is a true static object in the original editor. Its constructor sets
// the 10000x10000 default bounds and constructs the embedded LIST<R_DOT*>.
R_MAP RailMap;

unsigned long PauseOldClock=0; // original global 0x004CE514
