#include "mapedit/runtime.hpp"

template<> LIST<SPRITE*>::LIST() : m_no(0), m_max(0), m_data(0) {}

template<> LIST<SPRITE*>::~LIST()
{
    if (m_data)
        operator delete(m_data);
    m_data = 0;
    m_no = 0;
}

SPRITE_LIST::SPRITE_LIST() : LIST<SPRITE*>() {}
SPRITE_LIST::~SPRITE_LIST() {}

// Explicit LIST<SPRITE*> specializations must precede every specialization that calls them.
template<> void LIST<SPRITE*>::ExpandForInsert()
{
    if (m_no >= m_max)
        Expand(m_max * 2 + 4);
}

template<> int LIST<SPRITE*>::DeleteNumber(int index)
{
    if (index < 0 || index >= m_no)
        return 1;
    --m_no;
    m_data[index] = m_data[m_no];
    return 0;
}

template<> void LIST<SPRITE*>::Insert(SPRITE* item)
{
    ExpandForInsert();
    m_data[m_no] = item;
    ++m_no;
}

template<> void LIST<SPRITE*>::InsertFirst(SPRITE* item)
{
    ExpandForInsert();
    int i=m_no;
    ++m_no;
    while (i!=0) {
        m_data[i]=m_data[i-1];
        --i;
    }
    m_data[0]=item;
}

template<> void LIST<SPRITE*>::InsertBefore(int n,SPRITE* item)
{
    ExpandForInsert();
    int i=m_no;
    ++m_no;
    while (i>n) {
        m_data[i]=m_data[i-1];
        --i;
    }
    m_data[n]=item;
}

template<> int LIST<SPRITE*>::InsertUnique(SPRITE* const* item)
{
    if (Location(item) < 0) {
        Insert(*item);
        return 0;
    }
    return 1;
}

template<> int LIST<SPRITE*>::Delete(SPRITE* const* item)
{
    return DeleteNumber(Location(item));
}

// Reference-counted sprite-list wrappers derived from sprite_list.obj.
// The list owns one SPRITE reference for every stored entry, including duplicates.

void SPRITE_LIST::InsertFirst(SPRITE* sprite)
{
    if (!sprite)
        return;
    sprite->AddRef();
    LIST<SPRITE*>::InsertFirst(sprite);
}

void SPRITE_LIST::InsertBefore(int n,SPRITE* sprite)
{
    if (!sprite)
        return;
    sprite->AddRef();
    LIST<SPRITE*>::InsertBefore(n,sprite);
}

void SPRITE_LIST::Insert(SPRITE* sprite)
{
    if (!sprite)
        return;
    sprite->AddRef();
    LIST<SPRITE*>::Insert(sprite);
}

int SPRITE_LIST::InsertUnique(SPRITE* sprite)
{
    if (!sprite)
        return 1;
    sprite->AddRef();
    return LIST<SPRITE*>::InsertUnique(&sprite);
}

int SPRITE_LIST::Delete(SPRITE* sprite)
{
    if (sprite && LIST<SPRITE*>::Delete(&sprite) == 0) {
        sprite->Release();
        return 0;
    }
    return 1;
}

int MENU::Delete(SPRITE* sprite)
{
    if (this->sprite==sprite)
        this->sprite=0;
    return SPRITE_LIST::Delete(sprite);
}

int SPRITE_LIST::DeleteSpriteNumber(int index)
{
    if (index < 0 || index >= No())
        return 1;

    SPRITE* sprite = *(*this)[index];
    LIST<SPRITE*>::DeleteNumber(index);
    if (sprite->Release() != 0)
        sprite->ScalarDeletingDestructor(1);
    return 0;
}

void SPRITE_LIST::DeleteAll()
{
    // First collapse duplicate pointers. Every removed duplicate releases the
    // reference that belonged to that list entry.
    for (int first = 0; first < No(); ++first) {
        for (int scan = No() - 1; scan > first; --scan) {
            SPRITE** firstItem = (*this)[first];
            if (*firstItem && *firstItem == *(*this)[scan]) {
                (*(*this)[scan])->Release();
                LIST<SPRITE*>::DeleteNumber(scan);
            }
        }
    }

    // Delete one surviving entry for every distinct sprite, in reverse order.
    for (int index = No() - 1; index >= 0; --index) {
        if (*(*this)[index])
            DeleteSpriteNumber(index);
    }
}

void SPRITE_LIST::Release()
{
    for (int index = No() - 1; index >= 0; --index) {
        SPRITE* sprite = *(*this)[index];
        if (sprite && sprite->Release() != 0)
            LIST<SPRITE*>::DeleteNumber(index);
    }
}

int SPRITE_LIST::IsEqual(const SPRITE_LIST* compare)
{
    SPRITE_LIST* mutableCompare = const_cast<SPRITE_LIST*>(compare);
    if (!mutableCompare || mutableCompare->No() != No())
        return 0;

    for (int mine=No()-1; mine>=0; --mine) {
        int other=mutableCompare->No()-1;
        for (; other>=0; --other) {
            if (*(*this)[mine] == *(*mutableCompare)[other])
                break;
        }
        if (other < 0)
            return 0;
    }
    return 1;
}
