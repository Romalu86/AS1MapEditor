#pragma once
// SOUND owner. Included in ABI order by mapedit/runtime.hpp.
class SOUND {
public:
    SOUND(HWND__* hwnd,RESOURCE* res,int high_quality);
    ~SOUND();
    void InitDS();
    void LoadSFX(RESOURCE* res);
    void Enable();
    void Disable();
    void EnableSound();
    void DisableSound();
    void EnableMusic();
    void DisableMusic();
    void ReleaseDS();
    int ValidateSFX(int nSfx);
    int IsLooped(int nSfx);
    int CalcVolume(int volume);
    int GetNoPlayed();
    int LoadBuffer(int nSfx);
    void MusicTact();
    int PauseMusic();
    int ResumeMusic();
    int StopMusic(unsigned long fadeTime);
    int PlayAudio(int track1,int track2,int track3,int track4);
    int ReplayMusic();
    int PlayFile(STRING file,int loop);
    void PlaySFXFromCoor(int nsfx,float x,float y);
    void PlaySFX(int nsfx,int balance,int volume);
    void StopSFX(int nsfx);
    IDirectSoundBuffer* CreateSoundBufferFromOgg(const STRING& name,OggVorbis_File* vf,FILE** file,int buffer_size) const;
    IDirectSoundBuffer* CreateSoundBufferFromWav(const STRING& name,RESOURCE* wave,int buffer_size) const;
    void ReLoadBuffers();
    void StopSound();
    void PauseSound();
    void ResumeSound();
    void Pause();
    void Resume();
    void Tact();
    void VolumeSound(int volume);
    void VolumeMusic(int volume);
    int FadeAndPlayFile(const STRING* file,int loop,unsigned long fade_time);
    int IsPlayMusic();
private:
    void Error(TYPE_ERROR type,const char* text,unsigned long err) const;
    uint32_t m_caps;              // +0x000
    int m_noSfx;                  // +0x004
    SFX* m_sfx;                   // +0x008
    MIX m_mix[32];                // +0x00C
    HWND__* m_callbackWindow;     // +0x20C
    void* m_directSound;          // +0x210
    SFXBUFFER m_buffers[16];      // +0x214
    int m_curTrack;               // +0x3D4
    int m_track1;                 // +0x3D8
    int m_track2;                 // +0x3DC
    int m_track3;                 // +0x3E0
    int m_track4;                 // +0x3E4
    int m_soundDisabled;          // +0x3E8
    int m_auxAudio;               // +0x3EC
    int m_auxWave;                // +0x3F0
    unsigned long m_oldAudio;     // +0x3F4
    unsigned long m_oldWave;      // +0x3F8
    unsigned long m_audioVolume;  // +0x3FC
    unsigned long m_waveVolume;   // +0x400
    int m_soundVolume;            // +0x404
    int m_musicVolume;            // +0x408
    int m_loopMusic;              // +0x40C
    void* m_music;                // +0x410
    STRING m_nextMusicFile;       // +0x414
    int m_fadeMusicDuration;      // +0x418
    int m_fadeMusicTime;          // +0x41C
};
static_assert(sizeof(SOUND)==0x420, "debug metadata SOUND size");

