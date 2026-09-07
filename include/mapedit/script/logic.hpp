#pragma once
// LOGIC owners. Included in ABI order by mapedit/runtime.hpp.
struct LOGICSTACK {
    uint8_t type;                 // +0x00
    uint8_t pad01[3];
    int value;                    // +0x04
    STRING string;                // +0x08
    STRING name;                  // +0x0C
    int no_array_element;         // +0x10

    LOGICSTACK();
    LOGICSTACK(const LOGICSTACK& that);
    LOGICSTACK(int val);
    LOGICSTACK(const STRING* str);
    LOGICSTACK(const void* object);
    ~LOGICSTACK();
    int IsArray();
    int IsInt();
    int IsString();
    int IsObject();
    int IsPointer();
    int IsInit();
    int IsStrings(const LOGICSTACK* r);
    int Int();
    const STRING* String();
    void Inc();
    void Dec();
    void InitInt(int val);
    void InitString(const STRING* str);
    void SetInt(int val);
    void BinarOperator(int operation,const LOGICSTACK* r);
    void Read(STREAM* file);
    void Write(STREAM* file);
    void Write(FILE* file);
    LOGICSTACK* operator=(const LOGICSTACK* r);
};
static_assert(sizeof(LOGICSTACK)==0x14, "MapEdit.exe LIST<LOGICSTACK> stride / debug metadata size");

struct LOGICVAR {
    uint8_t type;                 // +0x00
    uint8_t pad01[3];
    STRING def_str;               // +0x04
    union {                       // +0x08
        int addr;
        int code;
        int def_value;
        int n_var;
    };
    int begvar;                   // +0x0C
    int no_var;                   // +0x10

    LOGICVAR();
    LOGICVAR(int typ,int var);
    ~LOGICVAR();
    LOGICVAR* operator=(const LOGICVAR* r);
};
static_assert(sizeof(LOGICVAR)==0x14, "debug metadata LOGICVAR size");
static_assert(sizeof(NAMED_LIST_STRUCT<LOGICVAR>)==0x18, "debug metadata named LOGICVAR item size");
static_assert(sizeof(NAMED_LIST_STRUCT<STRING>)==0x08, "debug metadata named STRING item size");

class LOGIC {
private:
    LIST<LOGICSTACK> stack;       // +0x00
    NAMED_LIST<LOGICVAR> var;     // +0x10
    NAMED_LIST<STRING> defines;   // +0x20
    STRING filename;              // +0x30
    unsigned char* data;          // +0x34
    int size;                     // +0x38
    char* pos;                    // +0x3C
    char* end;                    // +0x40
    char* ini;                    // +0x44
    int nline;                    // +0x48
    int ifdef_no;                 // +0x4C
    int mainFunc;                 // +0x50
    int withoutDefine;            // +0x54

    void Error(int type,char* text,int err);
    void StackError(char* text,int err);
    int skipempty2();
    void skipempty();
    int Word(char* str);
    int WordEnd(char* str);
    int GetName(STRING* out);
    int GetInt();
    int GetString(char* out);
    void GetLine(STRING* out);
    void SetOperation(int operation,int value);
    void mnog();
    void slag();
    void cmpslag();
    void logicslag();
    void vyrag();
    void vyrag_oper();
    void oper(int* value);
    int func();
    void SetNoElement(int no);
    void IntVar();
    void StringVar();
public:
    LOGIC();
    ~LOGIC();
    int Load(const STRING* name);
    int LoadLGC(const STRING* name);
    int Save();
    void SaveVar(STREAM* file);
    void LoadVar(STREAM* file);
    int CallFunction(int n_func,const void* var1,const void* var2,int var3);
    int DeletePointerToObject(void* object);
    STRING GetVariableStr(const STRING* name);
    const NAMED_LIST<LOGICVAR>* Var();
    void PushInt(int value);
    void PushStr(const STRING* value);
    int PopInt();
    const STRING* PopStr();
    int PopObject();
    void PushObject(const void* object);
    int IsLastStackString();
    void Release();
};
static_assert(sizeof(LOGIC)==0x58, "debug metadata LOGIC size");
