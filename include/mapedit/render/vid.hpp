#pragma once
// VID base owner. Included in ABI order by mapedit/runtime.hpp.
class VID {
public:
    VID();
    virtual VID* CreateMirror();                                      // vtable +0x00
    virtual ~VID();                                                   // +0x04 scalar deleting destructor in retail
    virtual void DrawVidToVid(const SPRITE* sprite);                  // +0x08
    virtual void Draw(const SPRITE* sprite);                          // +0x0C
    virtual void DrawShadow(const SPRITE* sprite);                    // +0x10
    virtual void DrawToVid(const SPRITE* sprite,const VID_TEXCOOR* coor,TEXTURE* video_tex,TEXTURE* z_tex); // +0x14
    virtual void Load(RESOURCE* res);                                 // +0x18
    virtual void SetGamma(const GAMMA* gamma,unsigned int n_gamma);   // +0x1C
    virtual int HaveShadow();                                         // +0x20
    virtual void SetLayer();                                          // +0x24

    int m_idx;                   // +0x004 (compiler vfptr occupies +0x000)
    STRING m_name;              // +0x008; genuine retail STRING member
    uint32_t m_unknown0C;        // +0x00C
    uint32_t m_spriteClass;      // +0x010
    uint32_t m_flag;             // +0x014
    int m_unknown18;             // +0x018
    float m_footprintWidth;      // +0x01C; exact MapEdit editor snap step X
    float m_footprintHeight;     // +0x020; exact MapEdit editor snap step Y
    float m_hitVerticalOffset;   // +0x024; vertical hit-test offset used by SPRITE/REGION
    int m_baseHp;                // +0x028; base/default HP used by script VID HP scaling
    float m_defaultMaxSpeed;     // +0x02C; EX_SPRITE_DATA ctor source
    float m_maxZSpeed;           // +0x030; vertical-speed limit / fixed vertical speed
    float m_acceleration;        // +0x034; 999999 sentinel means instant acceleration
    float m_deceleration;        // +0x038; 999999 sentinel means instant stop
    float m_childDirectionLock;  // +0x03C; zero allows linked child direction updates
    int m_weaponIndex;            // +0x040; WEAPON record index used by MAP::LoadVid
    uint8_t opaque044[0x48-0x44];
    int m_fireDamage;             // +0x048; base recursive fire-damage term (VID::GetFireDamage)
    float m_linkOffsetX;         // +0x04C; link rotation local X offset
    float m_linkOffsetY;         // +0x050; link rotation local Y offset
    float m_linkOffsetZ;         // +0x054; linked-child Z offset
    int m_linkVidIndex;           // +0x058; serialized linked VID index
    VID* m_linkVid;              // +0x05C; expected VID of an attached child sprite
    float m_groundOffset;        // +0x060; vertical cruise/ground offset
    float m_groundToleranceAbove; // +0x064; CanPlace max ground-z delta
    float m_groundToleranceBelow; // +0x068; CanPlace max z-ground delta
    uint32_t m_defaultDeathTimer;// +0x06C; EX_SPRITE_DATA ctor source

    unsigned int m_noDirections; // +0x070
    int m_noAnimCadr[17];         // +0x074..0x0B7; animation availability/frame data
    int m_aniSfx[17];             // +0x0B8..0x0FB; animation SFX indices
    int m_aniFrameSpeed[17];      // +0x0FC..0x13F; per-animation frame period
    float m_aniSpawnX[17];        // +0x140..0x183; child/fire local X amplitude
    float m_aniSpawnY[17];        // +0x184..0x1C7; child/fire local Y amplitude
    float m_aniSpawnZ[17];        // +0x1C8..0x20B; child/fire local Z offset
    int m_aniSpawnMode[17];       // +0x20C..0x24F; child/fire spawn mode
    VID* m_aniChildVid[17];       // +0x250..0x293; child VID per animation
    int m_aniFireCount[17];       // +0x294..0x2D7; child/fire count per animation
    GAMMA m_gamma;              // +0x2D8; genuine retail GAMMA member
    float m_colorScaleR;         // +0x2E0; serialized float, forced to 1 for HW+Z
    float m_colorScaleG;         // +0x2E4
    float m_colorScaleB;         // +0x2E8
    STRING m_resourceName;       // +0x2EC; genuine retail STRING member
    uint16_t m_extraTypeFlags;   // +0x2F0; bit 0x0200 is IsExtraType()
    uint16_t m_phaseRandomInterval; // +0x2F2; constructor tact-phase randomization interval
    short m_dotFrameCount;       // +0x2F4; frame count for link-dot ranges
    short m_regionTileStepX;     // +0x2F6; REGION tiled draw X step
    short m_regionTileStepY;     // +0x2F8; REGION tiled draw Y step
    uint8_t opaque2FA[0x2FC-0x2FA];
    int m_aniFrameStart[17];     // +0x2FC..0x33F; first frame for each animation
    int m_aniFrameLimit[17];     // +0x340..0x383; exclusive frame limit per animation
    float m_snapOffsetX;         // +0x384; exact editor snap X offset
    float m_snapOffsetY;         // +0x388; exact editor snap Y offset
    int m_layer;                 // +0x38C; sprite layer selected by VID queries
    int m_editorDirectionOffset; // +0x390; retail writes a full dword (low byte used by some callers)
    // Retail VID creation limits used by PLAYER_STEAM::CanCreateUnit.  debug metadata
    // did not preserve these member names, so neutral offset-derived names are
    // kept until a stronger original-name source is found.
    int m_limit394;              // +0x394; global per-VID creation limit
    int m_limit398[4];           // +0x398..+0x3A7; per-army creation limits
    int m_entitiesNumber[4];     // +0x3A8..0x3B7
    int m_killed[4];             // +0x3B8..0x3C7
    int m_reColored[4];          // +0x3C8..0x3D7
    int m_maxHp[4];              // +0x3D8..0x3E7
    GAMMA m_gammaByArmy[4];      // +0x3E8..0x407; vector-constructed in VID::VID
    uint8_t opaque408[0x45C-0x408];
    WEAPON* m_weapon;             // +0x45C; exact CreateChild weapon owner
    VID* m_mirrorNext;          // +0x460; circular mirror chain
    VID* m_exchangeVid;          // +0x464; exchange/remap VID, initialized to self and swapped by MAP::ExchangeVid
    int m_nLinkDots;             // +0x468
    VID_DOT* m_linkDots;         // +0x46C; 12-byte x/y/z triples
    int* m_dotFrameStarts;       // +0x470; optional per-frame begin offsets
    int m_moveTactData;          // +0x474; nonzero enables MoveTactCalcCoor path
    int m_exSpriteData;          // +0x478; nonzero allocates EX_SPRITE_DATA in SPRITE ctor
    uint32_t m_propertyBits;     // +0x47C; PropHide and related VID properties
    uint32_t m_prop;             // +0x480
    int PropHide();
    int PropAlwaysTop();
    int PropDblLight();
    void Error(int type,char* text,unsigned long err);
    void SetPropHide(int flag);
    int IsSpriteType(unsigned int type) const;
    int IsExtraType();
    int IsEmptyType();
    int IsTextureType();
    int IsAlphaType();
    int IsZBufferType();
    int IsPaletteType();
    int IsNewVersionType();
    int IsHardwareType();
    int IsLightType();
    int IsCompressType();
    int IsAltGammaType();
    void SetAltGammaType();
    int IsDXTType();
    int IsPseudo3DType();
    int IsFontType();
    void SetExtraType();
    int PropSkipMapEd();
    int PropHash();
    int PropGamma();
    int PropBlur();
    int PropGround();
    int PropWave();
    int PropHardwareDirect();
    int PropBuildVidZToGridZ();
    int PropBuildSizeToGridZ();
    int PropWind();
    void SetChildAndLink();
    void LoadParameters(RESOURCE* res);
    int PropInvisibleForEnemy();
    int PropRadialDamage();
    int PropNotDamageForFriend();
    int IsInvulnerable();
    int PropVertDir();
    int PropBirthAsSmoke() const;
    int PropTrack() const;
    int PropChildInEnd();
    int PropRandBirth() const;
    int PropZeroZ();
    int PropNoise();
    int PropOnePhase();
    int PropGravity() const;
    int PropGravity2() const;
    int PropSelfMoving() const;
    int PropRandSpeed();
    int PropRandZSpeed();
    int PropBounce();
    int PropMoveWithAnyDirection();
    int PropCrush();
    int CanFight() const;
    float CalculateZSpeed(float delta_z,float size) const;
    int NoSprites();
    int NoSprites(int army);
    int GetMaxHp(int army);
    void SetHpCoeff(int army,int hpPercent);
    void SetMaxHp(int army,int newHp);
    int Killed(int army);
    int Killed();
    int ReColored(int army);
    int ReColored();
    void IncreaseNoSprites(int army);
    void DecreaseNoSprites(int army);
    void ResetSprites();
    int RealDirection(ANGLE direction);
    ANGLE SteppedDirection(ANGLE direction);
    int PropNotCreateAsChild();
    void SetPropNotCreateAsChild(int on);
    int PropNotChangeLinkerCoor();
    int GetMaxAmmo();
    int GetFireDamage();
    int GetBuildTime();
    STRING GetNumberName();
    void SetGridZ(const SPRITE* sprite);
    void ResetGridZ(const SPRITE* sprite);
    static int BoxInViewPort(int left,int top,int right,int bottom);
    static void SetViewPort(int x0,int y0,int x1,int y1);
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(VID,m_name)==0x08, "VID name STRING ABI");
static_assert(offsetof(VID,m_gamma)==0x2D8, "VID gamma ABI");
static_assert(offsetof(VID,m_resourceName)==0x2EC, "VID resource-name STRING ABI");
static_assert(offsetof(VID,m_editorDirectionOffset)==0x390, "VID direction-offset dword ABI");
static_assert(sizeof(((VID*)0)->m_editorDirectionOffset)==4, "VID +0x390 must be dword");
static_assert(offsetof(VID,m_gammaByArmy)==0x3E8, "VID per-army gamma ABI");
static_assert(offsetof(VID,m_mirrorNext)==0x460, "VID mirror-chain ABI");
static_assert(offsetof(VID,m_baseHp)==0x28, "VID base-HP ABI");
static_assert(offsetof(VID,m_maxZSpeed)==0x30, "VID max-Z-speed ABI");
static_assert(offsetof(VID,m_acceleration)==0x34, "VID acceleration ABI");
static_assert(offsetof(VID,m_deceleration)==0x38, "VID deceleration ABI");
static_assert(offsetof(VID,m_childDirectionLock)==0x3C, "VID linked-child direction-lock ABI");
static_assert(offsetof(VID,m_groundOffset)==0x60, "VID ground-offset ABI");
static_assert(offsetof(VID,m_linkOffsetX)==0x4C, "VID link-offset-X ABI");
static_assert(offsetof(VID,m_linkOffsetY)==0x50, "VID link-offset-Y ABI");
static_assert(offsetof(VID,m_groundToleranceAbove)==0x64, "VID CanPlace upper terrain tolerance ABI");
static_assert(offsetof(VID,m_groundToleranceBelow)==0x68, "VID CanPlace lower terrain tolerance ABI");
static_assert(offsetof(VID,m_noAnimCadr)==0x074, "VID no-animation-cadr ABI");
static_assert(offsetof(VID,m_aniSfx)==0x0B8, "VID animation SFX ABI");
static_assert(offsetof(VID,m_aniFrameSpeed)==0x0FC, "VID animation frame-speed ABI");
static_assert(offsetof(VID,m_aniSpawnX)==0x140, "VID animation spawn-X ABI");
static_assert(offsetof(VID,m_aniSpawnY)==0x184, "VID animation spawn-Y ABI");
static_assert(offsetof(VID,m_aniSpawnZ)==0x1C8, "VID animation spawn-Z ABI");
static_assert(offsetof(VID,m_aniSpawnMode)==0x20C, "VID animation spawn-mode ABI");
static_assert(offsetof(VID,m_aniChildVid)==0x250, "VID animation child VID ABI");
static_assert(offsetof(VID,m_aniFireCount)==0x294, "VID animation fire-count ABI");
static_assert(offsetof(VID,m_phaseRandomInterval)==0x2F2, "VID tact phase interval ABI");
static_assert(offsetof(VID,m_aniFrameStart)==0x2FC, "VID animation frame-start ABI");
static_assert(offsetof(VID,m_aniFrameLimit)==0x340, "VID animation frame-limit ABI");
static_assert(offsetof(VID,m_killed)==0x3B8, "VID killed counters ABI");
static_assert(offsetof(VID,m_reColored)==0x3C8, "VID recolor counters ABI");
static_assert(offsetof(VID,m_maxHp)==0x3D8, "VID max-HP counters ABI");
static_assert(offsetof(VID,m_weapon)==0x45C, "VID weapon-pointer ABI");
static_assert(offsetof(VID,m_exchangeVid)==0x464, "VID exchange-remap pointer ABI");
static_assert(offsetof(VID,m_dotFrameCount)==0x2F4, "VID dot-frame-count ABI");
static_assert(offsetof(VID,m_regionTileStepX)==0x2F6, "VID REGION tile-X ABI");
static_assert(offsetof(VID,m_regionTileStepY)==0x2F8, "VID REGION tile-Y ABI");
static_assert(offsetof(VID,m_nLinkDots)==0x468, "VID link-dot-count ABI");
static_assert(offsetof(VID,m_linkDots)==0x46C, "VID link-dot-array ABI");
static_assert(offsetof(VID,m_dotFrameStarts)==0x470, "VID link-dot-frame-table ABI");
static_assert(offsetof(VID,m_moveTactData)==0x474, "VID move-tact data ABI");
static_assert(offsetof(VID,m_exSpriteData)==0x478, "VID EX_SPRITE_DATA gate ABI");
static_assert(sizeof(VID)==1156, "debug metadata VID size");
#ifdef __clang__
#pragma clang diagnostic pop
#endif

// MapEdit debug metadata / original VID_HARDWARE layout.  This is a genuine
// derived polymorphic class; MSVC reuses VID's vfptr at +0x000.
