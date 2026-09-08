#include "mapedit/runtime.hpp"

// Original mutable router cursors used by MAP_EDIT::WorkWndMessage.
// Canonical storage in the retail image: 0x004CE358 and 0x004BCBDC.
int g_editorSelectedSpriteCycleIndex=0;
int g_editorMatchingUnitCycleIndex=0;

namespace {
struct TOOLTIP_DISPINFO_OLD {
    HWND__* hwndFrom;       // +0x00
    unsigned int idFrom;    // +0x04
    unsigned int code;      // +0x08
    char* text;             // +0x0C
};
static_assert(sizeof(TOOLTIP_DISPINFO_OLD)==0x10,"Win32 tooltip notify x86 ABI");

char g_editorTooltipText[256];

const unsigned int kToolbarEnableButton=0x401;
const unsigned int kToolbarCheckButton =0x402;
}

// The original compiler emits the WM_COMMAND switch as a jump table with
// 118 command ids collapsing to 50 real handlers.  Keep all retail side
// effects in this owner instead of hiding missing logic behind router stubs.
int MAP_EDIT::WorkWndMessage(HWND__* hwnd,unsigned long msg,unsigned long wParam,unsigned long lParam)
{
    if (msg==0x4Eu) { // WM_NOTIFY
        TOOLTIP_DISPINFO_OLD* tip=reinterpret_cast<TOOLTIP_DISPINFO_OLD*>(lParam);
        if (tip && tip->code==0xFFFFFDF8u) { // TTN_NEEDTEXTA (-520)
            LoadStringA(m_instance,tip->idFrom,g_editorTooltipText,256);
            tip->text=g_editorTooltipText;
        }
        return 0;
    }

    if (msg==0x111u) { // WM_COMMAND
        const unsigned int command=static_cast<unsigned int>(wParam)&0xFFFFu;
        switch (command) {
        case 0xA097: { // copy current mouse world coordinate
            Printf("%i,%i,%i",(int)Mouse->X(),(int)Mouse->Y(),(int)Mouse->Z()).WriteToClipboard(m_hWnd);
            break;
        }
        case 0xA095: { // paste map centre coordinate
            STRING str;
            float x=0.0f,y=0.0f;
            str.ReadFromClipboard(m_hWnd);
            sscanf(str.CharPtr(),"%f,%f",&x,&y);
            SetShiftCoor(x,y,2);
            break;
        }
        case 0xA0A1: { // cycle selected sprite
            if (optTacticMode && selectedSprites.No()>0) {
                ++g_editorSelectedSpriteCycleIndex;
                if (g_editorSelectedSpriteCycleIndex>=selectedSprites.No())
                    g_editorSelectedSpriteCycleIndex=0;
                SPRITE* spr=*selectedSprites[g_editorSelectedSpriteCycleIndex];
                SetShiftCoor(spr->X(),spr->Y(),2);
            }
            break;
        }
        case 0xA0A2: { // hide selected VID / mouse VID
            if (optTacticMode && selectedSprites.No()>0) {
                for (int i=0;i<selectedSprites.No();++i)
                    (*selectedSprites[i])->Vid()->SetPropHide(1);
            } else if (Mouse && Mouse->Vid()) {
                VID* vid=Mouse->Vid();
                vid->SetPropHide(!vid->PropHide());
            }
            break;
        }
        case 0xA09E: { // select all sprites of current editor class
            if (optTacticMode) {
                POLYGON polygon;
                polygon.CreateBox(0.0f,0.0f,m_w,m_h);
                FindSpritesInsidePolygon(spriteType<<20,&polygon,&selectedSprites);
            }
            break;
        }
        case 0xA09F: { // select current-class sprites inside view
            if (optTacticMode) {
                POLYGON polygon;
                polygon.CreateBox(Graph->ViewXMin()+m_shiftX,Graph->ViewYMin()+m_shiftY,
                                  Graph->ViewXMax()+m_shiftX,Graph->ViewYMax()+m_shiftY);
                FindSpritesInsidePolygon(spriteType<<20,&polygon,&selectedSprites);
            }
            break;
        }
        case 0xA0A0: { // select all sprites having selected VID
            if (optTacticMode && selectedSprites.No()>0) {
                POLYGON polygon;
                polygon.CreateBox(0.0f,0.0f,m_w,m_h);
                const int type=MAP::EncodeVidQuery((*selectedSprites[0])->Vid()->m_idx);
                FindSpritesInsidePolygon(type,&polygon,&selectedSprites);
            }
            break;
        }
        case 0xA09D: { // select selected-VID sprites inside view
            if (optTacticMode && selectedSprites.No()>0) {
                POLYGON polygon;
                polygon.CreateBox(Graph->ViewXMin()+m_shiftX,Graph->ViewYMin()+m_shiftY,
                                  Graph->ViewXMax()+m_shiftX,Graph->ViewYMax()+m_shiftY);
                const int type=MAP::EncodeVidQuery((*selectedSprites[0])->Vid()->m_idx);
                FindSpritesInsidePolygon(type,&polygon,&selectedSprites);
            }
            break;
        }
        case 0x9C64:
            { STRING dialogName("CONVERTSPRITE"); CallDialogBox(&dialogName,AppConvertSprite); }
            break;
        case 0xB02A:
            { STRING dialogName("OPTIONS"); CallDialogBox(&dialogName,AppOptions); }
            break;
        case 0xB01F:
            { STRING dialogName("MAP_PROPERTY"); CallDialogBox(&dialogName,AppMapProperty); }
            break;
        case 0x9C93:
            { STRING dialogName("SELECT_VID"); CallDialogBox(&dialogName,AppSelectVid); }
            break;
        case 0x9C89:
            if (selectedSprites.No()>0 && (*selectedSprites[0])->IsSpriteClass(0x13u))
                { STRING dialogName("TEXT_PROPERTY"); CallDialogBox(&dialogName,AppTextProperty); }
            else if (spriteType&0x40)
                { STRING dialogName("REGION_PROPERTY"); CallDialogBox(&dialogName,AppRegionProperty); }
            else
                { STRING dialogName("UNIT_PROPERTY"); CallDialogBox(&dialogName,AppUnitProperty); }
            break;
        case 0x9C8E:
            undo.Undo();
            break;
        case 0xB024:
            undo.Redo();
            break;
        case 0x9C42:
            undo.Reset();
            Save(editFileName);
            break;
        case 0x9C8B: { // save/export
            // Retail .data block at 0x004BC088: OPENFILENAMEA requires one
            // double-NUL terminated label/pattern chain, not a plain label.
            static const char kSaveFilter[] =
                "Map file\0*.map\0"
                "Menu file\0*.men\0"
                "AllMap(tga) file\0*.tga\0"
                "GridZ(tga) file\0*.bmp\0\0";
            STRING newName=SaveDialog(kSaveFilter);
            if (newName=="")
                break;
            STRING extPart=newName.AfterLast(".");
            STRING ext=extPart.ToLower();
            if (ext=="tga") {
                PICTURE pict((int)SizeX(),(int)SizeY(),PICTURE::TYPE_TGA);
                m_input.ChangeCoor(600.0f,600.0f);
                for (int y=0;y<(int)SizeY();y+=256) {
                    for (int x=0;x<(int)SizeX();x+=256) {
                        m_shiftX=(float)x-Graph->ViewXMin();
                        m_shiftY=(float)y-Graph->ViewYMin();
                        Graph->ClearScreen(COLOR(0,0,0));
                        Graph->PreTact();
                        Graph->Tact(1);
                        Graph->SavePict(&pict,x,y,(int)Graph->ViewXMin(),(int)Graph->ViewYMin(),256,256);
                        Graph->PostTact(1);
                    }
                }
                pict.SaveTGA(&newName,0,0,-1,-1);
                pict.Close();
            } else if (ext=="bmp") {
                PICTURE pict((int)SizeX()/8,(int)SizeY()/8,PICTURE::TYPE_TGA);
                for (int y=0;y<(int)SizeY();y+=8) {
                    for (int x=0;x<(int)SizeX();x+=8) {
                        const int z=(int)GetGroundZ((float)x,(float)y);
                        pict.PutPixel(x/8,y/8,COLOR(0,z/256,z));
                    }
                }
                newName=newName.ToLower();
                newName.Replace(".bmp",".tga");
                pict.SaveTGA(&newName,0,0,-1,-1);
                pict.Close();
            } else if (ext=="men") {
                m_menu.Save(&newName);
            } else {
                m_mapName=newName;
                editFileName=m_mapName;
                undo.Reset();
                Save(m_mapName);
            }
            DrawMapName();
            break;
        }
        case 0x9C41:
        case 0xB01C: { // open/import
            // Retail .data block at 0x004BC0F8.
            static const char kOpenFilter[] =
                "Map files\0*.map\0"
                "Menu files\0*.men\0"
                "Terrain files\0*.vid\0"
                "GridZ files\0*.tga;*.z\0"
                "All Files\0*.*\0\0";
            STRING name=OpenDialog(kOpenFilter);
            if (name=="")
                break;
            STRING extPart=name.AfterLast(".");
            STRING ext=extPart.ToLower();
            if (ext=="tga" || ext=="z") {
                PICTURE pict;
                if (pict.Load(&name)!=0) {
                    ::Error->Window("grid error:can't load %s",name.CharPtr());
                    pict.Close();
                    break;
                }
                ResetGroundZ();
                const int mapSizeX=(int)SizeX();
                const int mapSizeY=(int)SizeY();
                for (int y=0;y<mapSizeY;y+=8) {
                    for (int x=0;x<mapSizeX;x+=8) {
                        const float xz=(float)pict.SizeX()*(float)x/(float)mapSizeX;
                        const float yz=(float)pict.SizeY()*(float)y/(float)mapSizeY;
                        if (pict.IsZ()) {
                            SetGroundZ((float)x,(float)y,(float)((int)pict.GetData((int)xz,(int)yz))/8.0f);
                        } else {
                            COLOR color=pict.GetPixel((int)xz,(int)yz);
                            SetGroundZ((float)x,(float)y,(float)color.Green()*256.0f+(float)color.Blue());
                        }
                    }
                }
                pict.Close();
            } else if (ext=="vid") {
                LoadTerrain(name);
            } else if (ext=="men") {
                m_menu.Load(&name);
            } else {
                Load(name);
            }
            break;
        }
        case 0x9C4E:
            undo.Begin();
            FillBox(0.0f,0.0f,SizeX(),SizeY());
            undo.End();
            break;
        case 0xB027: { // remove exact duplicate sprites
            undo.Begin();
            for (int layer=0;layer<18;++layer) {
                int i=0;
                SPRITE* spr=FirstSprite(layer,&i);
                while (spr) {
                    int j=i;
                    SPRITE* spr2=NextSprite(layer,&j);
                    while (spr2) {
                        if (spr->Vid()==spr2->Vid() &&
                            spr->Direction().value==spr2->Direction().value &&
                            spr->X()==spr2->X() && spr->Y()==spr2->Y() && spr->Z()==spr2->Z()) {
                            undo.AddRemove(spr);
                            spr=spr2;
                        }
                        spr2=NextSprite(layer,&j);
                    }
                    spr=NextSprite(layer,&i);
                }
            }
            undo.End();
            break;
        }
        case 0x9C86: { // remove all instances of current mouse VID
            undo.Begin();
            int i=0;
            SPRITE* spr=FirstSprite(Mouse->Vid()->m_layer,&i);
            while (spr) {
                if (spr->Vid()==Mouse->Vid())
                    undo.AddRemove(spr);
                spr=NextSprite(Mouse->Vid()->m_layer,&i);
            }
            undo.End();
            break;
        }
        case 0xA08F:
            ResetGroundZ();
            break;
        case 0x9C85:
            m_groups.DeleteAll();
            break;
        case 0xB01E:
            DeleteExtraVid();
            break;
        case 0x9C8D:
            m_menu.DeleteAll();
            break;
        case 0xB01D:
            optGround0=!optGround0;
            break;
        case 0x9C59:
            optShiftSnapX=0;
            optShiftSnapY=0;
            optSnap=!optSnap;
            SendMessageA(hToolBar,kToolbarCheckButton,0x9C59,(long)optSnap);
            break;
        case 0x9C62:
            optChessSnap=!optChessSnap;
            break;
        case 0xB029:
            optAirBrush=!optAirBrush;
            break;
        case 0xB02B:
            optRandomDir=!optRandomDir;
            break;
        case 0x9C75:
            optDelete=!optDelete;
            break;
        case 0x9C7E:
            ChangeMouseVid(Vid(Right()),(unsigned int)spriteType);
            break;
        case 0x9C7D:
            ChangeMouseVid(Vid(Left()),(unsigned int)spriteType);
            break;
        case 0xA093:
            SetControlPanel(!optControlPanel);
            break;
        case 0xA091:
            SetShiftCoor(SizeX()/2.0f,SizeY()/2.0f,0);
            break;
        case 0xA092:
            m_input.ChangeCoor(Graph->SizeX()/2.0f,Graph->SizeY()/2.0f);
            break;
        case 0x9C6F:
            ChangeMouseVid(Mouse->Vid(),1u);
            break;
        case 0x9C70:
            ChangeMouseVid(Mouse->Vid(),2u);
            break;
        case 0x9C71:
            ChangeMouseVid(Mouse->Vid(),4u);
            break;
        case 0x9C72:
            ChangeMouseVid(Mouse->Vid(),8u);
            break;
        case 0x9C73:
            ChangeMouseVid(Mouse->Vid(),16u);
            break;
        case 0xB022:
            ChangeMouseVid(Vid(Right()),0x40u);
            break;
        case 0xB023:
            ChangeMouseVid(Mouse->Vid(),0x20u);
            break;
        case 0x9C60: { // toggle tactic mode and related toolbar state
            optTacticMode=!optTacticMode;
            const int normalEnable=!optTacticMode;
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C59,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C62,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C75,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0xB01D,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0xB029,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0xB02B,normalEnable);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C89,optTacticMode);
            SendMessageA(hToolBar,kToolbarEnableButton,0x9C83,optTacticMode);
            if (optTacticMode) {
                savedVid=Mouse->Vid();

                // A21 editor stability fix. Retail MOUSE::HardwareOn() changes
                // m_vid directly to EmptyVid and intentionally does not touch the
                // software cursor LinkVid chain. If ChangeMouseVid() is then used
                // while tactical mode is active, SPRITE::Action(ACT_CHANGE_VID)
                // can no longer identify that old chain from EmptyVid and a new
                // chain is prepended to it. On HardwareOff() MAP::DrawLayer() then
                // renders the stale children as unrelated NVID assets.
                //
                // Keep all derived MOUSE/SPRITE owners behavior-compatible and normalize
                // only this editor-mode boundary, while savedVid still identifies
                // the chain that legitimately belongs to the software cursor.
                for (VID* link=savedVid ? savedVid->m_linkVid : 0;
                     link;
                     link=link->m_linkVid) {
                    Mouse->DestroyLink(link);
                }

                Mouse->HardwareOn();
            } else {
                Mouse->HardwareOff();
                ChangeMouseVid(savedVid,savedVid->m_unknown0C);
            }
            break;
        }
        case 0x9C83: { // create/delete current selection group
            GROUP* first=m_groups.First();
            if (selectedSprites.IsEqual(first)) {
                delete first;
                break;
            }
            if (selectedSprites.No()==0)
                break;
            SPRITE* spr=*selectedSprites[0];
            m_groups.DeletePointerToSprite(spr);
            GROUP* group=m_groups.CreateNewGroup(spr);
            for (int i=1;i<selectedSprites.No();++i) {
                spr=*selectedSprites[i];
                m_groups.DeletePointerToSprite(spr);
                group->Insert(spr);
            }
            break;
        }
        case 0x9C92: { // find next unit using current mouse VID
            SPRITE* spr=Hash->NextUnit(&g_editorMatchingUnitCycleIndex);
            if (!spr)
                spr=Hash->FirstUnit(&g_editorMatchingUnitCycleIndex);
            while (spr && spr->Vid()!=Mouse->Vid()) {
                spr=Hash->NextUnit(&g_editorMatchingUnitCycleIndex);
                if (!spr)
                    spr=Hash->FirstUnit(&g_editorMatchingUnitCycleIndex);
            }
            if (spr)
                SetShiftCoor(spr->X(),spr->Y(),2);
            break;
        }
        case 0xB026: { // rebuild ground height from rendered Z buffer
            const int noX=(int)SizeX()/8;
            const int noY=(int)SizeY()/8;
            const unsigned int cells=(unsigned int)(noX*noY);
            int* data=static_cast<int*>(malloc(cells*sizeof(int)));
            if (!data) {
                ::Error->Window("Enough memory");
                break;
            }
            int* count=static_cast<int*>(malloc(cells*sizeof(int)));
            if (!count) {
                // Retail owner returns here without freeing data.
                ::Error->Window("Enough memory");
                break;
            }
            memset(data,0,cells*sizeof(int));
            memset(count,0,cells*sizeof(int));
            m_input.ChangeCoor(600.0f,600.0f);
            for (int tileY=0;tileY<(int)SizeY();tileY+=256) {
                for (int tileX=0;tileX<(int)SizeX();tileX+=256) {
                    m_shiftX=(float)tileX-Graph->ViewXMin();
                    m_shiftY=(float)tileY-Graph->ViewYMin();
                    Graph->ClearScreen(COLOR(0,0,0));
                    Graph->PreTact();
                    Graph->Tact(1);
                    int pitch=0;
                    unsigned short* zbuffer=Graph->LockZ(&pitch);
                    for (int localY=0;localY<256;++localY) {
                        for (int localX=0;localX<256;++localX) {
                            const int mapX=tileX+localX;
                            const int mapY=tileY+localY;
                            if (!ValidateXY((float)mapX,(float)mapY))
                                continue;
                            const int screenX=localX+(int)Graph->ViewXMin();
                            const int screenY=localY+(int)Graph->ViewYMin();
                            const int z=(int)zbuffer[screenX+screenY*pitch]/8-128;
                            const int yy=mapY+z;
                            if (!ValidateXY((float)mapX,(float)yy))
                                continue;
                            const int cellX=mapX/8;
                            const int cellY=yy/8;
                            const int cell=cellY*noX+cellX;
                            if (z>data[cell])
                                data[cell]=z;
                            count[cell]=1;
                        }
                    }
                    Graph->UnLockZ();
                    Graph->PostTact(1);
                }
            }
            for (int y=0;y<noY;++y) {
                for (int x=0;x<noX;++x) {
                    const int cell=y*noX+x;
                    if (count[cell]) {
                        data[cell]/=count[cell];
                        if (data[cell]>m_groundz[cell])
                            m_groundz[cell]=(short)data[cell];
                    }
                }
            }
            free(data);
            free(count);
            break;
        }
        case 0xB028: { // bake visible ground into hardware_ground.vid
            PICTURE_MAKEVID pict((int)SizeX(),(int)SizeY(),5u);
            GAMMA gamma=Graph->GetGamma();
            GAMMA neutral;
            Graph->SetGamma(&neutral);
            m_input.ChangeCoor(600.0f,600.0f);
            for (int y=0;y<(int)SizeY();y+=256) {
                for (int x=0;x<(int)SizeX();x+=256) {
                    m_shiftX=(float)x-Graph->ViewXMin();
                    m_shiftY=(float)y-Graph->ViewYMin();
                    Graph->ClearScreen(COLOR(0,0,0));
                    Graph->PreTact();
                    Graph->Tact(1);
                    Graph->SavePictAndZ(&pict,x,y,(int)Graph->ViewXMin(),(int)Graph->ViewYMin(),256,256);
                    Graph->PostTact(1);
                }
            }
            Graph->SetGamma(&gamma);
            const int err=pict.MakeVid(1u,STRING("hardware_ground.vid"));
            pict.Close();
            if (err)
                break;

            int i=0;
            for (;i<m_noVid;++i) {
                VID* vid=VidSlot(i);
                if (vid && vid->IsExtraType() && i==0x400) {
                    STRING* terrainName=&vid->m_resourceName;
                    FRemove(terrainName);
                    STRING baked("hardware_ground.vid");
                    FRename(&baked,terrainName);
                    break;
                }
            }
            if (i>=m_noVid) {
                STRING filename=editFileName;
                STRING dir=FCurrentDirectory();
                filename.Replace(".map",".vid");
                STRING empty("");
                filename.Replace(&dir,&empty);
                filename.RemoveBeginChars("\\");
                FRemove(&filename);
                STRING baked("hardware_ground.vid");
                FRename(&baked,&filename);
                LoadTerrain(filename);
            }

            for (int layer=0;layer<18;++layer) {
                int index=0;
                SPRITE* spr=FirstSprite(layer,&index);
                while (spr) {
                    if (!spr->Vid()->PropHide() && spr->Vid()->m_idx!=0x400)
                        spr->ScalarDeletingDestructor(1u);
                    spr=NextSprite(layer,&index);
                }
            }
            undo.Reset();
            Save(editFileName);
            Load(editFileName);
            break;
        }
        case 0x9C44:
            SendMessageA(hwnd,0x10u,0u,0); // WM_CLOSE
            break;
        default:
            // Retail jump table maps the remaining command IDs to its common no-op return.
            break;
        }
        return 0;
    }

    if (msg==0x84u) { // WM_NCHITTEST: show editor control panel only over its viewport strip
        RECT_OLD rect;
        GetWindowRect(m_hWnd,&rect);
        const float screenX=(float)((unsigned int)lParam&0xFFFFu)-(float)rect.left;
        const float screenY=(float)(((unsigned int)lParam>>16)&0xFFFFu)-(float)rect.top;
        if (screenX>Graph->ViewXMax() && screenY>Graph->ViewYMin()) {
            m_input.screenMouseX=screenX;
            m_input.screenMouseY=screenY;
            EnableWindow(hControlPanel,1);
        } else if (EnableWindow(hControlPanel,0)==0) {
            SetFocus(m_hWnd);
        }
    }

    return MAP::WorkWndMessage(hwnd,msg,wParam,lParam);
}
