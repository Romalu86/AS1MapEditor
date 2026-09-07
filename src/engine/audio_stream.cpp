#include "mapedit/runtime.hpp"

// Audio owner closure implemented from MapEdit.exe / debug metadata.  The Vorbis API
// declarations below deliberately target the historical Xiph 1.0 ABI used by
// the retail executable.  Implementations are vendored as a separate third-
// party boundary; this source does not substitute a modern decoder.

struct OggVorbis_File {
    unsigned char opaque[720];
};
static_assert(sizeof(OggVorbis_File)==720,"libvorbisfile 1.0 OggVorbis_File ABI");

struct vorbis_info_1_0 {
    int version;
    int channels;
    long rate;
    long bitrate_upper;
    long bitrate_nominal;
    long bitrate_lower;
    long bitrate_window;
    void* codec_setup;
};
static_assert(sizeof(vorbis_info_1_0)==0x20,"libvorbis 1.0 vorbis_info ABI");

extern "C" int __cdecl ov_open(void* datasource,OggVorbis_File* vf,const char* initial,long ibytes);
extern "C" int __cdecl ov_clear(OggVorbis_File* vf);
extern "C" __int64 __cdecl ov_pcm_total(OggVorbis_File* vf,int i);
extern "C" int __cdecl ov_raw_seek(OggVorbis_File* vf,__int64 pos);
extern "C" vorbis_info_1_0* __cdecl ov_info(OggVorbis_File* vf,int link);
extern "C" long __cdecl ov_read(OggVorbis_File* vf,char* buffer,int length,int bigendianp,int word,int sgned,int* bitstream);
extern "C" void __cdecl rewind(FILE* file);

namespace {

struct Guid32 {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
};
static_assert(sizeof(Guid32)==16,"Win32 GUID ABI");

// Raw bytes verified directly against MapEdit.exe .rdata.
const Guid32 kClsidFilterGraph={0xE436EBB3u,0x524Fu,0x11CEu,{0x9F,0x53,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
const Guid32 kIidGraphBuilder ={0x56A868A9u,0x0AD4u,0x11CEu,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
const Guid32 kIidMediaControl ={0x56A868B1u,0x0AD4u,0x11CEu,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};
const Guid32 kIidMediaSeeking ={0x36B73880u,0xC2C8u,0x11CFu,{0x8B,0x46,0x00,0x80,0x5F,0x6C,0xEF,0x60}};
const Guid32 kIidBasicAudio   ={0x56A868B3u,0x0AD4u,0x11CEu,{0xB0,0x3A,0x00,0x20,0xAF,0x0B,0xA7,0x70}};

extern "C" long __stdcall CoCreateInstance(const Guid32& rclsid,void* outer,unsigned long clsContext,
                                             const Guid32& riid,void** object);

#pragma pack(push,2)
#pragma pack(push,1)
struct WaveFormatExRetail {
    unsigned short wFormatTag;
    unsigned short nChannels;
    unsigned long nSamplesPerSec;
    unsigned long nAvgBytesPerSec;
    unsigned short nBlockAlign;
    unsigned short wBitsPerSample;
    unsigned short cbSize;
};
static_assert(sizeof(WaveFormatExRetail)==0x12,"retail WAVEFORMATEX ABI");
#pragma pack(pop)

struct DsBufferDescRetail {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwBufferBytes;
    unsigned long dwReserved;
    WaveFormatExRetail* lpwfxFormat;
    Guid32 guid3DAlgorithm;
};
static_assert(sizeof(DsBufferDescRetail)==0x24,"retail DSBUFFERDESC ABI");
#pragma pack(pop)

struct SoundAudioFields {
    unsigned long caps;
    int noSfx;
    SFX* sfx;
    unsigned char mix[0x200];
    HWND__* callbackWindow;
    void* directSound;
    SFXBUFFER buffers[16];
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
    STRING nextMusicFile;
    int fadeMusicDuration;
    int fadeMusicTime;
};
static_assert(sizeof(SoundAudioFields)==0x420,"debug metadata SOUND ABI");

SoundAudioFields& AudioFields(SOUND* sound)
{
    return *reinterpret_cast<SoundAudioFields*>(sound);
}

const SoundAudioFields& AudioFields(const SOUND* sound)
{
    return *reinterpret_cast<const SoundAudioFields*>(sound);
}

long DsCreateSoundBuffer(void* directSound,DsBufferDescRetail* desc,void** buffer)
{
    void** vtable=*reinterpret_cast<void***>(directSound);
    typedef long (__stdcall *Method)(void*,DsBufferDescRetail*,void**,void*);
    return reinterpret_cast<Method>(vtable[0x0C/4])(directSound,desc,buffer,0);
}

unsigned long ComRelease(void* object)
{
    void** vtable=*reinterpret_cast<void***>(object);
    typedef unsigned long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vtable[0x08/4])(object);
}

long ComQueryInterface(void* object,const Guid32& iid,void** out)
{
    void** vtable=*reinterpret_cast<void***>(object);
    typedef long (__stdcall *Method)(void*,const Guid32&,void**);
    return reinterpret_cast<Method>(vtable[0])(object,iid,out);
}

long DsStop(void* buffer)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vtable[0x48/4])(buffer);
}

long DsPlay(void* buffer,unsigned long reserved1,unsigned long priority,unsigned long flags)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long,unsigned long);
    return reinterpret_cast<Method>(vtable[0x30/4])(buffer,reserved1,priority,flags);
}

long DsSetCurrentPosition(void* buffer,unsigned long position)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,unsigned long);
    return reinterpret_cast<Method>(vtable[0x34/4])(buffer,position);
}

long DsSetVolume(void* buffer,long volume)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,long);
    return reinterpret_cast<Method>(vtable[0x3C/4])(buffer,volume);
}

long DsGetStatus(void* buffer,unsigned long* status)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,unsigned long*);
    return reinterpret_cast<Method>(vtable[0x24/4])(buffer,status);
}

long DsGetCurrentPosition(void* buffer,unsigned long* play,unsigned long* write)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,unsigned long*,unsigned long*);
    return reinterpret_cast<Method>(vtable[0x10/4])(buffer,play,write);
}

long DsLock(void* buffer,unsigned long offset,unsigned long bytes,void** p1,unsigned long* n1,
            void** p2,unsigned long* n2,unsigned long flags)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,unsigned long,unsigned long,void**,unsigned long*,void**,unsigned long*,unsigned long);
    return reinterpret_cast<Method>(vtable[0x2C/4])(buffer,offset,bytes,p1,n1,p2,n2,flags);
}

long DsUnlock(void* buffer,void* p1,unsigned long n1,void* p2,unsigned long n2)
{
    void** vtable=*reinterpret_cast<void***>(buffer);
    typedef long (__stdcall *Method)(void*,void*,unsigned long,void*,unsigned long);
    return reinterpret_cast<Method>(vtable[0x4C/4])(buffer,p1,n1,p2,n2);
}

long DsGraphRenderFile(void* graph,const unsigned short* file,void* playlist)
{
    void** vtable=*reinterpret_cast<void***>(graph);
    typedef long (__stdcall *Method)(void*,const unsigned short*,void*);
    return reinterpret_cast<Method>(vtable[0x34/4])(graph,file,playlist);
}

long MediaRun(void* control)
{
    void** vtable=*reinterpret_cast<void***>(control);
    typedef long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vtable[0x1C/4])(control);
}
long MediaPause(void* control)
{
    void** vtable=*reinterpret_cast<void***>(control);
    typedef long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vtable[0x20/4])(control);
}
long MediaStop(void* control)
{
    void** vtable=*reinterpret_cast<void***>(control);
    typedef long (__stdcall *Method)(void*);
    return reinterpret_cast<Method>(vtable[0x24/4])(control);
}
long MediaGetState(void* control,long timeout,long* state)
{
    void** vtable=*reinterpret_cast<void***>(control);
    typedef long (__stdcall *Method)(void*,long,long*);
    return reinterpret_cast<Method>(vtable[0x28/4])(control,timeout,state);
}
long MediaSetPositions(void* seeking,__int64* current,unsigned long currentFlags,__int64* stop,unsigned long stopFlags)
{
    void** vtable=*reinterpret_cast<void***>(seeking);
    typedef long (__stdcall *Method)(void*,__int64*,unsigned long,__int64*,unsigned long);
    return reinterpret_cast<Method>(vtable[0x38/4])(seeking,current,currentFlags,stop,stopFlags);
}
long MediaGetPositions(void* seeking,__int64* current,__int64* stop)
{
    void** vtable=*reinterpret_cast<void***>(seeking);
    typedef long (__stdcall *Method)(void*,__int64*,__int64*);
    return reinterpret_cast<Method>(vtable[0x3C/4])(seeking,current,stop);
}
long BasicAudioPutVolume(void* audio,long volume)
{
    void** vtable=*reinterpret_cast<void***>(audio);
    typedef long (__stdcall *Method)(void*,long);
    return reinterpret_cast<Method>(vtable[0x1C/4])(audio,volume);
}
long BasicAudioGetVolume(void* audio,long* volume)
{
    void** vtable=*reinterpret_cast<void***>(audio);
    typedef long (__stdcall *Method)(void*,long*);
    return reinterpret_cast<Method>(vtable[0x20/4])(audio,volume);
}

void DeleteMusicObject(void* object)
{
    if (!object)
        return;
    void** vtable=*reinterpret_cast<void***>(object);
    typedef void* (__thiscall *DeletingDestructor)(void*,unsigned int);
    reinterpret_cast<DeletingDestructor>(vtable[0])(object,1u);
}

// Retail uses one shared decode buffer in BSS.  Its address is not part of the
// C++ contract; the capacity and sharing semantics are.
unsigned char gOggDecodeBuffer[0x1000];
char gMusicEmptySentinel[8]={0};

} // namespace

class MUSIC {
public:
    STRING filename;

    explicit MUSIC(const STRING& name);
    virtual ~MUSIC();
    virtual int IsPlaying();
    virtual void Tact();
    virtual void Pause();
    virtual void Resume();
    virtual void Stop();
    virtual void Play();
    virtual void SetVolume(int volume);
    void Error(TYPE_ERROR type,const char* text,unsigned long err);
    const STRING& FileName() const;
};
static_assert(sizeof(MUSIC)==0x08,"debug metadata MUSIC size");

class OGG_MUSIC : public MUSIC {
public:
    void* soundBuffer;
    int played;
    int endOfFile;
    unsigned long alignBeforeVorbis;
    OggVorbis_File vorbisFile;
    FILE* file;
    unsigned long writePosition;

    explicit OGG_MUSIC(const STRING& name);
    virtual ~OGG_MUSIC();
    virtual int IsPlaying();
    virtual void Tact();
    virtual void Pause();
    virtual void Resume();
    virtual void Stop();
    virtual void Play();
    virtual void SetVolume(int volume);
};
static_assert(sizeof(OGG_MUSIC)==0x2F0,"debug metadata OGG_MUSIC size");

class DIRECT_SHOW_MUSIC : public MUSIC {
public:
    void* GraphBuilder;
    void* MediaControl;
    void* MediaSeeking;

    explicit DIRECT_SHOW_MUSIC(const STRING& name);
    virtual ~DIRECT_SHOW_MUSIC();
    virtual int IsPlaying();
    virtual void Pause();
    virtual void Resume();
    virtual void Stop();
    virtual void Play();
    virtual void SetVolume(int volume);
};
static_assert(sizeof(DIRECT_SHOW_MUSIC)==0x14,"debug metadata DIRECT_SHOW_MUSIC size");

MUSIC::MUSIC(const STRING& name) : filename(&name) {}

MUSIC::~MUSIC() {}

void MUSIC::Error(TYPE_ERROR type,const char* text,unsigned long err)
{
    MYERROR::Error(::Error,"MUSIC '%s'",type,text,err,filename.CharPtr());
}

int MUSIC::IsPlaying() { return 0; }
void MUSIC::Tact() {}
void MUSIC::Pause() {}
void MUSIC::Resume() {}
void MUSIC::Stop() {}
void MUSIC::Play() {}
void MUSIC::SetVolume(int) {}

const STRING& MUSIC::FileName() const { return filename; }

OGG_MUSIC::OGG_MUSIC(const STRING& name)
    : MUSIC(name), soundBuffer(0), played(0), endOfFile(0), alignBeforeVorbis(0), file(0), writePosition(0)
{
    // Retail does not rely on zeroed OggVorbis_File state before ov_open.
    soundBuffer=Sound->CreateSoundBufferFromOgg(name,&vorbisFile,&file,0x40000);
    if (!soundBuffer)
        Error(E_CREATE,"SoundBuffer",0);
}

OGG_MUSIC::~OGG_MUSIC()
{
    Stop();
    if (soundBuffer) {
        const unsigned long result=ComRelease(soundBuffer);
        if (result)
            Error(E_ERROR,"SoundBuffer release !=0",result);
        soundBuffer=0;
    }
    if (file) {
        ov_clear(&vorbisFile);
        fclose(file);
        file=0;
    }
}

void OGG_MUSIC::Play()
{
    if (!soundBuffer || !file)
        return;
    DsStop(soundBuffer);
    rewind(file);
    writePosition=0x10;
    endOfFile=0;
    DsSetCurrentPosition(soundBuffer,0);
    ov_raw_seek(&vorbisFile,0);
    Tact();
    DsPlay(soundBuffer,0,0,1);
    played=1;
}

void OGG_MUSIC::Stop()
{
    played=0;
    if (soundBuffer)
        DsStop(soundBuffer);
}

void OGG_MUSIC::Pause()
{
    if (played && soundBuffer)
        DsStop(soundBuffer);
}

void OGG_MUSIC::Resume()
{
    if (played && soundBuffer)
        DsPlay(soundBuffer,0,0,1);
}

int OGG_MUSIC::IsPlaying()
{
    if (!soundBuffer || !played)
        return 0;
    unsigned long status=0;
    DsGetStatus(soundBuffer,&status);
    if (!(status&1u))
        played=0;
    return played;
}

void OGG_MUSIC::SetVolume(int volume)
{
    if (soundBuffer)
        DsSetVolume(soundBuffer,volume);
}

void OGG_MUSIC::Tact()
{
    if (!soundBuffer)
        return;

    for (;;) {
        unsigned long playPosition=0;
        unsigned long hardwareWritePosition=0;
        DsGetCurrentPosition(soundBuffer,&playPosition,&hardwareWritePosition);

        if (endOfFile) {
            if (endOfFile==2 && playPosition<writePosition)
                endOfFile=1;
            if (endOfFile==1 && playPosition>=writePosition)
                Stop();
            return;
        }

        unsigned long freeSize=0;
        if (playPosition<writePosition)
            freeSize=0x40000u-(writePosition-playPosition);
        else
            freeSize=playPosition-writePosition;
        if (freeSize<0x1000u)
            return;

        int currentSection=0;
        const long ret=ov_read(&vorbisFile,reinterpret_cast<char*>(gOggDecodeBuffer),0x1000,0,2,1,&currentSection);
        if (ret==0) {
            endOfFile=(writePosition<playPosition) ? 2 : 1;
            continue;
        }
        if (ret<0) {
            Error(E_ERROR,"decode",0);
            continue;
        }

        void* p1=0;
        void* p2=0;
        unsigned long n1=0;
        unsigned long n2=0;
        if (DsLock(soundBuffer,writePosition,static_cast<unsigned long>(ret),&p1,&n1,&p2,&n2,0)>=0) {
            const unsigned long requested=static_cast<unsigned long>(ret);
            if (requested<=n1) {
                n1=requested;
                n2=0;
            } else {
                n2=requested-n1;
            }
            memcpy(p1,gOggDecodeBuffer,n1);
            if (n2)
                memcpy(p2,gOggDecodeBuffer+n1,n2);
            DsUnlock(soundBuffer,p1,n1,p2,n2);
            writePosition=(writePosition+requested)%0x40000u;
        }
    }
}

DIRECT_SHOW_MUSIC::DIRECT_SHOW_MUSIC(const STRING& name)
    : MUSIC(name), GraphBuilder(0), MediaControl(0), MediaSeeking(0)
{
    long hr=0;
    hr=CoCreateInstance(kClsidFilterGraph,0,3,kIidGraphBuilder,&GraphBuilder);
    if (hr<0) {
        Error(E_CREATE,"GraphBuilder",static_cast<unsigned long>(hr));
        return;
    }

    if (ComQueryInterface(GraphBuilder,kIidMediaControl,&MediaControl)<0) {
        // Retail passes the preceding CoCreateInstance result here.
        Error(E_CREATE,"MediaControl",static_cast<unsigned long>(hr));
        return;
    }
    if (ComQueryInterface(GraphBuilder,kIidMediaSeeking,&MediaSeeking)<0) {
        Error(E_CREATE,"MediaSeeking",0);
        return;
    }
    if (!FExist(&name)) {
        Error(E_OPEN,name.m_buf,0);
        return;
    }

    unsigned short wideName[0x400];
    const_cast<STRING&>(name).ToWideChar(wideName,0x400);
    hr=DsGraphRenderFile(GraphBuilder,wideName,0);
    if (hr<0)
        Error(E_INVALID,"RenderFile",static_cast<unsigned long>(hr));
}

DIRECT_SHOW_MUSIC::~DIRECT_SHOW_MUSIC()
{
    Stop();
    if (MediaSeeking) { ComRelease(MediaSeeking); MediaSeeking=0; }
    if (MediaControl) { ComRelease(MediaControl); MediaControl=0; }
    if (GraphBuilder) { ComRelease(GraphBuilder); GraphBuilder=0; }
}

void DIRECT_SHOW_MUSIC::Play()
{
    if (MediaSeeking) {
        __int64 start=0;
        if (MediaSetPositions(MediaSeeking,&start,1,0,0)<0 && MediaControl)
            MediaStop(MediaControl);
    }
    if (MediaControl)
        MediaRun(MediaControl);
}
void DIRECT_SHOW_MUSIC::Stop() { if (MediaControl) MediaStop(MediaControl); }
void DIRECT_SHOW_MUSIC::Pause() { if (MediaControl) MediaPause(MediaControl); }
void DIRECT_SHOW_MUSIC::Resume() { if (MediaControl) MediaRun(MediaControl); }
int DIRECT_SHOW_MUSIC::IsPlaying()
{
    if (!MediaControl)
        return 0;
    long state=0;
    MediaGetState(MediaControl,3000,&state);
    if (state!=2 || !MediaSeeking)
        return 0;
    __int64 current=0;
    __int64 stop=0;
    MediaGetPositions(MediaSeeking,&current,&stop);
    return (stop-current)!=0;
}

void DIRECT_SHOW_MUSIC::SetVolume(int volume)
{
    if (!GraphBuilder)
        return;
    void* basicAudio=0;
    long hr=ComQueryInterface(GraphBuilder,kIidBasicAudio,&basicAudio);
    if (hr<0) {
        Error(E_GET,"BasicAudio",static_cast<unsigned long>(hr));
        return;
    }

    long oldVolume=0;
    hr=BasicAudioGetVolume(basicAudio,&oldVolume);
    if (static_cast<unsigned long>(hr)!=0x80004001u) {
        if (hr>=0) {
            hr=BasicAudioPutVolume(basicAudio,volume);
            if (hr<0)
                Error(E_SET,"Volume",static_cast<unsigned long>(hr));
        } else {
            Error(E_GET,"Volume",static_cast<unsigned long>(hr));
        }
    }
    ComRelease(basicAudio);
}

IDirectSoundBuffer* SOUND::CreateSoundBufferFromOgg(const STRING& name,OggVorbis_File* vf,FILE** file,int buffer_size) const
{
    const SoundAudioFields& fields=AudioFields(this);
    if (!fields.directSound) {
        *file=0;
        return 0;
    }

    *file=FOpen(&name,"rb");
    if (!*file) {
        Error(E_OPEN,const_cast<STRING&>(name).CharPtr(),0);
        return 0;
    }
    if (ov_open(*file,vf,0,0)<0) {
        Error(E_INVALID,const_cast<STRING&>(name).CharPtr(),0);
        fclose(*file);
        *file=0;
        return 0;
    }

    vorbis_info_1_0* info=ov_info(vf,-1);
    WaveFormatExRetail format;
    format.wFormatTag=1;
    format.nChannels=static_cast<unsigned short>(info->channels);
    format.nSamplesPerSec=static_cast<unsigned long>(info->rate);
    format.nAvgBytesPerSec=static_cast<unsigned long>(info->rate*info->channels*2);
    format.nBlockAlign=static_cast<unsigned short>(info->channels*2);
    format.wBitsPerSample=16;
    // Yes, retail stores 0x12 here instead of the canonical PCM cbSize=0.
    format.cbSize=0x12;

    DsBufferDescRetail desc;
    memset(&desc,0,sizeof(desc));
    desc.dwSize=sizeof(desc);
    desc.dwFlags=0x10082;
    if (buffer_size)
        desc.dwBufferBytes=static_cast<unsigned long>(buffer_size);
    else
        desc.dwBufferBytes=static_cast<unsigned long>(ov_pcm_total(vf,-1)*info->channels*2);
    desc.lpwfxFormat=&format;

    void* soundBuffer=0;
    const long hr=DsCreateSoundBuffer(fields.directSound,&desc,&soundBuffer);
    if (hr<0) {
        Error(E_CREATE,"SoundBuffer for ogg",static_cast<unsigned long>(hr));
        return 0;
    }
    return reinterpret_cast<IDirectSoundBuffer*>(soundBuffer);
}

IDirectSoundBuffer* SOUND::CreateSoundBufferFromWav(const STRING& name,RESOURCE* wave,int) const
{
    const SoundAudioFields& fields=AudioFields(this);
    if (!fields.directSound)
        return 0;
    if (const_cast<STRING&>(name).operator==("wav\\null.wav") ||
        const_cast<STRING&>(name).operator==("null.wav") ||
        const_cast<STRING&>(name).operator==("null"))
        return 0;

    if (wave->OpenForRead(&name,0x45564157u))
        return 0;
    if (wave->GoBegin(0x20746D66u)) {
        MYERROR::Log(::Error,"!!!ERROR!!!SFX:'%s' 'fmt ' not found",name.m_buf);
        return 0;
    }
    if (wave->ResSize()<14) {
        MYERROR::Log(::Error,"!!!ERROR!!!SFX:'%s' incorrect size %i",name.m_buf,wave->ResSize());
        return 0;
    }

    WaveFormatExRetail format;
    wave->Read(&format,sizeof(format));
    if (wave->GoNext(0x61746164u) && wave->GoBegin(0x61746164u)) {
        MYERROR::Log(::Error,"!!!ERROR!!!SFX:'%s' 'data' not found",name.m_buf);
        return 0;
    }

    DsBufferDescRetail desc;
    memset(&desc,0,sizeof(desc));
    desc.dwSize=sizeof(desc);
    desc.dwFlags=0xC2;
    desc.dwBufferBytes=static_cast<unsigned long>(wave->ResSize());
    desc.lpwfxFormat=&format;
    void* soundBuffer=0;
    const long hr=DsCreateSoundBuffer(fields.directSound,&desc,&soundBuffer);
    if (hr<0) {
        Error(E_CREATE,"SoundBuffer",static_cast<unsigned long>(hr));
        return 0;
    }
    return reinterpret_cast<IDirectSoundBuffer*>(soundBuffer);
}

int SOUND::PlayFile(STRING file,int loop)
{
    SoundAudioFields& fields=AudioFields(this);
    if (file==gMusicEmptySentinel)
        return 1;

    fields.loopMusic=loop;
    fields.fadeMusicTime=0;
    if (loop)
        fields.nextMusicFile=&file;
    else {
        STRING empty(gMusicEmptySentinel+4);
        fields.nextMusicFile=&empty;
    }

    if (fields.music) {
        MUSIC* current=reinterpret_cast<MUSIC*>(fields.music);
        if (file.operator!=(&current->FileName())) {
            // Same file: retail retains the existing stream object and simply
            // restarts it below.  Different file: scalar deleting destructor.
            DeleteMusicObject(fields.music);
            fields.music=0;
        }
    }

    if (!fields.music) {
        if (fields.musicVolume==0)
            return 0;
        if (file.HaveSubStr(".ogg") || file.HaveSubStr(".OGG") || file.HaveSubStr(".Ogg"))
            fields.music=new OGG_MUSIC(file);
        else
            fields.music=new DIRECT_SHOW_MUSIC(file);
    }

    if (fields.musicVolume>0)
        reinterpret_cast<MUSIC*>(fields.music)->SetVolume(CalcVolume(fields.musicVolume));
    reinterpret_cast<MUSIC*>(fields.music)->Play();
    return 0;
}

void SFX::Load(STRING* const name,unsigned long newProperty,unsigned char newPriority,STRING* const newFfbName,SOUND* sound)
{
    RESOURCE wave;
    Release();

    for (int i=0;i<8;++i) {
        filename[i]=&name[i];
        ffbname[i]=&newFfbName[i];
    }
    priority=newPriority;
    property=newProperty;

    for (int i=0;i<8;++i) {
        if (!filename[i].operator!=(STRING::EMPTY))
            break;

        if (filename[i].HaveSubStr(".ogg") || filename[i].HaveSubStr(".OGG") || filename[i].HaveSubStr(".Ogg")) {
            OggVorbis_File vf;
            FILE* file=0;
            buffer[i]=sound->CreateSoundBufferFromOgg(filename[i],&vf,&file,0);
            unsigned long position=0;
            if (buffer[i]) {
                for (;;) {
                    int currentSection=0;
                    const long ret=ov_read(&vf,reinterpret_cast<char*>(gOggDecodeBuffer),0x1000,0,2,1,&currentSection);
                    if (ret==0)
                        break;
                    if (ret<0) {
                        MYERROR::Error(::Error,"SFX",10,"decode ogg",static_cast<unsigned long>(ret));
                        // Retail 0x0043E86B still executes position += ret after the decode error.
                        position+=static_cast<unsigned long>(ret);
                        continue;
                    }
                    void* p1=0;
                    void* p2=0;
                    unsigned long n1=0;
                    unsigned long n2=0;
                    if (DsLock(buffer[i],position,static_cast<unsigned long>(ret),&p1,&n1,&p2,&n2,0)>=0) {
                        memcpy(p1,gOggDecodeBuffer,n1);
                        if (n2)
                            memcpy(p2,gOggDecodeBuffer+n1,n2);
                        DsUnlock(buffer[i],p1,n1,p2,n2);
                    }
                    position+=static_cast<unsigned long>(ret);
                }
            }
            // This ordering is intentional: it is what the retail SFX owner
            // emits, even though OGG_MUSIC::~OGG_MUSIC uses the reverse order.
            if (file) {
                fclose(file);
                ov_clear(&vf);
            }
        } else {
            buffer[i]=sound->CreateSoundBufferFromWav(filename[i],&wave,0);
            if (buffer[i]) {
                void* p1=0;
                void* p2=0;
                unsigned long n1=0;
                unsigned long n2=0;
                if (DsLock(buffer[i],0,static_cast<unsigned long>(wave.ResSize()),&p1,&n1,&p2,&n2,0)>=0) {
                    wave.Read(p1,n1);
                    if (n2)
                        wave.Read(p2,n2);
                    DsUnlock(buffer[i],p1,n1,p2,n2);
                }
            }
            wave.Close();
        }
        noRandomFile=i+1;
    }
}

void SOUND::StopSFX(int nsfx)
{
    SoundAudioFields& fields=AudioFields(this);
    if (fields.soundDisabled)
        return;

    if (nsfx==-1) {
        StopSound();
        return;
    }

    for (int i=0;i<16;++i) {
        if (fields.buffers[i].GetSFX()==nsfx)
            fields.buffers[i].Destroy();
    }
}

void SOUND::VolumeMusic(int volume)
{
    SoundAudioFields& fields=AudioFields(this);
    if (volume<0)
        volume=0;
    if (volume>100)
        volume=100;

    if (volume==0 && fields.music)
        DisableMusic();

    if (volume!=0 && !fields.music) {
        const unsigned long channel=static_cast<unsigned long>((volume*0xFFFF)/100);
        fields.audioVolume=channel | (channel<<16);
        if (fields.auxAudio>=0)
            auxSetVolume(static_cast<unsigned int>(fields.auxAudio),fields.audioVolume);
    }

    fields.musicVolume=volume;

    if (!fields.music && volume!=0 && fields.nextMusicFile.m_buf && *fields.nextMusicFile.m_buf)
        PlayFile(fields.nextMusicFile,fields.loopMusic);

    if (fields.music)
        reinterpret_cast<MUSIC*>(fields.music)->SetVolume(CalcVolume(volume));
}

int SOUND::FadeAndPlayFile(const STRING* file,int loop,unsigned long fadeTime)
{
    SoundAudioFields& fields=AudioFields(this);
    if (!file)
        return 0;
    if (!fields.music)
        return PlayFile(*file,loop);
    fields.nextMusicFile=file;
    fields.loopMusic=loop;
    fields.fadeMusicTime=(int)CurrentTime;
    fields.fadeMusicDuration=(int)fadeTime;
    return 0;
}

int SOUND::IsPlayMusic()
{
    const SoundAudioFields& fields=AudioFields(this);
    if (!fields.music)
        return 0;
    return reinterpret_cast<MUSIC*>(fields.music)->IsPlaying() || fields.loopMusic;
}
