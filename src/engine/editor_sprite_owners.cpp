#include "mapedit/runtime.hpp"

// Retail editor-specific sprite construction owners exposed again by
// MAP_EDIT::CreateSprite.  These are direct debug metadata/reference implementation reconstructions; no
// fallback class substitution is used.

R_POS::R_POS()
{
    dot=0;
    pos_fract=0;
    // Retail deliberately leaves pos_real/link untouched here.
}

RAIL::RAIL(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : TERRAIN(vid,x,y,z,direction,parent)
{
    p2=0;
    p1=0;
    Action(0x3C,direction.Int(),0,0);
}

BUILDING::BUILDING(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : UNIT(vid,x,y,z,direction,parent)
{
    cur_unit_need_repair=0;
    buildUnit=0;
    lastreparedunit=0;
    // buildUnitX/buildUnitY are intentionally not initialized by retail ctor.
}

DEPO::DEPO(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : UNIT(vid,x,y,z,direction,parent)
{
    currentTrainIdentificator=0;
    curunit=0;
    numunit=0;
    unitlim=10;
    // Exact retail loop bound is 20 although the embedded arrays contain 100.
    for (int i=0;i<20;++i) {
        unitsToBuild[i]=0;
        timers[i]=0;
        pause[i]=0;
    }
}

CREATURE::CREATURE(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : UNIT(vid,x,y,z,direction,parent)
{
    m_unknown24=m_vid->m_maxZSpeed;
    stateJustCreated|=1u;
    StartMove();
    in_region=FindRegion(X(),Y());
}

CIV_ROBOT::CIV_ROBOT(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : CREATURE(vid,x,y,z,direction,parent), near_train()
{
    behave_state=0;
    near_train=static_cast<SPRITE*>(0);
    near_explosion=0;
    // change_state_flag (+0xA4) is intentionally not initialized by retail ctor.
}

BALLOON::BALLOON(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : PLANE(vid,x,y,z,direction,parent)
{
    zspeed_state=0;
    must_taran=0;
}

ENGINE::ENGINE(VID* vid,float x,float y,float z,ANGLE direction,SPRITE* parent)
    : UNIT(vid,x,y,z,direction,parent), head(), tail()
{
    stateInvMove=0;
    isPushed=0;
    is_busy=1;
    lastz=0.0f;
    lasty=0.0f;
    lastx=0.0f;
    was_near_mine=0;
    minecreatetime=0;
    lastunitintrain=0;
    depo_train_num=-1;
    noFindedPath=0;
    patrolto2=0;
    patrolto1=0;
    maxSpeed=0.0f;
    acceleration=0;
    prev_engine=0;
    next_engine=0;
    goal=0;
    commandEngine=0;

    if (!Map->OptLoad()) {
        SetRDot();
        if (HaveLink())
            Link()->ChangeDirection(Direction());
    }
}

RAIL::~RAIL()
{
    if (p1)
        p1->Release();
    if (p2)
        p2->Release();
}

void* RAIL::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->RAIL::~RAIL();
    if (flags&1u) operator delete(self);
    return self;
}


int BUILDING::Action(int act,int var1,int var2,int var3)
{
    switch (act) {
    case 0x49:
        AddActionImmediate(act,var1,var2,var3);
        Action(0x82,0,0,0);
        return 0;

    case 0x46:
        if (IsCommand(0x10) && buildUnit) {
            AddActionImmediate(0x23,buildUnit->m_idx,0,0);
            SetCommand(0,0);
            return 0;
        }
        return UNIT::Action(act,var1,var2,var3);

    case 0x23:
        if (var1==0)
            var1=Action(0x3b,4,0,0);
        if (var1>0 && !IsCommand(0))
            return 0;
        if (!Map->ValidateVid(var1))
            return 0;
        buildUnit=Map->Vid(var1);
        buildUnitX=var2;
        buildUnitY=var3;
        SetTimer(buildUnit->m_weapon->m_buildTime);
        SetCommand(0x10,0);
        if (!IsDying())
            ChangeAnimation(1);
        return 0;

    case 0x82: {
        if (IsDying())
            return 0;
        if (IsCommand(0x10))
            ChangeAnimation(1);
        else if (Animation()!=10)
            ChangeAnimation(0);

        if (Vid()->m_idx==104)
            MasterRepair();

        if (!IsCommand(0x10) || GetTimer()!=0 || !buildUnit)
            return 0;

        float fx=static_cast<float>(buildUnitX);
        float fy=static_cast<float>(buildUnitY);
        if (buildUnitX==0 && buildUnitY==0) {
            fx=X();
            fy=Y();
        }
        if (buildUnitX<0)
            fx=X()-static_cast<float>(buildUnitX)-2.0f*static_cast<float>(Random(-buildUnitX));
        if (buildUnitY<0)
            fy=Y()-static_cast<float>(buildUnitY)-2.0f*static_cast<float>(Random(-buildUnitY));

        if (!Hash->CanPlace(buildUnit,fx,fy,Z())) {
            SetCommand(0,0);
            SPRITE* created=Map->CreateSprite(buildUnit,fx,fy,Z(),Direction(),this);
            if (created)
                Action(0x4b,reinterpret_cast<int>(created),0,0);
            buildUnit=0;
            ChangeAnimation(0);
        }
        return 0;
    }

    case 0x50:
        if (!ActionStackHaveCommand(0x49))
            AddActionImmediate(0x49,0,0,0);
        UNIT::Action(act,var1,var2,var3);
        return 0;

    case 0x0f:
        return UNIT::Action(act,var1,var2,var3);

    default:
        return UNIT::Action(act,var1,var2,var3);
    }
}


static __declspec(noinline) double sqr(double value)
{
    return value*value;
}

void BUILDING::MasterRepair()
{
    const int halfX=50;
    const int halfY=50;
    ENGINE* nearest=0;
    double nearestDistance=100000.0;

    for (SPRITE* sprite=Hash->FirstInBox(X()-halfX,Y()-halfY,X()+halfX,Y()+halfY);
         sprite;
         sprite=Hash->NextInBox()) {
        if (fabsf(sprite->X()-X())>static_cast<float>(halfX) ||
            fabsf(sprite->Y()-Y())>static_cast<float>(halfY) ||
            !sprite->IsSpriteClass(21))
            continue;

        ENGINE* const engine=static_cast<ENGINE*>(sprite);
        if (engine->HaveFightLink())
            engine->Link()->SetTimer(6000);

        const double dx=static_cast<double>(engine->X()-X());
        const double dy=static_cast<double>(engine->Y()-Y());
        const double distance=sqrt(sqr(dx)+sqr(dy));
        if (!nearest || distance<nearestDistance) {
            nearest=engine;
            nearestDistance=distance;
        }
    }

    cur_unit_need_repair=0;

    if (nearest && IsEnemy(nearest) && Army()!=2) {
        int canCapture=0;
        for (ENGINE* engine=nearest->FirstEngine();engine;engine=engine->NextEngine()) {
            if (engine->IsPowerEngine() && engine->Army()!=nearest->Army()) {
                canCapture=1;
                break;
            }
        }

        if (canCapture) {
            ++nearest->Vid()->m_reColored[nearest->Army()];
            nearest->ChangeArmy(Army());

            if (nearest->Goal() &&
                nearest->Goal()->Army()==nearest->Army() &&
                nearest->Goal()->IsSpriteClass(21))
                nearest->SetCommandSameOther();

            if (nearest->Link() && nearest->Link()->Goal() &&
                nearest->Link()->Goal()->Army()==nearest->Army() &&
                nearest->Link()->Goal()->IsSpriteClass(21))
                nearest->Link()->SetGoal(0);

            cur_unit_need_repair=1;
            nearest->DeleteAttackToEngine();
        }
    }

    if (!nearest || !IsEnemy(nearest)) {
        lastreparedunit=0;
        ChangeAnimation(0);
        return;
    }

    const int inRepairDistance=nearestDistance<20.0;
    if (nearest->NeedRepairByMaster())
        cur_unit_need_repair=1;

    if (inRepairDistance && nearest==lastreparedunit)
        cur_unit_need_repair=0;
    if (inRepairDistance)
        lastreparedunit=nearest;

    if (cur_unit_need_repair && inRepairDistance) {
        nearest->Action(0x56,0,0,0);
        ChangeAnimation(1);
    }
    else if (!inRepairDistance) {
        ChangeAnimation(0);
    }
}

BUILDING::~BUILDING()
{
}

void* BUILDING::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->BUILDING::~BUILDING();
    if (flags&1u) operator delete(self);
    return self;
}

void BUILDING::MoveTact()
{
    if (m_vid->m_idx==104)
        AddHpPerSecond(Const->MasterAutoAddHpPerSecond);
}



int DEPO::Action(int act,int var1,int var2,int var3)
{
    switch (act) {
    case 0x49:
        AddActionImmediate(act,var1,var2,var3);
        Action(0x82,0,0,0);
        return 0;

    case 0x46:
        if (IsCommand(0x10)) {
            AddActionImmediate(0x23,static_cast<unsigned short>(unitsToBuild[0]),0,0);
            SetCommand(0,0);
            return 0;
        }
        return UNIT::Action(act,var1,var2,var3);

    case 0x23:
        if (!Map->ValidateVid(var1))
            return 0;
        AddUnitToQueue(var1);
        BuildNextUnit();
        return 0;

    case 0x82:
        if (IsDying())
            return 0;
        if (Animation()==13)
            ChangeAnimation(0);

        if (curunit && !pause[curunit-1]) {
            if (!GetTimer()) {
                ActionBuildUnit(var1,var2);
                return 0;
            }
            if (Animation()!=1)
                ChangeAnimation(1);
        }
        else {
            if (Animation()!=0)
                ChangeAnimation(0);
            if (!IsCommand(0))
                SetCommand(0,0);
            Map->ScriptRun(EvFunctionNumber[14],this,0,0);
        }
        return 0;

    case 0x55:
        if (var1>0 && !(m_flag&2u) && var2 &&
            ((m_flag ^ reinterpret_cast<SPRITE*>(var2)->m_flag)&0x1800u)) {
            Map->ScriptRun(EvFunctionNumber[13],this,reinterpret_cast<SPRITE*>(var2),0);
            m_flag|=2u;
        }
        return UNIT::Action(act,var1,var2,var3);

    case 0x61: {
        const int oldArmy=Army();
        Map->Player(oldArmy)->DeletePointerToSprite(this);
        ChangeArmy(var1);
        if (oldArmy!=Army())
            Map->ScriptRun(EvFunctionNumber[15],this,0,0);
        Map->Player(Army())->AddUnitToStateBar(this);
        return 0;
    }

    case 0x51:
    case 0xC8: {
        UNIT::Action(act,var1,var2,var3);
        Map->Player(Army())->AddUnitToStateBar(this);
        EX_SPRITE_DATA* const ex=ExData();
        if (!ex)
            return 0;

        const int order[14]={5,10,20,25,80,85,45,30,35,82,97,90,75,62};
        int put=0;
        for (int i=0;i<14;++i) {
            for (int scan=put;scan<ex->items.No();++scan) {
                if (*ex->items[scan]==order[i]) {
                    const int old=*ex->items[put];
                    *ex->items[put++]=*ex->items[scan];
                    *ex->items[scan]=old;
                }
            }
        }
        return 0;
    }

    case 0x50:
        if (!ActionStackHaveCommand(0x49))
            AddActionImmediate(0x49,0,0,0);
        return UNIT::Action(act,var1,var2,var3);

    default:
        return UNIT::Action(act,var1,var2,var3);
    }
}

DEPO::~DEPO()
{
}

void* DEPO::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->DEPO::~DEPO();
    if (flags&1u) operator delete(self);
    return self;
}

void DEPO::MoveTact()
{
    if ((CurrentTime&0xFFFFC000u)>PrevCurrentTime)
        m_flag&=~2u;
    AddHpPerSecond(Const->DepoAutoAddHpPerSecond);
}



void CREATURE::MoveTact()
{
    if ((stateJustCreated&1u) && !in_region) {
        stateJustCreated&=~1u;
        in_region=FindRegion(X(),Y());
    }
    float x,y,z;
    MoveTactCalcCoor(&x,&y,&z);
    if (blocktick) {
        if (blocktick&1) {
            if (Animation()==4)
                Rotate(ANGLE(static_cast<unsigned char>(Direction().Int()-0x40)),DeltaTime());
            else if (Animation()!=5 && Random(1)==0)
                Rotate(ANGLE(static_cast<unsigned char>(Direction().Int()-0x40)),DeltaTime());
            else
                Rotate(ANGLE(static_cast<unsigned char>(Direction().Int()+0x40)),DeltaTime());
        }
        if (blocktick<0) ++blocktick; else --blocktick;
    }
    else if (Goal() && Speed()!=0.0f) {
        ANGLE target=DirectionTo(Goal());
        if (Speed()<0.0f)
            target=ANGLE(static_cast<unsigned char>(target.Int()+0x80));
        Rotate(GlideDirection(target),DeltaTime());
        if (IsMoveFinished()) Stop();
    }
    if (Vid()->PropZeroZ())
        z=Map->GetGroundZ(Vid(),x,y);
    if (CanPlaceWithCrush(x,y,z)) {
        m_speed=0.0f;
        if (!blocktick) blocktick=10;
        return;
    }
    if (in_region && !in_region->IsInsideXY(x,y)) {
        REGION* const nextRegion=FindRegion(x,y);
        if (!nextRegion || in_region->Vid()!=nextRegion->Vid()) {
            m_speed=0.0f;
            if (!blocktick) blocktick=10;
            return;
        }
        in_region=nextRegion;
    }
    MoveTactMapLimit(x,y);
    ChangeCoor(x,y,z);
}

int CREATURE::Action(int act,int var1,int var2,int var3)
{
    if (act==0x55) {
        if (var1>0) {
            for (SPRITE* sprite=Hash->FirstInBox(X()-150.0f,Y()-150.0f,X()+150.0f,Y()+150.0f);
                 sprite; sprite=Hash->NextInBox()) {
                if (sprite->IsSpriteClass(25) && sprite->NearDistanceTo(X(),Y())<150.0f)
                    sprite->StartMove();
            }
        }
        return UNIT::Action(act,var1,var2,var3);
    }
    if (act!=0x82)
        return UNIT::Action(act,var1,var2,var3);
    if (IsDying())
        return 0;
    if (Animation()==8 || Animation()==13)
        ChangeAnimation(0);
    if ((stateJustCreated&1u) && !in_region) {
        stateJustCreated&=~1u;
        in_region=FindRegion(X(),Y());
    }
    if (Goal() && IsCommand(1)) {
        if (Speed()==0.0f)
            ChangeAnimation(0);
        else if (Animation()!=2) {
            ChangeAnimation(2);
            return 0;
        }
    }
    else {
        const unsigned int quantizedTime=CurrentTime-(CurrentTime&0x7ffu);
        if (IsCommand(3) || IsCommand(4) || quantizedTime>m_tactTime) {
            m_unknown04=AttackTact(DeltaTime());
            const int state=m_unknown04;
            if (state==1 && Speed()==0.0f) {
                if ((HaveFightLink() && Link()->Goal()) || Goal())
                    SetCommand(0,static_cast<SPRITE*>(0));
            }
            if ((state==2 || state==5) && (behave&1u) && (IsCommand(0) || IsCommand(1))) {
                SPRITE* const enemy=SeekEnemy();
                if (enemy) SetCommand(4,enemy);
            }
        }
        if (!IsCommand(1) && !IsCommand(3) && !IsCommand(4)) {
            if (Animation()==4 && Random(2)!=0)
                Rotate(ANGLE(static_cast<unsigned char>(Direction().Int()-0x20)),DeltaTime());
            if (Animation()==5 && Random(2)!=0)
                Rotate(ANGLE(static_cast<unsigned char>(Direction().Int()+0x20)),DeltaTime());
            if (quantizedTime>m_tactTime) {
                if (Random(3)==0)
                    StartMove();
                else if (Speed()==0.0f && Random(2)==0 && HaveAction(12))
                    ChangeAnimation(12);
                else if (Random(3)==0) {
                    if (Random(1)!=0)
                        Rotate(ANGLE(static_cast<unsigned char>(Direction().Int()-0x20)),DeltaTime());
                    else
                        Rotate(ANGLE(static_cast<unsigned char>(Direction().Int()+0x20)),DeltaTime());
                }
                else
                    Stop();
            }
        }
        if (Animation()==13 || Animation()==9)
            ChangeAnimation(0);
    }
    return 0;
}

CREATURE::~CREATURE()
{
}

void* CREATURE::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->CREATURE::~CREATURE();
    if (flags&1u) operator delete(self);
    return self;
}

void CREATURE::DeletePointerToSprite(SPRITE* sprite)
{
    if (in_region==sprite)
        in_region=0;
    SPRITE::DeletePointerToSprite(sprite);
}

REGION* CREATURE::FindRegion(float x,float y)
{
    int i;
    SPRITE* spr=Map->FirstSpriteByType(10,&i,0x40u);
    while (spr) {
        if (spr->IsSpriteClass(23u) && static_cast<REGION*>(spr)->IsInsideXY(x,y))
            return static_cast<REGION*>(spr);
        spr=Map->NextSpriteByType(10,&i,0x40u);
    }
    return 0;
}

CIV_ROBOT::~CIV_ROBOT()
{
}

void* CIV_ROBOT::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->CIV_ROBOT::~CIV_ROBOT();
    if (flags&1u) operator delete(self);
    return self;
}

void CIV_ROBOT::DeletePointerToSprite(SPRITE* sprite)
{
    SPRITE* const current=near_train;
    if (current==sprite)
        near_train=static_cast<SPRITE*>(0);
    CREATURE::DeletePointerToSprite(sprite);
}

BALLOON::~BALLOON()
{
}

void* BALLOON::ScalarDeletingDestructor(unsigned int flags)
{
    void* const self=this;
    this->BALLOON::~BALLOON();
    if (flags&1u) operator delete(self);
    return self;
}

// -----------------------------------------------------------------------------
// A9: CIV_ROBOT retail behavior closure (robot.cpp / robot.h)
// -----------------------------------------------------------------------------

static const int kRobotBuildingVids[16] = {
    170,803,805,806,807,809,810,813,846,848,861,863,864,874,875,895
};

int CIV_ROBOT::IsRobotBuilding(const SPRITE* building)
{
    if (!building)
        return 0;
    const int idx=building->Vid()->m_idx;
    for (int i=0;i<16;++i)
        if (idx==kRobotBuildingVids[i])
            return 1;
    return 0;
}

SPRITE* CIV_ROBOT::FindRobotBuilding()
{
    SPRITE* result=0;
    SPRITE* s=Hash->FirstInBox(X()-200.0f,Y()-133.0f,X()+200.0f,Y()+Z()+133.0f);
    while (s) {
        if (s->IsSpriteType(2u) && s->IsSpriteClass(1u) && IsRobotBuilding(s)) {
            if (!in_region) {
                result=s;
            } else {
                REGION* r=FindRegion(s->X(),s->Y());
                if (r && in_region->Vid()==r->Vid())
                    result=s;
            }
            if (result && Random(3)==0)
                break;
        }
        s=Hash->NextInBox();
    }
    return result;
}

void CIV_ROBOT::PathIsBlocked()
{
    m_speed=0.0f;
    if (!blocktick)
        blocktick=Random(1) ? 10 : -10;
    if (IsCommand(1u) && Random(8)==0) {
        Stop();
        behave_state=0;
    }
}

void CIV_ROBOT::ChangeAnimation(int act)
{
    if (HaveLink()) {
        SPRITE* child=Link();
        if (child->Animation()!=act && child->m_noCadr>=child->m_endCadr)
            child->ChangeAnimation(act);
    } else if (Animation()!=act) {
        SPRITE::ChangeAnimation(act);
    }
}

void CIV_ROBOT::RotateHead(ANGLE direct)
{
    if (!HaveLink())
        return;
    SPRITE* child=Link();
    if (child->m_noCadr<child->m_endCadr)
        return;
    child->Rotate(direct,DeltaTime());
}

void CIV_ROBOT::MoveTact()
{
    if ((stateJustCreated&1u) && !in_region) {
        stateJustCreated&=~1u;
        in_region=FindRegion(X(),Y());
    }

    float x,y,z;
    MoveTactCalcCoor(&x,&y,&z);
    if (Vid()->PropZeroZ())
        z=Map->GetGroundZ(Vid(),x,y);

    if (z-Z()>Vid()->m_groundToleranceAbove || Z()-z>Vid()->m_groundToleranceBelow) {
        PathIsBlocked();
        return;
    }

    if (in_region && !in_region->IsInsideXY(x,y)) {
        REGION* found=FindRegion(x,y);
        if (!found || in_region->Vid()!=found->Vid()) {
            PathIsBlocked();
            return;
        }
        in_region=found;
    }

    if (!Map->ValidateXY(x,y)) {
        PathIsBlocked();
        return;
    }

    SPRITE* blocker=CanPlaceWithCrush(x,y,Z());
    if (!blocker) {
        ChangeCoor(x,y,z);
        return;
    }

    if (IsXYCross(blocker) && IsZCross(blocker)) {
        ChangeCoor(x,y,z);
        return;
    }

    if (IsRobotBuilding(blocker) && behave_state==14u) {
        const unsigned char aim=(unsigned char)(blocker->Direction().value+0x80u);
        const unsigned char approach=DirectionTo(blocker).value;
        const unsigned char d1=(unsigned char)(approach-aim);
        const unsigned char d2=(unsigned char)(aim-approach);
        if ((d1<d2?d1:d2)<30u) {
            ChangeCoor(x,y,z);
            return;
        }
    }

    if (blocker->Vid()==Vid()) {
        CIV_ROBOT* other=static_cast<CIV_ROBOT*>(blocker);
        if (behave_state==15u || other->behave_state==15u) {
            ChangeCoor(x,y,z);
            return;
        }
    }
    PathIsBlocked();
}

int CIV_ROBOT::Action(int act,int var1,int var2,int var3)
{
    switch (act) {
    case 9:
        if (Animation()!=8) {
            if (HaveLink())
                Link()->ChangeAnimation(9);
            else
                ChangeAnimation(9);
        }
        return 0;

    case 34: {
        SPRITE* b=reinterpret_cast<SPRITE*>(var1);
        if (b) {
            ANGLE bd=b->Direction();
            const float bx=b->X();
            const float by=b->Y();
            const float bz=b->Z()+b->Vid()->m_groundToleranceAbove;
            SPRITE* marker=new SPRITE(EmptyVid,
                bx+bd.Sin()*4.0f,
                by-bd.Cos()*4.0f,
                bz,ANGLE((unsigned char)0),0);
            Move(marker);
        }
        return 0;
    }

    case 85:
        if (var1>0) {
            SPRITE* s=Hash->FirstInBox(X()-150.0f,Y()-150.0f,X()+150.0f,Y()+300.0f);
            while (s) {
                if (s->IsSpriteClass(20u) && NearDistanceTo(s)<150.0f) {
                    CIV_ROBOT* r=static_cast<CIV_ROBOT*>(s);
                    r->near_explosion=1;
                    r->change_state_flag=1;
                }
                s=Hash->NextInBox();
            }
        }
        return UNIT::Action(act,var1,var2,var3);

    case 130:
        break;

    default:
        return CREATURE::Action(act,var1,var2,var3);
    }

    if (Animation()>=15)
        return 0;

    if ((stateJustCreated&1u) && !in_region) {
        stateJustCreated&=~1u;
        in_region=FindRegion(X(),Y());
    }

    if ((m_flag&0x4000u) && (m_flag&0x8000u))
        Stop();

    if (blocktick) {
        ANGLE d=Direction();
        d.value=(unsigned char)(d.value+(blocktick>0 ? -32 : 32));
        Rotate(d,DeltaTime());
        if (blocktick<0) ++blocktick; else --blocktick;
    } else if (Goal() && IsCommand(1u)) {
        ANGLE d=DirectionTo(Goal());
        if (Speed()<0.0f)
            d.value=(unsigned char)(d.value+0x80u);
        ANGLE rem=Rotate(d,DeltaTime());
        if (!rem.value)
            ChangeAnimation(2);
    }

    if (Animation()==13)
        ChangeAnimation(0);

    if (IsCommand(1u) || IsCommand(3u))
        return 0;

    if (CurrentTime-(CurrentTime&0x7ffu)>m_tactTime)
        change_state_flag=1;

    if (change_state_flag) {
        change_state_flag=0;
        near_train=Map->FindNearestSprite(0x9015,X(),Y(),350.0f,0);

        if (behave_state==14u) {
            if (!(m_flag&0x80u) && (!m_actions.m_no || m_actions.m_data[m_actions.m_no-1].act==73))
                behave_state=15;
        } else if (behave_state==15u && Random(4)==0) {
            behave_state=5;
            Move(new SPRITE(EmptyVid,X(),Y()+50.0f,Z(),ANGLE((unsigned char)0),0));
        } else if (behave_state==7u && Random(1)!=0) {
            behave_state=7;
        } else if (near_explosion) {
            if (Random(4)!=0)
                behave_state=7;
            else
                behave_state=Random(2)!=0 ? 12u : 10u;
        } else if ((SPRITE*)near_train && ((behave_state!=9u && Random(2)==0) || Random(2)==0)) {
            behave_state=9;
        } else if ((SPRITE*)near_train && Random(1)==0) {
            behave_state=11;
        } else if (behave_state==5u && Random(2)!=0) {
            behave_state=5;
        } else if (behave_state==1u && Random(5)!=0) {
            behave_state=1;
        } else if (behave_state==4u && Random(1)==0) {
            behave_state=4;
        } else if (Random(6)==0) {
            behave_state=4;
        } else if (Random(5)==0) {
            PlaySFX(129);
            behave_state=3;
        } else if (Random(4)==0) {
            PlaySFX(129);
            behave_state=2;
        } else {
            behave_state=Random(4)!=0 ? 5u : 14u;
        }
        near_explosion=0;
    }

    switch (behave_state) {
    case 0:
        if (m_flag&0x80u) Stop();
        RotateHead(Direction());
        break;

    case 13: {
        if (m_flag&0x80u) Stop();
        ANGLE d=Direction();
        d.value=(unsigned char)(d.value-32u);
        ChangeDirection(d);
        RotateHead(Direction());
        break;
    }

    case 12:
        if (m_flag&0x80u) Stop();
        ChangeAnimation(7);
        break;

    case 11:
        if (m_flag&0x80u) Stop();
        if ((SPRITE*)near_train)
            RotateHead(DirectionTo((SPRITE*)near_train));
        break;

    case 2:
        if (m_flag&0x80u) Stop();
        if (HaveLink()) {
            ANGLE d=Link()->Direction();
            d.value=(unsigned char)(d.value-32u);
            RotateHead(d);
        }
        break;

    case 3:
        if (m_flag&0x80u) Stop();
        if (HaveLink()) {
            ANGLE d=Link()->Direction();
            d.value=(unsigned char)(d.value+32u);
            RotateHead(d);
        }
        break;

    case 4:
        if (m_flag&0x80u) Stop();
        if (HaveLink()) {
            Link()->Rotate(Direction(),DeltaTime());
            ChangeAnimation(12);
        }
        break;

    case 1:
        if (Goal()) {
            if (NearDistanceTo(Goal())<150.0f) {
                SetCommand(0,0);
                if (Random(2)==0)
                    Rotate(ANGLE((unsigned char)Random(255)),DeltaTime());
                if (Random(9)==0)
                    ChangeAnimation(11);
                else if (Random(9)==0)
                    ChangeAnimation(9);
                else if (Random(9)!=0)
                    ChangeAnimation(0);
                else
                    ChangeAnimation(6);
            } else {
                Move(Goal());
            }
        }
        break;

    case 5:
        if (!(m_flag&0x80u)) StartMove();
        if (Animation()==2 && Random(2)==0) {
            ANGLE d=Direction(); d.value=(unsigned char)(d.value-32u); Rotate(d,DeltaTime());
        } else if (Animation()==2 && Random(2)==0) {
            ANGLE d=Direction(); d.value=(unsigned char)(d.value+32u); Rotate(d,DeltaTime());
        } else {
            ChangeAnimation(2);
        }
        RotateHead(Direction());
        break;

    case 14:
        if (Goal() || (m_actions.m_no && m_actions.m_data[m_actions.m_no-1].act!=73)) {
            if (!(m_flag&0x80u) && (!m_actions.m_no || m_actions.m_data[m_actions.m_no-1].act==73))
                behave_state=15;
            RotateHead(Direction());
        } else {
            SPRITE* b=FindRobotBuilding();
            if (b) {
                ANGLE bd=b->Direction();
                SPRITE* beacon=new SPRITE(EmptyVid,
                    b->X()-bd.Sin()*32.0f,
                    b->Y()+bd.Cos()*32.0f,
                    b->Z(),ANGLE((unsigned char)0),0);
                Move(beacon);
                m_actions.InsertFirst(ACT(34,(int)b,0,0));
            } else {
                change_state_flag=1;
            }
        }
        break;

    case 8:
        if (!(m_flag&0x80u)) StartMove();
        if (m_speed>=Vid()->m_defaultMaxSpeed)
            m_speed=Vid()->m_defaultMaxSpeed*2.0f;
        if (HaveLink()) {
            Link()->Rotate(Direction(),DeltaTime());
            ChangeAnimation(11);
        }
        break;

    case 7:
        if (!(m_flag&0x80u)) StartMove();
        if (m_speed>=Vid()->m_defaultMaxSpeed)
            m_speed=Vid()->m_defaultMaxSpeed*2.0f;
        if ((SPRITE*)near_train) {
            ANGLE d=DirectionTo((SPRITE*)near_train).GetInversed();
            Rotate(d,DeltaTime());
        }
        if (HaveLink()) {
            Link()->Rotate(Direction(),DeltaTime());
            ChangeAnimation(11);
        }
        break;

    case 9:
        if (m_flag&0x80u) Stop();
        if (HaveLink()) {
            Link()->Rotate((SPRITE*)near_train ? DirectionTo((SPRITE*)near_train) : Direction(),DeltaTime());
            ChangeAnimation(Random(2)!=0 ? 9 : 11);
        }
        break;

    case 10:
        if (m_flag&0x80u) Stop();
        if (HaveLink()) {
            Link()->Rotate((SPRITE*)near_train ? DirectionTo((SPRITE*)near_train) : Direction(),DeltaTime());
            ChangeAnimation(6);
        }
        break;
    }
    return 0;
}

// -----------------------------------------------------------------------------
// A9: BALLOON retail flight/base closure (balloon.cpp / balloon.h)
// -----------------------------------------------------------------------------

int BALLOON::IsItBase(const SPRITE* p)
{
    if (!p)
        return 0;
    if (p->Vid()->m_linkVid!=Vid())
        return 0;
    return ((p->m_flag>>11)&3u)==((m_flag>>11)&3u);
}

int BALLOON::IsItFreeBase(const SPRITE* p)
{
    return IsItBase(p) && !const_cast<SPRITE*>(p)->HaveLink();
}

void BALLOON::MoveToNearestBase()
{
    SPRITE* best=0;
    for (SPRITE* p=Hash->FirstUnit();p;p=Hash->NextUnit()) {
        if (!IsItFreeBase(p))
            continue;
        ENGINE* base=static_cast<ENGINE*>(p);
        if (base->is_busy)
            continue;
        if (!best || NearDistanceTo(p)<NearDistanceTo(best))
            best=p;
    }
    if (best) {
        static_cast<ENGINE*>(best)->is_busy=1;
        Move(best);
    }
}

void BALLOON::ConnectToBase()
{
    SPRITE* base=Goal();
    if (!IsItBase(base)) {
        ChangeAnimation(15);
        return;
    }
    if (base->HaveLink()) {
        SetCommand(0,0);
        zspeed_state=1;
        return;
    }

    m_unknown24=0.0f;
    ChangeCoor(base->X()+base->Vid()->m_linkOffsetX,
               base->Y()+base->Vid()->m_linkOffsetY,
               base->Z()+base->Vid()->m_linkOffsetZ);
    behave=base->Action(94,0,0,0);
    base->AddLink(this);
    static_cast<ENGINE*>(base)->is_busy=1;
    SetCommand(0,0);
}

void BALLOON::ZSpeedInitialization()
{
    const float ground=GetGroundZ();

    if (IsLinked()) {
        zspeed_state=0;
    } else if (IsItBase(Goal())) {
        static_cast<ENGINE*>(Goal())->is_busy=1;
    }

    switch (zspeed_state) {
    case 0:
        if (!IsLinked()) {
            const float cruise=ground+Vid()->m_groundOffset;
            if (Z()<cruise-10.0f)
                m_unknown24=Vid()->m_maxZSpeed;
            else if (Z()>cruise+10.0f)
                m_unknown24=-Vid()->m_maxZSpeed;
            else
                m_unknown24=0.0f;
        } else {
            m_unknown24=0.0f;
        }
        break;

    case 1:
        m_unknown24=Vid()->m_maxZSpeed;
        m_flag&=~0x80u;
        m_speed=0.0f;
        if (Z()>=ground+Vid()->m_groundOffset) {
            zspeed_state=0;
            m_flag|=0x80u;
        }
        break;

    case 2:
        m_unknown24=-Vid()->m_maxZSpeed;
        m_flag&=~0x80u;
        m_speed=0.0f;
        if (!IsItFreeBase(Goal())) {
            zspeed_state=1;
            break;
        }
        ChangeCoor(Goal()->X(),Goal()->Y(),Z());
        Rotate(Goal()->Direction(),DeltaTime());
        if (Z()<=Goal()->Z()+Goal()->Vid()->m_linkOffsetZ)
            ConnectToBase();
        break;

    default:
        break;
    }
}

int BALLOON::IsBalloonMoveFinished()
{
    SPRITE* g=Goal();
    if (g && fabsf(g->X()-X())<10.0f && fabsf(g->Y()-Y())<10.0f)
        return 1;
    return ((m_flag&0x4000u)!=0u && (m_flag&0x8000u)!=0u) ? 1 : 0;
}

void BALLOON::CheckFlightProperties()
{
    if (IsLinked()) {
        if ((CurrentTime&0xfffffc00u)>m_tactTime)
            AddAmmoTick(Const->DirijbanAmmoReloadTime);

        SPRITE* target=Goal();
        bool canDetach=(target!=0);
        if (canDetach && m_parent->Goal()==target &&
            (m_parent->IsCommand(27u) || m_parent->IsCommand(26u)))
            canDetach=false;
        if (canDetach && m_parent->NearDistanceTo(target)>=m_parent->Vid()->m_weapon->m_battleRange)
            canDetach=false;
        if (canDetach && GetTimer()!=0)
            canDetach=false;

        if (!canDetach) {
            ANGLE zero((unsigned char)0);
            if (Rotate(m_parent->Direction(),DeltaTime())==&zero)
                ChangeAnimation(0);
            return;
        }

        if (IsCommand(8u)) {
            SetCommand(3,target);
            must_taran=1;
        }

        DestroyLink(m_parent->Vid()->m_aniChildVid[13]);
        m_parent->m_child=0;
        static_cast<ENGINE*>(m_parent)->is_busy=0;

        SPRITE* oldTarget=target;
        SPRITE* best=m_ptrSprite;
        if (best && best!=oldTarget && NearDistanceTo(best)<Vid()->m_weapon->m_battleRange)
            Attack(best);

        if (oldTarget && oldTarget->Vid()==EmptyVid) {
            m_parent->SetCommand(0,0);
            SPRITE* marker=new SPRITE(EmptyVid,oldTarget->X(),oldTarget->Y(),oldTarget->Z(),ANGLE((unsigned char)0),0);
            Attack(marker);
        }

        ChangeAnimation(2);
        m_flag&=~0x80u;
        zspeed_state=1;
        m_parent=0;
        return;
    }

    SPRITE* best=m_ptrSprite;
    if (best && best!=Goal() && NearDistanceTo(best)<Vid()->m_weapon->m_detectRange && Ammo()>0)
        Attack(best);

    if (Ammo()<=0 || !Goal()) {
        if (!IsItFreeBase(Goal())) {
            if (Goal())
                SetCommand(0,0);
            MoveToNearestBase();
        }
    }

    if (IsItBase(Goal()) && Goal()->HaveLink()) {
        SetCommand(0,0);
        MoveToNearestBase();
    }

    if (zspeed_state!=2 && IsCommand(1u) && IsBalloonMoveFinished() && IsItFreeBase(Goal())) {
        zspeed_state=2;
        ZSpeedInitialization();
    }
}

void BALLOON::FlightToTargetAdditionalActions()
{
    if (!IsItBase(Goal()))
        m_unknown04=AttackTact(DeltaTime());

    SPRITE* g=Goal();
    if (must_taran && g && fabsf(g->X()-X())<10.0f && fabsf(g->Y()-Y())<10.0f) {
        ChangeAnimation(15);
        return;
    }

    if (IsItBase(g) && Ammo()>0 && (behave&1u)) {
        SPRITE* enemy=SeekEnemy();
        if (enemy)
            SetCommand(4,enemy);
    }
}

int BALLOON::SetCommand(int command,SPRITE* new_goal)
{
    must_taran=0;
    if (IsItBase(Goal()))
        static_cast<ENGINE*>(Goal())->is_busy=0;
    if (IsItBase(new_goal))
        static_cast<ENGINE*>(new_goal)->is_busy=1;
    return SPRITE::SetCommand(command,new_goal);
}

void BALLOON::MoveTact()
{
    float x,y,z;
    MoveTactCalcCoor(&x,&y,&z);

    if (zspeed_state) {
        ChangeCoor(X(),Y(),z);
        return;
    }

    if (Goal() && Speed()!=0.0f) {
        ANGLE add((unsigned char)(Speed()<0.0f ? 0x80u : 0u));
        ANGLE wanted=DirectionTo(Goal())+&add;
        ANGLE glide=GlideDirection(wanted);
        Rotate(glide,DeltaTime());
    }

    if ((X()!=x || Y()!=y) && !CanPlaceWithCrushAndGlide(&x,&y,&z)) {
        MoveTactMapLimit(x,y);
        ChangeCoor(x,y,z);
    }
}

void DEPO::AddUnitToQueue(int vidnum)
{
    if (numunit>=unitlim)
        return;

    PLAYER* player=Map->Player(Army());
    VID* vid=Map->Vid(vidnum);
    const int cost=vid->GetBuildTime();
    if (player->GetMoney()<cost)
        return;

    player->AddMoney(-cost);
    unitsToBuild[numunit]=static_cast<unsigned short>(vidnum);
    timers[numunit]=static_cast<unsigned long>(Map->Vid(vidnum)->GetBuildTime()*Const->DepoMillisecondsInSecond);
    pause[numunit]=0;
    ++numunit;
}

int DEPO::ActionBuildUnit(int var1,int var2)
{
    (void)var1;
    (void)var2;

    SPRITE* blocker=0;
    do {
        const int idx=curunit-1;
        VID* buildVid=Map->Vid(static_cast<int>(unitsToBuild[idx]));
        blocker=Hash->CanPlace(buildVid,X(),Y(),Z());
        if (blocker && blocker!=Mouse)
            blocker->ScalarDeletingDestructor(1u);
    } while (blocker && blocker!=Mouse);

    const int idx=curunit-1;
    SPRITE* created=Map->CreateSprite(Map->Vid(static_cast<int>(unitsToBuild[idx])),
                                      X(),Y(),Z(),Direction(),this);
    if (!created) {
        Error(10,"Depo can't create unit",static_cast<unsigned long>(unitsToBuild[idx]));
        return 0;
    }

    if (created->IsSpriteClass(21u))
        static_cast<ENGINE*>(created)->depo_train_num=currentTrainIdentificator;

    --numunit;
    for (int i=idx;i<numunit;++i) {
        unitsToBuild[i]=unitsToBuild[i+1];
        timers[i]=timers[i+1];
        pause[i]=pause[i+1];
    }

    curunit=0;
    created->SetJustBuilded();
    Map->ScriptRun(EvFunctionNumber[23],created,0,0);
    created->PlaySFX(105);

    if (numunit==0 && IsActionStackEmpty()) {
        if (created->IsSpriteClass(21u))
            static_cast<ENGINE*>(created)->lastunitintrain=1;
        ++currentTrainIdentificator;
    } else {
        BuildNextUnit();
    }
    return 0;
}
