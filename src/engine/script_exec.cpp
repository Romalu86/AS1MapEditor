#include "mapedit/runtime.hpp"

static int g_execUnitType = 0;
static int g_execUnitIterator = 0;
static int g_execSpriteLayer = 0;
static int g_execSpriteIterator = 0;
static int g_trainArmy = 0;
static int g_trainOrdinal = 0;
static STRING g_execCommands;
static char* g_registrationInfo = STRING::EMPTY;

extern "C" void* __stdcall ShellExecuteA(HWND__*,const char*,const char*,const char*,const char*,int);

void __stdcall GetRegistrationInformation(char* newUserName)
{
    g_registrationInfo = newUserName ? newUserName : STRING::EMPTY;
}

unsigned char CountByteGamma(int b1,int b2,int time)
{
    if (b1>=0x80)
        b1-=0xFE;
    if (b2>=0x80)
        b2-=0xFE;
    return static_cast<unsigned char>(b1+(b2-b1)*time/255);
}

int CountGamma(int g1,int g2,int time)
{
    const unsigned int a=static_cast<unsigned int>(g1);
    const unsigned int b=static_cast<unsigned int>(g2);
    unsigned int result=0;
    result|=static_cast<unsigned int>(CountByteGamma(a&0xFFu,b&0xFFu,time));
    result|=static_cast<unsigned int>(CountByteGamma((a>>8)&0xFFu,(b>>8)&0xFFu,time))<<8;
    result|=static_cast<unsigned int>(CountByteGamma((a>>16)&0xFFu,(b>>16)&0xFFu,time))<<16;
    return static_cast<int>(result);
}

ENGINE* GetTrainEng(int army,int ordinal,int* physicalOrdinal)
{
    int physical=0;
    int logical=0;
    int iterator;
    for (SPRITE* sprite=Hash->FirstUnit(&iterator);
         sprite;
         sprite=Hash->NextUnit(&iterator)) {
        if (!sprite->IsSpriteClass(0x15))
            continue;
        ENGINE* engine=static_cast<ENGINE*>(sprite);
        if (!engine->IsFirst())
            continue;

        if (army==4 || engine->Army()==army) {
            ++physical;
            ++logical;
            if (logical==ordinal) {
                *physicalOrdinal=physical;
                ENGINE* train=engine->GetTrain();
                if (train && train->Army()!=engine->Army()) {
                    --logical;
                    continue;
                }
                return train ? train : engine;
            }
        } else {
            ++physical;
        }
    }
    *physicalOrdinal=0;
    return 0;
}

ENGINE* FirstTrain(int army)
{
    if (army<0)
        return 0;
    if (army>=4)
        army=3;
    g_trainArmy=army;
    g_trainOrdinal=0;
    ++g_trainOrdinal;
    int physical=0;
    return GetTrainEng(g_trainArmy,g_trainOrdinal,&physical);
}

ENGINE* NextTrain()
{
    ++g_trainOrdinal;
    int physical=0;
    return GetTrainEng(g_trainArmy,g_trainOrdinal,&physical);
}

namespace {
void SetVidChild(VID* vid,int slot,int value)
{
    if (!vid || slot<0 || slot>=17) return;
    if (!value) {
        vid->m_aniSpawnMode[slot]=0;
        vid->m_aniChildVid[slot]=0;
        return;
    }

    const int childNvid=abs(value);
    if (!Map->ValidateVid(childNvid)) {
        char errorText[]="SetVid child";
        Map->Error(4,errorText,static_cast<unsigned long>(value));
        return;
    }

    vid->m_aniSpawnMode[slot]=value;
    vid->m_aniChildVid[slot]=Map->Vid(childNvid);
    if (vid->m_aniChildVid[slot]->PropBirthAsSmoke())
        vid->m_exSpriteData|=1;
}
}

int MAP::ExecFunc(int command)
{
    static STRING execText;

    switch (command) {
    case 65: {
        SPRITE* parent=PopSprite(); int dir=PopInt(),z=PopInt(),y=PopInt(),x=PopInt(); VID* v=PopVid("for CreateSprite()");
        PushSprite(v==EmptyVid ? 0 : CreateSprite(v,(float)x,(float)y,(float)z,ANGLE((uint8_t)dir),parent)); return 0;
    }
    case 66: PushSprite(Flagman(PopInt())); return 0;
    case 68: {
        g_execUnitType=PopInt(); SPRITE* s=Hash->units.BeginIterate(&g_execUnitIterator);
        while (s && !(g_execUnitType & (int)s->m_vid->m_unknown0C)) s=Hash->units.NextIterate(&g_execUnitIterator);
        PushSprite(s); return 0;
    }
    case 69: {
        SPRITE* s=Hash->units.NextIterate(&g_execUnitIterator);
        while (s && !(g_execUnitType & (int)s->m_vid->m_unknown0C)) s=Hash->units.NextIterate(&g_execUnitIterator);
        PushSprite(s); return 0;
    }
    case 70: { SPRITE* prev=PopSprite(); int y=PopInt(),x=PopInt(),type=PopInt(); PushSprite(GetSprite(type,(float)x,(float)y,prev)); return 0; }
    case 71: { int y=PopInt(),x=PopInt(),type=PopInt(); PushSprite(GetSpriteScr(type,(float)x,(float)y)); return 0; }
    case 72: { SPRITE* prev=PopSprite(); int radius=PopInt(),y=PopInt(),x=PopInt(),type=PopInt(); PushSprite(FindNearestSprite(type,(float)x,(float)y,(float)radius,prev)); return 0; }
    case 74: { int bottom=PopInt(),right=PopInt(),top=PopInt(),left=PopInt(); PushSprite(Hash->FirstInBox((float)left,(float)top,(float)right,(float)bottom)); return 0; }
    case 75: PushSprite(Hash->NextInBox()); return 0;
    case 76: g_execSpriteLayer=0; g_execSpriteIterator=m_layers[0].m_no; PushSprite(NextSprite(0,&g_execSpriteIterator)); return 0;
    case 77: {
        SPRITE* s=NextSprite(g_execSpriteLayer,&g_execSpriteIterator);
        while (!s && g_execSpriteLayer<16) { ++g_execSpriteLayer; g_execSpriteIterator=m_layers[g_execSpriteLayer].m_no; s=NextSprite(g_execSpriteLayer,&g_execSpriteIterator); }
        PushSprite(s); return 0;
    }
    case 79: {
        int v3=PopInt(),v2=PopInt(),v1=PopInt(),action=PopInt(); SPRITE* s=PopSprite();
        if (!s) { PushInt(0); return 0; }
        if (action<17) { s->ChangeAnimation(action); PushInt(0); return 0; }
        int r=s->Action(action,v1,v2,v3);
        if (action==0x79) { const STRING* p=reinterpret_cast<const STRING*>(r); if (p) PushStr(p); else { STRING e(""); PushStr(&e); } }
        else if (action==0x5A || action==0x9C || action==0x9B || action==0x9A || action==0x65 || action==0x67) PushSprite(reinterpret_cast<SPRITE*>(r));
        else PushInt(r);
        return 0;
    }
    case 80: { int y=PopInt(),x=PopInt(); SPRITE* s=PopSprite(); PushInt(s ? (int)s->NearDistanceTo((float)x-s->X(),(float)y-s->Y()) : 60000); return 0; }
    case 82: { int v3=PopInt(),v2=PopInt(),v1=PopInt(),a=PopInt(); SPRITE* s=PopSprite(); if(s)s->AddActionAfterStop(a,v1,v2,v3); return 0; }
    case 83: { SPRITE* s=PopSprite(); PushInt(s&&s->m_vid?s->m_vid->m_idx:0); return 0; }
    case 84: { SPRITE* s=PopSprite(); if(s)s->ScalarDeletingDestructor(1); return 0; }
    case 85: { SPRITE* s=PopSprite(); PushInt(s?(int)s->X():0); return 0; }
    case 86: { SPRITE* s=PopSprite(); PushInt(s?(int)s->Y():0); return 0; }
    case 87: { SPRITE* s=PopSprite(); PushInt(s?(int)s->Z():0); return 0; }
    case 88: { SPRITE* s=PopSprite(); PushInt(s?s->Direction().Int():0); return 0; }
    case 89: { SPRITE* s=PopSprite(); PushInt(s?s->Animation():0); return 0; }
    case 90: { int y=PopInt(),x=PopInt(); SPRITE* s=PopSprite(); PushInt(s?s->DirectionTo((float)x,(float)y).Int():0); return 0; }
    case 96: {
        SPRITE* s=PopSprite(); STRING out("");
        if(s){ STRING a=s->GetTextActions(); STRING i=s->GetTextItems(); out=i+a; }
        PushStr(&out); return 0;
    }
    case 97: {
        const STRING* text=PopStr(); g_execCommands=text?*text:STRING(""); SPRITE* s=PopSprite();
        if(s){ s->SetTextItems(&g_execCommands); STRING actions=g_execCommands; if(actions.HaveSubStr("\2")) actions=actions.After("\2"); s->SetTextActions(&actions); }
        return 0;
    }
    case 98: LoadInEndTact(PopStr()); return 0;
    case 99: Save(*PopStr()); return 0;
    case 100: { if(!m_resource.IsOpen())m_resource.OpenForWrite(PopStr(),0x4F4D4544u); return 0; }
    case 101: {
        int ndir=PopInt(); VID* v=PopVid("for MenuFind"); SPRITE* found=0;
        if(v!=EmptyVid && v->NoSprites()!=0){ for(int i=0;i<m_menu.m_no;++i){SPRITE* s=m_menu.m_data[i]; if(s&&s->m_vid==v&&(ndir==999999||ndir==v->RealDirection(s->Direction()))){found=s;break;}} }
        PushSprite(found); return 0;
    }
    case 102: { const STRING* s=PopStr(); if(s)m_menu.Load(s); return 0; }
    case 103: { const STRING* s=PopStr(); m_menu.DeleteFromFile(s); return 0; }
    case 104: PushInt(m_menu.NVidUnderCursor()); return 0;
    case 105: PushInt(m_menu.NDirUnderCursor()); return 0;
    case 106: {
        int v3=PopInt(),v2=PopInt(),v1=PopInt(),a=PopInt(),ndir=PopInt(); VID* v=PopVid("for MenuAction");
        if(v!=EmptyVid) for(int i=0;i<m_menu.m_no;++i){SPRITE* s=m_menu.m_data[i];if(s&&s->m_vid==v&&(ndir==999999||ndir==v->RealDirection(s->Direction()))){if(a<17)s->ChangeAnimation(a);else s->Action(a,v1,v2,v3);}}
        return 0;
    }
    case 107: { int z=PopInt(),y=PopInt(),x=PopInt(),ndir=PopInt(); VID* v=PopVid("for MenuCreate"); if(v==EmptyVid){PushSprite(0);return 0;} int d=ndir*256/(int)v->m_noDirections; PushSprite(CreateSprite(v,(float)x,(float)(y+z),(float)z,ANGLE((uint8_t)d),0)); return 0; }
    case 108: PushSprite((m_menu.clickFlags&1)?m_menu.sprite:0); return 0;
    case 109: PushInt((int)m_input.screenMouseX); return 0;
    case 110: PushInt((int)m_input.screenMouseY); return 0;
    case 111: PushInt((int)m_input.key); return 0;
    case 112: { int on=PopInt(); if(on){PauseOn();Mouse->ChangeAnimation(0);}else PauseOff(); return 0; }
    case 113: { int type=PopInt(); if(type==-1)Mouse->Disable(); else if(type==256)Mouse->HardwareOn(); else if(type==257)Mouse->HardwareOff(); else {if(!Mouse->IsHardware())Mouse->Enable(); Mouse->ChangeAnimation(type);} return 0; }
    case 114: { int y=PopInt(),x=PopInt(); Player()->PutMessage(PopStr(),(float)x,(float)y); return 0; }
    case 115: PushInt((int)m_input.GetState()); return 0;
    case 116: { int y=PopInt(),x=PopInt(); SetShiftCoor((float)x,(float)y,0); return 0; }
    case 117: m_shiftFlag=(uint32_t)PopInt(); return 0;
    case 118: PushInt((int)m_shiftFlag); return 0;
    case 119: PushInt((int)Graph->SizeX()); return 0;
    case 120: PushInt((int)Graph->SizeY()); return 0;
    case 121: { int on=PopInt(); m_flags=(m_flags&~0x80u)|(on?0x80u:0u); return 0; }
    case 122: { int on=PopInt(); if(on)Player()->StateBarOn();else Player()->StateBarOff(); return 0; }
    case 123: { STRING key=*PopStr(); STRING section=*PopStr(); STRING def(""); STRING out=Profile->GetString(&section,&key,&def); PushStr(&out); return 0; }
    case 124: PopStr(); PostMessageA(m_hWnd,0x10,0,0); return 0;
    case 125: PushInt((int)ToScreenX((float)PopInt())); return 0;
    case 126: { int z=PopInt(),y=PopInt(); PushInt((int)ToScreenY((float)y,(float)z)); return 0; }
    case 127: PushSprite((m_menu.clickFlags&2)?m_menu.sprite:0); return 0;
    case 128: { int key=PopInt(),which=PopInt(); if(which==0x400){g_inputFirstPrimary=key;g_inputFirstSecondary=key;}else if(which==0x800){g_inputSecondPrimary=key;g_inputSecondSecondary=key;} return 0; }
    case 129: { int v1=PopInt(),v2=PopInt(); Mouse->Action(0x3F,v2,v1,0); return 0; }
    case 130: Sound->VolumeSound(PopInt()); return 0;
    case 131: Sound->VolumeMusic(PopInt()); return 0;
    case 132: Sound->PlaySFX(PopInt(),0,0); return 0;
    case 133: Sound->StopSFX(PopInt()); return 0;
    case 134: Sound->StopMusic(0); return 0;
    case 135: { int y=PopInt(),x=PopInt(),sfx=PopInt(); Sound->PlaySFXFromCoor(sfx,(float)x-m_shiftX-Graph->SizeX()*0.5f,(float)y-m_shiftY-Graph->SizeY()*0.5f); return 0; }
    case 136: { int loop=PopInt(); Sound->FadeAndPlayFile(PopStr(),loop,3000); return 0; }
    case 137: { int duration=PopInt(),v2=PopInt(),v1=PopInt(),eff=PopInt(); Graph->Effect(eff,v1,v2,duration); return 0; }
    case 138: Graph->SetEnvironment((unsigned int)PopInt()); return 0;
    case 139: PopInt(); return 0;
    case 140: { unsigned int val=(unsigned int)PopInt(); GAMMA gamma(GAMMA::DECODE,val); Graph->SetGamma(&gamma); return 0; }
    case 141: { int angle=PopInt(),force=PopInt(); Graph->SetWind(force,ANGLE((uint8_t)angle)); return 0; }
    case 142: Graph->PlayMovie(PopStr()); return 0;
    case 143: PushInt(Graph->IsPlayMovie()); return 0;
    case 144: Graph->StopMovie(); return 0;
    case 145: PushInt(Sound->IsPlayMusic()); return 0;
    case 146: { int time=PopInt(),g2=PopInt(),g1=PopInt(); PushInt(CountGamma(g1,g2,time)); return 0; }
    case 147: { GAMMA g=Graph->GetGamma(); PushInt((int)g.EncodeToDword()); return 0; }
    case 148: PushInt(Graph->GetEffectState(PopInt())); return 0;
    case 149: PushStr(&m_prevMap); return 0;
    case 150: { STRING s(g_registrationInfo); PushStr(&s); return 0; }
    case 151: PushStr(&m_mapName); return 0;
    case 152: {
        execText=PopStr();
        MYERROR::Log(::Error,"Exec '%s'",execText.CharPtr());
        STRING params=execText.After(" ");
        STRING file=execText.Before(" ");
        ShellExecuteA(0,0,file.CharPtr(),params.CharPtr()," ",5);
        return 0;
    }
    case 153: { int index=PopInt(); const STRING* s=PopStr(); PushInt((signed char)(*const_cast<STRING*>(s))[index]); return 0; }
    case 154: { const STRING* s=PopStr(); if(s)MYERROR::Log(::Error,"%s",s->m_buf); return 0; }
    case 155: PushInt(Random(PopInt())); return 0;
    case 156: { int z=PopInt(); VID* v=PopVid("for ChangeZUnit"); if(v!=EmptyVid){int it=m_layers[v->m_layer].m_no;for(SPRITE* s=NextSprite(v->m_layer,&it);s;s=NextSprite(v->m_layer,&it))if(s->m_vid==v)s->ChangeCoor(s->X(),s->Y(),(float)z);} return 0; }
    case 157: PushInt((int)CurrentTime); return 0;
    case 158: { int y=PopInt(),x=PopInt(); PushInt((int)GetGroundZ((float)x,(float)y)); return 0; }
    case 159: case 205: PushInt(const_cast<STRING*>(PopStr())->Length()); return 0;
    case 160: { SPRITE* s=PopSprite(); SetFlagman(PopInt(),s); return 0; }
    case 161: { int z=PopInt(),y=PopInt(),x=PopInt(); VID* v=PopVid("for CanPlace"); PushSprite(Hash->CanPlace(v,(float)x,(float)y,(float)z)); return 0; }
    case 162: {
        int type=PopInt(); VID* v=PopVid("for GetVid"); if(v==EmptyVid){PushInt(0);return 0;}
        switch(type){
        case 1:PushInt(v->m_baseHp);break; case 23:PushInt(v->m_weapon?(int)v->m_weapon->m_battleRange:0);break; case 26:PushInt(v->GetMaxAmmo());break;
        case 27:{STRING s(v->m_name);PushStr(&s);break;} case 28:PushInt(v->NoSprites());break; case 29:PushInt(v->Killed());break;
        case 30:case 31:case 32:case 33:PushInt(v->Killed(type-30));break; case 34:case 35:case 36:case 37:PushInt(v->NoSprites(type-34));break;
        case 38:case 39:case 40:case 41:PushInt(v->GetMaxHp(type-38));break; case 46:PushInt((int)v->m_unknown0C);break; case 47:PushInt((int)v->m_spriteClass);break;
        case 48:PushInt(v->m_defaultMaxSpeed==999999.0f?999999:(int)(v->m_defaultMaxSpeed*1000.0f));break; case 49:PushInt((int)v->m_defaultDeathTimer);break;
        case 50:PushInt(v->m_weapon?(int)v->m_weapon->m_detectRange:0);break; case 51:PushInt(v->m_weapon?(int)v->m_weapon->m_targetRadius:0);break;
        case 53:PushInt((int)v->m_noDirections);break; case 54:PushInt(v->m_unknown18);break; case 55:PushInt(v->m_weapon?v->m_weapon->m_buildTime:0);break;
        case 56:PushInt(v->PropHide());break; case 57:PushInt(v->PropNotCreateAsChild());break; case 58:PushInt(v->m_phaseRandomInterval);break;
        case 59:PushInt(v->m_linkVid?v->m_linkVid->m_idx:0);break; case 124:PushInt(v->m_fireDamage);break; case 125:PushInt(v->ReColored());break;
        case 126:case 127:case 128:case 129:PushInt(v->ReColored(type-126));break;
        default: if(type>=60&&type<77){VID* c=v->m_aniChildVid[type-60];PushInt(c?c->m_idx:0);}else if(type>=92&&type<109)PushInt(v->m_aniFireCount[type-92]);else {char e[]="GetVid type";Error(14,e,(unsigned long)type);} break;
        }
        return 0;
    }
    case 163: {
        const int value=PopInt();
        const int type=PopInt();
        VID* v=PopVid("for SetVid");
        if(v==EmptyVid)return 0;

        switch(type) {
        case 1:
            v->m_baseHp=value;
            break;

        case 18: case 19: case 20: case 21: {
            GAMMA gamma(GAMMA::DECODE,static_cast<unsigned long>(value));
            v->SetGamma(&gamma,static_cast<unsigned int>(type-18));
            break;
        }

        case 26: {
            VID* ammoVid=(v->m_linkVid && v->m_linkVid->CanFight()) ? v->m_linkVid : v;
            ammoVid->m_weapon->m_maxAmmo=value;
            break;
        }

        case 29:
            for(int i=0;i<4;++i)v->m_killed[i]=value;
            break;
        case 30: case 31: case 32: case 33:
            v->m_killed[type-30]=value;
            break;

        case 38: case 39: case 40: case 41:
            v->SetMaxHp(type-38,value);
            break;
        case 42: case 43: case 44: case 45:
            v->SetHpCoeff(type-42,value);
            break;

        case 48:
            v->m_defaultMaxSpeed=(value==999999)?999999.0f:static_cast<float>(value)/1000.0f;
            int iterator;
            for (SPRITE* sprite=FirstSprite(v->m_layer,&iterator);
                 sprite;
                 sprite=NextSprite(v->m_layer,&iterator)) {
                if (sprite->Vid()==v && sprite->ExData())
                    sprite->ExData()->maxSpeed=v->m_defaultMaxSpeed;
            }
            break;

        case 49:
            if(v->m_idx){v->m_defaultDeathTimer=static_cast<unsigned int>(value);v->m_exSpriteData=1;}
            break;
        case 50:
            v->m_weapon->m_detectRange=static_cast<float>(value);
            break;
        case 51:
            v->m_weapon->m_targetRadius=static_cast<float>(value);
            break;
        case 52:
            if(ValidateVid(value))
                ExchangeVid(v,Vid(value));
            else {
                char errorText[]="SetVid get_image";
                Error(4,errorText,static_cast<unsigned long>(value));
            }
            break;
        case 54:
            v->m_unknown18=value;
            break;
        case 55:
            v->m_weapon->m_buildTime=value;
            break;
        case 56:
            v->SetPropHide(value);
            break;
        case 57:
            v->SetPropNotCreateAsChild(value);
            break;
        case 58:
            v->m_phaseRandomInterval=static_cast<uint16_t>(value);
            for(int i=0;i<17;++i)v->m_aniFrameSpeed[i]=value;
            break;
        case 59:
            v->m_linkVid=Vid(value);
            break;
        case 124:
            v->m_fireDamage=value;
            break;

        default:
            if(type>=60 && type<77)
                SetVidChild(v,type-60,value);
            else if(type>=92 && type<109)
                v->m_aniFireCount[type-92]=value;
            else {
                char errorText[]="SetVid type";
                Error(14,errorText,static_cast<unsigned long>(type));
            }
            break;
        }
        return 0;
    }
    case 164: { STRING s=Int2Str(PopInt()); PushStr(&s); return 0; }
    case 165: { ANGLE a((uint8_t)PopInt()); PushInt((int)(a.Sin()*1024.0f)); return 0; }
    case 166: { ANGLE a((uint8_t)PopInt()); PushInt((int)(a.Cos()*1024.0f)); return 0; }
    case 167: PushInt((int)m_w); return 0;
    case 168: PushInt((int)m_h); return 0;
    case 169: { VID* v=PopVid("for Genocide"); if(v!=EmptyVid){int it=m_layers[v->m_layer].m_no;for(SPRITE* s=NextSprite(v->m_layer,&it);s;){SPRITE* n=NextSprite(v->m_layer,&it);if(s->m_vid==v)s->ScalarDeletingDestructor(1);s=n;}} return 0; }
    case 170: { VID* nv=PopVid("for Replace Unit 2"); VID* ov=PopVid("for Replace Unit 1"); if(nv!=EmptyVid&&ov!=EmptyVid){int it=m_layers[ov->m_layer].m_no;for(SPRITE* s=NextSprite(ov->m_layer,&it);s;){SPRITE* n=NextSprite(ov->m_layer,&it);if(s->m_vid==ov){CreateSprite(nv,s->X(),s->Y(),s->Z(),s->Direction(),0);s->ScalarDeletingDestructor(1);}s=n;}} return 0; }
    case 171: {
        execText=PopStr();
        CRC32 crc(execText.CharPtr(),static_cast<unsigned int>(execText.Length()));
        PushInt(static_cast<int>(static_cast<unsigned int>(crc)));
        return 0;
    }
    case 172: {
        if(m_logic.IsLastStackString()) {
            STRING arg(PopStr());
            const STRING* format=PopStr();
            STRING out=Printf(const_cast<STRING*>(format)->CharPtr(),arg.CharPtr());
            PushStr(&out);
        } else {
            const int arg=PopInt();
            const STRING* format=PopStr();
            STRING out=Printf(const_cast<STRING*>(format)->CharPtr(),arg);
            PushStr(&out);
        }
        return 0;
    }
    case 173: ReloadVid(); return 0;
    case 174: {
        execText=PopStr();
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        if(!file) return 0;
        execText.Write(file);
        fseek(file,-1,1);
        fputs("\n",file);
        return 0;
    }
    case 175: {
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        if(m_flags&0x200u)
            execText.Read(&m_resource);
        else if(file)
            execText.Read(file);
        if(m_flags&0x100u)
            execText.Write(&m_resource);
        PushStr(&execText);
        return 0;
    }
    case 176: {
        execText=PopStr();
        FILE* file=(m_flags&0x200u) ? 0 : FOpen(&execText,"r+t");
        if(!file)
            Error(7,execText.CharPtr(),0);
        PushInt(reinterpret_cast<int>(file));
        return 0;
    }
    case 177: {
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        if(file) fclose(file);
        return 0;
    }
    case 178: {
        execText=PopStr();
        if(m_flags&0x200u)
            PushInt(0);
        else
            PushInt(reinterpret_cast<int>(FOpen(&execText,"w+t")));
        return 0;
    }
    case 179: {
        FILE* file=reinterpret_cast<FILE*>(PopInt());
        PushInt(file ? (feof(file) ? 0x10 : 0) : 1);
        return 0;
    }
    case 182: { STRING def=*PopStr(),name=*PopStr(),path=*PopStr(); REGISTRY r(path); STRING out=r.GetString(name,def); PushStr(&out); return 0; }
    case 183: { STRING value=*PopStr(),name=*PopStr(),path=*PopStr(); REGISTRY r(path); r.SetString(name,value); return 0; }
    case 184: { STRING name=*PopStr(),path=*PopStr(); REGISTRY r(path); r.Delete(name); return 0; }
    case 185: PushStr(Registry->Path()); return 0;
    case 206: { STRING o=const_cast<STRING*>(PopStr())->ToLower(); PushStr(&o); return 0; }
    case 207: { STRING o=const_cast<STRING*>(PopStr())->ToUpper(); PushStr(&o); return 0; }
    case 208: { int key=PopInt(); STRING o=const_cast<STRING*>(PopStr())->ToBase64(key); PushStr(&o); return 0; }
    case 212: {
        int query=PopInt(); SPRITE* s=PopSprite(); if(!s||!s->IsSpriteClass(0x15)){PushInt(0);return 0;} TRAIN_INFO info(static_cast<ENGINE*>(s));
        switch(query){case 1:PushInt(info.speed);break;case 2:PushInt(info.weapon);break;case 3:PushInt(info.max_hp?info.hp*100/info.max_hp:0);break;case 4:PushInt(info.hp);break;case 5:PushInt(info.percentAmmo);break;case 6:PushInt(info.Acceleration());break;case 7:PushInt(info.build_time);break;case 10:PushInt(info.ammo);break;case 11:PushInt(info.maxAmmo);break;default:PushInt(0);break;} return 0;
    }
    case 213: {
        int count=0;
        int monstersVid[0x1000];
        for (int i=0;i<0x1000;++i)
            monstersVid[i]=0;

        for (int i=0;i<0x1000;++i) {
            STRING variableName="MonstersVid["+Int2Str(i)+"]";
            STRING value=ScriptVariable(variableName);
            if (value==STRING::EMPTY)
                break;
            monstersVid[value.Int()]=1;
        }

        for (int layer=0;layer<17;++layer) {
            int iterator;
            for (SPRITE* sprite=FirstSprite(layer,&iterator);sprite;sprite=NextSprite(layer,&iterator)) {
                VID* vid=sprite->Vid();
                if (monstersVid[vid->m_idx]) {
                    ++count;
                    VID* child=vid->m_aniChildVid[15];
                    if (child) {
                        if (monstersVid[vid->m_aniSpawnMode[15]])
                            ++count;
                        if (monstersVid[child->m_aniSpawnMode[15]])
                            ++count;
                    }
                }

                if (sprite->IsActionStackEmpty())
                    continue;

                LIST<ACT>* actions=sprite->ActionStack();
                for (int actionIndex=actions->No()-1;actionIndex>=0;--actionIndex) {
                    ACT action((*actions)[actionIndex]);
                    if (action.act==0x49)
                        break;
                    if (action.act!=0x23 || action.var1<=0 || !monstersVid[action.var1])
                        continue;

                    ++count;
                    VID* actionVid=Vid(action.var1);
                    VID* child=actionVid->m_aniChildVid[15];
                    if (child) {
                        if (monstersVid[actionVid->m_aniSpawnMode[15]])
                            ++count;
                        if (monstersVid[child->m_aniSpawnMode[15]])
                            ++count;
                    }
                }
            }
        }
        PushInt(count);
        return 0;
    }
    case 231: { int d=PopInt(),value=PopInt(),y=PopInt(),x=PopInt(); RailMap.SetSemaphoreOrMine(x,y,value,d); return 0; }
    case 239: { int y=PopInt(),x=PopInt(); SPRITE* s=PopSprite(); if(s&&s->IsSpriteClass(0x15))static_cast<ENGINE*>(s)->BreakTrain((float)x,(float)y); return 0; }
    case 240: PushSprite(FirstTrain(PopInt())); return 0;
    case 241: PushSprite(NextTrain()); return 0;
    case 242: { int param=PopInt(),cmd=PopInt(); SPRITE* s=PopSprite(); if(s&&s->IsSpriteClass(0x15))static_cast<ENGINE*>(s)->SetCommandToTrain(25,cmd,param); return 0; }
    case 243: { int value=PopInt(),y2=PopInt(),x2=PopInt(),y1=PopInt(),x1=PopInt(); RailMap.SetPushLine(x1,y1,x2,y2,value); return 0; }
    case 244: PushInt((int)m_input.mouseX); return 0;
    case 245: PushInt((int)m_input.mouseY); return 0;
    case 246: static_cast<PLAYER_STEAM*>(Player(1))->SetCleverEnemyAttack(PopInt()); return 0;
    case 247: PopInt(); return 0;
    case 249: { int index=PopInt(); VID* v=PopVid("for AddUnitLimit"); int limit=PopInt(); if(v!=EmptyVid){if(index==0xFF)v->m_limit394=limit;else v->m_limit398[index]=limit;} return 0; }
    case 250: { int on=PopInt();m_flags=(m_flags&~2u)|(on?2u:0u);return 0; }
    case 251: { int money=PopInt();Player(PopInt())->SetMoney(money);return 0; }
    case 252: PushInt(Player(PopInt())->GetMoney());return 0;
    case 253: PopInt();PopInt();PopSprite();return 0;
    case 254: PopSprite();PopSprite();return 0;
    default: MYERROR::Log(::Error,"!!!ERROR!!!LOGIC: Unknown extern Function %i",command); return 0;
    }
}
