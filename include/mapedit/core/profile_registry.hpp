#pragma once
// PROFILE/REGISTRY owners. Included in ABI order by mapedit/runtime.hpp.
class PROFILE {
public:
    STRING FileName;                 // +0x00
    PROFILE(const STRING* filename);
    ~PROFILE();
    int Load(const STRING* filename);
    int GetInt(const STRING* section,const STRING* keyword,int defaultValue);
    STRING GetString(const STRING* section,const STRING* keyword,const STRING* defaultString);
    STRING GetSection(const STRING* section,const STRING* defaultSection);
};
static_assert(sizeof(PROFILE)==4, "debug metadata PROFILE size");

class REGISTRY {
public:
    STRING path;
    REGISTRY(const STRING& registryPath);
    ~REGISTRY();
    const STRING* Path();
    int GetInt(const STRING& name,int default_int);
    unsigned long GetData(const STRING& name,void* data,unsigned long size);
    void SetInt(const STRING& name,int value);
    void SetData(const STRING& name,const void* data,unsigned long size);
    STRING GetString(const STRING& name,const STRING& defaultString);
    void SetString(const STRING& name,const STRING& value);
    void Delete(const STRING& name);
};
static_assert(sizeof(REGISTRY)==4, "debug metadata REGISTRY size");
