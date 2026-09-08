#include "mapedit/runtime.hpp"

HASH_MAP::~HASH_MAP()
{
    units.Release();
    if (map) {
        for (int i=noX*noY-1;i>=0;--i)
            map[i].Release();
        delete[] map;
        map=0;
    }
}


int HASH_MAP::ConvX(float x)
{
    x*=scaleCellX;
    if (x<0.0f)
        return 0;
    if (x>=static_cast<float>(noX))
        return noX-1;
    return static_cast<int>(x);
}

int HASH_MAP::ConvY(float y)
{
    y*=scaleCellY;
    if (y<0.0f)
        return 0;
    if (y>=static_cast<float>(noY))
        return noY-1;
    return static_cast<int>(y);
}

SPRITE* HASH_MAP::FirstUnit(int* index)
{
    return units.BeginIterate(index);
}

SPRITE* HASH_MAP::NextUnit(int* index)
{
    return units.NextIterate(index);
}

SPRITE* HASH_MAP::FirstUnit()
{
    return units.BeginIterate(&curUnit);
}

SPRITE* HASH_MAP::NextUnit()
{
    return units.NextIterate(&curUnit);
}

namespace {
int HashCellX(HASH_MAP* hash,float x)
{
    x *= hash->scaleCellX;
    if (x < 0.0f)
        return 0;
    if (x >= (float)hash->noX)
        return hash->noX - 1;
    return (int)x;
}

int HashCellY(HASH_MAP* hash,float y)
{
    y *= hash->scaleCellY;
    if (y < 0.0f)
        return 0;
    if (y >= (float)hash->noY)
        return hash->noY - 1;
    return (int)y;
}
}

SPRITE* HASH_MAP::FirstInBox(float left,float top,float right,float bottom)
{
    begx = HashCellX(this,left - 1.0f / scaleCellX);
    begy = HashCellY(this,top - 1.0f / scaleCellY);
    endx = HashCellX(this,right + 1.0f / scaleCellX);
    endy = HashCellY(this,bottom + 1.0f / scaleCellY);
    curx = begx;
    curindex = 0;
    return NextInBox();
}

// The original debug metadata range continues over adjacent emitted iterator code;
// the reachable FirstInBox iterator path ends at the null return shown here.
SPRITE* HASH_MAP::NextInBox()
{
    while (begy <= endy) {
        while (curx <= endx) {
            SPRITE_LIST& cell = map[(begy << shiftY) + curx];
            if (curindex < cell.No())
                return *cell[curindex++];
            ++curx;
            curindex = 0;
        }
        ++begy;
        curx = begx;
        curindex = 0;
    }
    return 0;
}


void HASH_MAP::ChangeCoor(SPRITE* sprite,float x,float y)
{
    const int oldX=HashCellX(this,sprite->X());
    const int oldY=HashCellY(this,sprite->Y());
    const int newX=HashCellX(this,x);
    const int newY=HashCellY(this,y);
    if (oldX==newX && oldY==newY)
        return;

    SPRITE_LIST& oldCell=map[(oldY << shiftY) + oldX];
    if (oldCell.Delete(sprite) != 0)
        return;
    map[(newY << shiftY) + newX].Insert(sprite);
}

SPRITE* HASH_MAP::CanPlace(const VID* vid,float x,float y,float z)
{
    if (Map->GetGroundZ(vid,x,y) > z)
        return Mouse;
    if (vid->m_unknown18 == 0)
        return 0;

    SPRITE* sprite=FirstInBox(x-vid->m_snapOffsetX,
                              y-vid->m_snapOffsetY,
                              x+vid->m_snapOffsetX,
                              y+vid->m_snapOffsetY);
    while (sprite) {
        if (sprite->IsCross(vid,x,y,z) &&
            (sprite->Vid()->m_unknown18 & vid->m_unknown18) != 0)
            return sprite;
        sprite=NextInBox();
    }
    return 0;
}

// Retail uses the VC6 x87 float-to-int helper at 0x0049909C, which truncates
// toward zero.  static_cast<int> has the same conversion rule here.
int HASH_MAP::AskLine(const VID* vid,float x,float y,float z,
                      float* endx,float* endy,float* endz)
{
    if (!vid || vid->m_unknown18 == 0)
        return 0;

    int x2=static_cast<int>(x);
    int y2=static_cast<int>(y);
    int z2=static_cast<int>(z);
    unsigned int dx=static_cast<unsigned int>(
        abs(static_cast<int>(*endx)-static_cast<int>(x)));
    unsigned int dy=static_cast<unsigned int>(
        abs(static_cast<int>(*endy)-static_cast<int>(y)));
    unsigned int steep=0;
    int sx=(*endx>x) ? 1 : -1;
    int sy=(*endy>y) ? 1 : -1;

    if (dy>dx) {
        steep=1;
        int t=x2; x2=y2; y2=t;
        unsigned int ut=dx; dx=dy; dy=ut;
        t=sx; sx=sy; sy=t;
    }

    int e=static_cast<int>(2u*dy-dx);
    int sz=0;
    if (dx!=0) {
        sz=((static_cast<int>(*endz)-static_cast<int>(z))<<4) /
           static_cast<int>(dx);
    }

    for (unsigned int i=0;i<dx;++i) {
        if ((i%16u)==0u && i>0u) {
            z2+=sz;
            SPRITE* hit;
            if (steep) {
                hit=CanPlace(vid,static_cast<float>(y2),static_cast<float>(x2),
                             static_cast<float>(z2));
                if (hit) {
                    *endx=static_cast<float>(y2);
                    *endy=static_cast<float>(x2);
                    *endz=static_cast<float>(z2);
                    return 1;
                }
            } else {
                hit=CanPlace(vid,static_cast<float>(x2),static_cast<float>(y2),
                             static_cast<float>(z2));
                if (hit) {
                    *endx=static_cast<float>(x2);
                    *endy=static_cast<float>(y2);
                    *endz=static_cast<float>(z2);
                    return 1;
                }
            }
        }

        while (e>=0) {
            y2+=sy;
            e-=static_cast<int>(2u*dx);
        }
        x2+=sx;
        e+=static_cast<int>(2u*dy);
    }
    return 0;
}

void HASH_MAP::Insert(SPRITE* sprite)
{
    if (sprite->Vid()->PropHash()) {
        int x=static_cast<int>(sprite->X()*scaleCellX);
        if (x<0) x=0;
        else if (x>=noX) x=noX-1;
        int y=static_cast<int>(sprite->Y()*scaleCellY);
        if (y<0) y=0;
        else if (y>=noY) y=noY-1;
        if (map[x+(y<<shiftY)].InsertUnique(sprite))
            sprite->Error(10,"hash second insert",0);
    }
    if (sprite->IsSpriteType(0x0Cu)) {
        if (units.InsertUnique(sprite))
            sprite->Error(10,"hash second unit insert",0);
    }
}

int HASH_MAP::Delete(SPRITE* sprite)
{
    int result=0;
    if (map && sprite->Vid()->PropHash()) {
        int x=static_cast<int>(sprite->X()*scaleCellX);
        if (x<0) x=0;
        else if (x>=noX) x=noX-1;
        int y=static_cast<int>(sprite->Y()*scaleCellY);
        if (y<0) y=0;
        else if (y>=noY) y=noY-1;

        if (x==curx && y==begy && curindex>0 &&
            curindex<map[x+(y<<shiftY)].No() &&
            *map[x+(y<<shiftY)][curindex-1]==sprite)
            --curindex;

        result=map[x+(y<<shiftY)].Delete(sprite);
    }
    if (sprite->IsSpriteType(0x0Cu))
        result|=2*units.Delete(sprite);

    if (result && sprite!=Mouse && (!Mouse || sprite!=Mouse->m_child))
        sprite->Error(10,"hash can't delete",static_cast<unsigned long>(result));
    return result;
}

HASH_MAP::HASH_MAP(float size_x,float size_y,MAP* owner,int no_vid)
{
    // Retail 0x00450BF0 initializes only these iterator fields before the
    // sizing pass; the remaining box iterator fields are left untouched until
    // their respective query owners assign them.
    curUnit=0;
    curindex=0;

    float max_x=0.0f;
    float max_y=0.0f;
    for (int i=0;i<no_vid;++i) {
        VID* vid=owner->VidSlot(i);
        // reference behavior @ 0x00450C54 calls VID::PropHash @ 0x0041F5F0.
        // The pre-A11 implementation incorrectly used PropGround here.
        if (!vid || !vid->PropHash())
            continue;
        if (vid->m_footprintWidth>max_x)
            max_x=vid->m_footprintWidth;
        if (vid->m_footprintHeight>max_y)
            max_y=vid->m_footprintHeight;
    }


    // Retail divides both maxima by the 2.0f constant at 0x004B50BC.
    max_x/=2.0f;
    max_y/=2.0f;
    int shift_x=0;
    int shift_y_cell=0;
    while (static_cast<float>(1<<shift_x)<max_x)
        ++shift_x;
    while (static_cast<float>(1<<shift_y_cell)<max_y)
        ++shift_y_cell;

    scaleCellX=1.0f/static_cast<float>(1<<shift_x);
    scaleCellY=1.0f/static_cast<float>(1<<shift_y_cell);
    noY=static_cast<int>((size_y-1.0f)*scaleCellY+3.0f);

    shiftY=0;
    const float need_x=scaleCellX*(size_x-1.0f)+1.0f;
    while (static_cast<float>(1<<shiftY)<need_x)
        ++shiftY;
    noX=1<<shiftY;


    map=new SPRITE_LIST[noX*noY];
    if (!map)
        MYERROR::LogExit(Error,"!!!ERROR!!!HASH_MAP: Enough memory %i,%i",noX,noY);
}
