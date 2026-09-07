#pragma once
// VID_LIGHT owner. Included in ABI order by mapedit/runtime.hpp.
class VID_LIGHT : public VID {
public:
    VID_LIGHT();
    explicit VID_LIGHT(VID_LIGHT* source);
    virtual ~VID_LIGHT();
    virtual VID* CreateMirror();
    virtual void Draw(const SPRITE* sprite);
    virtual void Load(RESOURCE* res);
    virtual void SetLayer();

    int m_cadrSize;   // +0x484
    COLOR* m_cadrs;   // +0x488
};
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#endif
static_assert(offsetof(VID_LIGHT,m_cadrSize)==0x484,"VID_LIGHT cadr-size ABI");
static_assert(offsetof(VID_LIGHT,m_cadrs)==0x488,"VID_LIGHT cadr-data ABI");
static_assert(sizeof(VID_LIGHT)==0x48C,"debug metadata VID_LIGHT size");
#ifdef __clang__
#pragma clang diagnostic pop
#endif

