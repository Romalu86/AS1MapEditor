#pragma once
// MAP owner and layout check. Included in ABI order by mapedit/runtime.hpp.
class MAP {
public:
    MAP(HINSTANCE__* instance,HINSTANCE__* prev,const STRING* command_line,int sw,GRAPH_INIT* init);
    virtual ~MAP();
    virtual int WorkWndMessage(HWND__*, unsigned long, unsigned long, unsigned long);
    virtual void DeletePointerToSprite(SPRITE*);
    virtual int Tact();
    virtual void DrawSecondaryInfo();
    virtual void Release();
    virtual void Load(STRING name);
    virtual void Save(STRING name);
    int ScriptRun(int n_func,const SPRITE* var1,const SPRITE* var2,int var3);
    STRING ScriptVariable(STRING name);
    void RemoveSpriteFromLayer(SPRITE* spr);
    void InsertSpriteToLayer(SPRITE* spr);
    virtual SPRITE* CreateSprite(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent);
    SPRITE* LoadSprite(STREAM* res,int version);
    SPRITE* ReadPointer(STREAM* res);
    SPRITE* Decode(SPRITE* old_sprite);
    SPRITE* OldLoadSprite(RESOURCE* res);
    void LoadVid(RESOURCE* res);
    void LoadWeapon(RESOURCE* res);
    VID* CreateVid(RESOURCE* res,int nvid);

    int      m_fps;             // +0x004
    int      m_fpsCnt;          // +0x008
    uint32_t m_flags;           // +0x00C; init-success is bit 2
    uint32_t m_unknown10;       // +0x010
    float    m_speed;           // +0x014
    STRING   m_title;           // +0x018
    STRING   m_mapName;         // +0x01C
    STRING   m_startupLoad;      // +0x020; exact semantic name pending; MAP_EDIT ctor feeds this to Load
    STRING   m_prevMap;         // +0x024
    STRING   m_resName;         // +0x028
    int      m_noTact;          // +0x02C
    uint32_t m_unknown30;       // +0x030
    float    m_w;               // +0x034
    float    m_h;               // +0x038
    uint32_t m_shiftFlag;       // +0x03C
    float m_shiftX1,m_shiftX2,m_shiftY1,m_shiftY2,m_shiftX,m_shiftY; // +0x040..0x054
    SPRITE_LIST m_layers[18];   // +0x058..0x177; exact original layer count
    LOGIC      m_logic;         // +0x178
    RESOURCE   m_resource;      // +0x1D0
    RELATION   m_relation;      // +0x210
    short*     m_groundz;       // +0x230
    short*     m_tempGroundz;   // +0x234
    int        m_groundW;       // +0x238
    int        m_groundH;       // +0x23C
    HINSTANCE__* m_instance;    // +0x240
    HWND__*      m_hWnd;        // +0x244
    HACCEL__*    m_hAccel;      // +0x248
    int          m_curArmy;     // +0x24C; current army/player index (layout-verified)
    PLAYER*      m_player[4];   // +0x250
    INPUT        m_input;       // +0x260
    MENU         m_menu;        // +0x280
    GROUPS       m_groups;      // +0x298
    int          m_noWeapon;    // +0x2BC
    void*        m_weapon;      // +0x2C0
    int          m_noVid;       // +0x2C4
    VID*         m_vids[2048];  // +0x2C8, original MAX_VID=2048
    MOUSETIPS    m_mousetips;   // +0x22C8

    int IsInitSuccess();
    int IsMapEdit();
    int OptLoad();
    int IsPaused();
    STRING GetMouseTipsString();
    int OptEnemyAttackNeutralTrains();
    int OptDrawHpLines(const SPRITE* spr);
    float SizeX(); float SizeY();
    int ValidateXY(float x,float y);
    int ValidateVid(int nvid);
    VID* ReadVid(STREAM* res);
    void WriteVid(STREAM* res,const VID* vid);
    int StartTact();
    int DemoTact();
    float ToScreenX(float x); float ToScreenY(float y); float ToScreenY(float y,float z);
    int ToScreenXInt(float x); int ToScreenYInt(float y);
    float FromScreenX(float screenX); float FromScreenY(float screenY);
    float GetGroundZ(float x,float y);
    float GetGroundZScr(float screenX,float screenY);
    float GetGroundZ(const VID* vid,float x,float y);
    void ResetGroundZ();
    void SetGroundZ(float x,float y,float newZ);
    void SetTempGroundZ(float x,float y,float newZ);
    void ClearTempGroundZ(float x,float y,float newZ);
    void DeleteExtraVid();
    void ExchangeVid(VID* vid1,VID* vid2);
    void CreateEmptyHardwareGround();
    void RestoreDeviceObjects();
    void InvalidateDeviceObjects();
    void NetworkTact();
    void SetScrollBox(float minX,float minY,float maxX,float maxY);
    void ChangeSizeXY(float newSizeX,float newSizeY);
    SPRITE* FirstSprite(int nlayer,int* i);
    SPRITE* NextSprite(int nlayer,int* i);
    SPRITE* FirstSpriteByType(int nlayer,int* i,unsigned int type);
    SPRITE* NextSpriteByType(int nlayer,int* i,unsigned int type);
    SPRITE* GetSprite(int type,float x,float y,SPRITE* prev);
    SPRITE* GetSpriteScr(int type,float screenX,float screenY);
    SPRITE* FindNearestSprite(int type,float x,float y,float radius,SPRITE* prev);
    void FindSpritesInsidePolygon(int type,const POLYGON* polygon,SPRITE_LIST* list);
    int VidToControlBox(DIALOG_COMBO_BOX* list,unsigned long unitTypeMask,int selectVid);
    int VidToListBox(DIALOG_LIST_BOX* list,unsigned long unitTypeMask,int selectVid,int sort);
    STRING OpenSaveDialog(int save,const char* filter);
    STRING OpenDialog(const char* filter);
    STRING SaveDialog(const char* filter);
    int NextVid(int old_vid,unsigned int typeunit);
    int PrevVid(int old_vid,unsigned int typeunit);
    void SetShiftCoor(float centerX,float centerY,int effect);
    void ControlShiftCoor();
    SPRITE* Flagman(int army);
    SPRITE* Flagman();
    SPRITE* SpriteUnderCursor();
    MENU* Menu();
    int GetFPS();
    int NPlayer();
    PLAYER* Player(int narmy);
    PLAYER* Player();
    int GetScrollType();
    float GetTimeCoeff();
    void SetSelectSpriteUnderCursor(int flag);
    int OptSelectSpriteUnderCursor();
    VID* Vid(int nvid);
    void Error(int type,char* text,unsigned long value);
    void SetScrollType(int type);
    void DrawLayer(int layer);
    int PopInt();
    const STRING* PopStr();
    SPRITE* PopSprite();
    VID* PopVid(const char* operation);
    void PushInt(int value);
    void PushStr(const STRING* value);
    void PushSprite(SPRITE* sprite);
    STRING FileName();
    void PauseOn();
    void PauseOff();
    void ReloadVid();
    void LoadInEndTact(const STRING* filename);
    void SetFlagman(int army,SPRITE* sprite);
    int ExecFunc(int command);
};
static_assert(sizeof(MAP)==0x22D0, "MapEdit MAP exact base span");

// Standard-layout mirror: raw ABI anchors avoid compiler-specific offsetof warnings
// from polymorphic helper classes while preserving every checked original offset.
struct MAP_LAYOUT_CHECK {
    uint8_t pad00[0x0C];
    uint32_t flags;                     // +0x0C
    uint8_t pad10[0x0C];
    uint32_t mapName;                   // +0x1C (STRING payload pointer)
    uint32_t startupLoad;               // +0x20 (STRING payload pointer)
    uint8_t pad24[0x08];
    int noTact;                         // +0x2C
    uint8_t pad30[0x28];
    uint8_t layers[0x120];              // +0x58, 18 * sizeof(SPRITE_LIST=0x10)
    uint8_t logic[0x58];                // +0x178
    uint8_t resource[0x40];             // +0x1D0
    uint8_t relation[0x20];             // +0x210
    uint32_t groundz;                   // +0x230
    uint8_t pad234[0x0C];
    uint32_t instance;                  // +0x240
    uint32_t hwnd;                      // +0x244
    uint32_t accel;                     // +0x248
    int currentArmy;                    // +0x24C
    uint32_t player[4];                 // +0x250
    uint8_t input[0x20];                // +0x260
    uint8_t menu[0x18];                 // +0x280
    uint8_t groups[0x24];               // +0x298
    uint8_t noWeaponAndWeapon[0x08];    // +0x2BC
    int noVid;                          // +0x2C4
    uint32_t vids[2048];                // +0x2C8
    uint8_t mousetips[0x08];            // +0x22C8
};
static_assert(offsetof(MAP_LAYOUT_CHECK,flags)==0x0C, "MAP flags offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,mapName)==0x1C, "MAP mapName offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,startupLoad)==0x20, "MAP +0x20 STRING offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,currentArmy)==0x24C, "MAP current-army offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,player)==0x250, "MAP player-array offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,noTact)==0x2C, "MAP noTact offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,layers)==0x58, "MAP sprite layers offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,logic)==0x178, "MAP LOGIC offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,resource)==0x1D0, "MAP RESOURCE offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,relation)==0x210, "MAP RELATION offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,groundz)==0x230, "MAP ground offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,instance)==0x240, "MAP HINSTANCE offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,hwnd)==0x244, "MAP HWND offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,player)==0x250, "MAP players offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,input)==0x260, "MAP INPUT offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,menu)==0x280, "MAP MENU offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,groups)==0x298, "MAP GROUPS offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,noVid)==0x2C4, "MAP noVid offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,vids)==0x2C8, "MAP vids offset");
static_assert(offsetof(MAP_LAYOUT_CHECK,mousetips)==0x22C8, "MAP mousetips offset");
static_assert(sizeof(MAP_LAYOUT_CHECK)==0x22D0, "MAP layout mirror size");


