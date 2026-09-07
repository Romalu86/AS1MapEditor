#include "mapedit/runtime.hpp"

float MAP::ToScreenX(float x) { return x - m_shiftX; }
float MAP::ToScreenY(float y) { return y - m_shiftY; }

int MAP::ToScreenXInt(float x) { return static_cast<int>(x) - static_cast<int>(m_shiftX); }
int MAP::ToScreenYInt(float y) { return static_cast<int>(y) - static_cast<int>(m_shiftY); }

float REGION::ScreenLeft()
{
    return Map->ToScreenX(X() - sizeX * 0.5f);
}
float REGION::ScreenTop()
{
    return Map->ToScreenY(Y() - Z() - sizeY * 0.5f);
}
float REGION::ScreenRight()
{
    return Map->ToScreenX(X() + sizeX * 0.5f);
}
float REGION::ScreenBottom()
{
    return Map->ToScreenY(Y() - Z() + sizeY * 0.5f);
}

float REGION::SizeX()
{
    return (property&8u) ? Map->SizeX() : sizeX;
}

float REGION::SizeY()
{
    return (property&8u) ? Map->SizeY() : sizeY;
}

template<> int LIST<R_DOT*>::No() { return m_no; }
template<> R_DOT** LIST<R_DOT*>::operator[](int index) { return m_data + index; }

template<> R_DOT* const* LIST<R_DOT*>::operator[](int index) const { return m_data + index; }

template<> LIST<R_DOT*>::LIST() : m_no(0), m_max(0), m_data(0) {}
template<> LIST<R_DOT*>::~LIST()
{
    if (m_data)
        ::operator delete(m_data);
    m_data = 0;
    m_no = 0;
}


template<> int LIST<R_DOT*>::Location(R_DOT* const* item)
{
    for (int i=m_no-1;i>=0;--i)
        if (m_data[i]==*item)
            return i;
    return -1;
}

template<> int LIST<R_DOT*>::DeleteNumber(int n)
{
    if (n<0 || n>=m_no)
        return 1;
    --m_no;
    m_data[n]=m_data[m_no];
    return 0;
}

template<> int LIST<R_DOT*>::Delete(R_DOT* const* item)
{
    return DeleteNumber(Location(item));
}

R_MAP::R_MAP() : minx(10000), miny(10000), maxx(0), maxy(0), Dots() {}

// is destruction of the embedded LIST<R_DOT*>.
R_MAP::~R_MAP() {}

float MAP::ToScreenY(float y,float z)
{
    return y-z-m_shiftY;
}

namespace {
int RailDotInt(const R_DOT* dot,unsigned int offset)
{
    return *reinterpret_cast<const int*>(reinterpret_cast<const uint8_t*>(dot)+offset);
}

R_DOT* RailDotPointer(const R_DOT* dot,unsigned int offset)
{
    return *reinterpret_cast<R_DOT* const*>(reinterpret_cast<const uint8_t*>(dot)+offset);
}

void* RailRawPointer(const R_DOT* dot,unsigned int offset)
{
    return *reinterpret_cast<void* const*>(reinterpret_cast<const uint8_t*>(dot)+offset);
}
}

float R_DOT::ScreenX()
{
    return Map->ToScreenX(static_cast<float>(x));
}

float R_DOT::ScreenY()
{
    return Map->ToScreenY(static_cast<float>(y-z));
}

int R_DOT::IsBreak()
{
    return RailDotInt(this,0x04);
}

void R_DOT::DebugDraw()
{
    for (int i=0;i<noLinks;++i) {
        Graph->PrintfXY(
            Graph->ViewXMax()-140.0f,
            Graph->ViewYMin()+70.0f+static_cast<float>((i+1)*30),
            "%i, %i, %i",
            static_cast<int>(links[i].direction.value),
            links[i].dot->x,
            links[i].dot->y);
    }
}

void R_DOT::Error(int type,char* text,unsigned long err)
{
    MYERROR::Error(::Error,"R_DOT %i,%i",type,text,err,x,y);
}

void R_MAP::Error(int type,char* text,unsigned long err)
{
    MYERROR::Error(::Error,"R_MAP",type,text,err);
}

void R_MAP::DebugDraw()
{
    for (int n=0;n<Dots.No();++n) {
        R_DOT* dot=*Dots[n];

        const float sx=dot->ScreenX();
        const float sy=dot->ScreenY();
        if (sx<=-50.0f || sy<=-50.0f || sx>=1000.0f || sy>=1000.0f)
            continue;

        const float markerY=sy+static_cast<float>(dot->z);
        Graph->Line(sx-1.0f,sy,sx-1.0f,markerY,GRAPH::BLUE);
        Graph->Line(sx-2.0f,markerY,sx,markerY,GRAPH::BLUE);

        COLOR baseColor=RailDotInt(dot,0xDC) ? GRAPH::RED : GRAPH::WHITE;
        const int noLinks=RailDotInt(dot,0x18);
        for (int link=0;link<noLinks;++link) {
            const unsigned int entry=0x1Cu+static_cast<unsigned int>(link)*0x14u;
            R_DOT* target=RailDotPointer(dot,entry);

            COLOR linkColor=GRAPH::WHITE;
            if (RailDotInt(dot,0x14)>3 && RailDotInt(target,0x14)>3)
                linkColor=GRAPH::BLACK;
            if (dot->IsBreak() && target->IsBreak())
                linkColor=GRAPH::RED;
            if (RailDotInt(dot,0x0C)==link || RailDotInt(target,0x0C)==RailDotInt(dot,entry+0x08u))
                linkColor=GRAPH::LIGHTRED;

            Graph->Line(sx,sy,target->ScreenX(),target->ScreenY(),linkColor);

            void* viaObject=RailRawPointer(dot,entry+0x0Cu);
            if (viaObject) {
                R_DOT* via=*reinterpret_cast<R_DOT**>(viaObject);
                Graph->Line(sx,sy,via->ScreenX(),via->ScreenY(),GRAPH::GREEN);
            }
        }

        Graph->Line(sx-2.0f,sy-2.0f,sx+2.0f,sy+2.0f,baseColor);
        Graph->Line(sx+2.0f,sy-2.0f,sx-2.0f,sy+2.0f,baseColor);
    }
}


// -----------------------------------------------------------------------------
// Core railway-dot lifetime owners restored directly from canonical
// MapEdit.exe / Exec\\railway.obj.  These are required by RAIL and ENGINE and
// intentionally preserve the original reference-counted self-destruction path.
// -----------------------------------------------------------------------------

R_DOT::R_DOT()
    : refCount(0), isBreakFlag(0), oneDirection(-1), pushed(0), isMined(0),
      noLinks(0), busyEngine(0)
{
    // Retail does not initialize nations, step/realStep/railLength or x/y/z
    // here.  The R_DOT_LINK array is default-constructed so only ANGLE members
    // receive their implicit constructor, exactly as in the VC6 owner.
}

R_DOT::~R_DOT()
{
    Release();
}

void R_DOT::UnLink(R_DOT* dot)
{
    for (int i=0;i<noLinks;++i) {
        if (links[i].dot!=dot)
            continue;

        if (oneDirection==i)
            oneDirection=-1;

        --noLinks;
        links[i]=links[noLinks];
        links[i].dot->links[links[i].backLink].backLink=i;
        if (oneDirection==noLinks)
            oneDirection=i;
        return;
    }
}

void R_DOT::AddRef()
{
    ++refCount;
}

void R_DOT::Release()
{
    if (!refCount)
        return;

    --refCount;
    if (refCount)
        return;

    busyEngine=0;
    for (int i=0;i<noLinks;++i)
        links[i].dot->UnLink(this);
    noLinks=0;

    R_DOT* self=this;
    RailMap.Dots.Delete(&self);
    delete this;
}

R_DOT* R_MAP::GetNearestDot(int x0,int y0)
{
    int bestDistance=0x0fffffff;
    R_DOT* best=0;
    for (int i=0;i<Dots.No();++i) {
        R_DOT* dot=*Dots[i];
        if (dot->noLinks<=0)
            continue;
        const int distance=abs(dot->x-x0)+abs((dot->y-dot->z)-y0);
        if (distance<bestDistance) {
            best=dot;
            bestDistance=distance;
        }
    }
    return best;
}

R_DOT* R_MAP::GetNearestDot2(int x0,int y0)
{
    int bestDistance=0x0fffffff;
    R_DOT* best=0;
    for (int i=0;i<Dots.No();++i) {
        R_DOT* dot=*Dots[i];
        if (dot->noLinks<=0)
            continue;
        const int distance=abs(dot->x-x0)+abs(dot->y-y0);
        if (distance<bestDistance) {
            best=dot;
            bestDistance=distance;
        }
    }
    return best;
}

void R_MAP::ClearAllDots()
{
    int i=Dots.No();
    while (--i>=0)
        (*Dots[i])->Release();
}

R_DOT* R_MAP::GetNearestDot(int x0,int y0,int z0)
{
    int bestDistance=0x0fffffff;
    R_DOT* best=0;
    for (int i=0;i<Dots.No();++i) {
        R_DOT* dot=*Dots[i];
        // +0x18 is the retail number-of-links field; dead/unlinked dots are skipped.
        if (RailDotInt(dot,0x18)<=0)
            continue;
        const int distance=abs(dot->x-x0)+abs(dot->y-y0)+abs(z0-dot->z);
        if (distance<bestDistance) {
            best=dot;
            bestDistance=distance;
        }
    }
    return best;
}

int R_DOT::GetLink(R_DOT* dot)
{
    for (int i=0;i<noLinks;++i)
        if (links[i].dot==dot)
            return i;
    return -1;
}



namespace {
int g_findNewDotStep=0;
int g_findNewDotRealStep=0;
int g_findNewDotRailLength=0;
int g_returnDotsNo=0;
int g_returnDots[95]={};
const double g_stoppedRailSpeed=0.03;

}

// -----------------------------------------------------------------------------
// Railway pathfinding owners.  Names/layout are original MapEdit debug metadata debug metadata;
// bodies below are implemented against the MapEdit.exe railway.cpp blocks.
// -----------------------------------------------------------------------------
R_DOT* R_DOT::FindedDot = 0;
R_DOT* R_DOT::Goal = 0;
SPRITE* R_DOT::Target = 0;
int R_DOT::NoStep = 0xffff;
int R_DOT::NoStepForNotFound = 0xffff;
int R_DOT::MaxPathDots = 0;
int R_DOT::train_length_in_rails = 0;
unsigned char* R_DOT::FindedPath = 0;
unsigned char R_DOT::CurrentPath[10000] = {};
ENGINE* R_DOT::eng = 0;
unsigned int R_DOT::command = 0;
int R_DOT::weaponrange = 0;
int R_DOT::repair_in_head = 0;
int R_DOT::repair_in_tail = 0;
unsigned int R_DOT::head_is_head = 0;

R_DOT* R_POS::Dot2()
{
    if (!dot)
        return 0;
    return dot->links[link].dot;
}

ANGLE R_POS::Direct()
{
    if (dot)
        return dot->links[link].direction;
    return ANGLE(static_cast<uint8_t>(0));
}

int R_POS::Length()
{
    if (!dot)
        return 0;
    return dot->links[link].distance;
}

// target/path selection: max axis + half of the smaller axis.
float R_DOT::SizeTo(float endx,float endy)
{
    return NearDistance(endx-static_cast<float>(x),endy-static_cast<float>(y));
}

// Exact integer overload present as a separate retail owner.  Do not collapse
// it into the float overload: callers depend on EAX integer return semantics.
int R_DOT::SizeTo(int endx,int endy)
{
    return NearDistance(endx-x,endy-y);
}

int Square(int val)
{
    return val*val;
}

int Distance(int d1,int d2,int d3)
{
    return Sqrt(d1*d1+d2*d2+d3*d3);
}

float Distance(float d1,float d2,float d3)
{
    return static_cast<float>(sqrt(static_cast<double>(d1*d1+d2*d2+d3*d3)));
}

int R_DOT::DistanceTo(R_DOT* dot0)
{
    return DistanceTo(dot0->x,dot0->y,dot0->z);
}

int R_DOT::DistanceTo(int xx,int yy,int zz)
{
    return Distance(x-xx,y-yy,z-zz);
}

float R_DOT::DistanceTo(float xx,float yy,float zz)
{
    return Distance(static_cast<float>(x)-xx,
                    static_cast<float>(y)-yy,
                    static_cast<float>(z)-zz);
}

int R_DOT::GetPos(int xx,int yy,int /*zz*/,int linkIndex)
{
    ANGLE right(static_cast<uint8_t>(0x40));
    ANGLE projection=links[linkIndex].direction.operator-(&right);
    const float dx=static_cast<float>(xx-x);
    const float dy=static_cast<float>((yy-y)*3);
    // 0x004B50BC in canonical MapEdit.exe is the IEEE-754 value 2.0f.
    return static_cast<int>(projection.Cos()*dx + projection.Sin()*dy/2.0f);
}

int R_DOT::GetDistance(int xx,int yy,int zz,int linkIndex)
{
    R_DOT* linked=links[linkIndex].dot;
    const int pos=GetPos(xx,yy,zz,linkIndex);
    if (pos>=0 && pos<links[linkIndex].distance) {
        const int ax=xx-x;
        const int ay=yy-y;
        const int az=zz-z;
        const int bx=linked->x-x;
        const int by=linked->y-y;
        const int bz=linked->z-z;
        const int cx=ax*by-ay*bx;
        const int cy=ay*bz-az*by;
        const int cz=az*bx-ax*bz;
        return Sqrt(Square(cx)+Square(cy)+Square(cz))/DistanceTo(linked);
    }

    return Distance((linked->x+x)/2-xx,
                    (linked->y+y)/2-yy,
                    (linked->z+z)/2-zz+4);
}

void R_DOT::SetNearestPos(int xx,int yy,int zz,R_POS* pos)
{
    if (!noLinks)
        return;

    int bestDistance=0xffff;
    for (int i=0;i<noLinks;++i) {
        R_DOT* linked=links[i].dot;
        for (int j=0;j<linked->noLinks;++j) {
            const int distance=linked->GetDistance(xx,yy,zz,j);
            if (distance<bestDistance) {
                bestDistance=distance;
                pos->dot=linked;
                pos->link=j;
            }
        }
    }

    pos->pos_real=pos->dot->GetPos(xx,yy,zz,pos->link);
    if (pos->pos_real<0)
        pos->pos_real=0;
    if (pos->pos_real>=pos->Length())
        pos->pos_real=pos->Length()-1;
}

R_POS::R_POS(R_DOT* d,int p,int l)
    : dot(d), pos_real(p), pos_fract(0), link(l)
{
}

R_POS R_POS::GetInversed()
{
    R_DOT_LINK& l=dot->links[link];
    return R_POS(l.dot,l.distance-pos_real,l.backLink);
}

void R_POS::Inverse()
{
    R_DOT* oldDot=dot;
    const int oldLink=link;
    pos_real=oldDot->links[oldLink].distance-pos_real;
    dot=oldDot->links[oldLink].dot;
    link=oldDot->links[oldLink].backLink;
}

void R_DOT::SetIfIsBetter(int len,int noStep,int /*unused*/,int* out)
{
    FindedDot=this;
    NoStep=noStep;
    *out=-1;
    if (FindedPath && len<2500) {
        if (eng)
            eng->noFindedPath=len;
        memcpy(FindedPath,CurrentPath,static_cast<unsigned int>(len));
    }
}

int R_POS::NoStepToTarget(R_DOT* goalDot,SPRITE* target,unsigned int cmd,ENGINE* engine)
{
    if (!engine || (!goalDot && !target))
        return 0x10000;

    RailMap.PrepareForFindDot(goalDot,target,cmd,engine);
    R_DOT* linked=Dot2();
    if (!linked)
        return R_DOT::NoStep;

    const int backLink=linked->GetLink(dot);
    const ANGLE direct=Direct();
    const int stepIndex=linked->FindNewDot(backLink,direct);
    if (stepIndex>=0 && stepIndex<linked->noLinks) {
        const unsigned char cur=direct.value;
        const unsigned char next=linked->links[stepIndex].direction.value;
        const unsigned char d1=static_cast<unsigned char>(cur-next);
        const unsigned char d2=static_cast<unsigned char>(next-cur);
        const unsigned char difference=d1<d2 ? d1 : d2;
        if (difference>0x1f && R_DOT::FindedDot && engine->IsTailInFindedPath(linked))
            R_DOT::NoStep=-R_DOT::NoStep;
    }
    return R_DOT::NoStep;
}

// initializer for FindNewDot and therefore owns all static search state.
void R_MAP::PrepareForFindDot(R_DOT* goalDot,SPRITE* target,unsigned int cmd,ENGINE* engine)
{
    int range=0;
    R_DOT::NoStep=0xffff;
    R_DOT::NoStepForNotFound=0xffff;
    R_DOT::MaxPathDots=0xffff;
    R_DOT::Goal=goalDot;
    R_DOT::FindedDot=0;
    R_DOT::eng=engine;

    if (engine) {
        engine->noFindedPath=0;
        R_DOT::train_length_in_rails=engine->GetTrainLengthInRails()+1;
        R_DOT::FindedPath=engine->findedway;
        ENGINE* first=engine->FirstEngine();
        ENGINE* last=engine->LastEngine();
        R_DOT::repair_in_head=(first && first->Vid()->m_idx==0x55) ? 1 : 0;
        R_DOT::repair_in_tail=(last && last->Vid()->m_idx==0x55) ? 1 : 0;
        if (cmd==28u || cmd==29u) {
            range=engine->TrainWeaponRange()-150-20*R_DOT::train_length_in_rails;
            if (range<20)
                range=20;
        }
    } else {
        R_DOT::train_length_in_rails=0;
        R_DOT::FindedPath=0;
        R_DOT::repair_in_head=0;
        R_DOT::repair_in_tail=0;
    }

    R_DOT::command=cmd;
    R_DOT::weaponrange=range;
    R_DOT::Target=target;
    R_DOT::head_is_head=0;

    for (int n=0;n<Dots.No();++n) {
        R_DOT* dot0=*Dots[n];
        for (int linkIndex=0;linkIndex<dot0->noLinks;++linkIndex) {
            dot0->step[linkIndex]=0x0fffffff;
            dot0->realStep[linkIndex]=0x0fffffff;
            dot0->railLength[linkIndex]=0x0fffffff;
        }
    }
}



R_DOT* R_POS::Dot4()
{
    R_DOT* dot2=dot ? dot->links[link].dot : 0;
    if (!dot2)
        return 0;

    for (int i=0;i<dot2->noLinks;++i) {
        R_DOT* dot3=dot2->links[i].dot;
        if (!dot3)
            continue;
        for (int j=0;j<dot3->noLinks;++j) {
            R_DOT* dot4=dot3->links[j].dot;
            if (dot4->busyEngine)
                return dot4;
        }
    }
    return 0;
}

int R_POS::NoStepToTargetWithoutBusyDots(R_DOT* goalDot,SPRITE* target)
{
    if (!goalDot && !target)
        return 0x10000;

    RailMap.PrepareForFindDot(goalDot,target,0,0);
    Dot2()->FindNewDotWithoutBusyDots();
    return R_DOT::NoStep;
}

int R_DOT::TryToFindRailsForReturn(int depth,R_DOT* prev,R_DOT* avoid,ENGINE* engine,ANGLE direct)
{
    if (!depth)
        return 1;

    for (int i=0;i<noLinks;++i) {
        R_DOT_LINK* link0=&links[i];
        R_DOT* next=link0->dot;
        if (next==prev || next==avoid || !engine)
            continue;
        if (!CanEnginePassTo(i,engine) || !next->CanEnginePassTo(link0->backLink,engine))
            continue;

        const unsigned char linkDir=link0->direction.value;
        const unsigned char d1=static_cast<unsigned char>(direct.value-linkDir);
        const unsigned char d2=static_cast<unsigned char>(linkDir-direct.value);
        const unsigned char delta=d1<d2 ? d1 : d2;
        if (delta>0x1f)
            continue;

        if (g_returnDotsNo<95) {
            g_returnDots[g_returnDotsNo]=i;
            ++g_returnDotsNo;
        } else {
            MYERROR::Error(::Error,"R_DOT %i,%i",10,"dots_num is large",
                           static_cast<unsigned long>(g_returnDotsNo),x,y);
        }

        if (next->TryToFindRailsForReturn(depth-1,this,avoid,engine,link0->direction))
            return 1;
        --g_returnDotsNo;
    }
    return 0;
}

int R_DOT::CanEnginePassTo(int linkIndex,ENGINE* engine)
{
    if (linkIndex<0 || linkIndex>=noLinks)
        return 0;

    R_DOT* linked=links[linkIndex].dot;
    if (engine) {
        if (isMined==engine->Army()+4 && linked->isMined==isMined)
            return 0;

        ENGINE* busy=linked->busyEngine;
        if (busy && !engine->InTrain(busy)) {
            if (R_DOT::command==26u && busy->InTrain(R_DOT::Target))
                return 0;

            if (static_cast<double>(fabsf(busy->Speed()))<g_stoppedRailSpeed || busy->head.Dot2()==this) {
                SPRITE* busyTarget=busy->GetMoveTarget();
                if (!busyTarget || busyTarget!=engine->GetMoveTarget()) {
                    R_DOT* busyGoal=busy->GetGoal();
                    if (!busyGoal || busyGoal!=engine->GetGoal())
                        return 0;
                }
            }
        }
    }
    return linked->oneDirection!=links[linkIndex].backLink;
}

// retail rail search used by ENGINE::ReCalcMoveParameters.
int R_DOT::FindNewDot(int backLink,ANGLE direct)
{
    const unsigned int savedHead=head_is_head;
    int result=-2;

    if (MaxPathDots>g_findNewDotStep) {
        if (backLink>=0) {
            step[backLink]=g_findNewDotStep;
            realStep[backLink]=g_findNewDotRealStep;
            railLength[backLink]=g_findNewDotRailLength;
        }

        if (this!=Goal) {
            if (!Target || !busyEngine || !busyEngine->InTrain(Target))
                goto nearTarget;

            if (command==26u) {
                if (backLink<0)
                    goto notFound;
                ENGINE* busy=busyEngine;
                if (busy->prev_engine || (busy->head.Dot2()!=links[backLink].dot && busy->head.Dot2()!=this)) {
                    if (busy->next_engine)
                        return -2;
                    if (busy->tail.Dot2()!=links[backLink].dot && busy->tail.Dot2()!=this)
                        return -2;
                }
            }
        }
        SetIfIsBetter(g_findNewDotStep,g_findNewDotRealStep,weaponrange,&result);

nearTarget:
        if ((command==28u || command==29u) && Target &&
            (!busyEngine || (eng && eng->InTrain(busyEngine)))) {
            const int dist=static_cast<int>(SizeTo(Target->X(),Target->Y()));
            if (dist<=weaponrange) {
                if (FindedDot) {
                    if (FindedDot->SizeTo(Target->X(),Target->Y())>static_cast<float>(dist)) {
                        if (NoStep>g_findNewDotRealStep)
                            SetIfIsBetter(g_findNewDotStep,g_findNewDotRealStep,weaponrange,&result);
                    } else if (NoStep>g_findNewDotRealStep) {
                        SetIfIsBetter(g_findNewDotStep,g_findNewDotRealStep,weaponrange,&result);
                    }
                } else if (NoStep>g_findNewDotRealStep) {
                    SetIfIsBetter(g_findNewDotStep,g_findNewDotRealStep,weaponrange,&result);
                }
            }
        }

notFound:
        if (NoStep>=0xffff) {
            if (Goal) {
                int keep=0;
                if (FindedDot) {
                    if (NearDistance(Goal->x-FindedDot->x,Goal->y-FindedDot->y)<=
                            NearDistance(Goal->x-x,Goal->y-y) &&
                        (FindedDot!=this || NoStepForNotFound<=g_findNewDotRealStep)) {
                        keep=1;
                    }
                }
                if (!keep) {
                    NoStepForNotFound=g_findNewDotRealStep;
                    FindedDot=this;
                    if (FindedPath && g_findNewDotStep<2500) {
                        if (eng)
                            eng->noFindedPath=g_findNewDotStep;
                        memcpy(FindedPath,CurrentPath,static_cast<unsigned int>(g_findNewDotStep));
                    }
                    result=-1;
                }
            }

            if (Target) {
                int better=1;
                if (FindedDot) {
                    better=(FindedDot->SizeTo(Target->X(),Target->Y())>SizeTo(Target->X(),Target->Y()) ||
                            (FindedDot==this && NoStepForNotFound>g_findNewDotRealStep));
                }
                if (better) {
                    FindedDot=this;
                    NoStepForNotFound=g_findNewDotRealStep;
                    if (FindedPath && g_findNewDotStep<2500) {
                        if (eng)
                            eng->noFindedPath=g_findNewDotStep;
                        memcpy(FindedPath,CurrentPath,static_cast<unsigned int>(g_findNewDotStep));
                    }
                    result=-1;
                }
            }
        }

        int canPassBack=1;
        if (backLink>=0 && eng)
            canPassBack=links[backLink].dot->CanEnginePassTo(links[backLink].backLink,eng);

        if (g_findNewDotRealStep<NoStep) {
            ++g_findNewDotStep;
            ++g_findNewDotRealStep;
            for (int i=0;i<noLinks;++i) {
                if (!canPassBack) {
                    if (g_findNewDotStep!=1 || i!=backLink)
                        continue;
                } else if (i==backLink && g_findNewDotStep>=3) {
                    continue;
                }

                R_DOT_LINK* link0=&links[i];
                if (backLink>=0 && link0->dot->realStep[link0->backLink]<=g_findNewDotRealStep)
                    continue;

                head_is_head=savedHead;
                int reversed=0;
                int railsForReturn=1;
                if (backLink>=0) {
                    const unsigned char d1=static_cast<unsigned char>(direct.value-link0->direction.value);
                    const unsigned char d2=static_cast<unsigned char>(link0->direction.value-direct.value);
                    const unsigned char delta=d1<d2 ? d1 : d2;
                    if (delta>0x1f) {
                        if ((g_findNewDotStep>1 || !eng || eng->Speed()!=0.0f) && i!=backLink) {
                            if (train_length_in_rails) {
                                g_returnDotsNo=0;
                                railsForReturn=TryToFindRailsForReturn(train_length_in_rails,
                                    links[backLink].dot,link0->dot,eng,direct);
                            }
                            reversed=1;
                        }
                        head_is_head^=1u;
                    }
                }

                const int dist=link0->distance;
                g_findNewDotRailLength+=dist;
                if (reversed)
                    g_findNewDotRealStep+=train_length_in_rails;

                int passable=1;
                R_DOT_LINK* cross=link0->crossLink;
                if (cross) {
                    R_DOT* other=cross->dot->links[cross->backLink].dot;
                    ENGINE* busy=cross->dot->busyEngine;
                    if ((busy && static_cast<double>(fabsf(busy->Speed()))<g_stoppedRailSpeed &&
                         (!eng || !busy->InTrain(eng))) ||
                        ((busy=other->busyEngine)!=0 && static_cast<double>(fabsf(busy->Speed()))<g_stoppedRailSpeed &&
                         (!eng || !busy->InTrain(eng)))) {
                        passable=0;
                    }
                }

                if (railsForReturn && passable) {
                    if (g_findNewDotStep-1<2500)
                        CurrentPath[g_findNewDotStep-1]=static_cast<unsigned char>(i);
                    if (link0->dot->FindNewDot(link0->backLink,link0->direction)>=-1)
                        result=i;
                }

                g_findNewDotRailLength-=dist;
                if (reversed)
                    g_findNewDotRealStep-=train_length_in_rails;
            }
            --g_findNewDotStep;
            --g_findNewDotRealStep;
        }
    }

    head_is_head=savedHead;
    return result;
}

namespace {
int g_railDotArray[300000]; // original global at 0x004F0C64

}

void R_MAP::AddDotToArray(int gx,int gy,int width,int height,int dotIndex)
{
    if (gx>=width || gy>=height || gx<0 || gy<0)
        return;

    const int cell=gy+100*gx;
    int& count=g_railDotArray[30*cell];
    ++count;
    if (count>=30) {
        Error(10,const_cast<char*>("AddDotToArray() a[x][y][0]>=ARR_SIZE"),0);
        --count;
        return;
    }

    g_railDotArray[30*cell+count]=dotIndex;
}

void R_MAP::CreateAdditionalDots()
{
    int xCells=2*(maxx/150)+10;
    int yCells=2*(maxy/150)+10;
    if (xCells>=100) xCells=100;
    if (yCells>=100) yCells=100;

    for (int gx=0;gx<xCells;++gx)
        for (int gy=0;gy<yCells;++gy)
            g_railDotArray[3000*gx+30*gy]=0;

    for (int i=0;i<Dots.No();++i) {
        R_DOT* dot=*Dots[i];
        dot->step[0]=0;
        const int cx=dot->x/75;
        const int cy=dot->y/75;
        AddDotToArray(cx,cy,xCells,yCells,i);
        AddDotToArray(cx-1,cy,xCells,yCells,i);
        AddDotToArray(cx,cy-1,xCells,yCells,i);
        AddDotToArray(cx-1,cy-1,xCells,yCells,i);
    }

    for (int gx=0;gx<xCells;++gx) {
        for (int gy=0;gy<yCells;++gy) {
            const int base=30*(gy+100*gx);
            for (int a=1;a<=g_railDotArray[base];++a) {
                for (int b=a+1;b<=g_railDotArray[base];++b) {
                    R_DOT* da=*Dots[g_railDotArray[base+a]];
                    R_DOT* db=*Dots[g_railDotArray[base+b]];
                    for (int la=0;la<da->noLinks;++la)
                        for (int lb=0;lb<db->noLinks;++lb)
                            CreateIntersectedDot(da,da->links[la].dot,db,db->links[lb].dot);
                }
            }
        }
    }
}

void R_MAP::CreateIntersectedDot(R_DOT* a,R_DOT* b,R_DOT* c,R_DOT* d)
{
    if (a==c || a==d || b==c || b==d)
        return;
    if (a->step[0] || b->step[0] || c->step[0] || d->step[0])
        return;

    const int cx=c->x;
    const int bx=a->x;
    const int ax=b->x;
    const int by=a->y;
    const int ay=b->y;
    const int cy=c->y;
    const int dx=d->x;
    const int dy=d->y;
    const int abx=bx-ax;
    const int crossA=abx*ay-(by-ay)*ax;
    const int cdx=dx-cx;
    const int cyDy=cy-dy;
    const int ayBy=ay-by;
    const int crossC=cdx*cy-(dy-cy)*cx;
    const int denom=cdx*ayBy-cyDy*abx;
    if (denom==0)
        return;

    const double ixD=static_cast<double>(cdx*crossA-crossC*abx)/static_cast<double>(denom);
    double iyD=0.0;
    if (abx)
        iyD=(static_cast<double>(crossA)-static_cast<double>(ayBy)*ixD)/static_cast<double>(abx);
    else if (cdx)
        iyD=(static_cast<double>(crossC)-static_cast<double>(cyDy)*ixD)/static_cast<double>(cdx);

    const int ix=static_cast<int>(ixD);
    const int iy=static_cast<int>(iyD);
    if (!Between(ix,a->x,b->x) || !Between(iy,a->y,b->y) ||
        !Between(ix,c->x,d->x) || !Between(iy,c->y,d->y))
        return;

    const int ab=a->GetLink(b);
    const int ba=b->GetLink(a);
    const int cd=c->GetLink(d);
    const int dc=d->GetLink(c);
    if (ab>=0 && ba>=0 && cd>=0 && dc>=0) {
        R_DOT_LINK* cLink=&c->links[cd];
        a->links[ab].crossLink=cLink;
        b->links[ba].crossLink=cLink;
        R_DOT_LINK* aLink=&a->links[ab];
        c->links[cd].crossLink=aLink;
        d->links[dc].crossLink=aLink;
    }
}


// Recursive railway search used specifically by R_MAP::SetPushLine.  The
// global step counter corresponds to original .bss 0x006184E0.
static int g_findNewDotWithoutBusyDotsStep=0;

int R_DOT::FindNewDotWithoutBusyDots()
{
    int result=-2;
    if (step[0] <= g_findNewDotWithoutBusyDotsStep)
        return result;

    step[0]=g_findNewDotWithoutBusyDotsStep;
    if (this==Goal || (Target && busyEngine && busyEngine->InTrain(Target))) {
        NoStep=g_findNewDotWithoutBusyDotsStep;
        FindedDot=this;
        return -1;
    }

    if (busyEngine && NoStep>=0xffff) {
        if (Goal) {
            if (FindedDot) {
                const int a=abs(Goal->x-FindedDot->x);
                const int b=abs(Goal->y-FindedDot->y);
                const int d1=(a>b) ? (a+b/2) : (b+a/2);
                const int c=abs(Goal->x-x);
                const int d=abs(Goal->y-y);
                const int d2=(c>d) ? (c+d/2) : (d+c/2);
                if (d1>d2) {
                    FindedDot=this;
                    result=-1;
                }
            } else {
                FindedDot=this;
                result=-1;
            }
        } else if (Target) {
            if (FindedDot) {
                const float d1=FindedDot->SizeTo(Target->X(),Target->Y());
                const float d2=SizeTo(Target->X(),Target->Y());
                if (d1>d2) {
                    FindedDot=this;
                    result=-1;
                }
            } else {
                FindedDot=this;
                result=-1;
            }
        }
    }

    if (g_findNewDotWithoutBusyDotsStep<NoStep) {
        ++g_findNewDotWithoutBusyDotsStep;
        for (int i=0;i<noLinks;++i) {
            if (links[i].dot->FindNewDotWithoutBusyDots()>=-1)
                result=i;
        }
        --g_findNewDotWithoutBusyDotsStep;
    }
    return result;
}

void R_MAP::SetPushLine(int begin_x,int begin_y,int end_x,int end_y,int push_flag)
{
    R_DOT* begin=GetNearestDot(begin_x,begin_y);
    R_DOT* end=GetNearestDot(end_x,end_y);

    if (!begin) {
        MYERROR::Log(::Error,"!!!ERROR!!!R_MAP: Can't found dot in %i,%i",begin_x,begin_y);
        return;
    }
    if (!end) {
        MYERROR::Log(::Error,"!!!ERROR!!!R_MAP: Can't found dot in %i,%i",end_x,end_y);
        return;
    }
    if (begin->noLinks<=0) {
        MYERROR::Log(::Error,"!!!ERROR!!!R_MAP: Can't SetPushLine in %i,%i",begin_x,begin_y);
        return;
    }

    if (end==begin) {
        int best=999999;
        for (int i=0;i<begin->noLinks;++i) {
            R_DOT* candidate=begin->links[i].dot;
            const int distance=abs(candidate->x-end_x)+abs(candidate->y-candidate->z-end_y);
            if (distance<best) {
                best=distance;
                end=candidate;
            }
        }
    }

    PrepareForFindDot(begin,0,0,0);
    if (end->FindNewDotWithoutBusyDots()>=0) {
        R_DOT* dot=begin;
        while (dot && dot!=end) {
            for (int i=0;i<dot->noLinks;++i) {
                R_DOT* next=dot->links[i].dot;
                if (dot->step[0]==next->step[0]+1) {
                    dot->oneDirection=i;
                    dot->pushed=push_flag;
                    dot=next;
                    break;
                }
            }
        }
        return;
    }

    MYERROR::Log(::Error,"!!!ERROR!!!R_MAP: Can't PushLineFindDot in %i,%i",begin_x,begin_y);
}

static __declspec(noinline) double sqr(double value)
{
    return value*value;
}

void R_MAP::SetSemaphoreOrMine(int x0,int y0,int new_semaphore_or_mine,int /*nations*/)
{
    R_DOT* dot=GetNearestDot(x0,y0);
    if (!dot)
        return;

    double minlen=10000.0;
    int minlink=-1;
    for (int i=0;i<dot->noLinks;++i) {
        R_DOT* linked=dot->links[i].dot;
        const double dx=static_cast<double>(x0-linked->x);
        const double dy=static_cast<double>(y0-linked->y);
        const double len=sqrt(sqr(dx)+sqr(dy));
        if (len<minlen) {
            minlen=len;
            minlink=i;
        }
    }

    if (minlink<0) {
        MYERROR::Error(::Error,"R_MAP",10,"нету связей у точки - такого быть не может",0);
        return;
    }

    R_DOT* linked=dot->links[minlink].dot;
    dot->isMined=new_semaphore_or_mine;
    if (linked)
        linked->isMined=new_semaphore_or_mine;
}


float Distance(float d1,float d2)
{
    return static_cast<float>(sqrt(static_cast<double>(d1*d1+d2*d2)));
}

int R_DOT::GetLink(ANGLE direct)
{
    int best=0;
    for (int i=1;i<noLinks;++i) {
        ANGLE current=direct.Difference(&links[i].direction);
        ANGLE previous=direct.Difference(&links[best].direction);
        if (current<&previous)
            best=i;
    }
    return best;
}

int R_POS::DoStep(R_DOT* goalDot,SPRITE* target,ENGINE* engine)
{
    R_DOT* prev=dot;
    const ANGLE oldDirect=Direct();

    if (pos_real>Length())
        pos_real-=Length();
    dot=Dot2();

    if (goalDot || target) {
        RailMap.PrepareForFindDot(goalDot,target,static_cast<unsigned int>(engine->Command()),engine);

        float distance;
        if (goalDot)
            distance=goalDot->SizeTo(engine->X(),engine->Y())/10.0f;
        else
            distance=target->NearDistanceTo(engine)/10.0f;
        R_DOT::MaxPathDots=static_cast<int>(distance);

        const int prevToCurrent=prev->GetLink(dot);
        const int back=dot->GetLink(prev);
        link=dot->FindNewDot(back,prev->links[prevToCurrent].direction);

        if (R_DOT::NoStep==0xffff) {
            RailMap.PrepareForFindDot(goalDot,target,static_cast<unsigned int>(engine->Command()),engine);
            const int prevToCurrent2=prev->GetLink(dot);
            const int back2=dot->GetLink(prev);
            link=dot->FindNewDot(back2,prev->links[prevToCurrent2].direction);
        }
    } else {
        link=-1;
    }

    if (link>=0) {
        const unsigned char oldDir=oldDirect.value;
        const unsigned char newDir=dot->links[link].direction.value;
        const unsigned char d1=static_cast<unsigned char>(oldDir-newDir);
        const unsigned char d2=static_cast<unsigned char>(newDir-oldDir);
        const unsigned char delta=d1<d2 ? d1 : d2;
        if (delta>0x1f) {
            R_DOT* avoid=Dot2();
            link=dot->GetLink(oldDirect);
            if (R_DOT::train_length_in_rails) {
                g_returnDotsNo=0;
                if (!dot->TryToFindRailsForReturn(R_DOT::train_length_in_rails,prev,avoid,engine,oldDirect))
                    R_DOT::NoStep=0xffff;
                else
                    link=g_returnDots[0];
            }
            if (engine && R_DOT::FindedDot && engine->IsTailInFindedPath(dot))
                R_DOT::NoStep=-R_DOT::NoStep;
        }
    } else {
        link=dot->GetLink(oldDirect);
    }

    if (pos_real>Length())
        pos_real=Length();
    return R_DOT::NoStep;
}

int SPRITE::HaveAction(int act)
{
    return m_vid->m_noAnimCadr[act] || m_vid->m_aniSpawnMode[act] || m_vid->m_aniSfx[act];
}

template<> void LIST<R_DOT*>::Expand(int newAllocation)
{
    if (newAllocation<=m_max)
        return;
    R_DOT** oldData=m_data;
    m_data=static_cast<R_DOT**>(::operator new(sizeof(R_DOT*)*newAllocation));
    if (!m_data)
        MYERROR::LogExit(Error,"!!!ERROR!!!::LIST: Not enough memory %i",newAllocation);
    if (oldData) {
        for (int i=0;i<m_max;++i)
            m_data[i]=oldData[i];
        ::operator delete(oldData);
    }
    m_max=newAllocation;
}

template<> void LIST<R_DOT*>::ExpandForInsert()
{
    if (m_no>=m_max)
        Expand(m_max*2+4);
}

template<> void LIST<R_DOT*>::Insert(R_DOT* item)
{
    ExpandForInsert();
    m_data[m_no]=item;
    ++m_no;
}

template<> int LIST<R_DOT*>::InsertUnique(R_DOT* const* item)
{
    if (Location(item)>=0)
        return 1;
    Insert(*item);
    return 0;
}
