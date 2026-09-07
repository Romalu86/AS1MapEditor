#pragma once
// MESSAGE_STACK/MESSAGE owners. Included in ABI order by mapedit/runtime.hpp.
struct MESSAGE_STACK {
    STRING text;            // +0x00
    float eventX;           // +0x04
    float eventY;           // +0x08
    unsigned long time;     // +0x0C
    MESSAGE_STACK();
    MESSAGE_STACK(const STRING str,float x,float y);
    MESSAGE_STACK(const MESSAGE_STACK& that);
    ~MESSAGE_STACK();
    MESSAGE_STACK& operator=(const MESSAGE_STACK& that);
};
static_assert(sizeof(MESSAGE_STACK)==0x10,"debug metadata MESSAGE_STACK size");

class MESSAGE {
public:
    MESSAGE(int nvidFont,int nvidButton,float messageX,float messageY,
            int maxMessage,unsigned long delayMessageShift);
    virtual ~MESSAGE();
    virtual void DeletePointerToSprite(SPRITE* sprite);
    void Release();
    void Tact();
    void PutToStack(const STRING* text,float x,float y);
    void Put(const STRING* text,float x,float y);
    void Shift();

    int m_maxMessage;              // +0x04
    unsigned long m_delayShift;    // +0x08
    int m_nvidFont;                // +0x0C
    int m_nvidButton;              // +0x10
    float m_messageX;              // +0x14
    float m_messageY;              // +0x18
    enum { MAX_MESSAGE = 45 };
    int m_current;                 // +0x1C
    unsigned long m_lastShift;     // +0x20
    SPRITE* m_messageButton[MAX_MESSAGE]; // +0x024
    SPRITE* m_messageText[MAX_MESSAGE];   // +0x0D8
    float m_eventX[MAX_MESSAGE];           // +0x18C
    float m_eventY[MAX_MESSAGE];           // +0x240
    LIST<MESSAGE_STACK> m_stack;           // +0x2F4
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(MESSAGE,m_messageButton)==0x24,"MESSAGE button array ABI");
static_assert(offsetof(MESSAGE,m_messageText)==0xD8,"MESSAGE text array ABI");
static_assert(offsetof(MESSAGE,m_eventX)==0x18C,"MESSAGE event-X ABI");
static_assert(offsetof(MESSAGE,m_eventY)==0x240,"MESSAGE event-Y ABI");
static_assert(offsetof(MESSAGE,m_stack)==0x2F4,"MESSAGE stack ABI");
#ifdef __clang__
#pragma clang diagnostic pop
#endif
static_assert(sizeof(MESSAGE)==0x304,"retail MESSAGE size");

// MapEdit.exe allocates 0x380 bytes for PLAYER_STEAM.  The embedded arrays are
// therefore kill[4] and trains[10].  debug metadata's stale int[16]/UNIT*[40] extents
// contradict both the ctor loops and the allocator immediate and are not used.
