#include "mapedit/runtime.hpp"

DRAW_MAP::DRAW_MAP(SPRITE* map)
{
    // Retail writes only the one declared option bit and preserves the
    // remaining 31 bits of the bitfield storage.
    optDrawBackGround=0;
    VID* const vid=map->Vid();
    begx=map->ScreenX()-static_cast<float>(vid->m_regionTileStepX/2)+8.0f;
    begy=map->ScreenY()-static_cast<float>(vid->m_regionTileStepY/2)+10.0f;
    sizex=static_cast<float>(vid->m_regionTileStepX)-14.0f;
    sizey=static_cast<float>(vid->m_regionTileStepY)-13.0f;
}

DRAW_MAP::DRAW_MAP(float bx,float by,float sx,float sy)
{
    // Same retail RMW bitfield semantics as the SPRITE* constructor.
    optDrawBackGround=1;
    begx=bx;
    begy=by;
    sizex=sx;
    sizey=sy;
}

int DRAW_MAP::IsInside(float x,float y)
{
    return x >= begx && y >= begy && x <= begx + sizex && y <= begy + sizey;
}

float DRAW_MAP::FromScreenX(float x)
{
    return Map->SizeX() * (x - begx) / sizex;
}

float DRAW_MAP::FromScreenY(float y)
{
    return Map->SizeY() * (y - begy) / sizey;
}

float DRAW_MAP::X(float x)
{
    return begx + x * sizex / Map->SizeX();
}

float DRAW_MAP::Y(float y)
{
    return begy + y * sizey / Map->SizeY();
}

void DRAW_MAP::DrawDot(float x,float y,COLOR color)
{
    x=X(x);
    y=Y(y);
    if (IsInside(x,y))
        Graph->PutPixel(x,y,color);
}

void DRAW_MAP::DrawDot2x2(float x,float y,COLOR color)
{
    x=X(x);
    y=Y(y);
    if (IsInside(x,y))
        Graph->PutBigPixel(x,y,color);
}

void DRAW_MAP::DrawDot4x4(float x,float y,COLOR color)
{
    x=X(x);
    y=Y(y);
    if (!IsInside(x,y))
        return;

    Graph->PutBigPixel(x-2.0f,y-2.0f,color);
    Graph->PutBigPixel(x-2.0f,y,color);
    Graph->PutBigPixel(x,y-2.0f,color);
    Graph->PutBigPixel(x,y,color);
}

int DRAW_MAP::Control(INPUT* input)
{
    if (!IsInside(input->screenMouseX,input->screenMouseY))
        return 0;

    if (input->lDown)
        input->lDown=0;

    if (!input->lClick)
        return 0;

    Map->SetShiftCoor(FromScreenX(input->screenMouseX),FromScreenY(input->screenMouseY),2);
    input->lClick=0;
    return 1;
}

int DRAW_MAP::IsClickOnMap(const INPUT* input)
{
    // Original MapEdit checks the lClick bit first, then calls IsInside only
    // when the bit is set (0x0046EEE0..0x0046EF16).  Keep the two branches
    // explicit: this also avoids the VS logical/bitwise code-analysis false positive.
    if (!input->lClick)
        return 0;
    return IsInside(input->screenMouseX,input->screenMouseY);
}

void DRAW_MAP::ClickOnMap(const INPUT* input)
{
    Map->SetShiftCoor(
        Map->SizeX() * (input->screenMouseX-begx) / sizex,
        Map->SizeY() * (input->screenMouseY-begy) / sizey,
        2);
}


void DRAW_MAP::Draw()
{
    if (optDrawBackGround) {
        Graph->Bar(begx,begy,begx+sizex,begy+sizey,COLOR(170,0,0,150));
    }

    for (int i=0;i<RailMap.Dots.No();++i) {
        R_DOT* dot=*RailMap.Dots[i];
        DrawDot((float)dot->x,(float)(dot->y-dot->z),GRAPH::GRAY);
    }

    Graph->LightBar(
        X(-Map->ToScreenX(0.0f)),
        Y(-Map->ToScreenY(0.0f)),
        X(Graph->ViewXMax()-Map->ToScreenX(0.0f)-Graph->ViewXMin()),
        Y(Graph->ViewYMax()-Map->ToScreenY(0.0f)-Graph->ViewYMin()),
        0x00CFCFCFu);

    if (Map->IsMapEdit()) {
        for (int layer=0;layer<17;++layer) {
            int iterator=0;
            for (SPRITE* sprite=Map->FirstSprite(layer,&iterator);sprite;sprite=Map->NextSprite(layer,&iterator)) {
                if (!sprite->IsSpriteType(2))
                    continue;
                VID* vid=sprite->Vid();
                if (!vid->PropHash() && !vid->PropBuildVidZToGridZ() && !vid->PropBuildSizeToGridZ())
                    continue;
                DrawDot(sprite->X(),sprite->Y()-sprite->Z(),COLOR(128,128,128));
            }
        }
    }

    for (SPRITE* sprite=Hash->FirstUnit();sprite;sprite=Hash->NextUnit()) {
        const int army=sprite->Army();
        if (army==0) {
            if (sprite->IsSpriteType(8)) {
                DrawDot2x2(sprite->X(),sprite->Y()-sprite->Z(),COLOR(0,255,210));
            }
            else if (sprite->IsSpriteClass(0x15) || sprite->CanFight() || sprite->HaveFightLink()) {
                DrawDot2x2(sprite->X(),sprite->Y()-sprite->Z(),GRAPH::GREEN);
            }
            else if (sprite->IsSpriteClass(3) || sprite->IsSpriteClass(0x18)) {
                DrawDot4x4(sprite->X(),sprite->Y()-sprite->Z(),GRAPH::GREEN);
            }
            continue;
        }

        if (army==1) {
            if (sprite->IsSpriteType(8)) {
                DrawDot2x2(sprite->X(),sprite->Y()-sprite->Z(),GRAPH::LIGHTRED);
            }
            else if (sprite->IsSpriteClass(0x15) || sprite->CanFight() || sprite->HaveFightLink()) {
                DrawDot2x2(sprite->X(),sprite->Y()-sprite->Z(),GRAPH::RED);
            }
            else if (sprite->IsSpriteClass(3) || sprite->IsSpriteClass(0x18)) {
                DrawDot4x4(sprite->X(),sprite->Y()-sprite->Z(),GRAPH::RED);
            }
            continue;
        }

        if (sprite->IsSpriteClass(0x15)) {
            DrawDot2x2(sprite->X(),sprite->Y()-sprite->Z(),GRAPH::YELLOW);
        }
        else if (sprite->IsSpriteClass(3) || sprite->IsSpriteClass(0x18)) {
            DrawDot4x4(sprite->X(),sprite->Y()-sprite->Z(),GRAPH::YELLOW);
        }
    }
}
