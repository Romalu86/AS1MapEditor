#include "mapedit/runtime.hpp"

namespace {
void SwapFloat(float& left, float& right)
{
    const float value = left;
    left = right;
    right = value;
}
}

// This is the editor's rectangular unit fill/delete tool.  The slightly unusual
// chess-row offset is intentional and follows the retail MapEdit code exactly.
void MAP_EDIT::FillBox(float beginX, float beginY, float endX, float endY)
{
    if (spriteType >= 0x20)
        return;

    if (beginX > endX)
        SwapFloat(beginX, endX);
    if (beginY > endY)
        SwapFloat(beginY, endY);

    if (optDelete) {
        for (float x = beginX; x < endX; x += Mouse->Vid()->m_footprintWidth) {
            for (float y = beginY; y < endY; y += Mouse->Vid()->m_footprintHeight)
                DeleteUnit(x, y, spriteType);
        }
        return;
    }

    if (optChessSnap) {
        float y = beginY + Mouse->Vid()->m_snapOffsetY;
        int row = 0;
        while (y < endY) {
            const float rowOffset = (row & 1) ? 0.0f : Mouse->Vid()->m_snapOffsetX;
            float x = beginX + rowOffset;
            while (x < endX) {
                const float z = optGround0
                    ? GetGroundZ(Mouse->Vid(), x, y) + insertZ
                    : insertZ;
                InsertUnit(Mouse->Vid(), x, y, z, Mouse->Direction());
                x += Mouse->Vid()->m_footprintWidth;
            }
            ++row;
            y += Mouse->Vid()->m_footprintHeight;
        }
        return;
    }

    float y = beginY + Mouse->Vid()->m_snapOffsetY;
    while (y < endY) {
        float x = beginX + Mouse->Vid()->m_snapOffsetX;
        while (x < endX) {
            const float z = optGround0
                ? GetGroundZ(Mouse->Vid(), x, y) + insertZ
                : insertZ;
            InsertUnit(Mouse->Vid(), x, y, z, Mouse->Direction());
            x += Mouse->Vid()->m_footprintWidth;
        }
        y += Mouse->Vid()->m_footprintHeight;
    }
}

// "Romb" is the spelling used by the original source symbols.  The fill walks
// two diagonal axes; chess snap halves only the X step, exactly as retail does.
void MAP_EDIT::FillRomb(float beginX, float beginY, float endX, float endY)
{
    if (spriteType >= 0x20)
        return;

    float stepX = optChessSnap
        ? Mouse->Vid()->m_footprintWidth / 2.0f
        : Mouse->Vid()->m_footprintWidth;
    float stepY = Mouse->Vid()->m_footprintHeight;

    const int rowCount = static_cast<int>(
        (fabsf(endX - beginX) / stepX + fabsf(endY - beginY) / stepY) / 2.0f + 1.0f);
    const int columnCount = static_cast<int>(
        fabsf(fabsf(endY - beginY) / stepY - fabsf(endX - beginX) / stepX) / 2.0f + 1.0f);

    if (beginX > endX)
        stepX = -stepX;
    if (beginY > endY)
        stepY = -stepY;

    for (int row = 0; row < rowCount; ++row) {
        for (int column = 0; column < columnCount; ++column) {
            const float x = beginX + static_cast<float>(row - column) * stepX;
            const float y = beginY + static_cast<float>(row + column) * stepY;

            if (optDelete) {
                DeleteUnit(x, y, spriteType);
            } else {
                const float z = optGround0
                    ? GetGroundZ(Mouse->Vid(), x, y) + insertZ
                    : insertZ;
                InsertUnit(Mouse->Vid(), x, y, z, Mouse->Direction());
            }
        }
    }
}

extern "C" int __cdecl rand(void);

int Random(int interval)
{
    return rand() % (interval + 1);
}

SPRITE* MAP_EDIT::InsertUnit(VID* vid,float x,float y,float z,ANGLE direction)
{
    if (!vid)
        return 0;

    if (spriteType < 0x20 && optAirBrush) {
        // VC6's signed divide-by-two sequence rounds toward zero.
        const int halfAirBrushSize=optAirBrushSize / 2;
        x += static_cast<float>(halfAirBrushSize - Random(optAirBrushSize));
        y += static_cast<float>(halfAirBrushSize - Random(optAirBrushSize));
        if (!ValidateXY(x,y))
            return 0;

        if (optGround0)
            z=GetGroundZ(vid,x,y)+insertZ;

        int index=0;
        for (SPRITE* sprite=FirstSprite(vid->m_layer,&index);
             sprite;
             sprite=NextSprite(vid->m_layer,&index)) {
            if (sprite->Vid()==vid &&
                sprite->IsXYCross(vid,x,y) &&
                sprite->IsZCross(vid,z))
                return 0;
        }
    }

    if (vid->m_spriteClass==2u && Hash->CanPlace(vid,x,y,z))
        return 0;

    for (int layer=0;layer<18;++layer) {
        int index=0;
        for (SPRITE* sprite=FirstSprite(layer,&index);
             sprite;
             sprite=NextSprite(layer,&index)) {
            if (sprite->X()!=x || sprite->Y()!=y || sprite->Z()!=z)
                continue;

            if (sprite->Vid()==vid) {
                ANGLE spriteDirection=sprite->Direction();
                if (spriteDirection==&direction || sprite->IsSpriteType(1u))
                    return 0;
            }

            if (layer <= vid->m_layer) {
                if ((vid->m_extraTypeFlags & 0x3Fu)==0) {
                    VID* existingVid=sprite->Vid();
                    if (vid->m_footprintWidth >= existingVid->m_footprintWidth &&
                        vid->m_footprintHeight >= existingVid->m_footprintHeight)
                        undo.AddRemove(sprite);
                }
            } else {
                VID* existingVid=sprite->Vid();
                if ((existingVid->m_extraTypeFlags & 0x3Fu)==0 &&
                    vid->m_footprintWidth <= existingVid->m_footprintWidth &&
                    vid->m_footprintHeight <= existingVid->m_footprintHeight)
                    return 0;
            }
        }
    }

    SPRITE* sprite;
    if (vid->IsSpriteType(0x10u) || vid->m_spriteClass==10u)
        sprite=CreateSprite(vid,ToScreenX(x),ToScreenY(y),z,direction,0);
    else
        sprite=CreateSprite(vid,x,y,z,direction,0);

    undo.AddInsert(sprite);
    if (optRandomDir)
        Mouse->ChangeDirection(ANGLE(static_cast<unsigned char>(Random(0xFF))));
    return sprite;
}

void MAP_EDIT::DeleteUnit(float x,float y,int sprite_type)
{
    SPRITE* sprite;
    if (optAirBrush) {
        sprite=FindNearestSprite(Mouse->Vid()->m_idx + 0x800,
                                 x,y,static_cast<float>(optAirBrushSize),0);
    } else {
        sprite=FindNearestSprite(sprite_type << 20,x,y,100.0f,0);
    }
    undo.AddRemove(sprite);
}

namespace {
// MapEdit.exe .data owners used by map_edit.cpp:1175..1306.
// These tables and initial values are byte-for-byte derived from the retail image.
static const int kRailDirCode[64]={
    0,4,36,36,36,36,36,5, 7,2,11,36,36,36,36,36,
    36,9,1,8,36,36,36,36, 36,36,10,3,5,36,36,36,
    36,36,36,6,0,7,36,36, 36,36,36,36,4,2,9,36,
    36,36,36,36,36,11,1,10, 6,36,36,36,36,36,8,3
};
static const int kRailVariant[64]={
    0,0,36,36,36,36,36,0, 1,0,1,36,36,36,36,36,
    36,0,0,0,36,36,36,36, 36,36,1,1,1,36,36,36,
    36,36,36,0,1,0,36,36, 36,36,36,36,1,1,1,36,
    36,36,36,36,36,0,1,0, 1,36,36,36,36,36,1,0
};
static const int kRailStepY[8]={0,1,1,1,0,-1,-1,-1};
static const int kRailStepX[8]={-1,-1,0,1,1,1,0,-1};
static const int kRailRealDirMap[12]={32,50,41,59,33,39,24,40,30,14,58,42};
static const int kRailTransition[64]={
    0,1,1,9,9,9,7,7, 0,1,2,2,9,9,9,0,
    1,1,2,3,3,9,9,9, 9,2,2,3,4,4,9,9,
    9,9,3,3,4,5,5,9, 9,9,9,4,4,5,6,6,
    7,9,9,9,5,5,6,7, 0,0,9,9,9,6,6,7
};

static int gRailLastDir=-1;     // retail 0x004BCE34
static float gRailPreviousZ=0.0f; // retail 0x004CE23C
static unsigned char gRailInitFlags=0; // retail 0x004CE234
static int gRailPositiveStep=0; // retail 0x004CE36C
static int gRailNegativeStep=0; // retail 0x004CE370
static int gRailZeroStep=0;     // retail 0x004CE374

static int OptRailDir(SPRITE* rail,int dirIn,int dirOut,int previousZ,int nextZ)
{
    if (!rail || dirOut>7 || dirIn>7)
        return 1;

    if (dirOut==-1 || dirIn==-1) {
        int iterator=0;
        SPRITE* sprite=Map->FirstSprite(rail->Vid()->m_layer,&iterator);
        while (sprite) {
            if (sprite->IsSpriteClass(0x16u) &&
                rail->X()==sprite->X() && rail->Y()==sprite->Y() && rail->Z()==sprite->Z() &&
                sprite!=rail) {
                const int realDirection=sprite->RealDirection()%12;
                for (int i=0;i<64;++i) {
                    if (kRailDirCode[i]==realDirection && dirIn==-1 &&
                        (dirOut==-1 || dirOut==(i>>3) ||
                         dirOut==(((i>>3)+1)&7) || dirOut==(((i>>3)-1)&7))) {
                        dirIn=i>>3;
                    }
                }
            }
            sprite=Map->NextSprite(rail->Vid()->m_layer,&iterator);
        }
    }

    if (dirIn==-1)
        dirIn=dirOut;
    if (dirOut==-1)
        dirOut=dirIn;

    const int tableIndex=(dirIn<<3)|dirOut;
    int variant=kRailVariant[tableIndex];
    if (variant!=36) {
        const float railZ=rail->Z();
        if (static_cast<float>(nextZ)>railZ)
            variant=(variant+1)*12;
        else if (static_cast<float>(previousZ)>railZ)
            variant=((variant^1)+1)*12;
        else
            variant=0;
    }

    const int directionCode=kRailDirCode[tableIndex];
    const long actionDirection=((directionCode+variant)<<8) / static_cast<int>(rail->Vid()->m_noDirections);
    rail->Action(0x3c,actionDirection,0,0);
    return directionCode==36;
}

static int RailDirectionFromVector(float x,float y,int* radius)
{
    ANGLE angle(x,y,radius);
    return ((static_cast<int>(angle.value)+0x10)&0xff)>>5;
}
}

void MAP_EDIT::CreateNewRail()
{
    if (!(gRailInitFlags&1u)) {
        gRailInitFlags|=1u;
        const int step=static_cast<int>(Mouse->Vid()->m_hitVerticalOffset);
        gRailPositiveStep=step;
        gRailNegativeStep=-step;
        gRailZeroStep=0;
    }

    if (selectedSprites.No()==0) {
        gRailLastDir=-1;
        gRailPreviousZ=Mouse->Z();
        SPRITE* inserted=InsertUnit(Mouse->Vid(),Mouse->X(),Mouse->Y(),Mouse->Z(),
                                    ANGLE(Mouse->Vid()->m_editorDirectionOffset));
        selectedSprites.Insert(inserted);
        return;
    }

    SPRITE* rail=*selectedSprites[selectedSprites.No()-1];

    int size1=0;
    int currentDir=RailDirectionFromVector(m_input.mouseX-rail->X(),
                                           m_input.mouseY-rail->Y(),&size1);
    if (gRailLastDir!=-1)
        currentDir=kRailTransition[(gRailLastDir<<3)|currentDir];

    const float deltaY=m_input.mouseY-rail->Y()-
        static_cast<float>(kRailStepX[currentDir])*Mouse->Vid()->m_footprintHeight;
    const float deltaX=m_input.mouseX-rail->X()-
        static_cast<float>(kRailStepY[currentDir])*Mouse->Vid()->m_footprintWidth;
    int size2=0;
    int nextDir=RailDirectionFromVector(deltaX,deltaY,&size2);
    nextDir=kRailTransition[(currentDir<<3)|nextDir];

    if (selectedSprites.No()>1)
        gRailPreviousZ=(*selectedSprites[selectedSprites.No()-2])->Z();

    // Retail compares the integer CORDIC radii returned by ANGLE(x,y,&radius):
    // an intermediate piece is emitted only after the cursor crosses one rail step.
    if (size2<size1 && nextDir!=9) {
        float newZ=insertZ;
        if (optGround0) {
            const float x=rail->X()+static_cast<float>(kRailStepY[currentDir])*Mouse->Vid()->m_footprintWidth;
            const float y=rail->Y()+static_cast<float>(kRailStepX[currentDir])*Mouse->Vid()->m_footprintHeight;
            newZ+=GetGroundZ(Mouse->Vid(),x,y);
        }

        if (newZ>=0.0f)
            newZ=(newZ+Mouse->Vid()->m_hitVerticalOffset-1.0f)/Mouse->Vid()->m_hitVerticalOffset*Mouse->Vid()->m_hitVerticalOffset;
        else
            newZ=(newZ-Mouse->Vid()->m_hitVerticalOffset+1.0f)/Mouse->Vid()->m_hitVerticalOffset*Mouse->Vid()->m_hitVerticalOffset;

        Mouse->ChangeCoor(Mouse->X(),Mouse->Y(),newZ);
        OptRailDir(rail,gRailLastDir,currentDir,
                   static_cast<int>(gRailPreviousZ),static_cast<int>(Mouse->Z()));
        gRailPreviousZ=rail->Z();

        float insertRailZ=gRailPreviousZ;
        if (Mouse->Z()<gRailPreviousZ)
            insertRailZ-=Mouse->Vid()->m_hitVerticalOffset;
        else if (Mouse->Z()>gRailPreviousZ)
            insertRailZ+=Mouse->Vid()->m_hitVerticalOffset;

        SPRITE* inserted=InsertUnit(Mouse->Vid(),
            rail->X()+static_cast<float>(kRailStepY[currentDir])*Mouse->Vid()->m_footprintWidth,
            rail->Y()+static_cast<float>(kRailStepX[currentDir])*Mouse->Vid()->m_footprintHeight,
            insertRailZ,ANGLE(static_cast<uint8_t>(1)));
        if (!inserted)
            return;

        selectedSprites.Insert(inserted);
        gRailLastDir=currentDir;
        currentDir=nextDir;
        rail=inserted;
    }

    if (OptRailDir(rail,gRailLastDir,currentDir,
                   static_cast<int>(gRailPreviousZ),static_cast<int>(Mouse->Z()))) {
        selectedSprites.Delete(rail);
        undo.DeleteLast();
        rail->ScalarDeletingDestructor(1);

        if (selectedSprites.No()==1) {
            gRailLastDir=-1;
            return;
        }

        rail=*selectedSprites[selectedSprites.No()-1];
        const int realDirection=rail->RealDirection()%12;
        if (gRailLastDir==(kRailRealDirMap[realDirection]&7))
            gRailLastDir=(kRailRealDirMap[realDirection]>>3)^4;
        else
            gRailLastDir=(kRailRealDirMap[realDirection]&7)^4;
    }
}

float Random(float interval)
{
    return static_cast<float>(rand())*interval/32767.0f;
}

