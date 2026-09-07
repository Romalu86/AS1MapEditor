#include "mapedit/runtime.hpp"

// Original global storage referenced at VA 0x004CE378 by SetControlPanel.
float g_controlPanelPreviousViewXMax=0.0f;

// The four VID offsets used here are backed by the original MapEdit accesses:
// +0x1C/+0x20 are the grid footprint steps and +0x384/+0x388 are the snap offsets.
void MAP_EDIT::ChangeMouseCoorWithSnap()
{
    int x=(int)Mouse->X();
    int y=(int)Mouse->Y();
    int stepX=(int)Mouse->Vid()->m_footprintWidth;
    int stepY=(int)Mouse->Vid()->m_footprintHeight;

    if (stepY)
        y/=stepY;

    if (stepX) {
        x/=stepX;
        int chessOffset=0;
        if (optChessSnap && (y&1))
            chessOffset=(int)Mouse->Vid()->m_snapOffsetX;
        x=x*stepX+(int)Mouse->Vid()->m_snapOffsetX-chessOffset;
    }

    if (stepY)
        y=y*stepY+(int)Mouse->Vid()->m_snapOffsetY;

    Mouse->ChangeCoor((float)(x+optShiftSnapX),(float)(y+optShiftSnapY),Mouse->Z());
}

int MAP_EDIT::Left()
{
    if (optTacticMode) {
        GROUP* group=m_groups.First();
        if (group) {
            selectedSprites.Release();
            m_groups.ShiftFirstLeft();
            group=m_groups.First();
            for (int i=0;i<group->No();++i)
                selectedSprites.Insert(*(*group)[i]);
            if (group->No())
                SetShiftCoor(group->X(),group->Y(),0);
        }
    } else if (spriteType==0x40) {
        if (curRegion) {
            int i;
            if (curRegion!=FirstSprite(10,&i)) {
                REGION* previous=0;
                SPRITE* sprite=FirstSprite(10,&i);
                while (sprite && sprite!=curRegion) {
                    if (sprite->IsSpriteClass(0x17))
                        previous=(REGION*)sprite;
                    sprite=NextSprite(10,&i);
                }
                if (previous) {
                    curRegion=previous;
                    SetShiftCoor(curRegion->X(),curRegion->Y(),0);
                }
            }
        }
        if (curRegion)
            return curRegion->Vid()->m_idx;
    } else {
        return PrevVid(Mouse->Vid()->m_idx,(unsigned int)spriteType);
    }

    return Mouse->Vid()->m_idx;
}

int MAP_EDIT::Right()
{
    if (optTacticMode) {
        GROUP* group=m_groups.First();
        if (group) {
            selectedSprites.Release();
            m_groups.ShiftFirstRight();
            group=m_groups.First();
            for (int i=0;i<group->No();++i)
                selectedSprites.Insert(*(*group)[i]);
            SetShiftCoor(group->X(),group->Y(),0);
        }
    } else if (spriteType==0x40) {
        SPRITE* sprite=(SPRITE*)curRegion;
        int i=0;
        if (sprite) {
            i=m_layers[10].Location(reinterpret_cast<SPRITE* const*>(&sprite));
            do {
                sprite=NextSprite(10,&i);
                if (!sprite || sprite->IsSpriteClass(0x17))
                    break;
            } while (1);
        }

        if (!sprite) {
            sprite=FirstSprite(10,&i);
            while (sprite && !sprite->IsSpriteClass(0x17))
                sprite=NextSprite(10,&i);
        }

        if (sprite) {
            curRegion=(REGION*)sprite;
            SetShiftCoor(curRegion->X(),curRegion->Y(),0);
        }
        if (curRegion)
            return curRegion->Vid()->m_idx;
    } else {
        return NextVid(Mouse->Vid()->m_idx,(unsigned int)spriteType);
    }

    return Mouse->Vid()->m_idx;
}

void MAP_EDIT::SetControlPanel(int flag)
{
    if (optControlPanel && !flag) {
        if (hControlPanel)
            DestroyWindow(hControlPanel);
        hControlPanel=0;

        RECT_OLD toolbarRect;
        GetWindowRect(hToolBar,&toolbarRect);
        Graph->SetViewPort(Graph->ViewXMin(),Graph->ViewYMin(),
                           g_controlPanelPreviousViewXMax,Graph->ViewYMax());
    } else if (!optControlPanel && flag) {
        g_controlPanelPreviousViewXMax=Graph->ViewXMax();
        hControlPanel=CreateDialogParamA(m_instance,"CONTROL_PANEL",m_hWnd,AppControlPanel,0);

        RECT_OLD clientRect;
        RECT_OLD toolbarRect;
        RECT_OLD panelRect;
        GetClientRect(m_hWnd,&clientRect);
        GetWindowRect(hToolBar,&toolbarRect);
        GetWindowRect(hControlPanel,&panelRect);

        const int panelWidth=panelRect.right-panelRect.left;
        const int toolbarHeight=toolbarRect.bottom-toolbarRect.top;
        const int x=(int)Graph->SizeX()-(int)Graph->ViewXMin()-panelWidth;
        const int y=clientRect.top+toolbarHeight-2;
        const int height=(int)Graph->SizeY()+2-(int)Graph->ViewYMin();
        SetWindowPos(hControlPanel,0,x,y,panelWidth,height,0x40);

        Graph->SetViewPort(Graph->ViewXMin(),Graph->ViewYMin(),
                           Graph->SizeX()-(float)panelWidth,Graph->ViewYMax());
        SetFocus(m_hWnd);
        EnableWindow(hControlPanel,0);
    }

    optControlPanel=flag!=0;
}
