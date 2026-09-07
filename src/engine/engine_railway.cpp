#include "mapedit/runtime.hpp"

// Railway positioning / occupancy cluster implemented directly from the
// canonical MapEdit.exe engine.cpp owners.  Keep these as separate owners:
// collapsing them into ENGINE ctor/movement changes the original call graph.


R_DOT* R_MAP::GetDot(int xx,int yy,int zz)
{
    for (int i=0;i<Dots.No();++i) {
        R_DOT* d=*Dots[i];
        if (abs(d->x-xx)<=1 && abs(d->y-yy)<=1 && d->z==zz)
            return d;
    }
    return 0;
}


R_DOT* R_MAP::CreateDot(float fx,float fy,float fz)
{
    const int xx=static_cast<int>(fx);
    const int yy=static_cast<int>(fy);
    const int zz=static_cast<int>(fz);
    R_DOT* dot=GetDot(xx,yy,zz);
    if (!dot) {
        dot=new R_DOT;
        if (!dot)
            MYERROR::LogExit(::Error,"!!!R_MAP::CreateDot- Not enough memory");
        dot->x=xx;
        dot->y=yy;
        dot->z=zz;
        Dots.InsertUnique(&dot);
        if (dot->x>maxx) maxx=dot->x;
        else if (dot->x<minx) minx=dot->x;
        if (dot->y>maxy) maxy=dot->y;
        else if (dot->y<miny) miny=dot->y;
    }
    dot->AddRef();
    return dot;
}

void R_DOT::Break()
{
    isBreakFlag=1;
}

void R_DOT::Link(R_DOT* target)
{
    if (!target || GetLink(target)>=0)
        return;

    ANGLE direct(static_cast<float>(target->x-x),
                 static_cast<float>(target->y-y),
                 static_cast<int*>(0));
    const int dx=x-target->x;
    const int dy=y-target->y;
    const int distance=Sqrt(dx*dx+(dy*dy*9)/4);
    const int targetBackLink=target->noLinks;

    if (noLinks<6) {
        R_DOT_LINK& out=links[noLinks++];
        out.dot=target;
        out.distance=distance;
        out.backLink=targetBackLink;
        out.crossLink=0;
        out.direction=direct;
    } else {
        MYERROR::Log(::Error,"!!!ERROR!!!R_DOT: Too many links in %i,%i,%i",x,y,z);
    }

    direct=direct.GetInversed();
    const int myBackLink=noLinks-1;
    if (target->noLinks<6) {
        R_DOT_LINK& out=target->links[target->noLinks++];
        out.dot=this;
        out.distance=distance;
        out.backLink=myBackLink;
        out.crossLink=0;
        out.direction=direct;
    } else {
        MYERROR::Log(::Error,"!!!ERROR!!!R_DOT: Too many links2 in %i,%i,%i",
                     target->x,target->y,target->z);
    }
}

void R_DOT::Link(float fx,float fy,float fz)
{
    Link(RailMap.GetDot(static_cast<int>(fx),static_cast<int>(fy),static_cast<int>(fz)));
}

namespace {
const float kRailX1[12]={0.0f,1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f,-1.0f,1.0f,1.0f,-1.0f,-1.0f};
const float kRailY1[12]={-1.0f,0.0f,-1.0f,-1.0f,-1.0f,-1.0f,1.0f,1.0f,1.0f,-1.0f,-1.0f,1.0f};
const float kRailX2[12]={0.0f,-1.0f,-1.0f,1.0f,0.0f,0.0f,0.0f,0.0f,-1.0f,-1.0f,1.0f,1.0f};
const float kRailY2[12]={1.0f,0.0f,1.0f,1.0f,1.0f,1.0f,-1.0f,-1.0f,0.0f,0.0f,0.0f,0.0f};
}

int RAIL::Action(int act,int var1,int var2,int var3)
{
    static float z1[4]={0.0f,Vid()->m_hitVerticalOffset,0.0f,0.0f};
    static float z2[4]={0.0f,0.0f,Vid()->m_hitVerticalOffset,0.0f};
    const float altitude=Vid()->m_groundOffset;

    if (act==0x3c) {
        ChangeDirection(ANGLE(static_cast<uint8_t>(var1)));
        if (p1) p1->Release();
        if (p2) p2->Release();

        const unsigned int realDirection=static_cast<unsigned int>(RealDirection());
        const unsigned int railDirection=realDirection%12u;
        const unsigned int railHeight=realDirection/12u;
        VID* vid=Vid();

        p1=RailMap.CreateDot(
            X()+vid->m_footprintWidth*kRailX1[railDirection]/4.0f,
            Y()+vid->m_footprintHeight*kRailY1[railDirection]/4.0f,
            Z()+altitude+z1[railHeight]);
        p2=RailMap.CreateDot(
            X()+vid->m_footprintWidth*kRailX2[railDirection]/4.0f,
            Y()+vid->m_footprintHeight*kRailY2[railDirection]/4.0f,
            Z()+altitude+z2[railHeight]);

        p1->Link(
            X()+vid->m_footprintWidth*kRailX1[railDirection]*3.0f/4.0f,
            Y()+vid->m_footprintHeight*kRailY1[railDirection]*3.0f/4.0f,
            Z()+altitude+z1[railHeight]);
        p2->Link(
            X()+vid->m_footprintWidth*kRailX2[railDirection]*3.0f/4.0f,
            Y()+vid->m_footprintHeight*kRailY2[railDirection]*3.0f/4.0f,
            Z()+altitude+z2[railHeight]);
        p2->Link(p1);

        if (vid->m_aniSpawnMode[15]<vid->m_idx) {
            p1->Break();
            p2->Break();
        }
        return 0;
    }

    if (act==0x55) {
        SPRITE::Action(act,var1,var2,var3);
        if (IsDying()) {
            VID* vid=Vid();
            VID* replacementVid=vid->m_aniChildVid[15];
            if (replacementVid && replacementVid->m_spriteClass==22) {
                if (vid->m_aniSpawnMode[15]>vid->m_idx) {
                    p1->Break();
                    p2->Break();
                }
                Action(0x3e,vid->m_aniSpawnMode[15],0,0);
                ChangeHp(MaxHp());
            }
        }
        return 0;
    }

    return TERRAIN::Action(act,var1,var2,var3);
}

void R_POS::Write(STREAM* file)
{
    file->Write(&dot->x,2);
    file->Write(&dot->y,2);
    file->Write(&dot->z,2);
    R_DOT* linked=Dot2();
    file->Write(&linked->x,2);
    linked=Dot2();
    file->Write(&linked->y,2);
    linked=Dot2();
    file->Write(&linked->z,2);
    int packed=static_cast<int>(static_cast<unsigned int>(pos_real)<<16);
    file->Write(&packed,4);
}

void R_POS::Read(STREAM* file)
{
    short x=0,y=0,z=0;
    file->Read(&x,2);
    file->Read(&y,2);
    file->Read(&z,2);
    dot=RailMap.GetDot(static_cast<int>(x),static_cast<int>(y),static_cast<int>(z));
    file->Read(&x,2);
    file->Read(&y,2);
    file->Read(&z,2);
    if (dot)
        link=dot->GetLink(RailMap.GetDot(static_cast<int>(x),static_cast<int>(y),static_cast<int>(z)));
    if (link<0)
        MYERROR::Log(Error,"!!!ERROR!!!RAIL: Read error");
    file->Read(&pos_real,4);
    pos_real>>=16;
}

void ENGINE::ClearDotBusy()
{
    if (head.dot && head.dot->busyEngine==this)
        head.dot->busyEngine=0;
    if (tail.dot && tail.dot->busyEngine==this)
        tail.dot->busyEngine=0;

    R_DOT* head2=head.Dot2();
    if (head2 && head2->busyEngine==this)
        head2->busyEngine=0;

    R_DOT* tail2=tail.Dot2();
    if (tail2 && tail2->busyEngine==this)
        tail2->busyEngine=0;
}

void ENGINE::SetDotBusy()
{
    ClearDotBusy();
    if (head.dot)
        head.dot->busyEngine=this;
    if (tail.dot && tail.dot!=head.dot)
        tail.dot->busyEngine=this;
}

ENGINE* ENGINE::GetIntersecting()
{
    ENGINE* candidate=head.dot->busyEngine;
    if (candidate && IsTouch(candidate,0))
        return candidate;

    candidate=head.Dot2()->busyEngine;
    if (candidate && IsTouch(candidate,0))
        return candidate;

    candidate=tail.dot->busyEngine;
    if (candidate && IsTouch(candidate,0))
        return candidate;

    candidate=tail.Dot2()->busyEngine;
    if (candidate && IsTouch(candidate,0))
        return candidate;

    return 0;
}

void ENGINE::SetRDot()
{
    ANGLE direct=Direction();
    const float halfLength=Vid()->m_weapon->m_length/2.0f;

    R_DOT* dot=RailMap.GetNearestDot(static_cast<int>(X()),
                                    static_cast<int>(Y()),
                                    static_cast<int>(Z()));
    if (!dot || !dot->noLinks)
        return;

    int link=dot->noLinks;
    for (;;) {
        --link;
        if (link<0)
            break;

        ANGLE high(static_cast<uint8_t>(0x6c));
        ANGLE difference=direct.Difference(&dot->links[link].direction);
        if (difference>&high)
            break;

        ANGLE low(static_cast<uint8_t>(0x14));
        difference=direct.Difference(&dot->links[link].direction);
        if (difference<&low)
            break;
    }

    if (link<0)
        direct=dot->links[0].direction;
    else
        direct=Direction();

    dot->SetNearestPos(
        static_cast<int>(X()+direct.Sin()*halfLength),
        static_cast<int>(Y()-direct.Cos()*halfLength),
        static_cast<int>(Z()),
        &head);

    if (head.Dot2()->DistanceTo(X(),Y(),Z()) < head.dot->DistanceTo(X(),Y(),Z()))
        head.Inverse();

    ANGLE inverse=direct.GetInversed();
    dot->SetNearestPos(
        static_cast<int>(X()+inverse.Sin()*halfLength),
        static_cast<int>(Y()-inverse.Cos()*halfLength),
        static_cast<int>(Z()),
        &tail);

    if (tail.Dot2()->DistanceTo(X(),Y(),Z()) < tail.dot->DistanceTo(X(),Y(),Z()))
        tail.Inverse();

    PullTail(&head);
    CalcCoor();
    ForceLink(GetIntersecting());
    SetDotBusy();
}

void ENGINE::CalcCoor()
{
    const int headLength=head.Length();
    R_DOT* head2=head.Dot2();
    const float headX=static_cast<float>(((head.pos_real*(head2->x-head.dot->x))<<8)/headLength)
                     + static_cast<float>(head.dot->x)*256.0f;
    const float headY=static_cast<float>(((head.pos_real*(head2->y-head.dot->y))<<8)/headLength)
                     + static_cast<float>(head.dot->y)*256.0f;
    const float headZ=static_cast<float>(((head.pos_real*(head2->z-head.dot->z))<<8)/headLength)
                     + static_cast<float>(head.dot->z)*256.0f;

    if (tail.link>=tail.dot->noLinks) {
        Error(10,const_cast<char*>("рельсы неправильные link>nolink"),0);
        tail.link=tail.dot->noLinks-1;
    }

    R_DOT* tail2=tail.Dot2();
    if (!tail2) {
        Error(10,const_cast<char*>("рельсы неправильные нет tail.Dot2()"),0);
        return;
    }

    const int tailLength=tail.Length();
    const float tailX=static_cast<float>(((tail.pos_real*(tail2->x-tail.dot->x))<<8)/tailLength)
                     + static_cast<float>(tail.dot->x)*256.0f;
    const float tailY=static_cast<float>(((tail.pos_real*(tail2->y-tail.dot->y))<<8)/tailLength)
                     + static_cast<float>(tail.dot->y)*256.0f;
    const float tailZ=static_cast<float>(((tail.pos_real*(tail2->z-tail.dot->z))<<8)/tailLength)
                     + static_cast<float>(tail.dot->z)*256.0f;

    ChangeCoor((headX+tailX)/512.0f,
               (headY+tailY)/512.0f,
               (headZ+tailZ)/512.0f);

    if (stateInvMove) {
        ANGLE newDirection((tailX-headX+128.0f)/256.0f,
                           (tailY-headY+128.0f)*3.0f/512.0f);
        ChangeDirection(newDirection);
    } else {
        ANGLE newDirection((headX-tailX+128.0f)/256.0f,
                           (headY-tailY+128.0f)*3.0f/512.0f);
        ChangeDirection(newDirection);
    }
}

int ENGINE::ForceLink(ENGINE* engine)
{
    int reverse=0;
    if (!engine || InTrain(engine))
        return 1;

    if (engine->prev_engine || engine->next_engine) {
        const float firstDistance=DistanceTo(engine->FirstEngine());
        const float lastDistance=DistanceTo(engine->LastEngine());
        reverse=(lastDistance>firstDistance) ? 1 : 0;
    } else {
        const float headDistance=engine->head.Dot2()->DistanceTo(X(),Y(),Z());
        const float tailDistance=engine->tail.Dot2()->DistanceTo(X(),Y(),Z());
        reverse=(tailDistance>headDistance) ? 1 : 0;
    }

    if (reverse)
        engine->ReverseTrain();

    engine=engine->LastEngine();
    engine->next_engine=this;
    prev_engine=engine;

    head=engine->tail.GetInversed();

    ANGLE tailDirect=engine->tail.Direct();
    tail.dot=head.dot->links[head.dot->GetLink(tailDirect)].dot;
    tail.link=tail.dot->GetLink(tailDirect);

    ANGLE limit(static_cast<uint8_t>(0x7f));
    ANGLE ownDirection=Direction();
    ANGLE headDirection=head.Direct();
    ANGLE difference=ownDirection.Difference(&headDirection);
    if (difference>&limit)
        stateInvMove=1;

    PullTail(&head);
    CalcCoor();
    return 0;
}

int ENGINE::NeedAddAmmo()
{
    const int maxAmmo=MaxAmmo();
    if (!maxAmmo)
        return 0;
    return 100-(Ammo()*100)/maxAmmo;
}

int ENGINE::NeedRepairByRepair()
{
    int need;
    if (Hp()<MaxHp()) {
        if (IsLinkDestroy()) {
            need=100-(Hp()*100)/(MaxHp()+Vid()->m_linkVid->GetMaxHp(Army()));
        } else {
            need=100-(Hp()*100)/MaxHp();
        }
        return need;
    }

    if (IsLinkDestroy())
        return Vid()->m_idx!=0x23 ? 50 : 0;

    const int linkedDamaged=HaveLink() && !Link()->IsSpriteClass(9) && Link()->Hp()<Link()->MaxHp();
    need=linkedDamaged ? 1 : 0;
    if (need) {
        need=100-((Hp()+Link()->Hp())*100)/(MaxHp()+Link()->MaxHp());
    }
    return need;
}

int ENGINE::RepairByRepair(ENGINE* eng_to_repair)
{
    if (!eng_to_repair)
        return 0;

    int repaired=(eng_to_repair->Action(0x55,-Const->RepairSpeed,0,0)==0) ? 1 : 0;
    if (repaired)
        return repaired;

    if (eng_to_repair->IsLinkDestroy()) {
        if (eng_to_repair->linkhp<0)
            eng_to_repair->linkhp=0;
        eng_to_repair->linkhp+=Const->RepairSpeed;
        if (eng_to_repair->linkhp>=eng_to_repair->Vid()->m_linkVid->GetMaxHp(eng_to_repair->Army())) {
            const int repairChildFighter=1;
            eng_to_repair->Repair(repairChildFighter);
            linkhp=-1;
        }
    }
    return repaired;
}

void ENGINE::AddAmmoTact()
{
    ENGINE* selected=0;
    int bestNeed=0;
    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        if (engine->Army()!=Army())
            continue;
        const int need=engine->NeedAddAmmo();
        if (need>bestNeed && engine->Vid()->m_idx!=0x55) {
            bestNeed=need;
            selected=engine;
        }
    }

    if (selected) {
        selected->AddAmmoTick(Const->AmmoReloadTime);
        if (Animation()!=6)
            ChangeAnimation(6);
    } else if (Animation()==6) {
        ChangeAnimation(0);
    }
}

void ENGINE::RepairTact()
{
    ENGINE* selected=0;
    int bestNeed=0;
    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        if (engine->Army()!=Army())
            continue;
        const int need=engine->NeedRepairByRepair();
        if (need>bestNeed) {
            bestNeed=need;
            selected=engine;
        }
    }

    if (selected) {
        RepairByRepair(selected);
        if (Animation()!=6)
            ChangeAnimation(6);
    } else if (Animation()==6) {
        ChangeAnimation(0);
    }
}

void ENGINE::MoveTact()
{
    MoveEngineTact();

    if ((CurrentTime&0xffffc000u)>PrevCurrentTime)
        m_flag&=~2u;

    if ((CurrentTime&0xfffffc00u)>PrevCurrentTime && IsFirst()) {
        TRAIN_INFO info(this);
        if (info.percentAmmo<=10)
            Map->ScriptRun(EvFunctionNumber[1],this,0,0);
        if (info.hp<info.max_hp/2)
            Map->ScriptRun(EvFunctionNumber[3],this,0,0);
    }

    if ((CurrentTime&0xfffffc00u)>PrevCurrentTime) {
        if (Vid()->m_idx==0x55)
            RepairTact();
        else if (Vid()->m_idx==0x2d)
            AddAmmoTact();
        else if (Vid()->m_idx==0x23)
            is_busy=0;
    }
}

void ENGINE::PullTail(const R_POS* old_head)
{
    R_POS* oldHead=const_cast<R_POS*>(old_head);
    const float length=Vid()->m_weapon->m_length;
    const int intLength=static_cast<int>(length);

    if (static_cast<float>(head.pos_real)>length) {
        tail=head.GetInversed();
        tail.pos_real=intLength+head.Length()-head.pos_real;
        return;
    }

    if (tail.dot==head.dot) {
        tail.pos_real=intLength-head.pos_real;
        return;
    }

    if (oldHead->dot==head.dot) {
        int linkIndex=head.dot->GetLink(tail.dot);
        if (linkIndex<0) {
            Error(10,const_cast<char*>("rail 1"),0);
            ANGLE inverse=head.Direct().GetInversed();
            linkIndex=head.dot->GetLink(inverse);
            tail.dot=head.dot->links[linkIndex].dot;
        }

        const int total=head.pos_real+head.dot->links[linkIndex].distance;
        if (static_cast<float>(total)>length) {
            if (head.link==linkIndex) {
                tail.pos_real=head.dot->links[linkIndex].distance-(head.pos_real-intLength);
            } else {
                tail.dot=head.dot;
                tail.link=linkIndex;
                tail.pos_real=intLength-head.pos_real;
            }
        } else {
            tail.pos_real=intLength-(head.pos_real+head.dot->links[linkIndex].distance);
        }
        return;
    }

    if (oldHead->dot==tail.dot) {
        const int total=head.pos_real+oldHead->Length();
        if (static_cast<float>(total)>length) {
            tail=oldHead->GetInversed();
            tail.pos_real=intLength-head.pos_real;
        } else {
            tail.pos_real=intLength-(head.pos_real+oldHead->Length());
        }
        return;
    }

    const int total=head.pos_real+oldHead->Length();
    if (static_cast<float>(total)>length) {
        MYERROR::Log(::Error,"zmdots5");
        tail=oldHead->GetInversed();
        tail.pos_real=intLength-head.pos_real;
        return;
    }

    MYERROR::Log(::Error,"zmdots6");
    tail.link=oldHead->dot->GetLink(tail.dot);
    tail.dot=oldHead->dot;
    tail.pos_real=intLength-(head.pos_real+oldHead->Length());
    if (tail.link<0) {
        Error(10,const_cast<char*>("rail 2"),0);
        ANGLE inverse=oldHead->Direct().GetInversed();
        tail.link=tail.dot->GetLink(inverse);
    }
}

int ENGINE::IsTouch(ENGINE* eng,int frommovetact)
{
    if (!eng || InTrain(eng))
        return 0;

    if (head.dot) {
        if (eng->head.dot && head.Dot2()==eng->head.dot && head.dot==eng->head.Dot2()) {
            const int opposite=head.Length()-eng->head.pos_real;
            if (frommovetact ? (head.pos_real>opposite) : (head.pos_real>=opposite))
                return 1;
        }

        if (eng->tail.dot && head.Dot2()==eng->tail.dot && head.dot==eng->tail.Dot2()) {
            const int opposite=head.Length()-eng->tail.pos_real;
            if (frommovetact ? (head.pos_real>opposite) : (head.pos_real>=opposite))
                return 2;
        }
    }

    if (tail.dot) {
        if (eng->head.dot && tail.Dot2()==eng->head.dot && tail.dot==eng->head.Dot2()) {
            const int opposite=tail.Length()-eng->head.pos_real;
            if (frommovetact ? (tail.pos_real>opposite) : (tail.pos_real>=opposite))
                return 3;
        }

        if (eng->tail.dot && tail.Dot2()==eng->tail.dot && tail.dot==eng->tail.Dot2()) {
            const int opposite=tail.Length()-eng->tail.pos_real;
            if (frommovetact ? (tail.pos_real>opposite) : (tail.pos_real>=opposite))
                return 4;
        }
    }

    if (!frommovetact) {
        if (head.dot) {
            if (head.dot==eng->head.dot)
                return 100;
            if (head.dot==eng->tail.dot)
                return 200;
        }
        if (tail.dot) {
            if (tail.dot==eng->head.dot)
                return 300;
            if (tail.dot==eng->tail.dot)
                return 400;
        }
        return 0;
    }

    if (head.dot) {
        if (head.dot==eng->head.dot)
            return head.link==eng->head.link ? 2 : 1;
        if (head.dot==eng->tail.dot)
            return 200;
    }
    if (tail.dot) {
        if (tail.dot==eng->head.dot)
            return 300;
        if (tail.dot==eng->tail.dot)
            return 400;
    }
    return 0;
}

int R_DOT::IsNeedPush(int link)
{
    return oneDirection==link && pushed!=0 ? 1 : 0;
}

int R_DOT::IsOneDirect(int link)
{
    return oneDirection==link ? 1 : 0;
}

void R_DOT::UnBreak()
{
    isBreakFlag=0;
}

int R_POS::Link2()
{
    return dot ? dot->links[link].backLink : 0;
}

void RAIL::UnBreak(R_DOT* railDot)
{
    if (p1!=railDot && p2!=railDot)
        return;

    VID* vid=Vid();
    const int replacement=vid->m_aniSpawnMode[15];
    if (replacement && replacement<vid->m_idx) {
        Action(62,replacement,0,0);
        ChangeHp(MaxHp());
    }
}

ENGINE* ENGINE::GetBadIntersecting()
{
    if (head.dot) {
        R_DOT_LINK* cross=head.dot->links[head.link].crossLink;
        if (cross) {
            ENGINE* engine=cross->dot->busyEngine;
            if (engine && !engine->InTrain(this))
                return engine;

            R_DOT_LINK* back=&cross->dot->links[cross->backLink];
            engine=back->dot->busyEngine;
            if (engine && !engine->InTrain(this))
                return engine;
        }
    }

    R_DOT* linked=head.Dot2();
    if (linked) {
        const int backLink=head.dot->links[head.link].backLink;
        R_DOT_LINK* cross=linked->links[backLink].crossLink;
        if (cross) {
            ENGINE* engine=cross->dot->busyEngine;
            if (engine && !engine->InTrain(this))
                return engine;

            R_DOT_LINK* back=&cross->dot->links[cross->backLink];
            engine=back->dot->busyEngine;
            if (engine && !engine->InTrain(this))
                return engine;
        }
    }
    return 0;
}

int ENGINE::IsBadTouch(ENGINE* /*eng*/)
{
    // Canonical MapEdit.exe owner is literally `return 1;`.
    return 1;
}

ENGINE* ENGINE::GetForwardIntersecting(int* typeofintersecting)
{
    ENGINE* engine=head.dot->busyEngine;
    if (engine) {
        *typeofintersecting=IsTouch(engine,1);
        if (*typeofintersecting)
            return engine;
    }

    R_DOT* linked=head.Dot2();
    engine=linked->busyEngine;
    if (engine) {
        *typeofintersecting=IsTouch(engine,1);
        if (*typeofintersecting)
            return engine;
    }

    *typeofintersecting=0;
    return 0;
}

void ENGINE::MT_SpeedProcessing(float* abs_speed)
{
    if (isPushed && maxSpeed==0.0f) {
        *abs_speed=0.035f;
        acceleration=0;
        return;
    }

    if (maxSpeed>*abs_speed) {
        if (!acceleration)
            ReCalcMoveParameters();
        if (acceleration) {
            const int scaled=acceleration*static_cast<int>(CurrentTime-PrevCurrentTime);
            *abs_speed=static_cast<float>(scaled)/1000000.0f*2.0f+*abs_speed;
        }
        if (*abs_speed<maxSpeed)
            return;
        *abs_speed=maxSpeed;
        acceleration=0;
        return;
    }

    if (maxSpeed>=*abs_speed) {
        acceleration=0;
        return;
    }

    acceleration=-static_cast<int>((*abs_speed*1000.0f+10.0f)/2.0f);
    {
        const int scaled=acceleration*static_cast<int>(CurrentTime-PrevCurrentTime);
        *abs_speed=static_cast<float>(scaled)/1000000.0f*2.0f+*abs_speed;
    }
    if (*abs_speed>maxSpeed)
        return;

    *abs_speed=maxSpeed;
    acceleration=0;
}

int ENGINE::CanLinkWithEngine(ENGINE* eng)
{
    if (!eng)
        return 0;

    if ((eng->InTrain(GetMoveTarget()) && IsCommand(26)) ||
        (InTrain(eng->GetMoveTarget()) && eng->IsCommand(26)))
        return 1;

    if ((isPushed || eng->isPushed) && Army()==eng->Army())
        return 1;

    return 0;
}

int ENGINE::MT_IntersectingProcessing(const R_POS* old_head,float* abs_speed)
{
    int result=0;
    int touch=-1;
    ENGINE* hit=GetForwardIntersecting(&touch);
    if (hit) {
        head=*old_head;
        if (CanLinkWithEngine(hit)) {
            if (hit->next_engine || touch==1 || touch==4)
                hit->ReverseTrain();

            if (Army()==hit->Army()) {
                if (IsCommand(26) && hit->InTrain(Goal())) {
                    if (!hit->IsCommand(26) || !InTrain(hit->Goal()))
                        Action(70,0,0,0);
                } else {
                    hit->Action(70,0,0,0);
                }
            }

            if (hit->next_engine) {
                Error(10,const_cast<char*>("can't link"),0);
            } else {
                hit->next_engine=this;
                prev_engine=hit;
            }

            SetCommandToTrain(0,0,0,0);
            ChangeAnimation(11);

            if (Army()!=hit->Army()) {
                for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
                    if (engine->Army()!=0)
                        engine->DeleteAttackToEngine();
                }

                ENGINE* who=this;
                if (hit->Army()==1)
                    who=hit;
                Map->ScriptRun(EvFunctionNumber[21],who,0,0);
            }
        } else {
            result=1;
            *abs_speed=Clash(hit,touch);
            Map->ScriptRun(EvFunctionNumber[22],this,hit,0);
        }
    } else {
        hit=GetBadIntersecting();
        if (hit) {
            result=1;
            if (*abs_speed>0.2f)
                PlaySFX(148);
            head=*old_head;
            *abs_speed=0.0f;
            maxSpeed=-maxSpeed;
        }
    }
    return result;
}

void ENGINE::DeleteAttackToEngine()
{
    for (SPRITE* sprite=Hash->FirstUnit();sprite;sprite=Hash->NextUnit()) {
        if (sprite->Goal()==this) {
            if (sprite->IsSpriteClass(21) &&
                (sprite->Command()==28 || sprite->Command()==29)) {
                static_cast<ENGINE*>(sprite)->SetCommandToTrain(0,0,0,0);
            } else {
                const int command=sprite->Command();
                if (command==5 || command==3 || command==4)
                    sprite->SetCommand(0,static_cast<SPRITE*>(0));
            }
            continue;
        }

        SPRITE* link=sprite->Link();
        if (!link || link->Goal()!=this)
            continue;

        const int command=link->Command();
        if (command==5 || command==3 || command==4 || command==28 || command==29)
            link->SetCommand(0,static_cast<SPRITE*>(0));
    }
}

int ENGINE::ReachTheTarget()
{
    int result=2;
    SPRITE* target=Goal();
    if (!target || (!IsCommand(28) && !IsCommand(29)))
        return 2;

    if (commandEngine) {
        if (commandEngine->HaveFightLink()) {
            SPRITE* weapon=commandEngine->Link();
            const float distance=weapon->NearDistanceTo(target);
            const float range=weapon->Vid()->m_weapon->m_battleRange-10.0f;
            return distance>range ? 0 : 1;
        }

        if (commandEngine->Vid()->m_idx==0x23) {
            const float distance=commandEngine->NearDistanceTo(target);
            const float range=commandEngine->Vid()->m_weapon->m_battleRange-10.0f;
            return distance>range ? 0 : 1;
        }
        return result;
    }

    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        if (engine->HaveFightLink()) {
            SPRITE* weapon=engine->Link();
            const float distance=weapon->NearDistanceTo(target);
            const float range=weapon->Vid()->m_weapon->m_battleRange-10.0f;
            if (distance>range)
                return 0;
            result=1;
        } else if (engine->Vid()->m_idx==0x23) {
            const float distance=engine->NearDistanceTo(target);
            const float range=engine->Vid()->m_weapon->m_battleRange-10.0f;
            if (distance>range)
                return 0;
            result=1;
        }
    }
    return result;
}

float ENGINE::Clash(ENGINE* eng,int typeofintersecting)
{
    TRAIN_INFO this_info(this);
    TRAIN_INFO other_info(eng);

    const float this_speed=fabsf(Speed());
    const float other_speed=fabsf(eng->Speed());
    float new_speed=(this_info.weight*this_speed+other_info.weight*other_speed)/
                    (this_info.weight+other_info.weight);
    if (new_speed>0.001f && new_speed<0.01f)
        new_speed=0.01f;

    float impact_speed;
    if (typeofintersecting==2 || typeofintersecting==3)
        impact_speed=fabsf(this_speed-other_speed);
    else
        impact_speed=this_speed+other_speed;

    if (typeofintersecting==1 || typeofintersecting==3)
        eng->ReverseTrain();

    for (ENGINE* e=eng->FirstEngine();e;e=e->NextEngine())
        e->SetAbsSpeed(new_speed);

    const int crash_command=
        (IsCommand(27) && eng->InTrain(Goal())) ||
        (eng->IsCommand(27) && InTrain(eng->Goal()));

    if (impact_speed>Const->SafeClashSpeed && crash_command) {
        ChangeAnimation(12);
        PlaySFX(16);

        // Retail x87 order:
        // impact * 1000.0f * 3.0f / 2.0f -> _ftol -> signed / 2.
        int damage=static_cast<int>(impact_speed*1000.0f*3.0f/2.0f);
        damage/=2;
        const int base_damage=damage;

        if (eng->Vid()->m_idx==0x61)
            eng->Action(85,eng->MaxHp()+10,0,0);
        else
            eng->Action(85,damage,0,0);

        for (ENGINE* e=eng->PrevEngine();e && damage>=2;e=e->PrevEngine()) {
            damage/=2;
            e->Action(85,damage,0,0);
        }

        damage=base_damage;
        if (Vid()->m_idx==0x61)
            Action(85,MaxHp()+10,0,0);
        else
            Action(85,damage,0,0);

        if (PrevEngine()) {
            for (ENGINE* e=PrevEngine();e && damage>=2;e=e->PrevEngine()) {
                damage/=2;
                e->Action(85,damage,0,0);
            }
        } else {
            for (ENGINE* e=NextEngine();e && damage>=2;e=e->NextEngine()) {
                damage/=2;
                e->Action(85,damage,0,0);
            }
        }
    } else {
        PlaySFX(19);
    }
    return 0.0f;
}

void ENGINE::MT_PullCoordinates(R_POS* old_head,float abs_speed,int acceleration_)
{
    for (ENGINE* engine=this;engine;engine=engine->next_engine) {
        engine->ClearDotBusy();
        engine->PullTail(old_head);

        if (engine->next_engine) {
            *old_head=engine->next_engine->head;
            R_POS inverse=engine->tail.GetInversed();
            engine->next_engine->head=inverse;
        }

        engine->CalcCoor();
        if ((engine->lastx!=0.0f || engine->lasty!=0.0f) &&
            fabsf(engine->lastx-engine->X())>30.0f) {
            MYERROR::Log(::Error,"\xF1\xF5\xEB\xE0\xEF\xFB\xE2\xE0\xED\xE8\xE5 \xE2\xE0\xE3\xEE\xED\xEE\xE2 zm-error - kawabanga - begin");
        }

        engine->lastx=engine->X();
        engine->lasty=engine->Y();
        engine->lastz=engine->Z();
        engine->SetDotBusy();
        engine->SetSpeed(engine->stateInvMove ? -abs_speed : abs_speed);
        engine->acceleration=acceleration_;
    }

    if (abs_speed==0.0f)
        return;

    const float x=X();
    const float y=Y();
    const float z=Z();
    VID* vid=Vid();
    for (SPRITE* sprite=Hash->FirstInBox(x-vid->m_snapOffsetX,
                                          y-vid->m_snapOffsetY,
                                          x+vid->m_snapOffsetX,
                                          y+vid->m_snapOffsetY);
         sprite;
         sprite=Hash->NextInBox()) {
        if (sprite==this)
            continue;
        if (!sprite->IsCross(vid,x,y,z))
            continue;
        if (!sprite->Vid()->PropCrush())
            continue;
        sprite->Action(85,5,0,0);
    }
}

// Canonical railway movement state machine.  This deliberately keeps the
// retail command numbers/branch ordering instead of folding them into newer
// engine abstractions: those details are observable in the reference behavior.
void ENGINE::MoveEngineTact()
{
    int nostep=0xffff;
    R_POS old_head=head;

    if (!head.dot || !tail.dot || !IsFirst())
        return;

    if (Vid()->m_idx!=0x55 && head.Dot2() &&
        head.Dot2()->isMined-4==Army() &&
        head.pos_real>head.Length()/2) {
        Stop();
        m_speed=0.0f;
        ReverseTrain();
        return;
    }

    int saw_flag1=0;
    ENGINE* scan=this;
    for (;scan;scan=scan->NextEngine()) {
        if (scan->head.dot && scan->head.dot->IsNeedPush(scan->head.link))
            break;

        if (scan->head.Dot2() && scan->head.Dot2()->IsOneDirect(scan->head.Link2())) {
            if (m_speed!=0.0f) {
                if (IsCommand(23) &&
                    (!noFindedPath || head.dot->links[findedway[0]].dot==head.Dot2())) {
                    SetCommandToTrain(0,0,0,0);
                } else {
                    m_flag&=~0x80u;
                }
                m_speed=0.0f;
                PlaySFX(148);
            }
            if (maxSpeed>0.0f)
                maxSpeed=-maxSpeed;
            ReverseTrain();
            return;
        }

        scan->isPushed=0;
        saw_flag1|=(scan->m_flag&1u)!=0u;
    }

    if (scan) {
        for (ENGINE* e=this;e;e=e->NextEngine())
            e->isPushed=1;
    } else if (saw_flag1) {
        for (ENGINE* e=this;e;e=e->NextEngine()) {
            if (!(e->m_flag&1u))
                continue;
            e->m_flag&=~1u;
            if (e->Vid()->m_weapon->m_power!=0.0f)
                Map->Player(Army())->AddUnitToStateBar(e);
            if (e->lastunitintrain) {
                Map->ScriptRun(EvFunctionNumber[4],e,0,0);
                e->lastunitintrain=0;
            }
        }
    }

    if (!isPushed) {
        if (!(m_flag&0x80u) && m_speed==0.0f) {
            if (IsCommand(23) || IsCommand(26) || IsCommand(27) || IsCommand(25))
                StartMove();

            if (IsCommand(24)) {
                ENGINE* e=FirstEngine();
                while (e && (e->Vid()->m_idx!=0x55 || e->was_near_mine))
                    e=e->NextEngine();
                if (e)
                    StartMove();
            }
        }

        if (!(m_flag&0x80u) &&
            (IsCommand(28) || IsCommand(29)) &&
            Random(4)==0 && ReachTheTarget()!=1) {
            StartMove();
        }

        if (maxSpeed==0.0f && (m_flag&0x80u)) {
            ReCalcMoveParameters();
            if (maxSpeed==0.0f && m_speed==0.0f)
                SetCommandToTrain(0,0,0,0);
        }
    }

    SPRITE* target=GetMoveTarget();
    if (goal)
        target=0;

    float abs_speed=fabsf(m_speed);
    MT_SpeedProcessing(&abs_speed);
    if (abs_speed<0.0f) {
        m_speed=0.0f;
        ReverseTrain();
        ReCalcMoveParameters();
        return;
    }

    if (head.dot->IsBreak() && abs_speed>Const->RailRepairSpeed) {
        if (Vid()->m_idx!=0x55)
            abs_speed=Const->RailRepairSpeed;
        Map->CreateSprite(Map->Vid(0x24c),X(),Y(),Z(),ANGLE(static_cast<unsigned char>(0)),this);
    }

    const int dt=static_cast<int>(CurrentTime-PrevCurrentTime);
    const int delta=static_cast<int>(static_cast<float>(dt)*abs_speed*64.0f*1000.0f);
    head.pos_fract+=delta;
    if (head.pos_fract<0) {
        head.pos_fract=0;
    } else if (head.pos_fract>0xffff) {
        head.pos_real+=head.pos_fract>>16;
        head.pos_fract&=0xffff;
    }

    if (head.pos_real>head.Length()) {
        if (head.dot->IsBreak()) {
            const float bx=static_cast<float>(head.dot->x);
            const float by=static_cast<float>(head.dot->y);
            for (SPRITE* sprite=Hash->FirstInBox(bx-100.0f,by-60.0f,bx+100.0f,by+60.0f);
                 sprite;
                 sprite=Hash->NextInBox()) {
                if (sprite->IsSpriteClass(22))
                    static_cast<RAIL*>(sprite)->UnBreak(head.dot);
            }
            head.dot->UnBreak();
        }

        if (head.Dot2()->noLinks<2) {
            abs_speed=0.0f;
            head.pos_real=head.Length();
            if (IsCommand(23) &&
                (!noFindedPath || head.dot->links[findedway[0]].dot==head.Dot2())) {
                SetCommandToTrain(0,0,0,0);
            } else {
                Stop();
            }
        } else {
            nostep=head.DoStep(goal,target,this);

            SPRITE* flagman=Map->Flagman();
            if (InTrain(flagman) && flagman->Army()==0)
                CreatePathDots(head.dot);

            // The original performs two Goal() calls in the following path even
            // though their return values are not consumed by later instructions.
            // Preserve those calls rather than silently optimizing the state
            // machine into a different call graph.
            if (!IsCommand(27) && head.dot->busyEngine &&
                head.dot->busyEngine->InTrain(Goal())) {
                (void)Goal();
            } else if (head.dot==R_DOT::FindedDot && IsCommand(27)) {
                (void)Goal();
            }

            int allow_nostep_direction=1;
            R_DOT* next_dot=head.Dot2();
            if (next_dot->busyEngine && !next_dot->busyEngine->InTrain(Goal()))
                allow_nostep_direction=0;

            if (allow_nostep_direction &&
                (IsCommand(27) || nostep!=0 || IsCommand(0) || IsCommand(24)) &&
                nostep<0 && maxSpeed>0.0f) {
                maxSpeed=-maxSpeed;
            }

            next_dot=head.Dot2();
            if (Vid()->m_idx!=0x55 && next_dot && next_dot->isMined-4==Army()) {
                abs_speed/=2.0f;
                Stop();
            }

            const int reached=(goal && head.Dot2()==goal) ||
                (nostep!=0 && head.Dot2()==R_DOT::FindedDot);

            if (reached) {
                if (IsCommand(24)) {
                    for (ENGINE* e=this;e;e=e->next_engine) {
                        if (e->Vid()->m_idx==0x55) {
                            e->was_near_mine=1;
                            Stop();
                        }
                    }
                    if (m_flag&0x80u)
                        SetCommandToTrain(0,0,0,0);
                } else if (IsCommand(23)) {
                    R_DOT* d=head.Dot2();
                    if (!(Vid()->m_idx==0x55 && d && d->isMined-4==Army())) {
                        SetCommandToTrain(0,0,0,0);
                        Map->ScriptRun(EvFunctionNumber[8],this,0,0);
                    }
                } else if (IsCommand(25)) {
                    Stop();
                    StartMove();
                } else if (IsCommand(28) || IsCommand(29)) {
                    if (ReachTheTarget()==1)
                        Stop();
                }
            } else {
                if (IsCommand(28) || IsCommand(29)) {
                    if (ReachTheTarget()==1)
                        Stop();
                } else if (head.dot->noLinks<2 || head.Dot2()->noLinks<2) {
                    if (IsCommand(23) &&
                        (!noFindedPath || head.dot->links[findedway[0]].dot==head.Dot2())) {
                        SetCommandToTrain(0,0,0,0);
                    } else {
                        Stop();
                    }
                } else if (head.Dot2()->busyEngine &&
                           !head.Dot2()->busyEngine->InTrain(Goal())) {
                    Stop();
                }
            }
        }
    }

    (void)MT_IntersectingProcessing(&old_head,&abs_speed);
    for (ENGINE* e=FirstEngine();e;e=e->NextEngine())
        e->ClearDotBusy();
    MT_PullCoordinates(&old_head,abs_speed,acceleration);
}

ENGINE::~ENGINE()
{
    if (!globaldeleting)
        Map->ScriptRun(EvFunctionNumber[24],this,0,0);

    if (commandEngine==this) {
        SetCommandToTrain(0,0,0,0);
    } else if (commandEngine) {
        commandEngine->Release();
        commandEngine=0;
    }

    if (static_cast<SPRITE*>(this)==Map->Flagman())
        ReleasePathDots();

    if (!globaldeleting)
        ClearDotBusy();

    if (prev_engine) {
        prev_engine->next_engine=0;
        prev_engine->ReCalcMoveParameters();
    }

    if (next_engine) {
        next_engine->prev_engine=0;
        next_engine->ReCalcMoveParameters();
    }

    if (!globaldeleting) {
        if (prev_engine && next_engine)
            Map->ScriptRun(EvFunctionNumber[5],prev_engine,next_engine,0);
        if (IsPowerEngine())
            Map->ScriptRun(EvFunctionNumber[7],this,0,0);
        if (!prev_engine && !next_engine)
            Map->ScriptRun(EvFunctionNumber[6],this,0,0);

        if (lastunitintrain) {
            SPRITE* matching=0;
            for (SPRITE* sprite=Hash->FirstUnit();sprite;sprite=Hash->NextUnit()) {
                if (sprite->IsSpriteClass(21) && sprite->Army()==Army() &&
                    static_cast<ENGINE*>(sprite)->depo_train_num==depo_train_num) {
                    matching=sprite;
                    break;
                }
            }
            if (matching)
                Map->ScriptRun(EvFunctionNumber[4],matching,0,0);
        }
    }
}

void* ENGINE::ScalarDeletingDestructor(unsigned int flags)
{
    ENGINE* result=this;
    this->ENGINE::~ENGINE();
    if (flags&1u)
        ::operator delete(result);
    return result;
}

void ENGINE::DeletePointerToSprite(SPRITE* sprite)
{
    if (commandEngine==sprite)
        SetCommandToTrain(0,0,0,0);

    if (sprite->IsSpriteClass(21) && Goal()==sprite && IsCommand(26)) {
        ENGINE* engine=static_cast<ENGINE*>(sprite);
        if (engine->NextEngine())
            SetCommandToTrain(26,engine->NextEngine(),0,0);
        else if (engine->PrevEngine())
            SetCommandToTrain(26,engine->PrevEngine(),0,0);
    }

    if (Goal()==sprite)
        SetCommandToTrain(0,0,0,0);

    SPRITE::DeletePointerToSprite(sprite);
}

void ENGINE::DrawGoalLine()
{
    if (patrolto1) {
        Graph->Line(ScreenX()+2.0f,ScreenY()+2.0f,
                    patrolto1->ScreenX()+2.0f,patrolto1->ScreenY()+2.0f,
                    GRAPH::WHITE);
    }
    if (patrolto2) {
        Graph->Line(ScreenX()+2.0f,ScreenY()+2.0f,
                    patrolto2->ScreenX()+2.0f,patrolto2->ScreenY()+2.0f,
                    GRAPH::WHITE);
    }
    if (Goal())
        Graph->Line(ScreenX(),ScreenY(),Goal()->ScreenX(),Goal()->ScreenY(),GRAPH::GREEN);

    SPRITE* linked=Link();
    if (linked && linked->Goal()) {
        Graph->Line(linked->ScreenX(),linked->ScreenY(),
                    linked->Goal()->ScreenX(),linked->Goal()->ScreenY(),GRAPH::RED);
    }

    if (goal)
        Graph->Line(ScreenX(),ScreenY(),goal->ScreenX(),goal->ScreenY(),GRAPH::BLUE);

    Graph->UnLock();
}


// Retail debug visualization owner derived from canonical engine.cpp reference implementation.
void ENGINE::DebugDraw()
{
    for (ENGINE* engine=FirstEngine();engine;engine=engine->NextEngine()) {
        SPRITE* spriteGoal=engine->Goal();
        if (spriteGoal) {
            Graph->Line(engine->ScreenX(),engine->ScreenY(),
                        spriteGoal->ScreenX(),spriteGoal->ScreenY(),GRAPH::BLUE);
        }

        SPRITE* target=static_cast<SPRITE*>(engine->m_ptrSprite);
        if (!target && engine->HaveFightLink()) {
            SPRITE* linked=engine->Link();
            if (linked)
                target=static_cast<SPRITE*>(linked->m_ptrSprite);
        }
        if (target) {
            Graph->Line(engine->ScreenX(),engine->ScreenY(),
                        target->ScreenX(),target->ScreenY(),GRAPH::YELLOW);
        }

        if (engine->HaveFightLink()) {
            SPRITE* linked=engine->Link();
            if (linked && linked->Goal()) {
                COLOR color=GRAPH::BLACK;
                if (engine->IsCommand(3))
                    color=GRAPH::RED;
                else if (engine->IsCommand(4))
                    color=GRAPH::GRAY;
                // Retail command 5 selects a second zero-valued COLOR owner;
                // semantically it is the same black color as GRAPH::BLACK.
                else if (engine->IsCommand(5))
                    color=GRAPH::BLACK;
                Graph->Line(engine->ScreenX(),engine->ScreenY(),
                            linked->Goal()->ScreenX(),linked->Goal()->ScreenY(),color);
            }
        }
    }

    if (patrolto1 && patrolto2) {
        const float x1=Map->ToScreenX(static_cast<float>(patrolto1->x));
        const float y1=Map->ToScreenY(static_cast<float>(patrolto1->y-patrolto1->z));
        Graph->Line(x1-10.0f,y1-10.0f,x1+10.0f,y1+10.0f,GRAPH::GREEN);
        Graph->Line(x1-10.0f,y1+10.0f,x1+10.0f,y1-10.0f,GRAPH::GREEN);

        const float x2=Map->ToScreenX(static_cast<float>(patrolto2->x));
        const float y2=Map->ToScreenY(static_cast<float>(patrolto2->y-patrolto2->z));
        Graph->Line(x2-10.0f,y2-10.0f,x2+10.0f,y2+10.0f,GRAPH::RED);
        Graph->Line(x2-10.0f,y2+10.0f,x2+10.0f,y2-10.0f,GRAPH::RED);
    }

    if (IsCommand(27)) {
        ENGINE* first=FirstEngine();
        if (first->Goal()) {
            Graph->Box(first->ScreenX()-15.0f,first->ScreenY()-15.0f,
                       first->ScreenX()+15.0f,first->ScreenY()+15.0f,GRAPH::RED);
            SPRITE* firstGoal=first->Goal();
            Graph->Box(firstGoal->ScreenX()-15.0f,firstGoal->ScreenY()-15.0f,
                       firstGoal->ScreenX()+15.0f,firstGoal->ScreenY()+15.0f,GRAPH::RED);
        }
    }

    int previousAngle=-1;
    R_DOT* dot=head.dot;
    for (int i=0;i<noFindedPath;++i) {
        const int link=static_cast<unsigned int>(findedway[i]);
        if (link<0 || link>=dot->noLinks)
            continue;
        R_DOT* next=dot->links[link].dot;
        Graph->Line(Map->ToScreenX(static_cast<float>(dot->x)),
                    Map->ToScreenY(static_cast<float>(dot->y-dot->z)),
                    Map->ToScreenX(static_cast<float>(next->x)),
                    Map->ToScreenY(static_cast<float>(next->y-next->z)),
                    GRAPH::YELLOW);

        // Retail keeps this unfinished debug-angle branch even though the two
        // projected values are not subsequently drawn. Preserve its calls.
        if (previousAngle>=0) {
            ANGLE limit(static_cast<uint8_t>(31));
            ANGLE previous(static_cast<uint8_t>(previousAngle));
            ANGLE difference=previous.Difference(&dot->links[link].direction);
            if (difference.operator>(&limit)) {
                const float debugX=Map->ToScreenX(static_cast<float>(dot->x));
                const float debugY=Map->ToScreenX(static_cast<float>(dot->y-dot->z));
                (void)debugX;
                (void)debugY;
            }
        }
        previousAngle=dot->links[link].direction.Int();
        dot=next;
    }
}

void ENGINE::DrawSecondaryInfo()
{
    float y=Graph->ViewYMin();

    int bestIdx=0;
    if (static_cast<SPRITE*>(m_ptrSprite))
        bestIdx=m_ptrSprite->Vid()->m_idx;

    int goalIdx=0;
    if (Goal())
        goalIdx=Goal()->Vid()->m_idx;

    Graph->PrintfXY(Graph->ViewXMin()+22.0f,y,
        "Ref=%-3i cmd=%1i ani=%-2i ammo=%-3i hp=%-3i AT=%i goal=%-3i best=%-3i speed=%-3i timer=%i",
        NoRef(),Command(),Animation(),Action(92,0,0,0),Hp(),m_unknown04,
        goalIdx,bestIdx,static_cast<int>(m_speed*1000.0f),GetTimer());

    y+=12.0f;
    Graph->PrintfXY(Graph->ViewXMin()+22.0f,y,
        "IS_PBJMF(%i%i%i%i%i) Accel=%i MaxSpeed=%i NoStep=%i NoStepNotF=%i",
        isPushed,is_busy,static_cast<int>(m_flag&1u),was_near_mine,
        static_cast<int>((m_flag>>7)&1u),acceleration,
        static_cast<int>(maxSpeed*1000.0f),NoStep,NoStepForNotFound);

    if (HaveLink()) {
        y+=12.0f;
        SPRITE* linked=Link();

        int linkedBestIdx=0;
        if (static_cast<SPRITE*>(linked->m_ptrSprite))
            linkedBestIdx=linked->m_ptrSprite->Vid()->m_idx;

        int linkedGoalIdx=0;
        if (linked->Goal())
            linkedGoalIdx=linked->Goal()->Vid()->m_idx;

        Graph->PrintfXY(Graph->ViewXMin()+22.0f,y,
            "Ref=%-3i cmd=%1i ani=%-2i ammo=%-3i hp=%-3i AT=%i goal=%-3i best=%-3i timer=%i",
            linked->NoRef(),linked->Command(),linked->Animation(),linked->Action(92,0,0,0),
            linked->Hp(),linked->m_unknown04,linkedGoalIdx,linkedBestIdx,linked->GetTimer());
    }

    LIST<ACT>* actions=ActionStack();
    if (actions->No()) {
        STRING text;
        y+=12.0f;

        STRING prefix=Printf("%i - ",actions->No());
        text+=&prefix;
        for (int i=0;i<actions->No();++i) {
            ACT* act=(*actions)[i];
            STRING item=Printf("%i(%i,%i,%i) ",act->act,act->var1,act->var2,act->var3);
            text+=&item;
        }

        COLOR white(255,255,255);
        Graph->PutsXY(Graph->ViewXMin()+30.0f,y,&text,white);
    }

    DrawGoalLine();
    if (commandEngine)
        Graph->PutBigPixel(commandEngine->ScreenX(),commandEngine->ScreenY(),GRAPH::RED);
}

// Canonical MapEdit.exe switch owner.  Keep pointer/int action ABI exactly x86.
int ENGINE::Action(int act,int var1,int var2,int var3)
{
    switch (act&0xFF) {
    case 86: { // repair/ammo request path
        if (Vid()->m_idx!=0x52 || Ammo()>=MaxAmmo())
            return UNIT::Action(act,var1,var2,var3);

        const int no_desant_eng=Vid()->NoSprites(Army());
        const int no_desant_robot=Map->Vid(0x46)->NoSprites(Army());
        int no_desant_ammo=0;
        int i=0;
        for (SPRITE* spr=Hash->FirstUnit(&i);spr;spr=Hash->NextUnit(&i)) {
            if (spr->Vid()->m_idx==0x52 && spr->Army()==Army())
                no_desant_ammo+=static_cast<UNIT*>(spr)->Ammo();
        }
        if (no_desant_ammo+no_desant_robot < MaxAmmo()*no_desant_eng)
            return UNIT::Action(act,var1,var2,var3);
        return TERRAIN::Action(act,var1,var2,var3);
    }

    case 132:
        SPRITE::Action(act,var1,var2,var3);
        ClearDotBusy();
        if (prev_engine) prev_engine->next_engine=0;
        if (next_engine) next_engine->prev_engine=0;
        return 0;

    case 133:
        SPRITE::Action(act,var1,var2,var3);
        SetDotBusy();
        if (prev_engine) prev_engine->next_engine=this;
        if (next_engine) next_engine->prev_engine=this;
        return 0;

    case 97:
        Map->Player(Army())->DeletePointerToSprite(this);
        ChangeArmy(var1);
        if (IsSelfMoving())
            Map->Player(Army())->AddUnitToStateBar(this);
        return 0;

    case 90:
        return reinterpret_cast<int>(FirstEngine()->Goal());

    case 74:
        SetCommandToTrain((act>>8)&0xFF,reinterpret_cast<SPRITE*>(var1),
                          reinterpret_cast<R_DOT*>(var2),reinterpret_cast<R_DOT*>(var3));
        if (var1) reinterpret_cast<SPRITE*>(var1)->Release();
        return 0;

    case 70:
        AddActionImmediate((Command()<<8)+74,reinterpret_cast<int>(Goal()),
                           reinterpret_cast<int>(patrolto1),reinterpret_cast<int>(patrolto2));
        if (Goal()) Goal()->AddRef();
        return 0;

    case 130: {
        if (IsDying()) return 0;
        if (acceleration>0) {
            if (Animation()!=3) ChangeAnimation(3);
        } else if (acceleration<0) {
            if (Animation()!=1) ChangeAnimation(1);
        } else if (m_speed!=0.0f) {
            if (Animation()!=2) ChangeAnimation(2);
        } else if (Animation()!=0) {
            ChangeAnimation(0);
        }

        if (CanFight() || HaveFightLink())
            ActNextCommandFighter();

        if (!IsCommand(0x18) || Vid()->m_idx!=0x55 || !was_near_mine || m_speed!=0.0f)
            return 0;

        if (!minecreatetime) {
            minecreatetime=CurrentTime;
        } else if (static_cast<int>(CurrentTime-minecreatetime)>Const->RepairSettingMineTime) {
            if (Ammo()>0) {
                Map->CreateSprite(Map->Vid(0x253),static_cast<float>(goal->x),
                                  static_cast<float>(goal->y),static_cast<float>(goal->z),
                                  ANGLE(static_cast<uint8_t>(0)),this);
                SPRITE* s=Map->CreateSprite(Map->Vid(0x56),X(),Y(),Z(),Direction(),this);
                if (s) {
                    s->Action(0x5F,0,0,0);
                    Action(0x5D,-1,0,0);
                }
            }
            SetCommandToTrain(0,0,0,0);
        }
        Map->CreateSprite(Map->Vid(0x24C),X(),Y(),Z(),ANGLE(static_cast<uint8_t>(0)),this);
        return 0;
    }

    case 85: {
        if (var1>0) {
            ENGINE* first=FirstEngine();
            if (!(first->m_flag&2u) && var2 && IsEnemy(reinterpret_cast<SPRITE*>(var2))) {
                Map->ScriptRun(EvFunctionNumber[10],this,reinterpret_cast<SPRITE*>(var2),0);
                first->m_flag|=2u;
            }
        }
        if (HaveLink() && Link()->MaxHp()!=0)
            return Link()->Action(act,var1,var2,var3);
        return SPRITE::Action(act,var1,var2,var3);
    }

    case 80: {
        UNIT::Action(act,var1,var2,var3);
        STREAM* file=reinterpret_cast<STREAM*>(var1);
        head.Write(file);
        tail.Write(file);
        file->Write(&prev_engine,4);
        file->Write(&next_engine,4);
        return 0;
    }

    case 81:
    case 200: {
        UNIT::Action(act,var1,var2,var3);
        STREAM* file=reinterpret_cast<STREAM*>(var1);
        if (var2>=6) {
            head.Read(file);
            tail.Read(file);
            prev_engine=static_cast<ENGINE*>(Map->ReadPointer(file));
            next_engine=static_cast<ENGINE*>(Map->ReadPointer(file));
            SetDotBusy();
            ANGLE direct=Direction();
            ANGLE railDirect=head.Direct();
            ANGLE difference=direct.Difference(&railDirect);
            ANGLE limit(0x7F);
            if (difference.operator>(&limit))
                stateInvMove=1;
        } else {
            SetRDot();
        }
        if (IsSelfMoving())
            Map->Player(Army())->AddUnitToStateBar(this);
        return 0;
    }

    case 37: {
        if (Vid()->m_idx==0x55) {
            SetCommandToTrain(0x18,var1,var2);
            return 0;
        }
        if (Vid()->m_idx==0x61 && IsSingle()) {
            SetCommandToTrain(0x1B,var1,var2);
            return 0;
        }
        float target_z=Map->GetGroundZ(static_cast<float>(var1),static_cast<float>(var2))+19.0f;
        var2+=static_cast<int>(target_z)-19;
        SPRITE* marker=new SPRITE(EmptyVid,static_cast<float>(var1),static_cast<float>(var2),
                                  target_z,ANGLE(static_cast<uint8_t>(0)),0);
        SetCommandToTrain(0x1D,marker,0,0);
        return 0;
    }

    case 33:
        Move(static_cast<float>(var1),static_cast<float>(var2),0.0f,0,0);
        return 0;

    case 32:
        if (!var1) return 0;
        if (Vid()->m_idx==0x61 && IsSingle())
            SetCommandToTrain(0x1B,reinterpret_cast<SPRITE*>(var1),0,0);
        else
            SetCommandToTrain(0x1C,reinterpret_cast<SPRITE*>(var1),0,0);
        return 0;

    case 150:
    case 152:
        if (!var1 || !reinterpret_cast<SPRITE*>(var1)->IsSpriteClass(21) ||
            InTrain(reinterpret_cast<SPRITE*>(var1)))
            return 0;
        SetCommandToTrain(0x1A,reinterpret_cast<SPRITE*>(var1),0,0);
        return 0;

    case 151:
        if (!var1 || !reinterpret_cast<SPRITE*>(var1)->IsSpriteClass(21) ||
            InTrain(reinterpret_cast<SPRITE*>(var1)))
            return 0;
        if (Vid()->m_idx==0x23 && HaveLink())
            Link()->SetCommand(8,reinterpret_cast<SPRITE*>(var1));
        else
            SetCommandToTrain(0x1B,reinterpret_cast<SPRITE*>(var1),0,0);
        return 0;

    case 153:
        for (ENGINE* eng=FirstEngine();eng;eng=eng->NextEngine()) {
            if (var1==1) {
                eng->behave=0;
                continue;
            }
            eng->behave=1;
            if (eng->MaxAmmo()<=5 && (var1==2 || var1==4))
                eng->behave=0;
            if (var1==4 || var1==5)
                eng->behave|=2u;
            if (var1==3 || var1==5)
                eng->behave|=0x10u;
        }
        return 0;

    case 39:
        if (var1) ForceStop();
        SetCommandToTrain(0,0,0,0);
        return 0;

    case 159:
        return IsPowerEngine();
    case 154:
        return reinterpret_cast<int>(FirstEngine());
    case 155:
        return reinterpret_cast<int>(LastEngine());
    case 156:
        return reinterpret_cast<int>(NextEngine());
    case 157:
        return IsFirst();
    case 158:
        return InTrain(reinterpret_cast<SPRITE*>(var1));
    default:
        return UNIT::Action(act,var1,var2,var3);
    }
}

void ENGINE::ActNextCommandFighter()
{
    if (HaveFightLink()) {
        SPRITE* linked=Link();
        if (linked->Goal() && Army()==linked->Goal()->Army() &&
            (linked->IsCommand(3) || linked->IsCommand(4))) {
            linked->SetCommand(0,0);
        }
    }

    if (Vid()->m_idx==0x23) {
        if (!HaveFightLink())
            return;
        SPRITE* linked=Link();
        if (linked->Action(0x5C,0,0,0) < linked->Vid()->m_weapon->m_maxAmmo)
            return;
        // Retail evaluates Link()->Goal()/Goal() here before entering the common
        // path; there is no state mutation in the test itself.
        if (!linked->Goal())
            (void)Goal();
    }

    if (m_flag&1u)
        return;

    if (IsCommand(0x1C) || IsCommand(0x1D)) {
        if (!commandEngine || commandEngine==this) {
            SPRITE* linked=Link();
            SPRITE* target=Goal();
            if (target!=linked->Goal() &&
                (target->Vid()==EmptyVid || CanAttackThisSprite(target))) {
                const float distance=NearDistanceTo(target);
                if (BattleRange()>=distance)
                    linked->SetCommandWithoutLink(4,target);
            }
        }
    } else if (HaveFightLink()) {
        SPRITE* linked=Link();
        if (linked->Goal()) {
            const float distance=NearDistanceTo(linked->Goal());
            if (distance>linked->Vid()->m_weapon->m_detectRange)
                linked->SetCommandWithoutLink(0,0);
        }
    }

    const int state=AttackTact(DeltaTime());
    m_unknown04=state;
    if (state!=1 && state!=2 && state!=5 && state!=6)
        return;

    if (behave&1u) {
        SPRITE* linked=Link();
        if (linked->GetTimer()!=0 || Random(3)==0) {
            SPRITE* enemy=SeekEnemy();
            if (enemy)
                linked->SetCommand(5,enemy);
            return;
        }
    }

    SPRITE* best=static_cast<SPRITE*>(m_ptrSprite);
    if (best)
        Link()->SetCommand(5,best);
}

void ENGINE::ForceStop()
{
    if (!IsFirst()) {
        FirstEngine()->ForceStop();
        return;
    }

    if (fabsf(m_speed)>Const->SafeClashSpeed) {
        for (ENGINE* eng=this;eng;eng=eng->NextEngine())
            eng->Action(85,2,0,0);
    }
    maxSpeed=0.0f;
    m_speed=0.0f;
}

void ENGINE::SetCommandSameOther()
{
    ENGINE* const next=NextEngine();
    if (next && next->Command()) {
        SetCommand(next->Command(),next->Goal());
        return;
    }

    ENGINE* const prev=PrevEngine();
    if (prev && prev->Command()) {
        // Retail 0x0047CB57..0x0047CB7F calls NextEngine() here despite
        // having just tested PrevEngine(). Preserve that exact call graph.
        ENGINE* const retailNext=NextEngine();
        SetCommand(retailNext->Command(),retailNext->Goal());
        return;
    }

    SetCommand(0,static_cast<SPRITE*>(0));
}

int ENGINE::NeedRepairByMaster()
{
    if (Hp()<MaxHp())
        return 1;

    if (IsLinkDestroy() && Vid()->m_idx==35) {
        const int army=Army();
        if (Map->Vid(40)->NoSprites(army) < Map->Vid(35)->NoSprites(army))
            return 1;
    }

    if (HaveLink()) {
        SPRITE* const link=Link();
        if (!link->IsSpriteClass(9) && link->Hp()<link->MaxHp())
            return 1;
    }

    if (Ammo()<MaxAmmo())
        return 1;
    return 0;
}

int ENGINE::IsLast()
{
    return next_engine==0;
}

void ENGINE::Attack(SPRITE* goal)
{
    if (Vid()->m_idx==0x61 && goal && IsSingle())
        SetCommandToTrain(0x1B,goal,0,0);
    else
        SetCommandToTrain(0x1C,goal,0,0);
}

void ENGINE::SubMove(SPRITE* /*goal*/)
{
    FirstEngine()->StartMove();
}
