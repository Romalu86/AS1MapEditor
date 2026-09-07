#pragma once
// PLAYER owner. Included in ABI order by mapedit/runtime.hpp.
class PLAYER {
public:
    PLAYER(int type,int army);
    virtual ~PLAYER();                                      // vtable +0x00
    virtual void DeletePointerToSprite(SPRITE* sprite);     // +0x04
    virtual void Save(STREAM* res);                         // +0x08
    virtual void Load(STREAM* res);                         // +0x0C
    virtual void Release();                                 // +0x10
    virtual void SetFlagman(SPRITE* unit);                  // +0x14
    virtual void Control(INPUT* input);                     // +0x18
    virtual void StateBarOn();                              // +0x1C
    virtual void StateBarOff();                             // +0x20
    virtual void PutMessage(const STRING* text,float x,float y); // +0x24
    virtual void AddUnitToStateBar(SPRITE* unit);           // +0x28
    virtual STRING GetMouseTipsString();                    // +0x2C

    int m_money;                 // +0x04
    int m_type;                  // +0x08
    int m_army;                  // +0x0C
    PTR_SPRITE m_flagman;        // +0x10
    SPRITE_LIST m_stateBar;      // +0x14
    PTR_SPRITE m_underCursor;    // +0x24

    SPRITE* Flagman();
    SPRITE* SpriteUnderCursor();
    int IsStateBarOn();
    int GetMoney();
    void SetMoney(int newMoney);
    int AddMoney(int addMoney);
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(PLAYER,m_money)==0x04,"PLAYER money ABI");
static_assert(offsetof(PLAYER,m_type)==0x08,"PLAYER type ABI");
static_assert(offsetof(PLAYER,m_army)==0x0C,"PLAYER army ABI");
static_assert(offsetof(PLAYER,m_flagman)==0x10,"PLAYER flagman ABI");
static_assert(offsetof(PLAYER,m_stateBar)==0x14,"PLAYER statebar ABI");
static_assert(offsetof(PLAYER,m_underCursor)==0x24,"PLAYER under-cursor ABI");
#ifdef __clang__
#pragma clang diagnostic pop
#endif
static_assert(sizeof(PLAYER)==0x28,"retail PLAYER size");


// MapEdit MESSAGE owner used by PLAYER_STEAM.  The class vtable at 0x004B53CC
// has exactly two entries: scalar deleting destructor and DeletePointerToSprite.
