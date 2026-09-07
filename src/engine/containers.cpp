#include "mapedit/runtime.hpp"

template<> int LIST<SPRITE*>::No() { return m_no; }
template<> SPRITE** LIST<SPRITE*>::First() { return m_data; }
template<> SPRITE** LIST<SPRITE*>::operator[](int index) { return m_data + index; }

template<> SPRITE* const* LIST<SPRITE*>::operator[](int index) const { return m_data + index; }

template<> SPRITE** LIST<SPRITE*>::Last() { return m_data + (m_no-1); }

template<> SPRITE** LIST<SPRITE*>::Item(int index) { return m_data + index; }

template<> int LIST<SPRITE*>::Location(SPRITE* const* item)
{
    for (int i = m_no - 1; i >= 0; --i) {
        if (m_data[i] == *item)
            return i;
    }
    return -1;
}

template<> int LIST<SPRITE*>::ExistIn(SPRITE* const* item)
{
    return Location(item)>=0;
}

GROUP* GROUPS::First()
{
    return first.nextGroup != &first ? first.nextGroup : 0;
}

const GROUP* GROUPS::First() const
{
    return first.nextGroup != &first ? first.nextGroup : 0;
}

float GROUP::X() const { return x; }
float GROUP::Y() const { return y; }

void GROUPS::DeleteAll()
{
    while (GROUP* group = First())
        delete group;
}

GROUP* GROUPS::CreateNewGroup(SPRITE* sprite)
{
    return new GROUP(reinterpret_cast<GROUP*>(this), sprite);
}

int UNDO::IsUndo() { return noUndo; }
int UNDO::IsRedo() { return noUndo < 5 && sprites[noUndo].No(); }

void UNDO::Reset()
{
    for (int i = 0; i < 5; ++i)
        ReleaseUndo(i);
    noUndo = 0;
}

template<> SPRITE* LIST<SPRITE*>::BeginIterate(int* index)
{
    if (!m_no)
        return 0;
    *index=m_no-1;
    return m_data[*index];
}

template<> SPRITE* LIST<SPRITE*>::NextIterate(int* index)
{
    if (*index>m_no)
        *index=m_no;
    --*index;
    return *index>=0 ? m_data[*index] : 0;
}

template<> void LIST<SPRITE*>::Release()
{
    m_max = 0;
    m_no = 0;
    if (m_data)
        ::operator delete(m_data);
    m_data = 0;
}

template<> void LIST<SPRITE*>::Expand(int newAllocation)
{
    if (newAllocation <= m_max)
        return;

    SPRITE** oldData = m_data;
    SPRITE** newData = static_cast<SPRITE**>(::operator new(static_cast<unsigned int>(newAllocation) * sizeof(SPRITE*)));
    m_data = newData;
    if (oldData) {
        for (int i = 0; i < m_max; ++i)
            newData[i] = oldData[i];
        ::operator delete(oldData);
    }
    m_max = newAllocation;
}

template<> void LIST<SPRITE*>::DeleteFrom(int n)
{
    if (n <= 0) {
        Release();
        return;
    }
    if (n < m_no)
        m_no = n;
}


template<> LIST<int>::LIST() : m_no(0), m_max(0), m_data(0) {}

template<> void LIST<int>::Release()
{
    m_max=0;
    m_no=0;
    if (m_data)
        ::operator delete(m_data);
    m_data=0;
}

template<> LIST<int>::~LIST()
{
    Release();
}

ACT::ACT() {}

ACT::ACT(const ACT* r)
    : act(r->act), var1(r->var1), var2(r->var2), var3(r->var3)
{
}

int ACT::operator==(const ACT* r)
{
    return act==r->act && var1==r->var1 && var2==r->var2 && var3==r->var3;
}

int ACT::operator!=(const ACT* r)
{
    return act!=r->act || var1!=r->var1 || var2!=r->var2 || var3!=r->var3;
}

ACT::ACT(int a,int v1,int v2,int v3)
    : act(a), var1(v1), var2(v2), var3(v3)
{
}

// LIST<ACT> owners emitted into map_edit.obj.  LIST keeps a VC6-style
// vptr followed by no/max/data; the algorithms below follow the retail
// mylib.h owners at 0x00410C90 / 0x00410E00 / 0x00410E30.
template<> LIST<ACT>::LIST() : m_no(0), m_max(0), m_data(0) {}

template<> void LIST<ACT>::Release()
{
    m_max=0;
    m_no=0;
    if (m_data)
        ::operator delete(m_data);
    m_data=0;
}

template<> LIST<ACT>::~LIST()
{
    Release();
}

template<> int LIST<ACT>::No() { return m_no; }
template<> ACT* LIST<ACT>::First() { return m_data; }
template<> ACT* LIST<ACT>::operator[](int index) { return m_data + index; }

template<> const ACT* LIST<ACT>::operator[](int index) const { return m_data + index; }

template<> int LIST<ACT>::Location(const ACT* item)
{
    int index=m_no;
    while (index) {
        --index;
        if (m_data[index].operator==(item))
            return index;
    }
    return -1;
}

// The explicit specialization is declared before source-inline users so modern
// compilers do not instantiate the unspecialized template before its body below.
template<> void LIST<ACT>::Expand(int newAllocation);

template<> void LIST<ACT>::SetNo(int newNo)
{
    m_no=newNo;
    if (m_no>m_max)
        Expand(m_no);
}


template<> ACT* LIST<ACT>::Last() { return m_data + (m_no-1); }

template<> ACT* LIST<ACT>::Pop() { return m_data + (--m_no); }

template<> void LIST<ACT>::Expand(int newAllocation)
{
    if (newAllocation <= m_max)
        return;

    ACT* oldData=m_data;
    // ACT's retail default constructor (0x00410F40) is intentionally empty.
    // The VC6 vector-constructor loop therefore has no observable writes, so raw
    // allocation preserves the exact initialized-state semantics without needing
    // a modern placement-new declaration.
    ACT* newData=static_cast<ACT*>(::operator new(static_cast<unsigned int>(newAllocation) * sizeof(ACT)));

    if (oldData) {
        for (int i=0;i<m_max;++i)
            newData[i]=oldData[i];
        ::operator delete(oldData);
    }
    m_data=newData;
    m_max=newAllocation;
}

template<> void LIST<ACT>::ExpandForInsert()
{
    if (m_no >= m_max)
        Expand(m_max * 2 + 4);
}

template<> void LIST<ACT>::InsertFirst(ACT item)
{
    ExpandForInsert();
    int index=m_no;
    ++m_no;
    while (index != 0) {
        m_data[index]=m_data[index-1];
        --index;
    }
    m_data[0]=item;
}

template<> void LIST<ACT>::Insert(ACT item)
{
    ExpandForInsert();
    m_data[m_no++]=item;
}

template<> void LIST<ACT>::InsertBefore(int n,ACT item)
{
    ExpandForInsert();
    int index=m_no;
    ++m_no;
    while (index>n) {
        m_data[index]=m_data[index-1];
        --index;
    }
    m_data[n]=item;
}

template<> void LIST<ACT>::DeleteNumberS(int n)
{
    if (n>=0 && n<m_no) {
        --m_no;
        while (n<m_no) {
            m_data[n]=m_data[n+1];
            ++n;
        }
    }
    if (!m_no)
        Release();
}

template<> void LIST<ACT>::ShiftUp(int n)
{
    if (n!=0) {
        ACT item(m_data+n-1);
        m_data[n-1]=m_data[n];
        m_data[n]=item;
    }
}

template<> void LIST<ACT>::ShiftDown(int n)
{
    if (n<m_no-1) {
        ACT item(m_data+n+1);
        m_data[n+1]=m_data[n];
        m_data[n]=item;
    }
}

template<> const LIST<ACT>* LIST<ACT>::operator=(const LIST<ACT>* r)
{
    if (this==r)
        return this;
    Release();
    m_no=r->m_no;
    m_max=r->m_max;
    m_data=static_cast<ACT*>(::operator new(static_cast<unsigned int>(m_max)*sizeof(ACT)));
    if (!m_data && m_max) {
        MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory for = %i",m_max);
        return this;
    }
    for (int i=0;i<m_no;++i)
        m_data[i]=r->m_data[i];
    return this;
}

// Legacy ACT format stores only the first three DWORDs of each action. Retail
// reads the old payload as one packed 12-byte-per-item block into LIST storage.
template<> void LIST<ACT>::OldRead(STREAM* file)
{
    short count=0;
    file->Read(&count,2u);
    m_no=static_cast<int>(count);
    Expand(m_no);
    file->Read(m_data,static_cast<unsigned int>(m_no*12));
}

template<> int LIST<int>::No() { return m_no; }
template<> int* LIST<int>::operator[](int index) { return m_data + index; }

template<> void LIST<int>::Expand(int newAllocation)
{
    if (newAllocation <= m_max)
        return;
    int* oldData=m_data;
    int* newData=static_cast<int*>(::operator new(static_cast<unsigned int>(newAllocation)*sizeof(int)));
    if (oldData) {
        for (int i=0;i<m_max;++i)
            newData[i]=oldData[i];
        ::operator delete(oldData);
    }
    m_data=newData;
    m_max=newAllocation;
}

template<> int LIST<int>::Location(int const* item)
{
    for (int index=m_no-1; index>=0; --index) {
        if (m_data[index] == *item)
            return index;
    }
    return -1;
}

template<> void LIST<int>::ExpandForInsert()
{
    if (m_no >= m_max)
        Expand(m_max*2 + 4);
}

template<> void LIST<int>::Insert(int item)
{
    ExpandForInsert();
    m_data[m_no++] = item;
}

template<> int LIST<int>::InsertUnique(int const* item)
{
    if (Location(item) < 0) {
        Insert(*item);
        return 0;
    }
    return 1;
}


template<> int LIST<int>::DeleteNumber(int index)
{
    if (index<0 || index>=m_no)
        return 1;
    --m_no;
    m_data[index]=m_data[m_no];
    return 0;
}

template<> int LIST<int>::Delete(const int* item)
{
    return DeleteNumber(Location(item));
}

template<> void LIST<int>::Write(STREAM* file)
{
    file->Write(&m_no,sizeof(m_no));
    file->Write(m_data,static_cast<unsigned int>(m_no)*sizeof(int));
}


template<> void LIST<ACT>::Write(STREAM* file)
{
    file->Write(&m_no,sizeof(m_no));
    file->Write(m_data,static_cast<unsigned int>(m_no)*sizeof(ACT));
}

template<> int LIST<ACT>::DeleteNumber(int n)
{
    if (n<0 || n>=m_no)
        return 1;
    --m_no;
    m_data[n]=m_data[m_no];
    return 0;
}

template<> void LIST<ACT>::Read(STREAM* file)
{
    file->Read(&m_no,sizeof(m_no));
    Expand(m_no);
    file->Read(m_data,static_cast<unsigned int>(m_no)*sizeof(ACT));
}

template<> const LIST<int>* LIST<int>::operator=(const LIST<int>* r)
{
    if (this==r)
        return this;
    Release();
    m_no=r->m_no;
    m_max=r->m_max;
    m_data=static_cast<int*>(::operator new(static_cast<unsigned int>(m_max)*sizeof(int)));
    if (!m_data && m_max) {
        MYERROR::LogExit(::Error,"!!!ERROR!!!::LIST: Not enough memory for = %i",m_max);
        return this;
    }
    for (int i=0;i<m_no;++i)
        m_data[i]=r->m_data[i];
    return this;
}

template<> void LIST<int>::Read(STREAM* file)
{
    file->Read(&m_no,sizeof(m_no));
    Expand(m_no);
    file->Read(m_data,static_cast<unsigned int>(m_no)*sizeof(int));
}

// Legacy pre-version-11 item list used by UNIT::Action load compatibility.
template<> LIST<short>::LIST() : m_no(0), m_max(0), m_data(0) {}

template<> void LIST<short>::Release()
{
    m_max=0;
    m_no=0;
    if (m_data)
        ::operator delete(m_data);
    m_data=0;
}

template<> LIST<short>::~LIST() { Release(); }
template<> int LIST<short>::No() { return m_no; }
template<> short* LIST<short>::operator[](int index) { return m_data + index; }

template<> void LIST<short>::Expand(int newAllocation)
{
    if (newAllocation <= m_max)
        return;
    short* oldData=m_data;
    short* newData=static_cast<short*>(::operator new(static_cast<unsigned int>(newAllocation)*sizeof(short)));
    if (oldData) {
        for (int i=0;i<m_max;++i)
            newData[i]=oldData[i];
        ::operator delete(oldData);
    }
    m_data=newData;
    m_max=newAllocation;
}

template<> void LIST<short>::Read(STREAM* file)
{
    file->Read(&m_no,sizeof(m_no));
    Expand(m_no);
    file->Read(m_data,static_cast<unsigned int>(m_no)*sizeof(short));
}
