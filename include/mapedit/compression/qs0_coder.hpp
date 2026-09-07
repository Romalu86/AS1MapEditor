#pragma once
// QS0_CODER owner. Included in ABI order by mapedit/runtime.hpp.
class QS0_CODER {
public:
    QSMODEL model;               // +0x00
    R_CODER coder;               // +0x28
    int syFreq;                  // +0x40
    int ltFreq;                  // +0x44

    QS0_CODER();
    ~QS0_CODER();
    void Reset();
    void StartEncoding(FILE* file);
    int StartDecoding(FILE* file);
    int EndEncoding();
    void EndDecoding();
    void EncodeByte(int value);
    int DecodeByte();
    int Encode(const void* data,unsigned long size,FILE* file);
    int Decode(void* data,unsigned long size,FILE* file);
};
static_assert(sizeof(QS0_CODER)==0x48,"debug metadata QS0_CODER size");

