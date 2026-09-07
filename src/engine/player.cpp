#include "mapedit/runtime.hpp"

// Retail MapEdit PLAYER base owner.  The function/address comments are taken
// from the executable/debug metadata player.obj and are intentionally kept here because
// PLAYER_STEAM must reuse this exact 12-slot base ABI.

PLAYER::PLAYER(int type,int army)
    : m_money(1000), m_type(type), m_army(army),
      m_flagman(), m_stateBar(), m_underCursor()
{
    m_flagman=static_cast<SPRITE*>(0);
    m_underCursor=static_cast<SPRITE*>(0);
}

PLAYER::~PLAYER()
{
    Release();
}

void PLAYER::DeletePointerToSprite(SPRITE* sprite)
{
    m_stateBar.Delete(sprite);
    if (static_cast<SPRITE*>(m_flagman)==sprite)
        m_flagman=static_cast<SPRITE*>(0);
    if (static_cast<SPRITE*>(m_underCursor)==sprite)
        m_underCursor=static_cast<SPRITE*>(0);
}

void PLAYER::Release()
{
    m_flagman=static_cast<SPRITE*>(0);
    m_underCursor=static_cast<SPRITE*>(0);
    m_money=1000;
}

void PLAYER::Save(STREAM* res)
{
    res->Write(&m_flagman,4);
}

void PLAYER::Load(STREAM* res)
{
    m_flagman=Map->ReadPointer(res);
}

STRING PLAYER::GetMouseTipsString()
{
    if (static_cast<SPRITE*>(m_underCursor)) {
        STRING defaultString;
        STRING section("Units");
        char number[128];
        const int nvid=m_underCursor->Vid()->m_idx;
        STRING key(_itoa(nvid,number,10));
        return Profile->GetString(&section,&key,&defaultString);
    }
    return STRING();
}

void PLAYER::SetFlagman(SPRITE* unit) { m_flagman=unit; }

// Exact retail no-op virtual leaves.
// 0x00420DD0, 0x00420DE0, 0x00420DF0, 0x00420E00, 0x00420E10.
void PLAYER::Control(INPUT*) {}
void PLAYER::StateBarOn() {}
void PLAYER::StateBarOff() {}
void PLAYER::PutMessage(const STRING*,float,float) {}
void PLAYER::AddUnitToStateBar(SPRITE*) {}

int PLAYER::IsStateBarOn() { return m_stateBar.No()!=0; }

int PLAYER::GetMoney() { return m_money; }

void PLAYER::SetMoney(int newMoney) { m_money=newMoney; }

int PLAYER::AddMoney(int addMoney)
{
    m_money+=addMoney;
    return m_money;
}

// ---- PLAYER_STEAM embedded MESSAGE owner ---------------------------------

// deliberately left untouched.
MESSAGE_STACK::MESSAGE_STACK() : text() {}
MESSAGE_STACK::MESSAGE_STACK(const STRING str,float x,float y)
    : text(str), eventX(x), eventY(y),
      time(static_cast<unsigned long>(Const->MessageStartDelay)) {}
MESSAGE_STACK::MESSAGE_STACK(const MESSAGE_STACK& that)
    : text(that.text), eventX(that.eventX), eventY(that.eventY), time(that.time) {}
MESSAGE_STACK::~MESSAGE_STACK() {}
MESSAGE_STACK& MESSAGE_STACK::operator=(const MESSAGE_STACK& that)
{
    text=that.text;
    eventX=that.eventX;
    eventY=that.eventY;
    time=that.time;
    return *this;
}

// This is the complete LIST<MESSAGE_STACK> specialization used by MESSAGE.
template<> void LIST<MESSAGE_STACK>::Release();
template<> void LIST<MESSAGE_STACK>::Expand(int newAllocation);
template<> LIST<MESSAGE_STACK>::LIST() : m_no(0),m_max(0),m_data(0) {}
template<> LIST<MESSAGE_STACK>::~LIST() { Release(); }

template<> int LIST<MESSAGE_STACK>::No() { return m_no; }

template<> MESSAGE_STACK* LIST<MESSAGE_STACK>::operator[](int index)
{
    return m_data+index;
}

template<> void LIST<MESSAGE_STACK>::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<MESSAGE_STACK>::Insert(MESSAGE_STACK item)
{
    ExpandForInsert();
    m_data[m_no]=item;
    ++m_no;
}

template<> void LIST<MESSAGE_STACK>::DeleteNumberS(int index)
{
    if (index>=0 && index<m_no) {
        --m_no;
        while (index<m_no) {
            m_data[index]=m_data[index+1];
            ++index;
        }
    }
    if (m_no==0)
        Release();
}

template<> void LIST<MESSAGE_STACK>::Release()
{
    m_max=0;
    m_no=0;
    if (m_data)
        delete[] m_data;
    m_data=0;
}

template<> void LIST<MESSAGE_STACK>::Expand(int newAllocation)
{
    if (newAllocation<=m_max)
        return;

    MESSAGE_STACK* const oldData=m_data;
    const int oldMax=m_max;
    MESSAGE_STACK* const newData=new MESSAGE_STACK[newAllocation];
    if (!newData)
        MYERROR::LogExit(Error,"!!!ERROR!!!::LIST: Not enough memory %i",newAllocation);

    if (oldData) {
        // Retail copies every constructed slot in the old allocation, not only
        // the logical [0,m_no) prefix.
        for (int i=0;i<oldMax;++i)
            newData[i]=oldData[i];
        delete[] oldData;
    }
    m_data=newData;
    m_max=newAllocation;
}

MESSAGE::MESSAGE(int nvidFont,int nvidButton,float messageX,float messageY,
                 int maxMessage,unsigned long delayMessageShift)
    : m_maxMessage(maxMessage), m_delayShift(delayMessageShift),
      m_nvidFont(nvidFont), m_nvidButton(nvidButton),
      m_messageX(messageX), m_messageY(messageY), m_current(-1),
      m_stack()
{
    // Retail ctor deliberately does not initialize m_lastShift/event arrays.
    for (int i=0;i<m_maxMessage;++i) {
        m_messageButton[i]=0;
        m_messageText[i]=0;
    }
}

MESSAGE::~MESSAGE()
{
    Release();
}

void MESSAGE::Release()
{
    for (int i=0;i<m_maxMessage;++i) {
        if (m_messageText[i])
            m_messageText[i]->ScalarDeletingDestructor(1);
        m_messageText[i]=0;

        if (m_messageButton[i])
            m_messageButton[i]->ScalarDeletingDestructor(1);
        m_messageButton[i]=0;
    }
}

void MESSAGE::DeletePointerToSprite(SPRITE* sprite)
{
    for (int i=0;i<m_maxMessage;++i) {
        if (m_messageText[i]==sprite)
            m_messageText[i]=0;
        if (m_messageButton[i]==sprite)
            m_messageButton[i]=0;
    }
}

void MESSAGE::PutToStack(const STRING* text,float x,float y)
{
    m_stack.Insert(MESSAGE_STACK(*text,x,y));
}

void MESSAGE::Tact()
{
    const int lineHeight=Map->Vid(m_nvidFont)->m_regionTileStepY+1;

    for (int i=0;i<m_stack.No();++i) {
        MESSAGE_STACK* const item=m_stack[i];
        const unsigned long delta=CurrentTime-PrevCurrentTime;
        if (delta>=item->time) {
            Put(&item->text,item->eventX,item->eventY);
            m_stack.DeleteNumberS(i);
            --i;
        } else {
            item->time-=delta;
        }
    }

    if (CurrentTime-m_lastShift>m_delayShift) {
        Shift();
        m_lastShift=CurrentTime;
    }

    MENU* const menu=Map->Menu();
    if (menu->IsLClick() && menu->NVidUnderCursor()==m_nvidButton) {
        const int screenOffset=static_cast<int>(
            menu->SpriteUnderCursor()->ScreenY()-Graph->ViewYMin());
        const int row=screenOffset/lineHeight;
        if (m_eventX[row]!=-999999.0f || m_eventY[row]!=-999999.0f)
            Map->SetShiftCoor(m_eventX[row],m_eventY[row],2);
    }
}

void MESSAGE::Put(const STRING* text,float x,float y)
{
    const int lineHeight=Map->Vid(m_nvidFont)->m_regionTileStepY+1;
    if ((*const_cast<STRING*>(text))=="")
        return;

    m_lastShift=CurrentTime;
    const int last=m_maxMessage-1;
    if (m_messageText[last])
        Shift();

    if (!m_messageText[last]) {
        m_messageText[last]=Map->CreateSprite(
            Map->Vid(m_nvidFont),
            m_messageX,
            Graph->ViewYMin()+m_messageY+2000.0f+
                static_cast<float>(lineHeight*last),
            2000.0f,ANGLE(static_cast<unsigned char>(0)),0);
    }
    if (m_messageText[last])
        m_messageText[last]->Action(120,reinterpret_cast<int>(text),0,0);

    m_eventX[last]=x;
    m_eventY[last]=y;

    if (x!=-1.0f || y!=-1.0f) {
        m_messageButton[last]=Map->CreateSprite(
            Map->Vid(m_nvidButton),
            m_messageX-10.0f,
            Graph->ViewYMin()+m_messageY+2000.0f+
                static_cast<float>(lineHeight*last+lineHeight/2),
            2000.0f,ANGLE(static_cast<unsigned char>(0)),0);
    }

    Sound->PlaySFX(106,0,0);
}

void MESSAGE::Shift()
{
    const int lineHeight=Map->Vid(m_nvidFont)->m_regionTileStepY+1;

    if (m_messageText[0])
        m_messageText[0]->ScalarDeletingDestructor(1);
    m_messageText[0]=0;

    if (m_messageButton[0])
        m_messageButton[0]->ScalarDeletingDestructor(1);
    m_messageButton[0]=0;

    for (int i=1;i<m_maxMessage;++i) {
        if (m_messageButton[i]) {
            m_messageButton[i]->ChangeYCoor(
                m_messageButton[i]->Y()+static_cast<float>(m_current*lineHeight));
            m_messageButton[i-1]=m_messageButton[i];
            m_eventX[i-1]=m_eventX[i];
            m_eventY[i-1]=m_eventY[i];
            m_messageButton[i]=0;
        }
    }

    for (int i=1;i<m_maxMessage;++i) {
        if (m_messageText[i]) {
            m_messageText[i]->ChangeYCoor(
                m_messageText[i]->Y()+static_cast<float>(m_current*lineHeight));
            m_messageText[i-1]=m_messageText[i];
            m_messageText[i]=0;
        }
    }
}

int MENU::IsLClick()
{
    return static_cast<int>(clickFlags&1u);
}

int MENU::IsRClick()
{
    return static_cast<int>((clickFlags>>1)&1u);
}


// ---- PLAYER_STEAM lifetime / pointer-owner closure -----------------------
PLAYER_STEAM::PLAYER_STEAM(int type,int army)
    : PLAYER(type,army), m_flagmanPathDots(),
      m_message(14,449,20.0f,5.0f,3,15000ul)
{
    for (int i=0;i<10;++i)
        m_trains[i]=0;
    m_statebarMode=6;
    m_options&=~1u;
    m_options|=2u;
    m_statebarDepoUnit=-1;
    g_inputAllowFirst=0;
    g_inputAllowSecond=0;
}

PLAYER_STEAM::~PLAYER_STEAM()
{
    Release();
}

void PLAYER_STEAM::DeletePointerToSprite(SPRITE* sprite)
{
    m_message.DeletePointerToSprite(sprite);
    m_flagmanPathDots.Delete(sprite);

    if (Flagman()==sprite) {
        m_flagman=static_cast<SPRITE*>(0);
        ChangeStateBar(0);
    }

    for (int i=0;i<10;++i) {
        if (m_trains[i]!=sprite)
            continue;
        sprite->Release();
        m_trains[i]=0;
        ChangeStateBar(m_statebarMode);
    }

    PLAYER::DeletePointerToSprite(sprite);
}

void PLAYER_STEAM::Release()
{
    StateBarOff();
    m_message.Release();
    for (int i=0;i<10;++i) {
        if (!m_trains[i])
            continue;
        m_trains[i]->Release();
        m_trains[i]=0;
    }
    PLAYER::Release();
}

void PLAYER_STEAM::PutMessage(const STRING* text,float x,float y)
{
    m_message.PutToStack(text,x,y);
}

// ---- PLAYER_STEAM derived leaf/short owner ring ------------------------
// These methods are intentionally integrated before the three large control/
// state-bar SCCs.  Every body below is taken directly from MapEdit.exe and
// does not use a compatibility shim.

void PLAYER_STEAM::StateBarOn()
{
    Map->Vid(14)->SetPropHide(0);
    Map->Vid(16)->SetPropHide(0);
    Map->Vid(17)->SetPropHide(0);
    Map->Vid(18)->SetPropHide(0);
    Map->Vid(449)->SetPropHide(0);

    if (m_stateBar.No()==0) {
        const int iconStep=41;
        const float baseX=Graph->SizeX()/2.0f+95.0f-320.0f;
        const float baseY=Graph->ViewYMax()-
            static_cast<float>(Map->Vid(450)->m_regionTileStepY/2)-2.0f;

        for (int i=1;i<11;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(4),baseX+static_cast<float>(i*iconStep)-14.0f,
                baseY-13.0f+2005.0f,2005.0f,ANGLE(static_cast<unsigned char>(0)),0));
        }

        for (int i=1;i<11;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(8),baseX+static_cast<float>(i*iconStep)-17.0f,
                baseY-17.0f-41.0f+2005.0f,2005.0f,ANGLE(static_cast<unsigned char>(0)),0));
        }

        m_stateBar.Insert(Map->CreateSprite(
            Map->Vid(456),Graph->SizeX()/2.0f,baseY+2004.0f,
            2004.0f,ANGLE(static_cast<unsigned char>(0)),0));
        m_stateBar.Insert(Map->CreateSprite(
            Map->Vid(498),baseX-49.0f,baseY+8.0f+2003.0f,
            2003.0f,ANGLE(static_cast<unsigned char>(0)),0));
        m_stateBar.Insert(Map->CreateSprite(
            Map->Vid(499),baseX-11.0f,baseY+3.0f+2003.0f,
            2003.0f,ANGLE(static_cast<unsigned char>(0)),0));

        for (int i=1;i<11;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(455),baseX+static_cast<float>(i*iconStep),
                baseY+2003.0f,2003.0f,ANGLE(static_cast<unsigned char>(0)),0));
        }
        for (int i=1;i<11;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(463),baseX+static_cast<float>(i*iconStep)+3.0f,
                baseY+22.0f+2002.0f,2002.0f,ANGLE(static_cast<unsigned char>(0)),0));
        }
        for (int i=1;i<11;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(457),baseX+static_cast<float>(i*iconStep)+3.0f,
                baseY+21.0f+2001.0f,2001.0f,ANGLE(static_cast<unsigned char>(0)),0));
        }
        for (int i=1;i<11;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(458),baseX+static_cast<float>(i*iconStep)+3.0f,
                baseY+24.0f+2001.0f,2001.0f,ANGLE(static_cast<unsigned char>(0)),0));
        }
        for (int i=1;i<11;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(459),baseX+static_cast<float>(i*iconStep)-17.0f,
                baseY+22.0f+2001.0f,2001.0f,ANGLE(static_cast<unsigned char>(0)),0));
        }

        VID* const mapButtonVid=Map->Vid(464);
        m_stateBar.Insert(Map->CreateSprite(
            mapButtonVid,
            Graph->ViewXMax()-static_cast<float>(mapButtonVid->m_regionTileStepX/2),
            Graph->ViewYMax()-static_cast<float>(mapButtonVid->m_regionTileStepY/2)+2001.0f,
            2001.0f,ANGLE(static_cast<unsigned char>(0)),0));

        VID* const stateBarVid=Map->Vid(450);
        m_stateBar.Insert(Map->CreateSprite(
            stateBarVid,Graph->SizeX()/2.0f,
            Graph->ViewYMax()-static_cast<float>(stateBarVid->m_regionTileStepY/2)+2000.0f,
            2000.0f,ANGLE(static_cast<unsigned char>(0)),0));

        VID* const sideVid=Map->Vid(469);
        const float sideY=Graph->ViewYMin()+sideVid->m_footprintHeight/2.0f+2000.0f;
        const float sideX=Graph->ViewXMax()+1.0f-sideVid->m_footprintWidth/2.0f;
        m_stateBar.Insert(Map->CreateSprite(sideVid,sideX,sideY,2000.0f,ANGLE(static_cast<unsigned char>(0)),0));

        m_stateBar.Insert(Map->CreateSprite(
            Map->Vid(2),Graph->ViewXMax()-75.0f-20.0f,
            Graph->ViewYMin()+4.0f+2001.0f,2001.0f,ANGLE(static_cast<unsigned char>(0)),0));
        m_stateBar.Insert(Map->CreateSprite(
            Map->Vid(2),Graph->ViewXMax()-43.0f,
            Graph->ViewYMin()+4.0f+2001.0f,2001.0f,ANGLE(static_cast<unsigned char>(0)),0));

        m_stateBar.Insert(Map->CreateSprite(
            Map->Vid(502),sideX-30.0f,sideY,2000.0f,ANGLE(static_cast<unsigned char>(0)),0));
        m_stateBar.Insert(Map->CreateSprite(
            Map->Vid(503),sideX+30.0f,sideY,2000.0f,ANGLE(static_cast<unsigned char>(0)),0));

        (*m_stateBar[22])->ChangeRealDirection(1u);
        (*m_stateBar[22])->ChangeArmy(2);

        if (Map->NPlayer()!=m_army) {
            for (int i=0;i<m_stateBar.No();++i)
                (*m_stateBar[i])->InvisibleOn();
        }
    }

    if (static_cast<SPRITE*>(m_flagman) && m_flagman->IsSpriteClass(0x15u))
        ChangeStateBar(1);
    else
        ChangeStateBar(0);

    Map->SetScrollBox(0.0f,0.0f,Map->SizeX(),
        Map->SizeY()+static_cast<float>(Map->Vid(450)->m_regionTileStepY)-2.0f);
}

void PLAYER_STEAM::StateBarOff()
{
    Map->Vid(14)->SetPropHide(1);
    Map->Vid(16)->SetPropHide(1);
    Map->Vid(17)->SetPropHide(1);
    Map->Vid(18)->SetPropHide(1);
    Map->Vid(449)->SetPropHide(1);
    Map->SetScrollBox(0.0f,0.0f,Map->SizeX(),Map->SizeY());
    if (m_stateBar.No()!=0)
        m_stateBar.DeleteAll();
    m_statebarMode=6;
}

int PLAYER_STEAM::ChangeIcon(int nIcon,SPRITE* sprite)
{
    if (!sprite->Vid() || sprite->Vid()->m_idx==0)
        return 1;

    int icon;
    if (sprite->HaveLink() && sprite->Vid()->m_linkVid->m_weapon->m_icon!=0)
        icon=sprite->Vid()->m_linkVid->m_weapon->m_icon;
    else
        icon=sprite->Vid()->m_weapon->m_icon;
    (*m_stateBar[nIcon+23])->ChangeRealDirection(static_cast<unsigned int>(icon));
    return 0;
}

int PLAYER_STEAM::ChangeIcon(int nIcon,VID* vid)
{
    if (!vid || vid->m_idx==0)
        return 1;

    int icon;
    if (vid->m_linkVid && vid->m_linkVid->m_weapon->m_icon!=0)
        icon=vid->m_linkVid->m_weapon->m_icon;
    else
        icon=vid->m_weapon->m_icon;
    (*m_stateBar[nIcon+23])->ChangeRealDirection(static_cast<unsigned int>(icon));
    return 0;
}

int PLAYER_STEAM::ChangeBuildIcon(int nIcon,VID* vid)
{
    if (!vid || vid->m_idx==0 || nIcon+80>=m_stateBar.No())
        return 1;

    int icon;
    if (vid->m_linkVid && vid->m_linkVid->m_weapon->m_icon!=0)
        icon=vid->m_linkVid->m_weapon->m_icon;
    else
        icon=vid->m_weapon->m_icon;
    (*m_stateBar[nIcon+80])->ChangeRealDirection(static_cast<unsigned int>(icon));

    STRING buildTime=Int2Str(vid->GetBuildTime());
    (*m_stateBar[nIcon+10])->Action(120,
        reinterpret_cast<int>(&buildTime),0,0);
    return 0;
}

void PLAYER_STEAM::ChangeStateBar(int newMode)
{
    if (m_stateBar.No()==0 && newMode==2) {
        newMode|=0x10;
        StateBarOn();
    }
    if ((m_statebarMode&0x10)!=0 && newMode!=2 && newMode!=5)
        StateBarOff();
    if (m_stateBar.No()==0)
        return;

    if (m_statebarMode!=newMode)
        Sound->PlaySFX(109,0,0);

    if ((newMode&~0x10)==0)
        (*m_stateBar[22])->ChangeAnimation(6);
    else
        (*m_stateBar[22])->ChangeAnimation(0);

    m_statebarMode=(m_statebarMode&0x10)|newMode;
    (*m_stateBar[20])->ChangeXCoor((*m_stateBar[20])->X()-1000.0f);

    for (int i=0;i<m_stateBar.No();++i) {
        SPRITE* const sprite=*m_stateBar[i];
        if (sprite && (i<20 || i>=33))
            sprite->ChangeRealDirection(sprite->Vid()->m_noDirections-1u);
    }

    for (int i=0;i<10;++i) {
        (*m_stateBar[i+23])->ChangeDirection(ANGLE(static_cast<unsigned char>(0)));
        (*m_stateBar[i+23])->ChangeArmy(2);
        STRING empty("");
        (*m_stateBar[i+10])->Action(120,reinterpret_cast<int>(&empty),0,0);
    }

    while (m_stateBar.No()>80)
        m_stateBar.DeleteSpriteNumber(m_stateBar.No()-1);

    const int mode=newMode&~0x10;
    if (mode==0) {
        for (int i=0;i<10;++i) {
            if (!m_trains[i])
                continue;
            if (static_cast<SPRITE*>(m_flagman)==m_trains[i])
                (*m_stateBar[20])->ChangeXCoor((*m_stateBar[i+23])->X());
            if (ChangeIcon(i,m_trains[i])==0)
                (*m_stateBar[i+23])->ChangeArmy(0);
        }
        return;
    }
    if (mode!=2 && mode!=5)
        return;

    if (!static_cast<SPRITE*>(m_flagman) || !m_flagman->IsSpriteClass(0x18u)) {
        ChangeStateBar(0);
        return;
    }

    DEPO* const depo=static_cast<DEPO*>(static_cast<SPRITE*>(m_flagman));
    const float baseX=(*m_stateBar[23])->ScreenX();
    const float baseY=(*m_stateBar[23])->ScreenY()-41.0f;

    if (mode==5) {
        for (int i=0;i<10;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(455),baseX+static_cast<float>(i*41),baseY+2003.0f,
                2003.0f,ANGLE(static_cast<unsigned char>(0)),0));
            (*m_stateBar[i+80])->ChangeArmy(1);
        }
        for (int i=0;i<5;++i) {
            (*m_stateBar[i+80])->ChangeRealDirection(static_cast<unsigned int>(i+3));
            (*m_stateBar[i+80])->ChangeArmy(3);
        }
    } else {
        for (int i=0;i<10;++i) {
            m_stateBar.Insert(Map->CreateSprite(
                Map->Vid(455),baseX+static_cast<float>(i*41),baseY+2003.0f,
                2003.0f,ANGLE(static_cast<unsigned char>(0)),0));

            const int nvid=depo->CanBuildUnit(i);
            if (ChangeBuildIcon(i,Map->Vid(nvid))==0) {
                if (CanCreateUnit(Map->Vid(nvid)))
                    (*m_stateBar[i+80])->ChangeArmy(3);
                else
                    (*m_stateBar[i+80])->ChangeArmy(2);
            } else {
                (*m_stateBar[i+80])->ChangeArmy(1);
            }
        }

        for (int i=0;i<10;++i) {
            SPRITE* const sprite=Map->CreateSprite(
                Map->Vid(8),baseX+static_cast<float>(i*41)+18.0f,
                baseY+10.0f+2004.0f,2004.0f,
                ANGLE(static_cast<unsigned char>(0)),0);
            if (!sprite)
                continue;

            m_stateBar.Insert(sprite);
            sprite->Action(95,2,0,0);

            const char* key=0;
            switch (depo->CanBuildUnit(i)) {
            case 5:  key="F1";  break;
            case 10: key="F2";  break;
            case 20: key="F3";  break;
            case 25: key="F4";  break;
            case 80: key="F5";  break;
            case 85: key="F6";  break;
            case 45: key="F7";  break;
            case 30: key="F8";  break;
            case 35: key="F9";  break;
            case 82: key="F10"; break;
            case 97: key="F11"; break;
            case 90: key="F12"; break;
            case 75: key="[";   break;
            case 62: key="]";   break;
            default: break;
            }
            if (key) {
                STRING keyText(key);
                sprite->Action(120,reinterpret_cast<int>(&keyText),0,0);
            }
        }
    }

    m_stateBar.Insert(Map->CreateSprite(
        Map->Vid(497),Graph->SizeX()/2.0f,baseY+2002.0f,2002.0f,
        ANGLE(static_cast<unsigned char>(0)),0));

    for (int i=0;i<10;++i) {
        if (ChangeIcon(i,Map->Vid(depo->GetQueueUnit(i)))==0) {
            if (depo->IsPausedQueueUnit(i))
                (*m_stateBar[i+63])->ChangeRealDirection(3u);
            else
                (*m_stateBar[i+63])->ChangeRealDirection(4u);
        } else {
            (*m_stateBar[i+23])->ChangeRealDirection(2u);
        }
        (*m_stateBar[i+23])->ChangeArmy(1);
    }
}

void PLAYER_STEAM::DrawStateBar(const INPUT* input)
{
    m_message.Tact();
    if (!IsStateBarOn() || Map->NPlayer()!=m_army)
        return;

    DRAW_MAP DrawMap(*m_stateBar[73]);
    DrawMap.Draw();
    Map->SetSelectSpriteUnderCursor(!DrawMap.IsInside(input->screenMouseX,input->screenMouseY));

    if (Map->Menu()->IsLClick() && Map->Menu()->NVidUnderCursor()==464) {
        DrawMap.ClickOnMap(input);
        if (Map->GetScrollType()&4)
            Map->SetScrollType(0x21);
    }

    {
        STRING text=Printf("%i",GetMoney());
        (*m_stateBar[77])->Action(77,120,reinterpret_cast<int>(&text),0);
    }
    {
        STRING text=Printf("%.1f",static_cast<double>(Map->GetTimeCoeff()));
        (*m_stateBar[76])->Action(77,120,reinterpret_cast<int>(&text),0);
    }

    switch (m_statebarMode&~0x10) {
    case 1: {
        if (!static_cast<SPRITE*>(m_flagman) || !m_flagman->IsSpriteClass(0x15u)) {
            ChangeStateBar(0);
            return;
        }

        ENGINE* eng=reinterpret_cast<ENGINE*>(static_cast<SPRITE*>(m_flagman));
        int i=0;
        for (eng=eng->FirstEngine();eng && i<10;eng=eng->NextEngine(),++i) {
            if (static_cast<SPRITE*>(m_flagman)==reinterpret_cast<SPRITE*>(eng))
                (*m_stateBar[20])->ChangeXCoor((*m_stateBar[i+23])->X());

            UNIT* const unit=reinterpret_cast<UNIT*>(eng);
            ChangeIcon(i,reinterpret_cast<SPRITE*>(eng));
            (*m_stateBar[i+23])->ChangeArmy(0);
            (*m_stateBar[i+43])->ChangeDirection(
                ANGLE(static_cast<unsigned char>((251-unit->PercentHp())&0xFF)));
            (*m_stateBar[i+53])->ChangeDirection(
                ANGLE(static_cast<unsigned char>((251-unit->PercentAmmo())&0xFF)));
            (*m_stateBar[i+63])->ChangeRealDirection(static_cast<unsigned int>(unit->GetActive()));

            if (unit->GetNumber()>0) {
                (*m_stateBar[i])->ChangeRealDirection(static_cast<unsigned int>(unit->GetNumber()+49));
            } else if (*m_stateBar[i]) {
                (*m_stateBar[i])->ChangeRealDirection(
                    static_cast<unsigned int>((*m_stateBar[i])->Vid()->m_noDirections-1));
            }
        }

        for (;i<10;++i) {
            (*m_stateBar[i+23])->ChangeDirection(ANGLE(static_cast<unsigned char>(0)));
            (*m_stateBar[i+23])->ChangeArmy(2);
            (*m_stateBar[i+43])->ChangeRealDirection(
                static_cast<unsigned int>((*m_stateBar[i+43])->Vid()->m_noDirections-1));
            (*m_stateBar[i+53])->ChangeRealDirection(
                static_cast<unsigned int>((*m_stateBar[i+53])->Vid()->m_noDirections-1));
            (*m_stateBar[i+63])->ChangeRealDirection(
                static_cast<unsigned int>((*m_stateBar[i+63])->Vid()->m_noDirections-1));
        }
        return;
    }

    case 2:
    case 5: {
        if (!static_cast<SPRITE*>(m_flagman) || !m_flagman->IsSpriteClass(0x18u)) {
            ChangeStateBar(0);
            return;
        }

        DEPO* const depo=static_cast<DEPO*>(static_cast<SPRITE*>(m_flagman));
        for (int i=0;i<10;++i) {
            if (ChangeIcon(i,Map->Vid(depo->GetQueueUnit(i)))==0) {
                if (depo->IsPausedQueueUnit(i))
                    (*m_stateBar[i+63])->ChangeRealDirection(3u);
                else
                    (*m_stateBar[i+63])->ChangeRealDirection(4u);
            } else {
                (*m_stateBar[i+23])->ChangeRealDirection(2u);
            }
            (*m_stateBar[i+23])->ChangeArmy(1);

            const int progress=depo->GetBuildTimePercent(i);
            const int directions=(*m_stateBar[i+33])->Vid()->m_noDirections;
            (*m_stateBar[i+33])->ChangeRealDirection(
                static_cast<unsigned int>((progress*directions)/256));
        }

        if (m_statebarDepoUnit>=0) {
            (*m_stateBar[20])->ChangeXCoor((*m_stateBar[m_statebarDepoUnit+23])->X());
            (*m_stateBar[m_statebarDepoUnit+23])->ChangeArmy(3);
        } else {
            const int queueNo=depo->NoQueueUnit();
            (*m_stateBar[20])->ChangeXCoor((*m_stateBar[queueNo+23])->X());
            (*m_stateBar[queueNo+23])->ChangeArmy(3);
        }
        return;
    }

    case 0:
        for (int i=0;i<10;++i) {
            if (!m_trains[i])
                continue;

            if (static_cast<SPRITE*>(m_flagman)==m_trains[i])
                (*m_stateBar[20])->ChangeXCoor((*m_stateBar[i+23])->X());

            if (ChangeIcon(i,m_trains[i])==0)
                (*m_stateBar[i+23])->ChangeArmy(0);

            (*m_stateBar[i+43])->ChangeDirection(
                ANGLE(static_cast<unsigned char>((251-m_trains[i]->PercentHp())&0xFF)));
            (*m_stateBar[i+53])->ChangeDirection(
                ANGLE(static_cast<unsigned char>((251-m_trains[i]->PercentAmmo())&0xFF)));
            (*m_stateBar[i+63])->ChangeRealDirection(static_cast<unsigned int>(m_trains[i]->GetActive()));

            if (*m_stateBar[i] && (*m_stateBar[i])->Vid() && (*m_stateBar[i])->Vid()->m_noDirections!=0)
                (*m_stateBar[i])->ChangeRealDirection(static_cast<unsigned int>(i+49));
        }
        return;

    default:
        return;
    }
}

void PLAYER_STEAM::SetFlagman(SPRITE* unit)
{
    if (static_cast<SPRITE*>(m_flagman) && static_cast<SPRITE*>(m_flagman)==unit) {
        ChangeStateBar(m_flagman->IsSpriteClass(0x18u) ? 2 : 1);
        return;
    }

    m_flagman=unit;
    ENGINE::ReleasePathDots();
    if (static_cast<SPRITE*>(m_flagman))
        ChangeStateBar(m_flagman->IsSpriteClass(0x18u) ? 2 : 1);
    else
        ChangeStateBar(0);
}

void PLAYER_STEAM::AddUnitToStateBar(SPRITE* unit)
{
    if (unit->IsSpriteClass(0x18u) && !m_trains[0]) {
        m_trains[0]=reinterpret_cast<UNIT*>(unit);
        unit->AddRef();
    } else {
        for (int i=1;i<10;++i) {
            if (m_trains[i])
                continue;
            m_trains[i]=reinterpret_cast<UNIT*>(unit);
            unit->AddRef();
            reinterpret_cast<UNIT*>(unit)->SetNumber(i);
            break;
        }
    }

    if (m_statebarMode==0)
        ChangeStateBar(m_statebarMode);
}

void PLAYER_STEAM::SetTrainForKey(UNIT* engine,int n)
{
    if (n<0 || n>9 || !engine || !engine->IsSpriteClass(0x15u))
        return;

    ENGINE* engineHelper=reinterpret_cast<ENGINE*>(engine);
    if (!engineHelper->IsSelfMoving())
        engine=reinterpret_cast<UNIT*>(engineHelper->GetTrain());
    if (!engine)
        return;

    for (int i=0;i<10;++i) {
        if (m_trains[i]!=engine)
            continue;

        m_trains[i]->Release();
        if (m_trains[n]) {
            m_trains[n]->AddRef();
            m_trains[n]->SetNumber(i);
        }
        m_trains[i]=m_trains[n];
    }

    engine->AddRef();
    if (m_trains[n])
        m_trains[n]->Release();
    m_trains[n]=engine;
    engine->SetNumber(n);

    if (m_statebarMode==0)
        ChangeStateBar(m_statebarMode);
}

void PLAYER_STEAM::DebugEngineDraw()
{
    for (SPRITE* unit=Hash->FirstUnit();unit;unit=Hash->NextUnit()) {
        if (unit->IsSpriteClass(0x15u) && reinterpret_cast<ENGINE*>(unit)->IsFirst()) {
            reinterpret_cast<ENGINE*>(unit)->DebugDraw();
        } else if (unit->IsSpriteClass(2u) || unit->IsSpriteClass(4u) ||
                   unit->IsSpriteClass(0x1Au) || unit->IsSpriteClass(0x18u) ||
                   unit->IsSpriteClass(3u)) {
            reinterpret_cast<UNIT*>(unit)->LineDraw();
        }
    }
}

int PLAYER_STEAM::IsEnoughMoney(int nvid)
{
    if (static_cast<SPRITE*>(m_flagman) && m_flagman->IsSpriteClass(0x18u) &&
        m_statebarDepoUnit>=0) {
        return GetMoney() >=
            Map->Vid(nvid)->GetBuildTime()-
            Map->Vid(static_cast<DEPO*>(static_cast<SPRITE*>(m_flagman))->GetQueueUnit(m_statebarDepoUnit))->GetBuildTime();
    }
    return GetMoney()>=Map->Vid(nvid)->GetBuildTime();
}


// Retail top-level Steam player control owner.  The three large sub-routines
// it delegates to are derived separately below; this function preserves the
// original menu/minimap hit routing and selection order around them.
void PLAYER_STEAM::Control(INPUT* input)
{
    int button=-1;

    if (Mouse->IsDisable())
        return;
    if (m_type!=1 && m_type!=3)
        return;

    if (m_options&1u)
        Graph->PrintfXY(Graph->ViewXMax()-211.0f,Graph->ViewYMin()+17.0f,"CanSelectEnemy");

    MENU* menu=Map->Menu();
    if (menu->IsLClick() && menu->NVidUnderCursor()==455) {
        SPRITE* menuSprite=menu->SpriteUnderCursor();
        if (menuSprite->Direction().value!=0) {
            if (m_stateBar.No()>80 && menuSprite->Y()==(*m_stateBar[80])->Y()) {
                button=(static_cast<int>(menuSprite->X())-static_cast<int>((*m_stateBar[80])->X()))/41+1;
            } else if ((m_statebarMode&~0x10)==2 || (m_statebarMode&~0x10)==5) {
                button=(static_cast<int>(menuSprite->X())-static_cast<int>((*m_stateBar[22])->X()))/41+100;
            } else {
                button=(static_cast<int>(menuSprite->X())-static_cast<int>((*m_stateBar[22])->X()))/41;
            }
        }
    }

    if ((m_statebarMode&~0x10)==2 && m_flagman->IsSpriteClass(0x18u)) {
        DEPO* depo=reinterpret_cast<DEPO*>(static_cast<SPRITE*>(m_flagman));
        switch ((input->key>>8)-0x70u) {
        case 0:  button=depo->SlotForUnit(5);  break;
        case 1:  button=depo->SlotForUnit(10); break;
        case 2:  button=depo->SlotForUnit(20); break;
        case 3:  button=depo->SlotForUnit(25); break;
        case 4:  button=depo->SlotForUnit(80); break;
        case 5:  button=depo->SlotForUnit(85); break;
        case 6:  button=depo->SlotForUnit(45); break;
        case 7:  button=depo->SlotForUnit(30); break;
        case 8:  button=depo->SlotForUnit(35); break;
        case 9:  button=depo->SlotForUnit(82); break;
        case 10: button=depo->SlotForUnit(97); break;
        case 11: button=depo->SlotForUnit(90); break;
        default: break;
        }
        if (input->key==0x5Bu)
            button=depo->SlotForUnit(75);
        else if (input->key==0x5Du)
            button=depo->SlotForUnit(62);
    }

    if (input->key==0x1Bu || (menu->IsLClick() && menu->NVidUnderCursor()==499))
        button=0;

    Control_KeyProcessing(input);
    Control_MenuProcessing(input,&button);

    if (IsStateBarOn() && menu->SpriteUnderCursor() && menu->NVidUnderCursor()!=464) {
        m_underCursor=static_cast<SPRITE*>(0);
        Mouse->ChangeAnimation(0);
    } else {
        const float savedMouseX=input->mouseX;
        const float savedMouseY=input->mouseY;

        if (IsStateBarOn() && menu->NVidUnderCursor()==464) {
            DRAW_MAP drawMap(*m_stateBar[73]);
            if (menu->IsRClick() && g_inputSecondPrimary==2) {
                input->stateBits|=0x8000u;
                input->stateBits|=0x4u;
            }
            if (menu->IsLClick() && g_inputSecondPrimary==1) {
                input->stateBits|=0x8000u;
                input->stateBits|=0x1u;
            }
            input->mouseX=drawMap.FromScreenX(input->screenMouseX);
            input->mouseY=drawMap.FromScreenY(input->screenMouseY);
        }

        m_underCursor=Map->GetSpriteScr(0x400000,input->mouseX,input->mouseY);
        if (!static_cast<SPRITE*>(m_underCursor))
            m_underCursor=Map->GetSpriteScr(0xA08000,input->mouseX,input->mouseY);

        if (static_cast<SPRITE*>(m_underCursor) && m_underCursor->Vid()->PropInvisibleForEnemy()
            && m_underCursor->Army()==1)
            m_underCursor=static_cast<SPRITE*>(0);

        Control_RClickProcessing(input);

        if ((input->stateBits&0x4000u) || ((input->stateBits&0x8000u) && !Flagman())) {
            if (static_cast<SPRITE*>(m_underCursor) && static_cast<SPRITE*>(m_underCursor)!=Flagman()) {
                if (!((m_underCursor->Army()==1 && m_underCursor->Vid()->m_idx!=409)
                      || (m_underCursor->Army()==2 && m_underCursor->IsSpriteClass(0x15u))))
                    m_underCursor->Action(9,0,0,0);
            }

            if (static_cast<SPRITE*>(m_underCursor) && CanSelect(static_cast<SPRITE*>(m_underCursor)))
                SetFlagman(static_cast<SPRITE*>(m_underCursor));
            else
                SetFlagman(0);
        }

        input->mouseX=savedMouseX;
        input->mouseY=savedMouseY;
    }

    if (static_cast<SPRITE*>(m_underCursor)
        && (m_underCursor->IsSpriteType(2u) || m_underCursor->Vid()->m_idx==162))
        m_underCursor=static_cast<SPRITE*>(0);

    DrawStateBar(input);
}

void PLAYER_STEAM::Control_KeyProcessing(INPUT* input)
{
    if (Map->GetScrollType()&4) {
        if (!Flagman() || input->screenMouseX<5.0f ||
            input->screenMouseX>Graph->ViewXMax()-5.0f ||
            input->screenMouseY<5.0f ||
            input->screenMouseY>Graph->ViewYMax()-5.0f) {
            Map->SetScrollType(0x21);
        }
    }

    if (Const->DebugMode) {
        switch (input->key) {
        case 0x12:
            if (Flagman())
                reinterpret_cast<ENGINE*>(Flagman())->ReverseTrain();
            break;
        case 0x45:
            m_options=(m_options&~1u)|((m_options&1u)^1u);
            break;
        case 0x47:
            if (Flagman()) {
                TERRAIN* const terrain=static_cast<TERRAIN*>(Flagman());
                terrain->SetGodeMode(!terrain->GetGodeMode());
            }
            break;
        default:
            break;
        }
    }

    const unsigned int highKey=input->key>>8;
    if (highKey>=0x31u && highKey<=0x39u && input->ctrl)
        SetTrainForKey(reinterpret_cast<UNIT*>(static_cast<SPRITE*>(m_flagman)),static_cast<int>(highKey-0x30u));

    switch (input->key) {
    case 0x4C:
    case 0x6C:
        Map->SetScrollType((Map->GetScrollType()&4) ? 2 : 4);
        break;

    case 0x52:
    case 0x72:
        if (static_cast<SPRITE*>(m_flagman) && m_flagman->IsSpriteClass(0x15u))
            reinterpret_cast<ENGINE*>(static_cast<SPRITE*>(m_flagman))->MoveToRepair();
        break;

    case 0x20:
        if (static_cast<SPRITE*>(m_flagman) && m_flagman->IsSpriteClass(0x15u))
            reinterpret_cast<ENGINE*>(static_cast<SPRITE*>(m_flagman))->SetCommandToTrain(0,0,0,0);
        break;

    case 0x41:
    case 0x61:
        if (static_cast<SPRITE*>(m_flagman))
            reinterpret_cast<UNIT*>(static_cast<SPRITE*>(m_flagman))->InverseActive();
        break;

    case 0x43:
    case 0x63:
        if (static_cast<SPRITE*>(m_flagman))
            Map->SetShiftCoor(m_flagman->X(),m_flagman->Y()-m_flagman->Z(),2);
        break;

    case 0x01:
        if (static_cast<SPRITE*>(m_flagman) && m_flagman->IsSpriteClass(0x15u))
            reinterpret_cast<ENGINE*>(static_cast<SPRITE*>(m_flagman))->InverseTrainActive();
        break;

    case 0x4D:
    case 0x6D:
        if (static_cast<SPRITE*>(m_flagman) && m_flagman->IsSpriteClass(0x15u) &&
            reinterpret_cast<ENGINE*>(static_cast<SPRITE*>(m_flagman))->GetRepair()) {
            ENGINE* const repair=reinterpret_cast<ENGINE*>(static_cast<SPRITE*>(m_flagman))->GetRepair();
            R_DOT* const repairDot=*reinterpret_cast<R_DOT**>(reinterpret_cast<unsigned char*>(repair)+0xB8);
            repair->SetCommandToTrain(24,0,repairDot,0);
        } else {
            Sound->PlaySFX(0x72,0,0);
        }
        break;

    default:
        break;
    }

    if (input->key>=0x30u && input->key<=0x39u) {
        UNIT* const train=m_trains[input->key-0x30u];
        if (train) {
            if (static_cast<SPRITE*>(m_flagman)==train)
                Map->SetShiftCoor(m_flagman->X(),m_flagman->Y()-m_flagman->Z(),2);
            SetFlagman(train);
            Sound->PlaySFX(0x6D,0,0);
        }
    }

    if ((input->key>>8)==0x24u && m_trains[0]) {
        if (static_cast<SPRITE*>(m_flagman)==m_trains[0])
            Map->SetShiftCoor(m_flagman->X(),m_flagman->Y()-m_flagman->Z(),2);
        SetFlagman(m_trains[0]);
    }
}

void PLAYER_STEAM::Control_MenuProcessing(INPUT* input,int* button)
{
    const int mode=m_statebarMode&~0x10;

    switch (mode) {
    case 0:
        if (*button>0 && m_trains[*button-1])
            SetFlagman(m_trains[*button-1]);
        return;

    case 1:
        if (*button==0) {
            ChangeStateBar(0);
        } else if (*button>0 && static_cast<SPRITE*>(m_flagman)) {
            ENGINE* const newSelected=
                reinterpret_cast<ENGINE*>(static_cast<SPRITE*>(m_flagman))->GetChainEngine(*button-1);
            Sound->PlaySFX(0x6D,0,0);
            if (static_cast<SPRITE*>(m_flagman)==reinterpret_cast<SPRITE*>(newSelected))
                Map->SetShiftCoor(m_flagman->X(),m_flagman->Y()-m_flagman->Z(),2);
            SetFlagman(reinterpret_cast<SPRITE*>(newSelected));
        }
        return;

    case 5: {
        DEPO* const depo=reinterpret_cast<DEPO*>(static_cast<SPRITE*>(m_flagman));
        if (*button==100 || *button==0) {
            m_statebarDepoUnit=-1;
            ChangeStateBar(2);
            return;
        }

        if (*button>100) {
            const int queueUnit=*button-101;
            if (depo->GetQueueUnit(queueUnit)) {
                m_statebarDepoUnit=queueUnit;
                ChangeStateBar(5);
            } else {
                m_statebarDepoUnit=-1;
                ChangeStateBar(2);
            }
            return;
        }

        if (*button==1 || input->key==0x6E || input->key==0x4E || (input->key>>8)==0x2Eu) {
            Sound->PlaySFX(0x6D,0,0);
            depo->DeleteQueueUnit(m_statebarDepoUnit);
            if (depo->GetQueueUnit(m_statebarDepoUnit)==0)
                --m_statebarDepoUnit;
            if (m_statebarDepoUnit<0)
                ChangeStateBar(2);
            return;
        }

        if (*button==2 || input->key==0x6D || input->key==0x4D) {
            Sound->PlaySFX(0x6D,0,0);
            depo->PauseQueueUnit(m_statebarDepoUnit);
            ChangeStateBar(5);
            return;
        }

        if (*button==3 || input->key==0x3C || input->key==0x2C) {
            Sound->PlaySFX(0x6D,0,0);
            depo->LeftShiftQueueUnit(m_statebarDepoUnit);
            if (m_statebarDepoUnit!=0)
                --m_statebarDepoUnit;
            return;
        }

        if (*button==4 || input->key==0x3E || input->key==0x2E) {
            Sound->PlaySFX(0x6D,0,0);
            depo->RightShiftQueueUnit(m_statebarDepoUnit);
            if (m_statebarDepoUnit<depo->NoQueueUnit()-1)
                ++m_statebarDepoUnit;
            return;
        }

        if (*button!=0 && (*button==5 || input->key==0x2F || input->key==0x3F))
            ChangeStateBar(2);
        return;
    }

    case 2: {
        DEPO* const depo=reinterpret_cast<DEPO*>(static_cast<SPRITE*>(m_flagman));
        if (*button==100 || *button==0) {
            ChangeStateBar(0);
            return;
        }

        if (*button>100) {
            const int queueUnit=*button-101;
            if (depo->GetQueueUnit(queueUnit)) {
                m_statebarDepoUnit=queueUnit;
                ChangeStateBar(5);
            } else {
                m_statebarDepoUnit=-1;
                ChangeStateBar(2);
            }
            return;
        }

        if (*button<=0)
            return;

        if ((*m_stateBar[*button+79])->Army()==2)
            return;

        Sound->PlaySFX(0x18,0,0);
        const int newVid=depo->CanBuildUnit(*button-1);
        if (m_statebarDepoUnit>=0) {
            depo->ReplaceQueueUnit(m_statebarDepoUnit,newVid);
            ChangeStateBar(5);
        } else {
            static_cast<SPRITE*>(m_flagman)->Action(0x23,newVid,0,0);
            ChangeStateBar(2);
        }
        return;
    }

    case 3:
    case 4:
    default:
        return;
    }
}

void PLAYER_STEAM::Control_RClickProcessing(INPUT* input)
{
    if (Map->NPlayer()!=m_army)
        return;

    const int mouse_small=(IsStateBarOn() && Map->Menu()->NVidUnderCursor()==0x1D0) ? 0x12 : 0;
    SPRITE* const flagmanSprite=static_cast<SPRITE*>(m_flagman);
    if (!flagmanSprite || !flagmanSprite->IsSpriteClass(0x15u)) {
        m_flagmanPathDots.DeleteAll();
        Mouse->ChangeAnimation(mouse_small);
        return;
    }

    ENGINE* const flagman=static_cast<ENGINE*>(flagmanSprite);
    if (input->second)
        flagman->ResetActionStack();

    TRAIN_INFO info(flagman);
    const int have_ammo=!flagman->IsCommandToAllTrain()
        ? (flagman->Ammo()!=0 && flagman->HaveFightLink()!=0)
        : info.HaveAmmo();
    const int can_move=info.CanMove();
    SPRITE* const target=static_cast<SPRITE*>(m_underCursor);

    if (input->shift) {
        if (IsTargetEngine() && !flagman->InTrain(target) && target->Army()!=0) {
            if (can_move || flagman->Vid()->m_idx==0x23) {
                if (input->second)
                    flagman->Action(0x97,reinterpret_cast<int>(target),0,0);
                else
                    Mouse->ChangeAnimation(mouse_small+3);
            } else {
                Mouse->ChangeAnimation(mouse_small+12);
            }
        } else if (flagman->Vid()->m_idx==0x55) {
            if (can_move) {
                if (flagman->Ammo()) {
                    if (input->second)
                        flagman->SetCommandToTrain(0x18,static_cast<int>(input->mouseX),static_cast<int>(input->mouseY));
                    else
                        Mouse->ChangeAnimation(mouse_small+16);
                } else {
                    Mouse->ChangeAnimation(mouse_small+1);
                }
            } else {
                Mouse->ChangeAnimation(mouse_small+12);
            }
        } else if (have_ammo) {
            if (input->second) {
                flagman->Action(0x25,static_cast<int>(input->mouseX),static_cast<int>(input->mouseY),0);
            } else {
                const float target_z=Map->GetGroundZ(input->mouseX,input->mouseY);
                const float range=flagman->IsCommandToAllTrain() ? info.maxBattleRange : flagman->BattleRange();
                if (flagman->NearDistanceTo(input->mouseX,input->mouseY+target_z,target_z)<range)
                    Mouse->ChangeAnimation(mouse_small+5);
                else if (can_move)
                    Mouse->ChangeAnimation(mouse_small+6);
                else
                    Mouse->ChangeAnimation(mouse_small+12);
            }
        } else {
            Mouse->ChangeAnimation(mouse_small+1);
        }
    } else if (IsTargetEngine() && !flagman->IsSingle() && flagman->InTrain(target)) {
        if (input->second) {
            if (flagman==target) {
                if (!flagman->PrevEngine()) {
                    flagman->BreakTrain(flagman->NextEngine());
                } else if (flagman->NextEngine()) {
                    if (flagman->PrevEngine()->NearDistanceTo(input->mouseX,input->mouseY) <
                        flagman->NextEngine()->NearDistanceTo(input->mouseX,input->mouseY))
                        flagman->BreakTrain(flagman->PrevEngine());
                    else
                        flagman->BreakTrain(flagman->NextEngine());
                } else {
                    flagman->BreakTrain(flagman->PrevEngine());
                }
            } else {
                flagman->BreakTrain(static_cast<ENGINE*>(target));
            }
        } else {
            Mouse->ChangeAnimation(mouse_small+11);
        }
    } else if (IsTargetEngine() && !flagman->InTrain(target) &&
               (static_cast<ENGINE*>(target)->HaveArmy(flagman->Army()) || input->ctrl ||
                (flagman->IsPowerEngine() && flagman->IsSingle())) &&
               static_cast<ENGINE*>(target)->CanBeLinkedByEnemy()) {
        if (can_move) {
            if (input->second)
                flagman->Action(0x96,reinterpret_cast<int>(target),0,0);
            else
                Mouse->ChangeAnimation(mouse_small+10);
        } else {
            Mouse->ChangeAnimation(mouse_small+12);
        }
    } else if (target && CanCapture(target)) {
        if (can_move) {
            if (input->second)
                flagman->Move(input->mouseX,input->mouseY,0.0f,0,1);
            else
                Mouse->ChangeAnimation(mouse_small+15);
        } else {
            Mouse->ChangeAnimation(mouse_small+12);
        }
    } else if (target && target->Vid()->m_idx==0x68 && target->Army()!=1 &&
               (info.NeedAmmo() || info.IsDamaged())) {
        if (can_move) {
            if (input->second)
                flagman->Move(input->mouseX,input->mouseY,0.0f,0,1);
            else
                Mouse->ChangeAnimation(mouse_small+4);
        } else {
            Mouse->ChangeAnimation(mouse_small+12);
        }
    } else if (flagman->Vid()->m_idx==0x55 && target && target->Vid()->m_idx==0x56) {
        if (can_move) {
            if (input->second)
                flagman->Move(target->X(),target->Y(),target->Z(),0,0);
            else
                Mouse->ChangeAnimation(mouse_small+17);
        } else {
            Mouse->ChangeAnimation(mouse_small+12);
        }
    } else if (input->ctrl) {
        if (can_move) {
            if (input->second)
                flagman->SetCommandToTrain(0x19,static_cast<int>(input->mouseX),static_cast<int>(input->mouseY));
            else
                Mouse->ChangeAnimation(mouse_small+13);
        } else {
            Mouse->ChangeAnimation(mouse_small+12);
        }
    } else if (target &&
               (target->Army()==1 || (target->IsSpriteClass(0x15u) && target->Army()==2)) &&
               target->Vid()->m_unknown0C>2) {
        if (have_ammo) {
            if (input->second) {
                flagman->Action(0x20,reinterpret_cast<int>(target),0,0);
            } else {
                const float range=flagman->IsCommandToAllTrain() ? info.maxBattleRange : flagman->BattleRange();
                if (flagman->NearDistanceTo(target->X(),target->Y(),target->Z())<range)
                    Mouse->ChangeAnimation(mouse_small+5);
                else if (can_move)
                    Mouse->ChangeAnimation(mouse_small+6);
                else
                    Mouse->ChangeAnimation(mouse_small+12);
            }
        } else if (!info.HaveAmmo() && can_move && target->IsSpriteClass(0x15u) && target->Vid()->m_idx!=0x15F) {
            if (input->second)
                flagman->Action(0x97,reinterpret_cast<int>(target),0,0);
            else
                Mouse->ChangeAnimation(mouse_small+3);
        } else {
            Mouse->ChangeAnimation(mouse_small+1);
        }
    } else {
        if (can_move) {
            if (input->second)
                flagman->Move(input->mouseX,input->mouseY,0.0f,0,1);
            else
                Mouse->ChangeAnimation(mouse_small+2);
        } else {
            Mouse->ChangeAnimation(mouse_small+12);
        }
    }

    if (input->second) {
        if (Mouse->Animation()==mouse_small+1)
            flagman->PlaySFX(0x72);
        else if (Mouse->Animation()==mouse_small+12)
            flagman->PlaySFX(0x6B);
        else
            flagman->PlaySFX(0x99);
    }
}

int PLAYER_STEAM::CanCreateUnit(VID* nvid)
{
    if (!nvid || nvid->m_idx<=0)
        return 0;
    if (m_army==0 && !IsEnoughMoney(nvid->m_idx))
        return 0;

    if (nvid->m_limit398[m_army]<0 && nvid->m_limit394<0)
        return 1;

    int sameArmyQueued=0;
    int allQueued=0;
    for (SPRITE* unit=Hash->FirstUnit();unit;unit=Hash->NextUnit()) {
        if (!unit->IsSpriteClass(0x18u) || unit->Army()!=0)
            continue;
        DEPO* const depo=static_cast<DEPO*>(unit);
        for (int i=0;i<10;++i) {
            const int queueNvid=depo->GetQueueUnit(i);
            if (queueNvid==nvid->m_idx) {
                ++sameArmyQueued;
                ++allQueued;
            }
        }
    }

    if (nvid->m_limit398[m_army]>=0 &&
        sameArmyQueued+nvid->NoSprites(m_army)>=nvid->m_limit394)
        return 0;
    if (nvid->m_limit394>=0 && allQueued+nvid->NoSprites()>=nvid->m_limit394)
        return 0;
    return 1;
}

int PLAYER_STEAM::CanCapture(const SPRITE* sprite)
{
    if (!sprite->IsSpriteClass(0x18u) && !sprite->IsSpriteClass(3u))
        return 0;
    if (const_cast<SPRITE*>(sprite)->Army()==2)
        return 1;

    const int nvid=sprite->Vid()->m_idx;
    if (nvid!=0xA2 && nvid!=0xA5)
        return 0;

    const float captureRadius=150.0f;
    for (SPRITE* unit=Hash->FirstInBox(sprite->X()-captureRadius,
                                          sprite->Y()-captureRadius,
                                          sprite->X()+captureRadius,
                                          sprite->Y()+captureRadius);
         unit;
         unit=Hash->NextInBox()) {
        const int nearbyNvid=unit->Vid()->m_idx;
        if ((nearbyNvid==0x6E || nearbyNvid==0x73) &&
            const_cast<SPRITE*>(sprite)->NearDistanceTo(unit)<captureRadius)
            return 0;
    }
    return 1;
}

int PLAYER_STEAM::CanSelect(const SPRITE* sprite)
{
    if ((const_cast<SPRITE*>(sprite)->Army()==0 || (m_options&1u)!=0u) &&
        (sprite->IsSpriteClass(0x15u) || sprite->Vid()->m_idx==0x66))
        return 1;
    return 0;
}

int PLAYER_STEAM::IsTargetEngine()
{
    return static_cast<SPRITE*>(m_underCursor) && m_underCursor->IsSpriteClass(0x15u);
}

void PLAYER_STEAM::SetCleverEnemyAttack(int newAttack)
{
    m_options=(m_options&~2u)|(newAttack!=0 ? 2u : 0u);
}
