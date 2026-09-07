#pragma once
// MOVIE owner. Included in ABI order by mapedit/runtime.hpp.
class MOVIE {
public:
    MOVIE();
    void* pGraph;
    void* pMediaControl;
    void* pEvent;
    void* pVidWin;
    ~MOVIE();
    int IsOpen();
    int Update();
    void Pause();
    void Resume();
    void Release();
    void Open(const STRING* file,int center_x,int center_y);
    void Error(TYPE_ERROR type,char* text,unsigned long err);
};
static_assert(sizeof(MOVIE)==0x10, "debug metadata MOVIE size");

