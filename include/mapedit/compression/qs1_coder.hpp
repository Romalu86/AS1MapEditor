#pragma once
// QS1_CODER owner. Included in ABI order by mapedit/runtime.hpp.
class QS1_CODER : public FILTER {
public:
    int byteInWord;              // +0x04
    QSMODEL model[256];          // +0x08

    explicit QS1_CODER(int mode);
    virtual ~QS1_CODER();
    virtual void Reset();
    virtual int Encode(const void* data,unsigned long size,FILE* file);
    virtual int Decode(void* data,unsigned long size,FILE* file);
};
static_assert(sizeof(QS1_CODER)==0x2808,"debug metadata QS1_CODER size");

