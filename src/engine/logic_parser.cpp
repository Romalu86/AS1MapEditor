#include "mapedit/runtime.hpp"

namespace {
const unsigned char ST_ARRAY=0x04;
const unsigned char ST_INIT=0x08;
const unsigned char ST_POINTER=0x20;
}


int LOGIC::skipempty2()
{
    int comment=0;
    int skipDepth=0;
    char token[4096];

    while (pos<end) {
        if (!comment) {
            if ((isalpha((unsigned char)*pos) || *pos=='_') && !withoutDefine) {
                int len=0;
                while (isalnum((unsigned char)pos[len]) || pos[len]=='_') {
                    if (len>=4095) { Error(10,(char*)"Very long name",0); exit(1); }
                    token[len]=pos[len]; ++len;
                }
                token[len]=0;
                STRING key(token);
                int d=defines.Location(&key);
                if (d>=0) {
                    const char* replacement=defines.m_data[d].val.m_buf;
                    int replacementLen=(int)strlen(replacement);
                    pos+=len-replacementLen;
                    memcpy(pos,replacement,(unsigned int)replacementLen);
                }
            }

            if (*pos=='#' && pos[1]=='i' && pos[2]=='f' && pos[3]=='d' && pos[4]=='e' && pos[5]=='f') {
                pos+=6; ++ifdef_no;
                if (!skipDepth) {
                    STRING name; withoutDefine=1; GetName(&name); withoutDefine=0;
                    if (var.Location(&name)<0 && defines.Location(&name)<0) skipDepth=ifdef_no;
                }
            } else if (*pos=='#' && pos[1]=='i' && pos[2]=='f' && pos[3]=='n' && pos[4]=='d' && pos[5]=='e' && pos[6]=='f') {
                pos+=7; ++ifdef_no;
                if (!skipDepth) {
                    STRING name; withoutDefine=1; GetName(&name); withoutDefine=0;
                    if (var.Location(&name)>=0 || defines.Location(&name)>=0) skipDepth=ifdef_no;
                }
            } else if (*pos=='#' && pos[1]=='e' && pos[2]=='n' && pos[3]=='d' && pos[4]=='i' && pos[5]=='f') {
                pos+=6;
                if (skipDepth==ifdef_no) skipDepth=0;
                --ifdef_no;
                if (ifdef_no<0) Error(10,(char*)"#endif without #ifdef",0);
            } else if (*pos=='#' && pos[1]=='e' && pos[2]=='l' && pos[3]=='s' && pos[4]=='e') {
                pos+=5;
                if (!skipDepth && ifdef_no>0) skipDepth=ifdef_no;
                else if (skipDepth==ifdef_no) skipDepth=0;
                if (ifdef_no<=0) Error(10,(char*)"#else without #ifdef",0);
            }

            if (*pos=='/' && pos[1]=='/') comment=1;
            else if (*pos=='/' && pos[1]=='*') comment=2;
            else {
                if (*pos=='?') { Error(10,(char*)"?: not supported in this version",0); exit(1); }
                if (!skipDepth && !isspace((unsigned char)*pos) && *pos) return 0;
            }
        } else if ((comment==1 && *pos=='\n') || (comment==2 && *pos=='/' && pos[-1]=='*')) {
            comment=0;
        }

        char c=*pos++;
        if (c=='\n') ++nline;
    }

    if (ifdef_no>0) Error(10,(char*)"#ifdef without #endif",ifdef_no);
    return 1;
}

void LOGIC::skipempty()
{
    if (skipempty2()) { Error(10,(char*)"End of file",0); exit(1); }
}

void LOGIC::GetLine(STRING* out)
{
    skipempty();
    char buf[4096];
    int n=0;
    while (*pos!='\n' && *pos!='\r') {
        if (n>=4095) { Error(10,(char*)"Very long line",0); exit(1); }
        buf[n++]=*pos++;
    }
    buf[n]=0;
    *out=buf;
    if (!buf[0]) { Error(10,(char*)"empty line",0); exit(1); }
    *out=out->Before("//");
    out->RemoveEndChars(" \n\r\t");
    skipempty();
}

int LOGIC::GetName(STRING* out)
{
    skipempty();
    char buf[4096];
    int n=0;
    while (isalnum((unsigned char)*pos) || *pos=='_') {
        if (n>=4095) { Error(10,(char*)"Very long name",0); exit(1); }
        buf[n++]=*pos++;
    }
    buf[n]=0;
    *out=buf;
    if (!buf[0]) { Error(4,(char*)"name",0); exit(1); }
    skipempty();
    return n;
}

int LOGIC::Word(char* word)
{
    int len=(int)strlen(word);
    skipempty();
    if (strncmp(pos,word,(unsigned int)len) ||
        ((isalpha((unsigned char)*word) || *word=='#') &&
         (isalnum((unsigned char)pos[len]) || pos[len]=='_')))
        return 0;
    pos+=len;
    skipempty2();
    return 1;
}

int LOGIC::WordEnd(char* word)
{
    if (Word(word)) return 1;
    Error(13,word,0);
    exit(1);
    return 0;
}

int LOGIC::GetInt()
{
    skipempty();
    int old=size;
    vyrag();
    if (data[old]!=1 || size-old!=5) {
        Error(4,(char*)"constant int value",0); exit(1);
    }
    size-=5;
    return *reinterpret_cast<int*>(data+size+1);
}

int LOGIC::GetString(char* out)
{
    char* start=out;
    if (*pos=='"') {
        ++pos;
        while (*pos!='"' && pos<end) {
            if (*pos=='\\') {
                if (pos[1]=='\r' && pos[2]=='\n') { ++nline; pos+=2; }
                else if (pos[1]=='\n') { ++nline; ++pos; }
                else if (isdigit((unsigned char)pos[1]) && isdigit((unsigned char)pos[2]) && isdigit((unsigned char)pos[3])) {
                    *out++=(char)(((pos[1]-'0')*8+pos[2]-'0')*8+pos[3]-'0'); pos+=3;
                } else {
                    char e=pos[1];
                    if (e=='n') { *out++='\n'; ++pos; }
                    else if (e=='r') { *out++='\r'; ++pos; }
                    else if (e=='t') { *out++='\t'; ++pos; }
                    else if (e=='\'') { *out++='\''; ++pos; }
                    else if (!e) { *out++=0; ++pos; }
                    else if (e=='"') { *out++='"'; ++pos; }
                    else { ++pos; *out++=*pos; }
                }
            } else *out++=*pos;
            ++pos;
        }
        char* close=pos++;
        if (close>=end) { Error(10,(char*)"End of file",0); exit(1); }
        *out++=0;
    }
    return (int)(out-start);
}

void LOGIC::SetNoElement(int no)
{
    LOGICVAR* last=var.Last();
    stack[last->addr]->no_array_element=no;

    STRING sourceName=Printf("'%s' line %i '%s'",
        filename.CharPtr(),nline+1,var.Name(var.No()-1)->CharPtr());
    stack[var.Last()->addr]->name=&sourceName;

    var.Last()->no_var=no;
}

void LOGIC::IntVar()
{
    int count=0;
    int flags=0;
    STRING name;
    skipempty();
    if (*pos=='*') { flags=ST_POINTER; ++pos; }
    GetName(&name);
    if (var.Location(&name)>=0) { STRING e=Printf("int redefinition '%s'",name.m_buf); Error(10,e.m_buf,0); exit(1); }
    LOGICVAR lv(1,stack.m_no); var.Insert(name,lv);

    if (Word((char*)"[")) {
        flags|=ST_ARRAY;
        if (*pos==']') {
            ++pos;
            if (Word((char*)"=")) {
                WordEnd((char*)"{");
                do {
                    LOGICSTACK z(0); stack.Push(&z);
                    stack.m_data[stack.m_no-1].type=(uint8_t)(stack.m_data[stack.m_no-1].type|flags);
                    stack.m_data[stack.m_no-1].value=GetInt();
                    stack.m_data[stack.m_no-1].type=(uint8_t)(stack.m_data[stack.m_no-1].type|ST_INIT);
                    ++count;
                } while (Word((char*)","));
                WordEnd((char*)"}"); SetNoElement(count); return;
            }
            Error(10,(char*)"for [] need initialisation",0); exit(1);
        }
        count=GetInt(); WordEnd((char*)"]");
    } else count=1;

    for (int i=0;i<count;++i) {
        LOGICSTACK z(0); stack.Push(&z);
        stack.m_data[stack.m_no-1].type=(uint8_t)(stack.m_data[stack.m_no-1].type|flags|0x40);
    }
    if (Word((char*)"=")) {
        if (flags & ST_ARRAY) {
            WordEnd((char*)"{"); int init=0;
            do {
                if (init>=count) { Error(10,(char*)"too many initializers",0); exit(1); }
                LOGICSTACK& v=stack.m_data[stack.m_no+init-count]; ++init;
                v.type=(uint8_t)(v.type & ~0x40); v.value=GetInt(); v.type=(uint8_t)(v.type|ST_INIT);
            } while (Word((char*)","));
            WordEnd((char*)"}");
        } else {
            LOGICSTACK& v=stack.m_data[stack.m_no-1]; v.value=GetInt(); v.type=(uint8_t)(v.type|ST_INIT); v.type=(uint8_t)(v.type&~0x40);
        }
    }
    SetNoElement(count);
}

void LOGIC::StringVar()
{
    int flags=0;
    STRING name; GetName(&name);
    if (var.Location(&name)>=0) { STRING e=Printf("string redifinition '%s'",name.m_buf); Error(10,e.m_buf,0); exit(1); }
    int count;
    if (Word((char*)"[")) { flags=ST_ARRAY; count=GetInt(); WordEnd((char*)"]"); }
    else count=1;
    LOGICVAR lv(1,stack.m_no); var.Insert(name,lv);
    for (int i=0;i<count;++i) {
        STRING empty(STRING::EMPTY); LOGICSTACK z(&empty); stack.Push(&z);
        stack.m_data[stack.m_no-1].type=(uint8_t)(stack.m_data[stack.m_no-1].type|flags);
    }
    if (Word((char*)"=")) {
        char buf[4096]; GetString(buf);
        LOGICSTACK& v=stack.m_data[stack.m_no-1]; v.string=buf; v.type=(uint8_t)(v.type|ST_INIT); v.type=(uint8_t)(v.type&~0x40);
    }
    SetNoElement(count);
}

void LOGIC::mnog()
{
    STRING name;
    int operation;
    int unary;
    unsigned int indexCodeSize;
    int variable;
    bool done=false;

    while (!done) {
        unary=0;
        operation=36;
        if (pos[1]!='-' && Word((char*)"-")) unary=3;
        else if (Word((char*)"~")) unary=4;
        else if (Word((char*)"!")) unary=5;

        if (Word((char*)"--")) operation=35;
        else if (Word((char*)"++")) operation=34;
        else if (Word((char*)"&")) operation=37;

        if (isdigit((unsigned char)*pos)) {
            GetName(&name);
            sscanf(name.m_buf,"%i",&operation);
            if (unary==3) { unary=0; operation=-operation; }
            else if (unary==4) { unary=0; operation=~operation; }
            else if (unary==5) { unary=0; operation=operation==0; }
            data[size++]=1;
            *reinterpret_cast<int*>(data+size)=operation; size+=4;
            done=true;
        } else if (*pos=='"') {
            data[size++]=2;
            size+=GetString(reinterpret_cast<char*>(data+size));
            done=true;
        } else if (*pos=='\'') {
            data[size++]=1; ++pos;
            *reinterpret_cast<int*>(data+size)=*pos; size+=4; ++pos;
            if (*pos!='\'') { Error(13,(char*)"second '",0); exit(1); }
            ++pos; done=true;
        } else if (Word((char*)"sizeof")) {
            if (!Word((char*)"(")) { Error(13,(char*)"'(' for sizeof",0); exit(1); }
            data[size++]=1;
            if (Word((char*)"int") || Word((char*)"string"))
                *reinterpret_cast<int*>(data+size)=4;
            else {
                GetName(&name); variable=var.Location(&name);
                if (variable<0 || var.m_data[variable].val.type!=1) { Error(4,(char*)"sizeof parameter",0); exit(1); }
                *reinterpret_cast<int*>(data+size)=4*var.m_data[variable].val.no_var;
            }
            size+=4; WordEnd((char*)")"); done=true;
        } else if (Word((char*)"static")) {
            if (Word((char*)"int")) { do IntVar(); while (Word((char*)",")); }
            else {
                if (!Word((char*)"string")) { Error(4,(char*)"static variable",0); exit(1); }
                do StringVar(); while (Word((char*)","));
            }
            done=true;
        } else if (Word((char*)"int")) {
            do IntVar(); while (Word((char*)",")); done=true;
        } else if (Word((char*)"string")) {
            do StringVar(); while (Word((char*)",")); done=true;
        } else if (Word((char*)"return")) {
            vyrag(); data[size++]=31; done=true;
        } else if (Word((char*)"(")) {
            vyrag(); WordEnd((char*)")"); done=true;
        } else if (isalpha((unsigned char)*pos)) {
            GetName(&name);
            variable=var.Location(&name);
            if (variable>=0) {
                NAMED_LIST_STRUCT<LOGICVAR>& entry=var.m_data[variable];
                switch(entry.val.type) {
                case 1: {
                    indexCodeSize=0;
                    unsigned char* indexCode=0;
                    if (Word((char*)"[")) {
                        int indexStart=size;
                        LOGICSTACK* item=&stack.m_data[entry.val.addr];
                        if (!(item->type & 0x25)) { Error(10,(char*)"[] for not array",0); exit(1); }
                        vyrag(); data[size++]=40;
                        indexCodeSize=(unsigned int)(size-indexStart);
                        indexCode=static_cast<unsigned char*>(::operator new(indexCodeSize));
                        if (!indexCode) { Error(2,name.m_buf,0); exit(1); }
                        size=indexStart;
                        memcpy(indexCode,data+indexStart,indexCodeSize);
                        WordEnd((char*)"]");
                    }

                    int command;
                    if (pos[1]=='=' || !Word((char*)"=")) {
                        if (Word((char*)"+=")) { vyrag(); command=39; operation=8; }
                        else if (Word((char*)"-=")) { vyrag(); command=39; operation=9; }
                        else if (Word((char*)"/=")) { vyrag(); command=39; operation=6; }
                        else if (Word((char*)"*=")) { vyrag(); command=39; operation=19; }
                        else if (Word((char*)"%=")) { vyrag(); command=39; operation=7; }
                        else if (Word((char*)"&=")) { vyrag(); command=39; operation=12; }
                        else if (Word((char*)"|=")) { vyrag(); command=39; operation=11; }
                        else if (Word((char*)"^=")) { vyrag(); command=39; operation=10; }
                        else if (Word((char*)"<<=")) { vyrag(); command=39; operation=23; }
                        else if (Word((char*)">>=")) { vyrag(); command=39; operation=22; }
                        else if (Word((char*)"++")) command=32;
                        else if (Word((char*)"--")) command=33;
                        else command=operation;
                    } else { vyrag(); command=38; }

                    if (indexCodeSize) {
                        memcpy(data+size,indexCode,indexCodeSize); size+=(int)indexCodeSize; ::operator delete(indexCode);
                    } else {
                        LOGICSTACK* item=&stack.m_data[entry.val.addr];
                        if (item->type & 0x24) {
                            if (command==32 || command==33 || command==34 || command==35) {
                                Error(10,(char*)"Increment or decrement for array",0); exit(1);
                            }
                            if (command!=36) { Error(4,(char*)"operation for array",command); exit(1); }
                        }
                    }
                    data[size++]=(unsigned char)command;
                    *reinterpret_cast<int*>(data+size)=entry.val.addr; size+=4;
                    if (command==39) data[size++]=(unsigned char)operation;
                    done=true;
                    break;
                }
                case 2: {
                    WordEnd((char*)"("); int count=0;
                    while (!Word((char*)")")) { vyrag(); Word((char*)","); ++count; }
                    while (count<entry.val.no_var) {
                        int argument=count+entry.val.begvar;
                        LOGICSTACK* item=&stack.m_data[argument];
                        if (!(item->type & ST_INIT)) break;
                        data[size++]=36; *reinterpret_cast<int*>(data+size)=argument; size+=4; ++count;
                    }
                    if (count!=entry.val.no_var) { Error(4,(char*)"extern function parameters number",0); exit(1); }
                    data[size++]=(unsigned char)entry.val.addr;
                    done=true; break;
                }
                case 3: {
                    WordEnd((char*)"("); int count=0;
                    while (!Word((char*)")")) {
                        vyrag(); Word((char*)",");
                        data[size++]=38; *reinterpret_cast<int*>(data+size)=count+entry.val.begvar; size+=4;
                        data[size++]=26; ++count;
                    }
                    while (count<entry.val.no_var) {
                        int argument=count+entry.val.begvar;
                        LOGICSTACK* item=&stack.m_data[argument];
                        if (!(item->type & ST_INIT)) break;
                        if (item->type & 1) {
                            data[size++]=2;
                            unsigned int len=strlen(item->string.m_buf)+1;
                            memcpy(data+size,item->string.m_buf,len); size+=(int)len;
                        } else {
                            data[size++]=1; *reinterpret_cast<int*>(data+size)=item->value; size+=4;
                        }
                        data[size++]=38; *reinterpret_cast<int*>(data+size)=argument; size+=4;
                        data[size++]=26; ++count;
                    }
                    if (count!=entry.val.no_var) { Error(4,(char*)"function parameters number",0); exit(1); }
                    data[size++]=30; *reinterpret_cast<int*>(data+size)=entry.val.addr; size+=4;
                    done=true; break;
                }
                case 4: {
                    data[size++]=2;
                    unsigned int len=strlen(entry.val.def_str.m_buf)+1;
                    memcpy(data+size,entry.val.def_str.m_buf,len); size+=(int)len;
                    done=true; break;
                }
                case 5:
                    data[size++]=1; *reinterpret_cast<int*>(data+size)=entry.val.addr; size+=4; done=true; break;
                case 7: {
                    STRING e=Printf(*pos==':' ? "Label redefinition '%s'" : "Incorrect use label '%s'",name.m_buf);
                    Error(10,e.m_buf,0); exit(1); break;
                }
                case 8:
                    if (*pos!=':') { STRING e=Printf("Incorrect use label '%s'",name.m_buf); Error(10,e.m_buf,0); exit(1); }
                    ++pos; entry.val.type=7;
                    data[entry.val.addr]=(unsigned char)(size-entry.val.addr);
                    entry.val.addr=size;
                    break;
                default:
                    done=true; break;
                }
            } else {
                if (*pos!=':') { STRING e=Printf("Undeclared identifier '%s'",name.m_buf); Error(10,e.m_buf,0); exit(1); }
                ++pos; LOGICVAR lv(7,size); var.Insert(name,lv);
            }
        } else {
            if (unary) { Error(10,(char*)"error symbol",0); exit(1); }
            if (strchr(".$#@`",*pos)) { Error(10,(char*)"error symbol",0); exit(1); }
            done=true;
        }
        if (done && unary) data[size++]=(unsigned char)unary;
    }
}

void LOGIC::SetOperation(int check_size,int operation)
{
    if (data[check_size]==1 && data[check_size+5]==1 && size-check_size==10) {
        int& a=*reinterpret_cast<int*>(data+check_size+1);
        int b=*reinterpret_cast<int*>(data+check_size+6);
        switch(operation) {
        case 19: a*=b; break; case 6: a/=b; break; case 7: a%=b; break;
        case 8: a+=b; break; case 9: a-=b; break; case 22: a>>=b; break;
        case 23: a<<=b; break; case 10: a^=b; break; case 12: a&=b; break;
        case 11: a|=b; break; default: break;
        }
        size-=5;
    } else data[size++]=(unsigned char)operation;
}

void LOGIC::slag()
{
    int p=size; mnog();
    for (;;) {
        if (Word((char*)"*")) { mnog(); SetOperation(p,19); }
        else if (Word((char*)"/")) { mnog(); SetOperation(p,6); }
        else if (Word((char*)"%")) { mnog(); SetOperation(p,7); }
        else break;
    }
}

void LOGIC::cmpslag()
{
    int p=size; slag();
    for (;;) {
        if (Word((char*)"+")) { slag(); SetOperation(p,8); }
        else if (Word((char*)"-")) { slag(); SetOperation(p,9); }
        else break;
    }
}

void LOGIC::logicslag()
{
    int p=size; cmpslag();
    for (;;) {
        if (Word((char*)">=")) { cmpslag(); data[size++]=0x11; }
        else if (Word((char*)">>")) { cmpslag(); SetOperation(p,0x16); }
        else if (Word((char*)">")) { cmpslag(); data[size++]=0x0f; }
        else if (Word((char*)"<=")) { cmpslag(); data[size++]=0x12; }
        else if (Word((char*)"<<")) { cmpslag(); SetOperation(p,0x17); }
        else if (Word((char*)"<")) { cmpslag(); data[size++]=0x10; }
        else if (Word((char*)"==")) { cmpslag(); data[size++]=0x0d; }
        else if (Word((char*)"!=")) { cmpslag(); data[size++]=0x14; }
        else break;
    }
}

void LOGIC::vyrag()
{
    int p=size; logicslag();
    for (;;) {
        if (Word((char*)"^")) { logicslag(); SetOperation(p,10); }
        else if (Word((char*)"&&")) { logicslag(); data[size++]=0x15; }
        else if (Word((char*)"&")) { logicslag(); SetOperation(p,12); }
        else if (Word((char*)"||")) { logicslag(); data[size++]=0x0e; }
        else if (Word((char*)"|")) { logicslag(); SetOperation(p,11); }
        else break;
    }
}

void LOGIC::vyrag_oper()
{
    int again;
    do {
        vyrag(); data[size++]=25; *reinterpret_cast<int*>(data+size)=nline; size+=4;
        again=Word((char*)",");
    } while (again);
}


void LOGIC::oper(int* breakFixups)
{
    int inverted=Word((char*)"iff");
    if (inverted || Word((char*)"if")) {
        WordEnd((char*)"(");
        vyrag();
        WordEnd((char*)")");
        data[size++]=(unsigned char)(inverted ? 29 : 24);
        int branch=size;
        size+=4;
        oper(breakFixups);
        *reinterpret_cast<int*>(data+branch)=size-branch;
        if (Word((char*)"else")) {
            *reinterpret_cast<int*>(data+branch)+=5;
            data[size++]=28;
            int endBranch=size;
            size+=4;
            oper(breakFixups);
            *reinterpret_cast<int*>(data+endBranch)=size-endBranch;
        }
    }
    else if (Word((char*)"while")) {
        int localBreaks[128]={0};
        WordEnd((char*)"(");
        int loopStart=size;
        vyrag();
        WordEnd((char*)")");
        data[size++]=24;
        int loopEnd=size;
        size+=4;
        oper(localBreaks);
        data[size++]=28;
        int loopBack=size;
        *reinterpret_cast<int*>(data+loopBack)=loopStart-loopBack;
        size+=4;
        *reinterpret_cast<int*>(data+loopEnd)=size-loopEnd;
        for (int* p=localBreaks; *p; ++p)
            *reinterpret_cast<int*>(data+*p)=size-*p;
    }
    else if (Word((char*)"do")) {
        int localBreaks[128]={0};
        int loopStart=size;
        oper(localBreaks);
        WordEnd((char*)"while");
        WordEnd((char*)"(");
        vyrag();
        WordEnd((char*)")");
        data[size++]=5;
        data[size++]=24;
        int loopBack=size;
        *reinterpret_cast<int*>(data+loopBack)=loopStart-loopBack;
        size+=4;
        for (int* p=localBreaks; *p; ++p)
            *reinterpret_cast<int*>(data+*p)=size-*p;
    }
    else if (Word((char*)"for")) {
        int localBreaks[128]={0};
        WordEnd((char*)"(");
        vyrag_oper();
        WordEnd((char*)";");
        int condition=size;
        vyrag();
        WordEnd((char*)";");
        data[size++]=24;
        int loopEnd=size;
        size+=4;
        data[size++]=28;
        int bodyBranch=size;
        int increment=size+4;
        size+=4;
        vyrag_oper();
        WordEnd((char*)")");
        data[size++]=28;
        int conditionBack=size;
        *reinterpret_cast<int*>(data+conditionBack)=condition-conditionBack;
        size+=4;
        *reinterpret_cast<int*>(data+bodyBranch)=size-bodyBranch;
        oper(localBreaks);
        data[size++]=28;
        int incrementBack=size;
        *reinterpret_cast<int*>(data+incrementBack)=increment-incrementBack;
        size+=4;
        *reinterpret_cast<int*>(data+loopEnd)=size-loopEnd;
        for (int* p=localBreaks; *p; ++p)
            *reinterpret_cast<int*>(data+*p)=size-*p;
    }
    else if (Word((char*)"break")) {
        WordEnd((char*)";");
        if (!breakFixups) {
            Error(10,(char*)"'break' without loop",0);
            exit(1);
        }
        int count=0;
        while (breakFixups[count]) {
            if (count>=127) {
                Error(10,(char*)"Too many 'break'",0);
                exit(1);
            }
            ++count;
        }
        data[size++]=28;
        breakFixups[count]=size;
        size+=4;
        breakFixups[count+1]=0;
    }
    else if (Word((char*)"goto")) {
        STRING name;
        GetName(&name);
        int label=var.Location(&name);
        if (label<0) {
            LOGICVAR lv(8,size+1);
            var.Insert(name,lv);
            label=var.m_no-1;
        } else {
            if (var.m_data[label].val.type==8) {
                STRING e=Printf("second use undefined label '%s'",name.m_buf);
                Error(10,e.m_buf,0);
                exit(1);
            }
            if (var.m_data[label].val.type!=7) {
                STRING e=Printf("'%s' is not label",name.m_buf);
                Error(10,e.m_buf,0);
                exit(1);
            }
        }
        data[size++]=28;
        *reinterpret_cast<int*>(data+size)=var.m_data[label].val.addr-size;
        size+=4;
    }
    else if (Word((char*)"{")) {
        int variableCount=var.m_no;
        while (!Word((char*)"}"))
            oper(breakFixups);
        if (variableCount<=0) {
            var.m_max=0;
            var.m_no=0;
            delete[] var.m_data;
            var.m_data=0;
        } else if (variableCount<var.m_no) {
            var.m_no=variableCount;
        }
    }
    else {
        vyrag_oper();
        WordEnd((char*)";");
    }
}

int LOGIC::func()
{
    STRING name;
    char fileName[1024];

    if (skipempty2())
        return 1;

    if (*pos=='#' && pos[1]=='d' && pos[2]=='e' && pos[3]=='f' &&
        pos[4]=='i' && pos[5]=='n' && pos[6]=='e') {
        pos+=7;
        withoutDefine=1;
        GetName(&name);
        withoutDefine=0;
        STRING value;
        GetLine(&value);
        int define=defines.Location(&name);
        if (define<0)
            defines.Insert(name,value);
        else
            defines.m_data[define].val=value;
    }
    else if (*pos=='#' && pos[1]=='u' && pos[2]=='n' && pos[3]=='d' &&
             pos[4]=='e' && pos[5]=='f') {
        pos+=6;
        withoutDefine=1;
        GetName(&name);
        withoutDefine=0;
        int define=defines.Location(&name);
        if (define<0) {
            Error(4,(char*)"#undef parameters",0);
            exit(1);
        }
        if (define>=0 && define<defines.m_no) {
            --defines.m_no;
            for (int i=define;i<defines.m_no;++i)
                defines.m_data[i]=defines.m_data[i+1];
        }
        if (!defines.m_no) {
            defines.m_max=0;
            delete[] defines.m_data;
            defines.m_data=0;
        }
    }
    else if (Word((char*)"#include")) {
        STRING oldName(filename);
        if (*pos!='\"' && *pos!='<') {
            Error(13,(char*)"include file name",0);
            exit(1);
        }
        ++pos;
        int length=0;
        while (*pos!='\"' && *pos!='>') {
            if (pos>=end) {
                Error(10,(char*)"End of file",0);
                exit(1);
            }
            if (length>=1023) {
                Error(10,(char*)"Very long name",0);
                exit(1);
            }
            fileName[length++]=*pos++;
        }
        fileName[length]=0;
        ++pos;

        void* file=fopen(fileName,"rb");
        if (!file) {
            Error(7,fileName,0);
            exit(1);
        }
        int includeSize=(int)_filelength(_fileno(file));
        char* oldPos=pos;
        char* oldEnd=end;
        char* oldBuffer=ini;
        int oldLine=nline;
        int oldConditionalDepth=ifdef_no;

        ini=new char[includeSize+4096];
        if (!ini) {
            Error(2,(char*)"include",0);
            exit(1);
        }
        pos=ini+4066;
        end=pos+includeSize;
        fread(pos,(unsigned int)includeSize,1,file);
        nline=0;
        filename=fileName;
        ifdef_no=0;
        while (!func()) {}

        fclose(file);
        delete[] ini;
        pos=oldPos;
        end=oldEnd;
        ini=oldBuffer;
        nline=oldLine;
        ifdef_no=oldConditionalDepth;
        filename=oldName;
    }
    else if (Word((char*)"extern")) {
        int variableCount=var.m_no;
        GetName(&name);
        if (var.Location(&name)>=0) {
            Error(10,(char*)"function redefinition",0);
            exit(1);
        }
        int stackStart=stack.m_no;
        WordEnd((char*)"(");
        do {
            if (Word((char*)"int")) IntVar();
            else if (Word((char*)"string")) StringVar();
        } while (Word((char*)","));
        WordEnd((char*)")");
        int parameterCount=stack.m_no-stackStart;
        int code=GetInt();
        if (!isdigit((unsigned char)pos[-1])) {
            Error(13,(char*)"extern function code",0);
            exit(1);
        }
        if (variableCount<=0) {
            var.m_max=0;
            var.m_no=0;
            delete[] var.m_data;
            var.m_data=0;
        } else if (variableCount<var.m_no) {
            var.m_no=variableCount;
        }
        LOGICVAR function(2,code);
        function.begvar=stackStart;
        function.no_var=parameterCount;
        var.Insert(name,function);
        WordEnd((char*)";");
    }
    else if (Word((char*)"static")) {
        if (Word((char*)"int")) {
            do IntVar(); while (Word((char*)","));
            WordEnd((char*)";");
        }
        else if (Word((char*)"string")) {
            do StringVar(); while (Word((char*)","));
            WordEnd((char*)";");
        }
        else {
            Error(4,(char*)"static variable",0);
            exit(1);
        }
    }
    else if (Word((char*)"int")) {
        do IntVar(); while (Word((char*)","));
        WordEnd((char*)";");
    }
    else if (Word((char*)"string")) {
        do StringVar(); while (Word((char*)","));
        WordEnd((char*)";");
    }
    else {
        int variableCount=var.m_no;
        GetName(&name);
        if (var.Location(&name)>=0) {
            Error(10,(char*)"function redefinition",0);
            exit(1);
        }
        int stackStart=stack.m_no;
        int code=size;
        WordEnd((char*)"(");
        do {
            if (Word((char*)"int")) IntVar();
            else if (Word((char*)"string")) StringVar();
        } while (Word((char*)","));
        WordEnd((char*)")");
        int parameterCount=stack.m_no-stackStart;
        WordEnd((char*)"{");
        while (!Word((char*)"}"))
            oper(0);
        data[size++]=31;

        if (variableCount<=0) {
            var.m_max=0;
            var.m_no=0;
            delete[] var.m_data;
            var.m_data=0;
        } else if (variableCount<var.m_no) {
            var.m_no=variableCount;
        }
        LOGICVAR function(3,code);
        function.begvar=stackStart;
        function.no_var=parameterCount;
        var.Insert(name,function);
        if (!strcmp(name.m_buf,"main"))
            mainFunc=var.m_no-1;
    }

    return skipempty2();
}
