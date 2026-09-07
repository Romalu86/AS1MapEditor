#include "mapedit/runtime.hpp"

extern "C" unsigned int __stdcall timeBeginPeriod(unsigned int);
extern "C" long __stdcall CoInitialize(void*);
extern "C" void* __stdcall LoadIconA(HINSTANCE__*,const char*);
extern "C" void* __stdcall GetStockObject(int);
extern "C" unsigned short __stdcall RegisterClassA(const void*);
extern "C" HWND__* __stdcall CreateWindowExA(unsigned long,const char*,const char*,unsigned long,int,int,int,int,HWND__*,void*,HINSTANCE__*,void*);
extern "C" int __stdcall UpdateWindow(HWND__*);
extern "C" HACCEL__* __stdcall LoadAcceleratorsA(HINSTANCE__*,const char*);
extern "C" int __stdcall ShowCursor(int);
extern "C" int __stdcall DialogBoxParamA(HINSTANCE__*,const char*,HWND__*,int (__stdcall *)(HWND__*,unsigned int,unsigned int,long),long);
extern "C" char* _pgmptr;

extern int __stdcall AppStart(HWND__*,unsigned int,unsigned int,long);
extern long __stdcall AppWndProc(HWND__*,unsigned int,unsigned int,long);

namespace {
struct WNDCLASSA_OLD {
    unsigned int style;
    long (__stdcall *lpfnWndProc)(HWND__*,unsigned int,unsigned int,long);
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE__* hInstance;
    void* hIcon;
    void* hCursor;
    void* hbrBackground;
    const char* lpszMenuName;
    const char* lpszClassName;
};
static_assert(sizeof(WNDCLASSA_OLD)==40,"Win32 WNDCLASSA x86 ABI");

}

// Retail startup constructor.  The related game source is used only for names;
// the MapEdit-specific GRAPH_INIT defaults, PLAYER_STEAM allocation order and
// startup-load path are taken from the editor executable.
MAP::MAP(HINSTANCE__* instance,HINSTANCE__* prev,const STRING* command_line,int sw,GRAPH_INIT* init)
{
    timeBeginPeriod(1);
    RealCurrentTime=timeGetTime();
    PrevRealCurrentTime=RealCurrentTime-10;
    ANGLE::Init();

    // Exact retail flag initialization sequence, map.cpp:57..67.
    m_flags &= ~0x00080000u;
    m_flags &= ~0x00000002u;
    m_flags=(m_flags&~1u)|((init->options&1u)?1u:0u);
    m_flags &= ~0x00000004u;
    m_flags &= ~0x00000010u;
    m_flags &= ~0x00004000u;
    m_flags &= ~0x00000100u;
    m_flags &= ~0x00000200u;
    m_flags |=  0x00010000u;
    m_flags &= ~0x00000008u;
    m_flags &= ~0x00000800u;
    m_flags &= ~0x00008000u;
    m_flags |=  0x00000080u;
    m_flags &= ~0x00000040u;
    m_flags |=  0x00001000u;
    m_flags &= ~0x00000400u;
    m_flags &= ~0x00000020u;
    m_flags &= ~0x00002000u;
    m_flags &= ~0x00020000u;
    m_flags |=  0x00100000u;

    m_groundz=0;
    m_tempGroundz=0;
    m_w=640.0f;
    m_h=480.0f;
    m_curArmy=0;
    m_shiftX=0.0f;
    m_shiftY=1.0f;
    m_weapon=0;
    m_noWeapon=0;
    m_noVid=0;
    m_noTact=0;
    m_unknown30=RealCurrentTime;
    m_speed=1.0f;
    m_fps=0;
    m_fpsCnt=0;
    m_shiftFlag=1;
    memset(m_vids,0,sizeof(m_vids));
    memset(m_player,0,sizeof(m_player));
    m_instance=instance;
    ResetGroundZ();

    ::Error=new MYERROR(1);

    STRING exePath(_pgmptr ? _pgmptr : "MapEdit.exe");
    STRING exeName=exePath.AfterLast("\\");
    STRING projectName=exeName.BeforeLast(".");
    if (!projectName.Length()) projectName="MapEdit";

    STRING cfgPath=FCurrentDirectory()+"\\"+projectName+".cfg";
    PROFILE cfg(&cfgPath);
    if (const_cast<STRING*>(command_line)->HaveSubStr(".cfg")) {
        STRING commandCfg=FCurrentDirectory()+"\\"+*command_line;
        cfg.Load(&commandCfg);
    }

    const long comResult=CoInitialize(0);
    if (comResult<0) {
        char text[]="COM";
        MAP::Error(12,text,(unsigned long)comResult);
        return;
    }

    STRING stringsPath=FCurrentDirectory()+"\\Strings.ini";
    Profile=new PROFILE(&stringsPath);

    {
        STRING section("common"), key("Title"), def(projectName.CharPtr());
        STRING title=cfg.GetString(&section,&key,&def);
        m_title=title;
    }
    {
        STRING defaultPath=STRING("SOFTWARE\\Gromada\\")+projectName;
        STRING section("common"), key("RegPath");
        STRING regPath=cfg.GetString(&section,&key,&defaultPath);
        Registry=new REGISTRY(regPath);
    }

    int cfgVSync;
    { STRING section("graph"), key("VSync"); cfgVSync=cfg.GetInt(&section,&key,1); }
    if (cfgVSync) init->options|=2u;
    int cfgStartDialogIsFull;
    { STRING section("game"), key("StartDialogIsFull"); cfgStartDialogIsFull=cfg.GetInt(&section,&key,0); }
    if (cfgStartDialogIsFull) init->options|=4u;
    int cfgScreenX;
    { STRING section("graph"), key("DefaultScreenX"); cfgScreenX=cfg.GetInt(&section,&key,init->defaultScreenX); }
    int cfgScreenY;
    { STRING section("graph"), key("DefaultScreenY"); cfgScreenY=cfg.GetInt(&section,&key,init->defaultScreenY); }
    int cfgBpp;
    { STRING section("graph"), key("DefaultColorBPP"); cfgBpp=cfg.GetInt(&section,&key,init->defaultColorDepth); }
    const int retailDefaultFullScreen=init->defaultFullScreen;
    const int cfgFallbackDevice=(int)init->defaultDevice;
    { STRING key("Device"); init->defaultDevice=Registry->GetInt(key,cfgFallbackDevice); }
    { STRING key("ScreenX"); init->defaultScreenX=Registry->GetInt(key,cfgScreenX); }
    { STRING key("ScreenY"); init->defaultScreenY=Registry->GetInt(key,cfgScreenY); }
    { STRING key("BPP"); init->defaultColorDepth=Registry->GetInt(key,cfgBpp); }
    { STRING key("FullScreen"); init->defaultFullScreen=Registry->GetInt(key,retailDefaultFullScreen); }
    strcpy(init->gameName,m_title.CharPtr());

    Graph=new GRAPH(init);

    int startDialog;
    { STRING section("game"), key("StartDialog"); startDialog=cfg.GetInt(&section,&key,1); }
    if (startDialog &&
        !DialogBoxParamA(instance,"START_DIALOG",0,AppStart,0))
        return;

    if (!prev) {
        WNDCLASSA_OLD cls;
        cls.style=3;
        cls.lpfnWndProc=AppWndProc;
        cls.cbClsExtra=0;
        cls.cbWndExtra=0;
        cls.hInstance=instance;
        cls.hIcon=LoadIconA(instance,"AppIcon");
        cls.hCursor=0;
        cls.hbrBackground=GetStockObject(4);
        cls.lpszMenuName="AppMenu";
        cls.lpszClassName=projectName.CharPtr();
        if (!RegisterClassA(&cls)) return;
    }

    int windowX=0,windowY=0;
    if (!Graph->CapsFullScreen()) {
        { STRING key("WindowPositionX"); windowX=Registry->GetInt(key,0); }
        { STRING key("WindowPositionY"); windowY=Registry->GetInt(key,0); }
    }

    Map=this;
    const unsigned long style=Graph->CapsFullScreen()?0x80000000u:0x90CA0000u;
    m_hWnd=CreateWindowExA(0x00040000u,projectName.CharPtr(),m_title.CharPtr(),style,
        windowX,windowY,(int)Graph->SizeX(),(int)Graph->SizeY(),0,0,instance,0);
    ShowWindow(m_hWnd,sw);
    UpdateWindow(m_hWnd);
    m_hAccel=LoadAcceleratorsA(instance,"AppAccel");
    ShowCursor(0);

    if (Graph->Init(m_hWnd)) return;

    STRING font;
    { STRING section("graph"), key("Font"), def("Arial"); font=cfg.GetString(&section,&key,&def); }
    int fontSizeX;
    { STRING section("graph"), key("FontSizeX"); fontSizeX=cfg.GetInt(&section,&key,0); }
    int fontSizeY;
    { STRING section("graph"), key("FontSizeY"); fontSizeY=cfg.GetInt(&section,&key,8); }
    Graph->SetFont(&font,fontSizeX,fontSizeY);

    RESOURCE resource;
    { STRING section("game"), key("Resource"), def("objects.res"); m_resName=cfg.GetString(&section,&key,&def); }
    if (resource.OpenForRead(&m_resName,0x41544144u)) {
        char text[]="resource file";
        MAP::Error(7,text,0);
        return;
    }

    int soundHighQuality;
    { STRING key("SoundHighQuality"); soundHighQuality=Registry->GetInt(key,0); }
    Sound=new SOUND(m_hWnd,&resource,soundHighQuality);
    Const=new CONSTANT(&resource);
    { STRING section("game"), key("DebugMode"); Const->DebugMode=cfg.GetInt(&section,&key,0); }
    int cfgDrawFps;
    { STRING section("game"), key("DrawFPS"); cfgDrawFps=cfg.GetInt(&section,&key,0); }
    int cfgDrawPresentation;
    { STRING section("game"), key("DrawPresentation"); cfgDrawPresentation=cfg.GetInt(&section,&key,1); }
    m_flags=(m_flags&~0x00020000u)|((cfgDrawFps&1)?0x00020000u:0u);
    m_flags=(m_flags&~0x00040000u)|((cfgDrawPresentation&1)?0x00040000u:0u);

    LoadVid(&resource);
    ShowCursor(1);

    // Retail source line 186 is unconditional: EmptyVid is a required output
    // of the resource/VID startup chain, not an optional compatibility path.
    EmptyVid->m_weapon=(WEAPON*)m_weapon;
    Hash=new HASH_MAP(m_w,m_h,m_vids,m_noVid);
    resource.Close();
    SetCursor(0);

    Mouse=new MOUSE(EmptyVid,Graph->SizeX()*0.5f,Graph->SizeY()*0.5f,0.0f,ANGLE((unsigned char)0),0);
    Mouse->Enable();

    { STRING section("control"), key("Left"), def("%"); STRING value=cfg.GetString(&section,&key,&def); g_inputKeyLeftSecondary=(unsigned char)value.FirstChar(); }
    { STRING section("control"), key("Up"), def("&"); STRING value=cfg.GetString(&section,&key,&def); g_inputKeyUpSecondary=(unsigned char)value.FirstChar(); }
    { STRING section("control"), key("Right"), def("'"); STRING value=cfg.GetString(&section,&key,&def); g_inputKeyRightSecondary=(unsigned char)value.FirstChar(); }
    { STRING section("control"), key("Down"), def("\x04\x38"); STRING value=cfg.GetString(&section,&key,&def); g_inputKeyDownSecondary=(unsigned char)value.FirstChar(); }
    { STRING section("control"), key("Relative"); g_relativeControl=cfg.GetInt(&section,&key,0); }
    { STRING section("control"), key("First"), def("LBUTTON"); STRING value=cfg.GetString(&section,&key,&def); g_inputFirstPrimary=(int)INPUT::StringToKey(value); }
    { STRING section("control"), key("Second"), def("RBUTTON"); STRING value=cfg.GetString(&section,&key,&def); g_inputSecondPrimary=(int)INPUT::StringToKey(value); }
    { STRING section("control"), key("Prev"), def("["); STRING value=cfg.GetString(&section,&key,&def); g_inputPrevPrimary=(int)INPUT::StringToKey(value); }
    { STRING section("control"), key("Next"), def("]"); STRING value=cfg.GetString(&section,&key,&def); g_inputNextPrimary=(int)INPUT::StringToKey(value); }

    m_player[0]=new PLAYER_STEAM(1,0);
    m_player[2]=new PLAYER_STEAM(0,2);
    m_player[1]=new PLAYER_STEAM(2,1);
    m_player[3]=new PLAYER_STEAM(0,3);

    SetScrollBox(0.0f,0.0f,m_w,m_h);
    if (const_cast<STRING*>(command_line)->operator!=("") && !const_cast<STRING*>(command_line)->HaveSubStr(".cfg")) {
        m_startupLoad=command_line;
    } else {
        STRING section("game"), key("StartMap"), def("maps\\logo.map");
        STRING start=cfg.GetString(&section,&key,&def);
        m_startupLoad=start;
    }
    m_flags|=4u;
}
