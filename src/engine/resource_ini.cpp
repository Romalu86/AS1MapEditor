#include "mapedit/runtime.hpp"

namespace {
template<class T> void named_init(NAMED_LIST_STRUCT<T>* p,int n)
{
    for (int i=0;i<n;++i) {
        p[i].name.m_buf=STRING::EMPTY;
    }
}
template<> void named_init<CONSTVAR>(NAMED_LIST_STRUCT<CONSTVAR>* p,int n)
{
    for (int i=0;i<n;++i) {
        p[i].name.m_buf=STRING::EMPTY;
        p[i].val.varstr.m_buf=STRING::EMPTY;
    }
}
template<class T> void named_destroy(NAMED_LIST_STRUCT<T>* p,int n)
{
    for (int i=n-1;i>=0;--i) p[i].~NAMED_LIST_STRUCT<T>();
}

template<class T> void named_copy_value(T& dst,const T& src) { dst=src; }
template<> void named_copy_value<CONSTVAR>(CONSTVAR& dst,const CONSTVAR& src) { dst=&src; }
template<class T> void named_expand(LIST<NAMED_LIST_STRUCT<T> >* self,int alloc)
{
    if (alloc<=self->m_max) return;
    NAMED_LIST_STRUCT<T>* old=self->m_data;
    NAMED_LIST_STRUCT<T>* fresh=static_cast<NAMED_LIST_STRUCT<T>*>(::operator new((unsigned int)alloc*sizeof(NAMED_LIST_STRUCT<T>)));
    if (!fresh)
        MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",alloc);
    named_init<T>(fresh,alloc);
    for (int i=0;i<self->m_no;++i) {
        fresh[i].name=old[i].name;
        named_copy_value<T>(fresh[i].val,old[i].val);
    }
    if (old) { named_destroy<T>(old,self->m_max); ::operator delete(old); }
    self->m_data=fresh; self->m_max=alloc;
}
}

STRING::STRING(const char* str,int len)
{
    if (len) Copy(str,len); else m_buf=EMPTY;
}

void STRING::Read(FILE* file)
{
    *this="";
    char chunk[256];
    int n=0;
    int value=0;
    for (;;) {
        if (n==255) {
            chunk[n]=0;
            *this+=chunk;
            n=0;
        }

        value=fgetc(file);
        if (value<0)
            value=0;
        if (value==10)
            value=0;
        if (value!=13)
            chunk[n++]=static_cast<char>(value);
        if (value<=0)
            break;
    }
    *this+=chunk;
}

void STRING::Read(STREAM* file)
{
    *this="";
    char chunk[256];
    int n=0;
    int value=0;
    for (;;) {
        if (n==255) {
            chunk[n]=0;
            *this+=chunk;
            n=0;
        }

        value=0;
        if (file->Read(&value,1u))
            value=0;
        if (value==10)
            value=0;
        if (value!=13)
            chunk[n++]=static_cast<char>(value);

        // Canonical MapEdit.exe 0x00457F0A compares the zero-extended
        // 32-bit read value (JG), not a signed char.  This is essential for
        // CP1251 resource strings: bytes 0x80..0xFF must remain positive.
        if (value<=0)
            break;
    }

    // Retail stores the terminating NUL byte in chunk before leaving the loop.
    *this+=chunk;
}

void STRING::Write(STREAM* file) { file->Write(m_buf,strlen(m_buf)+1u); }

void STRING::RemoveEndChars(const char* chars)
{
    char* p=m_buf+(int)strlen(m_buf)-1;
    while (p>=m_buf && strchr(chars,*p)) --p;
    p[1]=0;
}

int STRING::IsDigit()
{
    return isdigit((unsigned char)m_buf[0]) || (m_buf[0]=='-' && isdigit((unsigned char)m_buf[1]));
}

int STRING::Int()
{
    if (m_buf[1]!='x') return atoi(m_buf);
    int value=0;
    sscanf(m_buf,"%i",&value);
    return value;
}

float STRING::Float()
{
    float value=0.0f;
    sscanf(m_buf,"%f",&value);
    return value;
}

unsigned int STRING::FourCC() { return *reinterpret_cast<unsigned int*>(m_buf); }

STRING STRING::BeforeChar(const char* chars) const
{
    return STRING(m_buf,(int)strcspn(m_buf,chars));
}

char STRING::operator[](int index) { return m_buf[index]; }
int STRING::operator!=(const char* rhs) { return strcmp(m_buf,rhs)!=0; }

FILE* FOpen(const STRING* name,const char* mode)
{
    return name->m_buf[0] ? static_cast<FILE*>(fopen(name->m_buf,mode)) : 0;
}

FSTREAM::FSTREAM(const STRING* name,const char* mode) : file(FOpen(name,mode)) {}
FSTREAM::~FSTREAM() { if (file) fclose(file); }
int FSTREAM::IsOpen() { return file!=0; }
int FSTREAM::IsEnd()
{
    // Retail reads VC6 FILE::_flag & 0x10 directly (0x00412D80).  UCRT FILE
    // is opaque, so feof() is the exact toolchain-boundary equivalent; keep
    // retail's observable 0x10/0 return value.
    return file && feof(file) ? 0x10 : 0;
}
int FSTREAM::Read(void* data,unsigned int size) { return (int)(size-fread(data,1u,size,file)); }
int FSTREAM::Write(const void* data,unsigned int size) { return (int)(size-fwrite(data,1u,size,file)); }
int FSTREAM::Length() { return (int)_filelength(_fileno(file)); }

STRING PROFILE::GetSection(const STRING* section,const STRING* defaultSection)
{
    char buffer[32767]={0};
    if (!GetPrivateProfileSectionA(section->m_buf,buffer,32767u,FileName.m_buf))
        return STRING(defaultSection);

    char* ptr=buffer;
    char* const limit=buffer+32766;
    while (ptr<limit && (*ptr || ptr[1])) {
        if (!*ptr)
            *ptr='\n';
        ++ptr;
    }
    return STRING(buffer);
}

int STRING_STREAM::Read(void* data,unsigned int size)
{
    unsigned char* buffer=static_cast<unsigned char*>(data);
    const unsigned int length=static_cast<unsigned int>(string.Length());
    unsigned int i=0;
    for (;i<size && position<length;++i,++buffer,position+=3u) {
        int byte=0;
        sscanf(string.CharPtr()+position,"%x",&byte);
        *buffer=static_cast<unsigned char>(byte);
    }
    return static_cast<int>(size-i);
}

int STRING_STREAM::Write(const void* data,unsigned int size)
{
    const unsigned char* buffer=static_cast<const unsigned char*>(data);
    for (unsigned int i=0;i<size;++i,++buffer) {
        STRING byte=Printf("%02X ",static_cast<unsigned int>(*buffer));
        string+=&byte;
    }
    return 0;
}

INIVAR::INIVAR() {}
INIVAR::INIVAR(int typ,int no) : type((uint16_t)typ), noelem((uint16_t)no) {}
CONSTVAR::CONSTVAR() : varstr() {}
CONSTVAR::CONSTVAR(const STRING* str) : varstr(), var(const_cast<STRING*>(str)->Int()) { varstr=*str; }
CONSTVAR::~CONSTVAR() {}
CONSTVAR* CONSTVAR::operator=(const CONSTVAR* that) { varstr=that->varstr; var=that->var; return this; }

template<> NAMED_LIST_STRUCT<INIVAR>::~NAMED_LIST_STRUCT() {}
template<> NAMED_LIST_STRUCT<CONSTVAR>::~NAMED_LIST_STRUCT() {}

template<> LIST<NAMED_LIST_STRUCT<INIVAR> >::LIST() : m_no(0),m_max(0),m_data(0) {}
template<> LIST<NAMED_LIST_STRUCT<CONSTVAR> >::LIST() : m_no(0),m_max(0),m_data(0) {}
template<> void LIST<NAMED_LIST_STRUCT<INIVAR> >::Release() { if(m_data){named_destroy<INIVAR>(m_data,m_max);::operator delete(m_data);} m_data=0;m_no=0;m_max=0; }
template<> void LIST<NAMED_LIST_STRUCT<CONSTVAR> >::Release() { if(m_data){named_destroy<CONSTVAR>(m_data,m_max);::operator delete(m_data);} m_data=0;m_no=0;m_max=0; }
template<> LIST<NAMED_LIST_STRUCT<INIVAR> >::~LIST() { Release(); }
template<> LIST<NAMED_LIST_STRUCT<CONSTVAR> >::~LIST() { Release(); }
template<> int LIST<NAMED_LIST_STRUCT<INIVAR> >::No() { return m_no; }
template<> int LIST<NAMED_LIST_STRUCT<CONSTVAR> >::No() { return m_no; }
template<> void LIST<NAMED_LIST_STRUCT<INIVAR> >::Expand(int n) { named_expand<INIVAR>(this,n); }
template<> void LIST<NAMED_LIST_STRUCT<CONSTVAR> >::Expand(int n) { named_expand<CONSTVAR>(this,n); }
template<> void LIST<NAMED_LIST_STRUCT<INIVAR> >::ExpandForInsert() { if(m_no>=m_max) Expand(m_max*2+4); }
template<> void LIST<NAMED_LIST_STRUCT<CONSTVAR> >::ExpandForInsert() { if(m_no>=m_max) Expand(m_max*2+4); }

template<> NAMED_LIST<INIVAR>::NAMED_LIST() {}
template<> NAMED_LIST<CONSTVAR>::NAMED_LIST() {}
template<> NAMED_LIST<INIVAR>::~NAMED_LIST() {}
template<> NAMED_LIST<CONSTVAR>::~NAMED_LIST() {}
template<> STRING* NAMED_LIST<INIVAR>::Name(int i) { return &this->m_data[i].name; }
template<> STRING* NAMED_LIST<CONSTVAR>::Name(int i) { return &this->m_data[i].name; }
template<> INIVAR* NAMED_LIST<INIVAR>::operator[](int i) { return &this->m_data[i].val; }
template<> CONSTVAR* NAMED_LIST<CONSTVAR>::operator[](int i) { return &this->m_data[i].val; }
template<> int NAMED_LIST<INIVAR>::Location(const STRING* name) { for(int i=this->m_no-1;i>=0;--i) if(this->m_data[i].name==name) return i; return -1; }
template<> int NAMED_LIST<CONSTVAR>::Location(const STRING* name) { for(int i=this->m_no-1;i>=0;--i) if(this->m_data[i].name==name) return i; return -1; }
template<> void NAMED_LIST<INIVAR>::Insert(STRING name,INIVAR item) { this->ExpandForInsert(); this->m_data[this->m_no].name=name; this->m_data[this->m_no].val=item; ++this->m_no; }
template<> void NAMED_LIST<CONSTVAR>::Insert(STRING name,CONSTVAR item) { this->ExpandForInsert(); this->m_data[this->m_no].name=name; this->m_data[this->m_no].val=&item; ++this->m_no; }

int RESOURCE::WriteFile(const STRING* filename)
{
    FILE* f=FOpen(filename,"rb");
    if (!f) return -1;
    long len=_filelength(_fileno(f));
    unsigned char* data=static_cast<unsigned char*>(malloc((unsigned int)len));
    if (!data) { fclose(f); return -2; }
    fread(data,1u,(unsigned int)len,f);
    int r=Write(data,(unsigned int)len);
    fclose(f); free(data); return r;
}

int RESOURCE::LoadIni(const STRING* filename)
{
    FSTREAM in(filename,"rt");
    STRING line,token;
    int last=-1,lineNo=0; unsigned int section=0;
    NAMED_LIST<INIVAR> vars;
    NAMED_LIST<CONSTVAR> constants;
    if (!IsOpen()) { ::Error->Window("Not open resouce file"); return 2; }
    if (!in.IsOpen()) { ::Error->Window("Can't open file '%s'",filename->m_buf); return 1; }

    while (!in.IsEnd()) {
        line.Read(&in); ++lineNo;
        line=line.Before(";"); line.RemoveEndChars(" \n\r\t"); line.RemoveBeginChars(" \n\r\t");
        if (line=="") continue;
        if (line.HaveFirst("[")) {
            token=line.After("=");
            section=(token!="") ? token.FourCC() : 0u;
            last=-1; vars.Release(); constants.Release(); continue;
        }
        if (!section) continue;

        int noelem=line.After("[").Int();
        line=line.Before("["); line.RemoveEndChars(" \n\r\t"); if(!noelem) noelem=1;
        struct DECL { const char* a; const char* b; int type; };
        static const DECL decls[]={
            {"CHAR ","CHAR\t",0},{"STRING ","STRING\t",6},{"FLOAT ","FLOAT\t",7},
            {"BYTE ","BYTE\t",1},{"DWORD ","DWORD\t",4},{"WORD ","WORD\t",2},{"FILE ","FILE\t",5}
        };
        bool declared=false;
        for (unsigned int di=0;di<sizeof(decls)/sizeof(decls[0]);++di) {
            if (line.Replace(decls[di].a,"") || line.Replace(decls[di].b,"")) {
                line.RemoveBeginChars(" \n\r\t");
                vars.Insert(STRING(line),INIVAR(decls[di].type,noelem)); declared=true; break;
            }
        }
        if (declared) continue;
        if (line.Replace("const ","") || line.Replace("const\t","")) {
            line.RemoveBeginChars(" \n\r\t");
            token=line.BeforeChar(" \n\r\t");
            line.Replace(token,STRING("")); line.RemoveBeginChars(" \n\r\t");
            constants.Insert(STRING(token),CONSTVAR(&line)); continue;
        }

        token=line.BeforeChar(" =\t");
        if (token=="") continue;
        int pos=vars.Location(&token);
        if (pos<0) { ::Error->Window("INI:Unknown word '%s'",token.m_buf); continue; }
        if (pos!=last+1) {
            int expected=last+1;
            ::Error->Window("INI:Enough word '%s' in %i line",vars.Name(expected)->m_buf,lineNo);
            last=pos; continue;
        }
        last=pos;
        line.Replace(token,STRING("")); line.RemoveBeginChars(" =\t");
        if (pos==0) PreAppend(section,0);
        INIVAR* var=vars[pos];
        switch(var->type) {
        case 0:
            if (line=="") { char zero=0; for(int i=0;i<var->noelem;++i) Write(&zero,1u); break; }
            // Retail CHAR shares the non-empty STRING serialization path.
        case 6:
            for(int i=0;i<var->noelem;++i) {
                STRING value;
                if (var->noelem>1) { value=line.Before(" "); line=line.After(" "); }
                else value=line;
                int ci=constants.Location(&value);
                if(ci>=0) constants[ci]->varstr.Write(this); else value.Write(this);
            }
            break;
        case 7: {
            // Retail initializes the accumulator once before the array loop.
            // Missing trailing Acceleration elements therefore repeat the previous value.
            float valueFloat=0.0f;
            for(int i=0;i<var->noelem;++i) {
                STRING value=line.BeforeChar(" \t");
                line.Replace(value,STRING("")); line.RemoveBeginChars(" \t");
                if (!value.IsDigit() && value[0]!='.' && value!="") {
                    int ci=constants.Location(&value);
                    if(ci>=0) value=constants[ci]->varstr;
                    else ::Error->Window("Undefine constant '%s'",value.m_buf);
                }
                if (value!="" || *vars.Name(pos)!="Acceleration")
                    valueFloat=value.Float();
                Write(&valueFloat,4u);
                if(i<15 && line=="" && i!=var->noelem-1 && *vars.Name(pos)!="Acceleration")
                    ::Error->Window("INI:Not enough element %i in '%s[%i] in %i line'",i,vars.Name(pos)->m_buf,var->noelem,lineNo);
            }
            break;
        }
        case 1: case 2: case 4: {
            // Same retail carry-forward rule as FLOAT for missing Acceleration elements.
            int valueInt=0;
            for(int i=0;i<var->noelem;++i) {
                if (!(line=="" && *vars.Name(pos)=="Acceleration")) {
                    valueInt=0;
                    while(line!="") {
                        STRING part=line.BeforeChar("+ |\t");
                        if (part.IsDigit()) valueInt+=part.Int();
                        else {
                            int ci=constants.Location(&part);
                            if(ci>=0) valueInt+=constants[ci]->var;
                            else ::Error->Window("Undefine constant '%s'",part.m_buf);
                        }
                        line.Replace(part,STRING("")); line.RemoveBeginChars(" \t");
                        if(line.HaveFirst("+")||line.HaveFirst("|")) { line.RemoveBeginChars("+ |\t"); continue; }
                        break;
                    }
                }
                Write(&valueInt,(unsigned int)var->type);
                line.RemoveBeginChars(" \t");
                if(i<15 && line=="" && i!=var->noelem-1 && *vars.Name(pos)!="Acceleration")
                    ::Error->Window("INI:Not enough element %i in '%s[%i]' in %i line",i,vars.Name(pos)->m_buf,var->noelem,lineNo);
            }
            break;
        }
        case 5: {
            int r=WriteFile(&line);
            if(r==-1) { ::Error->Window("INI:Can't open file '%s'",line.m_buf); }
            else if(r==-2) { ::Error->Window("INI:Not enough memory for '%s'",line.m_buf); }
            break;
        }
        default: break;
        }
        if (pos==vars.No()-1) { PostAppend(); last=-1; }
    }
    return 0;
}
