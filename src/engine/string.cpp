#include "mapedit/runtime.hpp"

// MapEdit string storage uses 16-byte allocation granularity.
STRING::STRING(const char* text)
{
    if (text && *text)
        Copy(text, static_cast<int>(strlen(text)));
    else
        m_buf = EMPTY;
}


STRING::STRING(const char* str1,const char* str2)
{
    const unsigned int left=strlen(str1);
    const unsigned int right=strlen(str2);
    m_buf=static_cast<char*>(::operator new(((left+right)&~0x0Fu)+0x10u));
    memcpy(m_buf,str1,left);
    memcpy(m_buf+left,str2,right+1u);
}

STRING::STRING(const STRING* other)
{
    if (other->m_buf[0])
        Copy(other->m_buf, static_cast<int>(strlen(other->m_buf)));
    else
        m_buf = EMPTY;
}

void STRING::Copy(const char* text, int length)
{
    const unsigned int bytes = (static_cast<unsigned int>(length) & ~0x0Fu) + 0x10u;
    m_buf = static_cast<char*>(::operator new(bytes));
    memcpy(m_buf, text, static_cast<unsigned int>(length));
    m_buf[length] = 0;
}

char* STRING::CharPtr()
{
    return m_buf;
}

int STRING::operator==(const char* text)
{
    return strcmp(m_buf, text) == 0;
}

char STRING::EMPTY[16] = {0};

STRING::STRING() : m_buf(EMPTY) {}

STRING::~STRING()
{
    if (m_buf != EMPTY)
        ::operator delete(m_buf);
}

int STRING::Replace(const STRING* source,const STRING* dest)
{
    return Replace(source->m_buf,dest->m_buf);
}

STRING STRING::ToLower()
{
    STRING result(m_buf);
    _strlwr(result.m_buf);
    return result;
}

// MapEdit copy construction has the same storage rules as the pointer overload.
STRING::STRING(const STRING& other)
{
    if (other.m_buf[0])
        Copy(other.m_buf, static_cast<int>(strlen(other.m_buf)));
    else
        m_buf = EMPTY;
}


void STRING::SetChar(int index,int c)
{
    m_buf[index]=static_cast<char>(c);
}

int STRING::Length()
{
    return static_cast<int>(strlen(m_buf));
}

int STRING::HaveChar(int chr)
{
    return strchr(m_buf,chr)!=0;
}

int STRING::WriteToBuf(void* buffer)
{
    const int size=Length()+1;
    memcpy(buffer,m_buf,static_cast<unsigned int>(size));
    return size;
}

char STRING::LastChar()
{
    return m_buf[0] ? m_buf[strlen(m_buf)-1] : 0;
}

// reuses a buffer when its 16-byte allocation class is unchanged.
const STRING* STRING::operator=(const STRING& other)
{
    if (this == &other)
        return this;

    const int newLength = static_cast<int>(strlen(other.m_buf));
    if (!newLength) {
        if (m_buf != EMPTY)
            ::operator delete(m_buf);
        m_buf = EMPTY;
        return this;
    }

    const unsigned int newBytes = (static_cast<unsigned int>(newLength) & ~0x0Fu) + 0x10u;
    unsigned int oldBytes = 0;
    if (m_buf != EMPTY) {
        const unsigned int oldLength = strlen(m_buf);
        oldBytes = (oldLength & ~0x0Fu) + 0x10u;
    }
    if (m_buf == EMPTY || oldBytes != newBytes) {
        if (m_buf != EMPTY)
            ::operator delete(m_buf);
        m_buf = static_cast<char*>(::operator new(newBytes));
    }
    memcpy(m_buf, other.m_buf, static_cast<unsigned int>(newLength + 1));
    return this;
}

int STRING::Replace(const char* source,const char* dest)
{
    if (!source)
        return 0;
    if (!dest)
        dest = EMPTY;

    const unsigned int sourceLength = strlen(source);
    char* found = strstr(m_buf, source);
    if (!found)
        return 0;

    const unsigned int destLength = strlen(dest);
    const unsigned int oldLength = strlen(m_buf);
    const unsigned int prefixLength = static_cast<unsigned int>(found - m_buf);
    const unsigned int suffixLength = oldLength - prefixLength - sourceLength;
    const unsigned int newLength = prefixLength + destLength + suffixLength;

    char* old = m_buf;
    char* replacement = EMPTY;
    if (newLength) {
        replacement = static_cast<char*>(::operator new((newLength & ~0x0Fu) + 0x10u));
        if (prefixLength) memcpy(replacement, old, prefixLength);
        if (destLength) memcpy(replacement + prefixLength, dest, destLength);
        if (suffixLength) memcpy(replacement + prefixLength + destLength,
                                 found + sourceLength, suffixLength);
        replacement[newLength] = 0;
    }
    m_buf = replacement;
    if (old != EMPTY)
        ::operator delete(old);
    return 1;
}

// TRUE when the current string begins with the supplied prefix.
int STRING::HaveFirst(const char* prefix) const
{
    const unsigned int length = strlen(prefix);
    return strncmp(m_buf, prefix, length) == 0;
}

int STRING::HaveSubStr(const char* str)
{
    return strstr(m_buf,str)!=0;
}

void STRING::ToWideChar(unsigned short* buffer,int size_buffer)
{
    MultiByteToWideChar(0,0,m_buf,-1,buffer,size_buffer);
}

STRING STRING::After(const char* marker) const
{
    char* found = strstr(m_buf, marker);
    if (!found)
        return STRING(EMPTY);
    return STRING(found + strlen(marker));
}


// Return the suffix following the final occurrence of marker. Retail repeatedly
// calls strstr(found+1, marker), retaining the previous hit as the last match.
const STRING STRING::AfterLast(const char* marker) const
{
    char* found = strstr(m_buf, marker);
    char* last = found;
    while (found) {
        last = found;
        found = strstr(found + 1, marker);
    }
    if (last)
        return STRING(last + strlen(marker));
    return STRING(EMPTY);
}

// Replace the trailing numeric run (normally the frame number before an extension)
// by that number plus arg, preserving the original field width.
const STRING STRING::Add(int arg) const
{
    STRING result(this);
    if (!arg)
        return result;

    int end = static_cast<int>(strlen(m_buf)) - 1;
    do {
        --end;
        if (end < 0)
            return STRING(EMPTY);
    } while (!isdigit(static_cast<unsigned char>(m_buf[end])));

    int begin = end;
    do {
        --begin;
        if (begin < 0)
            break;
    } while (isdigit(static_cast<unsigned char>(m_buf[begin])));

    if (begin >= 0 && m_buf[begin] != '-')
        ++begin;
    else if (begin < 0)
        begin = 0;

    const int value = atoi(m_buf + begin) + arg;
    const int width = end - begin + 1;
    char temp[128];
    sprintf(temp, "%0*i", width, value);
    memcpy(result.m_buf + begin, temp, static_cast<unsigned int>(width));
    return result;
}

// CF_TEXT clipboard path, including the original GMEM_MOVEABLE|GMEM_ZEROINIT
// allocation flags (0x2042) and 0/1 success convention.
int STRING::WriteToClipboard(HWND__* wnd)
{
    const int length = Length();
    int result = 0;
    if (!OpenClipboard(wnd))
        return 1;

    EmptyClipboard();
    if (length) {
        void* handle = GlobalAlloc(0x2042u, static_cast<unsigned int>(length + 1));
        if (handle) {
            void* dest = GlobalLock(handle);
            memcpy(dest, m_buf, static_cast<unsigned int>(length + 1));
            GlobalUnlock(handle);
            if (!SetClipboardData(1u, handle))
                result = 1;
        } else {
            result = 1;
        }
    }
    CloseClipboard();
    return result;
}

// Retail copies GlobalSize(handle) bytes through STRING::Copy, after releasing
// the previous non-empty allocation, then unlocks and closes the clipboard.
int STRING::ReadFromClipboard(HWND__* wnd)
{
    int result = 0;
    if (!OpenClipboard(wnd))
        return 1;

    void* handle = GetClipboardData(1u);
    if (handle) {
        const char* source = static_cast<const char*>(GlobalLock(handle));
        if (source) {
            if (m_buf != EMPTY)
                ::operator delete(m_buf);
            Copy(source, static_cast<int>(GlobalSize(handle)));
        } else {
            result = 1;
        }
        GlobalUnlock(handle);
    } else {
        result = 1;
    }
    CloseClipboard();
    return result;
}

// Split an old Win32 registry path into root HKEY and sub-key text.
const STRING STRING::SplitRegPath(HKEY__** root) const
{
    if (HaveFirst("HKEY_USERS\\"))
        *root = reinterpret_cast<HKEY__*>(0x80000003u);
    else if (HaveFirst("HKEY_CURRENT_USER\\"))
        *root = reinterpret_cast<HKEY__*>(0x80000001u);
    else if (HaveFirst("HKEY_CLASSES_ROOT\\"))
        *root = reinterpret_cast<HKEY__*>(0x80000000u);
    else if (HaveFirst("HKEY_CURRENT_CONFIG\\"))
        *root = reinterpret_cast<HKEY__*>(0x80000005u);
    else if (HaveFirst("HKEY_LOCAL_MACHINE\\"))
        *root = reinterpret_cast<HKEY__*>(0x80000002u);
    else {
        *root = reinterpret_cast<HKEY__*>(0x80000002u);
        return STRING(this);
    }
    return After("\\");
}

STRING STRING::Before(const char* marker) const
{
    const char* found = strstr(m_buf, marker);
    if (!found)
        return STRING(this);

    STRING result;
    result.Copy(m_buf, static_cast<int>(found - m_buf));
    return result;
}

// Remove all leading characters belonging to the supplied character set.
void STRING::RemoveBeginChars(const char* chars)
{
    const unsigned int count = strspn(m_buf, chars);
    if (count)
        memmove(m_buf, m_buf + count, strlen(m_buf + count) + 1u);
}

// Short MapEdit-local operator+ thunks at 0x00410860/0x00410890 ultimately
// construct the result from the two C strings. Keep the public source readable
// while preserving the original STRING allocation rules.
STRING STRING::operator+(const char* rhs) const
{
    const unsigned int left = strlen(m_buf);
    const unsigned int right = rhs ? strlen(rhs) : 0u;
    STRING result;
    if (!left && !right)
        return result;

    const unsigned int total = left + right;
    result.m_buf = static_cast<char*>(::operator new((total & ~0x0Fu) + 0x10u));
    if (left) memcpy(result.m_buf, m_buf, left);
    if (right) memcpy(result.m_buf + left, rhs, right);
    result.m_buf[total] = 0;
    return result;
}

STRING STRING::operator+(const STRING& rhs) const
{
    return *this + rhs.m_buf;
}

STRING Int2Str(int value)
{
    char buffer[128];
    return STRING(_itoa(value, buffer, 10));
}

// Exact public overload used by editor code; the original thin wrapper reaches
// the same char-buffer Replace implementation.
int STRING::Replace(const STRING& source,const STRING& dest)
{
    return Replace(source.m_buf, dest.m_buf);
}


// The original FSTREAM wrapper ultimately performs ordinary binary stdio.
int STRING::LoadFile(const STRING& filename)
{
    void* file = fopen(filename.m_buf, "rb");
    if (!file) {
        STRING empty;
        *this = empty;
        return 0;
    }

    fseek(file, 0, 2);
    const long end = ftell(file);
    fseek(file, 0, 0);
    if (end < 0) {
        fclose(file);
        STRING empty;
        *this = empty;
        return 0;
    }

    const unsigned int length = static_cast<unsigned int>(end);
    if (m_buf != EMPTY)
        ::operator delete(m_buf);
    m_buf = static_cast<char*>(::operator new((length & ~0x0Fu) + 0x10u));
    if (!m_buf) {
        m_buf = EMPTY;
        fclose(file);
        return 0;
    }

    fread(m_buf, 1u, length, file);
    m_buf[length] = 0;
    fclose(file);
    return static_cast<int>(length);
}

// FSTREAM::Write returns bytes not written, so the retail expression
// Length()-Write(...) is exactly the number of bytes written.
int STRING::SaveFile(const STRING& filename) const
{
    void* file = fopen(filename.m_buf, "wb");
    if (!file)
        return 0;
    const unsigned int length = static_cast<unsigned int>(strlen(m_buf));
    const unsigned int written = fwrite(m_buf, 1u, length, file);
    fclose(file);
    return static_cast<int>(written);
}

// Retail keeps the allocation when the 16-byte storage class does not change.
const STRING* STRING::operator+=(const char* rhs)
{
    if (!rhs || !*rhs)
        return this;

    const unsigned int oldLength = strlen(m_buf);
    const unsigned int addLength = strlen(rhs);
    const unsigned int oldBytes = (oldLength & ~0x0Fu) + 0x10u;
    const unsigned int newBytes = ((oldLength + addLength) & ~0x0Fu) + 0x10u;

    if (m_buf == EMPTY || newBytes != oldBytes) {
        char* old = m_buf;
        m_buf = static_cast<char*>(::operator new(newBytes));
        if (oldLength)
            memcpy(m_buf, old, oldLength);
        if (old != EMPTY)
            ::operator delete(old);
    }

    memcpy(m_buf + oldLength, rhs, addLength + 1u);
    return this;
}


// Retail has a distinct STRING overload; do not collapse it into the char* owner.
const STRING* STRING::operator+=(const STRING* r)
{
    if (!r || !r->m_buf || !*r->m_buf)
        return this;

    const char* rhs=r->m_buf;
    const unsigned int oldLength=strlen(m_buf);
    const unsigned int addLength=strlen(rhs);
    const unsigned int oldBytes=(oldLength&~0x0Fu)+0x10u;
    const unsigned int newBytes=((oldLength+addLength)&~0x0Fu)+0x10u;

    if (m_buf==EMPTY || newBytes!=oldBytes) {
        char* old=m_buf;
        m_buf=static_cast<char*>(::operator new(newBytes));
        if (oldLength)
            memcpy(m_buf,old,oldLength);
        if (old!=EMPTY)
            ::operator delete(old);
    }

    memcpy(m_buf+oldLength,rhs,addLength+1u);
    return this;
}


STRING __cdecl Printf(const char* format,...)
{
    char buffer[4096]={0};
    va_list args;
    va_start(args,format);
    vsprintf(buffer,format,args);
    va_end(args);
    return STRING(buffer);
}

int STRING::operator==(const STRING* r)
{
    return strcmp(m_buf,r->m_buf)==0;
}

const STRING* STRING::operator=(const STRING* r)
{
    if (this==r)
        return this;

    const unsigned int length=strlen(r->m_buf);
    if (m_buf==EMPTY) {
        if (length) {
            m_buf=static_cast<char*>(::operator new((length&~0x0Fu)+0x10u));
            memcpy(m_buf,r->m_buf,length+1u);
        }
        return this;
    }

    if (length) {
        const unsigned int oldLength=strlen(m_buf);
        if (((oldLength&~0x0Fu)+0x10u)!=((length&~0x0Fu)+0x10u)) {
            ::operator delete(m_buf);
            m_buf=static_cast<char*>(::operator new((length&~0x0Fu)+0x10u));
        }
        memcpy(m_buf,r->m_buf,length+1u);
    } else {
        ::operator delete(m_buf);
        m_buf=EMPTY;
    }
    return this;
}

const STRING* STRING::operator=(const char* str)
{
    const unsigned int length=strlen(str);
    if (m_buf==EMPTY) {
        if (length) {
            m_buf=static_cast<char*>(::operator new((length&~0x0Fu)+0x10u));
            memcpy(m_buf,str,length+1u);
        }
        return this;
    }

    if (length) {
        const unsigned int oldLength=strlen(m_buf);
        if (((oldLength&~0x0Fu)+0x10u)!=((length&~0x0Fu)+0x10u)) {
            ::operator delete(m_buf);
            m_buf=static_cast<char*>(::operator new((length&~0x0Fu)+0x10u));
        }
        memcpy(m_buf,str,length+1u);
    } else {
        ::operator delete(m_buf);
        m_buf=EMPTY;
    }
    return this;
}

int STRING::operator<(const STRING* r)
{
    return strcmp(m_buf,r->m_buf) < 0;
}

int STRING::operator>(const STRING* r)
{
    return strcmp(m_buf,r->m_buf) > 0;
}

int STRING::operator!=(const STRING* r)
{
    return strcmp(m_buf,r->m_buf)!=0;
}

int FExist(const STRING* name)
{
    if (!name->m_buf[0])
        return 0;
    void* file=fopen(name->m_buf,"rb");
    if (file)
        fclose(file);
    return file!=0;
}

const STRING __cdecl operator+(const char* lhs,const STRING& rhs)
{
    return STRING(lhs,rhs.m_buf);
}

STRING STRING::ToUpper()
{
    STRING result(*this);
    _strupr(result.m_buf);
    return result;
}

void STRING::Write(FILE* file)
{
    fwrite(m_buf,strlen(m_buf)+1u,1u,file);
}

STRING STRING::BeforeLast(const char* marker) const
{
    const char* found=strstr(m_buf,marker);
    const char* last=found;
    while (found) {
        last=found;
        found=strstr(found+1,marker);
    }
    if (!last)
        return STRING(EMPTY);
    return STRING(m_buf,static_cast<int>(last-m_buf));
}

char STRING::FirstChar() const
{
    return m_buf[0];
}

// Retail encodes 57 input bytes per line (76 Base64 chars) and adds add_code
// to every source byte before packing the 24-bit triples.
STRING STRING::ToBase64(int add_code)
{
    static const char table[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const unsigned int length=static_cast<unsigned int>(strlen(m_buf));
    STRING result;
    int written=0;
    for (unsigned int pos=0;pos<length;pos+=57u) {
        const unsigned int chunk=(pos+57u<length) ? 57u : (length-pos);
        unsigned char line[60];
        memcpy(line,m_buf+pos,chunk);
        memset(line+chunk,0,57u-chunk);
        for (unsigned int j=0;j<chunk;++j)
            line[j]=static_cast<unsigned char>(line[j]+add_code);

        char out[80];
        int o=0;
        for (unsigned int t=0;t<chunk;t+=3u) {
            unsigned int triple=line[t];
            triple=(triple<<8)+line[t+1];
            triple=(triple<<8)+line[t+2];
            char* dst=out+o+3;
            int count=4;
            do {
                *dst--=table[triple&0x3Fu];
                triple>>=6;
            } while (--count);
            o+=4;
        }
        written+=o;
        out[o]=0;
        result+=out;
    }
    int pad=static_cast<int>((3u-(length%3u))%3u);
    for (;pad>0;--pad)
        result.m_buf[written-pad]='=';
    return result;
}
