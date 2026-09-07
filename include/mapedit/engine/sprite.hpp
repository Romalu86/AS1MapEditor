#pragma once
// SPRITE owner. Included in ABI order by mapedit/runtime.hpp.
class SPRITE {
public:
    SPRITE(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent);
    ~SPRITE();
    virtual void* ScalarDeletingDestructor(unsigned int flags); // vtable +0x00
    virtual int Action(int action,int a,int b,int c);            // +0x04; debug metadata exact
    virtual void Tact();                                        // +0x08; debug metadata exact
    virtual void MoveTact();                                    // +0x0C
    virtual void DeletePointerToSprite(SPRITE* sprite);          // +0x10
    virtual void Draw();                                        // +0x14
    virtual void DrawSecondaryInfo();                            // +0x18
    virtual void DrawGoalLine();                                 // +0x1C
    int m_unknown04;             // +0x04
    int m_begCadr;               // +0x08
    int m_noCadr;                // +0x0C
    int m_endCadr;               // +0x10
    unsigned int m_tactTime;     // +0x14
    unsigned int m_createTime;   // +0x18
    VID* m_vid;                  // +0x1C
    float m_speed;               // +0x20
    float m_unknown24;           // +0x24
    unsigned int m_flag;         // +0x28
    int m_noRef;                 // +0x2C
    float m_x;                   // +0x30
    float m_y;                   // +0x34
    float m_z;                   // +0x38
    SPRITE* m_goal;              // +0x3C
    SPRITE* m_child;             // +0x40
    SPRITE* m_parent;            // +0x44
    int m_ani;                   // +0x48
    uint8_t m_dir;               // +0x4C
    uint8_t m_pad4D[3];
    uint32_t m_unknown50;        // +0x50
    LIST<ACT> m_actions;         // +0x54..0x63; exact LIST<ACT> owner
    EX_SPRITE_DATA* m_exData;    // +0x64
    int m_hp;                   // +0x68; retail current HP
    PTR_SPRITE m_ptrSprite;      // +0x6C; exact 4-byte reference holder
    int NoRef();
    int IsSpriteClass(unsigned int cls) const;
    int IsSpriteType(unsigned int type) const;
    int IsLinked();
    int IsLinkDestroy();
    int IsInUndo();
    int IsDying() const;
    int IsInvisible();
    int IsInside(float vx,float vy);
    int IsZCross(const VID* vid,float z);
    int IsZCross(const SPRITE* sprite);
    int IsXYCross(const VID* vid,float x,float y);
    int IsXYCross(const SPRITE* sprite);
    int IsCross(const VID* vid,float x,float y,float z);
    int Command();
    int IsCommand(unsigned int command);
    VID* Vid() const;
    float X() const;
    float Y() const;
    float Z() const;
    ANGLE Direction() const;
    int RealDirection();
    unsigned long FrameSpeed();
    float Speed() const;
    float ZSpeed() const;
    float MaxZSpeed() const;
    float MaxSpeed() const;
    int IsMoveFinished();
    void ChangeRealDirection(unsigned int realDirection);
    void ChangeDirection(ANGLE direction);
    float GetGroundZ();
    void ChangeCoor(float new_x,float new_y,float new_z);
    void ChangeCoorForLinkRotate(float new_x,float new_y);
    void ChangeDirection1(ANGLE direction);
    void ChangeXCoor(float new_x);
    void ChangeYCoor(float new_y);
    void ChangeZCoor(float new_z);
    int Animation() const;
    int CurrentCadr();
    int LastCadr();
    int Army();
    int Hp();
    int MaxHp();
    int IsEnemy(const SPRITE* sprite);
    void AddRef();
    int Release();
    void Error(int type,const char* text,unsigned long err);
    SPRITE* Link();
    int HaveLink() const;
    int CanFight() const;
    int HaveFightLink() const;
    int HaveAction(int act);
    int CanAttackThisSprite(const SPRITE* sprite) const;
    SPRITE* SeekEnemy();
    int IsBetterEnemy(float size,float minsize,SPRITE* sprite,SPRITE* mins);
    int CanShotEnemy(SPRITE* sprite);
    int EnemyRating();
    int MaxAmmo();
    int GetFireDamage();
    unsigned int StateForward();
    int AttackTact(int deltaTime);
    int IsWeaponReload();
    int Attack(SPRITE* goal);
    int Attack(float x,float y,float z);
    int AskLine(float* endx,float* endy,float* endz);
    unsigned long AttackedSpriteType();
    EX_SPRITE_DATA* ExData();
    SPRITE* Goal();
    void SetGoal(SPRITE* new_goal);
    void SetBestTarget(SPRITE* goal);
    int SetCommand(int command,SPRITE* new_goal);
    int SetCommand(int command,float x,float y,float z);
    int SetCommandWithoutLink(int command,SPRITE* new_goal);
    void Remove();
    void Insert();
    int AddLink(SPRITE* additional);
    int AddLinkToLast(SPRITE* additional);
    void ChangeAnimation(int newAnimation);
    void ChangeArmy(int army);
    void ChangeHp(int newHp);
    int DestroyLink(const VID* destroyed);
    SPRITE* CanPlace(float x,float y,float z);
    SPRITE* CanPlaceWithCrush(float x,float y,float z);
    int AskCycleAction(int action,int var1,int var2,int var3);
    int AddCycleAction(int action,int var1,int var2,int var3);
    void AddAction(int action,int var1,int var2,int var3);
    void AddActionAfterStop(int action,int var1,int var2,int var3);
    STRING GetTextActions();
    STRING GetTextItems();
    void SetTextActions(const STRING* text);
    void SetTextItems(const STRING* text);
    int IsActionStackEmpty();
    LIST<ACT>* ActionStack();
    int HaveItem(int item);
    void InsertItem(int item);
    int InsertUniqueItem(int item);
    void ResetActionStack();
    void DrawRectangle();
    void DrawActionStack();
    void DrawShadow();
    GAMMA GetGamma();
    int HaveUniqueGamma();
    float ScreenX();
    float ScreenY();
    int ScreenXInt();
    int ScreenYInt();
    float NearDistanceTo(float x1,float y1);
    float NearDistanceTo(float x1,float y1,float z1);
    float NearDistanceTo(const SPRITE* sprite);
    float DistanceTo(const SPRITE* sprite);
    float BattleRange();
    int PercentHp();
    int GetItemNumber(int number);
    unsigned long GetTimer();
    int DeltaTime();
    void InvisibleOn();
    void InvisibleOff();
    void SetTimer(unsigned long timer);
    void SetTime(unsigned long time);
    void Move(float x,float y,float z);
    void Move(SPRITE* goal);
    ANGLE Rotate(ANGLE endDirection,int deltaTime);
    ANGLE RotateToGoal(int deltaTime);
    void AddActionImmediate(int action,int var1,int var2,int var3);
    void SetSpeed(float speed);
    int ActionStackHaveCommand(int command);
    void SetJustBuilded();
    void SetGamma(const GAMMA* gamma);
    void PrimitiveTact();
    void CreateLink();
    void BreakLink();
    void CreateChild();
    void PlaySFX(int nsfx);
    void CreateChildAndPlaySFX(int for_animation);
    void MoveTactCalcCoor(float* new_x,float* new_y,float* new_z);
    int MoveTactMapLimit(float new_x,float new_y);
    SPRITE* CanPlaceWithCrushAndGlide(float* new_x,float* new_y,float* new_z);
    ANGLE GlideDirection(ANGLE direction);
    int StartMove();
    void Stop();
    void Save(STREAM* res);
    ANGLE DirectionTo(float x,float y) const;
    ANGLE DirectionTo(float x,float y,int* radius) const;
    ANGLE DirectionTo(const SPRITE* sprite) const;
    ANGLE DirectionTo(const SPRITE* sprite,int* radius) const;
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(SPRITE,m_unknown04)==0x04, "SPRITE unknown04 ABI");
static_assert(offsetof(SPRITE,m_begCadr)==0x08, "SPRITE begin-frame ABI");
static_assert(offsetof(SPRITE,m_tactTime)==0x14, "SPRITE tact-time ABI");
static_assert(offsetof(SPRITE,m_vid)==0x1C, "SPRITE VID-pointer ABI");
static_assert(offsetof(SPRITE,m_flag)==0x28, "SPRITE flags ABI");
static_assert(offsetof(SPRITE,m_noRef)==0x2C, "SPRITE ref-count ABI");
static_assert(offsetof(SPRITE,m_x)==0x30, "SPRITE X ABI");
static_assert(offsetof(SPRITE,m_goal)==0x3C, "SPRITE goal ABI");
static_assert(offsetof(SPRITE,m_child)==0x40, "SPRITE child ABI");
static_assert(offsetof(SPRITE,m_parent)==0x44, "SPRITE parent ABI");
static_assert(offsetof(SPRITE,m_ani)==0x48, "SPRITE animation ABI");
static_assert(offsetof(SPRITE,m_dir)==0x4C, "SPRITE ANGLE storage ABI");
static_assert(offsetof(SPRITE,m_actions)==0x54, "SPRITE LIST<ACT> ABI");
static_assert(offsetof(SPRITE,m_exData)==0x64, "SPRITE EX_SPRITE_DATA ABI");
static_assert(offsetof(SPRITE,m_hp)==0x68, "SPRITE HP ABI");
static_assert(offsetof(SPRITE,m_ptrSprite)==0x6C, "SPRITE PTR_SPRITE ABI");
static_assert(sizeof(SPRITE)==0x70, "debug metadata SPRITE base size");
#ifdef __clang__
#pragma clang diagnostic pop
#endif

