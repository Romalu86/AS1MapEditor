#pragma once
// PTR_SPRITE owner. Included in ABI order by mapedit/runtime.hpp.
class PTR_SPRITE {
public:
    SPRITE* sprite;
    PTR_SPRITE();
    ~PTR_SPRITE();
    PTR_SPRITE& operator=(SPRITE* spr);
    operator SPRITE*();
    SPRITE* operator->();
};
static_assert(sizeof(PTR_SPRITE)==4, "debug metadata PTR_SPRITE size");

