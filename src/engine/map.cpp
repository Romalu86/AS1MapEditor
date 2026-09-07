#include "mapedit/runtime.hpp"

extern "C" unsigned int __stdcall timeEndPeriod(unsigned int period);

namespace {
unsigned int g_demoOldCurrentTime=0;
unsigned int g_demoOldAbsoluteTime=0;
}

int MAP::IsInitSuccess()
{
    return (m_flags >> 2) & 1;
}

float MAP::SizeX() { return m_w; }
float MAP::SizeY() { return m_h; }


int MAP::DemoTact()
{
    int result=1;

    if (m_flags&0x200u) {
        int time=-1;
        m_resource.Read(&time,4u);

        if (time==-1 || m_input.key!=0 || m_input.lClick || m_input.rClick) {
            STRING nextMap;
            m_resource.GoNext(0x4F4D4544u); // DEMO
            m_resource.Shift(m_resource.ResSize());
            nextMap.Read(&m_resource);

            if (nextMap=="") {
                PostMessageA(m_hWnd,0x10u,0,0); // WM_CLOSE
            } else {
                m_resource.Close();
                Mouse->Enable();
                m_flags&=~0x200u;
                LoadInEndTact(&nextMap);
            }
            return 0;
        }

        m_input.Load(&m_resource);

        const unsigned int demoTime=static_cast<unsigned int>(time);
        if (demoTime-g_demoOldCurrentTime>0x47u)
            g_demoOldCurrentTime=demoTime-0x47u;

        const unsigned int target=demoTime-g_demoOldCurrentTime;
        const unsigned int elapsed=timeGetTime()-g_demoOldAbsoluteTime;
        if (target>elapsed) {
            // Retail MapEdit intentionally busy-spins; there is no Sleep call.
            while (target>=timeGetTime()-g_demoOldAbsoluteTime) {
            }
        } else {
            result=(CurrentTime-demoTime<=0x14u) ? 1 : 0;
        }

        CurrentTime=demoTime;
        g_demoOldCurrentTime=CurrentTime;
        g_demoOldAbsoluteTime=timeGetTime();
    }

    if (m_flags&0x100u) {
        m_resource.Write(&CurrentTime,4u);
        m_input.Save(&m_resource);
    }

    return result;
}

void MAP::NetworkTact()
{
}

int MAP::ValidateXY(float x, float y)
{
    return x >= 0.0f && x < m_w && y >= 0.0f && y < m_h;
}

SPRITE* MAP::NextSprite(int layer, int* index)
{
    for (;;) {
        --*index;
        if (*index < 0)
            return 0;
        SPRITE* sprite = *m_layers[layer][*index];
        if (sprite)
            return sprite;
    }
}

STRING MAP::OpenDialog(const char* filter)
{
    return OpenSaveDialog(0,filter);
}

STRING MAP::SaveDialog(const char* filter)
{
    return OpenSaveDialog(1,filter);
}

// The retail owner uses the Win32 4.00 (0x4C-byte) OPENFILENAMEA layout,
// pauses the renderer around the modal common dialog and returns an empty
// STRING when the user cancels.
STRING MAP::OpenSaveDialog(int save,const char* filter)
{
    char fileName[4096];
    memset(fileName,0,sizeof(fileName));

    OPENFILENAMEA_OLD ofn = {};
    ofn.lStructSize=0x4C;
    ofn.hwndOwner=m_hWnd;
    ofn.hInstance=m_instance;
    ofn.lpstrFilter=filter;
    ofn.nFilterIndex=1;
    ofn.lpstrFile=fileName;
    ofn.nMaxFile=0x1000;
    ofn.lpstrInitialDir="maps";
    ofn.Flags=0x0008080Cu;
    if (!save)
        ofn.Flags|=0x00001000u;
    ofn.lpstrDefExt="map";

    Graph->BeginPause();
    const int accepted=save ? GetSaveFileNameA(&ofn) : GetOpenFileNameA(&ofn);
    Graph->EndPause();

    return STRING(accepted ? fileName : "");
}

void REGION::SetSize(float width,float height)
{
    sizeX=width;
    sizeY=height;
}

float MAP::GetGroundZ(float x,float y)
{
    if (x < 0.0f)
        x=0.0f;
    else if (x >= m_w)
        x=(m_w-1.0f)/8.0f;
    else
        x/=8.0f;

    if (y < 0.0f)
        y=0.0f;
    else if (y >= m_h)
        y=(m_h-1.0f)/8.0f;
    else
        y/=8.0f;

    const int index=(int)x + (int)y*m_groundW;
    const short ground=m_groundz[index];
    const short temporary=m_tempGroundz[index];
    return (float)(ground > temporary ? ground : temporary);
}

float MAP::GetGroundZScr(float screenX,float screenY)
{
    if (screenX<0.0f)
        screenX=0.0f;
    else if (screenX>=m_w)
        screenX=(m_w-1.0f)/8.0f;
    else
        screenX/=8.0f;

    if (screenY<0.0f)
        screenY=0.0f;
    else if (screenY>=m_h)
        screenY=(m_h-1.0f)/8.0f;
    else
        screenY/=8.0f;

    const int baseY=static_cast<int>(screenY);
    int row=baseY+32;
    if (row>=m_groundH)
        row=m_groundH-1;

    int index=static_cast<int>(screenX)+row*m_groundW;
    for (;row>=baseY;--row,index-=m_groundW) {
        const int z=(row-baseY)*8;
        if (static_cast<int>(m_groundz[index])>=z ||
            static_cast<int>(m_tempGroundz[index])>=z)
            return static_cast<float>(z);
    }
    return 0.0f;
}

int MAP::IsMapEdit()
{
    return m_flags & 1u;
}

int MAP::OptLoad()
{
    return (m_flags >> 5) & 1u;
}

MENU* MAP::Menu()
{
    return &m_menu;
}


SPRITE* MAP::FirstSprite(int nlayer,int* index)
{
    *index = m_layers[nlayer].No();
    return NextSprite(nlayer,index);
}

int MAP::ValidateVid(int nvid)
{
    return nvid >= 0 && nvid < m_noVid && m_vids[nvid] != 0;
}

VID* MAP::Vid(int nvid)
{
    return ValidateVid(nvid) ? m_vids[nvid] : EmptyVid;
}

float MAP::FromScreenX(float screenX) { return screenX + m_shiftX; }
float MAP::FromScreenY(float screenY) { return screenY + m_shiftY; }


// Retail performs coarse integer culling for ordinary layers, but layers 0
// and 10 are drawn without that cull.  The software mouse/link chain is
// drawn after the map layer when it is not a hardware/always-top cursor.
void MAP::DrawLayer(int layer)
{
    int index=0;
    int centerX=0;
    int centerY=0;

    if (layer!=0 && layer!=10) {
        centerX=static_cast<int>(FromScreenX(Graph->SizeX()/2.0f));
        centerY=static_cast<int>(FromScreenY(Graph->SizeY()/2.0f));
    }

    for (SPRITE* sprite=FirstSprite(layer,&index); sprite; sprite=NextSprite(layer,&index)) {
        if (sprite->IsInvisible())
            continue;

        int draw=(layer==0 || layer==10);
        if (!draw) {
            const int dx=static_cast<int>(sprite->X())-centerX+0x400;
            const int yz=static_cast<int>(sprite->Y()-sprite->Z())-centerY+0x200;
            draw=(((dx & ~0x7ff)==0) && ((yz & ~0x3ff)==0)) ||
                 (static_cast<int>(sprite->Y())-centerY>=0x200);
        }

        if (draw)
            sprite->Draw();
    }

    if (!Mouse->IsHardware() && !Mouse->Vid()->PropAlwaysTop()) {
        for (SPRITE* sprite=Mouse; sprite; sprite=sprite->Link()) {
            if (sprite->Vid()->m_layer==layer && !sprite->IsInvisible())
                sprite->Draw();
        }
    }
}

// Exact editor variant: viewport clamp, effect-2 capture, then shift MENU,
// MOUSE and INPUT world coordinates.  The newer-reference terrain-camera /
// Flagman branch is intentionally absent because it is not in MapEdit.exe.
void MAP::SetShiftCoor(float center_x,float center_y,int effect)
{
    float shift_x=center_x-Graph->SizeX()/2.0f;
    float shift_y=center_y-Graph->SizeY()/2.0f;

    if ((effect & 0x10000000) == 0) {
        const float minX=m_shiftX1-Graph->ViewXMin();
        if (shift_x < minX)
            shift_x=minX;

        const float minY=m_shiftY1-Graph->ViewYMin();
        if (shift_y < minY)
            shift_y=minY;

        const float maxX=m_shiftX2-Graph->ViewXMax();
        if (shift_x > maxX)
            shift_x=maxX;

        const float maxY=m_shiftY2-Graph->ViewYMax();
        if (shift_y > maxY)
            shift_y=maxY;
    }

    if (m_shiftX == shift_x && m_shiftY == shift_y)
        return;

    if (effect == 2) {
        Graph->Effect(2,static_cast<int>(center_x),static_cast<int>(center_y),0);
        return;
    }

    const float dx=shift_x-m_shiftX;
    const float dy=shift_y-m_shiftY;
    m_shiftX=shift_x;
    m_shiftY=shift_y;

    for (int i=0;i<m_menu.No();++i) {
        SPRITE* sprite=*m_menu[i];
        sprite->ChangeCoor(sprite->X()+dx,sprite->Y()+dy,sprite->Z());
    }

    Mouse->ChangeCoor(Mouse->X()+dx,Mouse->Y()+dy,Mouse->Z());
    m_input.mouseX+=dx;
    m_input.mouseY+=dy;
}

void MAP::ResetGroundZ()
{
    if (m_groundz)
        ::operator delete(m_groundz);
    if (m_tempGroundz)
        ::operator delete(m_tempGroundz);

    m_groundW=(static_cast<int>(m_w+7.0f))/8;
    m_groundH=(static_cast<int>(m_h+7.0f))/8;
    const int bytes=2*m_groundW*m_groundH;

    m_groundz=static_cast<short*>(::operator new(static_cast<unsigned int>(bytes)));
    m_tempGroundz=static_cast<short*>(::operator new(static_cast<unsigned int>(bytes)));
    memset(m_groundz,0,static_cast<unsigned int>(bytes));
    memset(m_tempGroundz,0,static_cast<unsigned int>(bytes));
}

void MAP::SetGroundZ(float x,float y,float newZ)
{
    if (!ValidateXY(x,y))
        return;

    const int index=static_cast<int>(x)/8 + static_cast<int>(y/8.0f)*m_groundW;
    if (m_groundz[index] < static_cast<int>(newZ))
        m_groundz[index]=static_cast<short>(newZ);
}

void MAP::SetScrollBox(float minX,float minY,float maxX,float maxY)
{
    m_shiftX1=minX;
    m_shiftX2=maxX;
    m_shiftY1=minY;
    m_shiftY2=maxY;
}

int MAP::NextVid(int oldVid,unsigned int spriteType)
{
    if (oldVid < 0)
        oldVid = 0;

    int current = oldVid;
    for (;;) {
        if (++current >= m_noVid)
            current = 0;
        if (current == oldVid)
            return -1;
        if (!ValidateVid(current))
            continue;
        VID* vid = m_vids[current];
        if (vid->PropSkipMapEd())
            continue;
        if (!vid->IsSpriteType(spriteType))
            continue;
        if (vid->IsEmptyType())
            continue;
        return current;
    }
}

int MAP::PrevVid(int oldVid,unsigned int spriteType)
{
    if (oldVid < 0)
        oldVid = 0;

    int current = oldVid;
    for (;;) {
        if (--current < 0)
            current = m_noVid - 1;
        if (current == oldVid)
            return -1;
        if (!ValidateVid(current))
            continue;
        VID* vid = m_vids[current];
        if (vid->PropSkipMapEd())
            continue;
        if (!vid->IsSpriteType(spriteType))
            continue;
        if (vid->IsEmptyType())
            continue;
        return current;
    }
}

// Terrain-type VID (sprite class 7) samples the complete footprint against both
// ground grids; every other VID uses the ordinary point ground query.
float MAP::GetGroundZ(const VID* vid,float x,float y)
{
    if (!vid || vid->m_spriteClass != 7u)
        return GetGroundZ(x,y);

    float maxX = x + vid->m_footprintWidth * 0.5f - 3.0f;
    float maxY = y + vid->m_footprintHeight * 0.5f - 3.0f;
    float minX = x - (vid->m_footprintWidth * 0.5f - 3.0f);
    float minY = y - (vid->m_footprintHeight * 0.5f - 3.0f);

    if (minX < 0.0f) minX = 0.0f;
    else if (minX >= m_w) minX = (m_w - 1.0f) / 8.0f;
    else minX /= 8.0f;

    if (minY < 0.0f) minY = 0.0f;
    else if (minY >= m_h) minY = (m_h - 1.0f) / 8.0f;
    else minY /= 8.0f;

    if (maxX < 0.0f) maxX = 0.0f;
    else if (maxX >= m_w) maxX = (m_w - 1.0f) / 8.0f;
    else maxX /= 8.0f;

    if (maxY < 0.0f) maxY = 0.0f;
    else if (maxY >= m_h) maxY = (m_h - 1.0f) / 8.0f;
    else maxY /= 8.0f;

    int ground = -16383;
    for (float gy=minY; gy<=maxY; gy+=1.0f) {
        for (float gx=minX; gx<=maxX; gx+=1.0f) {
            const int ix = static_cast<int>(gx);
            const int iy = static_cast<int>(gy);
            const int index = ix + iy * m_groundW;
            if (m_groundz) {
                const int z = m_groundz[index];
                if (z > ground) ground = z;
            }
            if (m_tempGroundz) {
                const int z = m_tempGroundz[index];
                if (z > ground) ground = z;
            }
        }
    }
    return static_cast<float>(ground);
}

void MAP::DeleteExtraVid()
{
    for (int layer=0;layer<18;++layer) {
        int index=0;
        SPRITE* sprite=FirstSprite(layer,&index);
        while (sprite) {
            if (sprite->Vid()->IsExtraType())
                sprite->ScalarDeletingDestructor(1);
            sprite=NextSprite(layer,&index);
        }
    }

    for (int i=m_noVid-1;i>=0;--i) {
        VID* vid=m_vids[i];
        if (vid && vid->IsExtraType()) {
            void** vtable=*reinterpret_cast<void***>(vid);
            typedef void* (__thiscall *ScalarDelete)(void*,unsigned int);
            reinterpret_cast<ScalarDelete>(vtable[1])(vid,1);
            m_vids[i]=0;
        }
    }

    while (m_noVid > 0 && m_vids[m_noVid-1] == 0)
        --m_noVid;
}

void MAP::Error(int type,char* text,unsigned long value)
{
    if (::Error)
        MYERROR::Error(::Error,"MAP",type,text,value);
}

void MAP::SetTempGroundZ(float x,float y,float newZ)
{
    if (!ValidateXY(x,y))
        return;

    const int index=static_cast<int>(x)/8 + static_cast<int>(y/8.0f)*m_groundW;
    if (m_tempGroundz[index] < static_cast<int>(newZ))
        m_tempGroundz[index]=static_cast<short>(newZ);
}

void MAP::ClearTempGroundZ(float x,float y,float newZ)
{
    if (!ValidateXY(x,y))
        return;

    const int index=static_cast<int>(x)/8 + static_cast<int>(y/8.0f)*m_groundW;
    if (m_tempGroundz[index] == static_cast<int>(newZ))
        m_tempGroundz[index]=0;
}

// The retail PLAYER stores the sprite-under-cursor PTR_SPRITE payload at +0x24.
SPRITE* PLAYER::SpriteUnderCursor()
{
    return *reinterpret_cast<SPRITE**>(reinterpret_cast<uint8_t*>(this)+0x24);
}

// The retail PLAYER stores the flagman PTR_SPRITE payload at +0x10.
SPRITE* PLAYER::Flagman()
{
    return *reinterpret_cast<SPRITE**>(reinterpret_cast<uint8_t*>(this)+0x10);
}

SPRITE* MAP::Flagman(int army)
{
    return m_player[army & 3]->Flagman();
}

SPRITE* MAP::SpriteUnderCursor()
{
    return m_player[m_curArmy & 3]->SpriteUnderCursor();
}

namespace {
float ClampDirectionalAimAxis(float value,float viewportSize,float retailExtent)
{
    const float lower=viewportSize-retailExtent;
    if (value < lower)
        return lower;
    if (value > retailExtent)
        return retailExtent;
    return value;
}
}

// Direct editor camera control.  The two velocity globals are the retail
// storage at 0x004CE40C/0x004CE410 and +0x24C is the current army index.
void MAP::ControlShiftCoor()
{
    if (m_shiftFlag & 0x21u) {
        if (m_input.screenMouseX <= 5.0f && (m_shiftFlag & 0x01u)) {
            if (-Const->maxShiftSpeedX < g_shiftSpeedX)
                g_shiftSpeedX -= 0.04f;
        } else if (Graph->SizeX()-5.0f <= m_input.screenMouseX && (m_shiftFlag & 0x01u)) {
            if (g_shiftSpeedX < Const->maxShiftSpeedX)
                g_shiftSpeedX += 0.04f;
        } else if (m_input.left && (m_shiftFlag & 0x20u)) {
            if (-Const->maxShiftSpeedX < g_shiftSpeedX)
                g_shiftSpeedX -= 0.04f;
        } else if (m_input.right && (m_shiftFlag & 0x20u)) {
            if (g_shiftSpeedX < Const->maxShiftSpeedX)
                g_shiftSpeedX += 0.04f;
        } else {
            g_shiftSpeedX=0.0f;
        }

        if (m_input.screenMouseY <= 5.0f && (m_shiftFlag & 0x01u)) {
            if (-Const->maxShiftSpeedY < g_shiftSpeedY)
                g_shiftSpeedY -= 0.03f;
        } else if (Graph->SizeY()-5.0f <= m_input.screenMouseY && (m_shiftFlag & 0x01u)) {
            if (g_shiftSpeedY < Const->maxShiftSpeedY)
                g_shiftSpeedY += 0.03f;
        } else if (m_input.up && (m_shiftFlag & 0x20u)) {
            if (-Const->maxShiftSpeedY < g_shiftSpeedY)
                g_shiftSpeedY -= 0.03f;
        } else if (m_input.down && (m_shiftFlag & 0x20u)) {
            if (g_shiftSpeedY < Const->maxShiftSpeedY)
                g_shiftSpeedY += 0.03f;
        } else {
            g_shiftSpeedY=0.0f;
        }
    } else {
        g_shiftSpeedY=0.0f;
        g_shiftSpeedX=0.0f;
    }

    SPRITE* flagman=Flagman(m_curArmy);
    if (flagman && (m_shiftFlag & 0x04u) && g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        g_shiftSpeedX=(flagman->ScreenX()-Graph->SizeX()/2.0f)/1000.0f;
        g_shiftSpeedY=(flagman->ScreenY()-Graph->SizeY()/2.0f)/1000.0f;
    } else if (flagman && (m_shiftFlag & 0x08u) && g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        const float cx=ClampDirectionalAimAxis(m_input.screenMouseX,Graph->SizeX(),640.0f);
        const float cy=ClampDirectionalAimAxis(m_input.screenMouseY,Graph->SizeY(),480.0f);
        const float midX=(flagman->ScreenX()+cx)/2.0f;
        const float midY=(flagman->ScreenY()+cy)/2.0f;
        g_shiftSpeedX=(Graph->SizeX()/2.0f-midX)*(-4.0f/1000.0f);
        g_shiftSpeedY=(Graph->SizeY()/2.0f-midY)*(-4.0f/1000.0f);
    } else if (flagman && (m_shiftFlag & 0x10u) && g_shiftSpeedX==0.0f && g_shiftSpeedY==0.0f) {
        SetShiftCoor(flagman->ScreenX()+m_shiftX,flagman->ScreenY()+m_shiftY,0);
        return;
    }

    const float delta=(float)(CurrentTime-PrevCurrentTime);
    SetShiftCoor((int)(g_shiftSpeedX*delta)+Graph->SizeX()/2.0f+m_shiftX,
                 (int)(g_shiftSpeedY*delta)+Graph->SizeY()/2.0f+m_shiftY,
                 0);
}

SPRITE* MENU::SpriteUnderCursor()
{
    return sprite;
}

int MAP::GetFPS()
{
    return m_fps;
}

void MAP::DrawSecondaryInfo()
{
    if (m_flags & 0x00020000u)
        Graph->PrintfXY(Graph->ViewXMin(),Graph->ViewYMin()+1.0f,"%i",GetFPS());

    if (!Const->debugMode)
        return;

    if (m_flags & 0x00010000u)
        Graph->PrintfXY(Graph->ViewXMax()-20.0f,Graph->ViewYMin()+1.0f,"%2i",Sound->GetNoPlayed());

    if ((m_flags & 0x00000800u) && m_groundz) {
        for (int y=1;y<m_groundH;++y) {
            for (int x=1;x<m_groundW;++x) {
                const int i=x+y*m_groundW;
                const int z1=m_groundz[i] > m_tempGroundz[i] ? m_groundz[i] : m_tempGroundz[i];
                const int z0=m_groundz[i-1] > m_tempGroundz[i-1] ? m_groundz[i-1] : m_tempGroundz[i-1];
                const float x1=static_cast<float>(8*x+4)-m_shiftX;
                const float y1=static_cast<float>(8*y+4-z1)-m_shiftY;
                const float x0=static_cast<float>(8*x-4)-m_shiftX;
                const float y0=static_cast<float>(8*y+4-z0)-m_shiftY;
                if (Graph->InViewPort(x1,y1) || Graph->InViewPort(x0,y0))
                    Graph->Line(x1,y1,x0,y0,GRAPH::GRAY);
            }
        }
    }

    if (m_flags & 0x00008000u) {
        // Retail compares the layer counter with 0x10 and exits only when it is
        // greater, therefore debug rectangles cover layers 0..16 inclusive.
        for (int layer=0;layer<=16;++layer) {
            int index=0;
            for (SPRITE* sprite=FirstSprite(layer,&index);sprite;sprite=NextSprite(layer,&index))
                sprite->DrawRectangle();
        }
    }

    if (m_flags & 0x00001000u) {
        SPRITE* sprite=Flagman(m_curArmy);
        if (!sprite)
            sprite=SpriteUnderCursor();
        if (!sprite)
            sprite=m_menu.SpriteUnderCursor();
        if (sprite)
            sprite->DrawSecondaryInfo();
    }

    if (m_flags & 0x00004000u)
        m_groups.DrawNumber();

    if (m_flags & 0x00002000u)
        RailMap.DebugDraw();
}

RELATION::~RELATION()
{
    // C++ destroys newSprites then oldSprites, exactly matching the retail calls
    // at +0x10 and +0x00.
}

void RELATION::Release()
{
    oldSprites.Release();
    newSprites.Release();
}

MENU::~MENU()
{
    // SPRITE_LIST base destructor is emitted automatically.
}

GROUPS::~GROUPS()
{
    // GROUP first is destroyed automatically.
}

namespace {
void DeleteVirtualObject(void* object)
{
    if (!object)
        return;
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void* (__thiscall *DeletingDestructor)(void*,unsigned int);
    reinterpret_cast<DeletingDestructor>(vtable[0])(object,1u);
}

void DeleteVirtualObjectSlot1(void* object)
{
    if (!object)
        return;
    void** const vtable=*reinterpret_cast<void***>(object);
    typedef void* (__thiscall *DeletingDestructor)(void*,unsigned int);
    reinterpret_cast<DeletingDestructor>(vtable[1])(object,1u);
}
}

void MOUSETIPS::Tact(INPUT* input)
{
    static unsigned long lastMoveTime=0;
    static float lastMouseX=0.0f;
    static float lastMouseY=0.0f;
    static STRING cachedText;

    if (lastMouseX!=input->screenMouseX || lastMouseY!=input->screenMouseY) {
        lastMoveTime=CurrentTime;
        lastMouseX=input->screenMouseX;
        lastMouseY=input->screenMouseY;
    }

    if (CurrentTime-lastMoveTime<=static_cast<unsigned long>(Const->MouseTipsTime) ||
        input->lClick || input->key || Map->Menu()->IsLClick() ||
        !Map->OptSelectSpriteUnderCursor()) {
        Clear();
        return;
    }

    if (!tip) {
        VID* vid=Map->Vid(6);
        cachedText=Map->GetMouseTipsString();
        if (vid==EmptyVid || cachedText.Length()==0)
            return;

        float x=input->screenMouseX+5.0f;
        float y=input->screenMouseY-vid->m_footprintHeight+3000.0f-10.0f;
        const float upperEdge=vid->m_footprintHeight/2.0f+y-3000.0f;
        if (Graph->ViewYMin()>=upperEdge)
            y=vid->m_footprintHeight/2.0f+input->screenMouseY+3000.0f+10.0f;

        const float right=x+static_cast<float>(cachedText.Length()+2)*vid->m_footprintWidth;
        if (Graph->ViewXMax()<=right)
            x=Graph->ViewXMax()-static_cast<float>(cachedText.Length()+2)*vid->m_footprintWidth;

        tip=Map->CreateSprite(vid,x,y,3000.0f,ANGLE(static_cast<uint8_t>(0)),0);
        if (tip) {
            STRING actionText("{",cachedText.CharPtr());
            actionText+="}";
            reinterpret_cast<SPRITE*>(tip)->Action(0x78,reinterpret_cast<int>(&actionText),0,0);
        }
        return;
    }

    if (CurrentTime-lastMoveTime>static_cast<unsigned long>(Const->MouseTipsTime)+500u) {
        lastMoveTime+=500u;
        STRING current=Map->GetMouseTipsString();
        if (current!=&cachedText)
            Clear();
    }
}

void MOUSETIPS::Clear()
{
    if (tip)
        DeleteVirtualObject(tip);
    tip=0;
}

void MOUSETIPS::DeletePointerToSprite(SPRITE* spr)
{
    if (tip==spr)
        tip=0;
}

MOUSETIPS::~MOUSETIPS()
{
    Clear();
}

PROFILE::~PROFILE()
{
    // FileName is destroyed automatically.
}

MAP::~MAP()
{
    for (int i=0;i<18;++i)
        m_layers[i].DeleteAll();

    if (::Mouse)
        DeleteVirtualObject(::Mouse);

    for (int i=0;i<4;++i) {
        if (m_player[i])
            DeleteVirtualObject(m_player[i]);
    }

    if (::Hash)
        delete ::Hash;
    if (::Profile)
        delete ::Profile;
    if (::Sound)
        delete ::Sound;
    if (::Const)
        ::operator delete(::Const);
    if (::Registry)
        delete ::Registry;

    for (int i=m_noVid-1;i>=0;--i) {
        if (m_vids[i]) {
            DeleteVirtualObjectSlot1(m_vids[i]);
            m_vids[i]=0;
        }
    }
    m_noVid=0;

    MYERROR::Log(::Error,"Vid    release %i %i",SURFACE::MemoryInUse(),g_vidMemoryInUse);

    if (::Graph) {
        ::Graph->GRAPH::~GRAPH();
        ::operator delete(::Graph);
    }

    if (m_groundz)
        ::operator delete(m_groundz);
    if (m_tempGroundz)
        ::operator delete(m_tempGroundz);
    if (m_weapon)
        ::operator delete(m_weapon);

    if (::Error)
        DeleteVirtualObject(::Error);

    CoUninitialize();
    timeEndPeriod(1u);

    // Embedded MOUSETIPS/GROUPS/MENU/RELATION/RESOURCE/LOGIC/layers/STRING
    // members are destroyed automatically after this body in declaration-reverse
    // order, matching 0x00416680..0x00416723 exactly.
}

void MAP::SetScrollType(int type)
{
    m_shiftFlag=static_cast<uint32_t>(type);
}

namespace {
void PlayerReleaseVirtual(PLAYER* player)
{
    void** const vtable=*reinterpret_cast<void***>(player);
    typedef void (__thiscall *ReleaseMethod)(PLAYER*);
    reinterpret_cast<ReleaseMethod>(vtable[4])(player);
}
}

// Exact retail body: this base implementation is the vtable fallback and returns 0.
int MAP::Tact()
{
    return 0;
}

void MAP::Release()
{
    for (int i=0;i<m_noVid;++i) {
        if (!ValidateVid(i))
            continue;
        VID* const vid=m_vids[i];
        if (!vid->NoSprites())
            continue;
        const char* const fileName=vid->m_resourceName.m_buf;
        MYERROR::Log(::Error,"NoVid[%3i]=%i %i %i %i %s Layer=%i %s",
                     i,vid->NoSprites(0),vid->NoSprites(1),vid->NoSprites(2),vid->NoSprites(3),
                     vid->m_name.m_buf,vid->m_layer,fileName);
    }

    m_relation.Release();
    m_speed=1.0f;
    m_fps=0;
    m_fpsCnt=0;
    Graph->StopMovie();
    Graph->SetWind(25,ANGLE(static_cast<unsigned char>(200)));
    Graph->SetEnvironment(0xFFFFFFFFu);
    Graph->Effect(11,0,0,1);

    if ((m_flags>>9)&1u) {
        Mouse->Enable();
        m_resource.Close();
    }
    if ((m_flags>>8)&1u) {
        int end=-1;
        m_resource.Write(&end,4u);
        m_resource.PostAppend();
        m_resource.Close();
    }
    m_flags&=~0x300u;
    SetScrollType(1);

    MYERROR::Log(::Error,"Player release");
    for (int i=0;i<4;++i) {
        if (m_player[i])
            PlayerReleaseVirtual(m_player[i]);
    }

    MYERROR::Log(::Error,"Sprite release");
    ENGINE::globaldeleting=1;
    for (int layer=0;layer<18;++layer) {
        int index=0;
        SPRITE* sprite=FirstSprite(layer,&index);
        while (sprite) {
            sprite->ScalarDeletingDestructor(1u);
            sprite=NextSprite(layer,&index);
        }
    }
    ENGINE::globaldeleting=0;

    for (int layer=0;layer<18;++layer) {
        int index=0;
        SPRITE* const sprite=FirstSprite(layer,&index);
        if (sprite)
            sprite->Error(10,const_cast<char*>("Sprite exist after delete"),static_cast<unsigned long>(index));
    }

    if (m_groups.First())
        Error(10,const_cast<char*>("Incorrect delete groups in DeleteAll()"),0);

    MYERROR::Log(::Error,"Menu   release");
    if (m_menu.No()) {
        SPRITE** const item=m_menu[0];
        (*item)->Error(10,const_cast<char*>("Menu sprite exist after delete"),0);
        m_menu.DeleteAll();
    }

    const unsigned long elapsed=CurrentTime-m_unknown30;
    const unsigned int averageFps=elapsed ? (static_cast<unsigned int>(m_noTact)*1000u)/elapsed : 0u;
    MYERROR::Log(::Error,"Average fps=%i",averageFps);
    MYERROR::Log(::Error,"Script release");

    m_logic.Release();
    m_noTact=0;
    DeleteExtraVid();
    for (int i=0;i<m_noVid;++i) {
        if (m_vids[i])
            m_vids[i]->ResetSprites();
    }
    for (unsigned int i=0;i<64u;++i)
        EvFunctionNumber[i]=static_cast<int>(i)+1000000;
}

namespace {
void PlayerLoadVirtual(PLAYER* player,STREAM* res)
{
    void** const vtable=*reinterpret_cast<void***>(player);
    typedef void (__thiscall *LoadMethod)(PLAYER*,STREAM*);
    reinterpret_cast<LoadMethod>(vtable[3])(player,res);
}

void LegacyMapIntFieldToFloat(float* value)
{
    int integerValue=0;
    memcpy(&integerValue,value,4u);
    *value=static_cast<float>(integerValue);
}

const char* const kEventFunctionName[] = {
    "main",
    "TrainNotAmmo",
    "TrainNotPower",
    "TrainDamage",
    "TrainCreated",
    "TrainSplit",
    "TrainDestroy",
    "TrainDestroyPower",
    "TrainArrive",
    "???TrainNotArrive",
    "TrainAttacked",
    "DepoDestroy",
    "DepoBirth",
    "DepoAttacked",
    "DepoFree",
    "BuildingCapture",
    "MasterDestroy",
    "???",
    "???SuperWeaponWounded",
    "MineBlast",
    "MineRemove",
    "EnemyLinked",
    "TrainClash",
    "UnitCreated",
    "UnitDestroy",
    ""
};

int ParseThreeDigitVid(const STRING& name)
{
    return 100*(name.m_buf[1]-'0')+10*(name.m_buf[2]-'0')+(name.m_buf[3]-'0');
}

int ParseFourDigitVid(const STRING& name)
{
    return 1000*(name.m_buf[1]-'0')+100*(name.m_buf[2]-'0')+
           10*(name.m_buf[3]-'0')+(name.m_buf[4]-'0');
}

}

// Complete retail map loader.  The newer portable engine implementation was
// used only as a naming aid; resource ordering, legacy format branches, demo
// serialization, script binding and all 18-layer post-load passes follow the
// original MapEdit body.
void MAP::Load(STRING name)
{
    RESOURCE res;
    m_flags|=0x20u;
    if (name=="")
        return;

    if (m_w!=0.0f || m_h!=0.0f) {
        if (Const->debugMode)
            Graph->DrawDebugText("Release previous map");
        Release();
    }

    if (!m_resource.IsOpen() && m_resource.OpenForRead(&name,0x4F4D4544u)==0) {
        name.Read(&m_resource);
        m_flags|=0x200u;
    }

    if (res.OpenForRead(&name,0x2050414Du)!=0) {
        ::Error->Window("!!!ERROR!!!LOAD: Invalid map file %s",name.m_buf);
        return;
    }

    Mouse->Disable();
    m_prevMap=&m_mapName;
    m_mapName=&name;

    unsigned int seed=0;
    if (m_flags&0x200u)
        m_resource.Read(&seed,4u);
    else
        seed=timeGetTime();
    srand(seed);

    if (Const->debugMode)
        Graph->DrawDebugText("Load extra vid");
    LoadVid(&res);

    int version=0;
    if (res.GoBegin(0x48505247u)!=0) { // old GRPH layout
        if (res.GoBegin(0x44414548u)!=0) {
            Error(11,const_cast<char*>("HEAD"),0);
            return;
        }

        int dimension=0;
        res.Read(&dimension,4u); m_w=static_cast<float>(dimension);
        res.Read(&dimension,4u); m_h=static_cast<float>(dimension);
        short shift=0;
        res.Read(&shift,2u); m_shiftX=static_cast<float>(shift);
        res.Read(&shift,2u); m_shiftY=static_cast<float>(shift);
        res.Read(&CurrentTime,4u);
        m_unknown30=CurrentTime;
        PrevCurrentTime=CurrentTime;
        res.Read(&version,4u);
        Graph->OldLoadParameters(&res);

        if (Hash)
            delete Hash;
        Hash=new HASH_MAP(m_w,m_h,m_vids,m_noVid);
        SetScrollBox(0.0f,0.0f,m_w,m_h);
        ResetGroundZ();

        if (res.GoNext(0x44495247u)==0) {
            if (m_groundz)
                ::operator delete(m_groundz);
            m_groundz=0;
            const int loaded=res.SubLoad(reinterpret_cast<void**>(&m_groundz),0);
            const int expected=2*((static_cast<int>(m_w+7.0f))/8)*((static_cast<int>(m_h+7.0f))/8);
            if (loaded!=expected) {
                Error(4,const_cast<char*>("grid"),static_cast<unsigned long>(loaded));
                ResetGroundZ();
            }
        } else {
            res.GoBegin(0x20594E41u); // 'ANY '
        }

        if (res.GoNext(0x20525053u)!=0) {
            Error(11,const_cast<char*>("SPR "),0);
            return;
        }
        while (OldLoadSprite(&res)!=reinterpret_cast<SPRITE*>(-1)) {}
        RailMap.CreateAdditionalDots();

        if (res.GoNext(0x44525053u)!=0) {
            Error(11,const_cast<char*>("SPRD"),0);
            return;
        }
        for (SPRITE* sprite=ReadPointer(&res);
             sprite!=reinterpret_cast<SPRITE*>(-1);
             sprite=ReadPointer(&res)) {
            if (sprite)
                sprite->Action(0xC8,reinterpret_cast<int>(&res),version,0);
            res.GoNextSub(0x44525053u);
        }
    } else {
        if (Const->debugMode)
            Graph->DrawDebugText("Load graph parameters");
        Graph->LoadParameters(&res);

        if (res.GoNext(0x44414548u)!=0) {
            Error(11,const_cast<char*>("HEAD"),0);
            return;
        }
        res.Read(&m_w,4u);
        res.Read(&m_h,4u);
        res.Read(&m_shiftX,4u);
        res.Read(&m_shiftY,4u);
        res.Read(&CurrentTime,4u);
        PrevCurrentTime=1;
        CurrentTime=10;

        if (!(m_flags&0x200u) && m_resource.IsOpen()) {
            m_flags|=0x100u;
            m_resource.PreAppend(0x4F4D4544u,0);
            name.Write(&m_resource);
            m_resource.Write(&seed,4u);
            m_resource.Write(&CurrentTime,4u);
        }
        if (m_flags&0x200u)
            m_resource.Read(&CurrentTime,4u);
        m_unknown30=CurrentTime;

        res.Read(&version,4u);
        if (version<=9) {
            LegacyMapIntFieldToFloat(&m_w);
            LegacyMapIntFieldToFloat(&m_h);
            LegacyMapIntFieldToFloat(&m_shiftX);
            LegacyMapIntFieldToFloat(&m_shiftY);
        }
        MYERROR::Log(::Error,
            "CurrentTime   =%-15u   sizeof(SPRITE)=%-8i Map version   =%i",
            CurrentTime,static_cast<int>(sizeof(SPRITE)),version);

        if (Const->debugMode)
            Graph->DrawDebugText("Create new hash table");
        if (Hash)
            delete Hash;
        Hash=new HASH_MAP(m_w,m_h,m_vids,m_noVid);
        SetScrollBox(0.0f,0.0f,m_w,m_h);
        SetShiftCoor(Graph->SizeX()/2.0f+m_shiftX,Graph->SizeY()/2.0f+m_shiftY,0);

        if (Const->debugMode)
            Graph->DrawDebugText("Load gridZ");
        ResetGroundZ();
        if (res.GoNext(0x44495247u)==0) {
            if (m_groundz)
                ::operator delete(m_groundz);
            m_groundz=0;
            const int loaded=res.SubLoad(reinterpret_cast<void**>(&m_groundz),0);
            if (loaded!=2*m_groundW*m_groundH) {
                Error(4,const_cast<char*>("grid"),static_cast<unsigned long>(loaded));
                ResetGroundZ();
            }
        } else {
            res.GoBegin(0x20594E41u); // 'ANY '
        }

        if (res.GoNext(0x20525053u)!=0) {
            Error(11,const_cast<char*>("SPR "),0);
            return;
        }
        if (Const->debugMode)
            Graph->DrawDebugText("Load hardware terrain");

        int loadedLayer=0;
        for (SPRITE* sprite=LoadSprite(&res,version);
             sprite!=reinterpret_cast<SPRITE*>(-1);
             sprite=LoadSprite(&res,version)) {
            if (Const->debugMode && sprite && loadedLayer!=sprite->Vid()->m_layer) {
                loadedLayer=sprite->Vid()->m_layer;
                if (loadedLayer==0)
                    Graph->DrawDebugText("Load hardware terrain");
                else if (loadedLayer==1)
                    Graph->DrawDebugText("Build sprites in terrain");
                else if (loadedLayer==2)
                    Graph->DrawDebugText("Build sprites with alpha in terrain");
                else
                    Graph->DrawDebugText("Load sprites");
            }
            if (Graph->DrawLoadBar(m_vids[0]))
                Sound->MusicTact();
        }
        RailMap.CreateAdditionalDots();

        if (Const->debugMode)
            Graph->DrawDebugText("Load data for sprite");
        if (res.GoNext(0x44525053u)!=0) {
            Error(11,const_cast<char*>("SPRD"),0);
            return;
        }
        for (SPRITE* sprite=ReadPointer(&res);
             sprite!=reinterpret_cast<SPRITE*>(-1);
             sprite=ReadPointer(&res)) {
            if (Graph->DrawLoadBar(m_vids[0]))
                Sound->MusicTact();
            if (sprite)
                sprite->Action(0x51,reinterpret_cast<int>(&res),version,0);
            res.GoNextSub(0x44525053u);
        }

        if (Const->debugMode)
            Graph->DrawDebugText("Load players info");
        if (res.GoNext(0x59414C50u)!=0) {
            Error(11,const_cast<char*>("PLAY"),0);
            return;
        }
        for (int i=0;i<4;++i)
            PlayerLoadVirtual(m_player[i],&res);

        if (Const->debugMode)
            Graph->DrawDebugText("Load groups info");
        if (res.GoNext(0x554F5247u)!=0) {
            Error(11,const_cast<char*>("GROU"),0);
            return;
        }
        m_groups.Load(&res);
    }

    m_flags&=~0x20u;
    res.Close();
    m_relation.Release();
    MYERROR::Log(::Error,"Vid    release %i %i",SURFACE::MemoryInUse(),g_vidMemoryInUse);

    if (Const->debugMode)
        Graph->DrawDebugText("Load script file");

    const STRING lgc=name.BeforeLast(".")+".lgc";
    const int haveLgc=FExist(&lgc);
    int logicResult=0;
    if (haveLgc) {
        logicResult=m_logic.Load(&lgc);
    } else {
        const STRING lgd=name.BeforeLast(".")+".lgd";
        const int haveLgd=FExist(&lgd);
        if (haveLgd)
            logicResult=m_logic.Load(&lgd);
        else
            logicResult=m_logic.Load(&lgc);
    }
    (void)logicResult;

    if (Const->debugMode)
        Graph->DrawDebugText("Connect script function with VID");

    NAMED_LIST<LOGICVAR>* vars=const_cast<NAMED_LIST<LOGICVAR>*>(m_logic.Var());
    for (int function=0;function<vars->No();++function) {
        const LOGICVAR* const var=(*vars)[function];
        if (!var || var->type!=3)
            continue;

        STRING functionName(vars->Name(function));
        for (int event=0;kEventFunctionName[event][0];++event) {
            if (functionName==kEventFunctionName[event])
                EvFunctionNumber[event]=function;
        }

        if (functionName[0]!='F' || !isdigit(functionName[1]) ||
            !isdigit(functionName[2]) || !isdigit(functionName[3]))
            continue;

        if (functionName[4]=='_') {
            const int nvid=ParseThreeDigitVid(functionName);
            if (strncmp(functionName.CharPtr()+5,"DAMAGE",7u)==0) {
                if (var->no_var!=3)
                    {
                    STRING prefix("no parameters in functions '",functionName.m_buf);
                    STRING message(prefix.m_buf,"'");
                    Error(4,message.CharPtr(),static_cast<unsigned long>(var->no_var));
                }
                else if (ValidateVid(nvid))
                    reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(m_vids[nvid])+0x408)[18]=function;
            } else if (strncmp(functionName.CharPtr()+5,"DESTROY",7u)==0) {
                if (var->no_var!=1)
                    {
                    STRING prefix("no parameters in functions '",functionName.m_buf);
                    STRING message(prefix.m_buf,"'");
                    Error(4,message.CharPtr(),static_cast<unsigned long>(var->no_var));
                }
                else if (ValidateVid(nvid))
                    reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(m_vids[nvid])+0x408)[17]=function;
            } else if (strncmp(functionName.CharPtr()+5,"COLLISION",9u)==0) {
                if (var->no_var!=2)
                    {
                    STRING prefix("no parameters in functions '",functionName.m_buf);
                    STRING message(prefix.m_buf,"'");
                    Error(4,message.CharPtr(),static_cast<unsigned long>(var->no_var));
                }
                else if (ValidateVid(nvid))
                    reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(m_vids[nvid])+0x408)[19]=function;
            } else {
                const int animation=functionName[6] ?
                    10*(functionName[5]-'0')+(functionName[6]-'0') : (functionName[5]-'0');
                if (var->no_var!=1)
                    {
                    STRING prefix("no parameters in functions '",functionName.m_buf);
                    STRING message(prefix.m_buf,"'");
                    Error(4,message.CharPtr(),static_cast<unsigned long>(var->no_var));
                }
                else if (ValidateVid(nvid) && animation<17)
                    reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(m_vids[nvid])+0x408)[animation]=function;
            }
        } else if (isdigit(functionName[4]) && functionName[5]=='_') {
            const int nvid=ParseFourDigitVid(functionName);
            const int animation=functionName[7] ?
                10*(functionName[6]-'0')+(functionName[7]-'0') : (functionName[6]-'0');
            if (var->no_var!=1)
                {
                    STRING prefix("no parameters in functions '",functionName.m_buf);
                    STRING message(prefix.m_buf,"'");
                    Error(4,message.CharPtr(),static_cast<unsigned long>(var->no_var));
                }
            else if (ValidateVid(nvid) && animation<17)
                reinterpret_cast<int*>(reinterpret_cast<uint8_t*>(m_vids[nvid])+0x408)[animation]=function;
        }
    }

    if (m_flags&0x200u)
        m_logic.LoadVar(&m_resource);
    else if (m_flags&0x100u)
        m_logic.SaveVar(&m_resource);

    RealCurrentTime=timeGetTime();
    if (Const->debugMode)
        Graph->DrawDebugText("Run scripts for create sprites");

    if (!IsMapEdit()) {
        for (int layer=0;layer<18;++layer) {
            int index=0;
            SPRITE* sprite=FirstSprite(layer,&index);
            while (sprite) {
                const int function=reinterpret_cast<int*>(
                    reinterpret_cast<uint8_t*>(sprite->Vid())+0x408)[14];
                if (function>=0)
                    ScriptRun(function,sprite,0,0);
                sprite=NextSprite(layer,&index);
            }
        }
    }

    for (int layer=0;layer<18;++layer) {
        int index=0;
        SPRITE* sprite=FirstSprite(layer,&index);
        while (sprite) {
            if (sprite->IsSpriteClass(21u))
                reinterpret_cast<ENGINE*>(sprite)->CheckPrevNextEngine();
            sprite=NextSprite(layer,&index);
        }
    }

    if (!(m_flags&0x200u))
        Mouse->Enable();
    if (Const->debugMode)
        Graph->DrawDebugText("");
}

namespace {
void PlayerSaveVirtual(PLAYER* player,STREAM* res)
{
    void** const vtable=*reinterpret_cast<void***>(player);
    typedef void (__thiscall *SaveMethod)(PLAYER*,STREAM*);
    reinterpret_cast<SaveMethod>(vtable[2])(player,res);
}
}

void MAP::Save(STRING name)
{
    const int version=12;
    RESOURCE res;

    if (name=="")
        return;

    if (name==&m_mapName) {
        STRING temp("tmp_del!.map");
        FRename(&m_mapName,&temp);
        m_mapName="tmp_del!.map";
    }

    // Retail MAP::Save @ 0x004198B0 calls RESOURCE::OpenForWrite, not OpenForAppend.
    if (res.OpenForWrite(&name,0x2050414Du)!=0) {
        ::Error->Window("Can't open file %s",name.m_buf);
        return;
    }

    int extraVid=0;
    for (;extraVid<m_noVid;++extraVid) {
        if (m_vids[extraVid] && m_vids[extraVid]->IsExtraType())
            break;
    }

    if (extraVid<m_noVid) {
        RESOURCE oldRes;
        if (oldRes.OpenForRead(&m_mapName,0x2050414Du)==0) {
            res.Copy(&oldRes,0x50414557u); // WEAP
            res.Copy(&oldRes,0x204A424Fu); // OBJ 
            oldRes.Close();
        } else {
            ::Error->Window("Can't open file '%s', needed for save map",m_mapName.m_buf);
        }
    }

    res.PreAppend(0x48505247u,0); // GRPH
    Graph->SaveParameters(&res);
    res.PostAppend();

    res.PreAppend(0x44414548u,0); // HEAD
    res.Write(&m_w,4u);
    res.Write(&m_h,4u);
    res.Write(&m_shiftX,4u);
    res.Write(&m_shiftY,4u);
    res.Write(&CurrentTime,4u);
    res.Write(&version,4u);
    res.PostAppend();

    int haveGrid=0;
    for (int y=0;y<m_groundH && !haveGrid;++y) {
        for (int x=0;x<m_groundW;++x) {
            if (m_groundz[x+y*m_groundW]!=0) {
                res.PreAppend(0x44495247u,0); // GRID
                res.Write(m_groundz,static_cast<unsigned int>(m_groundW*m_groundH*2));
                res.PostAppend();
                haveGrid=1;
                break;
            }
        }
    }

    res.PreAppend(0x20525053u,0); // SPR 
    int savedSpr=0;
    for (int layer=0;layer<18;++layer) {
        int index=0;
        SPRITE* sprite=FirstSprite(layer,&index);
        while (sprite) {
            if (!sprite->IsLinked() && !m_menu.ExistIn(&sprite)) {
                const int beforeSprite=res.Tell();
                sprite->Save(&res);
                if (res.Tell()!=beforeSprite)
                    ++savedSpr;
            }
            sprite=NextSprite(layer,&index);
        }
    }
    int end=-1;
    res.Write(&end,4u);
    res.PostAppend();
    (void)savedSpr;

    int savedSprd=0;
    for (int layer=0;layer<18;++layer) {
        int index=0;
        SPRITE* sprite=FirstSprite(layer,&index);
        while (sprite) {
            if (!sprite->IsLinked() && !m_menu.ExistIn(&sprite)) {
                const int before=res.Tell();
                res.PreAppend(0x44525053u,0); // SPRD
                res.Write(&sprite,4u);
                sprite->Action(0x50,reinterpret_cast<long>(&res),0,0);
                const int afterAction=res.Tell();
                if (afterAction>before+5) {
                    res.PostAppend();
                    ++savedSprd;
                }
            }
            sprite=NextSprite(layer,&index);
        }
    }

    (void)savedSprd;
    res.PreAppend(0x44525053u,0); // SPRD terminator
    end=-1;
    res.Write(&end,4u);
    res.PostAppend();

    res.PreAppend(0x59414C50u,0); // PLAY
    for (int i=0;i<4;++i)
        PlayerSaveVirtual(m_player[i],&res);
    res.PostAppend();

    res.PreAppend(0x554F5247u,0); // GROU
    m_groups.Save(&res);
    res.PostAppend();
    res.Close();

    if (m_mapName=="tmp_del!.map") {
        STRING temp("tmp_del!.map");
        FRemove(&temp);
        m_mapName=&name;
    }
}

int __cdecl SortCallBack1(const int* lhs,const int* rhs)
{
    const int aIndex=*lhs;
    const int bIndex=*rhs;
    VID* a=Map->Vid(aIndex);
    VID* b=Map->Vid(bIndex);
    if (a==EmptyVid)
        return b==EmptyVid ? 0 : 1;
    if (b==EmptyVid)
        return -1;
    STRING* aName=&a->m_name;
    STRING* bName=&b->m_name;
    if (bName->operator<(aName))
        return 1;
    if (bName->operator>(aName))
        return -1;
    return 0;
}

int MAP::VidToListBox(DIALOG_LIST_BOX* list,unsigned long unitTypeMask,int selectVid,int sort)
{
    int order[2048];
    for (int i=0;i<m_noVid;++i)
        order[i]=i;
    if (sort)
        qsort(order,static_cast<unsigned int>(m_noVid),sizeof(int),reinterpret_cast<int (__cdecl *)(const void*,const void*)>(SortCallBack1));

    list->Reset();
    for (int i=0;i<m_noVid;++i) {
        const int nvid=order[i];
        VID* vid=m_vids[nvid];
        if (!vid)
            continue;
        if (vid->PropSkipMapEd())
            continue;
        if (!vid->IsSpriteType(unitTypeMask))
            continue;
        STRING label=vid->GetNumberName();
        const int listIndex=list->AddStringWithData(&label,nvid);
        if (vid->m_idx==selectVid)
            list->SetCurrent(listIndex);
    }
    return list->NoString();
}

int MAP::VidToControlBox(DIALOG_COMBO_BOX* list,unsigned long unitTypeMask,int selectVid)
{
    list->Reset();
    for (int i=0;i<m_noVid;++i) {
        VID* vid=m_vids[i];
        if (vid && !vid->PropSkipMapEd() && vid->IsSpriteType(static_cast<int>(unitTypeMask))) {
            STRING label=vid->GetNumberName();
            const int listIndex=list->AddStringWithData(&label,i);
            if (vid->m_idx==selectVid)
                list->SetCurrent(listIndex);
        }
    }
    return list->NoString();
}

void MAP::ChangeSizeXY(float newSizeX,float newSizeY)
{
    m_w=newSizeX;
    m_h=newSizeY;
    SetScrollBox(0.0f,0.0f,m_w,m_h);
    ResetGroundZ();
}

void MAP::ExchangeVid(VID* vid1,VID* vid2)
{
    if (!vid1 || !vid2 || vid1==vid2 || vid1==EmptyVid || vid2==EmptyVid)
        return;

    MYERROR::Log(::Error,"Start ExchangeVid %i %i",vid1->m_idx,vid2->m_idx);

    for (int i=0;i<m_noVid;++i) {
        VID* const vid=m_vids[i];
        if (!vid)
            continue;
        if (vid->m_linkVid==vid1)
            vid->m_linkVid=vid2;
        else if (vid->m_linkVid==vid2)
            vid->m_linkVid=vid1;
        for (int child=0;child<17;++child) {
            if (vid->m_aniChildVid[child]==vid1)
                vid->m_aniChildVid[child]=vid2;
            else if (vid->m_aniChildVid[child]==vid2)
                vid->m_aniChildVid[child]=vid1;
        }
    }

    m_vids[vid1->m_idx]=vid2;
    m_vids[vid2->m_idx]=vid1;

    VID* tempVid=vid1->m_exchangeVid;
    vid1->m_exchangeVid=vid2->m_exchangeVid;
    vid2->m_exchangeVid=tempVid;

    int tmp=vid1->m_idx;
    vid1->m_idx=vid2->m_idx;
    vid2->m_idx=tmp;

    int* const raw1=reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid1)+0x408);
    int* const raw2=reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid2)+0x408);
    for (int i=0;i<20;++i) {
        tmp=raw1[i]; raw1[i]=raw2[i]; raw2[i]=tmp;
    }

    MYERROR::Log(::Error,"End ExchangeVid %i %i",vid1->m_idx,vid2->m_idx);
}


SPRITE* MAP::Flagman()
{
    return Flagman(m_curArmy);
}

SPRITE* MAP::CreateSprite(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
{
    if (!vid)
        return 0;

    if ((vid->m_propertyBits>>4)&1u)
        vid=REGION::ConvertVid(vid,x,y,z);

    SPRITE* sprite=0;
    switch (vid->m_spriteClass) {
    case 2:  sprite=new UNIT(vid,x,y,z,direction,parent); break;
    case 4:  sprite=new PLANE(vid,x,y,z,direction,parent); break;
    case 5:  sprite=new CANNON(vid,x,y,z,direction,parent); break;
    case 6:  sprite=new PRIMITIVE(vid,x,y,z,direction,parent); break;
    case 7:  sprite=new MAN(vid,x,y,z,direction,parent); break;
    case 8:  sprite=new BUILDED_TERRAIN(vid,x,y,z,direction,parent); break;
    case 9:  sprite=new SPRITE(vid,x,y,z,direction,parent); break;
    case 10: sprite=new FRAME(vid,x,y,z,direction,parent); break;
    case 12: sprite=new LINKER(vid,x,y,z,direction,parent); break;
    case 19: sprite=new STEXT(vid,x,y,z,direction,parent); break;
    case 23: sprite=new REGION(vid,x,y,z,direction,parent); break;
    default:
        Error(3,const_cast<char*>("sprite - Behave is invalidate"),static_cast<unsigned long>(vid->m_spriteClass));
        return 0;
    }

    if (!IsMapEdit() && !OptLoad() && sprite) {
        const int function=*reinterpret_cast<const int*>(reinterpret_cast<const unsigned char*>(sprite->Vid())+0x440);
        if (function>=0)
            ScriptRun(function,sprite,0,0);
    }
    return sprite;
}

int MOUSETIPS::IsOut()
{
    return tip!=0;
}

int MAP::OptSelectSpriteUnderCursor()
{
    return static_cast<int>((m_flags>>20)&1u);
}

int MAP::OptDrawHpLines(const SPRITE* spr)
{
    if (m_flags&0x00000400u)
        return 1;
    if (m_mousetips.IsOut() && m_player[0]->SpriteUnderCursor()==spr)
        return 1;
    return 0;
}

void MAP::CreateEmptyHardwareGround()
{
    if (m_noVid<0x401)
        m_noVid=0x401;

    if (m_vids[1024]) {
        delete m_vids[1024];
        m_vids[1024]=0;
    }

    m_vids[1024]=new VID_HARDWARE(1024,static_cast<int>(SizeX()),static_cast<int>(SizeY()));
    if (!m_vids[1024])
        return;

    // Retail copies MAP+0x2C0 to VID+0x45C immediately after construction.
    m_vids[1024]->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);

    CreateSprite(m_vids[1024],SizeX()/2.0f,SizeY()/2.0f,0.0f,ANGLE(static_cast<uint8_t>(0)),0);
    MYERROR::Log(::Error,"Create Empty Hardware Ground");
}

RELATION::RELATION()
    : oldSprites(), newSprites()
{
}

MENU::MENU()
    : SPRITE_LIST()
{
    // Retail initializes the pointer first, then clears only the two declared
    // click bitfields.  The upper 30 bits remain untouched.
    sprite=0;
    clickFlags&=~1u;
    clickFlags&=~2u;
}

MOUSETIPS::MOUSETIPS()
    : tip(0)
{
}

PROFILE::PROFILE(const STRING* filename)
    : FileName()
{
    Load(filename);
}

SPRITE* MAP::ReadPointer(STREAM* res)
{
    int token=0;
    res->Read(&token,4u);
    if (token==-1)
        return reinterpret_cast<SPRITE*>(-1);
    return m_relation.Decode(reinterpret_cast<SPRITE*>(token));
}

SPRITE* MAP::OldLoadSprite(RESOURCE* res)
{
    int token=0;
    res->Read(&token,4u);
    if (token==-1)
        return reinterpret_cast<SPRITE*>(-1);

    short nvid=0;
    short x=0;
    short y=0;
    short z=0;
    unsigned char direction=0;
    unsigned char unused=0;
    res->Read(&nvid,2u);
    res->Read(&x,2u);
    res->Read(&y,2u);
    res->Read(&z,2u);
    res->Read(&direction,1u);
    res->Read(&unused,1u);

    SPRITE* sprite=0;
    if (ValidateVid(nvid)) {
        sprite=CreateSprite(m_vids[nvid],static_cast<float>(x),static_cast<float>(y),
                            static_cast<float>(z),ANGLE(direction),0);
    } else {
        Error(3,const_cast<char*>("sprite, this vid not exist"),static_cast<unsigned long>(nvid));
    }

    m_relation.Insert(reinterpret_cast<SPRITE*>(token),sprite);
    return sprite;
}

void MAP::RestoreDeviceObjects()
{
    for (int i=0;i<m_noVid;++i) {
        VID* vid=m_vids[i];
        if (vid && vid->IsFontType())
            static_cast<VID_FONT*>(vid)->RestoreDeviceObjects();
    }
}

void MAP::InvalidateDeviceObjects()
{
    for (int i=0;i<m_noVid;++i) {
        VID* vid=m_vids[i];
        if (vid && vid->IsFontType())
            static_cast<VID_FONT*>(vid)->InvalidateDeviceObjects();
    }
}

void MAP::LoadWeapon(RESOURCE* res)
{
    if (m_weapon)
        ::operator delete(m_weapon);
    m_weapon=0;
    m_noWeapon=res->Load(0x50414557u,&m_weapon,0x264);

    for (int i=0;i<m_noWeapon;++i) {
        unsigned char* const weapon=reinterpret_cast<unsigned char*>(m_weapon)+i*0x264;
        for (int j=0;j<8;++j) {
            float* const first=reinterpret_cast<float*>(weapon+0x224+j*4);
            float* const second=reinterpret_cast<float*>(weapon+0x244+j*4);
            if (*reinterpret_cast<unsigned int*>(first)!=0x497423F0u)
                *first/=1000.0f;
            if (*reinterpret_cast<unsigned int*>(second)!=0x497423F0u)
                *second/=1000.0f;
        }
    }
}

// Complete retail VID factory.  The temporary PICTURE conversion and path
// resolution deliberately preserve the old editor behavior, including the
// early-return paths which do not remove a converted temporary file.
VID* MAP::CreateVid(RESOURCE* res,int nvid)
{
    VID* vid=0;
    STRING name;
    STRING filename;
    VID scratch;

    name.Read(res);
    scratch.m_dotFrameCount=32000;
    const int parametersPos=res->Tell();
    scratch.LoadParameters(res);
    filename.Read(res);
    filename=filename.ToLower();
    res->Seek(parametersPos);

    // Reuse a compatible already-loaded resource through the retail mirror
    // chain.  Hardware resources intentionally ignore software gamma state.
    int i=0;
    for (;i<m_noVid;++i) {
        if (!ValidateVid(i))
            continue;
        VID* const other=m_vids[i];
        STRING* const otherFilename=reinterpret_cast<STRING*>(
            reinterpret_cast<unsigned char*>(other)+0x2EC);
        if (!otherFilename->operator==(&filename))
            continue;
        if (scratch.m_spriteClass==8) {
            if (other->m_spriteClass!=8)
                continue;
        } else if (other->m_spriteClass==8) {
            continue;
        }
        if (!other->IsHardwareType()) {
            GAMMA* const otherGamma=reinterpret_cast<GAMMA*>(
                reinterpret_cast<unsigned char*>(other)+0x2D8);
            GAMMA* const scratchGamma=reinterpret_cast<GAMMA*>(
                reinterpret_cast<unsigned char*>(&scratch)+0x2D8);
            if (!otherGamma->operator==(scratchGamma))
                continue;
        }
        if (!other->IsHardwareType() && (other->PropGamma() ^ scratch.PropGamma()))
            continue;

        vid=other->CreateMirror();
        vid->m_idx=nvid;
        vid->m_name.operator=(&name);
        vid->m_resourceName.operator=(&filename);
        vid->LoadParameters(res);
        break;
    }
    if (i<m_noVid) {
        vid->SetLayer();
        return vid;
    }

    const int isFont=filename.HaveSubStr(".fon") || filename.HaveSubStr(".ttf");
    const int isPicture=filename.HaveSubStr(".tga") || filename.HaveSubStr(".bmp") ||
                        filename.HaveSubStr(".flc");
    int removeTemp=0;

    if (isPicture) {
        PICTURE_MAKEVID* converter=0;
        if (scratch.m_spriteClass==19)
            converter=new PICTURE_FONT();
        else
            converter=new PICTURE_MAKEVID();

        if (converter->Load(filename,STRING(STRING::EMPTY),STRING(STRING::EMPTY))) {
            MYERROR::Log(::Error,"LOAD::Can't open file %s",filename.m_buf);
        } else {
            filename=FTempFile("c:\\tmp","vid");
            removeTemp=1;
            const int makeResult=converter->MakeVid(0,filename);
            (void)makeResult;
        }
        delete converter;
    }

    // The retail compiler ends the local RESOURCE lifetime before SetLayer()
    // (MapEdit.exe 0x0041E557), so keep the resource in a nested scope rather
    // than letting its destructor run at function return.
    {
    RESOURCE vidFile;
    if (!isFont) {
        STRING resourceName=res->GetFileName().ToLower();
        if (resourceName.HaveSubStr(".map") && resourceName.HaveSubStr(":\\")) {
            const STRING sub=filename.BeforeLast("\\");
            resourceName=resourceName.BeforeLast("\\");
            resourceName=resourceName.BeforeLast(sub.m_buf);
        } else {
            resourceName=STRING::EMPTY;
        }

        const STRING path=resourceName+filename;
        const int vidOpenResult=vidFile.OpenForRead(&path,0x20444956u);
        if (vidOpenResult) {
            Graph->FlipToGDISurface();
            MYERROR::Log(::Error,"LOAD::Can't open file %s",filename.m_buf);
            return vid;
        }
        if (vidFile.GoBegin(0x44414548u))
            MYERROR::Log(::Error,"!!!ERROR!!!VID '%s': Load() not HEAD ",filename.m_buf);
        vidFile.Read(&scratch.m_extraTypeFlags,2u);
    }

    if (isFont) {
        vid=new VID_FONT();
    } else if (scratch.IsPseudo3DType()) {
        return 0;
    } else if (scratch.IsLightType()) {
        vid=new VID_LIGHT();
    } else if (scratch.IsHardwareType() && scratch.IsAlphaType() && scratch.IsZBufferType()) {
        vid=new VID_HARDWARE_Z();
    } else if (scratch.IsHardwareType()) {
        vid=new VID_HARDWARE();
    } else if (scratch.m_spriteClass==8) {
        vid=new VID_SOFTWARE16();
    } else if (Graph->BytesPerPixel()==4) {
        vid=new VID_SOFTWARE();
    } else {
        vid=new VID_SOFTWARE16();
    }

    vid->m_extraTypeFlags=scratch.m_extraTypeFlags;
    vid->m_idx=nvid;
    vid->m_name.operator=(&name);
    vid->m_resourceName.operator=(&filename);
    vidFile.Read(&vid->m_phaseRandomInterval,2u);
    vidFile.Read(&vid->m_dotFrameCount,2u);
    vidFile.Read(&vid->m_regionTileStepX,2u);
    vidFile.Read(&vid->m_regionTileStepY,2u);
    vid->LoadParameters(res);
    vid->Load(&vidFile);
    vidFile.Close();
    if (removeTemp)
        FRemove(&filename);
    } // RESOURCE::~RESOURCE() precedes SetLayer() in the canonical executable.
    vid->SetLayer();
    return vid;
}


void MAP::LoadVid(RESOURCE* res)
{
    int idx=0;
    const unsigned long start=timeGetTime();
    if (!m_weapon) {
        LoadWeapon(res);
        if (m_noWeapon)
            MYERROR::Log(::Error,"LoadWeapon::No=%-5i             sizeof(WEAPON)=%-4i",m_noWeapon,0x264);
    }

    if (res->GoBegin(0x204A424Fu)) {
        Error(11,const_cast<char*>("load 'VID'"),0);
        return;
    }

    const int hadVids=(m_noVid!=0);
    do {
        idx=-1;
        res->Read(&idx,4u);
        // reject negative values here and does not continue after reporting the
        // error; preserve that behavior exactly for runtime compatibility.
        if (idx>=2048)
            Error(4,const_cast<char*>("nvid > MAX_VID"),static_cast<unsigned long>(idx));

        if (m_vids[idx]) {
            VID* old=m_vids[idx];
            delete old;
            m_vids[idx]=0;
            Error(5,const_cast<char*>("this VID already loaded"),static_cast<unsigned long>(idx));
        }

        m_vids[idx]=CreateVid(res,idx);
        if (!m_vids[idx])
            continue;
        if (idx>=m_noVid)
            m_noVid=idx+1;
        if (hadVids)
            m_vids[idx]->SetExtraType();

        VID* vid=m_vids[idx];
        if (vid->m_weaponIndex<m_noWeapon) {
            vid->m_weapon=reinterpret_cast<WEAPON*>(
                reinterpret_cast<unsigned char*>(m_weapon)+vid->m_weaponIndex*0x264);
        } else {
            vid->Error(10,const_cast<char*>("nWeapon > noWeapon"),static_cast<unsigned long>(vid->m_weaponIndex));
            vid->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);
        }
        Graph->DrawLoadBar(m_vids[0]);
    } while (res->GoNextSub(0x204A424Fu)==0);

    int maxX=0,maxY=0;
    for (idx=0;idx<m_noVid;++idx) {
        VID* vid=m_vids[idx];
        if (!vid)
            continue;
        vid->SetChildAndLink();
        if (vid->m_regionTileStepX>maxX) maxX=vid->m_regionTileStepX;
        if (vid->m_regionTileStepY>maxY) maxY=vid->m_regionTileStepY;
    }
    MYERROR::Log(::Error,
        "LoadVid::No   =%-15i   sizeof(VID)   =%-5i    load time     =%ims   MaxSizeX,Y=%i,%i",
        m_noVid,0x484,timeGetTime()-start,maxX,maxY);
}

// The resource contains 26 packed 32-bit values. The retail constructor reads
// the slot at +0x28 into a throwaway local because MAP startup overwrites
// debugMode from the profile immediately afterwards.
CONSTANT::CONSTANT(RESOURCE* res)
{
    if (res->GoBegin(0x54534E43u)!=0) { // 'CNST'
        MYERROR::Log(::Error,"!!!ERROR!!! CNST Load Constant section not found");
        return;
    }

    uint32_t discardedDebug=0;
    uint32_t* words=reinterpret_cast<uint32_t*>(this);
    for (int i=0; i<10; ++i)
        res->Read(&words[i],4);
    res->Read(&discardedDebug,4);
    for (int i=11; i<26; ++i)
        res->Read(&words[i],4);

    maxShiftSpeedX/=1000.0f;
    maxShiftSpeedY/=1000.0f;
    gravity/=1000000.0f;
    gravity2/=1000000.0f;
    MasterRepairSpeed/=1000.0f;
    RailRepairSpeed/=1000.0f;
    SafeClashSpeed/=1000.0f;
}

// MapEdit.exe map.cpp:1327.
SPRITE* MAP::NextSpriteByType(int nlayer,int* i,unsigned int type)
{
    SPRITE* sprite=NextSprite(nlayer,i);
    while (sprite && !sprite->IsSpriteType(type))
        sprite=NextSprite(nlayer,i);
    return sprite;
}

// Original inline owner emitted into creature.obj from map.h:366.
SPRITE* MAP::FirstSpriteByType(int nlayer,int* i,unsigned int type)
{
    *i=m_layers[nlayer].No();
    return NextSpriteByType(nlayer,i,type);
}
