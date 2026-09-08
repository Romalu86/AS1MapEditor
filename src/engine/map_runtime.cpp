#include "mapedit/runtime.hpp"

int MAP::StartTact()
{
    unsigned long deltaTime;
    do {
        deltaTime = timeGetTime() - RealCurrentTime;
    } while (!deltaTime);

    PrevRealCurrentTime = RealCurrentTime;
    RealCurrentTime = timeGetTime();
    PrevCurrentTime = CurrentTime;
    if (deltaTime > 0x47u)
        deltaTime = 71u;
    CurrentTime += (int)(deltaTime * m_speed);
    ++m_fpsCnt;

    if (RealCurrentTime - prev_second_time >= 1000u) {
        prev_second_time = RealCurrentTime;
        m_fps = m_fpsCnt;
        m_fpsCnt = 0;
    }

    m_input.Tact();

    if (!(CurrentTime & 3u)) {
        for (int layerIndex=0; layerIndex<18; ++layerIndex) {
            SPRITE_LIST& layer = m_layers[layerIndex];
            const int count = layer.No();
            for (int hole=0; hole<count; ++hole) {
                if (*layer[hole])
                    continue;

                int deleted = 1;
                for (int i=hole+1; i<layer.No(); ++i) {
                    SPRITE* sprite = *layer[i];
                    if (!sprite) {
                        ++deleted;
                    } else {
                        *layer[i-deleted] = sprite;
                    }
                }
                layer.DeleteFrom(layer.No()-deleted);
                break;
            }
        }
    }

    if (m_flags & 0x40u) {
        m_flags &= ~0x40u;
        Load(m_startupLoad);
    }

    MSG_OLD msg;
    while (PeekMessageA(&msg,0,0,0,1)) {
        if (msg.message == 0x12u)
            return 1;
        if (m_hWnd && TranslateAcceleratorA(m_hWnd,m_hAccel,&msg))
            continue;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

int MAP::WorkWndMessage(HWND__* hwnd,unsigned long msg,unsigned long wParam,unsigned long lParam)
{
    if ((m_flags & 8u) && m_input.WorkWndMessage(hwnd,msg,wParam,lParam))
        return 1;

    switch (msg) {
    case 0x000Fu: // WM_PAINT
        MYERROR::Log(::Error,"WM_PAINT");
        break;

    case 0x001Cu: // WM_ACTIVATEAPP
        m_flags = (m_flags & ~8u) | (wParam ? 8u : 0u);
        if (Sound && Mouse) {
            if (m_flags & 8u) {
                Sound->Resume();
                Mouse->Enable();
            } else {
                Sound->Pause();
                Mouse->Disable();
            }
        }
        break;

    case 0x0002u: { // WM_DESTROY
        if (!Graph->CapsFullScreen()) {
            RECT_OLD rect;
            GetWindowRect(m_hWnd,&rect);
            Registry->SetInt(STRING("WindowPositionX"),rect.left);
            Registry->SetInt(STRING("WindowPositionY"),rect.top);
        }
        m_hWnd = 0;
        PostQuitMessage(0);
        break;
    }

    case 0x0231u: // WM_ENTERSIZEMOVE
    case 0x0211u: // WM_ENTERMENULOOP
        Graph->BeginPause();
        Sound->Pause();
        break;

    case 0x0232u: // WM_EXITSIZEMOVE
    case 0x0212u: // WM_EXITMENULOOP
        Graph->EndPause();
        Sound->Resume();
        break;

    case 0x0112u: // WM_SYSCOMMAND
        switch (wParam) {
        case 0xF000u: // SC_SIZE
        case 0xF010u: // SC_MOVE
        case 0xF030u: // SC_MAXIMIZE
        case 0xF170u: // SC_MONITORPOWER
            if (Graph->CapsFullScreen())
                return 1;
            break;
        }
        break;
    }
    return 0;
}

void MAP::DeletePointerToSprite(SPRITE* sprite)
{
    for (int i=0;i<4;++i) {
        if (m_player[i]) {
            void** vtable=*reinterpret_cast<void***>(m_player[i]);
            typedef void (__thiscall *DeletePointerMethod)(void*,SPRITE*);
            reinterpret_cast<DeletePointerMethod>(vtable[1])(m_player[i],sprite);
        }
    }

    m_logic.DeletePointerToObject(sprite);
    m_groups.DeletePointerToSprite(sprite);

    if (sprite->NoRef()>1 && Hash) {
        int index=0;
        for (SPRITE* unit=Hash->FirstUnit(&index);unit;unit=Hash->NextUnit(&index)) {
            void** vtable=*reinterpret_cast<void***>(unit);
            typedef void (__thiscall *DeletePointerMethod)(void*,SPRITE*);
            reinterpret_cast<DeletePointerMethod>(vtable[4])(unit,sprite);
        }
    }

    for (int layer=0;layer<18;++layer) {
        if (sprite->NoRef()<=1)
            continue;
        int index=0;
        for (SPRITE* unit=FirstSprite(layer,&index);unit;unit=NextSprite(layer,&index)) {
            void** vtable=*reinterpret_cast<void***>(unit);
            typedef void (__thiscall *DeletePointerMethod)(void*,SPRITE*);
            reinterpret_cast<DeletePointerMethod>(vtable[4])(unit,sprite);
        }
    }
}

int MAP::NPlayer() { return m_curArmy; }

PLAYER* MAP::Player(int narmy) { return m_player[narmy&3]; }

int MAP::GetScrollType() { return static_cast<int>(m_shiftFlag); }

float MAP::GetTimeCoeff() { return m_speed; }

void MAP::SetSelectSpriteUnderCursor(int flag)
{
    m_flags=(m_flags&~0x00100000u)|(flag ? 0x00100000u : 0u);
}

void MAP::LoadInEndTact(const STRING* filename)
{
    m_flags|=0x40u;
    m_startupLoad=*filename;
}

void MAP::SetFlagman(int army,SPRITE* sprite)
{
    m_player[army&3]->SetFlagman(sprite);
}

void MAP::ReloadVid()
{
    RESOURCE res;
    for (int i=0;i<m_noVid;++i) {
        VID* v=VidSlot(i);
        if (v && v->m_exchangeVid && v->m_exchangeVid!=v)
            ExchangeVid(v->m_exchangeVid,v);
    }
    if (res.OpenForRead(&m_resName,0x41544144u)) {
        char text[]="resource file";
        Error(7,text,0);
        return;
    }
    LoadWeapon(&res);
    if (res.GoBegin(0x204A424Fu)) {
        char text[]="load 'VID'";
        Error(11,text,0);
        res.Close();
        return;
    }
    do {
        int idx=0;
        res.Read(&idx,4);
        if (idx>=MAP::kVidCapacity) {
            char text[]="nvid > MAX_VID";
            Error(4,text,(unsigned long)idx);
            continue;
        }
        if (idx>=0 && idx<m_noVid && VidSlot(idx)) {
            VID* current=VidSlot(idx);
            if (current->m_exchangeVid)
                idx=current->m_exchangeVid->m_idx;
            if (idx>=0 && idx<m_noVid && VidSlot(idx)) {
                VID* v=VidSlot(idx);
                v->m_name.Read(&res);
                v->LoadParameters(&res);
                if (v->m_weaponIndex>=0 && v->m_weaponIndex<m_noWeapon)
                    v->m_weapon=reinterpret_cast<WEAPON*>(reinterpret_cast<unsigned char*>(m_weapon)+0x264u*(unsigned int)v->m_weaponIndex);
                else
                    v->m_weapon=reinterpret_cast<WEAPON*>(m_weapon);
            }
        }
    } while (!res.GoNextSub(0x204A424Fu));
    for (int i=0;i<m_noVid;++i) {
        VID* v=VidSlot(i);
        if (v && !v->IsExtraType())
            v->SetChildAndLink();
    }
    res.Close();
}
