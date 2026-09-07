#pragma once
// PRIMITIVE/terrain/UNIT/DEPO owner family. Included in ABI order by mapedit/runtime.hpp.
class PRIMITIVE : public SPRITE {
public:
    PRIMITIVE(VID*,float,float,float,ANGLE,SPRITE*);
    ~PRIMITIVE();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual void Tact();
    virtual void MoveTact();
    virtual void DeletePointerToSprite(SPRITE*);
    using SPRITE::Draw;
    void Draw() const;  // const overload; retail vtable keeps SPRITE::Draw
    virtual void DrawSecondaryInfo();
    void Control(INPUT* input);
};
static_assert(sizeof(PRIMITIVE)==0x70, "debug metadata PRIMITIVE size");

class BUILDED_TERRAIN : public SPRITE {
public:
    BUILDED_TERRAIN(VID*,float,float,float,ANGLE,SPRITE*);
    ~BUILDED_TERRAIN();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual void Draw();
};
static_assert(sizeof(BUILDED_TERRAIN)==0x70, "debug metadata BUILDED_TERRAIN size");

class FRAME : public SPRITE {
public:
    FRAME(VID*,float,float,float,ANGLE,SPRITE*);
    ~FRAME();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
};
static_assert(sizeof(FRAME)==0x70, "debug metadata FRAME size");

class STEXT : public FRAME {
public:
    STRING text;
    STRING describe;
    int length;
    unsigned long behave;
    int noRow;
    int noColumn;
    STEXT(VID*,float,float,float,ANGLE,SPRITE*);
    ~STEXT();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
    virtual void Draw();
    void CalcTextProperty();
};
static_assert(sizeof(STEXT)==0x88, "debug metadata STEXT size");

class TERRAIN : public SPRITE {
public:
    int linkhp;
    int godemode;
    TERRAIN(VID*,float,float,float,ANGLE,SPRITE*);
    ~TERRAIN();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
    void SetGodeMode(int newGodemode);
    int GetGodeMode();
    int Repair(int repairChildFighter);
    void AddHpPerSecond(int hp_to_add);
    SPRITE* AskCell(float x,float y);
};
static_assert(sizeof(TERRAIN)==0x78, "debug metadata TERRAIN size");

class UNIT : public TERRAIN {
public:
    unsigned int stateLeftHandMove;
    unsigned int stateRightHandMove;
    int ammo;
    int number;
    int blocktick;
    unsigned long behave;
    UNIT(VID*,float,float,float,ANGLE,SPRITE*);
    ~UNIT();
    virtual void* ScalarDeletingDestructor(unsigned int flags);
    virtual int Action(int,int,int,int);
    virtual void MoveTact();
    virtual void Draw();
    virtual void DrawSecondaryInfo();
    void LineDraw();
    int Ammo();
    int GetNumber();
    void SetNumber(int num);
    int PercentAmmo();
    int AddAmmoTick(int tick);
    int GetActive();
    void InverseActive();
};
static_assert(sizeof(UNIT)==0x90, "debug metadata UNIT size");

class DEPO : public UNIT {
public:
    DEPO(VID*,float,float,float,ANGLE,SPRITE*);
    ~DEPO();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual int Action(int,int,int,int) override;
    virtual void MoveTact() override;

    int currentTrainIdentificator;       // +0x090
    unsigned short unitsToBuild[100];    // +0x094..+0x15B
    unsigned long timers[100];           // +0x15C..+0x2EB
    int pause[100];                      // +0x2EC..+0x47B
    int curunit;                         // +0x47C
    int unitlim;                         // +0x480
    int numunit;                         // +0x484
    int ActionBuildUnit(int var1,int var2);
    void AddUnitToQueue(int vidnum);
    int CanBuildUnit(int n);
    int GetQueueUnit(int n);
    int NoQueueUnit();
    int GetBuildTimePercent(int unitnum);
    int SlotForUnit(int nvid);
    void BreakQueue();
    void DeleteQueueUnit(int n);
    void BuildNextUnit();
    void ReplaceQueueUnit(int n,int new_vid);
    int PauseQueueUnit(int num);
    int IsPausedQueueUnit(int num);
    void ChangePlaces(int unit1,int unit2);
    void RightShiftQueueUnit(int n);
    void LeftShiftQueueUnit(int n);
};
static_assert(sizeof(DEPO)==0x488, "debug metadata DEPO size");

// debug metadata/editor-specific runtime classes required by MAP_EDIT::CreateSprite.
// These declarations restore the retail inheritance/layout/vtable contracts.
