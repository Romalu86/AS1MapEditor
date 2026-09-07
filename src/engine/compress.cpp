#include "mapedit/runtime.hpp"

// The retail VC6 source uses fprintf(stderr, ...) for three codec diagnostics.
// VS2022 no longer exposes the VC6 _iob data symbol, so resolve stderr through
// the current CRT accessor while preserving the original text/conditions.
extern "C" FILE* __cdecl __acrt_iob_func(unsigned int);
extern "C" int __cdecl fprintf(FILE*, const char*, ...);

namespace {
FILE* RetailStderr()
{
    return __acrt_iob_func(2u);
}
}

FILTER::FILTER() {}

FILTER::~FILTER() {}

void FILTER::StartEncoding(FILE*) {}

int FILTER::StartDecoding(FILE*) { return 0; }

int FILTER::EndEncoding() { return 0; }

void FILTER::EndDecoding() {}

void FILTER::EncodeByte(int) {}

int FILTER::DecodeByte() { return -1; }

void FILTER::Reset() {}

// MapEdit engine-family qsmodel; debug metadata layout and MapEdit callers agree with
// the 257-symbol / 12-bit arithmetic model used by QS1_CODER.
QSMODEL::QSMODEL()
    : cumulative(0),frequency(0),lookup(0)
{
    Init(257,12,2000,0);
}

QSMODEL::~QSMODEL()
{
    if (cumulative) ::operator delete(cumulative);
    cumulative=0;
    if (frequency) ::operator delete(frequency);
    frequency=0;
    if (lookup) ::operator delete(lookup);
    lookup=0;
}

void QSMODEL::dorescale()
{
    if (nextLeft) {
        ++increment;
        left=nextLeft;
        nextLeft=0;
        return;
    }

    if (rescaleInterval<targetRescale) {
        rescaleInterval*=2;
        if (rescaleInterval>targetRescale)
            rescaleInterval=targetRescale;
    }

    int current=static_cast<int>(cumulative[noSym]);
    int missing=current;
    for (int i=noSym-1;i!=0;--i) {
        int value=static_cast<int>(frequency[i]);
        current-=value;
        cumulative[i]=static_cast<unsigned short>(current);
        value=(value|2)>>1;
        missing-=value;
        frequency[i]=static_cast<unsigned short>(value);
    }

    if (current!=static_cast<int>(frequency[0])) {
        // Retail compress.cpp: fprintf(stderr,
        // "BUG: rescaling left %d total frequency\n", cumulative); then exit(1).
        // The pointer-as-%d argument is intentional and follows the reference behavior.
        fprintf(RetailStderr(),"BUG: rescaling left %d total frequency\n",
                static_cast<int>(reinterpret_cast<unsigned long>(cumulative)));
        exit(1);
    }

    frequency[0]=static_cast<unsigned short>((static_cast<int>(frequency[0])>>1)|1);
    missing-=static_cast<int>(frequency[0]);
    increment=missing/rescaleInterval;
    nextLeft=missing%rescaleInterval;
    left=rescaleInterval-nextLeft;

    if (lookup) {
        int i=noSym;
        while (i) {
            const int end=(static_cast<int>(cumulative[i])-1)>>searchShift;
            --i;
            int start=static_cast<int>(cumulative[i])>>searchShift;
            while (start<=end) {
                lookup[start]=static_cast<unsigned short>(i);
                ++start;
            }
        }
    }
}

void QSMODEL::Init(int symbols,int shift,int rescale,int* initial)
{
    targetRescale=rescale;
    noSym=symbols;
    searchShift=shift-7;
    if (searchShift<0)
        searchShift=0;

    if (cumulative) ::operator delete(cumulative);
    cumulative=static_cast<unsigned short*>(::operator new(static_cast<unsigned int>(2*symbols+2)));
    if (frequency) ::operator delete(frequency);
    frequency=static_cast<unsigned short*>(::operator new(static_cast<unsigned int>(2*symbols+2)));
    if (lookup) ::operator delete(lookup);
    lookup=static_cast<unsigned short*>(::operator new(0x102u));

    cumulative[symbols]=static_cast<unsigned short>(1<<shift);
    cumulative[0]=0;
    if (lookup)
        lookup[128]=static_cast<unsigned short>(symbols-1);
    Reset(initial);
}

void QSMODEL::Reset(int* initial)
{
    rescaleInterval=(noSym>>4)|2;
    nextLeft=0;
    if (!initial) {
        const int initialValue=static_cast<int>(cumulative[noSym])/noSym;
        const int remainder=static_cast<int>(cumulative[noSym])%noSym;
        int i=0;
        for (;i<remainder;++i)
            frequency[i]=static_cast<unsigned short>(initialValue+1);
        for (;i<noSym;++i)
            frequency[i]=static_cast<unsigned short>(initialValue);
    } else {
        for (int i=0;i<noSym;++i)
            frequency[i]=static_cast<unsigned short>(initial[i]);
    }
    dorescale();
}

int QSMODEL::GetSym(int count)
{
    const unsigned short* probe=lookup+(count>>searchShift);
    int lo=static_cast<int>(*probe);
    int hi=static_cast<int>(*(probe+1))+1;
    while (lo+1<hi) {
        const int mid=(hi+lo)>>1;
        if (count<static_cast<int>(cumulative[mid]))
            hi=mid;
        else
            lo=mid;
    }
    return lo;
}

void QSMODEL::GetFreq(int sym,int* freq,int* cumulativeFreq)
{
    *cumulativeFreq=static_cast<int>(cumulative[sym]);
    *freq=static_cast<int>(cumulative[sym+1])-*cumulativeFreq;
}

void QSMODEL::Update(int sym)
{
    if (left<=0)
        dorescale();
    --left;
    frequency[sym]=static_cast<unsigned short>(static_cast<int>(frequency[sym])+increment);
}

void R_CODER::start_encoding(char firstByte,int initialCache,FILE* output)
{
    if (IsInit())
        return;
    file=output;
    low=0;
    range=0x80000000u;
    byte=static_cast<unsigned char>(firstByte);
    help=0;
    cache=initialCache;
}

R_CODER::R_CODER() : file(0) {}

int R_CODER::IsInit() const
{
    return file!=0;
}

void R_CODER::OutByte(int value)
{
    fputc(value,file);
}

int R_CODER::InByte()
{
    return fgetc(file);
}

void R_CODER::enc_normalize()
{
    while (range<=0x800000u) {
        if (static_cast<unsigned int>(low)<0x7F800000u) {
            OutByte(byte);
            while (help) {
                OutByte(0xFF);
                --help;
            }
            byte=static_cast<unsigned char>(static_cast<unsigned int>(low)>>23);
        } else if (static_cast<unsigned int>(low)&0x80000000u) {
            OutByte(static_cast<unsigned char>(byte+1u));
            while (help) {
                OutByte(0);
                --help;
            }
            byte=static_cast<unsigned char>(static_cast<unsigned int>(low)>>23);
        } else {
            ++help;
        }
        range<<=8;
        low=(low<<8)&0x7FFFFFFF;
        ++cache;
    }
}

void R_CODER::dec_normalize()
{
    while (range<=0x800000u) {
        low=(low<<8)|((static_cast<int>(byte)<<7)&0xFF);
        byte=static_cast<unsigned char>(InByte());
        low|=static_cast<int>(byte)>>1;
        range<<=8;
    }
}

void R_CODER::StartEncoding(FILE* output)
{
    start_encoding(0,0,output);
}

void R_CODER::EncodeFreq(unsigned int syFreq,unsigned int ltFreq,unsigned int totalFreq)
{
    enc_normalize();
    const unsigned int unit=range/totalFreq;
    const unsigned int delta=unit*ltFreq;
    low+=static_cast<int>(delta);
    if (ltFreq+syFreq<totalFreq)
        range=unit*syFreq;
    else
        range-=delta;
}

void R_CODER::EncodeShift(unsigned int syFreq,unsigned int ltFreq,unsigned int shift)
{
    enc_normalize();
    const unsigned int unit=range>>shift;
    const unsigned int delta=unit*ltFreq;
    low+=static_cast<int>(delta);
    if ((ltFreq+syFreq)>>shift)
        range-=delta;
    else
        range=unit*syFreq;
}

int R_CODER::EndEncoding()
{
    if (!IsInit())
        return -1;

    enc_normalize();
    cache+=5;

    unsigned int rounded;
    if ((static_cast<unsigned int>(low)&0x7FFFFFu) <
        ((static_cast<unsigned int>(cache)&0xFFFFFFu)>>1))
        rounded=static_cast<unsigned int>(low)>>23;
    else
        rounded=(static_cast<unsigned int>(low)>>23)+1u;

    if (rounded>0xFFu) {
        OutByte(static_cast<unsigned char>(byte+1u));
        while (help) {
            OutByte(0);
            --help;
        }
    } else {
        OutByte(byte);
        while (help) {
            OutByte(0xFF);
            --help;
        }
    }
    OutByte(static_cast<unsigned char>(rounded));
    OutByte((cache>>16)&0xFF);
    OutByte((cache>>8)&0xFF);
    OutByte(cache&0xFF);
    return cache;
}

int R_CODER::StartDecoding(FILE* input)
{
    if (IsInit())
        return 0;
    file=input;
    const int first=InByte();
    if (first==-1)
        return -1;
    byte=static_cast<unsigned char>(InByte());
    low=static_cast<int>(byte)>>1;
    range=0x80u;
    return first ? -1 : 0;
}

unsigned int R_CODER::DecodeCulFreq(unsigned int totalFreq)
{
    dec_normalize();
    help=static_cast<int>(range/totalFreq);
    unsigned int value=static_cast<unsigned int>(low)/static_cast<unsigned int>(help);
    if (value>=totalFreq)
        value=totalFreq-1;
    return value;
}

unsigned int R_CODER::DecodeCulShift(unsigned int shift)
{
    dec_normalize();
    help=static_cast<int>(range>>shift);
    const unsigned int value=static_cast<unsigned int>(low)/static_cast<unsigned int>(help);
    return (value>>shift) ? (1u<<shift)-1u : value;
}

void R_CODER::DecodeUpdate(unsigned int syFreq,unsigned int ltFreq,unsigned int totalFreq)
{
    const unsigned int delta=static_cast<unsigned int>(help)*ltFreq;
    low-=static_cast<int>(delta);
    if (ltFreq+syFreq<totalFreq)
        range=static_cast<unsigned int>(help)*syFreq;
    else
        range-=delta;
}

int R_CODER::DecodeByte()
{
    const unsigned int value=DecodeCulShift(8);
    DecodeUpdate(1,value,0x100);
    return static_cast<int>(value);
}

unsigned short R_CODER::DecodeWord()
{
    const unsigned int value=DecodeCulShift(16);
    DecodeUpdate(1,value,0x10000);
    return static_cast<unsigned short>(value);
}

void R_CODER::EndDecoding()
{
    dec_normalize();
}

QS0_CODER::QS0_CODER() : model(),coder() {}
QS0_CODER::~QS0_CODER() {}

// EQUIVALENT MODEL: QS0_CODER::Reset uses the 0x00447CF0 MTF reset semantics; it is not a separate retail owner.
void QS0_CODER::Reset()
{
    model.Reset(0);
}

void QS0_CODER::StartEncoding(FILE* output)
{
    coder.StartEncoding(output);
}

int QS0_CODER::StartDecoding(FILE* input)
{
    return coder.StartDecoding(input);
}

int QS0_CODER::EndEncoding()
{
    model.GetFreq(256,&syFreq,&ltFreq);
    coder.EncodeShift(static_cast<unsigned int>(syFreq),static_cast<unsigned int>(ltFreq),12);
    return coder.EndEncoding();
}

void QS0_CODER::EndDecoding()
{
    model.GetFreq(256,&syFreq,&ltFreq);
    coder.DecodeUpdate(static_cast<unsigned int>(syFreq),static_cast<unsigned int>(ltFreq),0x1000);
    coder.EndDecoding();
}

void QS0_CODER::EncodeByte(int value)
{
    model.GetFreq(value,&syFreq,&ltFreq);
    coder.EncodeShift(static_cast<unsigned int>(syFreq),static_cast<unsigned int>(ltFreq),12);
    model.Update(value);
}

int QS0_CODER::DecodeByte()
{
    const int cumulative=static_cast<int>(coder.DecodeCulShift(12));
    const int value=model.GetSym(cumulative);
    if (value==256)
        return -1;
    model.GetFreq(value,&syFreq,&ltFreq);
    coder.DecodeUpdate(static_cast<unsigned int>(syFreq),static_cast<unsigned int>(ltFreq),0x1000);
    model.Update(value);
    return value;
}

int QS0_CODER::Encode(const void* data,unsigned long size,FILE* output)
{
    const unsigned char* bytes=static_cast<const unsigned char*>(data);
    StartEncoding(output);
    for (unsigned long i=0;i<size;++i)
        EncodeByte(bytes[i]);
    return EndEncoding();
}

// No standalone retail QS0_CODER::Decode owner is emitted; retain the natural
// counterpart for the implemented API without pretending it is a VC6 owner.
int QS0_CODER::Decode(void* data,unsigned long size,FILE* input)
{
    unsigned char* bytes=static_cast<unsigned char*>(data);
    if (StartDecoding(input)<0)
        return 0;
    unsigned long done=0;
    while (done<size) {
        const int value=DecodeByte();
        if (value<0)
            break;
        bytes[done++]=static_cast<unsigned char>(value);
    }
    while (DecodeByte()>=0) {}
    EndDecoding();
    return static_cast<int>(done);
}

int RLE_CODER::Encode(const void* data,unsigned long size,FILE* output)
{
    const unsigned char* bytes=static_cast<const unsigned char*>(data);
    int last=0;
    QS0_CODER coder;
    coder.StartEncoding(output);
    unsigned long inCount=0;
    while (inCount<size) {
        int value=bytes[inCount++];
        coder.EncodeByte(value);
        if (value==last) {
            int count=0;
            while (count<0xFF && inCount<size) {
                value=bytes[inCount++];
                if (value==last)
                    ++count;
                else
                    break;
            }
            coder.EncodeByte(count);
            if (count!=0xFF && inCount<=size)
                coder.EncodeByte(value);
        }
        last=value;
    }
    return coder.EndEncoding();
}

int RLE_CODER::Decode(void* data,unsigned long size,FILE* input)
{
    unsigned char* bytes=static_cast<unsigned char*>(data);
    int last=0;
    QS0_CODER coder;
    coder.StartDecoding(input);
    unsigned long outCount=0;
    while (outCount<size) {
        const int value=coder.DecodeByte();
        if (value<0) {
            fprintf(RetailStderr(),"RLE::Can't read %i bytes \n\r",
                    static_cast<int>(size-outCount));
            coder.EndDecoding();
            return static_cast<int>(outCount);
        }
        bytes[outCount++]=static_cast<unsigned char>(value);
        if (value==last) {
            int count=coder.DecodeByte();
            while (count-- > 0)
                bytes[outCount++]=static_cast<unsigned char>(value);
        }
        last=value;
    }
    while (coder.DecodeByte()>=0) {}
    coder.EndDecoding();
    return static_cast<int>(outCount);
}

MTF_FILTER::MTF_FILTER()
{
    Reset();
}

void MTF_FILTER::Reset()
{
    for (int i=0;i<256;++i)
        order[i]=static_cast<unsigned char>(i);
}

int MTF_FILTER::Encode(int value)
{
    // Retail calls memchr(order, value, 0x100), subtracts order and then
    // memmoves exactly that many bytes before storing the input byte at [0].
    unsigned char* const found=static_cast<unsigned char*>(memchr(order,value,0x100u));
    const int index=static_cast<int>(found-order);
    memmove(order+1,order,static_cast<unsigned int>(index));
    order[0]=static_cast<unsigned char>(value);
    return index;
}

int MTF_FILTER::Decode(int index)
{
    const unsigned char value=order[index];
    if (index)
        memmove(order+1,order,static_cast<unsigned int>(index));
    order[0]=value;
    return static_cast<int>(value);
}

QS1_CODER::QS1_CODER(int mode) : FILTER()
{
    // Retail assigns this after FILTER and the implicit QSMODEL[256] construction.
    byteInWord=mode;
}
QS1_CODER::~QS1_CODER() {}

void QS1_CODER::Reset()
{
    for (int i=0;i<256;++i)
        model[i].Reset(0);
}

int QS1_CODER::Encode(const void* data,unsigned long size,FILE* output)
{
    const unsigned char* const buffer=static_cast<const unsigned char*>(data);
    int previous=0;
    const unsigned int half=(static_cast<unsigned int>(size)+1u)>>1;
    unsigned int i=0;
    R_CODER coder;
    if (!output || !size)
        return 0;
    if (static_cast<unsigned int>(size)<10u)
        return static_cast<int>(fwrite(buffer,1u,static_cast<unsigned int>(size),output));

    coder.StartEncoding(output);
    int symbolFreq=0;
    int cumulativeFreq=0;
    int interleaved=0;
    while (i<static_cast<unsigned int>(size)) {
        int symbol;
        if (byteInWord==2)
            symbol=(i<half) ? static_cast<int>(buffer[interleaved])
                            : static_cast<int>(buffer[interleaved-static_cast<int>(2u*half)+1]);
        else
            symbol=static_cast<int>(buffer[i]);
        QSMODEL& m=model[previous];
        m.GetFreq(symbol,&symbolFreq,&cumulativeFreq);
        coder.EncodeShift(static_cast<unsigned int>(symbolFreq),static_cast<unsigned int>(cumulativeFreq),12u);
        m.Update(symbol);
        ++i;
        interleaved+=2;
        previous=symbol;
    }
    model[previous].GetFreq(256,&symbolFreq,&cumulativeFreq);
    coder.EncodeShift(static_cast<unsigned int>(symbolFreq),static_cast<unsigned int>(cumulativeFreq),12u);
    return coder.EndEncoding();
}

int QS1_CODER::Decode(void* data,unsigned long size,FILE* input)
{
    int done=0;
    int symbolFreq=0;
    const unsigned int half=(static_cast<unsigned int>(size)+1u)>>1;
    int previous=0;
    R_CODER coder;
    if (!input || !size)
        return 0;
    if (static_cast<unsigned int>(size)<10u)
        return static_cast<int>(fread(data,1u,static_cast<unsigned int>(size),input));
    if (coder.StartDecoding(input)<0)
        return 0;

    int symbol;
    int cumulativeFreq;
    for (;;) {
        cumulativeFreq=static_cast<int>(coder.DecodeCulShift(12u));
        symbol=model[previous].GetSym(cumulativeFreq);
        if (symbol==256)
            break;
        if (static_cast<unsigned int>(done)>=static_cast<unsigned int>(size)) {
            fprintf(RetailStderr(),"!!!ERROR!!! decode");
            break;
        }
        if (byteInWord==2) {
            if (static_cast<unsigned int>(done)<half)
                static_cast<unsigned char*>(data)[2*done++]=static_cast<unsigned char>(symbol);
            else
                static_cast<unsigned char*>(data)[2*done++-static_cast<int>(2u*half)+1]=static_cast<unsigned char>(symbol);
        } else {
            static_cast<unsigned char*>(data)[done++]=static_cast<unsigned char>(symbol);
        }
        model[previous].GetFreq(symbol,&symbolFreq,&cumulativeFreq);
        coder.DecodeUpdate(static_cast<unsigned int>(symbolFreq),static_cast<unsigned int>(cumulativeFreq),0x1000u);
        model[previous].Update(symbol);
        previous=symbol;
    }
    model[previous].GetFreq(256,&symbolFreq,&cumulativeFreq);
    coder.DecodeUpdate(static_cast<unsigned int>(symbolFreq),static_cast<unsigned int>(cumulativeFreq),0x1000u);
    coder.EndDecoding();
    return done;
}
