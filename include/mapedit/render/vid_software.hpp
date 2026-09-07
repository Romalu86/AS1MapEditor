#pragma once
// VID_SOFTWARE/VID_SOFTWARE16/VID_HARDWARE_Z owners. Included in ABI order by mapedit/runtime.hpp.
class VID_SOFTWARE : public VID {
public:
    VID_SOFTWARE();
    explicit VID_SOFTWARE(VID_SOFTWARE* source);
    virtual ~VID_SOFTWARE();
    virtual VID* CreateMirror();
    virtual void Draw(const SPRITE* sprite);
    virtual void DrawShadow(const SPRITE* sprite);
    virtual void Load(RESOURCE* res);
    virtual void SetGamma(const GAMMA* gamma,unsigned int n_gamma);
    virtual int HaveShadow();
    virtual void SetLayer();
    virtual int PaletteSize();                 // appended slot +0x28
    virtual void SetGammaToPalette(unsigned char* palette,const GAMMA* gamma); // +0x2C

    int* m_cadrShift;          // +0x484
    int m_cadrSize;            // +0x488
    unsigned char* m_cadrs;    // +0x48C
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(VID_SOFTWARE,m_cadrShift)==0x484,"VID_SOFTWARE frame-offset ABI");
static_assert(offsetof(VID_SOFTWARE,m_cadrSize)==0x488,"VID_SOFTWARE data-size ABI");
static_assert(offsetof(VID_SOFTWARE,m_cadrs)==0x48C,"VID_SOFTWARE data ABI");
static_assert(sizeof(VID_SOFTWARE)==0x490,"debug metadata VID_SOFTWARE size");
#ifdef __clang__
#pragma clang diagnostic pop
#endif

class VID_SOFTWARE16 : public VID_SOFTWARE {
public:
    VID_SOFTWARE16();
    explicit VID_SOFTWARE16(VID_SOFTWARE16* source);
    virtual ~VID_SOFTWARE16();
    virtual VID* CreateMirror();
    virtual void Draw(const SPRITE* sprite);
    virtual void DrawToVid(const SPRITE* sprite,const VID_TEXCOOR* coor,TEXTURE* video_tex,TEXTURE* z_tex);
    virtual int PaletteSize();
    virtual void SetGammaToPalette(unsigned char* palette,const GAMMA* gamma);
};
static_assert(sizeof(VID_SOFTWARE16)==0x490,"debug metadata VID_SOFTWARE16 size");

class VID_HARDWARE_Z : public VID_SOFTWARE {
public:
    VID_HARDWARE_Z();
    explicit VID_HARDWARE_Z(VID_HARDWARE_Z* source);
    virtual ~VID_HARDWARE_Z();
    virtual VID* CreateMirror();
    virtual void Draw(const SPRITE* sprite);
    virtual void DrawToVid(const SPRITE* sprite,const VID_TEXCOOR* coor,TEXTURE* video_tex,TEXTURE* z_tex);
    virtual void SetGamma(const GAMMA* gamma,unsigned int n_gamma);
    virtual void SetLayer();
};
static_assert(sizeof(VID_HARDWARE_Z)==0x490,"debug metadata VID_HARDWARE_Z size");

// MapEdit debug metadata/original vtable 0x004B5324.  VID_FONT is deliberately
// tiny in this editor build: the runtime Load/Draw/SetLayer and device hooks
// are real retail no-ops, while the factory still needs its genuine class ABI.
