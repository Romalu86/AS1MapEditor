#pragma once
// VID_FONT owner. Included in ABI order by mapedit/runtime.hpp.
class VID_FONT : public VID {
public:
    VID_FONT();
    explicit VID_FONT(VID_FONT* source);
    virtual ~VID_FONT();
    virtual VID* CreateMirror();
    virtual void Draw(const SPRITE* sprite);
    virtual void Load(RESOURCE* res);
    virtual void SetLayer();
    void RestoreDeviceObjects();
    void InvalidateDeviceObjects();

    CD3DFont* m_font; // +0x484
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(VID_FONT,m_font)==0x484,"VID_FONT font-pointer ABI");
static_assert(sizeof(VID_FONT)==0x488,"debug metadata VID_FONT size");
#ifdef __clang__
#pragma clang diagnostic pop
#endif

// MapEdit debug metadata: PTR_SPRITE is a 4-byte reference-counted SPRITE holder.
