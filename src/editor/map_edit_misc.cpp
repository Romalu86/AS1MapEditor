#include "mapedit/runtime.hpp"

void MAP_EDIT::DrawMapName()
{
    MENUITEMINFOA_OLD info;
    memset(&info,0,sizeof(info));
    info.cbSize=sizeof(info);
    info.fMask=0x10;       // MIIM_TYPE in the SDK generation used by the original build.
    info.fType=0x4000;     // MFT_STRING.
    info.dwTypeData=editFileName.CharPtr();
    info.cch=(uint32_t)editFileName.Length();
    SetMenuItemInfoA(GetMenu(m_hWnd),0xB01C,0,&info);
    DrawMenuBar(m_hWnd);
}

void MAP_EDIT::DeletePointerToSprite(SPRITE* sprite)
{
    if (sprite->NoRef()>1)
        selectedSprites.Delete(sprite);
    if (static_cast<SPRITE*>(curRegion)==sprite)
        curRegion=0;
    MAP::DeletePointerToSprite(sprite);
}

void MAP_EDIT::DrawSecondaryInfo()
{
    MAP::DrawSecondaryInfo();

    Graph->PrintfXY(200.0f,Graph->ViewYMin(),"%-8i %-8i",(int)m_input.mouseX,(int)m_input.mouseY);
    Graph->PrintfXY(300.0f,Graph->ViewYMin(),"%6i+%i",(int)insertZ,
                    (int)GetGroundZ(Mouse->Vid(),m_input.mouseX,m_input.mouseY));
    Graph->PrintfXY(400.0f,Graph->ViewYMin(),"%i",Graph->RealZBuffer(m_input.screenMouseX,m_input.screenMouseY));

    if (spriteType==0x40) {
        int i;
        for (SPRITE* sprite=FirstSprite(10,&i);sprite;sprite=NextSprite(10,&i)) {
            if (sprite->IsSpriteClass(0x17))
                sprite->DrawSecondaryInfo();
        }
        if (curRegion) {
            Graph->Box(curRegion->ScreenLeft()-1.0f,curRegion->ScreenTop()-1.0f,
                       curRegion->ScreenRight()+1.0f,curRegion->ScreenBottom()+1.0f,GRAPH::RED);
        }
    }

    Graph->Box(ToScreenX(0.0f)-1.0f,ToScreenY(0.0f)-1.0f,
               ToScreenX(SizeX()-1.0f)+1.0f,ToScreenY(SizeY()-1.0f)+1.0f,COLOR(255,255,255));
}

void MAP_EDIT::Release()
{
    MYERROR::Log(::Error,"Undo   release");
    undo.Reset();
    MYERROR::Log(::Error,"Select release");
    selectedSprites.Release();
    curRegion=0;
    MAP::Release();
}

void MAP_EDIT::Load(STRING name)
{
    undo.Reset();
    MAP::Load(name);
    SetScrollBox(-450.0f,-450.0f,SizeX()+450.0f,SizeY()+450.0f);

    if (g_editorUnknownFlag4BCBE0) {
        g_editorUnknownFlag4BCBE0=0;
        STRING defaultLogic("maps\\default.lgc");
        m_logic.Load(&defaultLogic);
        m_logic.CallFunction(-1,0,0,0);
        m_logic.Release();
    }

    editFileName=m_mapName;
    DrawMapName();
}
