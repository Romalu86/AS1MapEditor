#pragma once
// RLE_CODER owner. Included in ABI order by mapedit/runtime.hpp.
class RLE_CODER {
public:
    int Encode(const void* data,unsigned long size,FILE* file);
    int Decode(void* data,unsigned long size,FILE* file);
};

