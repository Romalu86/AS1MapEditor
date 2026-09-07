#include "mapedit/runtime.hpp"


int ScriptExecFunc(int command)
{
    return Map->ExecFunc(command);
}

namespace {
const unsigned char ST_STRING   = 0x01;
const unsigned char ST_VALUE    = 0x02;
const unsigned char ST_ARRAY    = 0x04;
const unsigned char ST_INIT     = 0x08;
const unsigned char ST_OBJECT   = 0x10;
const unsigned char ST_POINTER  = 0x20;
const unsigned char ST_UNINITIALIZED = 0x40;

void LogicOutOfMemory(int count)
{
    MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory %i",count);
}
}

// -----------------------------------------------------------------------------
// Exact debug metadata layout support for the retail script VM.
// -----------------------------------------------------------------------------

LOGICSTACK::LOGICSTACK()
    : type(0), value(0), string(), name(), no_array_element(1)
{
}

LOGICSTACK::LOGICSTACK(const LOGICSTACK& r)
    : type(r.type), value(r.value), string(r.string), name(r.name),
      no_array_element(r.no_array_element)
{
}

LOGICSTACK::LOGICSTACK(int val)
    : type(ST_VALUE), value(val), string(), name(), no_array_element(1)
{
}

LOGICSTACK::LOGICSTACK(const STRING* str)
    : type(ST_STRING), value(0), string(str), name(), no_array_element(1)
{
}

LOGICSTACK::LOGICSTACK(const void* object)
    : type(static_cast<unsigned char>(ST_OBJECT|ST_VALUE)),
      value(reinterpret_cast<int>(object)), string(), name(), no_array_element(1)
{
    if (!object)
        type=ST_VALUE;
}

LOGICSTACK::~LOGICSTACK()
{
    // STRING members are destroyed by the compiler in reverse order (name,string).
}

int LOGICSTACK::IsArray()   { return type & ST_ARRAY; }
int LOGICSTACK::IsInt()     { return type & ST_VALUE; }
int LOGICSTACK::IsString()  { return type & ST_STRING; }
int LOGICSTACK::IsObject()  { return type & ST_OBJECT; }
int LOGICSTACK::IsPointer() { return type & ST_POINTER; }
int LOGICSTACK::IsInit()    { return type & ST_INIT; }
int LOGICSTACK::IsStrings(const LOGICSTACK* r) { return (type & ST_STRING) && (r->type & ST_STRING); }

int LOGICSTACK::Int()
{
    return IsString() ? string.Int() : value;
}

LOGICSTACK* LOGICSTACK::operator=(const LOGICSTACK* r)
{
    type=r->type;
    value=r->value;
    string=r->string;
    name=r->name;
    no_array_element=r->no_array_element;
    return this;
}

void LOGICSTACK::Inc() { type=static_cast<unsigned char>(type & ~ST_OBJECT); if (type & (ST_POINTER|ST_VALUE)) ++value; }
void LOGICSTACK::Dec() { type=static_cast<unsigned char>(type & ~ST_OBJECT); if (type & (ST_POINTER|ST_VALUE)) --value; }
void LOGICSTACK::SetInt(int val) { type=ST_VALUE|ST_INIT; value=val; }
void LOGICSTACK::InitInt(int val) { type=ST_VALUE|ST_INIT; value=val; }
void LOGICSTACK::InitString(const STRING* str) { type=ST_STRING|ST_INIT; string=str; }

void LOGICSTACK::Read(STREAM* file)
{
    file->Read(&type,1u);
    if (!(type & ST_INIT))
        return;
    if (IsString()) {
        string.Read(file);
        for (int i=0;i<string.Length();++i)
            string.m_buf[i]^=0x17;
    } else {
        file->Read(&value,4u);
    }
}

// Retail counterpart of Read: the serialized form is tag byte, then an
// encrypted zero-terminated string or a 32-bit value for initialized entries.
void LOGICSTACK::Write(STREAM* file)
{
    file->Write(&type,1u);
    if (!(type & ST_INIT))
        return;
    if (IsString()) {
        STRING encrypted(string);
        for (int i=0;i<encrypted.Length();++i)
            encrypted.m_buf[i]^=0x17;
        encrypted.Write(file);
    } else {
        file->Write(&value,4u);
    }
}

const STRING* LOGICSTACK::String()
{
    if (IsInt())
        string=Int2Str(value);
    return &string;
}

void LOGICSTACK::BinarOperator(int operation,const LOGICSTACK* r)
{
    int temp=0;
    int right;
    if (r->type & ST_STRING) {
        const char* rs=r->string.m_buf;
        if (rs[1]!='x') right=atoi(rs);
        else { sscanf(rs,"%i",&temp); right=temp; }
    } else right=r->value;

    if (type & ST_STRING) {
        const char* ls=string.m_buf;
        if (ls[1]!='x') value=atoi(ls);
        else { sscanf(ls,"%i",&temp); value=temp; }
    }

    switch(operation) {
    case 8:
        if ((r->type & ST_STRING) && (type & ST_STRING)) {
            string=string+r->string;
            type=ST_STRING;
        } else { value+=right; type=ST_VALUE; }
        break;
    case 9: value-=right; type=ST_VALUE; break;
    case 19: value*=right; type=ST_VALUE; break;
    case 6: value=right ? value/right : 0x0fffffff; type=ST_VALUE; break;
    case 7: value%=right; type=ST_VALUE; break;
    case 11: value|=right; type=ST_VALUE; break;
    case 10: value^=right; type=ST_VALUE; break;
    case 12: value&=right; type=ST_VALUE; break;
    case 23: value<<=right; type=ST_VALUE; break;
    case 22: value>>=right; type=ST_VALUE; break;
    case 21: value=(value && right) ? 1 : 0; type=ST_VALUE; break;
    case 14: value=(value || right) ? 1 : 0; type=ST_VALUE; break;
    case 16: value=value<right; type=ST_VALUE; break;
    case 18: value=value<=right; type=ST_VALUE; break;
    case 15: value=value>right; type=ST_VALUE; break;
    case 17: value=value>=right; type=ST_VALUE; break;
    case 13:
        value=((r->type & ST_STRING) && (type & ST_STRING))
            ? !strcmp(string.m_buf,r->string.m_buf) : value==right;
        type=ST_VALUE;
        break;
    case 20:
        value=((r->type & ST_STRING) && (type & ST_STRING))
            ? strcmp(string.m_buf,r->string.m_buf)!=0 : value!=right;
        type=ST_VALUE;
        break;
    default:
        MYERROR::Log(::Error,"!!!ERROE!!!LOGIC::Unknown Binary command %i",operation);
        type=ST_VALUE;
        break;
    }
}

LOGICVAR::LOGICVAR() : type(0), def_str() {}

LOGICVAR::LOGICVAR(int typ,int variable) : type(static_cast<uint8_t>(typ)), def_str(), addr(variable) {}

LOGICVAR::~LOGICVAR() {}

LOGICVAR* LOGICVAR::operator=(const LOGICVAR* r)
{
    type=r->type;
    def_str=r->def_str;
    addr=r->addr;
    begvar=r->begvar;
    no_var=r->no_var;
    return this;
}

template<> NAMED_LIST_STRUCT<LOGICVAR>::~NAMED_LIST_STRUCT() {}
template<> NAMED_LIST_STRUCT<STRING>::~NAMED_LIST_STRUCT() {}

// -----------------------------------------------------------------------------
// LIST<LOGICSTACK> / NAMED_LIST script specializations.
// These are the concrete template owners emitted by retail script.obj/map.obj.
// -----------------------------------------------------------------------------

template<> LIST<LOGICSTACK>::LIST() : m_no(0),m_max(0),m_data(0) {}

template<> void LIST<LOGICSTACK>::Release()
{
    // Retail 0x00445230 clears allocation/count before destroying the array.
    m_max=0;
    m_no=0;
    if (m_data)
        delete[] m_data;
    m_data=0;
}

template<> LIST<LOGICSTACK>::~LIST() { Release(); }
template<> int LIST<LOGICSTACK>::No() { return m_no; }
template<> LOGICSTACK* LIST<LOGICSTACK>::First() { return m_data; }
template<> LOGICSTACK* LIST<LOGICSTACK>::operator[](int index) { return m_data+index; }

template<> void LIST<LOGICSTACK>::Expand(int newAllocation)
{
    if (newAllocation<=m_max)
        return;
    LOGICSTACK* fresh=new LOGICSTACK[newAllocation];
    if (!fresh)
        LogicOutOfMemory(newAllocation);
    if (m_data) {
        for (int i=0;i<m_max;++i)
            fresh[i]=&m_data[i];
        delete[] m_data;
    }
    m_data=fresh;
    m_max=newAllocation;
}

template<> void LIST<LOGICSTACK>::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<LOGICSTACK>::Insert(LOGICSTACK item)
{
    ExpandForInsert();
    m_data[m_no]=&item;
    ++m_no;
}

template<> LOGICSTACK* LIST<LOGICSTACK>::Last() { return m_data+m_no-1; }

template<> void LIST<LOGICSTACK>::Push(const LOGICSTACK* item)
{
    // Retail 0x00445190 materializes the copy and delegates to Insert.
    Insert(*item);
}

template<> LOGICSTACK* LIST<LOGICSTACK>::Pop()
{
    // Retail 0x004451C0 performs no empty-list guard.
    --m_no;
    return m_data+m_no;
}

template<> void LIST<LOGICSTACK>::SetNo(int newNo)
{
    // Retail 0x004451F0 stores m_no first, then expands against the updated count.
    m_no=newNo;
    if (m_no>m_max)
        Expand(m_no);
}


template<> LIST<NAMED_LIST_STRUCT<LOGICVAR> >::LIST() : m_no(0),m_max(0),m_data(0) {}

template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::Release()
{
    if (m_data) {
        delete[] m_data;
    }
    m_data=0; m_no=0; m_max=0;
}

template<> LIST<NAMED_LIST_STRUCT<LOGICVAR> >::~LIST() { Release(); }
template<> int LIST<NAMED_LIST_STRUCT<LOGICVAR> >::No() { return m_no; }
template<> NAMED_LIST_STRUCT<LOGICVAR>* LIST<NAMED_LIST_STRUCT<LOGICVAR> >::operator[](int index) { return m_data+index; }

template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::Expand(int newAllocation)
{
    if (newAllocation<=m_max)
        return;
    typedef NAMED_LIST_STRUCT<LOGICVAR> ITEM;
    ITEM* fresh=new ITEM[newAllocation];
    if (!fresh)
        LogicOutOfMemory(newAllocation);
    if (m_data) {
        for (int i=0;i<m_max;++i) {
            fresh[i].name=m_data[i].name;
            fresh[i].val=&m_data[i].val;
        }
        delete[] m_data;
    }
    m_data=fresh;
    m_max=newAllocation;
}

template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<NAMED_LIST_STRUCT<LOGICVAR> >::DeleteFrom(int n)
{
    if (n<=0) {
        Release();
        return;
    }
    if (n<m_no)
        m_no=n;
}

template<> LIST<NAMED_LIST_STRUCT<STRING> >::LIST() : m_no(0),m_max(0),m_data(0) {}

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::Release()
{
    if (m_data) {
        delete[] m_data;
    }
    m_data=0; m_no=0; m_max=0;
}

template<> LIST<NAMED_LIST_STRUCT<STRING> >::~LIST() { Release(); }
template<> int LIST<NAMED_LIST_STRUCT<STRING> >::No() { return m_no; }

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::Expand(int newAllocation)
{
    if (newAllocation<=m_max) return;
    typedef NAMED_LIST_STRUCT<STRING> ITEM;
    ITEM* fresh=new ITEM[newAllocation];
    if (!fresh) LogicOutOfMemory(newAllocation);
    if (m_data) {
        for (int i=0;i<m_max;++i) {
            fresh[i].name=m_data[i].name;
            fresh[i].val=m_data[i].val;
        }
        delete[] m_data;
    }
    m_data=fresh;
    m_max=newAllocation;
}

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<NAMED_LIST_STRUCT<STRING> >::DeleteNumberS(int n)
{
    if (n>=0 && n<m_no) {
        --m_no;
        while (n<m_no) {
            m_data[n]=m_data[n+1];
            ++n;
        }
    }
    if (m_no==0)
        Release();
}

template<> NAMED_LIST<LOGICVAR>::NAMED_LIST() {}
template<> NAMED_LIST<LOGICVAR>::~NAMED_LIST() {}
template<> STRING* NAMED_LIST<LOGICVAR>::Name(int index) { return &this->m_data[index].name; }

template<> LOGICVAR* NAMED_LIST<LOGICVAR>::Last() { return &this->m_data[this->m_no-1].val; }

template<> LOGICVAR* NAMED_LIST<LOGICVAR>::operator[](int index) { return &this->m_data[index].val; }

template<> NAMED_LIST<STRING>::NAMED_LIST() {}
template<> NAMED_LIST<STRING>::~NAMED_LIST() {}
template<> STRING* NAMED_LIST<STRING>::Name(int index) { return &this->m_data[index].name; }
template<> STRING* NAMED_LIST<STRING>::operator[](int index) { return &this->m_data[index].val; }


template<> int NAMED_LIST<LOGICVAR>::Location(const STRING* name)
{
    for (int i=0;i<this->m_no;++i)
        if (this->m_data[i].name==name) return i;
    return -1;
}

template<> void NAMED_LIST<LOGICVAR>::Insert(STRING name,LOGICVAR item)
{
    if (this->m_no>=this->m_max)
        this->Expand(this->m_max ? this->m_max*2 : 16);
    this->m_data[this->m_no].name=name;
    this->m_data[this->m_no].val=&item;
    ++this->m_no;
}

template<> void NAMED_LIST<LOGICVAR>::Write(STREAM* file)
{
    if (!file)
        return;
    file->Write(&this->m_no,4u);
    for (int i=0;i<this->m_no;++i) {
        this->m_data[i].name.Write(file);
        file->Write(&this->m_data[i].val,0x14u);
    }
}

template<> void NAMED_LIST<LOGICVAR>::Read(STREAM* file)
{
    if (!file)
        return;
    file->Read(&this->m_no,4u);
    this->Expand(this->m_no);
    for (int i=0;i<this->m_no;++i) {
        this->m_data[i].name.Read(file);
        file->Read(&this->m_data[i].val,0x14u);
    }
}

template<> int NAMED_LIST<STRING>::Location(const STRING* name)
{
    for (int i=0;i<this->m_no;++i)
        if (this->m_data[i].name==name) return i;
    return -1;
}

template<> void NAMED_LIST<STRING>::Insert(STRING name,STRING item)
{
    if (this->m_no>=this->m_max)
        this->Expand(this->m_max ? this->m_max*2 : 16);
    this->m_data[this->m_no].name=name;
    this->m_data[this->m_no].val=item;
    ++this->m_no;
}

// -----------------------------------------------------------------------------
// LOGIC object lifetime and binary .LGC reader.
// -----------------------------------------------------------------------------

void LOGIC::PushInt(int v) { LOGICSTACK value(v); stack.Push(&value); }
void LOGIC::PushStr(const STRING* str) { LOGICSTACK value(str); stack.Push(&value); }

void LOGIC::StackError(char* text,int i)
{
    MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: '%s' stack error %i",text,i);
}

int LOGIC::PopInt() { return stack.Pop()->Int(); }
const STRING* LOGIC::PopStr() { return stack.Pop()->String(); }
int LOGIC::PopObject()
{
    if (stack.Last()->value && !stack.Last()->IsObject())
        MYERROR::Error(::Error,"LOGIC",10,"this variable is not unit",0);
    return stack.Pop()->Int();
}
void LOGIC::PushObject(const void* object) { LOGICSTACK value(object); stack.Push(&value); }
int LOGIC::IsLastStackString() { return stack.Last()->IsString(); }

LOGIC::LOGIC()
    : stack(), var(), defines(), filename(), data(0), size(0), pos(0), end(0), ini(0),
      nline(0), ifdef_no(0), mainFunc(-1), withoutDefine(0)
{
}

// automatically after this body, matching retail order filename/defines/var/stack.
LOGIC::~LOGIC()
{
    Release();
}

void LOGIC::Release()
{
    if (data) {
        ::operator delete(data);
        data=0;
    }
    if (ini) {
        ::operator delete(ini);
        ini=0;
    }
    stack.Release();
    var.Release();
    defines.Release();
    size=0;
    ifdef_no=0;
    mainFunc=-1;
    pos=0;
    withoutDefine=0;
}

void LOGIC::LoadVar(STREAM* file)
{
    for (int i=0;i<stack.m_no;++i)
        stack.m_data[i].Read(file);

    // NAMED_LIST<LOGICVAR>::Read, 0x00445AC0.
    int count=0;
    file->Read(&count,4u);
    var.Expand(count);
    var.m_no=count;
    for (int i=0;i<count;++i) {
        NAMED_LIST_STRUCT<LOGICVAR>& item=var.m_data[i];
        item.name.Read(file);
        file->Read(&item.val,0x14u);
    }

    // The serialized LOGICVAR contains a stale pointer-sized def_str slot.
    // Retail overwrites it with STRING::EMPTY before reading the real string.
    for (int i=0;i<var.m_no;++i) {
        var.m_data[i].val.def_str.m_buf=STRING::EMPTY;
        var.m_data[i].val.def_str.Read(file);
    }
}

void LOGIC::Error(int type,char* text,int err)
{
    MYERROR::Error(::Error,"LOGIC '%s' line %i",type,text,static_cast<unsigned long>(err),filename.CharPtr(),nline+1);
    if (!pos)
        return;
    char context[61];
    for (int i=0;i<60;++i) {
        char c=pos[i-30];
        context[i]=(c=='\n' || c=='\r' || c=='\t') ? '?' : c;
    }
    context[60]=0;
    MYERROR::Error(::Error,"LOGIC",10,context,0);
    for (int i=0;i<60;++i)
        context[i]=(i==30) ? '^' : ' ';
    context[60]=0;
    MYERROR::Error(::Error,"LOGIC",10,context,0);
}

int LOGIC::DeletePointerToObject(void* object)
{
    int deleted=0;
    for (int i=0;i<stack.m_no;++i) {
        LOGICSTACK& entry=stack.m_data[i];
        if ((entry.type & ST_OBJECT) && reinterpret_cast<void*>(entry.value)==object) {
            entry.value=0;
            entry.type=static_cast<unsigned char>(entry.type & ~ST_OBJECT);
            ++deleted;
        }
    }
    return deleted;
}

// Exact binary .lgc path.  Textual source scripts are delegated to LoadLGC,
// exactly as in retail when the first dword has high bits set.
int LOGIC::Load(const STRING* scriptName)
{
    int stackCount=stack.No();
    FSTREAM file(scriptName,"rb");

    Release();
    filename=scriptName;

    if (!file.IsOpen()) {
        Error(7,STRING::EMPTY,0);
        return 1;
    }

    file.Read(&stackCount,4u);
    if (stackCount & static_cast<int>(0xFF000000u)) {
        const int result=LoadLGC(&filename);
        return result;
    }

    stack.Expand(128);
    stack.SetNo(stackCount);
    LoadVar(&file);

    file.Read(&size,4u);
    data=static_cast<unsigned char*>(::operator new(static_cast<unsigned int>(size)));
    if (!data) {
        Error(2,(char*)"data2",0);
        exit(1);
    }
    file.Read(data,static_cast<unsigned int>(size));

    for (int i=0;i<var.No();++i) {
        if (*var.Name(i)=="main")
            mainFunc=i;
    }
    return 0;
}

// Textual .lgc compiler front-end.  The parser itself is the next exact owner
// (LOGIC::func); this body intentionally preserves that dependency instead of
// hiding it behind a linker stub.
int LOGIC::LoadLGC(const STRING* scriptName)
{
    FSTREAM file(scriptName,"rb");
    Release();
    filename=scriptName;

    if (!file.IsOpen()) {
        Error(7,STRING::EMPTY,0);
        return 1;
    }

    const int iniSize=file.Length();
    data=static_cast<unsigned char*>(::operator new(256000u));
    if (!data) {
        Error(2,(char*)"data",0);
        exit(1);
    }

    ini=static_cast<char*>(::operator new(static_cast<unsigned int>(iniSize+4096)));
    if (!ini) {
        Error(2,(char*)"ini",0);
        exit(1);
    }

    pos=ini+4066;
    end=pos+iniSize;
    file.Read(pos,static_cast<unsigned int>(iniSize));

    stack.Expand(128);
    var.Expand(128);
    nline=0;
    while (!func()) {}

    MYERROR::Log(::Error,
        "LoadScript::ByteCode=%i varNo=%i DefineNo=%i stackNo=%i",
        size,var.m_no,defines.m_no,stack.m_no);
    defines.Release();

    if (size) {
        if (size>256000)
            Error(2,(char*)"byte code size",size);
        unsigned char* tmp=static_cast<unsigned char*>(::operator new(static_cast<unsigned int>(size)));
        if (!tmp) {
            Error(2,(char*)"tmp",0);
            exit(1);
        }
        memcpy(tmp,data,static_cast<unsigned int>(size));
        ::operator delete(data);
        data=tmp;
    } else {
        Release();
    }

    if (ini)
        ::operator delete(ini);
    ini=0;
    return 0;
}

// Adapted from the original-source family and checked against MapEdit debug metadata
// field layout. Native commands intentionally remain routed through
// ScriptExecFunc, the retail script_exec boundary.
int LOGIC::CallFunction(int n_func,const void* var1,const void* var2,int var3)
{
    int result=0;
    int arrayIndex=0;
    int indexActive=0;

    if (!data)
        return result;

    if (n_func<0)
        n_func=mainFunc;

    if (n_func>=var.No() || n_func<0) {
        MYERROR::Log(::Error,"!!!ERROR!!! SCRIPT Call unexisted function %i",n_func);
        return result;
    }
    if (var[n_func]->type!=3) {
        MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: Call unexisted function %s()",
            var.Name(n_func)->m_buf);
        return result;
    }

    const int initialStack=stack.No();
    PushInt(initialStack);
    PushInt(size);
    int frameBase=stack.No();

    int statementStart=var[n_func]->addr;
    int instruction=statementStart;

    const int parameterCount=var[n_func]->no_var;
    const int parameterBase=var[n_func]->begvar;
    if (parameterCount>=1) {
        LOGICSTACK value(var1);
        (*stack[parameterBase])=&value;
    }
    if (parameterCount>=2) {
        LOGICSTACK value(var2);
        (*stack[parameterBase+1])=&value;
    }
    if (parameterCount>=3) {
        LOGICSTACK value(var3);
        (*stack[parameterBase+2])=&value;
    }

    while (instruction<size) {
        if (stack.No()<frameBase) {
            StackError(const_cast<char*>("pop, but not push"),instruction);
            exit(1);
        }

        const unsigned int operation=data[instruction];

        if (operation>=6 && operation<=23) {
            const int binaryOperation=static_cast<int>(operation);
            ++instruction;
            stack[stack.No()-2]->BinarOperator(binaryOperation,stack.Last());
            stack.Pop();
        } else if (operation>=32 && operation<=39) {
            const int variable=*reinterpret_cast<int*>(data+instruction+1);

            if (variable<0 || variable>=stack.No())
                MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: Invalid stack pointer number %i",variable);

            int effective;
            if (stack[variable]->IsPointer() && indexActive)
                effective=stack[variable]->value+arrayIndex;
            else
                effective=variable+arrayIndex;

            if (effective<0 || effective>=stack.No()) {
                if (stack[variable]->IsPointer() && indexActive) {
                    MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: Invalid stack pointer to pointer number '%s'",
                        stack[variable]->name.m_buf);
                } else {
                    MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: Invalid stack pointer+array_index number %i+%i",
                        variable,arrayIndex);
                }
            }

            if (arrayIndex>=0) {
                int badIndex=0;
                if (stack[variable]->IsPointer()) {
                    badIndex=arrayIndex>=stack[stack[variable]->value]->no_array_element;
                } else {
                    badIndex=arrayIndex>=stack[variable]->no_array_element;
                }
                if (badIndex) {
                    MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: %s index [%i] error",
                        stack[variable]->name.m_buf,arrayIndex);
                }
            }

            if (operation!=38 && (stack[effective]->type & ST_UNINITIALIZED)) {
                MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: %s not initialized",
                    stack[variable]->name.m_buf);
            }

            ++instruction;
            switch(operation) {
            case 39:
                if (stack[variable]->IsString() && indexActive && !stack[variable]->IsArray()) {
                    MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: '%s' binar operator for string array",
                        stack[variable]->name.m_buf);
                }
                stack[effective]->BinarOperator(data[instruction+4],stack.Pop());
                stack.Push(stack[effective]);
                ++instruction;
                break;

            case 38:
                if (stack[variable]->IsString()) {
                    if (indexActive && !stack[variable]->IsArray()) {
                        if (arrayIndex>stack[variable]->string.Length()) {
                            MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: '%s' string index [%i] > string lentgh",
                                stack[variable]->name.m_buf,arrayIndex);
                        } else {
                            stack[variable]->string.SetChar(arrayIndex,stack.Last()->Int());
                        }
                    } else {
                        stack[effective]->string=*stack.Last()->String();
                    }
                } else {
                    stack[effective]->type=static_cast<uint8_t>(stack[effective]->type & 0xAF);
                    stack[effective]->value=stack.Last()->Int();
                    if (stack.Last()->IsObject())
                        stack[effective]->type=static_cast<uint8_t>(stack[effective]->type|ST_OBJECT);
                }
                break;

            case 36:
                if (!indexActive && stack[effective]->IsArray())
                    PushInt(*reinterpret_cast<int*>(data+instruction));
                else
                    stack.Push(stack[effective]);
                break;

            case 37:
                if (stack[effective]->IsString())
                    PushInt(reinterpret_cast<int>(&stack[effective]->string));
                else
                    PushInt(0);
                break;

            case 34:
                stack[effective]->Inc();
                stack.Push(stack[effective]);
                break;

            case 35:
                stack[effective]->Dec();
                stack.Push(stack[effective]);
                break;

            case 32:
                stack.Push(stack[effective]);
                stack[effective]->Inc();
                break;

            case 33:
                stack.Push(stack[effective]);
                stack[effective]->Dec();
                break;
            }

            indexActive=0;
            arrayIndex=0;
            instruction+=4;
        } else {
            ++instruction;
            switch(operation) {
            case 3:
                stack.Last()->SetInt(-stack.Last()->Int());
                break;
            case 4:
                stack.Last()->SetInt(~stack.Last()->Int());
                break;
            case 5:
                stack.Last()->SetInt(stack.Last()->Int()==0);
                break;
            case 1:
                PushInt(*reinterpret_cast<int*>(data+instruction));
                instruction+=4;
                break;
            case 2: {
                STRING text(reinterpret_cast<char*>(data+instruction));
                PushStr(&text);
                instruction+=static_cast<int>(strlen(reinterpret_cast<char*>(data+instruction)))+1;
                break;
            }
            case 26:
                stack.Pop();
                break;
            case 25:
                instruction+=4;
                statementStart=instruction;
                if (stack.No()-frameBase>1)
                    StackError(const_cast<char*>(";"),instruction);
                stack.SetNo(frameBase);
                break;
            case 24:
                if (PopInt())
                    instruction+=4;
                else
                    instruction+=*reinterpret_cast<int*>(data+instruction);
                if (stack.No()-frameBase>1)
                    StackError(const_cast<char*>("if"),instruction);
                statementStart=instruction;
                stack.SetNo(frameBase);
                break;
            case 28:
                instruction+=*reinterpret_cast<int*>(data+instruction);
                break;
            case 29:
                if (PopInt()) {
                    data[statementStart]=28;
                    *reinterpret_cast<int*>(data+statementStart+1)=
                        *reinterpret_cast<int*>(data+instruction)+instruction-statementStart-1;
                    instruction+=4;
                } else {
                    instruction+=*reinterpret_cast<int*>(data+instruction);
                }
                statementStart=instruction;
                if (stack.No()-frameBase>1)
                    StackError(const_cast<char*>("iff"),instruction);
                stack.SetNo(frameBase);
                break;
            case 30:
                PushInt(frameBase);
                PushInt(instruction+4);
                frameBase=stack.No();
                statementStart=*reinterpret_cast<int*>(data+instruction);
                instruction=statementStart;
                break;
            case 31:
                if (stack.No()-frameBase>1)
                    StackError(const_cast<char*>("return"),instruction);
                if (stack.No()>frameBase) {
                    LOGICSTACK returnValue(*stack.Pop());
                    stack.SetNo(frameBase);
                    instruction=PopInt();
                    statementStart=instruction;
                    frameBase=PopInt();
                    stack.Push(&returnValue);
                    if (instruction>=size && result==0)
                        result=returnValue.Int();
                } else {
                    stack.SetNo(frameBase);
                    instruction=PopInt();
                    statementStart=instruction;
                    frameBase=PopInt();
                }
                break;
            case 40:
                arrayIndex=PopInt();
                indexActive=1;
                break;
            default:
                if (data[instruction-1]==84) {
                    if (stack.Last()->Int()==reinterpret_cast<int>(var1))
                        result=1;
                }
                ScriptExecFunc(data[instruction-1]);
                break;
            }
        }
    }

    stack.SetNo(initialStack);
    return result;
}

STRING LOGIC::GetVariableStr(const STRING* name)
{
    STRING base=name->Before("[");
    const int location=var.Location(&base);
    if (location<0) {
        MYERROR::Log(::Error,"!!!ERROR!!! SCRIPT Can't find variable '%s' in GetVariableString",name->m_buf);
        return STRING(STRING::EMPTY);
    }

    const int index=name->After("[").Int();
    LOGICVAR* variable=var[location];
    if (index<variable->no_var)
        return STRING(stack[variable->begvar+index]->String());
    return STRING(STRING::EMPTY);
}

const NAMED_LIST<LOGICVAR>* LOGIC::Var()
{
    return &var;
}

int LOGIC::Save()
{
    const int no=stack.No();
    STRING path=filename.BeforeLast(".")+".lgd";
    FSTREAM file(&path,"wb");
    if (!file.IsOpen()) {
        Error(3,const_cast<char*>("file (.lgd)"),0);
        return 1;
    }

    file.Write(&no,4u);
    SaveVar(&file);
    file.Write(&size,4u);
    file.Write(data,static_cast<unsigned int>(size));
    return 0;
}

void LOGIC::SaveVar(STREAM* file)
{
    for (int i=0;i<stack.m_no;++i)
        stack.m_data[i].Write(file);

    file->Write(&var.m_no,4u);
    for (int i=0;i<var.m_no;++i) {
        var.m_data[i].name.Write(file);
        file->Write(&var.m_data[i].val,0x14u);
    }
    for (int i=0;i<var.m_no;++i)
        var.m_data[i].val.def_str.Write(file);
}
