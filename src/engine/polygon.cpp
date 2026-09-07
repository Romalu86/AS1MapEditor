#include "mapedit/runtime.hpp"

// Short inline owners emitted into map_edit.obj. The larger geometry routines
// remain in graph.cpp and are declared in runtime.hpp.
POLYGON::POLYGON() : head(0), closed(0), noPoint(0) {}
POLYGON::~POLYGON() { Release(); }
int POLYGON::NoPoint() { return noPoint; }

void POLYGON::Release()
{
    POINTLIST* current = head;
    for (int i=0; i<noPoint; ++i) {
        POINTLIST* next = current->next;
        ::operator delete(current);
        current = next;
    }
    head = 0;
    closed = 0;
    noPoint = 0;
}

void POLYGON::Closed()
{
    if (head)
        AddLinedPoint(head->x, head->y);
}

POINTLIST* POLYGON::CreatePoint(float x,float y)
{
    POINTLIST* point = static_cast<POINTLIST*>(::operator new(sizeof(POINTLIST)));
    if (!point) {
        Graph->Error(2,"AddPoint",0);
        return 0;
    }
    point->next = 0;
    point->x = x;
    point->y = y;
    return point;
}

void POLYGON::AddLinedPoint(float x,float y)
{
    if (!head) {
        head = CreatePoint(x,y);
        if (head) {
            head->next = head;
            noPoint = 1;
        }
        return;
    }

    if (noPoint > 1) {
        ANGLE oldLine(head->next->x - head->x, head->next->y - head->y);
        ANGLE newLine(head->next->x - x, head->next->y - y);
        int delta = static_cast<int>(oldLine.value) - static_cast<int>(newLine.value);
        if (delta < 0) delta = -delta;
        if (delta < 2) {
            head->x = x;
            head->y = y;
            return;
        }
    }

    const float oldX = head->x;
    const float oldY = head->y;
    head->x = x;
    head->y = y;
    POINTLIST* point = CreatePoint(oldX, oldY);
    if (!point)
        return;
    point->next = head->next;
    head->next = point;
    ++noPoint;
}

int POLYGON::AskInside(float x,float y)
{
    if (noPoint <= 1)
        return 0;

    POINTLIST* point = head;
    int crossings = 0;
    for (int i=0; i<noPoint; ++i, point=point->next) {
        POINTLIST* next = point->next;

        const int crossesX = (x > point->x && x < next->x) ||
                             (x < point->x && x > next->x);
        if (crossesX) {
            const int deltaY = static_cast<int>(next->y - point->y);
            const float crossY = point->y +
                (x - point->x) * static_cast<float>(deltaY) /
                (next->x - point->x);
            if (crossY > y)
                ++crossings;
        }

        if (next->x == x) {
            POINTLIST* after = next->next ? next->next : head->next;
            const int between = (x > point->x && x < after->x) ||
                                (x < point->x && x > after->x);
            if (between && next->y > y)
                ++crossings;
        }
    }
    return crossings & 1;
}

void POLYGON::CreateBox(float x0,float y0,float x1,float y1)
{
    head = CreatePoint(x0,y0);
    if (!head)
        return;
    head->next = CreatePoint(x1,y0);
    if (!head->next) { Release(); return; }
    head->next->next = CreatePoint(x1,y1);
    if (!head->next->next) { Release(); return; }
    head->next->next->next = CreatePoint(x0,y1);
    if (!head->next->next->next) { Release(); return; }
    head->next->next->next->next = head;
    noPoint = 4;
    closed = 2;
}

void POLYGON::Draw(COLOR color)
{
    POINTLIST* point = head;
    for (int i=0; i<noPoint; ++i) {
        Graph->Line(Map->ToScreenX(point->x), Map->ToScreenY(point->y),
                    Map->ToScreenX(point->next->x), Map->ToScreenY(point->next->y),
                    color);
        point = point->next;
    }
}
