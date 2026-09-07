#include "mapedit/runtime.hpp"

extern "C" unsigned long __stdcall mciSendStringA(const char* command,char* returnString,unsigned int returnLength,HWND__* callback);
extern "C" unsigned int __stdcall waveOutSetVolume(unsigned int deviceId,unsigned long volume);
extern "C" long __stdcall DirectSoundCreate8(const void* deviceGuid,void** directSound,void* outerUnknown);

namespace {
struct SoundAbiFields {
    uint32_t caps;                     // +0x000
    int noSfx;                         // +0x004
    SFX* sfx;                          // +0x008
    uint8_t mix[0x200];                // +0x00C..+0x20B, 32 x 0x10 MIX records
    HWND__* callbackWindow;            // +0x20C
    void* directSound;                 // +0x210 IDirectSound8*
    SFXBUFFER buffers[16];             // +0x214..0x3D3
    int curTrack;                      // +0x3D4
    int track1;                        // +0x3D8
    int track2;                        // +0x3DC
    int track3;                        // +0x3E0
    int track4;                        // +0x3E4
    int soundDisabled;                 // +0x3E8, zero while DirectSound is active
    int auxAudio;                      // +0x3EC
    int auxWave;                       // +0x3F0
    unsigned long oldAudio;            // +0x3F4
    unsigned long oldWave;             // +0x3F8
    unsigned long audioVolume;         // +0x3FC
    unsigned long waveVolume;          // +0x400
    int soundVolume;                   // +0x404
    int musicVolume;                   // +0x408
    int loopMusic;                     // +0x40C
    void* music;                       // +0x410 MUSIC*
    STRING nextMusicFile;              // +0x414
    int fadeMusicDuration;             // +0x418
    int fadeMusicTime;                 // +0x41C
};
// SoundAbiFields contains polymorphic SFXBUFFER objects, so standard offsetof is
// ill-formed under strict /W3 /WX.  Validate the exact debug metadata offsets with a
// POD mirror while retaining the typed overlay used by the implementation.
struct SoundAbiLayoutCheck {
    uint32_t caps;
    int noSfx;
    void* sfx;
    uint8_t mix[0x200];
    void* callbackWindow;
    void* directSound;
    uint8_t buffers[16*0x1C];
    int curTrack;
    int track1;
    int track2;
    int track3;
    int track4;
    int soundDisabled;
    int auxAudio;
    int auxWave;
    unsigned long oldAudio;
    unsigned long oldWave;
    unsigned long audioVolume;
    unsigned long waveVolume;
    int soundVolume;
    int musicVolume;
    int loopMusic;
    void* music;
    uint32_t nextMusicFile;
    int fadeMusicDuration;
    int fadeMusicTime;
};
static_assert(offsetof(SoundAbiLayoutCheck,callbackWindow)==0x20C,"SOUND callback HWND ABI");
static_assert(offsetof(SoundAbiLayoutCheck,directSound)==0x210,"SOUND DirectSound ABI");
static_assert(offsetof(SoundAbiLayoutCheck,buffers)==0x214,"SOUND SFXBUFFER array ABI");
static_assert(offsetof(SoundAbiLayoutCheck,soundDisabled)==0x3E8,"SOUND disabled flag ABI");
static_assert(offsetof(SoundAbiLayoutCheck,auxAudio)==0x3EC,"SOUND aux audio ABI");
static_assert(offsetof(SoundAbiLayoutCheck,auxWave)==0x3F0,"SOUND aux wave ABI");
static_assert(offsetof(SoundAbiLayoutCheck,music)==0x410,"SOUND music ABI");
static_assert(offsetof(SoundAbiLayoutCheck,nextMusicFile)==0x414,"SOUND next music filename ABI");
static_assert(sizeof(SoundAbiLayoutCheck)==0x420,"debug metadata SOUND layout size");
static_assert(sizeof(SoundAbiFields)==0x420,"debug metadata SOUND typed overlay size");

SoundAbiFields& Fields(SOUND* sound)
{
    return *reinterpret_cast<SoundAbiFields*>(sound);
}

void CallStreamControl(void* object,unsigned int byteOffset)
{
    if (!object)
        return;
    void** vtable=*reinterpret_cast<void***>(object);
    typedef long (__thiscall *Method)(void*);
    reinterpret_cast<Method>(vtable[byteOffset/4])(object);
}


void DeleteVirtualSoundObject(void* object)
{
    if (!object)
        return;
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void* (__thiscall *DeletingDestructor)(void*,unsigned int);
    reinterpret_cast<DeletingDestructor>(vtable[0])(object,1u);
}

unsigned long ReleaseSoundCom(void* object)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef unsigned long (__stdcall *ReleaseMethod)(void*);
    return reinterpret_cast<ReleaseMethod>(vtable[0x08/4])(object);
}

unsigned long AddRefSoundCom(void* object)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef unsigned long (__stdcall *AddRefMethod)(void*);
    return reinterpret_cast<AddRefMethod>(vtable[0x04/4])(object);
}

long DuplicateSoundBuffer(void* directSound,void* source,void** destination)
{
    void** const vtable=*reinterpret_cast<void***>(directSound);
    typedef long (__stdcall *DuplicateMethod)(void*,void*,void**);
    return reinterpret_cast<DuplicateMethod>(vtable[0x14/4])(directSound,source,destination);
}

long SetSoundBufferPosition(void* object,unsigned long position)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *SetPositionMethod)(void*,unsigned long);
    return reinterpret_cast<SetPositionMethod>(vtable[0x34/4])(object,position);
}

long SetSoundBufferVolume(void* object,long volume)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *SetVolumeMethod)(void*,long);
    return reinterpret_cast<SetVolumeMethod>(vtable[0x3C/4])(object,volume);
}

long SetSoundBufferPan(void* object,long pan)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *SetPanMethod)(void*,long);
    return reinterpret_cast<SetPanMethod>(vtable[0x40/4])(object,pan);
}

long PlaySoundBuffer(void* object,unsigned long reserved1,unsigned long priority,unsigned long flags)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *PlayMethod)(void*,unsigned long,unsigned long,unsigned long);
    return reinterpret_cast<PlayMethod>(vtable[0x30/4])(object,reserved1,priority,flags);
}

void CallMusicVoid(void* object,unsigned int byteOffset)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void (__thiscall *Method)(void*);
    reinterpret_cast<Method>(vtable[byteOffset/4])(object);
}

int CallMusicInt(void* object,unsigned int byteOffset)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef int (__thiscall *Method)(void*);
    return reinterpret_cast<Method>(vtable[byteOffset/4])(object);
}

void CallMusicSetVolume(void* object,int volume)
{
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void (__thiscall *Method)(void*,int);
    reinterpret_cast<Method>(vtable[0x1C/4])(object,volume);
}

struct MixRecord {
    int sfxIndex;
    int unknown04;
    int balance;
    int volume;
};
static_assert(sizeof(MixRecord)==0x10,"debug metadata MIX size");

MixRecord* MixRecords(SoundAbiFields& fields)
{
    return reinterpret_cast<MixRecord*>(fields.mix);
}

int RetailAbs(int value)
{
    return value<0 ? -value : value;
}

struct DsPrimaryBufferDescRetail {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwBufferBytes;
    unsigned long dwReserved;
    void* lpwfxFormat;
    const void* guid3DAlgorithm;
    unsigned long pad18;
    unsigned long pad1C;
    unsigned long pad20;
};
static_assert(sizeof(DsPrimaryBufferDescRetail)==0x24,"retail DSBUFFERDESC size");

#pragma pack(push,1)
struct WaveFormatExRetailSound {
    unsigned short wFormatTag;
    unsigned short nChannels;
    unsigned long nSamplesPerSec;
    unsigned long nAvgBytesPerSec;
    unsigned short nBlockAlign;
    unsigned short wBitsPerSample;
    unsigned short cbSize;
};
static_assert(sizeof(WaveFormatExRetailSound)==0x12,"retail WAVEFORMATEX size");
#pragma pack(pop)

long DirectSoundSetCooperativeLevel(void* directSound,HWND__* hwnd,unsigned long level)
{
    void** const vtable=*reinterpret_cast<void***>(directSound);
    typedef long (__stdcall *Method)(void*,HWND__*,unsigned long);
    return reinterpret_cast<Method>(vtable[0x18/4])(directSound,hwnd,level);
}

long DirectSoundCreatePrimaryBuffer(void* directSound,DsPrimaryBufferDescRetail* desc,void** buffer)
{
    void** const vtable=*reinterpret_cast<void***>(directSound);
    typedef long (__stdcall *Method)(void*,DsPrimaryBufferDescRetail*,void**,void*);
    return reinterpret_cast<Method>(vtable[0x0C/4])(directSound,desc,buffer,0);
}

long SoundBufferSetFormat(void* buffer,WaveFormatExRetailSound* format)
{
    void** const vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,WaveFormatExRetailSound*);
    return reinterpret_cast<Method>(vtable[0x38/4])(buffer,format);
}
}

MIX::MIX()
    : sfxIndex(-1)
{
    // Retail leaves the remaining three dwords untouched here.
}

SFXBUFFER::SFXBUFFER()
    : directSoundBuffer(0),active(0),sfxIndex(-1)
{
    // Retail leaves volume/balance/startTime untouched until Load/Play.
}

SFX::SFX()
{
    // Retail constructs the two STRING[8] arrays, then initializes only these
    // fields. property/priority remain untouched until SOUND::LoadSFX.
    noRandomFile=0;
    memset(buffer,0,sizeof(buffer));
}

SOUND::SOUND(HWND__* hwnd,RESOURCE* res,int high_quality)
{
    SoundAbiFields& f=Fields(this);
    f.loopMusic=0;
    f.noSfx=0;
    f.sfx=0;
    f.directSound=0;
    f.track1=-1;
    f.soundDisabled=1;
    // Retail source writes only the one-bit high-quality flag here.
    f.caps=(f.caps & ~1u) | (high_quality ? 1u : 0u);
    f.auxWave=-1;
    f.auxAudio=-1;
    f.callbackWindow=0;
    f.music=0;
    f.musicVolume=-1;
    f.fadeMusicTime=0;
    f.soundVolume=100;
    f.callbackWindow=hwnd;
    InitDS();
    LoadSFX(res);
}

void SOUND::InitDS()
{
    SoundAbiFields& f=Fields(this);
    if (!f.soundDisabled || f.directSound)
        return;

    long result=DirectSoundCreate8(0,&f.directSound,0);
    if (result<0) {
        f.soundDisabled=2;
        Error(E_CREATE,"DirectSound",static_cast<unsigned long>(result));
        return;
    }

    if (f.caps & 1u) {
        result=DirectSoundSetCooperativeLevel(f.directSound,f.callbackWindow,2u);
        if (result<0) {
            f.caps &= ~1u;
            Error(E_SET,"PriorityLevel",static_cast<unsigned long>(result));
        }
    }

    if (!(f.caps & 1u))
        result=DirectSoundSetCooperativeLevel(f.directSound,f.callbackWindow,1u);

    if (result<0) {
        ReleaseSoundCom(f.directSound);
        f.directSound=0;
        f.soundDisabled=3;
        Error(E_SET,"CooperativeLevel",static_cast<unsigned long>(result));
        return;
    }

    DsPrimaryBufferDescRetail desc;
    memset(&desc,0,sizeof(desc));
    desc.dwSize=sizeof(desc);
    desc.dwFlags=1u;
    void* primary=0;
    result=DirectSoundCreatePrimaryBuffer(f.directSound,&desc,&primary);
    if (result>=0) {
        result=PlaySoundBuffer(primary,0,0,1u);
        if (result<0)
            Error(E_INVALID,"unable play Primary",0);
        if (f.caps & 1u) {
            WaveFormatExRetailSound format;
            memset(&format,0,sizeof(format));
            format.wFormatTag=1;
            format.nChannels=2;
            format.nSamplesPerSec=44100;
            format.wBitsPerSample=16;
            format.nBlockAlign=static_cast<unsigned short>((format.wBitsPerSample/8)*format.nChannels);
            format.nAvgBytesPerSec=format.nSamplesPerSec*format.nBlockAlign;
            result=SoundBufferSetFormat(primary,&format);
            if (result<0)
                Error(E_SET,"format primary buffer",static_cast<unsigned long>(result));
        }
        ReleaseSoundCom(primary);
    } else {
        Error(E_CREATE,"Primary sound buffer",static_cast<unsigned long>(result));
    }

    f.soundDisabled=0;
    for (int i=0;i<f.noSfx;++i)
        f.sfx[i].ReLoad(this);
}

void SOUND::LoadSFX(RESOURCE* res)
{
    SoundAbiFields& f=Fields(this);
    STRING filename[8];
    STRING ffbname[8];
    const unsigned long beginTime=timeGetTime();
    if (f.soundDisabled)
        return;
    if (!res || !res->IsOpen()) {
        Error(E_OPEN,"res",0);
        return;
    }

    delete[] f.sfx;
    f.sfx=0;
    f.noSfx=res->GetNoSubRes(0x20584653u); // 'SFX '
    if (!f.noSfx) {
        Error(E_SECTION,"SFX ",0);
        return;
    }

    f.sfx=new SFX[f.noSfx+1];
    if (!f.sfx) {
        Error(E_MEMORY,"LoadSfx",static_cast<unsigned long>(f.noSfx));
        return;
    }
    if (res->GoBegin(0x20584653u))
        return;

    int i=0;
    do {
        unsigned long property=0;
        unsigned char priority=0;
        res->Read(&property,4u);
        res->Read(&priority,1u);
        for (int j=0;j<8;++j)
            filename[j].Read(res);
        for (int j=0;j<8;++j)
            ffbname[j].Read(res);
        f.sfx[i++].Load(filename,property,priority,ffbname,this);
    } while (!res->GoNextSub(0x20584653u));

    MYERROR::Log(::Error,
        "LoadSFX::No   =%-15i   sizeof(SFX)   =%-5i    load time     =%-6ims Quality        =%i",
        f.noSfx,static_cast<int>(sizeof(SFX)),timeGetTime()-beginTime,static_cast<int>(f.caps & 1u));
}

// Retail returns arg1 only for an ordered arg1 >= arg2 comparison; NaN falls
// through to arg2, matching this conditional expression.
float Max(float arg1,float arg2)
{
    return arg1>=arg2 ? arg1 : arg2;
}

void SOUND::PlaySFXFromCoor(int nsfx,float x,float y)
{
    SoundAbiFields& f=Fields(this);
    if (!ValidateSFX(nsfx)) {
        Error(E_INVALID,"nsfx",static_cast<unsigned long>(nsfx));
        return;
    }

    SFX& sfx=f.sfx[nsfx];
    if (sfx.IsDistIndepended() && sfx.IsDirIndepended()) {
        PlaySFX(nsfx,0,0);
        return;
    }

    const int distance=static_cast<int>(Max(fabsf(y),fabsf(x)));
    if (sfx.IsDistIndepended()) {
        PlaySFX(nsfx,static_cast<int>(x)*4,0);
        return;
    }
    if (sfx.IsDirIndepended()) {
        PlaySFX(nsfx,0,-distance*2);
        return;
    }
    PlaySFX(nsfx,static_cast<int>(x)*4,-distance*2);
}

void SOUND::PlaySFX(int nsfx,int balance,int volume)
{
    SoundAbiFields& f=Fields(this);
    if (f.soundDisabled)
        return;
    if (!ValidateSFX(nsfx)) {
        Error(E_INVALID,"nsfx",static_cast<unsigned long>(nsfx));
        return;
    }

    volume+=CalcVolume(f.soundVolume);
    if (volume < -3000)
        return;

    if (balance>10000)
        balance=10000;
    if (balance<-10000)
        balance=-10000;

    if (f.sfx[nsfx].IsVIP())
        volume=CalcVolume(f.soundVolume);

    MixRecord* const mix=MixRecords(f);
    int i=0;
    for (;i<32;++i) {
        if (mix[i].sfxIndex<0) {
            mix[i].sfxIndex=nsfx;
            mix[i].volume=volume;
            mix[i].balance=balance;
            return;
        }
    }

    int min=0;
    int exist=0;
    for (i=1;i<32;++i) {
        if (mix[i].sfxIndex==nsfx)
            exist=1;
        if (f.sfx[mix[i].sfxIndex].Priority() < f.sfx[mix[min].sfxIndex].Priority())
            min=i;
    }

    if (min==0 && exist)
        return;

    mix[min].sfxIndex=nsfx;
    mix[min].volume=volume;
    mix[min].balance=balance;
}

int SFX::IsLoaded()
{
    return noRandomFile != 0;
}

int SFX::NoRandomSound()
{
    return noRandomFile;
}

int SFX::IsDistIndepended()
{
    return static_cast<int>(property & 2u);
}

int SFX::IsDirIndepended()
{
    return static_cast<int>(property & 4u);
}

int SFX::IsVIP()
{
    return static_cast<int>(property & 8u);
}

int SFX::Priority()
{
    return static_cast<int>(priority);
}

int SFX::MaxSameSFX()
{
    if (IsVIP())
        return 999999;
    return (static_cast<int>(priority)+10)/10;
}

int SFXBUFFER::IsPlayed()
{
    return active;
}

int SFXBUFFER::GetSFX()
{
    return sfxIndex;
}

unsigned int SFXBUFFER::GetStartTime()
{
    return startTime;
}

int SFXBUFFER::Volume()
{
    return volume;
}

int SFXBUFFER::Balance()
{
    return balance;
}

int SOUND::CalcVolume(int volume)
{
    return static_cast<int>(static_cast<unsigned int>(volume-100) << 5);
}

// Preserve the retail upper-bound comparison exactly: nSfx > noSfx, not >=.
int SOUND::ValidateSFX(int nSfx)
{
    SoundAbiFields& f=Fields(this);
    if (!f.sfx)
        return 1;
    if (nSfx < 0 || nSfx > f.noSfx)
        return 0;
    return f.sfx[nSfx].IsLoaded() ? 1 : 0;
}

int SFX::IsLooped()
{
    return static_cast<int>(property & 1u);
}

int SOUND::IsLooped(int nSfx)
{
    SoundAbiFields& f=Fields(this);
    if (!f.sfx)
        return 0;
    if (ValidateSFX(nSfx) && f.sfx[nSfx].IsLooped())
        return 1;
    return 0;
}

void SOUND::Error(TYPE_ERROR type,const char* text,unsigned long err) const
{
    MYERROR::Error(::Error,"SOUND",type,text,err);
}

void SFXBUFFER::Error(TYPE_ERROR type,const char* text,unsigned long err) const
{
    MYERROR::Error(::Error,"SFXBUFFER[%i]",type,text,err,sfxIndex);
}

void SFXBUFFER::Destroy()
{
    if (directSoundBuffer) {
        void** const vtable=*reinterpret_cast<void***>(directSoundBuffer);
        typedef long (__stdcall *StopMethod)(void*);
        typedef unsigned long (__stdcall *ReleaseMethod)(void*);
        reinterpret_cast<StopMethod>(vtable[0x48/4])(directSoundBuffer);
        reinterpret_cast<ReleaseMethod>(vtable[0x08/4])(directSoundBuffer);
        directSoundBuffer=0;
    }
    directSoundBuffer=0;
    active=0;
    sfxIndex=-1;
    startTime=0;
}

void SFXBUFFER::Stop()
{
    active=0;
    if (directSoundBuffer) {
        void** const vtable=*reinterpret_cast<void***>(directSoundBuffer);
        typedef long (__stdcall *StopMethod)(void*);
        reinterpret_cast<StopMethod>(vtable[0x48/4])(directSoundBuffer);
    }
}

void SFXBUFFER::Pause()
{
    if (active && directSoundBuffer) {
        void** const vtable=*reinterpret_cast<void***>(directSoundBuffer);
        typedef long (__stdcall *StopMethod)(void*);
        reinterpret_cast<StopMethod>(vtable[0x48/4])(directSoundBuffer);
    }
}


void SFXBUFFER::Play(int newBalance,int newVolume)
{
    if (sfxIndex<0 || !directSoundBuffer)
        return;

    volume=newVolume;
    balance=newBalance;
    SetSoundBufferPan(directSoundBuffer,newBalance);
    SetSoundBufferVolume(directSoundBuffer,newVolume);

    if (!active) {
        const unsigned long flags=Sound->IsLooped(sfxIndex) ? 1u : 0u;
        const long result=PlaySoundBuffer(directSoundBuffer,0,0,flags);
        if (result==static_cast<long>(0x88780096u)) {
            Error(E_ERROR,"buffer is lost",0);
            Sound->ReLoadBuffers();
        }
    }
    active=1;
    startTime=CurrentTime;
}

void SFXBUFFER::Resume()
{
    if (sfxIndex<0 || !directSoundBuffer || !active)
        return;

    const unsigned long flags=Sound->IsLooped(sfxIndex) ? 1u : 0u;
    const long result=PlaySoundBuffer(directSoundBuffer,0,0,flags);
    if (result==static_cast<long>(0x88780096u)) {
        MYERROR::Log(::Error,"!!!ERROR!!! SFXBUFFER::buffer is lost");
        Sound->ReLoadBuffers();
    }
}

void SFX::ReLoad(SOUND* sound)
{
    Load(filename,property,priority,ffbname,sound);
}

IDirectSoundBuffer* SFX::Duplicate(IDirectSound* directSound)
{
    IDirectSoundBuffer* duplicated=0;
    const int randomIndex=Random(noRandomFile-1);
    void* source=buffer[randomIndex];
    if (!source)
        return 0;

    const unsigned long refs=AddRefSoundCom(source);
    if (refs>2) {
        ReleaseSoundCom(source);
        void* resultBuffer=0;
        const long result=DuplicateSoundBuffer(directSound,source,&resultBuffer);
        if (result<0)
            MYERROR::Log(::Error,"!!!ERROR!!!SFX:'%s' %X Couldn't duplicate buffer",this,result);
        duplicated=reinterpret_cast<IDirectSoundBuffer*>(resultBuffer);
    } else {
        duplicated=reinterpret_cast<IDirectSoundBuffer*>(source);
    }

    if (duplicated)
        SetSoundBufferPosition(duplicated,0);
    return duplicated;
}

int SOUND::LoadBuffer(int nSfx)
{
    SoundAbiFields& f=Fields(this);
    if (f.soundDisabled)
        return -1;

    if (!ValidateSFX(nSfx)) {
        Error(E_INVALID,"nsfx",static_cast<unsigned long>(nSfx));
        return -1;
    }

    int index=0;
    for (;index<16;++index) {
        if (f.buffers[index].GetSFX()!=nSfx)
            continue;
        if (CurrentTime-f.buffers[index].GetStartTime()<=40u)
            return -1;
        if (f.sfx[nSfx].NoRandomSound()==1 && !f.buffers[index].IsPlayed())
            return index;
    }

    for (index=0;index<16;++index) {
        if (f.buffers[index].GetSFX()<0)
            break;
    }

    if (index>=16) {
        for (index=0;index<16;++index) {
            if (!f.buffers[index].IsPlayed()) {
                f.buffers[index].Destroy();
                break;
            }
        }
    }

    if (index>=16)
        return -1;

    IDirectSoundBuffer* duplicate=f.sfx[nSfx].Duplicate(reinterpret_cast<IDirectSound*>(f.directSound));
    if (!duplicate)
        return -1;

    f.buffers[index].Load(nSfx,duplicate);
    return index;
}

void SOUND::ReLoadBuffers()
{
    SoundAbiFields& f=Fields(this);
    if (!f.sfx)
        return;
    for (int i=0;i<f.noSfx;++i)
        f.sfx[i].ReLoad(this);
}

void SOUND::ResumeSound()
{
    SoundAbiFields& f=Fields(this);
    if (f.soundDisabled)
        return;
    for (int i=0;i<16;++i)
        f.buffers[i].Resume();
}

int SOUND::PlayAudio(int track1,int track2,int track3,int track4)
{
    if (track1<0)
        return 1;

    SoundAbiFields& f=Fields(this);
    if (f.curTrack>=0 &&
        f.track1==track1 && f.track2==track2 &&
        f.track3==track3 && f.track4==track4)
        return 0;

    if (track2<0) track2=track1;
    if (track3<0) track3=track1;
    if (track4<0) track4=track1;

    StopMusic(0);
    f.track1=track1;
    f.track2=track2;
    f.track3=track3;
    f.track4=track4;

    char command[256];
    sprintf(command,"open cdaudio alias FWMUSIC shareable");
    if (mciSendStringA(command,0,0,0))
        return 1;

    mciSendStringA("set FWMUSIC time format tmsf",0,0,0);
    f.curTrack=track1;
    sprintf(command,"play FWMUSIC from %i to %i notify",f.curTrack,f.curTrack+1);
    mciSendStringA(command,0,0,f.callbackWindow);
    return 0;
}

int SOUND::StopMusic(unsigned long fadeTime)
{
    SoundAbiFields& f=Fields(this);
    if (f.music) {
        if (fadeTime) {
            f.fadeMusicTime=static_cast<int>(RealCurrentTime);
            f.fadeMusicDuration=static_cast<int>(fadeTime);
        } else {
            f.fadeMusicTime=0;
            f.fadeMusicDuration=0;
            CallMusicVoid(f.music,0x14);
        }
    }

    f.nextMusicFile="";
    f.loopMusic=0;
    f.track1=-1;
    mciSendStringA("stop FWMUSIC",0,0,0);
    return mciSendStringA("close FWMUSIC",0,0,0) ? 1 : 0;
}

int SOUND::ReplayMusic()
{
    SoundAbiFields& f=Fields(this);
    if (f.track1<0)
        return 0;

    if (f.curTrack==f.track1)
        f.curTrack=f.track2;
    else if (f.curTrack==f.track2)
        f.curTrack=f.track3;
    else if (f.curTrack==f.track3)
        f.curTrack=f.track4;
    else if (f.curTrack==f.track4)
        f.curTrack=f.track1;
    else
        f.curTrack=f.track1;

    char command[256];
    sprintf(command,"play FWMUSIC from %i to %i notify",f.curTrack,f.curTrack+1);
    return mciSendStringA(command,0,0,f.callbackWindow) ? 1 : 0;
}

void SOUND::MusicTact()
{
    SoundAbiFields& f=Fields(this);
    if (!f.music)
        return;

    CallMusicVoid(f.music,0x08);
    if (f.fadeMusicTime) {
        const unsigned long elapsed=RealCurrentTime-static_cast<unsigned long>(f.fadeMusicTime);
        if (elapsed>=static_cast<unsigned long>(f.fadeMusicDuration)) {
            f.fadeMusicTime=0;
            if (f.nextMusicFile!="")
                PlayFile(f.nextMusicFile,f.loopMusic);
            else
                StopMusic(0);
        } else {
            const int fade=-3000*static_cast<int>(elapsed)/f.fadeMusicDuration;
            const int base=(f.musicVolume>=0) ? CalcVolume(f.musicVolume) : 0;
            CallMusicSetVolume(f.music,fade+base);
        }
        return;
    }

    if (Random(15)!=0)
        return;
    if (CallMusicInt(f.music,0x04))
        return;

    if (f.nextMusicFile!="")
        PlayFile(f.nextMusicFile,f.loopMusic);
    else
        StopMusic(0);
}

void SOUND::Tact()
{
    MusicTact();

    SoundAbiFields& f=Fields(this);
    if (f.soundDisabled)
        return;

    MixRecord* const mix=MixRecords(f);
    int pendingCount=0;
    for (int i=0;i<32;++i) {
        if (mix[i].sfxIndex<0)
            continue;

        int sameCount=0;
        for (int b=0;b<16;++b) {
            if (f.buffers[b].GetSFX()!=mix[i].sfxIndex || !f.buffers[b].IsPlayed())
                continue;

            if (IsLooped(mix[i].sfxIndex)) {
                if (RetailAbs(f.buffers[b].Volume()-mix[i].volume)<500 &&
                    RetailAbs(f.buffers[b].Balance()-mix[i].balance)<500) {
                    mix[i].sfxIndex=-1;
                    f.buffers[b].Play(mix[i].balance,mix[i].volume);
                    break;
                }
            }

            ++sameCount;
            if (sameCount>f.sfx[mix[i].sfxIndex].MaxSameSFX()) {
                mix[i].sfxIndex=-1;
                break;
            }
        }
        if (mix[i].sfxIndex>=0)
            ++pendingCount;
    }

    int freeBuffers=0;
    for (int b=0;b<16;++b) {
        if (!f.buffers[b].CheckPlay())
            ++freeBuffers;
    }

    while (pendingCount>freeBuffers) {
        int found=0;
        if (mix[0].sfxIndex<0)
            mix[0].volume=0;
        int candidate=0;
        for (int i=0;i<32;++i) {
            if (mix[i].sfxIndex<0 || f.sfx[mix[i].sfxIndex].IsVIP())
                continue;
            if (mix[i].volume<=mix[candidate].volume) {
                found=1;
                candidate=i;
            }
        }
        if (!found)
            break;
        mix[candidate].sfxIndex=-1;
        --pendingCount;
    }

    for (int b=0;pendingCount>freeBuffers && b<16;++b) {
        if (!f.buffers[b].IsPlayed())
            continue;
        const int nSfx=f.buffers[b].GetSFX();
        if (f.sfx[nSfx].IsVIP())
            continue;
        f.buffers[b].Stop();
        ++freeBuffers;
    }

    for (int i=0;i<32;++i) {
        if (mix[i].sfxIndex<0)
            continue;
        const int bufferIndex=LoadBuffer(mix[i].sfxIndex);
        if (bufferIndex>=0)
            f.buffers[bufferIndex].Play(mix[i].balance,mix[i].volume);
        mix[i].sfxIndex=-1;
    }
}

int SFXBUFFER::CheckPlay()
{
    if (sfxIndex<0 || !directSoundBuffer || !active)
        return 0;

    unsigned long status=0;
    void** const vtable=*reinterpret_cast<void***>(directSoundBuffer);
    typedef long (__stdcall *GetStatusMethod)(void*,unsigned long*);
    reinterpret_cast<GetStatusMethod>(vtable[0x24/4])(directSoundBuffer,&status);
    if (!(status & 1u))
        active=0;

    if (Sound->IsLooped(sfxIndex) && CurrentTime-startTime>200u)
        Stop();
    return active;
}

void SFXBUFFER::Load(int nsfx,IDirectSoundBuffer* newSoundEffects)
{
    sfxIndex=nsfx;
    if (directSoundBuffer) {
        const unsigned long result=ReleaseSoundCom(directSoundBuffer);
        if (result)
            Error(E_ERROR,"SoundBuffer(Load()) release !=0",result);
        directSoundBuffer=0;
    }
    directSoundBuffer=newSoundEffects;
    active=0;
    startTime=0;
}

SFXBUFFER::~SFXBUFFER()
{
    if (directSoundBuffer) {
        const unsigned long result=ReleaseSoundCom(directSoundBuffer);
        if (result)
            Error(E_ERROR,"SoundBuffer release !=0",result);
        directSoundBuffer=0;
    }
}

void SFX::Release()
{
    for (int i=0;i<8;++i) {
        if (buffer[i]) {
            ReleaseSoundCom(buffer[i]);
            buffer[i]=0;
        }
    }
}

SFX::~SFX()
{
    Release();
    // STRING arrays are destroyed automatically in retail order: ffbname, filename.
}

void SOUND::ReleaseDS()
{
    SoundAbiFields& f=Fields(this);
    if (f.soundDisabled)
        return;

    for (int i=0;i<16;++i)
        f.buffers[i].Destroy();
    for (int i=0;i<f.noSfx;++i)
        f.sfx[i].Release();

    const unsigned long result=ReleaseSoundCom(f.directSound);
    if (result)
        Error(E_ERROR,"DirectSound release !=0",result);
    f.directSound=0;

    for (int i=0;i<32;++i)
        *reinterpret_cast<int*>(f.mix + i*0x10 + 0x0C)=-1;
    f.soundDisabled=1;
}

void SOUND::EnableSound()
{
    InitDS();
}

void SOUND::DisableSound()
{
    SoundAbiFields& f=Fields(this);
    if (f.soundDisabled)
        return;
    ReleaseDS();
    if (f.auxWave>=0)
        waveOutSetVolume(static_cast<unsigned int>(f.auxWave),f.oldWave);
}

void SOUND::Enable()
{
    EnableSound();
    EnableMusic();
}

void SOUND::EnableMusic()
{
    Fields(this).fadeMusicTime=0;
}

void SOUND::DisableMusic()
{
    SoundAbiFields& f=Fields(this);
    if (f.music)
        DeleteVirtualSoundObject(f.music);
    f.music=0;
    mciSendStringA("stop FWMUSIC",0,0,0);
    mciSendStringA("close FWMUSIC",0,0,0);
    if (f.auxAudio>=0)
        auxSetVolume(static_cast<unsigned int>(f.auxAudio),f.oldAudio);
    f.curTrack=-1;
    f.fadeMusicTime=0;
}

void SOUND::Disable()
{
    DisableMusic();
    DisableSound();
}

SOUND::~SOUND()
{
    SoundAbiFields& f=Fields(this);
    if (f.music)
        DeleteVirtualSoundObject(f.music);
    f.music=0;

    Disable();

    if (f.sfx) {
        const int count=*reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(f.sfx)-4);
        for (int i=count-1;i>=0;--i)
            f.sfx[i].~SFX();
        ::operator delete(reinterpret_cast<uint8_t*>(f.sfx)-4);
    }
    f.sfx=0;
    f.noSfx=0;

    // m_nextMusicFile and m_buffers are destroyed automatically in retail order.
}

int SOUND::GetNoPlayed()
{
    SoundAbiFields& fields=Fields(this);
    if (fields.soundDisabled)
        return 0;

    int count=0;
    for (int i=15;i>=0;--i) {
        if (fields.buffers[i].IsPlayed())
            ++count;
    }
    return count;
}

int SOUND::PauseMusic()
{
    SoundAbiFields& fields=Fields(this);
    CallStreamControl(fields.music,0x0C);
    if (fields.auxAudio >= 0)
        auxSetVolume(static_cast<unsigned int>(fields.auxAudio),fields.oldAudio);
    return mciSendStringA("stop FWMUSIC",0,0,fields.callbackWindow) ? 1 : 0;
}

int SOUND::ResumeMusic()
{
    SoundAbiFields& fields=Fields(this);
    CallStreamControl(fields.music,0x10);
    if (fields.auxAudio >= 0)
        auxSetVolume(static_cast<unsigned int>(fields.auxAudio),fields.audioVolume);

    char command[256];
    if (fields.track1 >= 0) {
        sprintf(command,"play FWMUSIC to %i notify",fields.curTrack + 1);
        return mciSendStringA(command,0,0,fields.callbackWindow) ? 1 : 0;
    }
    return mciSendStringA("play FWMUSIC notify",0,0,fields.callbackWindow) ? 1 : 0;
}

void SOUND::StopSound()
{
    if (m_soundDisabled)
        return;
    for (int i=0;i<16;++i)
        m_buffers[i].Stop();
}

void SOUND::PauseSound()
{
    SoundAbiFields& fields=Fields(this);
    if (fields.soundDisabled)
        return;
    for (int i=0;i<16;++i)
        fields.buffers[i].Pause();
}

void SOUND::Pause()
{
    PauseMusic();
    PauseSound();
}

void SOUND::Resume()
{
    ResumeMusic();
    ResumeSound();
}

void SOUND::VolumeSound(int volume)
{
    if(volume<0)
        volume=0;
    if(volume>100)
        volume=100;
    Fields(this).soundVolume=volume;
}
