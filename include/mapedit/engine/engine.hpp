#pragma once
// ENGINE owner. Included in ABI order by mapedit/runtime.hpp.
// Original MapEdit ENGINE debug metadata layout (engine.h).  ENGINE derives from UNIT;
// the fields below start exactly at UNIT+0x90 and extend to 0xAC0.
class ENGINE : public UNIT {
public:
    ENGINE(VID*,float,float,float,ANGLE,SPRITE*);
    ~ENGINE();
    virtual void* ScalarDeletingDestructor(unsigned int flags) override;
    virtual int Action(int,int,int,int) override;
    virtual void MoveTact() override;
    virtual void DeletePointerToSprite(SPRITE*) override;
    virtual void DrawSecondaryInfo() override;
    virtual void DrawGoalLine() override;

    unsigned int stateInvMove:1;       // +0x090 bit 0
    unsigned int stateInvMoveUnused:31;
    ENGINE* prev_engine;               // +0x094
    ENGINE* next_engine;               // +0x098
    ENGINE* commandEngine;             // +0x09C
    R_DOT* goal;                       // +0x0A0
    R_DOT* patrolto1;                  // +0x0A4
    R_DOT* patrolto2;                  // +0x0A8
    int acceleration;                  // +0x0AC
    float maxSpeed;                    // +0x0B0
    int isPushed;                      // +0x0B4
    R_POS head;                        // +0x0B8
    R_POS tail;                        // +0x0C8
    int is_busy;                       // +0x0D8
    int lastunitintrain;               // +0x0DC
    float lastx;                       // +0x0E0
    float lasty;                       // +0x0E4
    float lastz;                       // +0x0E8
    int was_near_mine;                 // +0x0EC
    unsigned int minecreatetime;       // +0x0F0
    int depo_train_num;                // +0x0F4
    unsigned char findedway[2500];     // +0x0F8
    int noFindedPath;                  // +0xABC

    static int globaldeleting;
    static int NoStepForNotFound;
    static int NoStep;
    static SPRITE_LIST PathDots;
    static void ReleasePathDots();
    ENGINE* FirstEngine();
    ENGINE* LastEngine();
    ENGINE* NextEngine();
    ENGINE* GetTrain();
    ENGINE* GetChainEngine(int n);
    ENGINE* GetRepair();
    ENGINE* PrevEngine();
    R_DOT* GetGoal();
    SPRITE* GetMoveTarget();
    int IsFirst();
    int IsLast();
    int IsSingle();
    int IsSelfMoving();
    int IsPowerEngine();
    int CanBeLinkedByEnemy();
    int InTrain(const SPRITE* engine);
    int HaveArmy(int army);
    int TrainWeaponRange();
    int IsCommandToAllTrain();
    void SetCommandToTrain(int command,int x,int y);
    void SetCommandToTrain(int command,SPRITE* target,R_DOT* dot_target,R_DOT* dot_target2);
    void InverseTrainActive();
    void MoveToRepair();
    void Attack(SPRITE* goal);
    void SubMove(SPRITE* goal);
    void Move(float xx,float yy,float zz,int frompatrol,int fromclick);
    void ReverseTrain();
    void ReCalcMoveParameters();
    int NeedAddAmmo();
    int NeedRepairByRepair();
    int RepairByRepair(ENGINE* eng_to_repair);
    void AddAmmoTact();
    void RepairTact();
    void SetRDot();
    void CalcCoor();
    void MoveEngineTact();
    void PullTail(const R_POS* old_head);
    void ClearDotBusy();
    void SetDotBusy();
    void MT_SpeedProcessing(float* abs_speed);
    int MT_IntersectingProcessing(const R_POS* old_head,float* abs_speed);
    void MT_PullCoordinates(R_POS* old_head,float abs_speed,int acceleration_);
    int IsTouch(ENGINE* eng,int frommovetact);
    int IsBadTouch(ENGINE* eng);
    ENGINE* GetIntersecting();
    ENGINE* GetBadIntersecting();
    ENGINE* GetForwardIntersecting(int* typeofintersecting);
    float Clash(ENGINE* eng,int typeofintersecting);
    int ForceLink(ENGINE* engine);
    void ForceStop();
    void ActNextCommandFighter();
    int CanLinkWithEngine(ENGINE* eng);
    int ReachTheTarget();
    void DeleteAttackToEngine();
    void SetCommandSameOther();
    int NeedRepairByMaster();
    void BreakTrain(float x,float y);
    void BreakTrain(ENGINE* eng);
    void CreatePathDots(R_DOT* dot);
    void CreatePathDots(R_DOT* dot,SPRITE_LIST* pathdots,int nvid);
    int IsTailInFindedPath(R_DOT* dot);
    int GetTrainLengthInRails();
    void SetAbsSpeed(float new_speed);
    void Stop();
    void CheckPrevNextEngine();
    void DebugDraw();
    void Error(int type,char* text,unsigned long err);
};
static_assert(sizeof(ENGINE)==0xAC0, "debug metadata ENGINE size");

