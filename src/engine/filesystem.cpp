#include "mapedit/runtime.hpp"

extern "C" unsigned long __stdcall GetCurrentDirectoryA(unsigned long length,char* buffer);
extern "C" int __cdecl remove(const char* filename);
extern "C" int __cdecl rename(const char* oldName,const char* newName);
extern "C" char* __cdecl _tempnam(const char* path,const char* prefix);


// Retail asks the CRT for a temporary name, copies it to a fixed local buffer,
// frees the CRT allocation and then constructs the returned STRING.
STRING FTempFile(const char* path,const char* prefix)
{
    char buffer[4096];
    char* generated=_tempnam(path,prefix);
    if (!generated)
        return STRING(STRING::EMPTY);
    strcpy(buffer,generated);
    free(generated);
    return STRING(buffer);
}

STRING FCurrentDirectory()
{
    char directory[4096];
    GetCurrentDirectoryA(sizeof(directory), directory);
    return STRING(directory);
}

int FRemove(const STRING* name)
{
    return remove(const_cast<STRING*>(name)->CharPtr());
}

int FRename(const STRING* oldName,const STRING* newName)
{
    return rename(const_cast<STRING*>(oldName)->CharPtr(),
                  const_cast<STRING*>(newName)->CharPtr());
}

struct FILETIME_OLD {
    unsigned long low;
    unsigned long high;
};
struct SYSTEMTIME_OLD {
    unsigned short year;
    unsigned short month;
    unsigned short dayOfWeek;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned short milliseconds;
};
struct TM32_DATE {
    int sec,min,hour,mday,mon,year,wday,yday,isdst;
};

extern "C" long __cdecl _time32(long*);
extern "C" TM32_DATE* __cdecl _localtime32(const long*);
extern "C" unsigned int __cdecl strftime(char*,unsigned int,const char*,const TM32_DATE*);
extern "C" void* __stdcall CreateFileA(const char*,unsigned long,unsigned long,void*,unsigned long,unsigned long,void*);
extern "C" int __stdcall GetFileTime(void*,FILETIME_OLD*,FILETIME_OLD*,FILETIME_OLD*);
extern "C" int __stdcall CloseHandle(void*);
extern "C" int __stdcall FileTimeToLocalFileTime(const FILETIME_OLD*,FILETIME_OLD*);
extern "C" int __stdcall FileTimeToSystemTime(const FILETIME_OLD*,SYSTEMTIME_OLD*);
extern "C" int __stdcall GetDateFormatA(unsigned long,unsigned long,const SYSTEMTIME_OLD*,const char*,char*,int);
extern "C" int __stdcall GetTimeFormatA(unsigned long,unsigned long,const SYSTEMTIME_OLD*,const char*,char*,int);

const STRING GetTimeString()
{
    long now=0;
    _time32(&now);
    TM32_DATE* local=_localtime32(&now);
    char text[256];
    strftime(text,sizeof(text),"%H:%M:%S",local);
    return STRING(text);
}

const STRING GetDateString()
{
    long now=0;
    _time32(&now);
    TM32_DATE* local=_localtime32(&now);
    char text[256];
    strftime(text,sizeof(text),"%Y-%m-%d",local);
    return STRING(text);
}

const STRING FGetTime(const STRING* fileName)
{
    STRING result;
    FILETIME_OLD created;
    FILETIME_OLD accessed;
    FILETIME_OLD written;
    void* handle=CreateFileA(const_cast<STRING*>(fileName)->CharPtr(),
                             0x80000000u,0,0,3,0x80u,0);
    GetFileTime(handle,&created,&accessed,&written);
    CloseHandle(handle);

    result+="Cr-";
    FILETIME_OLD localTime;
    SYSTEMTIME_OLD systemTime;
    char text[80];

    FileTimeToLocalFileTime(&created,&localTime);
    FileTimeToSystemTime(&localTime,&systemTime);
    GetDateFormatA(0x400,0,&systemTime,"yyyy-MM-dd",text,sizeof(text));
    result+=text;
    result+=" ";
    GetTimeFormatA(0x400,0,&systemTime,"hh:mm:ss",text,sizeof(text));
    result+=text;

    result+=" La-";
    FileTimeToLocalFileTime(&accessed,&localTime);
    FileTimeToSystemTime(&localTime,&systemTime);
    GetDateFormatA(0x400,0,&systemTime,"yyyy-MM-dd",text,sizeof(text));
    result+=text;
    result+=" ";
    GetTimeFormatA(0x400,0,&systemTime,"hh:mm:ss",text,sizeof(text));
    result+=text;

    result+=" Lw-";
    FileTimeToLocalFileTime(&written,&localTime);
    FileTimeToSystemTime(&localTime,&systemTime);
    GetDateFormatA(0x400,0,&systemTime,"yyyy-MM-dd",text,sizeof(text));
    result+=text;
    result+=" ";
    GetTimeFormatA(0x400,0,&systemTime,"hh:mm:ss",text,sizeof(text));
    result+=text;
    return result;
}

