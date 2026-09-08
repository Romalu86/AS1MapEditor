#include "mapedit/runtime.hpp"

// Source-integrated shared closure owners derived directly from MapEdit.exe.
// These are intentionally grouped because they are common dependencies of
// MAP::CreateSprite, MAP::Load, MAP::ExecFunc and MAP_EDIT::WorkWndMessage.

SPRITE* RELATION::Decode(SPRITE* old_sprite)
{
    const int index=oldSprites.Location(reinterpret_cast<SPRITE* const*>(&old_sprite));
    if(index<0)
        return 0;
    return *newSprites[index];
}

void RELATION::Insert(SPRITE* old_sprite,SPRITE* new_sprite)
{
    if(!old_sprite)
        return;
    oldSprites.Insert(old_sprite);
    newSprites.Insert(new_sprite);
}

SPRITE* MAP::Decode(SPRITE* old_sprite)
{
    return m_relation.Decode(old_sprite);
}

int MAP::ScriptRun(int n_func,const SPRITE* var1,const SPRITE* var2,int var3)
{
    if(m_flags&0x80000u)
        return 0;
    return m_logic.CallFunction(n_func,var1,var2,var3);
}

void MAP::RemoveSpriteFromLayer(SPRITE* spr)
{
    const int layer=spr->Vid()->m_layer;
    for(int i=m_layers[layer].No()-1;i>=0;--i) {
        SPRITE** const slot=m_layers[layer][i];
        if(slot && *slot==spr) {
            *slot=0;
            break;
        }
    }
}

void MAP::InsertSpriteToLayer(SPRITE* spr)
{
    m_layers[spr->Vid()->m_layer].Insert(spr);
    spr->Release();
}

EX_SPRITE_DATA* SPRITE::ExData()
{
    return m_exData;
}

SPRITE* SPRITE::Goal()
{
    return m_goal;
}

void SPRITE::SetGoal(SPRITE* new_goal)
{
    if(m_goal==new_goal)
        return;
    if(m_goal)
        m_goal->Release();
    m_goal=new_goal;
    if(m_goal)
        m_goal->AddRef();
}

void SPRITE::Remove()
{
    if(m_child)
        m_child->Remove();
    Map->RemoveSpriteFromLayer(this);
}

void SPRITE::Insert()
{
    if(m_child)
        m_child->Insert();
    Map->InsertSpriteToLayer(this);
}

int SPRITE::AddLink(SPRITE* additional)
{
    if(!additional || additional->m_parent)
        return 1;
    if(m_child) {
        m_child->m_parent=0;
        additional->AddLinkToLast(m_child);
    }
    m_child=additional;
    additional->m_parent=this;
    return 0;
}

int SPRITE::AddLinkToLast(SPRITE* additional)
{
    if(!additional || additional->m_parent)
        return 1;
    SPRITE* last=this;
    while(last->m_child)
        last=last->m_child;
    last->m_child=additional;
    additional->m_parent=last;
    return 0;
}

int PROFILE::Load(const STRING* filename)
{
    FileName=STRING::EMPTY;
    FileName=*filename;
    return 0;
}

int PROFILE::GetInt(const STRING* section,const STRING* keyword,int defaultValue)
{
    return static_cast<int>(GetPrivateProfileIntA(section->m_buf,keyword->m_buf,defaultValue,FileName.m_buf));
}

STRING PROFILE::GetString(const STRING* section,const STRING* keyword,const STRING* defaultString)
{
    char buffer[32767];
    GetPrivateProfileStringA(section->m_buf,keyword->m_buf,defaultString->m_buf,buffer,32767u,FileName.m_buf);
    return STRING(buffer);
}

int MENU::NVidUnderCursor()
{
    return sprite ? sprite->Vid()->m_idx : 0;
}

int MENU::NDirUnderCursor()
{
    return sprite ? sprite->RealDirection() : 0;
}

int SPRITE::SetCommand(int command,SPRITE* new_goal)
{
    if(IsCommand(18) && command!=18)
        m_unknown50=0;
    SetGoal(new_goal);
    if(HaveFightLink() && Link())
        Link()->SetCommand(command,new_goal);
    if(command<16 && !Goal()) {
        m_flag&=~0x7cu;
        return 1;
    }
    m_flag=(m_flag&~0x7cu)|((static_cast<unsigned int>(command)&0x1fu)<<2);
    return 0;
}

int SPRITE::SetCommandWithoutLink(int command,SPRITE* new_goal)
{
    if(((m_flag>>2)&0x1fu)==0x12u && command!=0x12)
        m_unknown50=0;
    SetGoal(new_goal);
    if(command<0x10 && !m_goal) {
        m_flag&=0xffffff83u;
        return 1;
    }
    m_flag=(m_flag&0xffffff83u)|((static_cast<unsigned int>(command)&0x1fu)<<2);
    return 0;
}

namespace {
int& RawSpriteHp(SPRITE* sprite)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(sprite)+0x68);
}
int& RawVidMaxHp(VID* vid,int army)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid)+0x3D8+4*(army&3));
}
int& RawVidDeathCounter(VID* vid,int army)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid)+0x3B8+4*(army&3));
}
int& RawVidBreakLinkFlag(VID* vid)
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(vid)+0x290);
}
VID*& RawVidBreakLinkVid(VID* vid)
{
    return *reinterpret_cast<VID**>(reinterpret_cast<unsigned char*>(vid)+0x284);
}
unsigned long& RawVidLastEntityTime(VID* vid)
{
    return *reinterpret_cast<unsigned long*>(reinterpret_cast<unsigned char*>(vid)+0x458);
}
}

int SPRITE::IsInUndo()
{
    return static_cast<int>((m_flag>>8)&1u);
}

int SPRITE::Hp()
{
    return RawSpriteHp(this);
}

int VID::GetMaxHp(int army)
{
    return RawVidMaxHp(this,army);
}

void VID::IncreaseNoSprites(int army)
{
    RawVidLastEntityTime(this)=RealCurrentTime;
    ++m_entitiesNumber[army];
}

void VID::DecreaseNoSprites(int army)
{
    if (m_entitiesNumber[army])
        --m_entitiesNumber[army];
}

int SPRITE::MaxHp()
{
    return m_vid->GetMaxHp(Army());
}

int SPRITE::DestroyLink(const VID* destroyed)
{
    SPRITE* owner=this;
    while (owner->m_child) {
        if (owner->m_child->m_vid==destroyed) {
            SPRITE* child=owner->m_child;
            SPRITE* grandChild=child->m_child;
            owner->m_child=grandChild;
            if (grandChild)
                grandChild->m_parent=owner;
            child->m_child=0;
            child->m_parent=0;
            child->ScalarDeletingDestructor(1u);
            return 1;
        }
        owner=owner->m_child;
    }
    return 0;
}

void SPRITE::ChangeHp(int newHp)
{
    int& hp=RawSpriteHp(this);
    if (newHp<=0 && *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(m_vid)+0x28)!=0) {
        if (!IsDying()) {
            ++RawVidDeathCounter(m_vid,Army());
            const int damage=hp-newHp;
            const int threshold=MaxHp()*3/2;
            if (damage>threshold && RawVidBreakLinkFlag(m_vid)!=0)
                ChangeAnimation(0x10);
            else
                ChangeAnimation(0x0F);
        }
        return;
    }

    const int half=MaxHp()/2;
    if (newHp>half && hp<=half)
        DestroyLink(RawVidBreakLinkVid(m_vid));
    if (newHp<=half && hp>half)
        ChangeAnimation(0x0D);
    hp=newHp;
}

void SPRITE::ChangeArmy(int army)
{
    const int oldArmy=Army();
    const int newArmy=army&3;
    m_flag=(m_flag&~0x1800u)|(static_cast<unsigned int>(newArmy)<<11);
    if (m_child)
        m_child->ChangeArmy(Army());

    const int newMax=m_vid->GetMaxHp(Army());
    const int oldMax=m_vid->GetMaxHp(oldArmy);
    if (newMax!=oldMax) {
        const int scaled=Hp()*256/oldMax*newMax/256;
        ChangeHp(scaled);
    }
    m_vid->DecreaseNoSprites(oldArmy);
    m_vid->IncreaseNoSprites(Army());
}

SPRITE* MAP::LoadSprite(STREAM* stream,int version)
{
    int pointerToken;
    int nvid;
    float x;
    float y;
    float z;
    ANGLE direction;
    int army;
    SPRITE* sprite=0;

    stream->Read(&pointerToken,4u);
    if (pointerToken==-1)
        return reinterpret_cast<SPRITE*>(-1);
    stream->Read(&nvid,4u);
    if (version>9) {
        stream->Read(&x,4u);
        stream->Read(&y,4u);
        stream->Read(&z,4u);
    } else {
        int coordinate;
        stream->Read(&coordinate,4u); x=static_cast<float>(coordinate);
        stream->Read(&coordinate,4u); y=static_cast<float>(coordinate);
        stream->Read(&coordinate,4u); z=static_cast<float>(coordinate);
    }
    // ANGLE is one byte, but the retail stream representation is a DWORD;
    // ANGLE::Read consumes four bytes and keeps the low byte.
    direction.Read(stream);
    stream->Read(&army,4u);

    if (ValidateVid(nvid))
        sprite=CreateSprite(VidSlot(nvid),x,y,z,direction,0);
    else
        Error(3,const_cast<char*>("sprite, this vid not exist"),static_cast<unsigned long>(nvid));

    m_relation.Insert(reinterpret_cast<SPRITE*>(pointerToken),sprite);
    if (sprite)
        sprite->ChangeArmy(army);
    return sprite;
}

void MENU::Error(TYPE_ERROR type,char* text,unsigned long err)
{
    MYERROR::Error(::Error,"MENU",static_cast<int>(type),text,err);
}

namespace {

// MapEdit.exe helper 0x0049909C (VC6 _ftol). MENU::Save consumes the low
// DWORD of the signed 64-bit truncation.  NaN/overflow produces the x87
// integer-indefinite value, whose low DWORD is zero.
inline int RetailFtolLow32ForMenu(float value)
{
    const double d=static_cast<double>(value);
    if (!(d>=-9223372036854775808.0 && d<9223372036854775808.0))
        return 0;
    const __int64 converted=static_cast<__int64>(d);
    return static_cast<int>(static_cast<unsigned int>(converted));
}

}

int MENU::Load(const STRING* name)
{
    RESOURCE file;
    if (file.OpenForRead(name,0x554E454Du)) {
        Error(E_OPEN,name->m_buf,0);
        return 1;
    }
    if (file.GoBegin(0x44414548u)) {
        Error(E_SECTION,const_cast<char*>("'HEAD'in menu"),0);
        return 1;
    }

    int version;
    int screenX;
    int screenY;
    int shiftX;
    int shiftY;
    file.Read(&version,4u);
    file.Read(&screenX,4u);
    file.Read(&screenY,4u);
    file.Read(&shiftX,4u);
    file.Read(&shiftY,4u);

    if (!file.GoNext(0x20525053u)) {
        for (;;) {
            SPRITE* spr=Map->LoadSprite(&file,version);
            if (spr==reinterpret_cast<SPRITE*>(-1))
                break;
            if (spr) {
                const float x=spr->X()-static_cast<float>(shiftX)-static_cast<float>(screenX/2)+Graph->SizeX()*0.5f;
                const float y=spr->Y()-static_cast<float>(shiftY)-static_cast<float>(screenY/2)+Graph->SizeY()*0.5f;
                spr->ChangeCoor(x,y,spr->Z());
                spr->Action(0x51,reinterpret_cast<long>(&file),version,0);
            }
            file.GoNextSub(0x20525053u);
        }
    } else if (!file.GoBegin(0x49525053u)) {
        for (;;) {
            SPRITE* spr=Map->LoadSprite(&file,version);
            if (spr==reinterpret_cast<SPRITE*>(-1))
                break;
            if (spr) {
                const float x=spr->X()-static_cast<float>(shiftX)-static_cast<float>(screenX/2)+Graph->SizeX()*0.5f;
                const float y=spr->Y()-static_cast<float>(shiftY)-static_cast<float>(screenY/2)+Graph->SizeY()*0.5f;
                spr->ChangeCoor(x,y,spr->Z());
            }
        }
    } else {
        Error(E_SECTION,const_cast<char*>("'SPR ' or 'SPRI' in menu"),0);
        return 1;
    }
    file.Close();
    return 0;
}

int MENU::DeleteFromFile(const STRING* name)
{
    RESOURCE file;
    if (file.OpenForRead(name,0x554E454Du)) {
        Error(E_OPEN,name->m_buf,0);
        return 1;
    }
    if (file.GoBegin(0x44414548u)) {
        Error(E_SECTION,const_cast<char*>("'HEAD'in menu"),0);
        return 1;
    }

    int version;
    int screenX;
    int screenY;
    int shiftX;
    int shiftY;
    file.Read(&version,4u);
    file.Read(&screenX,4u);
    file.Read(&screenY,4u);
    file.Read(&shiftX,4u);
    file.Read(&shiftY,4u);

    if (file.GoNext(0x20525053u)) {
        Error(E_SECTION,const_cast<char*>("'SPR ' in menu"),0);
        return 1;
    }

    for (;;) {
        int pointerToken;
        file.Read(&pointerToken,4u);
        if (pointerToken==-1)
            break;

        int nvid;
        float x;
        float y;
        float z;
        file.Read(&nvid,4u);
        file.Read(&x,4u);
        file.Read(&y,4u);
        file.Read(&z,4u);

        for (int i=0;i<No();++i) {
            SPRITE* spr=*Item(i);
            if (spr->Vid()->m_idx!=nvid)
                continue;

            const float targetX=x-static_cast<float>(shiftX)-static_cast<float>(screenX/2)+Graph->SizeX()/2.0f;
            if (fabsf(spr->X()-targetX)>=0.001f)
                continue;

            const float targetY=y-static_cast<float>(shiftY)-static_cast<float>(screenY/2)+Graph->SizeY()/2.0f;
            if (fabsf(spr->Y()-targetY)>=0.001f)
                continue;
            if (fabsf(spr->Z()-z)>=0.001f)
                continue;

            DeleteSpriteNumber(i);
            --i;
        }
        file.GoNextSub(0x20525053u);
    }

    file.Close();
    return 0;
}

int MENU::Save(const STRING* name)
{
    RESOURCE file;
    int version=12;
    if (file.OpenForWrite(name,0x554E454Du)) {
        Error(E_CREATE,name->m_buf,0);
        return 1;
    }

    file.PreAppend(0x44414548u,0);
    file.Write(&version,4u);
    int screenX=RetailFtolLow32ForMenu(Graph->SizeX());
    file.Write(&screenX,4u);
    int screenY=RetailFtolLow32ForMenu(Graph->SizeY());
    file.Write(&screenY,4u);
    int shiftX=RetailFtolLow32ForMenu(Map->FromScreenX(0.0f));
    file.Write(&shiftX,4u);
    int shiftY=RetailFtolLow32ForMenu(Map->FromScreenY(0.0f));
    file.Write(&shiftY,4u);
    file.PostAppend();

    for (int i=0;i<No();++i) {
        SPRITE* spr=*Item(i);
        if (spr->IsLinked() || spr->Vid()==EmptyVid || spr->IsInUndo())
            continue;
        file.PreAppend(0x20525053u,0);
        spr->Save(&file);
        spr->Action(0x50,reinterpret_cast<long>(&file),0,0);
        file.PostAppend();
    }
    file.PreAppend(0x20525053u,0);
    int end=-1;
    file.Write(&end,4u);
    file.PostAppend();
    file.Close();
    return 0;
}

namespace {
template<class T>
T& EngineRaw(ENGINE* engine,unsigned int offset)
{
    return *reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(engine)+offset);
}
}

void ENGINE::SetCommandToTrain(int command,int x,int y)
{
    SetCommandToTrain(command,0,RailMap.GetNearestDot(x,y),0);
}

// ENGINE is deliberately represented as a raw facade in runtime.hpp: this body
// uses the exact retail field offsets instead of inventing a derived layout.
void ENGINE::SetCommandToTrain(int command,SPRITE* target,R_DOT* dot_target,R_DOT* dot_target2)
{
    int suppressFightLinkCommand=0;
    if (command==30) {
        command=0;
        suppressFightLinkCommand=1;
    }

    if (!target && !dot_target && command!=29)
        command=0;

    if (command==24) {
        ENGINE* engine=FirstEngine();
        while (engine) {
            SPRITE* const sprite=reinterpret_cast<SPRITE*>(engine);
            if (sprite->Vid()->m_idx==0x55 && reinterpret_cast<UNIT*>(engine)->Ammo()>0)
                break;
            engine=engine->NextEngine();
        }
        if (!engine) {
            command=0;
            target=0;
            dot_target=0;
        }
    }

    R_DOT* pathDot=0;
    R_DOT* pathDot2=0;
    if (command==25) {
        pathDot=dot_target2 ? dot_target2 :
            RailMap.GetNearestDot(
                static_cast<int>(reinterpret_cast<SPRITE*>(this)->X()),
                static_cast<int>(reinterpret_cast<SPRITE*>(this)->Y()),
                static_cast<int>(reinterpret_cast<SPRITE*>(this)->Z()));
        pathDot2=dot_target;
    }

    ENGINE* engine=FirstEngine();
    while (engine) {
        SPRITE* const sprite=reinterpret_cast<SPRITE*>(engine);

        SPRITE*& commandOwner=EngineRaw<SPRITE*>(engine,0x9Cu);
        if (commandOwner) {
            commandOwner->Release();
            commandOwner=0;
        }

        if ((command==28 || command==29) && !IsCommandToAllTrain()) {
            commandOwner=reinterpret_cast<SPRITE*>(this);
            reinterpret_cast<SPRITE*>(this)->AddRef();
        }

        sprite->SetBestTarget(0);
        if (sprite->HaveFightLink())
            sprite->Link()->SetBestTarget(0);

        sprite->SetCommandWithoutLink(command,target);
        EngineRaw<int>(engine,0xB0u)=0;
        EngineRaw<R_DOT*>(engine,0xA0u)=dot_target;
        EngineRaw<R_DOT*>(engine,0xA4u)=pathDot;
        EngineRaw<R_DOT*>(engine,0xA8u)=pathDot2;
        EngineRaw<int>(engine,0xF0u)=0;
        EngineRaw<int>(engine,0xECu)=0;

        if (sprite->HaveFightLink()) {
            if (!suppressFightLinkCommand) {
                if (IsCommandToAllTrain() || engine==this) {
                    if (reinterpret_cast<SPRITE*>(this)->CanAttackThisSprite(target)) {
                        if (command==28) {
                            sprite->Link()->SetCommandWithoutLink(3,target);
                        } else if (command==29) {
                            if (sprite->AttackedSpriteType()==8u && target) {
                                SPRITE* const marker=new SPRITE(
                                    EmptyVid,
                                    target->X(),
                                    target->Y()+70.0f,
                                    target->Z()+70.0f,
                                    ANGLE(static_cast<uint8_t>(0)),
                                    0);
                                sprite->Link()->SetCommandWithoutLink(4,marker);
                            } else {
                                sprite->Link()->SetCommandWithoutLink(4,target);
                            }
                        } else {
                            sprite->Link()->SetCommandWithoutLink(0,static_cast<SPRITE*>(0));
                        }
                    } else {
                        sprite->Link()->SetCommandWithoutLink(0,static_cast<SPRITE*>(0));
                    }
                } else {
                    sprite->Link()->SetCommandWithoutLink(0,static_cast<SPRITE*>(0));
                }
            }
        }

        engine=engine->NextEngine();
    }

    if (command==23 || command==26 || command==27 || command==25) {
        reinterpret_cast<SPRITE*>(FirstEngine())->StartMove();
    } else if (command==24) {
        if (EngineRaw<R_DOT*>(this,0xB8u)==EngineRaw<R_DOT*>(this,0xA0u)) {
            FirstEngine()->Stop();
            ENGINE* item=FirstEngine();
            while (item) {
                SPRITE* const sprite=reinterpret_cast<SPRITE*>(item);
                if (sprite->Vid()->m_idx==0x55 && reinterpret_cast<UNIT*>(this)->Ammo()!=0)
                    EngineRaw<int>(item,0xECu)=1;
                item=item->NextEngine();
            }
        } else {
            reinterpret_cast<SPRITE*>(FirstEngine())->StartMove();
        }
    } else if (command==0) {
        FirstEngine()->Stop();
    }
}

STRING MAP::ScriptVariable(STRING name)
{
    return m_logic.GetVariableStr(&name);
}

VID* MAP::ReadVid(STREAM* res)
{
    int nvid;
    res->Read(&nvid,4);
    return ValidateVid(nvid) ? VidSlot(nvid) : 0;
}

void MAP::WriteVid(STREAM* res,const VID* vid)
{
    int empty=-1;
    res->Write(vid ? static_cast<const void*>(&vid->m_idx) : static_cast<const void*>(&empty),4);
}

int MAP::OptEnemyAttackNeutralTrains()
{
    return static_cast<int>((m_flags>>1)&1u);
}


// -----------------------------------------------------------------------------
// script_exec/map helper ring, derived from MapEdit.exe 0x00456BA0..0x00456E1D.
// -----------------------------------------------------------------------------
int MAP::PopInt() { return m_logic.PopInt(); }
const STRING* MAP::PopStr() { return m_logic.PopStr(); }
SPRITE* MAP::PopSprite() { return reinterpret_cast<SPRITE*>(m_logic.PopObject()); }
VID* MAP::PopVid(const char* operation)
{
    const int nvid=PopInt();
    VID* vid=Vid(nvid);
    if (vid==EmptyVid && ::Error)
        MYERROR::Log(::Error,"!!!ERROR!!!SCRIPT: Invalid nvid %s %i",operation,nvid);
    return vid;
}
void MAP::PushInt(int value) { m_logic.PushInt(value); }
void MAP::PushStr(const STRING* value) { m_logic.PushStr(value); }
void MAP::PushSprite(SPRITE* sprite) { m_logic.PushObject(sprite); }
STRING MAP::FileName() { return m_mapName; }
PLAYER* MAP::Player() { return m_player[m_curArmy]; }

void MAP::PauseOn()
{
    if (!(m_flags&0x10u)) PauseOldClock=CurrentTime;
    m_flags|=0x10u;
}
void MAP::PauseOff()
{
    if (m_flags&0x10u) {
        for (int layer=0;layer<18;++layer) {
            int index=0;
            for (SPRITE* spr=FirstSprite(layer,&index);spr;spr=NextSprite(layer,&index))
                spr->SetTime(PauseOldClock);
        }
        CurrentTime=PauseOldClock;
        PrevCurrentTime=PauseOldClock-10u;
    }
    m_flags&=~0x10u;
}

SPRITE* MAP::GetSprite(int type,float x,float y,SPRITE* prev)
{
    SPRITE* spr=FindNearestSprite(type,x,y,300.0f,prev);
    if (!spr) return 0;
    VID* vid=spr->Vid();
    if (!InSegment(x,spr->X(),vid->m_snapOffsetX)) return 0;
    if (!InSegment(y,spr->Y(),vid->m_snapOffsetY)) return 0;
    return spr;
}
