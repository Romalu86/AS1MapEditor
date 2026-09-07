#pragma once
// FILTER owner. Included in ABI order by mapedit/runtime.hpp.
// Original qsmodel/filter compressor family used by RESOURCE::WritePacked and
// PICTURE_MAKEVID.  Layouts are taken from MapEdit debug metadata; behavior is
// derived directly from the MapEdit owners/vtable layout.  The virtual
// Encode/Decode entries are the packed-resource data path.
class FILTER {
public:
    FILTER();
    virtual ~FILTER();
    virtual void StartEncoding(FILE* file);
    virtual int StartDecoding(FILE* file);
    virtual int EndEncoding();
    virtual void EndDecoding();
    virtual void EncodeByte(int value);
    virtual int DecodeByte();
    virtual void Reset();
    virtual int Encode(const void* data,unsigned long size,FILE* file)=0;
    virtual int Decode(void* data,unsigned long size,FILE* file)=0;
};
static_assert(sizeof(FILTER)==4,"debug metadata FILTER size");

