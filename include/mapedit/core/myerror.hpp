#pragma once
// MYERROR owner. Included in ABI order by mapedit/runtime.hpp.
// Retail debug metadata layout (Win32): vfptr +0x00, HWND +0x04, FILE* +0x08,
// 1024-byte filename +0x0C; total 0x40C.
class MYERROR {
public:
    explicit MYERROR(int clear_log_file);
    virtual ~MYERROR();

    HWND__* hwnd;
    void* file;
    char filename[1024];

    void __cdecl WindowExit(const char* str,...);
    static void __cdecl LogTmp(const MYERROR* self,const char* str,...);
    static void __cdecl Log(const MYERROR* self,const char* str,...);
    static void __cdecl LogExit(const MYERROR* self,const char* str,...);
    long FilterExcept(_EXCEPTION_RECORD* info);
    static void __cdecl Error(const MYERROR* self,const char* module,int type,const char* text,unsigned long err,...);
    int __cdecl Window(const char* text,...);
};
static_assert(sizeof(MYERROR)==0x40C, "debug metadata MYERROR size");
