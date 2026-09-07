#pragma once
// MENU/GROUP/GROUPS/MOUSETIPS/EX_SPRITE_DATA owners. Included in ABI order by mapedit/runtime.hpp.
class MENU : public SPRITE_LIST {
public:
    MENU();
    ~MENU();
    uint32_t clickFlags;          // +0x10; original lClick/rClick bitfield storage
    SPRITE* sprite;              // +0x14
    SPRITE* SpriteUnderCursor();
    int Control(INPUT* input);
    int NVidUnderCursor();
    int NDirUnderCursor();
    int IsLClick();
    int IsRClick();
    int Load(const STRING* name);
    int Save(const STRING* name);
    int DeleteFromFile(const STRING* name);
    void Error(TYPE_ERROR type,char* text,unsigned long err);
    int Delete(SPRITE* spr);
};
class GROUP : public SPRITE_LIST {
public:
    float x;                  // +0x10
    float y;                  // +0x14
    unsigned long behave;     // +0x18
    int nPlayer;              // +0x1C
    GROUP* nextGroup;         // +0x20
    GROUP(GROUP* prev,SPRITE* spr);
    virtual ~GROUP();
    void Draw();
    void DrawNumber(int number);
    void Insert(SPRITE* spr);
    GROUP* PrevGroup();
    float X() const;
    float Y() const;
    float DistanceTo(const SPRITE* spr);
};
static_assert(sizeof(GROUP)==0x24, "debug metadata GROUP size");
class GROUPS {
public:
    GROUPS();
    GROUP first;
    ~GROUPS();              // +0x00; intrusive sentinel, nextGroup at +0x20
    void DeleteAll();
    void DrawNumber();
    void InsertToNearGroup(SPRITE* spr);
    void ShiftFirstLeft();
    void ShiftFirstRight();
    GROUP* CreateNewGroup(SPRITE* spr);
    void DeletePointerToSprite(SPRITE* spr);
    GROUP* First();
    const GROUP* First() const;
    GROUP* Next(const GROUP* group);
    const GROUP* Next(const GROUP* group) const;
    void Save(RESOURCE* res);
    void Load(RESOURCE* res);
};
class MOUSETIPS {
public:
    MOUSETIPS();
    virtual ~MOUSETIPS();
    void* tip;
    void Tact(INPUT* input);
    void Clear();
    void DeletePointerToSprite(SPRITE* spr);
    int IsOut();
};
static_assert(sizeof(MENU)==0x18, "debug metadata MENU size");
static_assert(sizeof(GROUPS)==0x24, "debug metadata GROUPS size");
static_assert(sizeof(MOUSETIPS)==8, "debug metadata MOUSETIPS size");

class EX_SPRITE_DATA {
public:
    float lastX;
    float lastY;
    float lastZ;
    unsigned long changeCoorTime;
    unsigned long deathTimer;
    unsigned long birthSmokeTimer;
    unsigned long tableStartTime;
    float tableCoeff;
    float maxSpeed;
    GAMMA gamma;
    LIST<int> items;
    EX_SPRITE_DATA(const SPRITE* sprite);
};
static_assert(sizeof(EX_SPRITE_DATA)==0x3C, "debug metadata EX_SPRITE_DATA size");
struct EX_SPRITE_DATA_LAYOUT_CHECK {
    uint8_t pad000[0x24];
    GAMMA gamma;
    uint8_t items[0x10];
};
static_assert(offsetof(EX_SPRITE_DATA_LAYOUT_CHECK,gamma)==0x24, "EX_SPRITE_DATA gamma offset");
static_assert(offsetof(EX_SPRITE_DATA_LAYOUT_CHECK,items)==0x2C, "EX_SPRITE_DATA items offset");
static_assert(sizeof(EX_SPRITE_DATA_LAYOUT_CHECK)==0x3C, "EX_SPRITE_DATA layout mirror size");

struct VID_DOT { float x,y,z; };
static_assert(sizeof(VID_DOT)==0x0C, "VID link-dot ABI");

// MapEdit debug metadata/layout-visible prefix used by SPRITE::CreateChild.
// Only fields proven by direct retail accesses are exposed here; the rest of
// the historical WEAPON record remains intentionally opaque.
