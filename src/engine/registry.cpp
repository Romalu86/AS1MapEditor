#include "mapedit/runtime.hpp"

namespace {
char g_registryStringBuffer[512];
}

REGISTRY::REGISTRY(const STRING& registryPath) : path(registryPath) {}

REGISTRY::~REGISTRY() {}

const STRING* REGISTRY::Path()
{
    return &path;
}

int REGISTRY::GetInt(const STRING& name,int defaultInt)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    int result = defaultInt;
    if (RegOpenKeyExA(root, subKey.CharPtr(), 0, 1, &key) == 0) {
        unsigned long size = 0x1FF;
        unsigned long type = 0;
        RegQueryValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, &type,
                         reinterpret_cast<unsigned char*>(g_registryStringBuffer), &size);
        if (type == 1)
            result = atoi(g_registryStringBuffer);
        else if (type == 4 || type == 3)
            result = *reinterpret_cast<int*>(g_registryStringBuffer);
        RegCloseKey(key);
    }
    return result;
}

unsigned long REGISTRY::GetData(const STRING& name,void* data,unsigned long size)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    if (RegOpenKeyExA(root, subKey.CharPtr(), 0, 1, &key) != 0)
        return 0;

    unsigned long type = 3;
    RegQueryValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, &type,
                     static_cast<unsigned char*>(data), &size);
    RegCloseKey(key);
    return size;
}

void REGISTRY::SetInt(const STRING& name,int value)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    unsigned long disposition = 0;
    if (RegCreateKeyExA(root, subKey.CharPtr(), 0, 0, 0, 0x000F003Fu, 0, &key, &disposition) == 0) {
        RegSetValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, 4,
                       reinterpret_cast<const unsigned char*>(&value), 4);
        RegCloseKey(key);
    }
}

void REGISTRY::SetData(const STRING& name,const void* data,unsigned long size)
{
    HKEY__* root = 0;
    STRING subKey = path.SplitRegPath(&root);
    HKEY__* key = 0;
    unsigned long disposition = 0;
    if (RegCreateKeyExA(root, subKey.CharPtr(), 0, 0, 0, 0x000F003Fu, 0, &key, &disposition) == 0) {
        RegSetValueExA(key, const_cast<STRING&>(name).CharPtr(), 0, 3,
                       static_cast<const unsigned char*>(data), size);
        RegCloseKey(key);
    }
}

STRING REGISTRY::GetString(const STRING& name,const STRING& defaultString)
{
    HKEY__* root=0;
    STRING subKey=path.SplitRegPath(&root);
    HKEY__* key=0;
    if (RegOpenKeyExA(root,subKey.CharPtr(),0,1,&key)!=0)
        return defaultString;
    unsigned long size=0x1FF;
    unsigned long type=0;
    g_registryStringBuffer[0]=0;
    RegQueryValueExA(key,const_cast<STRING&>(name).CharPtr(),0,&type,
                     reinterpret_cast<unsigned char*>(g_registryStringBuffer),&size);
    RegCloseKey(key);
    if (type==1 && g_registryStringBuffer[0])
        return STRING(g_registryStringBuffer);
    return defaultString;
}

void REGISTRY::SetString(const STRING& name,const STRING& value)
{
    HKEY__* root=0;
    STRING subKey=path.SplitRegPath(&root);
    HKEY__* key=0;
    unsigned long disposition=0;
    if (RegCreateKeyExA(root,subKey.CharPtr(),0,0,0,0x000F003Fu,0,&key,&disposition)==0) {
        const unsigned long size=value.m_buf ? strlen(value.m_buf)+1 : 1;
        const unsigned char* data=reinterpret_cast<const unsigned char*>(value.m_buf ? value.m_buf : "");
        RegSetValueExA(key,const_cast<STRING&>(name).CharPtr(),0,1,data,size);
        RegCloseKey(key);
    }
}

void REGISTRY::Delete(const STRING& name)
{
    HKEY__* root=0;
    STRING subKey=path.SplitRegPath(&root);
    HKEY__* key=0;
    if (RegOpenKeyExA(root,subKey.CharPtr(),0,0x00020006u,&key)==0) {
        RegDeleteValueA(key,const_cast<STRING&>(name).CharPtr());
        RegCloseKey(key);
    }
}
