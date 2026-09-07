#include "mapedit/runtime.hpp"
void UNIT::LineDraw()
{
    SPRITE* goal=Goal();
    if (!goal)
        return;
    Graph->Line(ScreenX(),ScreenY(),goal->ScreenX(),goal->ScreenY(),
                IsCommand(1u) ? GRAPH::GREEN : GRAPH::BLUE);
}
