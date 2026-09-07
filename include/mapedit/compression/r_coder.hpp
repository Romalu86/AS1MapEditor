#pragma once
// R_CODER owner. Included in ABI order by mapedit/runtime.hpp.
class R_CODER {
public:
    int low;                     // +0x00
    unsigned int range;          // +0x04
    int help;                    // +0x08
    unsigned char byte;          // +0x0C
    unsigned char pad0D[3];
    int cache;                   // +0x10
    FILE* file;                  // +0x14

    R_CODER();
    int IsInit() const;
    void OutByte(int value);
    int InByte();
    void enc_normalize();
    void dec_normalize();
    void start_encoding(char firstByte,int initialCache,FILE* output);
    void StartEncoding(FILE* output);
    void EncodeFreq(unsigned int syFreq,unsigned int ltFreq,unsigned int totalFreq);
    void EncodeShift(unsigned int syFreq,unsigned int ltFreq,unsigned int shift);
    int EndEncoding();
    int StartDecoding(FILE* input);
    unsigned int DecodeCulFreq(unsigned int totalFreq);
    unsigned int DecodeCulShift(unsigned int shift);
    void DecodeUpdate(unsigned int syFreq,unsigned int ltFreq,unsigned int totalFreq);
    int DecodeByte();
    unsigned short DecodeWord();
    void EndDecoding();
};
static_assert(sizeof(R_CODER)==0x18,"debug metadata R_CODER size");

