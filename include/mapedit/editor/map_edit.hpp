#pragma once
// MAP_EDIT owner and tail layout. Included in ABI order by mapedit/runtime.hpp.
class MAP_EDIT : public MAP {
public:
    MAP_EDIT(HINSTANCE__* instance,HINSTANCE__* prev,const STRING* command_line,int sw,GRAPH_INIT* init);
    virtual ~MAP_EDIT();
    virtual int Tact() override;
    virtual int WorkWndMessage(HWND__*, unsigned long, unsigned long, unsigned long) override;
    virtual void DeletePointerToSprite(SPRITE*) override;
    virtual void DrawSecondaryInfo() override;
    virtual void Release() override;
    virtual void Load(STRING name) override;
    virtual SPRITE* CreateSprite(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent) override;

    void DrawMapName();
    void ChangeMouseVid(VID* nvid,unsigned int new_type);
    void ChangeMouseCoorWithSnap();
    void Control(INPUT* input);
    SPRITE* InsertUnit(VID* vid,float x,float y,float z,ANGLE direction);
    void DeleteUnit(float x,float y,int spriteType);
    void FillBox(float beginX,float beginY,float endX,float endY);
    void FillRomb(float beginX,float beginY,float endX,float endY);
    void LoadTerrain(STRING filename);
    void CreateNewRail();
    void SetControlPanel(int flag);
    int DialogControlPanel(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogSelectVid(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogMapProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogConvertSprite(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogUnitProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogTextProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogRegionProperty(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int DialogOptions(HWND__* hwnd,unsigned int msg,unsigned int wParam,long lParam);
    int Left();
    int Right();
    int IsLeftPossible();
    int IsRightPossible();
    int CallDialogBox(const STRING* name,DLGPROC_OLD f);

    union {
        uint32_t optionBits;
        struct {
            unsigned int optDrawMap:1;
            unsigned int optSortVid:1;
            unsigned int optDrawGroup:1;
            unsigned int optDelete:1;
            unsigned int optGround0:1;
            unsigned int optSnap:1;
            unsigned int optChessSnap:1;
            unsigned int optAirBrush:1;
            unsigned int optRandomDir:1;
            unsigned int optTacticMode:1;
            unsigned int optControlPanel:1;
            unsigned int optionUnused:21;
        };
    };                              // +0x22D0
    int optAirBrushSize;            // +0x22D4
    unsigned int optAirBrushDensity;// +0x22D8
    unsigned int optBigStepZ;       // +0x22DC
    int optShiftSnapX;              // +0x22E0
    int optShiftSnapY;              // +0x22E4
    SPRITE_LIST selectedSprites;    // +0x22E8
    int spriteType;                 // +0x22F8
    float insertZ;                  // +0x22FC
    UNDO undo;                      // +0x2300
    REGION* curRegion;              // +0x2354
    HWND__* hToolBar;               // +0x2358
    HWND__* hControlPanel;          // +0x235C
    STRING editFileName;            // +0x2360
    VID* savedVid;                  // +0x2364
};
static_assert(sizeof(MAP_EDIT)==0x2368, "MapEdit MAP_EDIT exact debug metadata size");
struct MAP_EDIT_TAIL_LAYOUT_CHECK {
    uint8_t base[0x22D0];
    uint32_t options;                   // +0x22D0
    int airSize;                        // +0x22D4
    unsigned int airDensity;            // +0x22D8
    unsigned int bigStepZ;              // +0x22DC
    int shiftSnapX;                     // +0x22E0
    int shiftSnapY;                     // +0x22E4
    uint8_t selected[0x10];             // +0x22E8
    int spriteType;                     // +0x22F8
    float insertZ;                      // +0x22FC
    uint8_t undo[0x54];                 // +0x2300
    uint32_t region;                    // +0x2354
    uint32_t toolbar;                   // +0x2358
    uint32_t controlPanel;              // +0x235C
    uint32_t filename;                  // +0x2360 (STRING payload pointer)
    uint32_t savedVid;                  // +0x2364
};
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,airSize)==0x22D4, "MAP_EDIT options layout");
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,selected)==0x22E8, "MAP_EDIT selectedSprites offset");
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,spriteType)==0x22F8, "MAP_EDIT spriteType offset");
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,undo)==0x2300, "MAP_EDIT undo offset");
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,region)==0x2354, "MAP_EDIT curRegion offset");
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,toolbar)==0x2358, "MAP_EDIT toolbar offset");
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,filename)==0x2360, "MAP_EDIT editFileName offset");
static_assert(offsetof(MAP_EDIT_TAIL_LAYOUT_CHECK,savedVid)==0x2364, "MAP_EDIT savedVid offset");
static_assert(sizeof(MAP_EDIT_TAIL_LAYOUT_CHECK)==0x2368, "MAP_EDIT layout mirror size");


