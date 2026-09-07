#pragma once
// QSMODEL owner. Included in ABI order by mapedit/runtime.hpp.
class QSMODEL {
public:
    int noSym;                    // +0x00
    int left;                     // +0x04
    int nextLeft;                 // +0x08
    int rescaleInterval;          // +0x0C
    int targetRescale;            // +0x10
    int increment;                // +0x14
    int searchShift;              // +0x18
    unsigned short* cumulative;   // +0x1C
    unsigned short* frequency;    // +0x20
    unsigned short* lookup;       // +0x24

    QSMODEL();
    ~QSMODEL();
    void dorescale();
    void Init(int symbols,int shift,int rescale,int* initial);
    void Reset(int* initial);
    int GetSym(int count);
    void GetFreq(int sym,int* freq,int* cumulativeFreq);
    void Update(int sym);
};
static_assert(sizeof(QSMODEL)==0x28,"debug metadata QSMODEL size");

