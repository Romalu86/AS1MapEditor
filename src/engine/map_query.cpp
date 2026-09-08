#include "mapedit/runtime.hpp"

namespace {
enum SpriteTypeMask {
    U_TERRAIN = 0x001,
    U_OBJECT  = 0x002,
    U_UNIT    = 0x004,
    U_AVIA    = 0x008,
    U_MENU    = 0x010,
    U_RAILWAY = 0x020,
    U_REGION  = 0x040,
    U_CANNON  = 0x200,
    U_SPRITE  = 0x400
};

bool IsExtendedVidQuery(int type)
{
    return (type&0x4000)!=0;
}

bool IsVidQuery(int type)
{
    return IsExtendedVidQuery(type) || (type&0x800)!=0;
}

int VidQueryIndex(int type)
{
    return IsExtendedVidQuery(type) ? (type&0x1FFF) : (type&0x7FF);
}

bool PreferScreenHit(SPRITE* candidate,SPRITE* current)
{
    return !current ||
           candidate->Vid()->m_footprintWidth < current->Vid()->m_footprintWidth ||
           candidate->Vid()->m_footprintHeight < current->Vid()->m_footprintHeight;
}

bool ArmyMatches(SPRITE* sprite,int armyMask)
{
    return ((0x10000u << sprite->Army()) & (unsigned int)armyMask) != 0;
}
}

int IsSpriteCorrectForGetSprite(SPRITE* sprite,int type)
{
    if ((type & (int)0x80000000u) && !sprite->IsCommand(0))
        return 0;
    if (!IsExtendedVidQuery(type) && (type & 0x1000) && !sprite->IsSpriteClass((unsigned int)(type & 0x7FF)))
        return 0;
    if (IsVidQuery(type) && sprite->Vid()->m_idx != VidQueryIndex(type))
        return 0;
    return 1;
}

SPRITE* MAP::GetSpriteScr(int type,float screenX,float screenY)
{
    const float radius = 300.0f;
    int spriteArmy = type & 0xF0000;
    if (!spriteArmy)
        spriteArmy = 0xF0000;

    int spriteType;
    const bool vidQuery = IsVidQuery(type);
    const int vidIndex = VidQueryIndex(type);
    if (vidQuery) {
        VID* queryVid = Vid(vidIndex);
        if (!queryVid->NoSprites())
            return 0;
        if (queryVid->PropHash())
            type |= 0x8000;
        spriteType = queryVid->m_unknown0C;
    } else {
        spriteType = (type >> 20) & 0x67F;
        if (!spriteType)
            spriteType = 0x67F;
    }

    SPRITE* found = 0;

    if (type & 0x8000) {
        for (SPRITE* sprite = Hash->FirstInBox(screenX-radius,
                                               screenY-radius,
                                               screenX+radius,
                                               GetGroundZ(screenX,screenY)+screenY+radius);
             sprite;
             sprite = Hash->NextInBox()) {
            if (!sprite->IsLinked() &&
                sprite->IsSpriteType((unsigned int)spriteType) &&
                ArmyMatches(sprite,spriteArmy) &&
                IsSpriteCorrectForGetSprite(sprite,type) &&
                sprite->IsInside(screenX,screenY) &&
                PreferScreenHit(sprite,found)) {
                found = sprite;
            }
        }
        return found;
    }

    if ((spriteType & (U_UNIT | U_AVIA)) &&
        !(spriteType & (U_TERRAIN | U_OBJECT | U_MENU | U_RAILWAY | U_REGION | U_CANNON | U_SPRITE))) {
        int index = 0;
        for (SPRITE* sprite = Hash->FirstUnit(&index); sprite; sprite = Hash->NextUnit(&index)) {
            if (sprite->IsSpriteType((unsigned int)spriteType) &&
                ArmyMatches(sprite,spriteArmy) &&
                IsSpriteCorrectForGetSprite(sprite,type) &&
                sprite->IsInside(screenX,screenY) &&
                PreferScreenHit(sprite,found)) {
                found = sprite;
            }
        }
        return found;
    }

    if (vidQuery && Vid(vidIndex)->m_spriteClass == 10u) {
        int index = 0;
        for (SPRITE* sprite = m_menu.BeginIterate(&index); sprite; sprite = m_menu.NextIterate(&index)) {
            if (!sprite->IsLinked() &&
                sprite->IsSpriteType((unsigned int)spriteType) &&
                ArmyMatches(sprite,spriteArmy) &&
                IsSpriteCorrectForGetSprite(sprite,type) &&
                sprite->IsInside(screenX,screenY) &&
                PreferScreenHit(sprite,found)) {
                found = sprite;
            }
        }
        return found;
    }

    const int beginLayer = vidQuery ? Vid(vidIndex)->m_layer : 0;
    const int endLayer = vidQuery ? beginLayer + 1 : 17;
    for (int layer=beginLayer; layer<endLayer; ++layer) {
        int index;
        for (SPRITE* sprite = FirstSprite(layer,&index); sprite; sprite = NextSprite(layer,&index)) {
            if (sprite->IsLinked() ||
                !sprite->IsSpriteType((unsigned int)spriteType) ||
                !ArmyMatches(sprite,spriteArmy) ||
                !IsSpriteCorrectForGetSprite(sprite,type)) {
                continue;
            }

            const bool inside = (spriteType & U_REGION) && sprite->IsSpriteClass(23)
                ? static_cast<REGION*>(sprite)->IsInsideScr(screenX,screenY) != 0
                : sprite->IsInside(screenX,screenY) != 0;
            if (inside && PreferScreenHit(sprite,found))
                found = sprite;
        }
    }
    return found;
}

SPRITE* MAP::FindNearestSprite(int type,float x,float y,float radius,SPRITE* prev)
{
    float findSize=radius;
    SPRITE* findSprite=0;
    int spriteArmy=type & 0xF0000;
    const float prevSize=prev ? prev->NearDistanceTo(x,y) : -1.0f;
    if (!spriteArmy)
        spriteArmy=0xF0000;

    const bool vidQuery=IsVidQuery(type);
    const int vidIndex=VidQueryIndex(type);
    int spriteType;
    if (vidQuery) {
        VID* queryVid=Vid(vidIndex);
        if (!queryVid->NoSprites())
            return 0;
        if (queryVid->PropHash())
            type |= 0x8000;
        spriteType=queryVid->m_unknown0C;
    } else {
        spriteType=(type & 0x67F00000) >> 20;
        if (!spriteType)
            spriteType=0x67F;
    }

    // Retail has four explicit iterator paths. Keep the candidate body explicit as well:
    // no lambda/helper callable is introduced into Debug code generation.
    if (type & 0x8000) {
        for (SPRITE* sprite=Hash->FirstInBox(x-radius,y-radius,x+radius,y+radius);
             sprite;
             sprite=Hash->NextInBox()) {
            if (sprite->IsLinked() ||
                !sprite->IsSpriteType((unsigned int)spriteType) ||
                (((0x10000u << sprite->Army()) & (unsigned int)spriteArmy) == 0) ||
                !IsSpriteCorrectForGetSprite(sprite,type))
                continue;
            const float size=sprite->NearDistanceTo(x,y);
            if (size < findSize && size > prevSize) {
                findSprite=sprite;
                findSize=size;
            }
        }
        return findSprite;
    }

    if ((spriteType & 0x0C) && !(spriteType & 0x673)) {
        int index=0;
        for (SPRITE* sprite=Hash->FirstUnit(&index); sprite; sprite=Hash->NextUnit(&index)) {
            if (sprite->IsLinked() ||
                !sprite->IsSpriteType((unsigned int)spriteType) ||
                (((0x10000u << sprite->Army()) & (unsigned int)spriteArmy) == 0) ||
                !IsSpriteCorrectForGetSprite(sprite,type))
                continue;
            const float size=sprite->NearDistanceTo(x,y);
            if (size < findSize && size > prevSize) {
                findSprite=sprite;
                findSize=size;
            }
        }
        return findSprite;
    }

    if (vidQuery) {
        const unsigned int spriteClass=Vid(vidIndex)->m_spriteClass;
        if (spriteClass==10u || spriteClass==19u) {
            int index=0;
            for (SPRITE* sprite=m_menu.BeginIterate(&index); sprite; sprite=m_menu.NextIterate(&index)) {
                if (sprite->IsLinked() ||
                    !sprite->IsSpriteType((unsigned int)spriteType) ||
                    (((0x10000u << sprite->Army()) & (unsigned int)spriteArmy) == 0) ||
                    !IsSpriteCorrectForGetSprite(sprite,type))
                    continue;
                const float size=sprite->NearDistanceTo(x,y);
                if (size < findSize && size > prevSize) {
                    findSprite=sprite;
                    findSize=size;
                }
            }
            return findSprite;
        }
    }

    const int beginLayer=vidQuery ? Vid(vidIndex)->m_layer : 0;
    const int endLayer=vidQuery ? beginLayer+1 : 17;
    for (int layer=beginLayer;layer<endLayer;++layer) {
        int index=0;
        for (SPRITE* sprite=FirstSprite(layer,&index); sprite; sprite=NextSprite(layer,&index)) {
            if (sprite->IsLinked() ||
                !sprite->IsSpriteType((unsigned int)spriteType) ||
                (((0x10000u << sprite->Army()) & (unsigned int)spriteArmy) == 0) ||
                !IsSpriteCorrectForGetSprite(sprite,type))
                continue;
            const float size=sprite->NearDistanceTo(x,y);
            if (size < findSize && size > prevSize) {
                findSprite=sprite;
                findSize=size;
            }
        }
    }
    return findSprite;
}

void MAP::FindSpritesInsidePolygon(int type,const POLYGON* polygon,SPRITE_LIST* list)
{
    int spriteArmy = type & 0xF0000;
    if (!spriteArmy)
        spriteArmy = 0xF0000;

    int spriteType;
    const bool vidQuery = IsVidQuery(type);
    const int vidIndex = VidQueryIndex(type);
    if (vidQuery) {
        VID* queryVid = Vid(vidIndex);
        if (queryVid->PropHash())
            type |= 0x8000;
        spriteType = queryVid->m_unknown0C;
    } else {
        spriteType = (type >> 20) & 0x67F;
        if (!spriteType)
            spriteType = 0x67F;
    }

    POLYGON* mutablePolygon = const_cast<POLYGON*>(polygon);
    if (type & 0x8000) {
        for (SPRITE* sprite = Hash->FirstInBox(-100.0f,-100.0f,SizeX()+100.0f,SizeY()+100.0f);
             sprite;
             sprite = Hash->NextInBox()) {
            if (!sprite->IsLinked() &&
                sprite->IsSpriteType((unsigned int)spriteType) &&
                ArmyMatches(sprite,spriteArmy) &&
                IsSpriteCorrectForGetSprite(sprite,type) &&
                mutablePolygon->AskInside(sprite->X(),sprite->Y()-sprite->Z())) {
                list->InsertUnique(sprite);
            }
        }
        return;
    }

    if ((spriteType & (U_UNIT | U_AVIA)) &&
        !(spriteType & (U_TERRAIN | U_OBJECT | U_MENU | U_RAILWAY | U_REGION | U_CANNON | U_SPRITE))) {
        int index = 0;
        for (SPRITE* sprite = Hash->FirstUnit(&index); sprite; sprite = Hash->NextUnit(&index)) {
            if (!sprite->IsLinked() &&
                sprite->IsSpriteType((unsigned int)spriteType) &&
                ArmyMatches(sprite,spriteArmy) &&
                IsSpriteCorrectForGetSprite(sprite,type) &&
                mutablePolygon->AskInside(sprite->X(),sprite->Y()-sprite->Z())) {
                list->InsertUnique(sprite);
            }
        }
        return;
    }

    const int beginLayer = vidQuery ? Vid(vidIndex)->m_layer : 0;
    const int endLayer = vidQuery ? beginLayer + 1 : 17;
    for (int layer=beginLayer; layer<endLayer; ++layer) {
        int index;
        for (SPRITE* sprite = FirstSprite(layer,&index); sprite; sprite = NextSprite(layer,&index)) {
            if (!sprite->IsLinked() &&
                sprite->IsSpriteType((unsigned int)spriteType) &&
                ArmyMatches(sprite,spriteArmy) &&
                IsSpriteCorrectForGetSprite(sprite,type) &&
                mutablePolygon->AskInside(sprite->X(),sprite->Y()-sprite->Z())) {
                list->InsertUnique(sprite);
            }
        }
    }
}

int MAP::IsPaused()
{
    return static_cast<int>((m_flags >> 4) & 1u);
}

STRING MAP::GetMouseTipsString()
{
    STRING result("");
    if (!IsPaused())
        result=m_player[m_curArmy]->GetMouseTipsString();

    if (result.operator==("") && m_menu.SpriteUnderCursor()) {
        STRING key="MenuVid"+Int2Str(m_menu.NVidUnderCursor());
        STRING section("MouseTips");
        STRING defaultString("");
        STRING allDir=key+"AllDir";
        result=Profile->GetString(&section,&allDir,&defaultString);
        if (result.operator==("")) {
            STRING dirKey=key+"Dir"+Int2Str(m_menu.NDirUnderCursor());
            result=Profile->GetString(&section,&dirKey,&defaultString);
        }
    }
    return result;
}


