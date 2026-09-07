#include "mapedit/runtime.hpp"

// groups.obj maintenance. These bodies are promoted only after checking the
// original MapEdit.exe instruction ranges against the debug metadata signatures.

GROUP::GROUP(GROUP* previous, SPRITE* sprite)
    : SPRITE_LIST(), x(0.0f), y(0.0f), behave(0), nPlayer(0), nextGroup(0)
{
    if (previous) {
        nextGroup = previous->nextGroup;
        previous->nextGroup = this;
    } else {
        nextGroup = this;
    }

    if (sprite)
        Insert(sprite);
}

GROUP::~GROUP()
{
    GROUP* previous = PrevGroup();
    if (previous)
        previous->nextGroup = nextGroup;
}

GROUP* GROUP::PrevGroup()
{
    GROUP* previous = nextGroup;
    if (!previous)
        return 0;
    while (previous->nextGroup != this)
        previous = previous->nextGroup;
    return previous;
}

void GROUP::Insert(SPRITE* sprite)
{
    if (No()) {
        x = (sprite->X() + x) / 2.0f;
        y = (sprite->Y() + y) / 2.0f;
    } else {
        x = sprite->X();
        y = sprite->Y();
    }
    SPRITE_LIST::Insert(sprite);
}

void GROUP::Draw()
{
    for (int i = 0; i < No(); ++i) {
        SPRITE* sprite = *(*this)[i];
        Graph->Line(Map->ToScreenX(x), Map->ToScreenY(y),
                    sprite->ScreenX(), sprite->ScreenY(), GRAPH::WHITE);
    }
}

void GROUP::DrawNumber(int number)
{
    for (int i = 0; i < No(); ++i) {
        SPRITE* sprite = *(*this)[i];
        Graph->PrintfXY(sprite->ScreenX(), sprite->ScreenY(), "%i", number);
    }
}

GROUP* GROUPS::Next(const GROUP* group)
{
    if (!group)
        return 0;
    GROUP* next = group->nextGroup;
    return next != &first ? next : 0;
}

const GROUP* GROUPS::Next(const GROUP* group) const
{
    if (!group)
        return 0;
    const GROUP* next = group->nextGroup;
    return next != &first ? next : 0;
}

void GROUPS::DeletePointerToSprite(SPRITE* sprite)
{
    GROUP* group = First();
    while (group) {
        if (group->Delete(sprite) == 0 && group->No() == 0) {
            GROUP* dead = group;
            group = Next(group);
            delete dead;
        } else {
            group = Next(group);
        }
    }
}

void GROUPS::DrawNumber()
{
    GROUP* group = First();
    int number = 0;
    while (group) {
        group->DrawNumber(number++);
        group = Next(group);
    }
}

void GROUPS::InsertToNearGroup(SPRITE* spr)
{
    GROUP* nearest=First();
    for (GROUP* group=nearest;group;group=Next(group)) {
        if (nearest->DistanceTo(spr)>group->DistanceTo(spr))
            nearest=group;
    }

    if (!nearest || nearest->DistanceTo(spr)>300.0f)
        nearest=new GROUP(&first,spr);
    else
        nearest->Insert(spr);
}

void GROUPS::ShiftFirstLeft()
{
    GROUP* group = First();
    if (!group || group->nextGroup == &first)
        return;
    GROUP* previous = group->PrevGroup();
    GROUP* previousPrevious = previous->PrevGroup();
    previousPrevious->nextGroup = group;
    previous->nextGroup = group->nextGroup;
    group->nextGroup = previous;
}

void GROUPS::ShiftFirstRight()
{
    GROUP* group = First();
    if (!group || group->nextGroup == &first)
        return;
    GROUP* next = group->nextGroup;
    GROUP* previous = group->PrevGroup();
    previous->nextGroup = next;
    group->nextGroup = next->nextGroup;
    next->nextGroup = group;
}

void GROUPS::Save(RESOURCE* res)
{
    GROUP* group=First();
    while (group) {
        if (group->No()) {
            for (int i=0;i<group->No();++i)
                res->Write((*group)[i],4u);
            const int end=-1;
            res->Write(&end,4u);
        }
        group=Next(group);
    }
    const int end=-1;
    res->Write(&end,4u);
}


float GROUP::DistanceTo(const SPRITE* spr)
{
    return NearDistance(spr->X()-x,spr->Y()-y);
}

GROUPS::GROUPS()
    : first(0,0)
{
}

void GROUPS::Load(RESOURCE* res)
{
    for (;;) {
        SPRITE* firstSprite=Map->ReadPointer(res);
        if (firstSprite==reinterpret_cast<SPRITE*>(-1))
            break;

        GROUP* group=new GROUP(&first,firstSprite);
        for (;;) {
            SPRITE* sprite=Map->ReadPointer(res);
            if (sprite==reinterpret_cast<SPRITE*>(-1))
                break;
            group->Insert(sprite);
        }
    }
}
