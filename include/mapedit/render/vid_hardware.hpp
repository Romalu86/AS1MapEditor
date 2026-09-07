#pragma once
// VID_HARDWARE owner. Included in ABI order by mapedit/runtime.hpp.
class VID_HARDWARE : public VID {
public:
    VID_HARDWARE();
    explicit VID_HARDWARE(VID_HARDWARE* source);
    VID_HARDWARE(int nvid,int size_x,int size_y);
    virtual ~VID_HARDWARE();
    virtual VID* CreateMirror();
    virtual void DrawVidToVid(const SPRITE* sprite);
    virtual void Draw(const SPRITE* sprite);
    virtual void Load(RESOURCE* res);
    virtual void SetLayer();

    VID_TEXCOOR* m_texCoor; // +0x484
    short m_noSurf;         // +0x488
    TEXTURE** m_textures;   // +0x48C (compiler inserts +0x48A..0x48B padding)
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(VID_HARDWARE,m_texCoor)==0x484,"VID_HARDWARE tex-coor ABI");
static_assert(offsetof(VID_HARDWARE,m_noSurf)==0x488,"VID_HARDWARE surface-count ABI");
static_assert(offsetof(VID_HARDWARE,m_textures)==0x48C,"VID_HARDWARE texture-array ABI");
static_assert(sizeof(VID_HARDWARE)==0x490,"debug metadata VID_HARDWARE size");
#ifdef __clang__
#pragma clang diagnostic pop
#endif


// debug metadata-derived software/light VID family required by MAP::CreateVid.
// These declarations preserve the genuine MSVC inheritance/vtable shape;
// large raster bodies are integrated only when their reference behavior is closed.
