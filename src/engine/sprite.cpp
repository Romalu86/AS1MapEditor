#include "mapedit/runtime.hpp"


namespace {
int SpriteVidInt(const VID* vid,unsigned int offset)
{
    const unsigned char* const raw=reinterpret_cast<const unsigned char*>(vid);
    return *reinterpret_cast<const int*>(raw+offset);
}
}

// Base lifecycle owner implemented directly from reference behavior/debug metadata offsets.
SPRITE::SPRITE(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
{
    if (vid->PropNoise()) {
        x+=static_cast<float>(8-Random(16));
        y+=static_cast<float>(8-Random(16));
    }

    m_vid=vid;
    m_x=x;
    m_y=y;

    if (m_vid->PropZeroZ()) {
        m_z=Map->GetGroundZ(m_vid,m_x,m_y);
        if (parent)
            m_z+=z-parent->Z();
    } else {
        m_z=z;
    }

    int army;
    if (parent) {
        army=parent->Army();
    } else if (SpriteVidInt(m_vid,0x40u)==0 && m_vid->m_linkVid) {
        army=m_vid->m_linkVid->m_weapon->m_army;
    } else {
        army=m_vid->m_weapon->m_army;
    }

    m_flag=(m_flag & ~0x00001800u) |
           ((static_cast<unsigned int>(army)&3u)<<11);

    const int phase=m_vid->PropOnePhase() ? 0 : Random(m_vid->m_phaseRandomInterval);
    m_tactTime=CurrentTime-static_cast<unsigned int>(phase);
    m_createTime=CurrentTime;
    m_noRef=0;
    m_parent=0;
    m_child=0;
    m_goal=0;

    // Exact sequence of debug metadata bitfield initializers represented as one mask:
    // preserve army bits 11..12 and untouched high/reserved bits, clear every
    // other base SPRITE flag initialized to zero by the retail constructor.
    m_flag&=~0x0001E7FFu;

    m_dir=0;
    m_unknown50=0;
    m_hp=MaxHp();
    m_speed=0.0f;
    m_unknown24=0.0f;
    m_unknown04=0;

    m_exData=m_vid->m_exSpriteData ? new EX_SPRITE_DATA(this) : 0;

    if (m_vid->m_noAnimCadr[0]==0 &&
        m_vid->m_noAnimCadr[2]==0 &&
        m_vid->m_noAnimCadr[15]!=0) {
        m_ani=15;
    } else if (m_vid->m_noAnimCadr[14]!=0 && !Map->OptLoad()) {
        m_ani=14;
    } else {
        m_ani=0;
    }

    m_begCadr=m_vid->m_aniFrameStart[m_ani];
    m_noCadr=m_begCadr;
    m_endCadr=m_vid->m_aniFrameLimit[m_ani]-1;

    ChangeDirection(direction);

    if (m_ani==0 && m_endCadr-m_begCadr>0 && !m_vid->PropOnePhase())
        m_noCadr+=Random(m_endCadr-m_begCadr);

    if (m_vid!=EmptyVid)
        ++m_noRef;
    if (m_vid!=EmptyVid)
        Insert();

    CreateLink();
    Hash->Insert(this);
    m_vid->IncreaseNoSprites(Army());

    if (Animation()!=14 && !Map->OptLoad())
        CreateChildAndPlaySFX(14);

    if (m_vid->m_nLinkDots)
        m_vid->SetGridZ(this);
}

SPRITE::~SPRITE()
{
    const int destroyFunction=SpriteVidInt(m_vid,0x44Cu);
    if (destroyFunction>=0)
        Map->ScriptRun(destroyFunction,this,0,0);

    if (m_vid->m_nLinkDots)
        m_vid->ResetGridZ(this);

    if (m_vid==EmptyVid && m_noRef!=0)
        Error(4,"noRef for SPRITE with EmptyVid",static_cast<unsigned long>(m_noRef));

    if (m_noRef>1)
        Hash->Delete(this);
    if (m_noRef>1)
        Map->DeletePointerToSprite(this);

    m_vid->DecreaseNoSprites(Army());
    SetGoal(0);
    m_ptrSprite=0;

    if (m_parent) {
        while (HaveLink()) {
            SPRITE* const linked=m_child;
            if (linked)
                linked->ScalarDeletingDestructor(1u);
        }

        m_parent->m_child=m_child;
        if (m_child)
            m_child->m_parent=m_parent;
    } else {
        while (m_child) {
            SPRITE* const child=m_child;
            child->ScalarDeletingDestructor(1u);
        }
    }

    if (m_vid!=EmptyVid) {
        Remove();
        --m_noRef;
    }

    if (m_noRef!=0)
        Error(10,"Reference count non zero after delete",static_cast<unsigned long>(m_noRef));

    delete m_exData;
    m_exData=0;
}

void* SPRITE::ScalarDeletingDestructor(unsigned int flags)
{
    SPRITE* const self=this;
    self->SPRITE::~SPRITE();
    if (flags&1u)
        ::operator delete(self);
    return self;
}

void SPRITE::DeletePointerToSprite(SPRITE* sprite)
{
    if (!sprite)
        return;

    if (m_child)
        m_child->DeletePointerToSprite(sprite);

    if (m_goal==sprite) {
        if (m_ani==8)
            SetCommand(4,sprite->m_x,sprite->m_y,sprite->m_z);
        else
            SetCommand(0,static_cast<SPRITE*>(0));
    }

    for (int i=0;i<m_actions.No();++i) {
        ACT* const action=m_actions[i];
        if (reinterpret_cast<SPRITE*>(action->var1)==sprite &&
            ActionNeedSpriteInVar1(action->act)) {
            action->act=255;
            action->var1=0;
        }
    }

    if (static_cast<SPRITE*>(m_ptrSprite)==sprite)
        SetBestTarget(0);
}

// implemented from the retail body with debug metadata line/local records.  Keep the
// original timing order: movement/grid maintenance precedes animation-frame
// gating, while birth-smoke children are scheduled independently of frames.
void SPRITE::Tact()
{
    // Source lines 345..374: optional lifetime/table coefficient track.
    if ((m_vid->m_propertyBits&0x0Fu)!=0u) {
        const unsigned char* const weaponRaw=
            reinterpret_cast<const unsigned char*>(m_vid->m_weapon);
        const unsigned long weaponDuration=
            *reinterpret_cast<const unsigned long*>(weaponRaw+0x40u);
        const float* const table=
            reinterpret_cast<const float*>(weaponRaw+0x44u);

        unsigned long tableDuration=0;
        const unsigned long tableTime=CurrentTime-m_exData->tableStartTime;
        if (weaponDuration==999999u) {
            if (IsDying() && m_endCadr!=m_begCadr) {
                tableDuration=static_cast<unsigned long>(m_endCadr-m_begCadr)*FrameSpeed();
            } else if (m_exData && m_exData->deathTimer!=999999u) {
                tableDuration=tableTime+m_exData->deathTimer;
            } else {
                tableDuration=static_cast<unsigned long>(IsCommand(1));
            }
        } else {
            tableDuration=weaponDuration;
        }

        if (tableDuration!=0u) {
            const int maxColumn=7;
            int column=static_cast<int>(m_exData->tableCoeff)+1;
            while (column<maxColumn) {
                if (static_cast<float>(tableTime) <
                    static_cast<float>(tableDuration)*table[column])
                    break;
                ++column;
            }
            --column;
            m_exData->tableCoeff=
                static_cast<float>(column)+
                (static_cast<float>(tableTime)-
                 static_cast<float>(tableDuration)*table[column])/
                ((table[column+1]-table[column])*
                 static_cast<float>(tableDuration));
        }

        if (weaponDuration!=999999u &&
            CurrentTime-m_exData->tableStartTime>weaponDuration) {
            m_exData->tableStartTime=CurrentTime;
        }
    }

    // Source lines 378..383: linked sprites do not run their own MoveTact.
    if (!m_parent) {
        const unsigned long backupTime=m_tactTime;
        m_tactTime=PrevCurrentTime;
        MoveTact();
        m_tactTime=backupTime;
    }

    if (m_vid->m_nLinkDots)
        m_vid->SetGridZ(this);

    // Source lines 390..417: child creation that is not tied to frame advance.
    VID* child=m_vid->m_aniChildVid[m_ani];
    if (child) {
        if (IsSpriteClass(23) &&
            m_vid->m_aniSpawnX[m_ani]==0.0f &&
            m_vid->m_aniSpawnY[m_ani]==0.0f) {
            REGION* const region=static_cast<REGION*>(this);
            const int noChild=static_cast<int>(
                region->sizeX*region->sizeY/
                child->m_footprintWidth/child->m_footprintHeight);
            if (noChild!=0 && CurrentTime!=PrevCurrentTime) {
                const unsigned long dt=CurrentTime-PrevCurrentTime;
                const unsigned long randomRange=1000u/dt/static_cast<unsigned long>(noChild);
                if (Random(static_cast<int>(randomRange))==0)
                    CreateChild();
            }
        } else if (child->PropBirthAsSmoke()) {
            const unsigned long period=m_exData->birthSmokeTimer;
            if (CurrentTime-(CurrentTime%period)>PrevCurrentTime) {
                float tx=Direction().Sin()*m_speed;
                float ty=Direction().Cos()*m_speed+m_unknown24-child->m_maxZSpeed;

                if (child->PropWind()) {
                    const float windSpeed=Graph->WindSpeed();
                    ANGLE windDirection=Graph->WindDirection();
                    tx-=windDirection.Sin()*windSpeed;
                    ty-=windDirection.Cos()*windSpeed;
                }

                if (tx!=0.0f) {
                    const float absTx=tx<0.0f ? -tx : tx;
                    tx=child->m_footprintWidth/absTx;
                } else {
                    tx=30000.0f;
                }
                if (ty!=0.0f) {
                    const float absTy=ty<0.0f ? -ty : ty;
                    ty=child->m_footprintHeight/absTy;
                } else {
                    ty=30000.0f;
                }

                m_exData->birthSmokeTimer=
                    static_cast<unsigned long>(static_cast<int>(tx<ty ? tx : ty));
                if (Animation()==8 && m_vid->m_weapon->PropInTurn())
                    m_exData->birthSmokeTimer>>=1;
                if (m_exData->birthSmokeTimer>30000u)
                    m_exData->birthSmokeTimer=30000u;
                if (m_exData->birthSmokeTimer==0u)
                    m_exData->birthSmokeTimer=1u;
                CreateChild();
            }
        }
    }

    // Source lines 421..423: advance only once per crossed frame boundary,
    // and never on the same CurrentTime in which the sprite was created.
    if (m_createTime==CurrentTime)
        return;
    {
        const unsigned long frameSpeed=FrameSpeed();
        if (CurrentTime-(CurrentTime%frameSpeed)<=PrevCurrentTime)
            return;
    }

    // Source lines 429..432: commands 1..15 require a goal.
    if (Command()!=0 && Command()<16 && !Goal()) {
        Error(10,"command need goal, but goal==NULL",static_cast<unsigned long>(Command()));
        SetCommand(0,static_cast<SPRITE*>(0));
    }

    // Source lines 436..444: delayed command timeout stored in +0x50.
    if (m_unknown50!=0u) {
        const unsigned long elapsed=CurrentTime-m_tactTime;
        if (elapsed>=m_unknown50) {
            m_unknown50=0u;
            if (IsCommand(18))
                SetCommand(0,static_cast<SPRITE*>(0));
        } else {
            m_unknown50-=elapsed;
        }
    }

    // Source lines 446..452: animation script can change the animation; when
    // it does, retail immediately re-enters the script gate for the new one.
    for (;;) {
        const int script=SpriteVidInt(m_vid,0x408u+4u*static_cast<unsigned int>(m_ani));
        if (script<0 || (m_noCadr!=m_begCadr && !m_vid->PropTrack()))
            break;
        const int previousAnimation=m_ani;
        if (Map->ScriptRun(script,this,0,0))
            return;
        if (previousAnimation==m_ani)
            break;
    }

    // Source lines 455..458: one-shot SFX latch, except looped SFX are
    // deliberately replayed at each eligible animation frame.
    const int sfx=m_vid->m_aniSfx[m_ani];
    if (sfx!=0) {
        if ((m_flag&0x00000200u)==0u || Sound->IsLooped(sfx)) {
            m_flag|=0x00000200u;
            PlaySFX(sfx);
        }
    }

    // Source lines 461..472: non-smoke child creation tied to animation frame.
    child=m_vid->m_aniChildVid[m_ani];
    if (child && !child->PropBirthAsSmoke()) {
        const int childFrame=m_vid->PropChildInEnd() ? m_endCadr : m_begCadr;
        if (m_ani==8 && m_vid->m_weapon->PropInTurn()) {
            const int middleFrame=(m_endCadr+m_begCadr+1)/2;
            if (m_noCadr==middleFrame || m_noCadr==childFrame)
                CreateChild();
        } else if (m_vid->PropTrack() || m_noCadr==childFrame) {
            CreateChild();
        }
    }

    ++m_noCadr;

    // Source lines 481..483: dying sprites finish animation 15/16 and then
    // invoke the virtual scalar-deleting-destructor slot exactly as retail.
    if (IsDying()) {
        if (m_noCadr>m_endCadr || m_vid->m_noAnimCadr[m_ani]==0) {
            m_noCadr=m_endCadr;
            Action(15,0,0,0);
            ScalarDeletingDestructor(1u);
            return;
        }
    }

    // Commands 17/18 skip the ordinary queued-action completion branch.
    if (!IsCommand(17) && !IsCommand(18)) {
        if (m_noCadr>m_endCadr || m_vid->PropTrack()) {
            if (m_actions.No()!=0 && IsCommand(0)) {
                if (((!IsDying() && m_ani>=7 && m_ani!=10) ||
                     (m_ani==2 && m_speed==0.0f)) &&
                    m_actions.Last()->act>=17) {
                    ChangeAnimation(0);
                }

                if (m_actions.Last()->act!=73) {
                    ACT act=*m_actions.Pop();
                    if (act.act>=17)
                        Action(act.act,act.var1,act.var2,act.var3);
                    else
                        ChangeAnimation(act.act);
                }
            } else {
                Action(130,0,0,0);
            }
        }
    }

    // Source lines 510..517: normalize locomotion animation after a completed
    // frame range unless the sprite is dying or animation 10 owns the state.
    if (!IsDying() && m_ani!=10 && m_noCadr>m_endCadr) {
        if (Speed()!=0.0f) {
            if (Animation()!=2)
                ChangeAnimation(2);
        } else if (Animation()==2 || Animation()>=7) {
            ChangeAnimation(0);
        }
    }

    // Source lines 519..525: finite death timer counts down by this tact span.
    if (m_exData && m_exData->deathTimer!=999999u && !IsDying()) {
        const unsigned long elapsed=CurrentTime-m_tactTime;
        if (elapsed<m_exData->deathTimer)
            m_exData->deathTimer-=elapsed;
        else
            ChangeAnimation(15);
    }

    m_tactTime=CurrentTime;
    if (m_noCadr>m_endCadr)
        m_noCadr=m_begCadr;
}


int SPRITE::GetItemNumber(int number)
{
    if (!m_exData || number<0 || number>=m_exData->items.m_no)
        return 0;
    return m_exData->items.m_data[number];
}

float SPRITE::BattleRange()
{
    if (HaveFightLink() && m_vid->m_weapon && m_vid->m_weapon->m_battleRange==0.0f &&
        m_child && m_child->m_vid->m_weapon)
        return m_child->m_vid->m_weapon->m_battleRange;
    return m_vid->m_weapon ? m_vid->m_weapon->m_battleRange : 0.0f;
}

int SPRITE::PercentHp()
{
    SPRITE* target=HaveFightLink() ? m_child : this;
    if (!target)
        target=this;
    const int maxHp=target->MaxHp();
    return maxHp ? target->Hp()*255/maxHp : 0;
}

unsigned long SPRITE::GetTimer() { return m_unknown50; }

int SPRITE::DeltaTime()
{
    const unsigned long dt=CurrentTime-PrevCurrentTime;
    const unsigned long frame=FrameSpeed();
    return static_cast<int>(dt<frame ? dt : frame);
}

void SPRITE::InvisibleOn()
{
    m_flag|=0x10000u;
    if (m_child)
        m_child->InvisibleOn();
}
void SPRITE::InvisibleOff()
{
    m_flag&=~0x10000u;
    if (m_child)
        m_child->InvisibleOff();
}

void SPRITE::SetTimer(unsigned long timer) { m_unknown50=timer; }
void SPRITE::SetTime(unsigned long time) { m_tactTime=time; }

void SPRITE::AddActionImmediate(int action,int var1,int var2,int var3)
{
    m_actions.Insert(ACT(action,var1,var2,var3));
}

void SPRITE::SetSpeed(float speed) { m_speed=speed; }

void SPRITE::Move(float x,float y,float z)
{
    SPRITE* marker=new SPRITE(EmptyVid,x,y,z,ANGLE(static_cast<uint8_t>(0)),0);
    Move(marker);
}

int SPRITE::SetCommand(int command,float x,float y,float z)
{
    SPRITE* goal=new SPRITE(EmptyVid,x,y,z,ANGLE(static_cast<uint8_t>(0)),0);
    return SetCommand(command,goal);
}

void SPRITE::Move(SPRITE* goal)
{
    if (SetCommandWithoutLink(1,goal) || StartMove()) {
        if (HaveFightLink() && m_child && !m_child->IsCommand(0) && !m_child->IsCommand(4))
            m_child->SetCommand(0,static_cast<SPRITE*>(0));
    } else {
        SetCommand(0,static_cast<SPRITE*>(0));
    }
}

ANGLE SPRITE::Rotate(ANGLE endDirection,int deltaTime)
{
    if (endDirection.value==m_dir)
        return ANGLE(static_cast<uint8_t>(0));

    // Retail uses the float at VID+0x3C as angular speed. 0x497423F0 is
    // the historical 999999.0f sentinel for an immediate direction change.
    const float rotateSpeed=m_vid->m_childDirectionLock;
    if (*reinterpret_cast<const int*>(&rotateSpeed)==0x497423F0) {
        ChangeDirection(endDirection);
        return ANGLE(static_cast<uint8_t>(1));
    }

    const int steps=static_cast<int>(static_cast<float>(deltaTime)*rotateSpeed+0.5f);
    const int a=abs(static_cast<int>(m_dir)-static_cast<int>(endDirection.value));
    const int rem=0x100-a;
    if (steps==0)
        return ANGLE(static_cast<uint8_t>(a<rem ? a : rem));

    int forward=0;
    int backward=0;
    if (m_dir>endDirection.value) {
        if (a<rem) forward=1; else backward=1;
    } else {
        if (a<=rem) backward=1; else forward=1;
    }

    const int distance=a<rem ? a : rem;
    if (steps>=distance) {
        ChangeDirection(endDirection);
        return ANGLE(static_cast<uint8_t>(0));
    }
    if (forward)
        ChangeDirection(ANGLE(static_cast<uint8_t>(m_dir-steps)));
    if (backward)
        ChangeDirection(ANGLE(static_cast<uint8_t>(m_dir+steps)));

    const int da=abs(static_cast<int>(m_dir)-static_cast<int>(endDirection.value));
    const int db=0x100-da;
    return ANGLE(static_cast<uint8_t>(da<db ? da : db));
}

// The case set is taken from the retail jump table at 0x0045E0F4; newer ZS1
// action IDs are deliberately absent.
int SPRITE::Action(int action,int var1,int var2,int var3)
{
    const int act=action&0xFF;
    switch (act) {
    case 95:
        return 0;
    case 201:
        ChangeAnimation(var1);
        ChangeDirection(ANGLE(static_cast<uint8_t>(var2)));
        return 0;
    case 202:
        return Animation();
    case 203:
        ChangeCoor(static_cast<float>(var1),static_cast<float>(var2),Z());
        return 0;
    case 204:
        ChangeZCoor(static_cast<float>(var1));
        return 0;
    case 9:
        if (Animation()!=8)
            ChangeAnimation(9);
        return 0;
    case 112:
        if (!m_exData)
            m_exData=new EX_SPRITE_DATA(this);
        m_exData->deathTimer=static_cast<unsigned long>(var1);
        return 0;
    case 134:
        if (Map->ValidateVid(var1)) {
            SPRITE* victim=Map->GetSpriteScr(var1+0x800,static_cast<float>(var2),static_cast<float>(var3));
            if (victim)
                victim->ScalarDeletingDestructor(1u);
        }
        return 0;
    case 35: {
        if (!var1)
            var1=Action(59,4,0,0);
        if (var1<=0 || !Map->ValidateVid(var1))
            return 0;
        float x=static_cast<float>(var2);
        float y=static_cast<float>(var3);
        if (var2==0 && var3==0) {
            x=X(); y=Y();
        } else {
            if (var2<0)
                x=X()-static_cast<float>(var2)-2.0f*static_cast<float>(Random(-var2));
            if (var3<0)
                y=Y()-static_cast<float>(var3)-2.0f*static_cast<float>(Random(-var3));
        }
        SPRITE* child=Map->CreateSprite(Map->Vid(var1),x,y,Z(),Direction(),this);
        if (child)
            Action(75,reinterpret_cast<int>(child),0,0);
        return 0;
    }
    case 101: return reinterpret_cast<int>(m_child);
    case 103: return reinterpret_cast<int>(m_parent);
    case 132:
        m_flag|=0x100u;
        Hash->Delete(this);
        Remove();
        return 0;
    case 43: {
        SPRITE* flagman=Map->Flagman();
        if (flagman && flagman->NearDistanceTo(static_cast<float>(var1),static_cast<float>(var2))<static_cast<float>(var3))
            return 0;
        AddActionImmediate(action,var1,var2,var3);
        return 0;
    }
    case 133:
        m_flag&=~0x100u;
        Hash->Insert(this);
        Insert();
        return 0;
    case 98:
        if (var1) InvisibleOn(); else InvisibleOff();
        return 0;
    case 109: return static_cast<int>(Speed()*1000.0f);
    case 110: SetSpeed(static_cast<float>(var1)*0.001f); return 0;
    case 107: return static_cast<int>(m_unknown24*1000.0f);
    case 108: m_unknown24=static_cast<float>(var1)*0.001f; return 0;
    case 73:
        AddActionImmediate(action,var1,var2,var3);
        Action(130,0,0,0);
        return 0;
    case 72:
        m_actions.Release();
        return 0;
    case 76:
        return m_actions.m_no;
    case 71:
        if (var1+1>m_actions.m_max)
            m_actions.Expand(var1+1);
        m_actions.m_no=var1+1;
        m_noCadr=m_endCadr;
        return 0;
    case 75: {
        SPRITE* dest=reinterpret_cast<SPRITE*>(var1);
        if (!dest || dest==this)
            return 0;
        dest->ResetActionStack();
        for (int i=0;i<m_actions.m_no;++i) {
            const ACT& src=m_actions.m_data[i];
            if (src.act==0x49)
                return 0;
            dest->AddActionImmediate(src.act,src.var1,src.var2,src.var3);
        }
        return 0;
    }
    case 74:
        SetCommand((action>>8)&0xFF,reinterpret_cast<SPRITE*>(var1));
        if (var1)
            reinterpret_cast<SPRITE*>(var1)->Release();
        return 0;
    case 70:
        AddActionImmediate((Command()<<8)+0x4A,reinterpret_cast<int>(Goal()),0,0);
        if (Goal()) Goal()->AddRef();
        return 0;
    case 54:
        InsertItem(var1);
        return 0;
    case 58:
        return GetItemNumber(var1);
    case 56:
        return HaveItem(var1);
    case 55:
        if (m_exData) {
            LIST<int>& list=m_exData->items;
            for (int i=list.m_no-1;i>=0;--i) {
                if (list.m_data[i]==var1) {
                    for (int j=i+1;j<list.m_no;++j)
                        list.m_data[j-1]=list.m_data[j];
                    --list.m_no;
                    return 1;
                }
            }
        }
        return 0;
    case 57:
        if (m_exData) m_exData->items.Release();
        return 0;
    case 59: {
        if (!var1) var1=0xFFFFFF;
        if (!m_exData) return -1;
        int count=0;
        for (int i=0;i<m_exData->items.m_no;++i) {
            const int nvid=m_exData->items.m_data[i];
            VID* vid=Map->Vid(nvid);
            if (vid && vid->IsSpriteType(static_cast<unsigned int>(var1)))
                ++count;
        }
        if (!count) return -1;
        int pick=Random(count-1);
        for (int i=0;i<m_exData->items.m_no;++i) {
            const int nvid=m_exData->items.m_data[i];
            VID* vid=Map->Vid(nvid);
            if (vid && vid->IsSpriteType(static_cast<unsigned int>(var1)) && --pick<0)
                return nvid;
        }
        return -1;
    }
    case 130: {
        if (IsDying()) return 0;
        if (m_parent || MaxSpeed()==0.0f) {
            if (m_vid->PropWind() && Graph->WindSpeed()!=0.0f)
                Rotate(Graph->WindDirection(),DeltaTime());
        }
        if (Animation()==8 && m_noCadr<=m_endCadr)
            return 0;
        if (m_parent) {
            if (m_parent->Vid()->m_spriteClass==7) {
                if (GetTimer()!=0) {
                    if (m_ani>=6 && m_ani!=10) ChangeAnimation(10);
                } else if (m_ani==10) ChangeAnimation(0);
            } else if (Goal()) {
                Rotate(DirectionTo(Goal()),DeltaTime());
            } else if (GetTimer()==0) {
                Rotate(m_parent->Direction(),DeltaTime());
            }
        }
        if (m_ani==10) return 0;
        if (m_speed!=0.0f) {
            if (Animation()!=2) ChangeAnimation(2);
        } else if (m_ani==2 || (m_ani>=6 && m_ani!=10)) {
            ChangeAnimation(0);
        }
        return 0;
    }
    case 62: {
        if (!Map->ValidateVid(var1) || m_vid->m_idx==var1)
            return 0;
        VID* newVid=Map->Vid(var1);
        if (!newVid) return 0;
        if (m_vid->m_spriteClass!=newVid->m_spriteClass)
            Error(4,"ACT_CHANGE_VID",static_cast<unsigned long>(var1));
        for (VID* link=m_vid->m_linkVid;link;link=link->m_linkVid)
            DestroyLink(link);
        m_flag&=~0x2000u;
        Hash->Delete(this);
        Remove();
        m_vid->DecreaseNoSprites(Army());
        m_vid=newVid;
        m_vid->IncreaseNoSprites(Army());
        if (var2<0) var2=m_ani;
        var3=static_cast<int>(m_dir);
        m_dir=0;
        m_ani=0;
        m_begCadr=0;
        m_noCadr=0;
        m_endCadr=m_vid->m_aniFrameLimit[0]-1;
        if (m_vid->m_exSpriteData) {
            if (!m_exData) m_exData=new EX_SPRITE_DATA(this);
            else m_exData->deathTimer=m_vid->m_defaultDeathTimer;
        }
        Insert();
        Hash->Insert(this);
        CreateLink();
        ChangeAnimation(var2);
        ChangeDirection(ANGLE(static_cast<uint8_t>(var3)));
        return 0;
    }
    case 135:
        PlaySFX(var1);
        return 0;
    case 131:
        Map->ScriptRun(var1,this,0,0);
        return 0;
    case 42:
        SetCommand(0,static_cast<SPRITE*>(0));
        return 0;
    case 61:
        ChangeAnimation(var1);
        return 0;
    case 60:
        ChangeDirection(ANGLE(static_cast<uint8_t>(var1)));
        return 0;
    case 63:
        ChangeCoor(static_cast<float>(var1),static_cast<float>(var2),static_cast<float>(var3));
        return 0;
    case 105:
        return static_cast<int>(GetTimer());
    case 106:
        SetTimer(static_cast<unsigned long>(var1));
        return 0;
    case 97:
        ChangeArmy(var1);
        return 0;
    case 96:
        return Army();
    case 90:
        return reinterpret_cast<int>(Goal());
    case 91: {
        SPRITE* goal=new SPRITE(EmptyVid,static_cast<float>(var1),static_cast<float>(var2),static_cast<float>(var3),ANGLE(static_cast<uint8_t>(0)),0);
        SetGoal(goal);
        return 0;
    }
    case 92:
    case 93:
        return m_parent ? m_parent->Action(action,var1,var2,var3) : 0;
    case 111:
        return Command();
    case 100:
        return static_cast<int>(BattleRange());
    case 87:
        return Hp();
    case 88:
        if (!var1) ChangeHp(MaxHp()*var2/100); else ChangeHp(var1);
        return 0;
    case 89:
        return PercentHp()*100/255;
    case 85: {
        if (Hp()>=MaxHp() && var1<0)
            return 1;
        if (IsDying())
            return 0;
        const int damageHook=SpriteVidInt(m_vid,0x450u);
        if (damageHook>=0 && Map->ScriptRun(damageHook,this,reinterpret_cast<SPRITE*>(var2),var1))
            return 0;
        // VID::IsInvulnerable in retail is (dword[+0x28] == 0).
        if (SpriteVidInt(m_vid,0x28u)!=0)
            ChangeHp(Hp()-var1);
        if (Hp()>MaxHp() && var1<0)
            m_hp=MaxHp();
        if (var1<=0)
            return 0;
        if (m_vid->m_noAnimCadr[7] && (Animation()==0 || Animation()==2)) {
            ChangeAnimation(7);
            return 0;
        }
        const int hitHook=SpriteVidInt(m_vid,0x424u);
        if (hitHook>=0 && Map->ScriptRun(hitHook,this,reinterpret_cast<SPRITE*>(var2),0))
            return 0;
        if (m_vid->m_aniSfx[7])
            PlaySFX(m_vid->m_aniSfx[7]);
        if (m_vid->m_aniChildVid[7]) {
            const int oldAni=m_ani;
            m_ani=7;
            CreateChild();
            m_ani=oldAni;
        }
        return 0;
    }
    case 86:
        DestroyLink(m_vid->m_linkVid);
        ChangeHp(MaxHp());
        if (m_child && m_child->m_vid==m_vid->m_linkVid)
            m_child->Action(86,0,0,0);
        else
            CreateLink();
        return 0;
    case 38:
        if (Random(4)==0) { ChangeAnimation(0); return 0; }
        if (Random(4)==0) { ChangeAnimation(12); return 0; }
        if (m_ani==4 && Random(2)!=0) { Rotate(ANGLE(static_cast<uint8_t>(m_dir-0x40)),DeltaTime()); return 0; }
        if (m_ani==5 && Random(2)!=0) { Rotate(ANGLE(static_cast<uint8_t>(m_dir+0x40)),DeltaTime()); return 0; }
        if (Random(3)!=0) return 0;
        if (Random(1)!=0) Rotate(ANGLE(static_cast<uint8_t>(m_dir-0x40)),DeltaTime());
        else Rotate(ANGLE(static_cast<uint8_t>(m_dir+0x40)),DeltaTime());
        return 0;
    case 40:
        if (Speed()!=0.0f) Stop();
        SetTimer(static_cast<unsigned long>(var1+Random(var2)));
        SetCommand(GetTimer()!=0 ? 18 : 0,static_cast<SPRITE*>(0));
        return 0;
    case 41:
        Rotate(ANGLE(static_cast<uint8_t>(var1)),DeltaTime());
        return 0;
    case 39:
        Stop();
        if (var1) SetSpeed(0.0f);
        return 0;
    case 33:
        Move(static_cast<float>(var1),static_cast<float>(var2),static_cast<float>(var3));
        return 0;
    case 34:
        Move(reinterpret_cast<SPRITE*>(var1));
        return 0;
    case 32:
        Attack(reinterpret_cast<SPRITE*>(var1));
        return 0;
    case 37: {
        float targetZ;
        if (AttackedSpriteType()==8u) {
            targetZ=Map->GetGroundZ(static_cast<float>(var1),static_cast<float>(var2)+80.0f)+80.0f;
            var2+=static_cast<int>(targetZ);
        } else {
            targetZ=Map->GetGroundZ(static_cast<float>(var1),static_cast<float>(var2))+19.0f;
            var2+=static_cast<int>(targetZ)-19;
        }
        SetCommand(4,static_cast<float>(var1),static_cast<float>(var2),targetZ);
        return 0;
    }
    case 200: {
        STREAM* res=reinterpret_cast<STREAM*>(var1);
        short count=0;
        res->Read(&count,2u);
        if (count>m_actions.m_max) m_actions.Expand(count);
        m_actions.m_no=count;
        for (int i=0;i<count;++i) {
            int rec[3]={0,0,0};
            res->Read(rec,12u);
            ACT& a=m_actions.m_data[i];
            a.act=rec[0]; a.var1=rec[1]; a.var2=rec[2]; a.var3=0;
        }
        if (var2<7) m_actions.Release();
        for (int i=0;i<m_actions.m_no;++i) {
            ACT& a=m_actions.m_data[i];
            a.act&=0xFF;
            a.var3=0;
            if (a.act==0x28) a.act=0x21;
            else if (a.act==0x27) a.act=0x20;
            else if (a.act==0x2F) a.act=0x49;
            else Error(14,"actionStack.act restore",static_cast<unsigned long>(a.act));
            if (ActionNeedSpriteInVar1(a.act))
                a.var1=reinterpret_cast<int>(Map->m_relation.Decode(reinterpret_cast<SPRITE*>(a.var1)));
        }
        if (var2>=7) {
            unsigned char army=0;
            res->Read(&army,1u);
            ChangeArmy(army);
        }
        return 0;
    }
    case 80: {
        // Retail 0x0045D967: a lone ACT 0x49 is transient and is not serialized.
        if (m_actions.No()==1 && m_actions[0]->act==0x49)
            m_actions.Release();

        STREAM* res=reinterpret_cast<STREAM*>(var1);
        m_actions.Write(res);

        LIST<int> items;
        if (m_exData)
            items=&m_exData->items;
        items.Write(res);
        return 0;
    }
    case 81: {
        STREAM* res=reinterpret_cast<STREAM*>(var1);
        m_actions.Read(res);
        for (int i=m_actions.No()-1;i>=0;--i) {
            ACT& a=*m_actions[i];
            if (ActionNeedSpriteInVar1(a.act)) {
                a.var1=reinterpret_cast<int>(Map->m_relation.Decode(reinterpret_cast<SPRITE*>(a.var1)));
            } else if (i>0 && a.act==0x49 && m_actions[i-1]->act==0x49) {
                m_actions.DeleteNumber(i);
            }
        }
        if (var2>=12) {
            LIST<int> items;
            items.Read(res);
            if (items.No()) {
                if (!m_exData)
                    m_exData=new EX_SPRITE_DATA(this);
                m_exData->items=&items;
            }
        }
        return 0;
    }
    case 15: {
        const int fireDamage=SpriteVidInt(m_vid,0x48u);
        if (!fireDamage) return 0;
        m_hp=0;
        const float blastRadius=*reinterpret_cast<const float*>(reinterpret_cast<const unsigned char*>(m_vid)+0x44u);
        const float rx=blastRadius+m_vid->m_snapOffsetX;
        const float ry=blastRadius+m_vid->m_snapOffsetY;
        const float rz=m_vid->m_hitVerticalOffset<20.0f ? 20.0f : m_vid->m_hitVerticalOffset;
        for (SPRITE* target=Hash->FirstInBox(m_x-rx,m_y-ry,m_x+rx,m_y+ry);target;target=Hash->NextInBox()) {
            if (target==this || SpriteVidInt(target->m_vid,0x28u)==0)
                continue;
            if ((m_vid->m_flag&0x80000000u) && target->Army()==Army())
                continue;
            if (fabsf(m_x-target->m_x)>=rx+target->m_vid->m_snapOffsetX ||
                fabsf(m_y-target->m_y)>=ry+target->m_vid->m_snapOffsetY ||
                fabsf(m_z-target->m_z)>=rz+target->m_vid->m_hitVerticalOffset)
                continue;
            const float g1=Map->GetGroundZ((target->m_x+m_x)*0.5f,(target->m_y+m_y)*0.5f);
            const float ztop=target->m_z+target->m_vid->m_hitVerticalOffset;
            if (g1>ztop) continue;
            const float g2=Map->GetGroundZ((m_x*3.0f+target->m_x)*0.25f,(m_y*3.0f+target->m_y)*0.25f);
            if (g2>ztop) continue;
            const float g3=Map->GetGroundZ((target->m_x*3.0f+m_x)*0.25f,(target->m_y*3.0f+m_y)*0.25f);
            if (g3>ztop) continue;
            int damage=fireDamage;
            if ((m_vid->m_flag&0x04000000u) && blastRadius!=0.0f) {
                const float distance=NearDistanceTo(target->m_x,target->m_y);
                if (distance>blastRadius) continue;
                damage=static_cast<int>(static_cast<float>(fireDamage)-static_cast<float>(fireDamage)*distance/blastRadius);
            }
            target->Action(85,damage,reinterpret_cast<int>(this),0);
        }
        return 0;
    }
    default:
        Error(10,"Action() have not this act",static_cast<unsigned long>(action));
        return 0;
    }
}

void SPRITE::MoveTact()
{
    if (!m_vid->m_moveTactData)
        return;

    float newX;
    float newY;
    float newZ;
    MoveTactCalcCoor(&newX,&newY,&newZ);

    const float oldGround=Map->GetGroundZ(m_x,m_y);
    const float newGround=Map->GetGroundZ(newX,newY);
    const float desiredZ=newGround+m_vid->m_groundOffset;

    if (IsSpriteType(0x200u) && m_vid->PropGravity()) {
        if (newZ<=newGround && m_z>=oldGround)
            newZ=m_z;
    }

    if (m_z!=newZ) {
        if (!m_vid->PropGravity() && desiredZ!=0.0f) {
            if (m_z<desiredZ) {
                if (newZ>=desiredZ) {
                    newZ=desiredZ;
                    m_unknown24=0.0f;
                }
            } else if (m_z>desiredZ) {
                if (newZ<desiredZ) {
                    newZ=desiredZ;
                    m_unknown24=0.0f;
                }
            } else if (!m_vid->PropSelfMoving()) {
                m_unknown24=0.0f;
            }
        }
    }

    if (Goal() && Speed()==0.0f && IsMoveFinished())
        Stop();

    if (m_x!=newX || m_y!=newY) {
        if (CanPlaceWithCrush(newX,newY,newZ)) {
            m_unknown24=0.0f;
            m_speed=0.0f;
            m_flag|=0x400u;
        } else {
            ChangeCoor(newX,newY,newZ);
        }
    }

    if (m_z!=newZ)
        ChangeZCoor(newZ);
}


void SPRITE::MoveTactCalcCoor(float* new_x,float* new_y,float* new_z)
{
    // Retail MapEdit.exe 0x0045B0F8.  This owner is deliberately kept in the
    // original state-machine form: bit 0x80 is the StartMove latch, 0x400 is
    // the line-intersection latch, and 0x4000/0x8000 are the per-axis finish
    // latches consumed by IsMoveFinished().
    m_flag&=~0x00000400u;
    *new_x=X();
    *new_y=Y();
    *new_z=Z();

    if (IsCommand(1) && (m_flag&0x00000080u)==0u) {
        Error(E_ERROR,"Move without StartMove()",0);
        StartMove();
    }
    if (IsCommand(1) && Goal()==0) {
        Error(E_ERROR,"Move without goal",0);
        Stop();
    }

    const float dt=static_cast<float>(CurrentTime-PrevCurrentTime);
    if ((m_flag&0x00000080u)!=0u) {
        if (MaxSpeed()>m_speed) {
            if (m_vid->m_acceleration!=999999.0f) {
                m_speed+=dt*m_vid->m_acceleration;
                if (m_speed>MaxSpeed())
                    m_speed=MaxSpeed();
            } else {
                m_speed=MaxSpeed();
            }
        }
    } else if (m_speed>0.0f) {
        if (m_vid->m_deceleration!=999999.0f) {
            m_speed-=dt*m_vid->m_deceleration;
            if (m_speed<0.0f)
                m_speed=0.0f;
        } else {
            m_speed=0.0f;
        }
    }

    if (m_speed!=0.0f || (Graph->WindSpeed()!=0.0f && m_vid->PropWind())) {
        if (m_speed!=999999.0f) {
            float nodot=dt*m_speed;
            ANGLE dir=(m_vid->PropMoveWithAnyDirection() && Goal()!=0)
                ? DirectionTo(Goal()) : m_dir;
            *new_x+=dir.Sin()*nodot;
            *new_y-=dir.Cos()*nodot;

            if (m_vid->PropWind()) {
                nodot=Graph->WindSpeed()*dt;
                ANGLE wind=Graph->WindDirection();
                *new_x+=wind.Sin()*nodot;
                *new_y-=wind.Cos()*nodot;
            }
        } else if (Goal()!=0 && !IsMoveFinished()) {
            float end_x=Goal()->X();
            float end_y=Goal()->Y();
            float end_z=Goal()->Z();
            const int intersection=AskLine(&end_x,&end_y,&end_z);
            ChangeCoor(end_x,end_y,end_z);
            if (intersection)
                m_flag|=0x00000400u;
            *new_x=end_x;
            *new_y=end_y;
            *new_z=end_z;
            m_flag|=0x0000C000u;
            return;
        } else if (m_speed==999999.0f) {
            m_flag|=0x0000C000u;
        }
    }

    if (m_vid->PropGravity())
        m_unknown24-=dt*Const->gravity;
    else if (m_vid->PropGravity2())
        m_unknown24-=dt*Const->gravity2;

    *new_z+=dt*m_unknown24;

    if (Goal()!=0) {
        if (NearBetween(Goal()->X(),m_x,*new_x,0.5f))
            m_flag|=0x00004000u;
        if (NearBetween(Goal()->Y(),m_y,*new_y,0.5f))
            m_flag|=0x00008000u;
    }
}

int SPRITE::AskLine(float* endx,float* endy,float* endz)
{
    // Retail MapEdit.exe 0x00460771: thin HASH_MAP::AskLine owner.
    return Hash->AskLine(m_vid,m_x,m_y,m_z,endx,endy,endz);
}

int SPRITE::Attack(SPRITE* goal)
{
    if (IsSpriteClass(12) && m_vid->m_aniChildVid[8]) {
        const int oldAnimation=m_ani;
        m_ani=8;
        SetCommand(4,goal);
        CreateChild();
        m_ani=oldAnimation;
        SetCommand(0,static_cast<SPRITE*>(0));
        return 0;
    }

    if (goal && goal->m_vid!=EmptyVid &&
        !CanAttackThisSprite(goal) && !IsSpriteClass(5)) {
        if (!Action(159,0,0,0))
            return 0;
    }

    SPRITE* const target=(goal && goal->m_vid!=EmptyVid) ? goal : 0;
    SetBestTarget(target);
    if (HaveFightLink())
        m_child->SetBestTarget(target);

    SetCommand(3,goal);
    return 0;
}

int SPRITE::Attack(float x,float y,float z)
{
    SPRITE* const marker=new SPRITE(EmptyVid,x,y,z,ANGLE(static_cast<uint8_t>(0)),0);
    return Attack(marker);
}

// implemented directly from reference behavior + debug metadata source-line/local records.
void SPRITE::CreateChild()
{
    VID* const nvid=m_vid->m_aniChildVid[m_ani];
    int nochild=abs(m_vid->m_aniFireCount[m_ani]);
    int n_child=0;

    if (!nvid || nvid->PropNotCreateAsChild())
        return;

    ANGLE step_direction = nvid->PropNotChangeLinkerCoor()
        ? ANGLE(static_cast<uint8_t>(0))
        : m_vid->SteppedDirection(Direction());

    // Retail uses two flag bits for the alternating two-barrel path:
    // 0x2000 selects the first child index and 0x0200 is cleared on that path.
    if (nochild==2 && m_vid->m_weapon->PropInTurn()) {
        if (m_flag&0x00002000u) {
            n_child=1;
            m_flag&=~0x00000200u;
        } else {
            nochild=1;
        }
        m_flag^=0x00002000u;
    }

    for (;n_child<nochild;++n_child) {
        float begx;
        float begy;
        float endx=0.0f;
        float endy=0.0f;

        if (m_vid->m_aniSpawnMode[m_ani]<0) {
            if (IsSpriteClass(23) &&
                m_vid->m_aniSpawnX[m_ani]==0.0f &&
                m_vid->m_aniSpawnY[m_ani]==0.0f) {
                REGION* const region=static_cast<REGION*>(this);
                if (region->property&8u) {
                    begx=Random(Map->SizeX())-X();
                    // This apparently unusual Z contribution is exactly what
                    // the MapEdit retail owner emits at source line 1823.
                    begy=Random(Map->SizeY())-Y()+Z();
                } else {
                    begx=region->sizeX/2.0f-Random(region->sizeX);
                    begy=region->sizeY/2.0f-Random(region->sizeY)+Z();
                }
            } else {
                const float rand_x=m_vid->m_aniSpawnX[m_ani]-
                                   Random(m_vid->m_aniSpawnX[m_ani]*2.0f);
                const float rand_y=m_vid->m_aniSpawnY[m_ani]-
                                   Random(m_vid->m_aniSpawnY[m_ani]*2.0f);
                endx=-rand_x*step_direction.Cos();
                endy=-rand_x*step_direction.SinY();
                begx=rand_y*step_direction.Sin()+endx;
                begy=endy-rand_y*step_direction.CosY();
            }
        } else if (n_child==1) {
            endx=-m_vid->m_aniSpawnX[m_ani]*step_direction.Cos();
            endy=-m_vid->m_aniSpawnX[m_ani]*step_direction.SinY();
            begx=m_vid->m_aniSpawnY[m_ani]*step_direction.Sin()+endx;
            begy=endy-m_vid->m_aniSpawnY[m_ani]*step_direction.CosY();
        } else if (n_child==2) {
            begx=m_vid->m_aniSpawnY[m_ani]*step_direction.Sin();
            begy=m_vid->m_aniSpawnY[m_ani]*step_direction.CosY();
            endx=0.0f;
            endy=0.0f;
        } else {
            endx=m_vid->m_aniSpawnX[m_ani]*step_direction.Cos();
            endy=m_vid->m_aniSpawnX[m_ani]*step_direction.SinY();
            begx=m_vid->m_aniSpawnY[m_ani]*step_direction.Sin()+endx;
            begy=endy-m_vid->m_aniSpawnY[m_ani]*step_direction.CosY();
        }

        if (nvid->m_spriteClass==2 &&
            Hash->CanPlace(nvid,m_x+begx,m_y+begy,
                           m_z+m_vid->m_aniSpawnZ[m_ani])) {
            continue;
        }

        if (m_ani==8 && !Goal())
            break;

        ANGLE dir=Direction();
        if (nvid->PropRandBirth()) {
            dir=ANGLE(static_cast<uint8_t>(Random(255)));
        } else if (nvid->PropVertDir()) {
            EX_SPRITE_DATA* const ex=ExData();
            if (X()!=ex->lastX || Y()!=ex->lastY) {
                dir=ANGLE(X()-ex->lastX,
                          (Y()-Z()-ex->lastY+ex->lastZ)/0.7070602178573608f);
            } else {
                ANGLE current=Direction();
                dir=ANGLE(current.Sin()*Speed(),
                          -(current.Cos()*Speed()+ZSpeed())/0.7070602178573608f);
            }
        }

        SPRITE* const spr=Map->CreateSprite(
            nvid,m_x+begx,m_y+begy,m_z+m_vid->m_aniSpawnZ[m_ani],dir,this);

        if (spr && Goal() && m_ani==8) {
            if (m_vid->m_weapon->PropSelfDirecting() && Goal()->m_vid!=EmptyVid) {
                spr->Attack(Goal());
                spr->StartMove();
            } else {
                const float aim=m_vid->m_weapon->m_targetRadius;
                float x=0.0f;
                float y=0.0f;
                int j=0;
                for (;j<5;++j) {
                    x=Goal()->m_x+endx+aim-Random(aim*2.0f);
                    y=Goal()->m_y+endy+aim-Random(aim*2.0f);
                    if (Map->GetGroundZ(x,y)<Goal()->m_z &&
                        Map->GetGroundZ((Goal()->m_x+x)/2.0f,
                                        (Goal()->m_y+y)/2.0f)<Goal()->m_z)
                        break;
                }
                spr->Attack(x,y,Goal()->m_z);
                spr->StartMove();
            }
        }
    }

    if (m_ani==8 && (m_flag&0x00002000u)==0u &&
        (IsCommand(4) || IsCommand(5))) {
        if ((m_vid->PropTrack() || nvid->PropBirthAsSmoke()) &&
            m_noCadr!=m_endCadr)
            return;

        if (Goal() && m_parent && m_parent->Goal()==Goal()) {
            if (m_parent->IsCommand(4) || m_parent->IsCommand(5))
                m_parent->SetCommand(0,static_cast<SPRITE*>(0));
            if (m_parent->IsSpriteClass(21) && m_parent->IsCommand(29))
                reinterpret_cast<ENGINE*>(m_parent)->SetCommandToTrain(30,0,0,0);
        }
        SetCommand(0,static_cast<SPRITE*>(0));
    }
}

void SPRITE::CreateChildAndPlaySFX(int for_animation)
{
    if (m_vid->m_aniChildVid[for_animation]) {
        const int oldAnimation=m_ani;
        m_ani=for_animation;
        CreateChild();
        m_ani=oldAnimation;
    }

    if (m_vid->m_aniSfx[for_animation])
        PlaySFX(m_vid->m_aniSfx[for_animation]);
}

PTR_SPRITE::PTR_SPRITE()
    : sprite(0)
{
}

// debug metadata lost the explicit SPRITE* parameter in the old ABI table; the push
// at [ebp+8] and the AddRef call prove the actual one-argument thiscall owner.
// The holder owns one reference to the assigned sprite.  Retail increments
// the incoming reference before releasing the previous one so self-assignment
// keeps the object alive throughout the exchange.
PTR_SPRITE& PTR_SPRITE::operator=(SPRITE* spr)
{
    if (spr)
        spr->AddRef();
    if (sprite)
        sprite->Release();
    sprite=spr;
    return *this;
}

PTR_SPRITE::operator SPRITE*()
{
    return sprite;
}

PTR_SPRITE::~PTR_SPRITE()
{
    if (sprite)
        sprite->Error(10,"PTR_SPRITE with this sprite not clear",0);
}

SPRITE* PTR_SPRITE::operator->()
{
    return sprite;
}

float SPRITE::X() const { return m_x; }
float SPRITE::Y() const { return m_y; }
float SPRITE::Z() const { return m_z; }



// -----------------------------------------------------------------------------
// Factory-derived lightweight SCC. These owners are direct reconstructions of
// the original MapEdit.exe VC6 bodies; no build-only stubs are used here.
// -----------------------------------------------------------------------------

PRIMITIVE::PRIMITIVE(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : SPRITE(vid,x,y,z,direction,parent)
{
}

PRIMITIVE::~PRIMITIVE()
{
}

void* PRIMITIVE::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->PRIMITIVE::~PRIMITIVE();
    if (flags&1u)
        operator delete(self);
    return self;
}

void SPRITE::BreakLink()
{
    if (m_child)
        m_child->m_parent=m_parent;
    if (m_parent)
        m_parent->m_child=m_child;
    m_parent=0;
    m_child=0;
}

LINKER::LINKER(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* owner)
    : SPRITE(vid,x,y,z,direction,owner),
      linkX(0.0f),linkY(0.0f),linkZ(0.0f),beginDirection(),parent(owner)
{
    if (owner) {
        owner->AddLinkToLast(this);
        linkX=x-owner->X();
        linkY=y-owner->Y();
        linkZ=z-owner->Z();
    }
    beginDirection.value=direction.value;
}

LINKER::~LINKER()
{
    BreakLink();
}

void* LINKER::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->LINKER::~LINKER();
    if (flags&1u)
        operator delete(self);
    return self;
}

FRAME::FRAME(VID* vid,float screenX,float screenY,float z,ANGLE direction,SPRITE* parent)
    : SPRITE(vid,Map->FromScreenX(screenX),Map->FromScreenY(screenY),z,direction,parent)
{
    Map->Menu()->Insert(this);
}

FRAME::~FRAME()
{
    Map->Menu()->Delete(this);
    Map->DeletePointerToSprite(this);
}

void* FRAME::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->FRAME::~FRAME();
    if (flags&1u)
        operator delete(self);
    return self;
}

int FRAME::Action(int action,int var1,int var2,int var3)
{
    if (action>=0 && action<=5)
        return 0;

    if (action==130) {
        if (IsDying())
            return 0;
        if (Animation()==4) {
            ChangeAnimation(2);
            m_flag|=0x00000200u;
        } else if (Animation()==5) {
            ChangeAnimation(3);
            m_flag|=0x00000200u;
        }
        if (Animation()==14)
            ChangeAnimation(0);
        return 0;
    }

    return SPRITE::Action(action,var1,var2,var3);
}


STEXT::STEXT(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : FRAME(vid,x,y,z,direction,parent),text(),describe(),length(0),behave(0),noRow(1),noColumn(0)
{
}

STEXT::~STEXT()
{
}

void* STEXT::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->STEXT::~STEXT();
    if (flags&1u)
        operator delete(self);
    return self;
}

void STEXT::CalcTextProperty()
{
    int last_nl=0;
    noRow=1;
    noColumn=0;
    int i=0;
    for (; text[i]; ++i) {
        if (text[i]=='\n') {
            if (i-last_nl>noColumn)
                noColumn=i-last_nl;
            last_nl=i+1;
            ++noRow;
        }
    }
    length=i;
    if (noColumn==0)
        noColumn=length;
}

void STEXT::Draw()
{
    if (!m_vid || m_vid==EmptyVid || m_vid->m_noDirections<=126) {
        Graph->PutsXY(m_x,m_y,text.CharPtr(),COLOR(255,255,255));
        return;
    }

    const int old_cadr=CurrentCadr();
    const float old_x=m_x;
    const float old_y=m_y;
    VID* draw_vid=m_vid;

    if ((behave&0x70u)==0x60u) {
        text=Map->ScriptVariable(STRING(&describe));
        CalcTextProperty();
    }
    if (text==STRING::EMPTY)
        return;

    if (behave&1u)
        m_x-=static_cast<float>(noColumn-1)*m_vid->m_footprintWidth/2.0f;
    else if (behave&2u)
        m_x-=m_vid->m_snapOffsetX+static_cast<float>(noColumn-1)*m_vid->m_footprintWidth;
    else
        m_x+=m_vid->m_snapOffsetX;

    if (behave&8u)
        m_y-=static_cast<float>(noRow-1)*m_vid->m_footprintHeight/2.0f;
    else if (behave&4u)
        m_y-=m_vid->m_snapOffsetY+static_cast<float>(noRow-1)*m_vid->m_footprintHeight;
    else
        m_y+=m_vid->m_snapOffsetY;

    const float begx=m_x-m_vid->m_footprintWidth;
    for (int i=0; text[i] && i<length; ++i,m_x+=draw_vid->m_footprintWidth) {
        if (text[i]=='\n') {
            m_y+=draw_vid->m_footprintHeight;
            m_x=begx;
            continue;
        }
        if (text[i]=='\r') {
            m_x=begx;
            continue;
        }
        if (text[i]=='\t') {
            m_x+=7.0f*draw_vid->m_footprintWidth;
            continue;
        }
        if (text[i]=='<' && text.HaveFirst("<Font=")) {
            draw_vid=Map->Vid(text.After("<Font=").Int());
            m_x-=draw_vid->m_footprintWidth;
            i+=text.Before(">").Length();
            continue;
        }
        if (text[i]==0x1b && i+1<length && Map->ValidateVid(static_cast<unsigned char>(text[i+1]))) {
            ++i;
            draw_vid=Map->Vid(static_cast<signed char>(text[i]));
            m_x-=draw_vid->m_footprintWidth;
            continue;
        }
        if (static_cast<unsigned char>(text[i])>=0x20u) {
            m_noCadr=static_cast<unsigned char>(text[i]);
            void** vtable=*reinterpret_cast<void***>(draw_vid);
            typedef void (__thiscall *Method)(VID*,const SPRITE*);
            reinterpret_cast<Method>(vtable[0x0C/4])(draw_vid,this);
        }
    }

    m_x=old_x;
    m_y=old_y;
    m_noCadr=old_cadr;
}

int STEXT::Action(int act,int var1,int var2,int var3)
{
    switch (act) {
    case 94:
        return static_cast<int>(behave);
    case 95:
        behave=static_cast<unsigned long>(var1);
        return 0;
    case 120: {
        if (var1) describe=reinterpret_cast<STRING*>(var1);
        else describe=STRING::EMPTY;
        switch (behave&0x70u) {
        case 0x10u: {
            STRING section("menu");
            text=Profile->GetString(&section,&describe,&describe);
            break;
        }
        case 0x20u:
            text.LoadFile(describe);
            break;
        case 0:
            text=describe;
            break;
        default:
            text=Map->ScriptVariable(STRING(&describe));
            if ((behave&0x70u)==0x40u)
                text.LoadFile(text);
            else if ((behave&0x70u)==0x50u) {
                STRING section("menu");
                text=Profile->GetString(&section,&text,&text);
            }
            break;
        }
        CalcTextProperty();
        return 0;
    }
    case 121:
        return reinterpret_cast<int>(&describe);
    case 122:
        length=var1;
        return 0;
    case 123:
        text.LoadFile(*reinterpret_cast<STRING*>(var1));
        length=text.Length();
        return 0;
    case 124:
        return reinterpret_cast<int>(&text);
    case 80: {
        FRAME::Action(act,var1,var2,var3);
        STREAM* stream=reinterpret_cast<STREAM*>(var1);
        stream->Write(&behave,4);
        describe.Write(stream);
        return 0;
    }
    case 81: {
        FRAME::Action(act,var1,var2,var3);
        STREAM* stream=reinterpret_cast<STREAM*>(var1);
        stream->Read(&behave,4);
        describe.Read(stream);
        Action(120,reinterpret_cast<int>(&describe),0,0);
        return 0;
    }
    case 130: {
        short flag=0;
        if (!(text==STRING::EMPTY) || GetTimer()!=0)
            return 0;
        const short size=static_cast<short>(text.Length());
        if (length<size) {
            do {
                const int index=length++;
                if (!isspace(static_cast<signed char>(text[index])))
                    break;
                if (length>=size)
                    break;
                if (text[length]=='\n')
                    flag=1;
            } while (1);
            if (flag) {
                if (length<size)
                    --length;
                SetTimer(150);
                ChangeAnimation(2);
            } else {
                ChangeAnimation(1);
                m_flag&=~0x00000200u;
            }
        } else if (Animation()!=0) {
            ChangeAnimation(0);
        }
        return 0;
    }
    default:
        return FRAME::Action(act,var1,var2,var3);
    }
}

TERRAIN::TERRAIN(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : SPRITE(vid,x,y,z,direction,parent),linkhp(-1),godemode(0)
{
}

TERRAIN::~TERRAIN()
{
}

void* TERRAIN::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->TERRAIN::~TERRAIN();
    if (flags&1u)
        operator delete(self);
    return self;
}

void TERRAIN::SetGodeMode(int newGodemode)
{
    godemode=newGodemode;
    Repair(0);
}

int TERRAIN::GetGodeMode()
{
    return godemode;
}

int TERRAIN::Repair(int repairChildFighter)
{
    ChangeHp(MaxHp());

    if (IsLinkDestroy() && repairChildFighter) {
        if (Vid()->m_idx!=35 ||
            Map->Vid(40)->NoSprites(Army()) < Map->Vid(35)->NoSprites(Army())) {
            CreateLink();
            Map->CreateSprite(Map->Vid(590),X(),Y(),Z(),ANGLE(static_cast<uint8_t>(0)),this);
            return 1;
        }
    }

    if (HaveLink())
        Link()->Action(86,0,0,0);
    return 1;
}

int TERRAIN::Action(int action,int var1,int var2,int var3)
{
    if (action==86) {
        Repair(1);
        return 0;
    }
    if (action==200) {
        SPRITE::Action(action,var1,var2,var3);
        int legacyLinkHp=0;
        reinterpret_cast<STREAM*>(var1)->Read(&legacyLinkHp,var2>7 ? 2u : 1u);
        return 0;
    }
    return SPRITE::Action(action,var1,var2,var3);
}

// debug metadata marks this as a const PRIMITIVE member.  It is therefore an
// overload, not the SPRITE::Draw virtual override; the PRIMITIVE vtable keeps
// SPRITE::Draw at slot +0x14 exactly as the retail image does.
void PRIMITIVE::Draw() const
{
    void** vtable=*reinterpret_cast<void***>(m_vid);
    typedef void (__thiscall *Method)(VID*,const SPRITE*);
    reinterpret_cast<Method>(vtable[0x0C/4])(m_vid,this);
}

void PRIMITIVE::Tact()
{
    PrimitiveTact();
}

void PRIMITIVE::MoveTact()
{
}

void PRIMITIVE::DeletePointerToSprite(SPRITE*)
{
}

void PRIMITIVE::DrawSecondaryInfo()
{
}

void PRIMITIVE::Control(INPUT* /*input*/)
{
}

BUILDED_TERRAIN::BUILDED_TERRAIN(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : SPRITE(vid,x,y,z,direction,parent)
{
    VID* hardware=Map->Vid(1024);
    if (hardware==EmptyVid)
        Map->CreateEmptyHardwareGround();

    hardware=Map->Vid(1024);
    if (hardware!=EmptyVid && hardware->m_noDirections==1) {
        // Retail VID_HARDWARE vtable +0x08 = DrawVidToVid(const SPRITE*).
        void** vtable=*reinterpret_cast<void***>(hardware);
        typedef void (__thiscall *DrawVidToVidMethod)(VID*,const SPRITE*);
        reinterpret_cast<DrawVidToVidMethod>(vtable[2])(hardware,this);
        if (!Vid()->PropHash())
            ChangeAnimation(15);
    }
}

BUILDED_TERRAIN::~BUILDED_TERRAIN()
{
}

void* BUILDED_TERRAIN::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->BUILDED_TERRAIN::~BUILDED_TERRAIN();
    if (flags&1u)
        operator delete(self);
    return self;
}

void BUILDED_TERRAIN::Draw()
{
    VID* hardware=Map->Vid(1024);
    if (hardware==EmptyVid || hardware->m_noDirections!=1)
        SPRITE::Draw();
}

// Retail VID::Draw is virtual slot +0x0C. VID itself is represented as the
// raw debug metadata layout here, so preserve that virtual dispatch explicitly.
void SPRITE::Draw()
{
    void** vtable=*reinterpret_cast<void***>(m_vid);
    typedef void (__thiscall *Method)(VID*,const SPRITE*);
    reinterpret_cast<Method>(vtable[0x0C/4])(m_vid,this);
}
VID* SPRITE::Vid() const { return m_vid; }

int SPRITE::IsSpriteClass(unsigned int spriteClass) const
{
    return m_vid->m_spriteClass == spriteClass;
}

float SPRITE::GetGroundZ()
{
    return Map->GetGroundZ(m_vid, m_x, m_y);
}

int SPRITE::RealDirection()
{
    return m_vid->RealDirection(Direction());
}

void SPRITE::ChangeRealDirection(unsigned int realDirection)
{
    ChangeDirection(ANGLE(static_cast<uint8_t>((realDirection << 8) / m_vid->m_noDirections)));
}

void SPRITE::ChangeXCoor(float newX) { ChangeCoor(newX, m_y, m_z); }
void SPRITE::ChangeYCoor(float newY) { ChangeCoor(m_x, newY, m_z); }
void SPRITE::ChangeZCoor(float newZ) { ChangeCoor(m_x, m_y, newZ); }

int VID::IsExtraType()
{
    return m_extraTypeFlags & 0x0200;
}

// Short accessors/helpers emitted into map_edit/player/map objects.
int SPRITE::Animation() const { return m_ani; }
int SPRITE::Army() { return (m_flag >> 11) & 3; }
int SPRITE::IsEnemy(const SPRITE* sprite)
{
    return Army() != const_cast<SPRITE*>(sprite)->Army();
}
void SPRITE::AddRef() { ++m_noRef; }
SPRITE* SPRITE::Link() { return m_child; }
int SPRITE::HaveLink() const
{
    return m_child && m_child->m_vid == m_vid->m_linkVid;
}

float SPRITE::ScreenX()
{
    return Map->ToScreenX(m_x);
}

float SPRITE::ScreenY()
{
    return Map->ToScreenY(m_y - m_z);
}

int SPRITE::ScreenXInt()
{
    return Map->ToScreenXInt(m_x);
}

int SPRITE::ScreenYInt()
{
    return Map->ToScreenYInt(m_y-m_z);
}

int SPRITE::HaveUniqueGamma()
{
    return m_exData && !m_exData->gamma.IsDefault();
}

int SPRITE::IsSpriteType(unsigned int type) const
{
    return m_vid->IsSpriteType(type);
}

int SPRITE::CanFight() const
{
    return m_vid->CanFight();
}

int SPRITE::HaveFightLink() const
{
    return HaveLink() && m_child->CanFight();
}

int InSegment(float x,float center,float halfsize)
{
    return center - halfsize <= x && x <= center + halfsize;
}

REGION::REGION(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : SPRITE(vid,x,y,z,direction,parent),fogColor()
{
    environmentVid=EmptyVid;
    gammaColor=0;
    property=0;
    sizeY=0.0f;
    sizeX=0.0f;
    fogBottom=0;
    fogTop=0;
    fogColor=COLOR(0,0,0);
    fogTable=0;
    fogEnd=0;
    fogTick=0;
    for (int i=0;i<6;++i) {
        conversionVid[i]=0;
        sourceVid[i]=0;
    }
    // windScale (+0x9C) is intentionally not initialized by the retail ctor.
}

REGION::~REGION()
{
    Map->DeletePointerToSprite(this);
    if (fogTable)
        delete[] fogTable;
}

void* REGION::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->REGION::~REGION();
    if (flags&1u)
        operator delete(self);
    return self;
}

void REGION::DrawSecondaryInfo()
{
    COLOR color(Graph->WHITE);
    (void)color;
    Graph->Box(ScreenLeft()-1.0f,ScreenTop()-1.0f,
               ScreenRight()+1.0f,ScreenBottom()+1.0f,COLOR(255,255,255));
}

void REGION::Draw()
{
    const int oldCadr=m_noCadr;
    const float oldX=X();
    const float oldY=Y();

    if (m_vid!=EmptyVid) {
        if ((property&8u)==0u) {
            VID::SetViewPort(static_cast<int>(ScreenLeft()),static_cast<int>(ScreenTop()),
                             static_cast<int>(ScreenRight()),static_cast<int>(ScreenBottom()));
        }

        float tileY=oldY-sizeY/2.0f;
        int n=0;
        while (tileY<oldY+sizeY/2.0f) {
            float tileX=oldX-sizeX/2.0f;
            while (tileX<oldX+sizeX/2.0f) {
                if (!m_vid->PropOnePhase()) {
                    m_noCadr+=2*n;
                    ++n;
                    m_noCadr%=m_vid->m_dotFrameCount;
                }
                m_x=tileX+static_cast<float>(static_cast<int>(m_vid->m_regionTileStepX)/2);
                m_y=tileY+static_cast<float>(static_cast<int>(m_vid->m_regionTileStepY)/2);
                void** vtable=*reinterpret_cast<void***>(m_vid);
                typedef void (__thiscall *DrawMethod)(VID*,const SPRITE*);
                reinterpret_cast<DrawMethod>(vtable[0x0C/4])(m_vid,this);
                tileX+=static_cast<float>(m_vid->m_regionTileStepX);
            }
            tileY+=static_cast<float>(m_vid->m_regionTileStepY);
        }

        if ((property&8u)==0u) {
            VID::SetViewPort(static_cast<int>(Graph->ViewXMin()),static_cast<int>(Graph->ViewYMin()),
                             static_cast<int>(Graph->ViewXMax()),static_cast<int>(Graph->ViewYMax()));
        }
    }

    m_x=oldX;
    m_y=oldY;
    m_noCadr=oldCadr;

    if (fogBottom<fogTop) {
        if (property&1u) {
            const unsigned int tick=CurrentTime&7u;
            if (tick<fogTick) {
                if (fogEnd<fogTop*8)
                    fogEnd+=2;
                else if (fogEnd>fogTop*8)
                    fogEnd=0;
            }
            fogTick=tick;
        } else {
            fogEnd=fogTop*8;
        }

        if (property&8u) {
            Graph->DrawFog(Graph->ViewXMin(),Graph->ViewYMin(),Graph->ViewXMax(),Graph->ViewYMax(),
                           fogBottom,fogTop,fogColor,fogTable,fogEnd,static_cast<int>(property&2u));
        } else {
            Graph->DrawFog(ScreenLeft(),ScreenTop(),ScreenRight(),ScreenBottom(),
                           fogBottom,fogTop,fogColor,fogTable,fogEnd,static_cast<int>(property&2u));
        }
    } else {
        fogEnd=0;
    }
}

int REGION::Action(int act,int var1,int var2,int var3)
{
    COLOR newColor;
    switch (act) {
    case 80: {
        SPRITE::Action(act,var1,var2,var3);
        STREAM* stream=reinterpret_cast<STREAM*>(var1);
        stream->Write(&property,4);
        stream->Write(&fogTop,4);
        stream->Write(&fogBottom,4);
        fogColor.Write(stream);
        stream->Write(&sizeX,4);
        stream->Write(&sizeY,4);
        stream->Write(&windScale,4);
        Map->WriteVid(stream,environmentVid);
        for (int i=0;i<6;++i) {
            Map->WriteVid(stream,sourceVid[i]);
            Map->WriteVid(stream,conversionVid[i]);
        }
        return 0;
    }
    case 81:
    case 200: {
        SPRITE::Action(act,var1,var2,var3);
        STREAM* stream=reinterpret_cast<STREAM*>(var1);
        stream->Read(&property,4);
        int newTop;
        int newBottom;
        stream->Read(&newTop,4);
        stream->Read(&newBottom,4);
        newColor.Read(stream);
        SetFogParameters(newBottom,newTop,newColor);
        if (var2>9) {
            stream->Read(&sizeX,4);
            stream->Read(&sizeY,4);
        } else {
            int oldSize;
            stream->Read(&oldSize,4);
            sizeX=static_cast<float>(oldSize);
            stream->Read(&oldSize,4);
            sizeY=static_cast<float>(oldSize);
        }
        stream->Read(&windScale,4);
        environmentVid=Map->ReadVid(stream);
        if (!environmentVid)
            environmentVid=EmptyVid;
        for (int i=0;i<6;++i) {
            sourceVid[i]=Map->ReadVid(stream);
            conversionVid[i]=Map->ReadVid(stream);
            if (sourceVid[i])
                sourceVid[i]->m_propertyBits|=0x10u;
        }
        return 0;
    }
    default:
        return SPRITE::Action(act,var1,var2,var3);
    }
}

int REGION::IsInsideXY(float x0,float y0)
{
    return InSegment(x0,X(),sizeX/2.0f) &&
           InSegment(y0,Y(),sizeY/2.0f);
}

// Regions live on layer 10. A region can remap one of six source VIDs when
// the point is below its Z+25 plane and either the region-wide bit is set
// or the XY point is inside the region rectangle.
VID* REGION::ConvertVid(VID* oldVid,float x,float y,float z)
{
    int iterator=0;
    for (SPRITE* spr=Map->FirstSprite(10,&iterator); spr;
         spr=Map->NextSprite(10,&iterator)) {
        if (!spr->IsSpriteClass(23))
            continue;

        REGION* region=static_cast<REGION*>(spr);
        if (!(region->Z()+25.0f > z))
            continue;
        if (!(region->property & 8u) && !region->IsInsideXY(x,y))
            continue;

        for (int i=0;i<6;++i) {
            if (region->sourceVid[i]==oldVid)
                return region->conversionVid[i];
        }
    }
    return oldVid;
}

int SPRITE::IsLinked()
{
    return m_parent != 0;
}

int SPRITE::IsInside(float vx,float vy)
{
    if (!InSegment(vx,m_x,m_vid->m_snapOffsetX))
        return 0;

    const float top = m_y - m_z;
    if (top - m_vid->m_hitVerticalOffset - m_vid->m_snapOffsetY >= vy)
        return 0;
    if (top + m_vid->m_snapOffsetY < vy)
        return 0;
    return 1;
}

int REGION::IsInsideScr(float x0,float y0)
{
    if (!InSegment(x0,m_x,sizeX * 0.5f))
        return 0;

    const float top = m_y - m_z;
    const float halfHeight = sizeY * 0.5f;
    if (top - m_vid->m_hitVerticalOffset - halfHeight >= y0)
        return 0;
    if (top + halfHeight < y0)
        return 0;
    return 1;
}

int SPRITE::IsCommand(unsigned int command)
{
    return command == ((m_flag >> 2) & 0x1Fu);
}

ANGLE SPRITE::Direction() const
{
    return ANGLE(m_dir);
}

int SPRITE::NoRef() { return m_noRef; }

int VID::RealDirection(ANGLE direction)
{
    // Retail maps the 0..255 ANGLE domain to the available direction count.
    return (((static_cast<unsigned int>(direction.value) + m_editorDirectionOffset) & 0xFFu) * m_noDirections) >> 8;
}

ANGLE SPRITE::DirectionTo(float x,float y) const
{
    return ANGLE(x - m_x, y - m_y);
}

ANGLE SPRITE::DirectionTo(float x,float y,int* radius) const
{
    return ANGLE(x-m_x,y-m_y,radius);
}

float SPRITE::Speed() const
{
    return m_speed;
}

int SPRITE::Command()
{
    return static_cast<int>((m_flag>>2)&0x1Fu);
}

unsigned long SPRITE::FrameSpeed()
{
    const unsigned long* speeds = reinterpret_cast<const unsigned long*>(
        reinterpret_cast<const uint8_t*>(m_vid) + 0xFC);
    return speeds[m_ani];
}

void SPRITE::PrimitiveTact()
{
    if (CurrentTime - m_tactTime >= FrameSpeed()) {
        m_tactTime = CurrentTime;
        ++m_noCadr;
        if (m_noCadr > m_endCadr)
            m_noCadr = m_begCadr;
    }
}

EX_SPRITE_DATA::EX_SPRITE_DATA(const SPRITE* sprite)
{
    // GAMMA and LIST<int> default constructors run before this body, exactly as
    // the retail owner does at 0x0045A52D/0x0045A538.
    SPRITE* mutableSprite=const_cast<SPRITE*>(sprite);
    birthSmokeTimer=1;
    deathTimer=mutableSprite->Vid()->m_defaultDeathTimer;
    tableCoeff=0.0f;
    tableStartTime=CurrentTime;
    maxSpeed=mutableSprite->Vid()->m_defaultMaxSpeed;
    lastX=mutableSprite->X();
    lastY=mutableSprite->Y();
    lastZ=mutableSprite->Z();
    // changeCoorTime is intentionally not written: retail ctor leaves +0x0C untouched.
}

void SPRITE::SetGamma(const GAMMA* newGamma)
{
    if (!m_exData)
        m_exData=new EX_SPRITE_DATA(this);
    m_exData->gamma=newGamma;
}

void SPRITE::AddAction(int action,int var1,int var2,int var3)
{
    m_actions.InsertFirst(ACT(action,var1,var2,var3));
}

namespace {
int SpriteAbsInt(int value)
{
    return value < 0 ? -value : value;
}
}

int SPRITE::AskCycleAction(int action,int var1,int var2,int var3)
{
    (void)var3;
    const int count=m_actions.No();
    if (count != 0 && action == 0x21 &&
        fabsf(static_cast<float>(var1) - m_x) < 24.0f &&
        fabsf(static_cast<float>(var2) - m_y + m_z) < 16.0f)
        return 1;

    for (int i=1;i<count;++i) {
        ACT* item=m_actions[i];
        if ((item->act == 0x21 || item->act == 0x25) &&
            SpriteAbsInt(var1 - item->var1) < 24 &&
            SpriteAbsInt(var2 - item->var2 + item->var3) < 16)
            return 1;
    }
    return 0;
}

int SPRITE::AddCycleAction(int action,int var1,int var2,int var3)
{
    const int count=m_actions.No();
    if (count != 0 && m_actions[0]->act == 0x47)
        return 0;

    if (count != 0 && (action == 0x21 || action == 0x25) &&
        fabsf(static_cast<float>(var1) - m_x) < 24.0f &&
        fabsf(static_cast<float>(var2 - var3) - m_y + m_z) < 16.0f) {
        AddAction(action,static_cast<int>(m_x),static_cast<int>(m_y),static_cast<int>(m_z));
        AddAction(0x47,m_actions.No(),0,0);
        return 1;
    }

    for (int i=1;i<m_actions.No();++i) {
        ACT* item=m_actions[i];
        if ((item->act == 0x21 || item->act == 0x25) &&
            SpriteAbsInt(var1 - item->var1) < 24 &&
            SpriteAbsInt(var2 - var3 - item->var2 + item->var3) < 16) {
            AddAction(action,item->var1,item->var2,item->var3);
            AddAction(0x47,i + 1,0,0);
            return 1;
        }
    }

    AddAction(action,var1,var2,var3);
    return 0;
}

void SPRITE::Error(int type,const char* text,unsigned long err)
{
    const int index=m_vid ? m_vid->m_idx : -1;
    MYERROR::Error(::Error,"SPRITE %i",type,text,err,index);
}

int SPRITE::Release()
{
    --m_noRef;
    if (m_noRef>0)
        return m_noRef;
    if (m_noRef<0) {
        Error(4,"noRef at Release",static_cast<unsigned long>(m_noRef));
        return 0;
    }
    ScalarDeletingDestructor(1u);
    return 0;
}


void SPRITE::DrawSecondaryInfo()
{
    float y=Graph->ViewYMin();
    const int goalIndex=m_goal ? m_goal->m_vid->m_idx : 0;

    Graph->PrintfXY(
        Graph->ViewXMin()+22.0f,
        y,
        "Ref=%-3i cmd=%1i ani=%-2i hp=%-3i AT=%i goal=%-3i spd=%-3i,%-3i timer=%i ammo=%i mvE=%1u%1u %i,%i,%i",
        m_noRef,
        (m_flag>>2)&0x1F,
        m_ani,
        m_hp,
        m_unknown04,
        goalIndex,
        static_cast<int>(m_speed*1000.0f),
        static_cast<int>(m_unknown24*1000.0f),
        m_unknown50,
        Action(92,0,0,0),
        (m_flag>>14)&1,
        (m_flag>>15)&1,
        static_cast<int>(m_x),
        static_cast<int>(m_y),
        static_cast<int>(m_z));

    if (HaveLink()) {
        y+=12.0f;
        SPRITE* child=m_child;
        const int childGoalIndex=child->m_goal ? child->m_goal->m_vid->m_idx : 0;
        Graph->PrintfXY(
            Graph->ViewXMin()+22.0f,
            y,
            "Ref=%-3i cmd=%1i ani=%-2i hp=%-3i AT=%i goal=%-3i timer=%i ammo=%i",
            child->m_noRef,
            (child->m_flag>>2)&0x1F,
            child->m_ani,
            child->m_hp,
            child->m_unknown04,
            childGoalIndex,
            child->m_unknown50,
            child->Action(92,0,0,0));
    }

    if (m_actions.No()) {
        y+=12.0f;
        STRING text;
        STRING count=Printf("%i - ",m_actions.No());
        text+=count.m_buf;
        for (int i=m_actions.No()-1;i>=0;--i) {
            ACT* action=m_actions[i];
            STRING item=Printf("%i(%i,%i,%i) ",action->act,action->var1,action->var2,action->var3);
            text+=item.m_buf;
        }
        Graph->PutsXY(Graph->ViewXMin()+30.0f,y,text.m_buf,GRAPH::WHITE);
    }

    DrawGoalLine();
}

// Editor debug rectangle/color selection follows the retail IsSpriteClass /
// IsSpriteType / PropHash decision chain and deliberately ends in GRAPH::PrintfXY.

int ActionNeedSpriteInVar1(int act)
{
    return act==0x20 || act==0x22 || act==0x4A ||
           act==0x96 || act==0x97 || act==0x98 || act==0x4B;
}

int ActionReturnSprite(int act)
{
    return act==0x5A || act==0x9C || act==0x9B ||
           act==0x9A || act==0x65 || act==0x67;
}

// The retail owner is intentionally empty; preserving that is exact behavior,
// not a placeholder implementation.
void SPRITE::SetBestTarget(SPRITE* /*goal*/)
{
}

LIST<ACT>* SPRITE::ActionStack()
{
    return &m_actions;
}

int SPRITE::HaveItem(int item)
{
    if (!m_exData)
        return 0;
    for (int i=0;i<m_exData->items.m_no;++i) {
        if (m_exData->items.m_data[i]==item)
            return 1;
    }
    return 0;
}

void SPRITE::InsertItem(int item)
{
    if (!m_exData)
        m_exData=new EX_SPRITE_DATA(this);

    LIST<int>& items=m_exData->items;
    if (items.m_no>=items.m_max) {
        const int newMax=items.m_max*2+4;
        int* newData=static_cast<int*>(::operator new(static_cast<unsigned int>(newMax)*sizeof(int)));
        for (int i=0;i<items.m_no;++i)
            newData[i]=items.m_data[i];
        if (items.m_data)
            ::operator delete(items.m_data);
        items.m_data=newData;
        items.m_max=newMax;
    }
    items.m_data[items.m_no++]=item;
}

int SPRITE::InsertUniqueItem(int item)
{
    if (!m_exData)
        m_exData=new EX_SPRITE_DATA(this);
    return m_exData->items.InsertUnique(&item);
}

void SPRITE::ResetActionStack()
{
    for (int i=0;i<m_actions.No();++i) {
        ACT* action=m_actions[i];
        if ((action->act&0xFF)==0x4A && action->var1)
            reinterpret_cast<SPRITE*>(action->var1)->Release();
    }
    m_actions.Release();
}

void SPRITE::DrawRectangle()
{
    if (IsSpriteClass(23)) {
        DrawSecondaryInfo();
        return;
    }

    COLOR color;
    if (IsSpriteType(1) && m_vid->PropHash())
        color = &GRAPH::BLACK;
    else if (m_vid->m_layer == 11)
        color = &GRAPH::WHITE;
    else if (IsSpriteType(4))
        color = &GRAPH::GREEN;
    else if (IsSpriteType(8))
        color = &GRAPH::LIGHTBLUE;
    else if (IsSpriteType(2))
        color = &GRAPH::YELLOW;
    else if (IsSpriteType(0x20))
        color = &GRAPH::RED;
    else if (IsSpriteType(0x200))
        color = &GRAPH::GRAY;
    else if (IsSpriteClass(10))
        color = &GRAPH::WHITE;
    else
        color = &GRAPH::RED;

    Graph->Box(
        Map->ToScreenX(m_x - m_vid->m_snapOffsetX),
        Map->ToScreenY(m_y - m_z - m_vid->m_snapOffsetY),
        Map->ToScreenX(m_x + m_vid->m_snapOffsetX),
        Map->ToScreenY(m_y - m_z + m_vid->m_snapOffsetY),
        color);

    if (IsSpriteType(6)) {
        Graph->Line(
            ScreenX(),
            Map->ToScreenY(m_y - m_z - m_vid->m_hitVerticalOffset),
            ScreenX(),
            ScreenY(),
            GRAPH::BLUE);
    }

    Graph->PrintfXY(
        ScreenX() + 1.0f,
        ScreenY(),
        "%i",
        Vid()->m_idx);
}

int SPRITE::CurrentCadr()
{
    return m_noCadr;
}

int SPRITE::LastCadr()
{
    return m_endCadr;
}

void SPRITE::ChangeCoor(float newX,float newY,float newZ)
{
    const float dx=newX-m_x;
    const float dy=newY-m_y;
    const float dz=newZ-m_z;
    SPRITE* const root=this;

    for (SPRITE* sprite=this;sprite;sprite=sprite->m_child) {
        if (sprite->m_vid->m_nLinkDots != 0)
            sprite->m_vid->ResetGridZ(root);

        if (sprite->m_vid->PropHash())
            Hash->ChangeCoor(sprite,sprite->m_x+dx,sprite->m_y+dy);

        if (sprite->m_exData && sprite->m_exData->changeCoorTime != RealCurrentTime) {
            sprite->m_exData->changeCoorTime=RealCurrentTime;
            sprite->m_exData->lastX=sprite->m_x;
            sprite->m_exData->lastY=sprite->m_y;
            sprite->m_exData->lastZ=sprite->m_z;
        }

        sprite->m_x+=dx;
        sprite->m_y+=dy;
        sprite->m_z+=dz;

        if (sprite->m_vid->m_nLinkDots != 0)
            sprite->m_vid->SetGridZ(root);
    }
}

int SPRITE::IsDying() const
{
    return m_ani >= 15;
}

namespace {
const int* VidAnimationBegin(const VID* vid)
{
    return reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(vid)+0x2FC);
}
const int* VidAnimationDirectionFrames(const VID* vid)
{
    return reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(vid)+0x340);
}
const int* VidAnimationSpecial(const VID* vid)
{
    return reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(vid)+0x74);
}
int VidLinkedAnimationGuard(const VID* vid)
{
    return *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(vid)+0x40);
}
}

void SPRITE::ChangeAnimation(int newAnimation)
{
    if (newAnimation >= 17) {
        Error(4,"new_animation in ChangeAnimation",static_cast<unsigned long>(newAnimation));
        return;
    }

    if (HaveLink()) {
        SPRITE* child=m_child;
        const int guarded = VidLinkedAnimationGuard(child->m_vid) || child->m_vid->PropWind() ||
                            child->m_ani==8 || child->IsDying();
        if (!guarded || newAnimation==15 || newAnimation==16)
            child->ChangeAnimation(newAnimation);
    }

    if (m_ani==newAnimation)
        return;

    m_flag &= ~0x200u;

    ANGLE direction;
    if (m_unknown24!=0.0f && m_vid->PropVertDir()) {
        ANGLE currentDirection=Direction();
        const float x=currentDirection.Sin()*m_speed*1000000.0f;
        const float y=-(currentDirection.Cos()*m_speed+m_unknown24)*1000000.0f;
        direction=ANGLE(x,y);
    } else {
        direction=Direction();
    }

    const int directionIndex=m_vid->RealDirection(direction);
    const int framesPerDirection=VidAnimationDirectionFrames(m_vid)[newAnimation];
    m_begCadr=VidAnimationBegin(m_vid)[newAnimation] + directionIndex*framesPerDirection;
    if (newAnimation>=13 && VidAnimationSpecial(m_vid)[newAnimation]==0)
        m_endCadr=m_begCadr;
    else
        m_endCadr=m_begCadr+framesPerDirection-1;
    m_noCadr=m_begCadr;
    m_ani=newAnimation;
}

int VID::PropZeroZ()
{
    return static_cast<int>(m_flag & 0x200u);
}

int SPRITE::IsZCross(const VID* vid,float z)
{
    if (m_z + m_vid->m_hitVerticalOffset < z)
        return 0;
    if (z + vid->m_hitVerticalOffset < m_z)
        return 0;
    return 1;
}

int SPRITE::IsZCross(const SPRITE* sprite)
{
    return IsZCross(sprite->Vid(),sprite->Z());
}

int SPRITE::IsXYCross(const VID* vid,float x,float y)
{
    if (fabsf(m_x - x) > m_vid->m_snapOffsetX + vid->m_snapOffsetX)
        return 0;
    if (fabsf(m_y - y) > m_vid->m_snapOffsetY + vid->m_snapOffsetY)
        return 0;
    return 1;
}

int SPRITE::IsXYCross(const SPRITE* sprite)
{
    return IsXYCross(sprite->Vid(),sprite->X(),sprite->Y());
}

int SPRITE::IsCross(const VID* vid,float x,float y,float z)
{
    if (IsDying())
        return 0;
    if (!IsXYCross(vid,x,y))
        return 0;
    if (!IsZCross(vid,z))
        return 0;
    return 1;
}

// VID::DrawShadow is virtual in the retail VID hierarchy.  VID is represented
// by its raw debug metadata layout in this implementation, so preserve the exact
// vtable slot (+0x10) explicitly instead of statically binding the base owner.
void SPRITE::DrawShadow()
{
    void** vtable=*reinterpret_cast<void***>(m_vid);
    typedef void (__thiscall *Method)(VID*,const SPRITE*);
    reinterpret_cast<Method>(vtable[0x10/4])(m_vid,this);
}

int WEAPON::PropMoved() const
{
    return static_cast<int>(m_property&0x40u);
}

float WEAPON::Interpolate(float* data,float coeff)
{
    const int maxColumn=7;
    const int curColumn=static_cast<int>(coeff);
    if (curColumn>=maxColumn)
        return data[maxColumn];
    return data[curColumn]+(data[curColumn+1]-data[curColumn])*(coeff-static_cast<float>(curColumn));
}

int WEAPON::Interpolate(int* data,float coeff)
{
    const int maxColumn=7;
    const int curColumn=static_cast<int>(coeff);
    if (curColumn>=maxColumn)
        return data[maxColumn];
    return static_cast<int>(static_cast<float>(data[curColumn]) +
                            static_cast<float>(data[curColumn+1]-data[curColumn]) *
                            (coeff-static_cast<float>(curColumn)));
}

GAMMA SPRITE::GetGamma()
{
    const GAMMA* selected=0;
    if (m_exData && !m_exData->gamma.IsDefault())
        selected=&m_exData->gamma;
    else
        selected=reinterpret_cast<const GAMMA*>(reinterpret_cast<const unsigned char*>(m_vid)+0x3E8)+Army();

    GAMMA result(selected);
    if (m_vid->m_propertyBits&1u) {
        unsigned char* weapon=reinterpret_cast<unsigned char*>(m_vid->m_weapon);
        const float coeff=m_exData->tableCoeff;
        const int blue=m_vid->m_weapon->Interpolate(reinterpret_cast<int*>(weapon+0xA4),coeff);
        const int green=m_vid->m_weapon->Interpolate(reinterpret_cast<int*>(weapon+0x84),coeff);
        const int red=m_vid->m_weapon->Interpolate(reinterpret_cast<int*>(weapon+0x64),coeff);
        const int alpha=m_vid->m_weapon->Interpolate(reinterpret_cast<int*>(weapon+0xC4),coeff);
        const GAMMA animated(alpha,red,green,blue);
        result+=&animated;
    }
    return result;
}

SPRITE* SPRITE::CanPlace(float x,float y,float z)
{
    VID* vid=m_vid;
    if (vid->m_unknown18==0)
        return 0;

    if (vid->PropZeroZ()) {
        if ((Map->GetGroundZ(vid,x,y)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(vid,x,y)>vid->m_groundToleranceBelow))
            return Mouse;

        if (!IsSpriteClass(7)) {
            const float dx=vid->m_snapOffsetX-2.0f;
            const float dy=vid->m_snapOffsetY-2.0f;

            if ((Map->GetGroundZ(x-dx,y-dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x-dx,y-dy)>vid->m_groundToleranceBelow)) return Mouse;
            if ((Map->GetGroundZ(x-dx,y+dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x-dx,y+dy)>vid->m_groundToleranceBelow)) return Mouse;
            if ((Map->GetGroundZ(x+dx,y-dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x+dx,y-dy)>vid->m_groundToleranceBelow)) return Mouse;
            if ((Map->GetGroundZ(x+dx,y+dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x+dx,y+dy)>vid->m_groundToleranceBelow)) return Mouse;
        }
    } else {
        if (Map->GetGroundZ(vid,x,y)>z)
            return Mouse;

        if (!IsSpriteClass(7)) {
            const float dx=vid->m_snapOffsetX-2.0f;
            const float dy=vid->m_snapOffsetY-2.0f;
            if (Map->GetGroundZ(x-dx,y-dy)>z) return Mouse;
            if (Map->GetGroundZ(x-dx,y+dy)>z) return Mouse;
            if (Map->GetGroundZ(x+dx,y-dy)>z) return Mouse;
            if (Map->GetGroundZ(x+dx,y+dy)>z) return Mouse;
        }
    }

    SPRITE* other=Hash->FirstInBox(x-vid->m_snapOffsetX,y-vid->m_snapOffsetY,
                                   x+vid->m_snapOffsetX,y+vid->m_snapOffsetY);
    while (other) {
        if (other!=this && other->IsCross(vid,x,y,z) &&
            (other->Vid()->m_unknown18 & vid->m_unknown18)!=0)
            return other;
        other=Hash->NextInBox();
    }
    return 0;
}

SPRITE* SPRITE::CanPlaceWithCrush(float x,float y,float z)
{
    VID* const vid=m_vid;
    if (vid->m_unknown18==0)
        return 0;

    if (vid->PropZeroZ()) {
        if ((Map->GetGroundZ(vid,x,y)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(vid,x,y)>vid->m_groundToleranceBelow))
            return Mouse;

        if (!IsSpriteClass(7)) {
            const float dx=vid->m_snapOffsetX-2.0f;
            const float dy=vid->m_snapOffsetY-2.0f;
            if ((Map->GetGroundZ(x-dx,y-dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x-dx,y-dy)>vid->m_groundToleranceBelow)) return Mouse;
            if ((Map->GetGroundZ(x-dx,y+dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x-dx,y+dy)>vid->m_groundToleranceBelow)) return Mouse;
            if ((Map->GetGroundZ(x+dx,y-dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x+dx,y-dy)>vid->m_groundToleranceBelow)) return Mouse;
            if ((Map->GetGroundZ(x+dx,y+dy)-z>vid->m_groundToleranceAbove || z-Map->GetGroundZ(x+dx,y+dy)>vid->m_groundToleranceBelow)) return Mouse;
        }
    } else {
        if (Map->GetGroundZ(vid,x,y)>z)
            return Mouse;

        if (!IsSpriteClass(7)) {
            const float dx=vid->m_snapOffsetX-2.0f;
            const float dy=vid->m_snapOffsetY-2.0f;
            if (Map->GetGroundZ(x-dx,y-dy)>z) return Mouse;
            if (Map->GetGroundZ(x-dx,y+dy)>z) return Mouse;
            if (Map->GetGroundZ(x+dx,y-dy)>z) return Mouse;
            if (Map->GetGroundZ(x+dx,y+dy)>z) return Mouse;
        }
    }

    SPRITE* other=Hash->FirstInBox(x-vid->m_snapOffsetX,y-vid->m_snapOffsetY,
                                   x+vid->m_snapOffsetX,y+vid->m_snapOffsetY);
    while (other) {
        if (other!=this && other->IsCross(vid,x,y,z)) {
            if ((other->Vid()->m_unknown18 & vid->m_unknown18)!=0)
                return other;
            if (other->Vid()->PropCrush())
                other->Action(0x55,5,0,0);
        }
        other=Hash->NextInBox();
    }
    return 0;
}

namespace {
int g_canPlaceWithCrushAndGlideDepth=0;
}

SPRITE* SPRITE::CanPlaceWithCrushAndGlide(float* new_x,float* new_y,float* new_z)
{
    if (Vid()->m_unknown18==0)
        return 0;

    SPRITE* const s=CanPlaceWithCrush(*new_x,*new_y,*new_z);
    if (s) {
        const int contactFunction=*reinterpret_cast<const int*>(
            reinterpret_cast<const unsigned char*>(Vid())+0x454);
        if (contactFunction>=0) {
            if (Map->ScriptRun(contactFunction,this,s==Mouse ? 0 : s,0)) {
                *new_x=X();
                *new_y=Y();
                *new_z=Z();
                return 0;
            }
        }

        if (s!=Mouse && s->Vid()->m_weapon->PropMoved()) {
            float x=s->X()+*new_x-X();
            float y=s->Y()+*new_y-Y();
            float z=s->Z();
            ++g_canPlaceWithCrushAndGlideDepth;
            if (g_canPlaceWithCrushAndGlideDepth<5 &&
                !s->CanPlaceWithCrushAndGlide(&x,&y,&z)) {
                int moveOther=1;
                if (Vid()->m_weapon->PropMoved()) {
                    ANGLE limit(static_cast<uint8_t>(0x10));
                    ANGLE ownDirection=Direction();
                    ANGLE inverse=ownDirection.GetInversed();
                    ANGLE otherDirection=s->Direction();
                    ANGLE difference=otherDirection.Difference(&inverse);
                    if (difference.operator<(&limit)) {
                        ANGLE halfTurn(static_cast<uint8_t>(0x80));
                        ownDirection=Direction();
                        if (!ownDirection.operator<(&halfTurn))
                            moveOther=0;
                    }
                }
                if (moveOther)
                    s->ChangeCoor(x,y,z);
            }
            if (g_canPlaceWithCrushAndGlideDepth!=0)
                --g_canPlaceWithCrushAndGlideDepth;
        }

        if (!CanPlace(X(),*new_y,*new_z)) {
            *new_x=X();
        }
        else if (!CanPlace(*new_x,Y(),*new_z)) {
            *new_y=Y();
        }
        else {
            if (s!=Mouse && s->IsCross(Vid(),X(),Y(),Z())) {
                // The original keeps the requested coordinates when the two
                // sprites already intersect at the current position.
            }
            else {
                *new_x=X();
                *new_y=Y();
                *new_z=Z();
                return s;
            }
        }
    }

    if (Vid()->PropZeroZ())
        *new_z=Map->GetGroundZ(Vid(),*new_x,*new_y);
    return 0;
}

ANGLE SPRITE::GlideDirection(ANGLE direction)
{
    const float halfWidth=Vid()->m_footprintWidth/2.0f;
    const float halfHeight=Vid()->m_footprintHeight/2.0f;
    const float x=X();
    const float y=Y();
    const float z=Z();

    ANGLE a60(static_cast<uint8_t>(0x60));
    ANGLE aA0(static_cast<uint8_t>(0xA0));
    if (direction.operator>(&a60) && direction.operator<(&aA0)) {
        if (CanPlace(x-halfWidth,y+halfHeight,z) &&
            !CanPlace(x+halfWidth,y,z))
            return ANGLE(static_cast<uint8_t>(0x58));
        if (CanPlace(x+halfWidth,y+halfHeight,z) &&
            !CanPlace(x-halfWidth,y,z))
            return ANGLE(static_cast<uint8_t>(0xA8));
        return direction;
    }

    ANGLE aE0(static_cast<uint8_t>(0xE0));
    ANGLE a20(static_cast<uint8_t>(0x20));
    if (direction.operator>(&aE0) || direction.operator<(&a20)) {
        if (CanPlace(x-halfWidth,y-halfHeight,z) &&
            !CanPlace(x+halfWidth,y,z))
            return ANGLE(static_cast<uint8_t>(0x28));
        if (CanPlace(x+halfWidth,y-halfHeight,z) &&
            !CanPlace(x-halfWidth,y,z))
            return ANGLE(static_cast<uint8_t>(0xD8));
        return direction;
    }

    ANGLE aB0(static_cast<uint8_t>(0xB0));
    ANGLE aD0(static_cast<uint8_t>(0xD0));
    if (direction.operator>(&aB0) && direction.operator<(&aD0)) {
        if (CanPlace(x-halfWidth,y+halfHeight,z) &&
            !CanPlace(x,y-halfHeight,z))
            return ANGLE(static_cast<uint8_t>(0xD8));
        if (CanPlace(x-halfWidth,y-halfHeight,z) &&
            !CanPlace(x,y+halfHeight,z))
            return ANGLE(static_cast<uint8_t>(0xA8));
        return direction;
    }

    ANGLE a30(static_cast<uint8_t>(0x30));
    ANGLE a50(static_cast<uint8_t>(0x50));
    if (direction.operator>(&a30) && direction.operator<(&a50)) {
        if (CanPlace(x+halfWidth,y+halfHeight,z) &&
            !CanPlace(x,y-halfHeight,z))
            return ANGLE(static_cast<uint8_t>(0x28));
        if (CanPlace(x+halfWidth,y-halfHeight,z) &&
            !CanPlace(x,y+halfHeight,z))
            return ANGLE(static_cast<uint8_t>(0x58));
    }
    return direction;
}

void SPRITE::ChangeCoorForLinkRotate(float newX,float newY)
{
    const float dx=newX-X();
    const float dy=newY-Y();
    SPRITE* const root=this;

    for (SPRITE* sprite=this;sprite;sprite=sprite->m_child) {
        if (sprite->m_vid->PropNotChangeLinkerCoor())
            continue;

        if (sprite->m_vid->PropBuildSizeToGridZ() || sprite->m_vid->PropBuildVidZToGridZ())
            sprite->m_vid->ResetGridZ(root);

        if (sprite->m_vid->PropHash())
            Hash->ChangeCoor(sprite,sprite->m_x+dx,sprite->m_y+dy);

        if (sprite->m_exData && sprite->m_exData->changeCoorTime != RealCurrentTime) {
            sprite->m_exData->changeCoorTime=RealCurrentTime;
            sprite->m_exData->lastX=sprite->m_x;
            sprite->m_exData->lastY=sprite->m_y;
        }

        sprite->m_x+=dx;
        sprite->m_y+=dy;

        if (sprite->m_vid->PropBuildSizeToGridZ() || sprite->m_vid->PropBuildVidZToGridZ())
            sprite->m_vid->SetGridZ(root);
    }
}

void SPRITE::ChangeDirection1(ANGLE direction)
{
    ANGLE current=Direction();
    if (!(current != &direction))
        return;

    if (m_vid->m_noDirections != 1) {
        const int shiftCadr=m_noCadr-m_begCadr;
        const int* begin=VidAnimationBegin(m_vid);
        const int* dirFrames=VidAnimationDirectionFrames(m_vid);
        m_begCadr=begin[m_ani] + dirFrames[m_ani]*m_vid->RealDirection(direction);
        m_endCadr=m_begCadr+dirFrames[m_ani]-1;
        m_noCadr=m_begCadr+shiftCadr;
    }

    ANGLE* ownDirection=reinterpret_cast<ANGLE*>(&m_dir);
    *ownDirection=&direction;
}

SPRITE* LINKER::Parent()
{
    return parent;
}

void LINKER::LinkRotate(ANGLE newDirection)
{
    ANGLE delta=newDirection-&beginDirection;
    const float dx=delta.RotateX(linkX,linkY);
    const float dy=delta.RotateY(linkX,linkY);
    SPRITE* owner=parent ? parent : m_parent;
    ChangeCoorForLinkRotate(owner->X()+dx,owner->Y()+dy);
}

void SPRITE::ChangeDirection(ANGLE newDirection)
{
    ANGLE original=newDirection;

    if (m_unknown24 != 0.0f && m_vid->PropVertDir()) {
        const float x=newDirection.Sin()*m_speed*1000000.0f;
        const float y=-(newDirection.Cos()*m_speed+m_unknown24)*1000000.0f;
        ANGLE corrected(x,y);
        newDirection=&corrected;
    }

    ANGLE current=Direction();
    if (!(current != &newDirection))
        return;

    if (m_vid->m_noDirections != 1) {
        const int shiftCadr=m_noCadr-m_begCadr;
        const int* begin=VidAnimationBegin(m_vid);
        const int* dirFrames=VidAnimationDirectionFrames(m_vid);
        m_begCadr=begin[m_ani] + dirFrames[m_ani]*m_vid->RealDirection(newDirection);
        m_endCadr=m_begCadr+dirFrames[m_ani]-1;
        m_noCadr=m_begCadr+shiftCadr;
    }

    for (SPRITE* sprite=this;sprite->m_child;sprite=sprite->m_child) {
        SPRITE* child=sprite->m_child;

        if (child->IsSpriteClass(12)) {
            LINKER* linker=static_cast<LINKER*>(child);
            if (linker->Parent()==this) {
                if (!child->m_vid->PropNotChangeLinkerCoor()) {
                    ANGLE stepped=child->m_vid->SteppedDirection(newDirection);
                    linker->LinkRotate(stepped);
                }
                if (child->m_vid->m_childDirectionLock==0.0f)
                    child->ChangeDirection1(original);
            }
        } else {
            if (!child->m_vid->PropNotChangeLinkerCoor() &&
                sprite->m_vid->m_linkVid==child->m_vid &&
                (sprite->m_vid->m_linkOffsetX!=0.0f || sprite->m_vid->m_linkOffsetY!=0.0f)) {
                ANGLE stepped=sprite->m_vid->SteppedDirection(newDirection);
                child->ChangeCoorForLinkRotate(
                    sprite->X()+stepped.RotateX(sprite->m_vid->m_linkOffsetX,sprite->m_vid->m_linkOffsetY),
                    sprite->Y()+stepped.RotateY(sprite->m_vid->m_linkOffsetX,sprite->m_vid->m_linkOffsetY));
            }
            if (child->m_vid->m_childDirectionLock==0.0f)
                child->ChangeDirection1(original);
        }
    }

    ANGLE* ownDirection=reinterpret_cast<ANGLE*>(&m_dir);
    *ownDirection=&original;
}


float SPRITE::NearDistanceTo(float x1,float y1)
{
    return NearDistance(x1-m_x,y1-m_y);
}

// Retail nests the same 2-D NearDistance helper: NearDistance(NearDistance(dx,dy),dz).
float SPRITE::NearDistanceTo(float x1,float y1,float z1)
{
    return NearDistance(NearDistance(x1-m_x,y1-m_y),z1-m_z);
}

float SPRITE::NearDistanceTo(const SPRITE* sprite)
{
    return NearDistance(sprite->m_x-m_x,sprite->m_y-m_y);
}

// Keep the four retail boundary tests in their original order.
int SPRITE::MoveTactMapLimit(float new_x,float new_y)
{
    const int delta=static_cast<int>(CurrentTime-PrevCurrentTime);
    if (new_x<0.0f) {
        Rotate(ANGLE(static_cast<uint8_t>(64)),delta);
        return 1;
    }
    if (new_y<0.0f) {
        Rotate(ANGLE(static_cast<uint8_t>(128)),delta);
        return 1;
    }
    if (Map->m_w<=new_x) {
        Rotate(ANGLE(static_cast<uint8_t>(192)),delta);
        return 1;
    }
    if (Map->m_h<=new_y) {
        Rotate(ANGLE(static_cast<uint8_t>(0)),delta);
        return 1;
    }
    return 0;
}

void SPRITE::DrawActionStack()
{
    if (m_actions.No()==0)
        return;

    int i=m_actions.No();
    float xold=ScreenX();
    float yold=ScreenY();

    while (--i>=0) {
        ACT* action=m_actions[i];
        if (action->act==0x25) {
            const float x=Map->ToScreenX(static_cast<float>(action->var1));
            const float y=Map->ToScreenY(static_cast<float>(action->var2),
                                         static_cast<float>(action->var3));
            Graph->WuLine(xold,yold,x,y,GRAPH::BLUE);
            xold=x;
            yold=y;
        } else if (action->act==0x21) {
            const float x=Map->ToScreenX(static_cast<float>(action->var1));
            const float y=Map->ToScreenY(static_cast<float>(action->var2),
                                         static_cast<float>(action->var3));
            Graph->WuLine(xold,yold,x,y,GRAPH::GREEN);
            xold=x;
            yold=y;
        } else if (action->act==0x20) {
            if (action->var1!=0) {
                SPRITE* target=reinterpret_cast<SPRITE*>(action->var1);
                const float x=target->ScreenX();
                const float y=target->ScreenY();
                Graph->WuLine(xold,yold,x,y,GRAPH::RED);
                xold=x;
                yold=y;
            }
        } else if (action->act==0x2B) {
            Graph->Circle(Map->ToScreenX(static_cast<float>(action->var1)),
                          Map->ToScreenY(static_cast<float>(action->var2)),
                          static_cast<float>(action->var3),GRAPH::WHITE);
        } else if (action->act==0x0C) {
            Graph->PutcXY(xold+static_cast<float>(Random(10))-5.0f,
                          yold+static_cast<float>(Random(10))-5.0f,'L',GRAPH::WHITE);
        } else if (action->act==0x26) {
            Graph->PutcXY(xold+static_cast<float>(Random(10))-5.0f,
                          yold+static_cast<float>(Random(10))-5.0f,'R',GRAPH::WHITE);
        } else if (action->act==0x28) {
            Graph->PutcXY(xold+static_cast<float>(Random(10))-5.0f,
                          yold+static_cast<float>(Random(10))-5.0f,'P',GRAPH::WHITE);
        }
    }
}

void SPRITE::PlaySFX(int nsfx)
{
    if (m_vid==EmptyVid)
        Error(E_ERROR,"PlaySFX for EmptyVid",0);
    Sound->PlaySFXFromCoor(
        nsfx,
        ScreenX()-Graph->SizeX()/2.0f,
        ScreenY()-Graph->SizeY()/2.0f);
}

void SPRITE::Save(STREAM* res)
{
    if ((m_flag&0x100u)!=0)
        return;

    SPRITE* self=this;
    const int army=Army();
    res->Write(&self,4u);
    res->Write(&m_vid->m_idx,4u);
    res->Write(&m_x,4u);
    res->Write(&m_y,4u);
    res->Write(&m_z,4u);
    reinterpret_cast<ANGLE*>(&m_dir)->Write(res);
    res->Write(&army,4u);
}

int SPRITE::IsInvisible()
{
    return static_cast<int>((m_flag >> 16) & 1u);
}



void SPRITE::DrawGoalLine()
{
    if (m_goal) {
        Graph->Line(ScreenX(),ScreenY(),
                    m_goal->ScreenX(),m_goal->ScreenY(),GRAPH::GREEN);
    }
    if (m_child && m_child->m_goal) {
        Graph->Line(m_child->ScreenX(),m_child->ScreenY(),
                    m_child->m_goal->ScreenX(),m_child->m_goal->ScreenY(),GRAPH::RED);
    }
}

void SPRITE::Stop()
{
    const unsigned int command=m_flag&0x7Cu;
    if (command==0 || command==4)
        SetCommandWithoutLink(0,0);
    m_flag&=0xFFFF3F7Fu;
    m_unknown24=0.0f;
    const float deceleration=*reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(m_vid)+0x38);
    if (deceleration==999999.0f)
        m_speed=0.0f;
}



float SPRITE::MaxSpeed() const
{
    return m_exData ? m_exData->maxSpeed : m_vid->m_defaultMaxSpeed;
}

int SPRITE::IsMoveFinished()
{
    return ((m_flag>>14)&1u) && ((m_flag>>15)&1u);
}

int NearBetween(float x,float x1,float x2,float err)
{
    if (x1<x2)
        return x1-err<=x && x<=x2+err;
    return x2-err<=x && x<=x1+err;
}

int Between(float x,float x1,float x2)
{
    if (x1<x2)
        return x>=x1 && x<=x2;
    return x>=x2 && x<=x1;
}

int Between(int x,int x1,int x2)
{
    if (x1<x2) return x>=x1 && x<=x2;
    return x>=x2 && x<=x1;
}

ANGLE SPRITE::DirectionTo(const SPRITE* sprite) const
{
    return ANGLE(sprite->m_x-m_x,sprite->m_y-m_y);
}

// Despite its historical name this is the exact "link VID exists but the
// matching child has not yet been attached" predicate used by CreateLink.
int SPRITE::IsLinkDestroy()
{
    return m_vid->m_linkVid && !HaveLink();
}

void SPRITE::CreateLink()
{
    if (!IsLinkDestroy())
        return;

    VID* linkVid=m_vid->m_linkVid;
    if (linkVid->PropNotCreateAsChild())
        return;

    float dx;
    float dy;
    if (linkVid->PropNotChangeLinkerCoor()) {
        dx=m_vid->m_linkOffsetX;
        dy=-m_vid->m_linkOffsetY;
    }
    else {
        ANGLE direction=Direction();
        dx=direction.RotateX(m_vid->m_linkOffsetX,-m_vid->m_linkOffsetY);
        dy=direction.RotateY(m_vid->m_linkOffsetX,-m_vid->m_linkOffsetY);
    }

    SPRITE* child=Map->CreateSprite(
        linkVid,
        m_x+dx,
        m_y+dy,
        m_z+m_vid->m_linkOffsetZ,
        Direction(),
        0);
    AddLink(child);

    if (m_child)
        m_child->ChangeArmy(Army());
    else
        Error(3,"link",0);
}

float SPRITE::ZSpeed() const
{
    return m_unknown24;
}

float SPRITE::MaxZSpeed() const
{
    return m_vid->m_maxZSpeed;
}

ANGLE SPRITE::RotateToGoal(int deltaTime)
{
    ANGLE bias(static_cast<uint8_t>(m_speed < 0.0f ? 0x80 : 0));
    ANGLE direction=DirectionTo(Goal());
    ANGLE target=direction.operator+(&bias);
    return Rotate(target,deltaTime);
}

int SPRITE::CanAttackThisSprite(const SPRITE* sprite) const
{
    if (!sprite)
        return 0;
    if (CanFight() && m_vid->m_weapon &&
        (m_vid->m_weapon->m_attackMask & sprite->m_vid->m_unknown0C))
        return 1;
    if (HaveFightLink() && m_child->m_vid->m_weapon &&
        (m_child->m_vid->m_weapon->m_attackMask & sprite->m_vid->m_unknown0C))
        return 1;
    return 0;
}

int SPRITE::StartMove()
{
    if (MaxSpeed()==0.0f)
        return 0;

    if (m_goal) {
        if (m_x==m_goal->m_x && m_y==m_goal->m_y)
            return 0;

        int distance=0;
        if (m_vid->m_noDirections==1 || m_vid->PropRandBirth()) {
            ANGLE direction=DirectionTo(m_goal->m_x,m_goal->m_y,&distance);
            ChangeDirection(direction);
        }

        if (IsSpriteType(0x200u) || IsSpriteClass(5u)) {
            if (!distance) {
                DirectionTo(m_goal->m_x,m_goal->m_y,&distance);
                if (!distance)
                    return 0;
            }
            m_unknown24=m_vid->CalculateZSpeed(m_goal->m_z-m_z,static_cast<float>(distance));
        }
        else if (m_goal->m_z>m_z) {
            m_unknown24=MaxZSpeed();
        }
        else if (m_goal->m_z<m_z) {
            m_unknown24=-MaxZSpeed();
        }
        else {
            m_unknown24=0.0f;
        }
    }

    // Preserve the retail source/control-flow shape even though bit 7 is set
    // immediately before the test. VC6 emitted this branch in MapEdit.exe.
    m_flag&=~0x8000u;
    m_flag&=~0x4000u;
    m_flag|=0x80u;
    if (!(m_flag&0x80u) && !IsDying() && Animation()!=3) {
        if (m_speed==0.0f && !m_vid->m_noAnimCadr[3] &&
            !m_vid->m_aniChildVid[3] && !m_vid->m_aniSfx[3])
            ChangeAnimation(2);
        else
            ChangeAnimation(3);
    }

    if (m_vid->m_acceleration==999999.0f)
        m_speed=MaxSpeed();
    return 1;
}


// -----------------------------------------------------------------------------
// UNIT retail lifecycle/support ring, MapEdit.exe 0x00465470..0x00466934.
// -----------------------------------------------------------------------------

UNIT::UNIT(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : TERRAIN(vid,x,y,z,direction,parent),
      stateLeftHandMove(0),stateRightHandMove(0),ammo(0),number(-1),blocktick(0),behave(0)
{
    if (HaveFightLink())
        behave=m_child->m_vid->m_weapon->m_defaultBehave;
    else
        behave=m_vid->m_weapon->m_defaultBehave;
    ammo=MaxAmmo()<<6;
}

UNIT::~UNIT()
{
    Map->DeletePointerToSprite(this);
}

void UNIT::Draw()
{
    if (Map->OptDrawHpLines(this) && Army()<2 &&
        (!Vid()->PropInvisibleForEnemy() || Army()!=1)) {
        int notdepo;
        if (Vid()->m_idx!=102 && Vid()->m_idx!=104)
            notdepo=static_cast<int>(Map->Vid(0x1CB)->m_footprintWidth);
        else
            notdepo=0;

        const int hp=PercentHp();
        if (hp!=0) {
            VID* const vidHp=Map->Vid(notdepo ? 0x2CE : 0x25D);
            const float shiftX=ScreenX()+
                (static_cast<float>(notdepo)+vidHp->m_footprintWidth+1.0f)/2.0f-
                vidHp->m_footprintWidth/2.0f;
            const int frame=vidHp->m_noDirections-1-hp*static_cast<int>(vidHp->m_noDirections)/256;
            Graph->DrawVid(vidHp,frame,shiftX,ScreenY()+1900.0f,1896.0f);
        }

        if (Vid()->m_idx==102) {
            const int build=static_cast<DEPO*>(this)->GetBuildTimePercent(0);
            if (build!=0) {
                VID* const vidBuild=Map->Vid(0x25E);
                const float shiftX=ScreenX()+
                    (static_cast<float>(notdepo)+vidBuild->m_footprintWidth+1.0f)/2.0f-
                    vidBuild->m_footprintWidth/2.0f;
                const int frame=vidBuild->m_noDirections-1-build*static_cast<int>(vidBuild->m_noDirections)/256;
                Graph->DrawVid(vidBuild,frame,shiftX,ScreenY()+1902.0f,1893.0f);
            }
        } else {
            const int ammoPercent=PercentAmmo();
            if (ammoPercent!=0) {
                VID* const vidAmmo=Map->Vid(0x1CD);
                const float shiftX=ScreenX()+
                    (static_cast<float>(notdepo)+vidAmmo->m_footprintWidth+1.0f)/2.0f-
                    vidAmmo->m_footprintWidth/2.0f;
                const int frame=vidAmmo->m_noDirections-1-ammoPercent*static_cast<int>(vidAmmo->m_noDirections)/256;
                Graph->DrawVid(vidAmmo,frame,shiftX,ScreenY()+1900.0f,1893.0f);
            }
        }

        if (Army()==0 && number>0) {
            const float screenX=ScreenX();
            VID* const vidHp=Map->Vid(0x1CB);
            VID* const vidActive=Map->Vid(0x1CE);
            const float shiftX=screenX-
                (vidHp->m_footprintWidth+vidActive->m_footprintWidth+1.0f)/2.0f+
                vidHp->m_footprintWidth/2.0f-1.0f-5.0f;
            Graph->DrawVid(Map->Vid(8),number+'0',shiftX,ScreenY()+1899.0f,1893.0f);
        }

        if (Army()==0 && notdepo!=0 && IsSpriteClass(0x15u)) {
            VID* const vidActive=Map->Vid(0x1CE);
            const float shiftX=ScreenX()-
                (static_cast<float>(notdepo)+vidActive->m_footprintWidth+1.0f)/2.0f+
                static_cast<float>(notdepo/2)-1.0f;
            Graph->DrawVid(Map->Vid(0x1CB),GetActive()*2,
                           shiftX,ScreenY()+1898.0f,1893.0f);
        }
    }

    SPRITE::Draw();
}

void UNIT::MoveTact()
{
    if (!IsSpriteClass(2u) && !IsSpriteClass(4u)) {
        SPRITE::MoveTact();
        return;
    }

    float newX;
    float newY;
    float newZ;
    MoveTactCalcCoor(&newX,&newY,&newZ);

    if (blocktick!=0) {
        if (abs(blocktick)>Map->GetFPS())
            blocktick=(blocktick>0 ? Map->GetFPS() : -Map->GetFPS())/2;

        const ANGLE turn(static_cast<uint8_t>(blocktick>0 ? 0x40 : 0xC0));
        ANGLE direction=Direction();
        const ANGLE target=direction+&turn;
        Rotate(target,static_cast<int>(CurrentTime-PrevCurrentTime));
        if (blocktick<0)
            ++blocktick;
        else
            --blocktick;
    } else if (Goal() && Speed()!=0.0f) {
        const ANGLE backward(static_cast<uint8_t>(Speed()<0.0f ? 0x80 : 0x00));
        ANGLE toGoal=DirectionTo(Goal());
        const ANGLE wanted=toGoal+&backward;
        const ANGLE glide=GlideDirection(wanted);
        Rotate(glide,static_cast<int>(CurrentTime-PrevCurrentTime));
        if (IsMoveFinished())
            Stop();
    }

    if (X()!=newX || Y()!=newY || Z()!=newZ) {
        if (!CanPlaceWithCrushAndGlide(&newX,&newY,&newZ)) {
            MoveTactMapLimit(newX,newY);
            ChangeCoor(newX,newY,newZ);
        } else if (blocktick==0) {
            blocktick=(Random(1)!=0 ? Map->GetFPS() : -Map->GetFPS())/2;
        }
    }
}

int UNIT::Action(int act,int var1,int var2,int var3)
{
    switch (act) {
    case 0x5D: {
        if (MaxAmmo()==999999) {
            ammo=0x03D08FC0;
            return ammo/64;
        }
        ammo+=var1<<6;
        const int ret=ammo;
        if (ammo<0)
            ammo=0;
        if (godemode)
            ammo=MaxAmmo()<<6;
        return ret;
    }
    case 0x5C:
        return ammo/64;

    case 0x5F:
        behave=static_cast<unsigned long>(var1);
        if (Link())
            Link()->Action(act,var1,var2,var3);
        return 0;

    case 0x5E:
        return static_cast<int>(behave);

    case 0x82: {
        if (IsDying())
            return 0;

        if (!StateForward() && (Animation()==8 || Animation()==10) &&
            m_vid->m_noAnimCadr[10]!=0) {
            if (GetTimer()!=0) {
                if (Animation()!=10)
                    ChangeAnimation(10);
            } else if (Animation()==10) {
                ChangeAnimation(0);
            }
        } else if (Speed()!=0.0f) {
            if (Animation()!=2)
                ChangeAnimation(2);
        } else {
            ChangeAnimation(0);
        }

        if (IsCommand(3) && HaveFightLink() && Goal() && !Link()->Goal())
            Link()->SetCommand(3,Goal());
        if (IsCommand(4) && HaveFightLink() && Goal() && !Link()->Goal())
            Link()->SetCommand(4,Goal());

        int ret;
        if (HaveFightLink() || blocktick==0) {
            m_unknown04=static_cast<uint32_t>(AttackTact(DeltaTime()));
            ret=static_cast<int>(m_unknown04);
        } else {
            ret=2;
        }

        if (ret==7) {
            SetCommand(0,static_cast<SPRITE*>(0));
        } else if (ret==1) {
            if (MaxSpeed()==0.0f) {
                if (HaveFightLink()) {
                    if (Link()->Goal())
                        SetCommand(0,static_cast<SPRITE*>(0));
                } else if (Goal()) {
                    SetCommand(0,static_cast<SPRITE*>(0));
                }
            } else if (m_speed==0.0f) {
                StartMove();
            }

            if ((IsCommand(3) || IsCommand(4)) && HaveFightLink() && (behave&1u)) {
                if (Link()->GetTimer()!=0 || Random(3)==0) {
                    SPRITE* const enemy=SeekEnemy();
                    if (enemy)
                        Link()->SetCommand(4,enemy);
                }
            }
        } else if (ret==0) {
            if (m_speed!=0.0f) {
                if (HaveFightLink()) {
                    if (Goal()==Link()->Goal())
                        Stop();
                } else {
                    Stop();
                }
            }
        } else if (ret==2) {
            if ((behave&2u) && m_speed==0.0f)
                StartMove();
        } else if (ret==3) {
            SetCommand(0,static_cast<SPRITE*>(0));
        }

        if (ret==6 && (behave&1u)) {
            if (behave&2u) {
                SPRITE* const enemy=SeekEnemy();
                if (enemy)
                    SetCommand(4,enemy);
            } else if (HaveFightLink()) {
                SPRITE* const enemy=SeekEnemy();
                if (enemy)
                    Link()->SetCommand(4,enemy);
            }
        }

        if ((ret==2 || ret==5) && (behave&1u) &&
            (IsCommand(0) || IsCommand(1) || IsCommand(4))) {
            int seek=0;
            if (m_endCadr>m_begCadr) {
                seek=1;
            } else {
                const unsigned long timer=HaveFightLink() ? Link()->GetTimer() : GetTimer();
                if (timer!=0 || Random(10)==0)
                    seek=1;
            }
            if (seek) {
                SPRITE* const enemy=SeekEnemy();
                if (enemy)
                    SetCommand(4,enemy);
            }
        }

        if (IsCommand(0) && !Goal() && GetTimer()==0)
            Stop();
        return 0;
    }

    case 0x55: {
        const int ret=TERRAIN::Action(act,var1,var2,var3);
        if (var1>=0 && IsCommand(0) && MaxSpeed()!=0.0f) {
            ANGLE dir(static_cast<uint8_t>(Random(255)));
            Move(X()+dir.Sin()*64.0f,Y()-dir.Cos()*64.0f,Z());
        }
        return ret;
    }

    case 0x56:
        ammo=MaxAmmo()<<6;
        return TERRAIN::Action(act,var1,var2,var3);

    case 0x50:
        TERRAIN::Action(act,var1,var2,var3);
        reinterpret_cast<STREAM*>(var1)->Write(&behave,4u);
        return 0;

    case 0x51: {
        TERRAIN::Action(act,var1,var2,var3);
        STREAM* const stream=reinterpret_cast<STREAM*>(var1);
        stream->Read(&behave,4u);
        if (var2==11) {
            LIST<int> readItems;
            readItems.Read(stream);
            for (int i=0;i<readItems.No();++i)
                InsertItem(*readItems[i]);
        } else if (var2<11) {
            LIST<short> oldItems;
            oldItems.Read(stream);
            for (int i=0;i<oldItems.No();++i)
                InsertItem(static_cast<int>(*oldItems[i]));
        }
        return 0;
    }

    case 0xC8: {
        TERRAIN::Action(act,var1,var2,var3);
        STREAM* const stream=reinterpret_cast<STREAM*>(var1);
        if (var2<7) {
            int oldArmy=0;
            stream->Read(&oldArmy,1u);
            ChangeArmy(oldArmy);
        }
        stream->Read(&behave,1u);
        return 0;
    }

    case 0x61: {
        const int oldArmy=Army();
        ChangeArmy(var1);
        if (Army()!=oldArmy && Vid()->m_idx==104)
            Map->ScriptRun(g_unitArmyChangeScript,this,0,0);
        return 0;
    }

    default:
        return TERRAIN::Action(act,var1,var2,var3);
    }
}

void* UNIT::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->UNIT::~UNIT();
    if (flags&1u)
        operator delete(self);
    return self;
}

int SPRITE::GetFireDamage()
{
    int damage=0;
    for (SPRITE* sprite=this;sprite;sprite=sprite->m_child) {
        if (sprite->CanFight())
            damage+=sprite->Vid()->m_aniChildVid[8]->GetFireDamage();
    }
    return damage;
}

int SPRITE::MaxAmmo()
{
    return m_vid->GetMaxAmmo();
}

unsigned int SPRITE::StateForward()
{
    return (m_flag>>7)&1u;
}

int UNIT::AddAmmoTick(int tick)
{
    const int maximum=MaxAmmo()<<6;
    if (maximum!=0 && maximum>ammo) {
        if (tick!=0) {
            ammo+=maximum/tick;
            if (ammo>maximum)
                ammo=maximum;
        } else {
            ammo=maximum;
        }
        return 1;
    }
    return 0;
}

int UNIT::PercentAmmo()
{
    const int maximum=MaxAmmo();
    if (maximum!=0)
        return Ammo()*255/maximum;
    return 0;
}

void UNIT::InverseActive()
{
    if (IsSpriteClass(0x15u)) {
        behave ^= (Vid()->m_weapon->m_power!=0.0f) ? 2u : 1u;
    } else if (IsSpriteClass(0x18u)) {
        behave ^= 2u;
    }
}

int UNIT::GetActive()
{
    if (IsSpriteClass(0x15u)) {
        if (Vid()->m_weapon->m_power!=0.0f)
            return (behave&2u) ? 0 : 1;
        return (behave&1u) ? 0 : 1;
    }
    if (IsSpriteClass(0x18u))
        return (behave&2u) ? 2 : 3;
    return (behave&1u) ? 0 : 1;
}

int DEPO::GetBuildTimePercent(int unitnum)
{
    if (numunit<=unitnum)
        return 255;

    VID* const buildVid=Map->Vid(static_cast<int>(unitsToBuild[unitnum]));
    if (!buildVid)
        return 255;

    unsigned long curpercent;
    if (unitnum==curunit-1)
        curpercent=GetTimer()*255u;
    else
        curpercent=timers[unitnum]*255u;

    curpercent/=static_cast<unsigned int>(buildVid->GetBuildTime());
    curpercent/=Const->buildTimeScale;
    return static_cast<int>(curpercent);
}

void UNIT::DrawSecondaryInfo()
{
    SPRITE::DrawSecondaryInfo();
}

// -----------------------------------------------------------------------------
// MAN / CANNON derived factory owners.
// -----------------------------------------------------------------------------

MAN::MAN(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : UNIT(vid,x,y,z,direction,parent)
{
    InsertUniqueItem(0x105);
    for (int i=2;i<10;++i)
        weaponAmmo[i]=0;
}

MAN::~MAN()
{
}

void* MAN::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->MAN::~MAN();
    if (flags&1u)
        operator delete(self);
    return self;
}

void MAN::MoveTact()
{
    float new_x;
    float new_y;
    float new_z;
    MoveTactCalcCoor(&new_x,&new_y,&new_z);

    if ((X()!=new_x || Y()!=new_y) && Map->ValidateXY(new_x,new_y)) {
        if (!CanPlaceWithCrushAndGlide(&new_x,&new_y,&new_z))
            ChangeCoor(new_x,new_y,new_z);
    }

    if (Goal() && IsCommand(1)) {
        ANGLE bias(static_cast<uint8_t>(Speed()<0.0f ? 0x80 : 0));
        ANGLE direction=DirectionTo(Goal());
        ANGLE target=direction.operator+(&bias);
        ANGLE glide=GlideDirection(target);
        Rotate(glide,static_cast<int>(CurrentTime-PrevCurrentTime));
        if (IsMoveFinished() || IsXYCross(Goal()))
            Stop();
    }
}


int MAN::Action(int action,int var1,int var2,int var3)
{
    switch (action) {
    case 0x25:
        if (HaveFightLink() && !Link()->IsWeaponReload()) {
            const float screen_x=Map->ToScreenX(static_cast<float>(var1));
            const float screen_y=Map->ToScreenY(static_cast<float>(var2));
            float target_z;

            if (Graph->InViewPort(screen_x,screen_y)) {
                int pitch=0;
                unsigned short* const zbuf=Graph->LockZ(&pitch);
                const int sx=static_cast<int>(screen_x);
                const int sy=static_cast<int>(screen_y);
                const int zvalue=static_cast<int>(zbuf[sx+sy*pitch])/8;
                target_z=static_cast<float>(zvalue)-128.0f;
                if (Z()+70.0f<target_z)
                    target_z=Z()+50.0f;
            }
            else if (AttackedSpriteType()==8u) {
                target_z=Map->GetGroundZ(static_cast<float>(var1),static_cast<float>(var2)+80.0f)+80.0f;
            }
            else {
                target_z=Map->GetGroundZ(static_cast<float>(var1),static_cast<float>(var2))+19.0f;
                var2-=19;
            }

            Link()->SetCommand(4,static_cast<float>(var1),static_cast<float>(var2)+target_z,target_z);
        }
        return 0;

    case 0x82:
        if (IsDying())
            return 0;

        if (Goal() || (Link() && Link()->Goal()))
            m_unknown04=AttackTact(DeltaTime());

        if (Speed()!=0.0f) {
            if ((m_flag&0x80u) || Speed()>MaxSpeed()/4.0f) {
                if (!(Animation()==6 && Link() && Vid()->m_idx==9) && Animation()!=2)
                    ChangeAnimation(2);
                if (Link() && Link()->Animation()==0)
                    Link()->ChangeAnimation(2);
            }
            else if (Animation()!=1) {
                ChangeAnimation(1);
            }
        }
        else {
            if (Animation()!=0 && Animation()!=10)
                ChangeAnimation(0);
            if (Link() && Link()->Animation()==2)
                Link()->ChangeAnimation(0);
        }
        return 0;

    case 0x36:
        if (var1==0x12D || var1==0xEB) {
            InsertItem(var1);
        }
        else if (!InsertUniqueItem(var1) && var1>=0x104 && var1<=0x10D) {
            if (UNIT::Action(0x5C,0,0,0)!=0) {
                const int weapon=var1-0x104;
                const int currentWeapon=Vid()->m_linkVid->m_idx-10;
                if (weapon<=currentWeapon && var1!=0x104)
                    return 0;
            }
            ChangeWeapon(var1-0x104);
        }
        return 0;

    case 0x5C:
        if (var1==0 || var1==Vid()->m_linkVid->m_idx-10)
            return UNIT::Action(0x5C,0,0,0);
        return weaponAmmo[var1];

    case 0x5D:
        if (var2>9)
            return 0;
        if (var2==0 || var2==Vid()->m_linkVid->m_idx-10)
            return UNIT::Action(0x5D,var1,0,0);
        weaponAmmo[var2]+=var1;
        return weaponAmmo[var2];

    case 0x55:
        if (var1>0 && Vid()->m_idx!=0x15E && Vid()->m_idx!=0x7C3) {
            SPRITE* armor=Link();
            while (armor) {
                const int armorVid=armor->Vid()->m_idx;
                if (armorVid==0xCB || armorVid==0xB5)
                    return 0;
                armor=armor->Link();
            }

            armor=Link();
            if (armor->Hp()>0) {
                int n_armor=0;
                if (armor->Hp()<100)
                    n_armor=0;
                else if (armor->Hp()<150)
                    n_armor=1;
                else
                    n_armor=2;

                static const int armorPercent[3]={50,70,90};
                const int damage=(var1*armorPercent[n_armor]+50)/100;
                if (damage>armor->Hp()) {
                    if (Map->Vid(11)->m_exchangeVid->m_idx!=11) {
                        const int vid_link=Vid()->m_linkVid->m_idx;
                        for (int i=10;i<=20;++i)
                            Map->ExchangeVid(Map->Vid(i),Map->Vid(i+0x21C));
                        Link()->Action(0x3E,vid_link,0,0);
                    }
                    armor->m_hp=0;
                }
                else {
                    armor->m_hp-=damage;
                }
                var1-=var1*armorPercent[n_armor]/100;
            }
        }

        if (var1>=Hp()) {
            if (this->Action(0x38,0xE6,0,0)) {
                this->Action(0x37,0xE6,0,0);
                ChangeHp(MaxHp());
                Map->CreateSprite(Map->Vid(0xB5),X(),Y(),Z()+22.0f,ANGLE(static_cast<uint8_t>(0)),this);
                return 0;
            }
        }
        return SPRITE::Action(action,var1,var2,var3);

    case 0x3E:
        if (Vid()->m_idx<20) {
            const int ammo=UNIT::Action(0x5C,0,0,0);
            const int linkIdx=Vid()->m_linkVid->m_idx;
            weaponAmmo[linkIdx-10]=ammo;
        }

        UNIT::Action(action,var1,var2,var3);

        if (Vid()->m_idx>20) {
            UNIT::Action(0x5D,Vid()->GetMaxAmmo()-UNIT::Action(0x5C,0,0,0),0,0);
        }
        else {
            const int linkIdx=Vid()->m_linkVid->m_idx;
            UNIT::Action(0x5D,weaponAmmo[linkIdx-10]-UNIT::Action(0x5C,0,0,0),0,0);
        }
        return 0;

    default:
        return UNIT::Action(action,var1,var2,var3);
    }
}

int MAN::ChangeWeapon(int weapon)
{
    if (Vid()->m_linkVid->m_idx>20)
        return 0;

    if (weapon==10)
        weapon=0;

    if (!HaveItem(weapon+0x104) || !HaveLink())
        return 0;

    const int currentWeapon=Vid()->m_linkVid->m_idx-10;
    weaponAmmo[currentWeapon]=UNIT::Action(0x5C,0,0,0);

    Link()->Action(0x3E,weapon+10,0,0);
    Vid()->m_linkVid=Map->Vid(weapon+10);

    const int currentAmmo=UNIT::Action(0x5C,0,0,0);
    UNIT::Action(0x5D,weaponAmmo[weapon]-currentAmmo,0,0);
    return 1;
}

CANNON::CANNON(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : SPRITE(vid,x,y,z,direction,parent)
{
    stateMissileStart |= 1u;

    if (Vid()->PropRandZSpeed() && MaxZSpeed()!=0.0f) {
        if (Random(1))
            m_unknown24=Random(MaxZSpeed());
        else
            m_unknown24=-Random(MaxZSpeed());
    }
    else if (parent && parent->IsDying() && Vid()->m_groundOffset<0.0f) {
        m_unknown24=MaxZSpeed();
        m_speed=MaxSpeed()+parent->Speed();
    }
    else {
        m_unknown24=MaxZSpeed();
    }

    if (Vid()->PropRandSpeed() && Vid()->m_defaultMaxSpeed!=0.0f)
        ExData()->maxSpeed=Random(Vid()->m_defaultMaxSpeed);

    StartMove();
}

CANNON::~CANNON()
{
}

void* CANNON::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->CANNON::~CANNON();
    if (flags&1u)
        operator delete(self);
    return self;
}

void CANNON::MoveTact()
{
    if (!Vid()->m_moveTactData)
        return;

    float new_x;
    float new_y;
    float new_z;
    MoveTactCalcCoor(&new_x,&new_y,&new_z);

    if ((m_flag&0x400u) && !IsDying())
        ChangeAnimation(15);

    const float old_ground_z=Map->GetGroundZ(X(),Y());
    const float ground_z=Map->GetGroundZ(new_x,new_y);
    const float top_z=ground_z+Vid()->m_groundOffset;
    const float old_z=Z();

    // Projectile crosses the terrain surface from the upper side.
    if (new_z<=ground_z && Z()>=old_ground_z) {
        new_z=Z();
        if (IsDying()) {
            Stop();
            if (ground_z>old_ground_z)
                CreateChildAndPlaySFX(11);
        }
        else if (!Vid()->PropBounce()) {
            Stop();
            new_z=old_ground_z;
            if (ground_z>old_ground_z)
                CreateChildAndPlaySFX(11);
            else
                CreateChildAndPlaySFX(12);
            ChangeAnimation(15);
        }
        else if (ground_z>old_ground_z) {
            ChangeAnimation(11);
            ANGLE direction=Direction();
            ChangeDirection(direction.GetInversed());
        }
        else if (m_unknown24<-0.022f) {
            ChangeAnimation(12);
            m_unknown24=-m_unknown24/2.0f;
        }
        else if (m_unknown24>0.005f) {
            m_unknown24/=2.0f;
            m_speed/=2.0f;
            ANGLE direction=Direction();
            ChangeDirection(direction.GetInversed());
        }
        else {
            m_unknown24=0.0f;
            ChangeAnimation(15);
        }

        new_x=X();
        new_y=Y();
    }
    else if (Z()!=new_z) {
        if (!Vid()->PropGravity() && top_z!=0.0f) {
            if (Z()<top_z) {
                if (new_z>=top_z) {
                    new_z=top_z;
                    m_unknown24=0.0f;
                }
            }
            else if (Z()>top_z) {
                if (new_z<top_z) {
                    new_z=top_z;
                    m_unknown24=0.0f;
                }
            }
            else if (!Vid()->PropSelfMoving()) {
                m_unknown24=0.0f;
            }
        }
    }

    if (X()!=new_x || Y()!=new_y) {
        if (CanPlaceWithCrush(new_x,new_y,new_z)) {
            if (!IsDying())
                ChangeAnimation(15);
        }
        else if (Map->ValidateXY(new_x,new_y)) {
            ChangeCoor(new_x,new_y,new_z);
            if (!IsDying() && !Vid()->PropGravity()) {
                if (X()<-220.0f || Y()<-200.0f ||
                    X()>Map->SizeX()+220.0f || Y()>Map->SizeY()+200.0f)
                    ChangeAnimation(15);
            }
        }
        else {
            ChangeCoor(new_x,new_y,new_z);
        }
    }

    if (Z()!=new_z)
        ChangeZCoor(new_z);

    SPRITE* const goal=Goal();
    if (goal) {
        const float size_to_goal=NearDistanceTo(goal);
        if (Vid()->PropSelfMoving()) {
            if (stateMissileStart&1u) {
                const float current_ground_z=Map->GetGroundZ(X(),Y());
                if ((Z()<current_ground_z+Vid()->m_groundOffset || Z()<goal->Z()) &&
                    m_unknown24>0.0f) {
                    m_unknown24-=static_cast<float>(CurrentTime-PrevCurrentTime)*Const->gravity;
                }
                else {
                    stateMissileStart&=~1u;
                }
            }
            else {
                ANGLE delta_dir=RotateToGoal(static_cast<int>(CurrentTime-PrevCurrentTime));
                ANGLE turnLimit(static_cast<uint8_t>(0x46));
                if (size_to_goal>=10.0f && size_to_goal<30.0f && delta_dir.operator>(&turnLimit)) {
                    stateMissileStart|=1u;
                    StartMove();
                }
                else if (goal->Z()<Z() && size_to_goal!=0.0f) {
                    m_unknown24=(goal->Z()-Z())/size_to_goal/10.0f;
                    if (m_unknown24<-Vid()->m_maxZSpeed)
                        m_unknown24=-Vid()->m_maxZSpeed;
                }
                else {
                    m_unknown24=0.0f;
                }
            }
        }
        else if (size_to_goal>100.0f) {
            ANGLE old_direct=Direction();
            RotateToGoal(static_cast<int>(CurrentTime-PrevCurrentTime));
            ANGLE current_direct=Direction();
            ANGLE difference=old_direct.Difference(&current_direct);
            ANGLE stopTurn(static_cast<uint8_t>(0x64));
            if (difference.operator>(&stopTurn)) {
                Stop();
                ChangeAnimation(15);
            }
        }

        if (IsMoveFinished() || IsXYCross(goal)) {
            if (IsZCross(goal) || Between(goal->Z(),old_z,Z())) {
                Stop();
                ChangeAnimation(15);
            }
        }

        if (Vid()->m_idx==0x13B && IsMoveFinished() && fabsf(goal->Z()-Z())<20.0f) {
            Stop();
            ChangeAnimation(15);
        }

        if (Vid()->PropSelfMoving() &&
            fabsf(goal->Z()-Z())<20.0f &&
            fabsf(goal->X()-X())<10.0f &&
            fabsf(goal->Y()-Y())<10.0f) {
            Stop();
            ChangeAnimation(15);
        }
    }
    else if (Vid()->m_groundOffset<0.0f && m_unknown24<0.0f) {
        ANGLE turn(static_cast<uint8_t>(0x20));
        ANGLE direction=Direction();
        ANGLE target=direction.operator+(&turn);
        Rotate(target,static_cast<int>(CurrentTime-PrevCurrentTime));
    }
}

int CANNON::Action(int act,int var1,int var2,int var3)
{
    if (act!=130)
        return SPRITE::Action(act,var1,var2,var3);

    if (Animation()==8) {
        ChangeAnimation(15);
        return 0;
    }

    if (IsDying())
        return 0;

    // The two literals are read directly from the original .rdata image:
    // 0x004B56F4 = -100.0f, 0x004B5104 = 0.0f.
    if (Z() < -100.0f) {
        ChangeAnimation(16);
    }
    else if (Speed()!=0.0f) {
        ChangeAnimation(2);
    }
    else if (Animation()>=7 && Animation()!=10) {
        ChangeAnimation(0);
    }
    return 0;
}

void CANNON::DeletePointerToSprite(SPRITE* sprite)
{
    if (sprite && Goal()==sprite) {
        SPRITE* const marker=new SPRITE(
            EmptyVid,
            sprite->X(),
            sprite->Y(),
            sprite->Z(),
            ANGLE(static_cast<uint8_t>(0)),
            0);
        SetGoal(marker);
    }
    SPRITE::DeletePointerToSprite(sprite);
}

int UNIT::GetNumber()
{
    return number;
}

void UNIT::SetNumber(int num)
{
    number=num;
}

// VC6 implements signed /64 with the explicit sign-bias sequence seen in reference implementation.
int UNIT::Ammo()
{
    return ammo / 64;
}

unsigned long SPRITE::AttackedSpriteType()
{
    if (HaveFightLink())
        return m_child->m_vid->m_weapon->m_attackMask;
    return m_vid->m_weapon->m_attackMask;
}

ENGINE* ENGINE::NextEngine()
{
    return *reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(this)+0x98);
}

ENGINE* ENGINE::FirstEngine()
{
    ENGINE* engine=this;
    while (*reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(engine)+0x94))
        engine=*reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(engine)+0x94);
    return engine;
}

int ENGINE::IsSelfMoving()
{
    const SPRITE* const sprite=reinterpret_cast<const SPRITE*>(this);
    return sprite->Vid()->m_weapon->m_power!=0.0f;
}

int ENGINE::IsPowerEngine()
{
    const SPRITE* const sprite=reinterpret_cast<const SPRITE*>(this);
    return sprite->Vid()->m_weapon->m_power-sprite->Vid()->m_weapon->m_weight>1.0f;
}

ENGINE* ENGINE::GetTrain()
{
    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        if (engine->IsPowerEngine())
            return engine;
    }
    return 0;
}

ENGINE* ENGINE::GetChainEngine(int n)
{
    int index=0;
    ENGINE* engine=FirstEngine();
    while (index<n && engine) {
        ++index;
        engine=engine->NextEngine();
    }
    return engine;
}

ENGINE* ENGINE::GetRepair()
{
    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        const SPRITE* const sprite=reinterpret_cast<const SPRITE*>(engine);
        if (sprite->Vid()->m_idx==0x55)
            return engine;
    }
    return 0;
}

void ENGINE::InverseTrainActive()
{
    const unsigned int activeMask=IsPowerEngine() ? 2u : 1u;
    const int newActive=((*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(this)+0x8C) & activeMask)==0u);

    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        unsigned int& flags=*reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(engine)+0x8C);
        const unsigned int mask=engine->IsPowerEngine() ? 2u : 1u;
        if (newActive)
            flags|=mask;
        else
            flags&=~mask;
    }
}

void ENGINE::Move(float xx,float yy,float zz,int /*frompatrol*/,int fromclick)
{
    R_DOT* dot;
    if (fromclick)
        dot=RailMap.GetNearestDot(static_cast<int>(xx),static_cast<int>(yy)-static_cast<int>(zz));
    else
        dot=RailMap.GetNearestDot(static_cast<int>(xx),static_cast<int>(yy),static_cast<int>(zz));
    SetCommandToTrain(23,0,dot,0);
}

void ENGINE::MoveToRepair()
{
    SPRITE* const self=reinterpret_cast<SPRITE*>(this);
    SPRITE* repair=Map->FindNearestSprite(0x50868,self->X(),self->Y(),15000.0f,0);
    if (!repair)
        repair=Map->FindNearestSprite(0x50869,self->X(),self->Y(),15000.0f,0);
    if (repair)
        Move(repair->X(),repair->Y(),repair->Z(),0,0);
}

void ENGINE::ReverseTrain()
{
    ENGINE* const first=FirstEngine();
    ENGINE* current=first;
    while (current) {
        unsigned char* const raw=reinterpret_cast<unsigned char*>(current);
        ENGINE* const next=*reinterpret_cast<ENGINE**>(raw+0x98);

        *reinterpret_cast<ENGINE**>(raw+0x98)=*reinterpret_cast<ENGINE**>(raw+0x94);
        *reinterpret_cast<ENGINE**>(raw+0x94)=next;

        unsigned char temp[16];
        memcpy(temp,raw+0xB8,16u);
        memcpy(raw+0xB8,raw+0xC8,16u);
        memcpy(raw+0xC8,temp,16u);

        unsigned int& directionFlags=*reinterpret_cast<unsigned int*>(raw+0x90);
        directionFlags=(directionFlags&~1u)|((directionFlags&1u)^1u);

        if (!next) {
            if (current!=first) {
                const unsigned char* const firstRaw=reinterpret_cast<const unsigned char*>(first);
                unsigned int& spriteFlags=*reinterpret_cast<unsigned int*>(raw+0x28);
                const unsigned int firstFlags=*reinterpret_cast<const unsigned int*>(firstRaw+0x28);
                spriteFlags=(spriteFlags&~0x80u)|(firstFlags&0x80u);
                *reinterpret_cast<unsigned int*>(raw+0x20)=*reinterpret_cast<const unsigned int*>(firstRaw+0x20);
                *reinterpret_cast<float*>(raw+0xB0)=-*reinterpret_cast<const float*>(firstRaw+0xB0);
                *reinterpret_cast<unsigned int*>(raw+0xAC)=*reinterpret_cast<const unsigned int*>(firstRaw+0xAC);
                const unsigned int copySize=*reinterpret_cast<const unsigned int*>(firstRaw+0xABC);
                memcpy(raw+0xF8,firstRaw+0xF8,copySize);
                *reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(first)+0xABC)=0u;
            } else {
                *reinterpret_cast<float*>(raw+0xB0)=-*reinterpret_cast<float*>(raw+0xB0);
            }
        }

        current=next;
    }
}

int ENGINE::IsCommandToAllTrain()
{
    SPRITE* sprite=reinterpret_cast<SPRITE*>(this);
    if (sprite->m_vid->m_idx==45)
        return 1;
    return sprite->m_vid->m_weapon->m_power==0.0f;
}


int ENGINE::IsFirst()
{
    return *reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(this)+0x94)==0;
}

int ENGINE::InTrain(const SPRITE* sprite)
{
    if (!sprite)
        return 0;
    ENGINE* p=this;
    while (p) {
        if (reinterpret_cast<const SPRITE*>(p)==sprite)
            return 1;
        p=*reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(p)+0x98);
    }
    p=*reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(this)+0x94);
    while (p) {
        if (reinterpret_cast<const SPRITE*>(p)==sprite)
            return 1;
        p=*reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(p)+0x94);
    }
    return 0;
}


// Original static owner at 0x00618518.  The implemented image gets its own
// relocated storage; the C++ lifetime is preserved for both VS2022 and VC6 lanes.
SPRITE_LIST ENGINE::PathDots;

void ENGINE::ReleasePathDots()
{
    PathDots.DeleteAll();
}

namespace {
ENGINE*& EngineNext(ENGINE* e)
{
    return *reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(e)+0x98);
}
R_DOT*& EngineCommandDot(ENGINE* e)
{
    return *reinterpret_cast<R_DOT**>(reinterpret_cast<unsigned char*>(e)+0xA0);
}
R_DOT*& EngineCommandPathDot(ENGINE* e)
{
    return *reinterpret_cast<R_DOT**>(reinterpret_cast<unsigned char*>(e)+0xA4);
}
R_DOT*& EngineSecondaryCommandDot(ENGINE* e)
{
    return *reinterpret_cast<R_DOT**>(reinterpret_cast<unsigned char*>(e)+0xA8);
}
int& EngineAcceleration(ENGINE* e)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(e)+0xAC);
}
float& EngineMaxSpeed(ENGINE* e)
{
    return *reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(e)+0xB0);
}
SPRITE* EngineAsSprite(ENGINE* e)
{
    return reinterpret_cast<SPRITE*>(e);
}
}

void ENGINE::Stop()
{
    ENGINE* p=this;
    for (;;) {
        if (p->InTrain(Map->Flagman()))
            PathDots.DeleteAll();
        if (p->IsFirst())
            break;
        p=p->FirstEngine();
    }

    SPRITE* ps=EngineAsSprite(p);
    const unsigned int flag=ps->m_flag;
    if ((flag&0x7Cu)==0x64u) {
        if (!(flag&0x80u))
            return;
        EngineCommandDot(p)=EngineCommandPathDot(p);
        ps->m_flag=flag|0x80u;
        ENGINE* e=EngineNext(p);
        EngineCommandPathDot(p)=EngineSecondaryCommandDot(p);
        EngineSecondaryCommandDot(p)=EngineCommandDot(p);
        EngineMaxSpeed(p)=0.0f;
        EngineAcceleration(p)=0;
        for (;e;e=EngineNext(e)) {
            EngineCommandDot(e)=EngineCommandDot(p);
            EngineSecondaryCommandDot(e)=EngineSecondaryCommandDot(p);
            EngineCommandPathDot(e)=EngineCommandPathDot(p);
            EngineAsSprite(e)->m_flag|=0x80u;
            EngineMaxSpeed(e)=0.0f;
            EngineAcceleration(e)=0;
        }
    }
    else {
        for (ENGINE* e=p;e;e=EngineNext(e)) {
            EngineAsSprite(e)->m_flag&=~0x80u;
            EngineMaxSpeed(e)=0.0f;
            if (ps->m_speed!=0.0f)
                EngineAcceleration(e)=-((abs(static_cast<int>(ps->m_speed*1000.0f))+10)/2);
        }
    }
}

int SPRITE::CanShotEnemy(SPRITE*)
{
    return 1;
}

int SPRITE::EnemyRating()
{
    if (HaveFightLink())
        return m_child->m_vid->m_weapon->m_enemyRating;
    return m_vid->m_weapon->m_enemyRating;
}

int SPRITE::IsWeaponReload()
{
    if (m_unknown50>5000u)
        return 1;
    if (Animation()==8 && m_noCadr<=m_endCadr)
        return 1;
    return 0;
}

ANGLE SPRITE::DirectionTo(const SPRITE* sprite,int* radius) const
{
    return ANGLE(sprite->m_x-m_x,sprite->m_y-m_y,radius);
}

int SPRITE::IsBetterEnemy(float size,float minsize,SPRITE* sprite,SPRITE* mins)
{
    if (!mins)
        return 1;

    if (m_vid->m_weapon->PropAttackNearOnly())
        return minsize>size;

    SPRITE* forced=m_ptrSprite.sprite;
    if (forced) {
        if (mins==forced)
            return 0;
        if (sprite==forced)
            return 1;
    }

    // Preserve the two retail tests literally; the second is redundant in the
    // original executable but is part of the observed CFG.
    if (sprite->Army()==2 && mins->Army()!=2)
        return 0;
    if (mins->Army()!=2 && sprite->Army()==2)
        return 1;

    if (mins->IsSpriteType(8) && !sprite->IsSpriteType(8))
        return 0;
    if (!mins->IsSpriteType(8) && sprite->IsSpriteType(8))
        return 1;

    const float range=BattleRange();
    if (minsize>range && size<=range)
        return 1;

    int spriteSeek=0;
    int minSeek=0;
    if (sprite->IsSpriteType(4))
        spriteSeek=sprite->Action(92,0,0,0);
    if (mins->IsSpriteType(4))
        minSeek=mins->Action(92,0,0,0);
    if (!minSeek && spriteSeek)
        return 1;
    if (minSeek && !spriteSeek)
        return 0;

    const int spriteRating=sprite->EnemyRating();
    const int minRating=mins->EnemyRating();
    if (spriteRating>minRating)
        return 1;
    if (spriteRating<minRating)
        return 0;
    return minsize>size;
}

SPRITE* SPRITE::SeekEnemy()
{
    SPRITE* best=0;
    if (HaveFightLink())
        return m_child->SeekEnemy();

    WEAPON* const weapon=m_vid->m_weapon;
    const float detectRange=weapon->m_detectRange;
    const float battleRange=weapon->m_battleRange;
    const float deadZone=weapon->m_deadZone;
    const unsigned int attackMask=weapon->m_attackMask;
    const int frontEye=weapon->PropFrontEye();
    const int randomTarget=weapon->PropRandomTarget();

    if (detectRange==0.0f || attackMask==0u)
        return 0;

    float bestDistance=detectRange+1.0f;
    SPRITE* sprite=Hash->FirstUnit();
    while (sprite) {
        if (!sprite->IsSpriteType(attackMask)) {
            sprite=Hash->NextUnit();
            continue;
        }
        if (sprite->m_vid->IsInvulnerable()) {
            sprite=Hash->NextUnit();
            continue;
        }
        if (!IsEnemy(sprite) && !weapon->PropAttackAnyArmy()) {
            sprite=Hash->NextUnit();
            continue;
        }
        if (sprite->m_vid->PropInvisibleForEnemy()) {
            sprite=Hash->NextUnit();
            continue;
        }

        if (sprite->Army()==2 && !weapon->PropAttackAnyArmy()) {
            if (!Map->OptEnemyAttackNeutralTrains() || Army()!=1 || sprite->Army()!=2 || !sprite->IsSpriteClass(0x15)) {
                sprite=Hash->NextUnit();
                continue;
            }
        }

        if (sprite->IsSpriteType(8) && sprite->m_parent) {
            sprite=Hash->NextUnit();
            continue;
        }

        if (frontEye) {
            ANGLE limit(static_cast<uint8_t>(32));
            ANGLE direction=DirectionTo(sprite);
            ANGLE current=Direction();
            ANGLE difference=direction.Difference(&current);
            if (!difference.operator<(&limit)) {
                sprite=Hash->NextUnit();
                continue;
            }
        }

        const float distance=NearDistanceTo(sprite);
        if (distance>detectRange || distance<deadZone) {
            sprite=Hash->NextUnit();
            continue;
        }

        int accept=0;
        if (IsSpriteType(8) && !m_ptrSprite.sprite) {
            if (distance<bestDistance)
                accept=1;
        } else if (CanShotEnemy(sprite) && sprite->m_vid->m_idx!=104) {
            if (bestDistance>battleRange && distance<=battleRange)
                accept=1;
            else if (!(bestDistance<=battleRange && distance>battleRange) &&
                     IsBetterEnemy(distance,bestDistance,sprite,best))
                accept=1;
        }

        if (accept) {
            if (Army()!=0 || !sprite->IsSpriteClass(0x15) ||
                !reinterpret_cast<ENGINE*>(sprite)->HaveArmy(0)) {
                bestDistance=distance;
                best=sprite;
                if (randomTarget && Random(2)==0)
                    return best;
            }
        }
        sprite=Hash->NextUnit();
    }
    return best;
}

int SPRITE::AttackTact(int deltaTime)
{
    if (HaveFightLink()) {
        int result=m_child->AttackTact(deltaTime);
        if (result==5 && Goal())
            result=6;
        return result;
    }
    if (!CanFight())
        return 8;

    SPRITE* goal=Goal();
    if (!goal) {
        if (m_parent && m_parent->m_vid->m_spriteClass!=7 && GetTimer()==0) {
            ANGLE parentDirection=m_parent->Direction();
            Rotate(parentDirection,deltaTime);
        }
        return 5;
    }

    if (IsWeaponReload()) {
        if (Goal()) {
            int radius=0;
            ANGLE direction=DirectionTo(Goal(),&radius);
            Rotate(direction,deltaTime);
        }
        return 4;
    }

    if (IsCommand(5) || IsCommand(3) || IsCommand(4)) {
        if (m_parent && m_parent->m_vid->m_spriteClass!=7 && GetTimer()==0) {
            ANGLE parentDirection=m_parent->Direction();
            Rotate(parentDirection,deltaTime);
        }
        return 6;
    }

    WEAPON* const weapon=m_vid->m_weapon;
    const float battleRange=weapon->m_battleRange;
    const float deadZone=weapon->m_deadZone;
    const float distance=NearDistanceTo(Goal());

    int needParentTurn=0;
    if (m_parent && !IsCommand(5) && (distance>=battleRange || distance<=deadZone))
        needParentTurn=1;

    if (!needParentTurn) {
        int radius=0;
        ANGLE zero(static_cast<uint8_t>(0));
        ANGLE targetDirection=DirectionTo(Goal(),&radius);
        ANGLE rotation=Rotate(targetDirection,deltaTime);
        if (rotation.operator==(&zero) || weapon->PropAnyDirFire()) {
            if (distance>battleRange) {
                if (IsCommand(3))
                    return 1;
                return 2.0f*weapon->m_detectRange>distance ? 2 : 3;
            }

            const int fireState=Action(92,0,0,0);
            const int fireGate=*reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(m_vid)+0x2B4);
            if (fireState>=abs(fireGate)) {
                Action(93,-fireGate,0,0);
                ChangeAnimation(8);
                m_unknown50=static_cast<unsigned int>(weapon->m_reloadTime+5000);
                return 0;
            }
            return 7;
        }
    }

    if (m_parent && m_parent->m_vid->m_spriteClass!=7) {
        ANGLE parentDirection=m_parent->Direction();
        Rotate(parentDirection,deltaTime);
    }

    if (distance>battleRange) {
        if (IsCommand(3))
            return 1;
        return 2.0f*weapon->m_detectRange>distance ? 2 : 3;
    }
    return 4;
}

SPRITE* TERRAIN::AskCell(float x,float y)
{
    if (!Map->ValidateXY(x,y))
        return this;
    const float ground=GetGroundZ();
    if (Z()<=ground)
        return this;
    return CanPlace(x,y,Z());
}

ENGINE* ENGINE::PrevEngine()
{
    return *reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(this)+0x94);
}

int ENGINE::HaveArmy(int army)
{
    ENGINE* engine=this;
    while (engine) {
        if (reinterpret_cast<SPRITE*>(engine)->Army()==army)
            return 1;
        engine=engine->NextEngine();
    }
    engine=PrevEngine();
    while (engine) {
        if (reinterpret_cast<SPRITE*>(engine)->Army()==army)
            return 1;
        engine=engine->PrevEngine();
    }
    return 0;
}




R_DOT* ENGINE::GetGoal() { return goal; }

SPRITE* ENGINE::GetMoveTarget() { return Goal(); }

// -----------------------------------------------------------------------------
// ENGINE railway/train path owners required by PLAYER_STEAM right-click control.
// Original names and member layout come from MapEdit debug metadata debug metadata.
// -----------------------------------------------------------------------------
int ENGINE::NoStepForNotFound = 0;
int ENGINE::NoStep = 0;

int ENGINE::TrainWeaponRange()
{
    if (!Goal() || (!IsCommand(28) && !IsCommand(29)))
        return 0;

    float range=10000.0f;
    ENGINE* engine=commandEngine ? commandEngine : FirstEngine();
    while (engine) {
        if (engine->HaveFightLink() && engine->Ammo()>0) {
            SPRITE* weaponSprite=engine;
            if (engine->Vid()->m_idx!=35)
                weaponSprite=engine->Link();
            if (weaponSprite && weaponSprite->Vid() && weaponSprite->Vid()->m_weapon) {
                const float battleRange=weaponSprite->Vid()->m_weapon->m_battleRange;
                if (battleRange<range)
                    range=battleRange;
            }
        }
        if (commandEngine)
            break;
        engine=engine->NextEngine();
    }
    return range==10000.0f ? 0 : static_cast<int>(range);
}

int ENGINE::GetTrainLengthInRails()
{
    int count=0;
    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine())
        ++count;
    return static_cast<int>(static_cast<float>(count)*1.33f+0.5f);
}

int ENGINE::IsTailInFindedPath(R_DOT* dot)
{
    R_DOT* current=dot;
    ENGINE* last=LastEngine();
    R_DOT* tailDot=last ? last->tail.Dot2() : 0;
    for (int i=0;i<noFindedPath;++i) {
        const int linkIndex=static_cast<int>(findedway[i]);
        if (linkIndex>=0 && current && linkIndex<current->noLinks) {
            current=current->links[linkIndex].dot;
            if (current==tailDot)
                return 1;
        }
    }
    return 0;
}

void ENGINE::CreatePathDots(R_DOT* dot,SPRITE_LIST* pathdots,int nvid)
{
    R_DOT* current=dot;
    NoStepForNotFound=R_DOT::NoStepForNotFound;
    NoStep=R_DOT::NoStep;
    pathdots->DeleteAll();
    for (int i=0;i<noFindedPath;++i) {
        const int linkIndex=static_cast<int>(findedway[i]);
        if (linkIndex>=0 && current && linkIndex<current->noLinks) {
            current=current->links[linkIndex].dot;
            if (current) {
                pathdots->Insert(Map->CreateSprite(Map->Vid(nvid),
                    static_cast<float>(current->x),static_cast<float>(current->y),static_cast<float>(current->z),ANGLE(static_cast<uint8_t>(0)),0));
            }
        }
    }
}

void ENGINE::CreatePathDots(R_DOT* dot)
{
    CreatePathDots(dot,&PathDots,0x25b);
    for (int i=0;i<PathDots.No();++i) {
        SPRITE* path=*PathDots[i];
        if (IsCommand(28) || IsCommand(29))
            path->ChangeArmy(1);
        else if (IsCommand(27))
            path->ChangeArmy(3);
        else if (IsCommand(26))
            path->ChangeArmy(2);
    }
}

void ENGINE::ReCalcMoveParameters()
{
    if (!IsFirst()) {
        FirstEngine()->ReCalcMoveParameters();
        return;
    }

    TRAIN_INFO info(this);
    if (maxSpeed==0.0f && info.CanMove() && (m_flag&0x80u) && !globaldeleting) {
        SPRITE* target=goal ? 0 : Goal();
        const int noStep=head.NoStepToTarget(goal,target,static_cast<unsigned int>(Command()),this);
        SPRITE* flagman=Map->Flagman();
        if (InTrain(flagman) && flagman->Army()==0)
            CreatePathDots(head.Dot2());
        maxSpeed=noStep<0 ? -0.001f : 0.001f;
    }

    if (maxSpeed<0.0f) {
        maxSpeed=-static_cast<float>(info.speed)/1000.0f;
    } else if (maxSpeed>0.0f) {
        maxSpeed=static_cast<float>(info.speed)/1000.0f;
        acceleration=info.Acceleration();
        if (!acceleration)
            m_flag&=~0x80u;
    }
}




void ENGINE::BreakTrain(float x,float y)
{
    ENGINE* first=FirstEngine();
    ENGINE* last=LastEngine();
    PlaySFX(15);

    int breakPrev=0;
    if (prev_engine && next_engine)
        breakPrev=prev_engine->NearDistanceTo(x,y)<next_engine->NearDistanceTo(x,y);

    if (next_engine && !breakPrev) {
        next_engine->prev_engine=0;
        next_engine=0;
    } else if (prev_engine) {
        prev_engine->next_engine=0;
        prev_engine=0;
    }

    if (last!=first) {
        last->Stop();
        if (last->Speed()==0.0f) {
            last->ReverseTrain();
            last->FirstEngine()->SetAbsSpeed(0.01f);
        }
        if (first->Speed()!=0.0f)
            first->ReCalcMoveParameters();
        else
            first->FirstEngine()->SetAbsSpeed(0.01f);
    }
}

void ENGINE::BreakTrain(ENGINE* eng)
{
    ENGINE* first=FirstEngine();
    ENGINE* last=LastEngine();
    PlaySFX(15);

    int done=0;
    for (ENGINE* current=PrevEngine();current;current=current->PrevEngine()) {
        if (current==eng) {
            eng->next_engine->prev_engine=0;
            eng->next_engine=0;
            done=1;
        }
    }

    if (!done) {
        for (ENGINE* current=NextEngine();current;current=current->NextEngine()) {
            if (current==eng) {
                eng->prev_engine->next_engine=0;
                eng->prev_engine=0;
                done=1;
            }
        }
    }

    if (!done) {
        static char text[]="\xED\xE5 \xED\xE0\xE9\xE4\xE5\xED \xE2\xE0\xE3\xEE\xED \xE4\xEB\xFF \xEE\xF2\xF6\xE5\xEF\xEB\xE5\xED\xE8\xFF";
        Error(10,text,0);
    }

    if (last!=first) {
        last->Stop();
        if (last->Speed()==0.0f) {
            last->ReverseTrain();
            last->FirstEngine()->SetAbsSpeed(0.01f);
        }
        if (first->Speed()!=0.0f)
            first->ReCalcMoveParameters();
        else
            first->FirstEngine()->SetAbsSpeed(0.01f);
    }
}


// -----------------------------------------------------------------------------
// PLANE retail flight SCC, MapEdit.exe 0x00478EA0..0x004794FF.
// The class extends UNIT's vtable with eight flight-control virtual owners.
// -----------------------------------------------------------------------------

PLANE::PLANE(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : UNIT(vid,x,y,z,direction,parent)
{
    StartMove();
    m_unknown24=m_vid->m_maxZSpeed;
}

PLANE::~PLANE()
{
}

void* PLANE::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->PLANE::~PLANE();
    if (flags&1u)
        operator delete(self);
    return self;
}

int PLANE::Action(int act,int var1,int var2,int var3)
{
    switch (act) {
    case 0x21:
        Move(static_cast<float>(var1),static_cast<float>(var2),
             Map->GetGroundZ(m_vid,static_cast<float>(var1),static_cast<float>(var2))+80.0f);
        return 0;
    case 0x82:
        PlaneNextCommand(var1,var2);
        return 0;
    case 0x55:
        return TERRAIN::Action(act,var1,var2,var3);
    default:
        return UNIT::Action(act,var1,var2,var3);
    }
}

void PLANE::FreeFlight()
{
    const int animation=Animation();
    if (animation==4) {
        if (Random(2)==0) {
            ChangeAnimation(2);
        } else {
            const ANGLE turn(32);
            const ANGLE dir=Direction()-&turn;
            ChangeDirection(dir);
        }
    } else if (animation==5) {
        if (Random(2)==0) {
            ChangeAnimation(2);
        } else {
            const ANGLE turn(32);
            const ANGLE dir=Direction()+&turn;
            ChangeDirection(dir);
        }
    } else if (animation==2) {
        if (Random(10)==0) {
            if (Random(1)!=0)
                ChangeAnimation(4);
            else
                ChangeAnimation(5);
        }
    } else {
        ChangeAnimation(2);
    }

    if (Random(20)==0 && (behave&1u)) {
        SPRITE* const enemy=SeekEnemy();
        if (enemy)
            SetCommand(4,enemy);
    }
}

void PLANE::CheckFlightProperties()
{
}

void PLANE::FlightToTargetAdditionalActions()
{
    m_unknown04=AttackTact(DeltaTime());
    const int result=m_unknown04;
    if ((behave&1u) && (result!=6 || Random(20)==0)) {
        SPRITE* const enemy=SeekEnemy();
        if (enemy)
            SetCommand(4,enemy);
    }
}

void PLANE::FlightToTarget()
{
    SPRITE* const goal=Goal();
    ANGLE target=DirectionTo(goal);
    ANGLE zero(static_cast<uint8_t>(0));
    ANGLE rotated=Rotate(target,DeltaTime());
    if (rotated==&zero)
        ChangeAnimation(2);
    FlightToTargetAdditionalActions();
}

void PLANE::ZSpeedInitialization()
{
    const float z=Z();
    const float center=GetGroundZ()+m_vid->m_groundOffset;
    if (center-10.0f>=z) {
        m_unknown24=m_vid->m_maxZSpeed;
    } else if (center+10.0f<z) {
        m_unknown24=-m_vid->m_maxZSpeed;
    } else {
        m_unknown24=0.0f;
    }
}

int PLANE::WayBlocked()
{
    ANGLE dir=Direction();
    return AskCell(X()+dir.Sin()*128.0f,Y()-dir.Cos()*128.0f)!=0;
}

void PLANE::FlightIfWayBlocked()
{
    const ANGLE turn(16);
    ANGLE right=Direction()+&turn;
    if (AskCell(X()+right.Sin()*128.0f,Y()-right.Cos()*128.0f)) {
        ChangeAnimation(5);
        right=Direction()+&turn;
        ChangeDirection(right);
    } else {
        ChangeAnimation(4);
        const ANGLE left=Direction()-&turn;
        ChangeDirection(left);
    }
}

void PLANE::PlaneNextCommand(int var1,int var2)
{
    (void)var1;
    (void)var2;
    if (IsDying() || Animation()==12)
        return;

    if (Animation()>=7 && Animation()!=10)
        ChangeAnimation(0);

    ZSpeedInitialization();
    CheckFlightProperties();
    if (IsLinked())
        return;

    if ((m_flag&0x80u)==0)
        m_flag|=0x80u;

    if (Goal())
        FlightToTarget();
    else
        FreeFlight();
}

void ENGINE::Error(int type,char* text,unsigned long err)
{
    SPRITE* const sprite=reinterpret_cast<SPRITE*>(this);
    const int nvid=(sprite->Vid()) ? sprite->Vid()->m_idx : -1;
    MYERROR::Error(::Error,"ENGINE %i",type,text,err,nvid);
}

void ENGINE::CheckPrevNextEngine()
{
    ENGINE* prev=PrevEngine();
    if (prev && prev->NextEngine()!=this) {
        Error(4,const_cast<char*>("PrevEngine->NextEngine!=this"),
              reinterpret_cast<unsigned long>(prev->NextEngine()));
        *reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(prev)+0x98)=this;
    }

    ENGINE* next=NextEngine();
    if (next && next->PrevEngine()!=this) {
        Error(4,const_cast<char*>("NextEngine->PrevEngine!=this"),
              reinterpret_cast<unsigned long>(next->PrevEngine()));
        *reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(next)+0x94)=this;
    }
}

int DEPO::CanBuildUnit(int n)
{
    return GetItemNumber(n);
}

int DEPO::GetQueueUnit(int n)
{
    return n<numunit ? static_cast<int>(unitsToBuild[n]) : 0;
}

int DEPO::NoQueueUnit()
{
    return numunit;
}

int DEPO::SlotForUnit(int nvid)
{
    EX_SPRITE_DATA* const data=ExData();
    if (!data)
        return -1;
    const int location=data->items.Location(&nvid);
    return location<0 ? location : location+1;
}

void DEPO::BreakQueue()
{
    numunit=0;
    curunit=0;
    SetCommand(0,0);
    ChangeAnimation(0);
}

void DEPO::DeleteQueueUnit(int n)
{
    if (numunit!=0) {
        if (curunit!=0)
            timers[curunit-1]=GetTimer();

        Map->Player(Army())->AddMoney(Map->Vid(unitsToBuild[n])->GetBuildTime());

        for (int i=n;i<numunit-1;++i) {
            unitsToBuild[i]=unitsToBuild[i+1];
            timers[i]=timers[i+1];
            pause[i]=pause[i+1];
        }

        --numunit;
        if (numunit==0) {
            BreakQueue();
            return;
        }
    }

    curunit=0;
    BuildNextUnit();
}

void DEPO::BuildNextUnit()
{
    if (curunit!=0)
        return;

    curunit=1;
    while (pause[curunit-1]!=0 && curunit<=unitlim)
        ++curunit;

    if (curunit<=numunit) {
        SetCommand(16,0);
        const int nvid=unitsToBuild[curunit-1];
        if (timers[curunit-1]!=0)
            SetTimer(timers[curunit-1]);
        else
            SetTimer(static_cast<unsigned long>(Map->Vid(nvid)->GetBuildTime()*Const->DepoMillisecondsInSecond));
        return;
    }

    curunit=0;
    SetCommand(0,0);
    ChangeAnimation(0);
}

void DEPO::ReplaceQueueUnit(int n,int new_vid)
{
    if (n>=numunit)
        return;

    Map->Player(Army())->AddMoney(Map->Vid(unitsToBuild[n])->GetBuildTime());
    Map->Player(Army())->AddMoney(-Map->Vid(new_vid)->GetBuildTime());
    unitsToBuild[n]=static_cast<unsigned short>(new_vid);
    timers[n]=static_cast<unsigned long>(Map->Vid(new_vid)->GetBuildTime()*Const->DepoMillisecondsInSecond);

    n=curunit-1;
    if (n==0)
        return;

    curunit=0;
    BuildNextUnit();
}

int DEPO::PauseQueueUnit(int num)
{
    if (num>=numunit)
        return 0;

    if (pause[num]==0) {
        pause[num]=1;
        if (num==curunit-1) {
            timers[num]=GetTimer();
            curunit=0;
            BuildNextUnit();
        }
    } else {
        pause[num]=0;
        if (num<curunit-1 || curunit==0) {
            if (curunit!=0)
                timers[curunit-1]=GetTimer();
            curunit=0;
            BuildNextUnit();
        }
    }
    return pause[num];
}

int DEPO::IsPausedQueueUnit(int num)
{
    return num<numunit && pause[num]!=0 ? 1 : 0;
}

void DEPO::ChangePlaces(int unit1,int unit2)
{
    if (unit1==unit2)
        return;

    int restart=0;
    if (unit1==curunit-1) {
        timers[unit1]=GetTimer();
        restart=1;
    } else if (unit2==curunit-1) {
        timers[unit2]=GetTimer();
        restart=1;
    }

    const unsigned short unit=unitsToBuild[unit1];
    unitsToBuild[unit1]=unitsToBuild[unit2];
    unitsToBuild[unit2]=unit;

    const unsigned long timer=timers[unit1];
    timers[unit1]=timers[unit2];
    timers[unit2]=timer;

    const int paused=pause[unit1];
    pause[unit1]=pause[unit2];
    pause[unit2]=paused;

    if (restart) {
        curunit=0;
        BuildNextUnit();
    }
}

void DEPO::RightShiftQueueUnit(int n)
{
    if (n+1<numunit)
        ChangePlaces(n,n+1);
}

void DEPO::LeftShiftQueueUnit(int n)
{
    if (n<numunit && n>=1)
        ChangePlaces(n-1,n);
}

TRAIN_INFO::TRAIN_INFO(const ENGINE* eng)
{
    weight=0.0f;
    power=0.0f;
    speed=10000;
    hp=0;
    no=0;
    percentAmmo=0;
    weapon=0;
    noAmmo=0;
    build_time=0;
    trainweight=0.0f;
    max_hp=0;
    // Retail VC6 emits two independent bitfield read/modify/write clears and
    // therefore does not initialise the remaining 30 bits of this dword.
    haveAmmo=0;
    haveRepair=0;
    maxBattleRange=0.0f;
    ammo=0;
    maxAmmo=0;
    minBattleRange=999999.0f;

    ENGINE* p=const_cast<ENGINE*>(eng);
    for (;p;p=p->NextEngine())
        AddEngine(p);

    p=const_cast<ENGINE*>(eng)->PrevEngine();
    for (;p;p=p->PrevEngine())
        AddEngine(p);

    if (noAmmo!=0)
        percentAmmo/=noAmmo;
    else
        percentAmmo=100;

    if (minBattleRange==999999.0f)
        minBattleRange=0.0f;

    const float nonPoweredWeight=weight-trainweight;
    const float poweredReserve=power-trainweight;
    if (poweredReserve!=0.0f) {
        speed=static_cast<int>((poweredReserve-nonPoweredWeight)*
                               static_cast<float>(speed)/poweredReserve);
        if (speed<5)
            speed=0;
    }
    if (speed==10000)
        speed=0;
}

void TRAIN_INFO::AddEngine(const ENGINE* eng)
{
    ENGINE* const engine=const_cast<ENGINE*>(eng);
    SPRITE* const sprite=reinterpret_cast<SPRITE*>(engine);
    UNIT* const unit=reinterpret_cast<UNIT*>(engine);
    VID* const vid=sprite->Vid();
    WEAPON* const engineInfo=vid->m_weapon;

    if (engineInfo->m_power!=0.0f) {
        const float maxSpeed=sprite->MaxSpeed()*1000.0f;
        if (maxSpeed<static_cast<float>(speed))
            speed=static_cast<int>(maxSpeed);
    }

    power+=engineInfo->m_power;
    weight+=engineInfo->m_weight;
    if (engineInfo->m_power>0.0f)
        trainweight+=engineInfo->m_weight;

    if (vid->m_idx!=45)
        haveAmmo=1;
    else if (vid->m_idx!=85)
        haveRepair=1;

    hp+=sprite->Hp();

    const int cur_ammo=unit->Ammo();
    const int max_ammo=sprite->MaxAmmo();
    if (cur_ammo>0) {
        const float battle_range=sprite->BattleRange();
        if (battle_range>maxBattleRange)
            maxBattleRange=battle_range;
        if (battle_range!=0.0f && battle_range<minBattleRange)
            minBattleRange=battle_range;
    }

    if (max_ammo!=0 && max_ammo!=999999 && vid->m_idx!=85) {
        ++noAmmo;
        ammo+=cur_ammo;
        maxAmmo+=max_ammo;
        percentAmmo+=cur_ammo*100/max_ammo;
    }

    max_hp+=sprite->MaxHp();
    build_time+=vid->GetBuildTime();

    int fire_damage=0;
    if (cur_ammo!=0) {
        if (vid->m_idx==82)
            fire_damage=Map->Vid(70)->GetFireDamage();
        else
            fire_damage=sprite->GetFireDamage();
    }
    weapon+=fire_damage;

    if (vid->m_linkVid) {
        const int army=sprite->Army();
        max_hp+=vid->m_linkVid->GetMaxHp(army);
        if (!sprite->HaveLink()) {
            if (vid->m_linkVid->NoSprites(army)>=vid->NoSprites(army))
                hp+=vid->m_linkVid->GetMaxHp(army);
        }
    }

    if (sprite->HaveLink()) {
        SPRITE* const link=sprite->Link();
        weight+=link->Vid()->m_weapon->m_weight;
        if (!link->IsSpriteClass(9))
            hp+=link->Hp();
        if (engineInfo->m_power>0.0f)
            trainweight+=link->Vid()->m_weapon->m_weight;
    }

    ++no;
}

int TRAIN_INFO::CanMove()
{
    return Acceleration()>7;
}

int TRAIN_INFO::HaveAmmo()
{
    return weapon>0;
}

int TRAIN_INFO::IsDamaged()
{
    return hp<max_hp;
}

int TRAIN_INFO::NeedAmmo()
{
    return percentAmmo<100;
}

int TRAIN_INFO::Acceleration()
{
    if (weight==0.0f)
        return 0;
    return static_cast<int>(power/weight*8.0f);
}

int ENGINE::IsSingle()
{
    const unsigned char* const raw=reinterpret_cast<const unsigned char*>(this);
    return *reinterpret_cast<ENGINE* const*>(raw+0x98)==0 &&
           *reinterpret_cast<ENGINE* const*>(raw+0x94)==0;
}

ENGINE* ENGINE::LastEngine()
{
    ENGINE* engine=this;
    while (*reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(engine)+0x98))
        engine=*reinterpret_cast<ENGINE**>(reinterpret_cast<unsigned char*>(engine)+0x98);
    return engine;
}

int ENGINE::CanBeLinkedByEnemy()
{
    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        SPRITE* const sprite=reinterpret_cast<SPRITE*>(engine);
        VID* const vid=sprite->Vid();
        if (vid->m_weapon->m_power==0.0f || vid->m_idx==351)
            return 0;
    }
    return 1;
}

void ENGINE::SetAbsSpeed(float new_speed)
{
    const unsigned int stateInvMove=
        *reinterpret_cast<unsigned int*>(reinterpret_cast<unsigned char*>(this)+0x90);
    reinterpret_cast<SPRITE*>(this)->SetSpeed((stateInvMove&1u)!=0 ? -new_speed : new_speed);
}

STRING SPRITE::GetTextActions()
{
    STRING result;
    for (int i=0;i<m_actions.m_no;++i) {
        const ACT& a=m_actions.m_data[i];
        STRING one=Printf("%c%i,%i,%i;",a.act+60,a.var1,a.var2,a.var3);
        result+=one.m_buf;
    }
    return result;
}

STRING SPRITE::GetTextItems()
{
    STRING result;
    if (m_exData && m_exData->items.m_no) {
        for (int i=0;i<m_exData->items.m_no;++i) {
            STRING one=Printf("\1%i",m_exData->items.m_data[i]);
            result+=one.m_buf;
        }
        result+="\2";
    }
    return result;
}

void SPRITE::SetTextItems(const STRING* text)
{
    if (!text || text->m_buf==STRING::EMPTY || !text->m_buf[0])
        return;
    STRING rest=text->After("\1");
    if (!m_exData)
        m_exData=new EX_SPRITE_DATA(this);
    while (rest.m_buf[0]) {
        int item=0;
        sscanf(rest.m_buf,"%i",&item);
        m_exData->items.Insert(item);
        rest=rest.After("\1");
    }
}

void SPRITE::SetTextActions(const STRING* text)
{
    if (!text)
        return;
    STRING rest(*text);
    while (rest.m_buf[0]) {
        char command=0;
        int a=0,b=0,c=0;
        sscanf(rest.m_buf,"%c%i,%i,%i",&command,&a,&b,&c);
        AddAction(static_cast<int>(command)-60,a,b,c);
        rest=rest.After(";");
    }
}

void SPRITE::AddActionAfterStop(int action,int var1,int var2,int var3)
{
    int location=-1;
    for (int i=m_actions.m_no-1;i>=0;--i) {
        const ACT& a=m_actions.m_data[i];
        if (a.act==73 && a.var1==0 && a.var2==0 && a.var3==0) {
            location=i;
            break;
        }
    }
    if (location<0) {
        AddAction(action,var1,var2,var3);
        return;
    }
    const int insertAt=location+1;
    m_actions.ExpandForInsert();
    for (int i=m_actions.m_no;i>insertAt;--i)
        m_actions.m_data[i]=m_actions.m_data[i-1];
    m_actions.m_data[insertAt]=ACT(action,var1,var2,var3);
    ++m_actions.m_no;
}

int SPRITE::IsActionStackEmpty()
{
    if (m_actions.m_no==0)
        return 1;
    return m_actions.m_data[m_actions.m_no-1].act==73 ? 1 : 0;
}

// MapEdit.exe terrain.cpp:65.  Retail updates at most once per 1024 game ticks.
void TERRAIN::AddHpPerSecond(int hp_to_add)
{
    if ((CurrentTime&0xFFFFFC00u)>PrevCurrentTime && Hp()>0) {
        int hp=Hp()+hp_to_add;
        if (hp>MaxHp())
            hp=MaxHp();
        ChangeHp(hp);
    }
}

float SPRITE::DistanceTo(const SPRITE* goal)
{
    return Distance(goal->m_x-m_x,goal->m_y-m_y);
}

int SPRITE::ActionStackHaveCommand(int act)
{
    for (int i=0;i<m_actions.No();++i) {
        if (m_actions[i]->act==act)
            return 1;
    }
    return 0;
}

void SPRITE::SetJustBuilded()
{
    m_flag|=1u;
}
