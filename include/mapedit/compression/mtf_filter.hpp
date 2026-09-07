#pragma once
// MTF_FILTER owner. Included in ABI order by mapedit/runtime.hpp.
class MTF_FILTER {
public:
    unsigned char order[256];

    MTF_FILTER();
    void Reset();
    int Encode(int value);
    int Decode(int index);
};
static_assert(sizeof(MTF_FILTER)==0x100,"debug metadata MTF_FILTER size");

