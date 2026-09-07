#include "mapedit/runtime.hpp"

// The original LIST<T> allocator grows max as max*2+4 and transfers list
// ownership during undo-history compaction. These specializations are emitted
// by undo.obj for UNDO_DATA specifically.
template<> LIST<UNDO::UNDO_DATA>::LIST() : m_no(0), m_max(0), m_data(0) {}
template<> int LIST<UNDO::UNDO_DATA>::No() { return m_no; }
template<> UNDO::UNDO_DATA* LIST<UNDO::UNDO_DATA>::operator[](int index) { return m_data+index; }

template<> void LIST<UNDO::UNDO_DATA>::Release()
{
    m_max=0;
    m_no=0;
    if (m_data)
        operator delete(m_data);
    m_data=0;
}

template<> LIST<UNDO::UNDO_DATA>::~LIST() { Release(); }

template<> void LIST<UNDO::UNDO_DATA>::Expand(int newAllocation)
{
    if (newAllocation<=m_max)
        return;
    UNDO::UNDO_DATA* old=m_data;
    UNDO::UNDO_DATA* replacement=static_cast<UNDO::UNDO_DATA*>(operator new(sizeof(UNDO::UNDO_DATA)*newAllocation));
    if (!replacement)
        return;
    for (int i=0;i<newAllocation;++i)
        replacement[i].sprite=0;
    if (old) {
        for (int i=0;i<m_max;++i)
            replacement[i]=old[i];
        operator delete(old);
    }
    m_data=replacement;
    m_max=newAllocation;
}

template<> void LIST<UNDO::UNDO_DATA>::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<UNDO::UNDO_DATA>::Insert(UNDO::UNDO_DATA item)
{
    ExpandForInsert();
    m_data[m_no]=item;
    ++m_no;
}

template<> int LIST<UNDO::UNDO_DATA>::DeleteNumber(int index)
{
    if (index<0 || index>=m_no)
        return 1;
    --m_no;
    m_data[index]=m_data[m_no];
    return 0;
}

template<> void LIST<UNDO::UNDO_DATA>::GetReleased(LIST<UNDO::UNDO_DATA>* source)
{
    if (this==source)
        return;
    if (m_data)
        operator delete(m_data);
    m_no=source->m_no;
    m_max=source->m_max;
    m_data=source->m_data;
    source->m_no=0;
    source->m_max=0;
    source->m_data=0;
}

UNDO::UNDO_DATA::UNDO_DATA() : sprite(0) {}
UNDO::UNDO_DATA::UNDO_DATA(TYPE_CREATE createType,SPRITE* spr) : type(createType),sprite(spr) {}

void UNDO::UNDO_DATA::Undo()
{
    if (type==INSERTED) {
        sprite->Action(0x84,0,0,0);
        type=DELETED;
    } else {
        sprite->Action(0x85,0,0,0);
        type=INSERTED;
    }
}

void UNDO::UNDO_DATA::DeleteSprite()
{
    if (sprite && type==DELETED)
        sprite->ScalarDeletingDestructor(1);
    sprite=0;
}

UNDO::UNDO() : noUndo(0) {}
UNDO::~UNDO() {}

void UNDO::Undo()
{
    if (!IsUndo())
        return;
    --noUndo;
    for (int i=0;i<sprites[noUndo].No();++i)
        sprites[noUndo][i]->Undo();
}

void UNDO::Redo()
{
    if (!IsRedo())
        return;
    for (int i=0;i<sprites[noUndo].No();++i)
        sprites[noUndo][i]->Undo();
    ++noUndo;
}

void UNDO::Begin()
{
    End();
    for (int i=noUndo;i<5;++i)
        ReleaseUndo(i);
    if (noUndo>=5) {
        ReleaseUndo(0);
        for (int i=1;i<5;++i)
            sprites[i-1].GetReleased(&sprites[i]);
        noUndo=4;
    }
}

void UNDO::End()
{
    if (noUndo<5 && sprites[noUndo].No())
        ++noUndo;
}

void UNDO::ReleaseUndo(int index)
{
    for (int i=sprites[index].No()-1;i>=0;--i)
        sprites[index][i]->DeleteSprite();
    sprites[index].Release();
}

void UNDO::AddInsert(SPRITE* sprite)
{
    if (sprite && !sprite->IsSpriteClass(0x15))
        sprites[noUndo].Insert(UNDO_DATA(UNDO_DATA::INSERTED,sprite));
}

void UNDO::AddRemove(SPRITE* sprite)
{
    if (!sprite)
        return;
    if (!sprite->IsSpriteClass(0x15)) {
        sprites[noUndo].Insert(UNDO_DATA(UNDO_DATA::DELETED,sprite));
        sprite->Action(0x84,0,0,0);
    } else {
        sprite->ScalarDeletingDestructor(1);
    }
}

void UNDO::DeleteLast()
{
    sprites[noUndo].DeleteNumber(sprites[noUndo].No()-1);
}
