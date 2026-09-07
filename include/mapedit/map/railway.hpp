#pragma once
// R_DOT/R_POS/R_MAP railway owners. Included in ABI order by mapedit/runtime.hpp.
class R_DOT;
class R_POS;
struct R_DOT_LINK {
    R_DOT* dot;              // +0x00
    int distance;            // +0x04
    int backLink;            // +0x08
    R_DOT_LINK* crossLink;   // +0x0C
    ANGLE direction;         // +0x10
};
static_assert(sizeof(R_DOT_LINK)==0x14, "debug metadata R_DOT_LINK size");

class R_DOT {
public:
    R_DOT();
    ~R_DOT();

    int refCount;                 // +0x000
    int isBreakFlag;              // +0x004
    int nations;                  // +0x008
    int oneDirection;             // +0x00C
    int pushed;                   // +0x010
    int isMined;                  // +0x014
    int noLinks;                  // +0x018
    R_DOT_LINK links[6];          // +0x01C..0x093
    int step[6];                  // +0x094
    int realStep[6];              // +0x0AC
    int railLength[6];            // +0x0C4
    ENGINE* busyEngine;           // +0x0DC
    int x;                        // +0x0E0
    int y;                        // +0x0E4
    int z;                        // +0x0E8

    static R_DOT* FindedDot;
    static R_DOT* Goal;
    static SPRITE* Target;
    static int NoStep;
    static int NoStepForNotFound;
    static int MaxPathDots;
    static int train_length_in_rails;
    static unsigned char* FindedPath;
    static unsigned char CurrentPath[10000];
    static ENGINE* eng;
    static unsigned int command;
    static int weaponrange;
    static int repair_in_head;
    static int repair_in_tail;
    static unsigned int head_is_head;

    float ScreenX();
    float ScreenY();
    float SizeTo(float endx,float endy);
    int SizeTo(int endx,int endy);
    int DistanceTo(R_DOT* dot);
    int DistanceTo(int xx,int yy,int zz);
    float DistanceTo(float xx,float yy,float zz);
    void SetNearestPos(int xx,int yy,int zz,R_POS* pos);
    int GetDistance(int xx,int yy,int zz,int link);
    int GetPos(int xx,int yy,int zz,int link);
    int IsBreak();
    int IsNeedPush(int link);
    int IsOneDirect(int link);
    void UnBreak();
    void Break();
    void Link(R_DOT* dot);
    void Link(float x,float y,float z);
    void AddRef();
    void Release();
    void DebugDraw();
    void Error(int type,char* text,unsigned long err);
    int GetLink(R_DOT* dot);
    int GetLink(ANGLE direct);
    void UnLink(R_DOT* dot);
    int FindNewDot(int backLink,ANGLE direct);
    int FindNewDotWithoutBusyDots();
    void SetIfIsBetter(int len,int noStep,int unused,int* out);
    int CanEnginePassTo(int link,ENGINE* engine);
    int TryToFindRailsForReturn(int len,R_DOT* prev,R_DOT* avoid,ENGINE* engine,ANGLE direct);
};
static_assert(sizeof(R_DOT)==0xEC, "debug metadata R_DOT size");

// Original MapEdit railway.cpp debug metadata type.  The member names and offsets are
// retained exactly; pos_real/pos_fract are the fixed-point railway position.
class R_POS {
public:
    R_POS();
    R_POS(R_DOT* d,int p,int l);
    R_DOT* dot;        // +0x00
    int pos_real;      // +0x04
    int pos_fract;     // +0x08
    int link;          // +0x0C

    R_DOT* Dot2();
    R_DOT* Dot4();
    R_POS GetInversed();
    void Inverse();
    ANGLE Direct();
    int Length();
    int Link2();
    int DoStep(R_DOT* goal,SPRITE* target,ENGINE* eng);
    int NoStepToTarget(R_DOT* goal,SPRITE* target,unsigned int command,ENGINE* eng);
    int NoStepToTargetWithoutBusyDots(R_DOT* goal,SPRITE* target);
    void Write(STREAM* file);
    void Read(STREAM* file);
};
static_assert(sizeof(R_POS)==0x10, "debug metadata R_POS size");

class R_MAP {
public:
    R_MAP();
    ~R_MAP();
    int minx,miny,maxx,maxy;
    LIST<R_DOT*> Dots;
    void DebugDraw();
    R_DOT* CreateDot(float x,float y,float z);
    R_DOT* GetDot(int x,int y,int z);
    R_DOT* GetNearestDot(int x,int y);
    R_DOT* GetNearestDot2(int x,int y);
    R_DOT* GetNearestDot(int x,int y,int z);
    void ClearAllDots();
    void PrepareForFindDot(R_DOT* goal,SPRITE* target,unsigned int command,ENGINE* eng);
    void Error(int type,char* text,unsigned long err);
    void AddDotToArray(int x,int y,int width,int height,int dotIndex);
    void CreateAdditionalDots();
    void CreateIntersectedDot(R_DOT* a,R_DOT* b,R_DOT* c,R_DOT* d);
    void SetPushLine(int begin_x,int begin_y,int end_x,int end_y,int push_flag);
    void SetSemaphoreOrMine(int x,int y,int new_semaphore_or_mine,int nations);
};
static_assert(sizeof(R_MAP)==0x20, "debug metadata R_MAP size");

