#include "mapedit/runtime.hpp"

// Retail global dynamic initializer allocates the permanent fallback VID before
// WinMain and stores it in EmptyVid (global 0x004EE688).  The pre-A12
// implementation incorrectly left EmptyVid zero-initialized, so MAP::MAP crashed
// immediately after LoadVid at source line 186 when it dereferenced EmptyVid.
VID* EmptyVid = new VID;

namespace {
inline unsigned char* VidRaw(VID* vid)
{
    return reinterpret_cast<unsigned char*>(vid);
}

// VC6 helper at 0x0049909C converts the x87 value to a signed 64-bit
// integer with truncation toward zero; callers in the VID loaders consume
// the low 32 bits (and, for tile extents, only AX).  Keep that observable
// behavior without embedding compiler-specific assembly in the derived
// source.  x87 FISTP returns the integer-indefinite value on NaN/overflow,
// whose low DWORD is zero.
inline int RetailFtolLow32(float value)
{
    const double d=static_cast<double>(value);
    if (!(d>=-9223372036854775808.0 && d<9223372036854775808.0))
        return 0;
    const __int64 converted=static_cast<__int64>(d);
    return static_cast<int>(static_cast<unsigned int>(converted));
}

}

// The retail object is genuinely polymorphic.  MSVC supplies the vfptr at +0;
// every explicit data write below is taken from the original constructor.
VID::VID()
{
    // STRING/GAMMA members are now represented by their genuine C++ types.
    // Their automatic member construction reproduces the retail ctor calls at
    // +0x008, +0x2D8, +0x2EC and the GAMMA[4] vector at +0x3E8.

    m_spriteClass=6;
    m_unknown0C=0;
    m_flag=0;
    m_footprintWidth=24.0f;
    m_footprintHeight=16.0f;
    m_hitVerticalOffset=20.0f;
    m_snapOffsetX=m_footprintWidth/2.0f;
    m_snapOffsetY=m_footprintHeight/2.0f;

    m_colorScaleR=1.0f;
    m_colorScaleG=1.0f;
    m_colorScaleB=1.0f;

    m_mirrorNext=this;
    m_extraTypeFlags=0;
    m_exchangeVid=this;
    m_layer=16;
    m_idx=-1;
    m_baseHp=0;
    m_noDirections=1;
    m_editorDirectionOffset=0;
    m_phaseRandomInterval=71;
    m_linkVidIndex=0;
    m_linkVid=0;
    m_weaponIndex=0;
    m_weapon=0;
    m_nLinkDots=0;
    m_linkDots=0;
    m_dotFrameStarts=0;
    m_exSpriteData=0;
    m_moveTactData=0;
    m_prop=0;

    // Retail clears the seven one-bit properties individually, preserving the
    // rest of the bitfield storage exactly as VC6 does for member bitfields.
    m_propertyBits&=~0x7Fu;

    for (int ani=0;ani<17;++ani) {
        m_aniSfx[ani]=0;
        m_aniSpawnMode[ani]=0;
        m_aniChildVid[ani]=0;
        m_noAnimCadr[ani]=0;
        m_aniFrameStart[ani]=0;
        m_aniFrameLimit[ani]=0;
        m_aniFrameSpeed[ani]=71;
    }

    ResetSprites();
}

VID::~VID()
{
    const int noSprites=NoSprites();
    if (noSprites)
        Error(10,const_cast<char*>("Not all sprites with this VID deleted"),static_cast<unsigned long>(NoSprites()));

    if (m_mirrorNext!=this) {
        VID* previous=m_mirrorNext;
        while (previous->m_mirrorNext!=this)
            previous=previous->m_mirrorNext;
        previous->m_mirrorNext=m_mirrorNext;
    }

    if (m_linkDots) {
        ::operator delete(m_linkDots);
        m_linkDots=0;
    }
    if (m_dotFrameStarts) {
        ::operator delete(m_dotFrameStarts);
        m_dotFrameStarts=0;
    }

    // m_resourceName and m_name are destroyed automatically in retail order.
}

// Serialized VID parameter block.  The portable engine is used only for local
// names; field order, conversions, editor branch and BuildSizeToGridZ dot-grid
// are taken from the MapEdit executable above.
void VID::LoadParameters(RESOURCE* res)
{
    unsigned char* const raw=VidRaw(this);
    const auto read4=[&](unsigned int offset) {
        res->Read(raw+offset,4u);
    };

    read4(0x0C); read4(0x10); read4(0x14); read4(0x18);
    read4(0x1C); read4(0x20); read4(0x24); read4(0x28);
    read4(0x2C); read4(0x30); read4(0x34); read4(0x38);
    read4(0x3C); read4(0x40); read4(0x44); read4(0x48);
    read4(0x4C); read4(0x50); read4(0x54); read4(0x58);
    read4(0x60); read4(0x64); read4(0x68); read4(0x6C);

    // The resource contains four legacy dwords which have no runtime fields.
    res->Shift(16);
    res->Read(&m_noDirections,4u);
    res->Read(m_noAnimCadr,0x44u);
    res->Read(m_aniSfx,0x44u);
    res->Read(m_aniFrameSpeed,0x44u);
    res->Read(m_aniSpawnX,0x44u);
    res->Read(m_aniSpawnY,0x44u);
    res->Read(m_aniSpawnZ,0x44u);
    res->Read(m_aniSpawnMode,0x44u);
    res->Read(m_aniFireCount,0x44u);

    int red=0,green=0,blue=0,alpha=0;
    res->Read(&red,4u);
    res->Read(&green,4u);
    res->Read(&blue,4u);
    res->Read(&alpha,4u);
    GAMMA loadedGamma(alpha,red,green,blue);
    m_gamma.operator=(&loadedGamma);

    res->Read(&m_colorScaleR,4u);
    res->Read(&m_colorScaleG,4u);
    res->Read(&m_colorScaleB,4u);
    if (IsZBufferType() && IsHardwareType()) {
        m_colorScaleR=1.0f;
        m_colorScaleG=1.0f;
        m_colorScaleB=1.0f;
    }

    const float sentinel=999999.0f;
    if (m_childDirectionLock==sentinel)
        m_childDirectionLock=0.0f;
    else if (m_childDirectionLock==0.0f)
        m_childDirectionLock=sentinel;
    else
        m_childDirectionLock=256.0f/m_childDirectionLock;

    if (m_defaultMaxSpeed!=sentinel) m_defaultMaxSpeed/=1000.0f;
    if (m_maxZSpeed!=sentinel) m_maxZSpeed/=1000.0f;
    if (m_acceleration!=sentinel) m_acceleration/=1000000.0f;
    if (m_deceleration!=sentinel) m_deceleration/=1000000.0f;

    // Original stores this derived movement gate at +0x474.
    m_moveTactData=(m_defaultMaxSpeed!=0.0f || m_maxZSpeed!=0.0f ||
                    PropGravity() || PropGravity2() || PropWind()) ? 1 : 0;

    if (!m_noDirections) {
        Error(4,const_cast<char*>("NoDir==0"),0);
        exit(1);
    }
    for (int i=0;i<17;++i) {
        if (!m_aniFrameSpeed[i])
            m_aniFrameSpeed[i]=static_cast<unsigned int>(m_phaseRandomInterval);
    }

    m_editorDirectionOffset=128/static_cast<int>(m_noDirections);
    m_snapOffsetX=m_footprintWidth/2.0f;
    m_snapOffsetY=m_footprintHeight/2.0f;

    if (!m_dotFrameCount) {
        Error(4,const_cast<char*>("noCadr==0"),0);
    } else if (m_dotFrameCount<static_cast<int>(m_noDirections)) {
        Error(4,const_cast<char*>("noCadr < noDir"),0);
        m_noDirections=static_cast<unsigned int>(m_dotFrameCount);
    }

    int total=0;
    for (int i=0;i<17;++i)
        total+=m_noAnimCadr[i]*static_cast<int>(m_noDirections);
    if (total>m_dotFrameCount) {
        Error(13,const_cast<char*>("noCadr for noAnimCadr and noDir"),0);
        for (int i=16;i>=0;--i) {
            const int block=m_noAnimCadr[i]*static_cast<int>(m_noDirections);
            if (total-block<=m_dotFrameCount) {
                m_noAnimCadr[i]-=(total-m_dotFrameCount)/static_cast<int>(m_noDirections);
                break;
            }
            total-=block;
            m_noAnimCadr[i]=0;
        }
    }

    int start=0;
    int firstAnimation=-1;
    for (int i=0;i<17;++i) {
        if (!Sound->ValidateSFX(m_aniSfx[i]) && m_idx!=-1) {
            Error(4,const_cast<char*>("sfx"),static_cast<unsigned long>(m_aniSfx[i]));
            m_aniSfx[i]=0;
        }

        if (m_noAnimCadr[i]) {
            m_aniFrameStart[i]=start;
            m_aniFrameLimit[i]=m_noAnimCadr[i];
            if (firstAnimation<0) {
                firstAnimation=i;
                for (int j=0;j<i;++j) {
                    if (!m_aniFrameLimit[j])
                        m_aniFrameLimit[j]=m_aniFrameLimit[i];
                }
            }
        } else if (m_spriteClass==10 && (i&1) && i<=7 &&
                   m_noAnimCadr[firstAnimation+1]) {
            m_aniFrameStart[i]=m_aniFrameStart[firstAnimation+1];
            m_aniFrameLimit[i]=m_aniFrameLimit[firstAnimation+1];
        } else {
            m_aniFrameStart[i]=0;
            m_aniFrameLimit[i]=m_aniFrameLimit[firstAnimation];
        }

        start+=m_noAnimCadr[i]*static_cast<int>(m_noDirections);
        if (start>m_dotFrameCount) {
            Error(10,const_cast<char*>("noCadr and noAnimCadr and noDir"),static_cast<unsigned long>(i));
            m_aniFrameStart[i]=0;
            m_aniFrameLimit[i]=m_aniFrameLimit[firstAnimation];
        }
    }

    if (m_spriteClass==8)
        m_propertyBits|=0x20u;
    if (m_spriteClass==8 && Map->IsMapEdit())
        m_spriteClass=0;

    if (!PropBuildSizeToGridZ() || m_nLinkDots)
        return;

    // Retail allocation uses width in both factors. Preserve that historical
    // expression rather than silently correcting it to width*height.
    const int roundedWidth=(static_cast<int>(m_footprintWidth)+17)/8;
    const int tempCount=((static_cast<int>(m_footprintWidth)+17)*roundedWidth)/8+1;
    VID_DOT* temp=static_cast<VID_DOT*>(::operator new(static_cast<unsigned int>(tempCount*12)));
    m_nLinkDots=0;

    for (int yi=0;static_cast<float>(yi)<m_footprintHeight;yi+=8) {
        for (int xi=0;static_cast<float>(xi)<m_footprintWidth;xi+=8) {
            temp[m_nLinkDots].x=static_cast<float>(xi)-m_footprintWidth/2.0f;
            temp[m_nLinkDots].y=static_cast<float>(yi)-m_footprintHeight/2.0f;
            temp[m_nLinkDots].z=m_hitVerticalOffset;
            ++m_nLinkDots;
        }
        temp[m_nLinkDots].x=m_footprintWidth/2.0f;
        temp[m_nLinkDots].y=static_cast<float>(yi)-m_footprintHeight/2.0f;
        temp[m_nLinkDots].z=m_hitVerticalOffset;
        ++m_nLinkDots;
    }
    for (int xi=0;static_cast<float>(xi)<m_footprintWidth;xi+=8) {
        temp[m_nLinkDots].x=static_cast<float>(xi)-m_footprintWidth/2.0f;
        temp[m_nLinkDots].y=m_footprintHeight/2.0f;
        temp[m_nLinkDots].z=m_hitVerticalOffset;
        ++m_nLinkDots;
    }
    temp[m_nLinkDots].x=m_footprintWidth/2.0f;
    temp[m_nLinkDots].y=m_footprintHeight/2.0f;
    temp[m_nLinkDots].z=m_hitVerticalOffset;
    ++m_nLinkDots;

    if (m_linkDots)
        ::operator delete(m_linkDots);
    m_linkDots=static_cast<VID_DOT*>(::operator new(static_cast<unsigned int>(m_nLinkDots*12)));
    for (int i=0;i<m_nLinkDots;++i)
        m_linkDots[i]=temp[i];
    ::operator delete(temp);
}


VID* VID::CreateMirror()
{
    return new VID();
}

void VID::DrawVidToVid(const SPRITE*)
{
}

void VID::Draw(const SPRITE*)
{
}

void VID::DrawShadow(const SPRITE*)
{
}

void VID::DrawToVid(const SPRITE*,const VID_TEXCOOR*,TEXTURE*,TEXTURE*)
{
}

void VID::Load(RESOURCE*)
{
}

int VID::HaveShadow()
{
    return 0;
}

void VID::SetLayer()
{
    m_layer=0;
}

void VID::Error(int type,char* text,unsigned long err)
{
    MYERROR::Error(::Error,"VID [%i-%s]",type,text,err,m_idx,m_name.m_buf);
}

void VID::SetGamma(const GAMMA* gamma,unsigned int n_gamma)
{
    if (n_gamma<4u) {
        m_gammaByArmy[n_gamma]=gamma;
    } else if (n_gamma!=4u) {
        Error(4,const_cast<char*>("n_gamma in VID::SetGamma"),n_gamma);
    }
}


namespace {
struct VID_FIGHT_LAYOUT {
    uint8_t pad000[0x40];
    void* field040;
    uint8_t pad044[0x270 - 0x44];
    void* field270;
};
static_assert(offsetof(VID_FIGHT_LAYOUT, field040) == 0x40, "VID +0x40 ABI");
static_assert(offsetof(VID_FIGHT_LAYOUT, field270) == 0x270, "VID +0x270 ABI");
}

int VID::IsSpriteType(unsigned int type) const
{
    return m_unknown0C & type;
}

void VID::SetExtraType()
{
    m_extraTypeFlags|=0x0200u;
}

int VID::IsAlphaType()
{
    return m_extraTypeFlags&0x0002u;
}

int VID::IsZBufferType()
{
    return m_extraTypeFlags&0x0004u;
}

int VID::IsHardwareType()
{
    return m_extraTypeFlags&0x0020u;
}

int VID::IsLightType()
{
    return m_extraTypeFlags&0x0080u;
}

int VID::IsPseudo3DType()
{
    return m_extraTypeFlags&0x1000u;
}

int VID::PropGamma()
{
    return m_flag&0x00000800u;
}

int VID::IsFontType()
{
    return m_extraTypeFlags&0x4000u;
}

int VID::PropBlur()
{
    return m_flag&0x00200000u;
}

int VID::IsTextureType()
{
    return m_extraTypeFlags&0x0001u;
}

int VID::IsPaletteType()
{
    return m_extraTypeFlags&0x0008u;
}

int VID::IsNewVersionType()
{
    return m_extraTypeFlags&0x0010u;
}

int VID::IsCompressType()
{
    return m_extraTypeFlags&0x0100u;
}

int VID::PropGround()
{
    return m_flag&0x40000000u;
}

int VID::IsAltGammaType()
{
    return m_extraTypeFlags&0x0400u;
}

int VID::PropWave()
{
    return m_flag&0x00010000u;
}

int VID::IsDXTType()
{
    return m_extraTypeFlags&0x0800u;
}

int VID::PropHardwareDirect()
{
    return m_flag&0x20000000u;
}

int VID::PropHash()
{
    return m_flag & 0x40u;
}

int VID::PropBuildVidZToGridZ()
{
    return m_flag & 0x20u;
}

int VID::PropBuildSizeToGridZ()
{
    return m_flag & 0x08u;
}

int VID::CanFight() const
{
    const VID_FIGHT_LAYOUT* fields = reinterpret_cast<const VID_FIGHT_LAYOUT*>(this);
    return fields->field270 != 0 && fields->field040 != 0;
}

int VID::GetMaxAmmo()
{
    if (m_linkVid && m_linkVid->CanFight())
        return m_linkVid->m_weapon->m_maxAmmo;
    return m_weapon->m_maxAmmo;
}

int VID::GetFireDamage()
{
    int damage=m_fireDamage;
    if (m_linkVid)
        damage+=m_linkVid->GetFireDamage();
    if (m_aniChildVid[15])
        damage+=m_aniChildVid[15]->GetFireDamage()*m_aniFireCount[15];
    if (m_aniChildVid[14])
        damage+=m_aniChildVid[14]->GetFireDamage()*m_aniFireCount[14];
    if (m_aniChildVid[8])
        damage+=m_aniChildVid[8]->GetFireDamage()*m_aniFireCount[8];
    return damage;
}

int VID::GetBuildTime()
{
    // Retail tests the +0x40 owner on LinkVid before selecting its weapon.
    const VID_FIGHT_LAYOUT* const linkFields =
        m_linkVid ? reinterpret_cast<const VID_FIGHT_LAYOUT*>(m_linkVid) : 0;
    if (m_linkVid && linkFields->field040)
        return m_linkVid->m_weapon->m_buildTime / 1000;
    return m_weapon->m_buildTime / 1000;
}

int VID::NoSprites()
{
    return m_entitiesNumber[0] + m_entitiesNumber[1] +
           m_entitiesNumber[2] + m_entitiesNumber[3];
}

int VID::PropHide()
{
    return (m_propertyBits >> 6) & 1u;
}

void VID::SetPropHide(int flag)
{
    m_propertyBits = (m_propertyBits & ~0x40u) | (flag ? 0x40u : 0u);
    if (m_linkVid)
        m_linkVid->SetPropHide(flag);
}

int VID::IsEmptyType()
{
    return m_extraTypeFlags == 0;
}

int VID::PropSkipMapEd()
{
    return m_flag & 0x2000u;
}

// at 0x004BD304 is "%04i %s".
STRING VID::GetNumberName()
{
    char buffer[1024];
    sprintf(buffer, "%04i %s", m_idx, m_name.m_buf);
    return STRING(buffer);
}

namespace {
int g_vidViewXMinRetail=0;
int g_vidViewXMaxRetail=0;
int g_vidViewYMinRetail=0;
int g_vidViewYMaxRetail=0;
}

int VID::BoxInViewPort(int left,int top,int right,int bottom)
{
    return right>=g_vidViewXMinRetail && left<g_vidViewXMaxRetail &&
           bottom>=g_vidViewYMinRetail && top<g_vidViewYMaxRetail;
}

void VID::SetViewPort(int x0,int y0,int x1,int y1)
{
    g_vidViewXMinRetail=x0;
    g_vidViewXMaxRetail=x1;
    g_vidViewYMinRetail=y0;
    g_vidViewYMaxRetail=y1;
}

void VID::SetGridZ(const SPRITE* sprite)
{
    if (sprite == Mouse)
        return;

    SPRITE* current=const_cast<SPRITE*>(sprite);
    int begin=0;
    int end=m_nLinkDots;
    if (m_dotFrameStarts) {
        const int frame=current->CurrentCadr();
        begin=(frame < m_dotFrameCount) ? m_dotFrameStarts[frame] : 0;
        end=(frame < m_dotFrameCount-1) ? m_dotFrameStarts[frame+1] : m_nLinkDots;
    }

    const int useGround=!Map->IsMapEdit() && current->IsSpriteClass(8);
    for (int i=begin;i<end;++i) {
        const float x=current->X()+m_linkDots[i].x;
        const float y=current->Y()+m_linkDots[i].y;
        const float z=current->Z()+m_linkDots[i].z;
        if (useGround)
            Map->SetGroundZ(x,y,z);
        else
            Map->SetTempGroundZ(x,y,z);
    }
}

void VID::ResetGridZ(const SPRITE* sprite)
{
    if (sprite == Mouse)
        return;

    SPRITE* current=const_cast<SPRITE*>(sprite);
    if (!Map->IsMapEdit() && current->IsSpriteClass(8))
        return;

    int begin=0;
    int end=m_nLinkDots;
    if (m_dotFrameStarts) {
        const int frame=current->CurrentCadr();
        begin=(frame < m_dotFrameCount) ? m_dotFrameStarts[frame] : 0;
        end=(frame < m_dotFrameCount-1) ? m_dotFrameStarts[frame+1] : m_nLinkDots;
    }

    for (int i=begin;i<end;++i)
        Map->ClearTempGroundZ(
            current->X()+m_linkDots[i].x,
            current->Y()+m_linkDots[i].y,
            current->Z()+m_linkDots[i].z);
}

int VID::PropWind()
{
    return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(this)+0x14) & 0x1000u;
}

int VID::PropVertDir()
{
    return *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(this)+0x14) & 0x80000u;
}

int VID::PropNotChangeLinkerCoor()
{
    return static_cast<int>(m_flag & 0x02000000u);
}

ANGLE VID::SteppedDirection(ANGLE direction)
{
    if (m_noDirections != 0)
        return ANGLE(static_cast<uint8_t>((RealDirection(direction) << 8) / static_cast<int>(m_noDirections)));
    return direction;
}

int VID::NoSprites(int army)
{
    return m_entitiesNumber[army];
}

void VID::ResetSprites()
{
    uint8_t* raw=reinterpret_cast<uint8_t*>(this);
    int* const frameSlots=reinterpret_cast<int*>(raw+0x408);
    for (unsigned int i=0;i<20u;++i)
        frameSlots[i]=-1;

    *reinterpret_cast<int*>(raw+0x3D4)=0;
    *reinterpret_cast<int*>(raw+0x3D0)=0;
    *reinterpret_cast<int*>(raw+0x3CC)=0;
    *reinterpret_cast<int*>(raw+0x3C8)=0;
    *reinterpret_cast<int*>(raw+0x3C4)=0;
    *reinterpret_cast<int*>(raw+0x3C0)=0;
    *reinterpret_cast<int*>(raw+0x3BC)=0;
    *reinterpret_cast<int*>(raw+0x3B8)=0;

    const int defaultValue=*reinterpret_cast<int*>(raw+0x28);
    *reinterpret_cast<int*>(raw+0x3E4)=defaultValue;
    *reinterpret_cast<int*>(raw+0x3E0)=defaultValue;
    *reinterpret_cast<int*>(raw+0x3DC)=defaultValue;
    *reinterpret_cast<int*>(raw+0x3D8)=defaultValue;

    *reinterpret_cast<int*>(raw+0x3B4)=0;
    *reinterpret_cast<int*>(raw+0x3B0)=0;
    *reinterpret_cast<int*>(raw+0x3AC)=0;
    *reinterpret_cast<int*>(raw+0x3A8)=0;

    *reinterpret_cast<int*>(raw+0x3A4)=-1;
    *reinterpret_cast<int*>(raw+0x3A0)=-1;
    *reinterpret_cast<int*>(raw+0x39C)=-1;
    *reinterpret_cast<int*>(raw+0x398)=-1;
    *reinterpret_cast<int*>(raw+0x394)=-1;
    *reinterpret_cast<int*>(raw+0x458)=0;
    *reinterpret_cast<uint32_t*>(raw+0x47C)&=~0x10u;
}

int VID::PropAlwaysTop()
{
    return static_cast<int>(m_flag & 0x8000u);
}


int VID::PropDblLight()
{
    return static_cast<int>(m_flag & 0x00800000u);
}

int VID::PropNoise()
{
    return static_cast<int>(m_flag & 0x00000100u);
}

int VID::PropOnePhase()
{
    return static_cast<int>(m_flag & 0x01000000u);
}

int VID::PropGravity() const
{
    return static_cast<int>(m_flag & 0x00000002u);
}

int VID::PropSelfMoving() const
{
    return static_cast<int>(m_flag & 0x08000000u);
}

int VID::PropCrush()
{
    return static_cast<int>(m_flag & 0x00004000u);
}

int VID::PropGravity2() const
{
    return static_cast<int>(m_flag & 0x00000004u);
}

int VID::PropMoveWithAnyDirection()
{
    return static_cast<int>(m_flag & 0x00100000u);
}

int VID::PropNotCreateAsChild()
{
    return static_cast<int>(m_prop);
}

int VID::PropBirthAsSmoke() const
{
    return static_cast<int>(m_flag & 0x00000080u);
}

int VID::PropTrack() const
{
    return static_cast<int>(m_flag & 0x00000010u);
}

int VID::PropChildInEnd()
{
    return static_cast<int>(m_flag & 0x00040000u);
}

int VID::PropRandBirth() const
{
    return static_cast<int>(m_flag & 0x00000001u);
}

int WEAPON::PropInTurn() const
{
    return static_cast<int>(m_property & 0x00000010u);
}

int WEAPON::PropSelfDirecting() const
{
    return static_cast<int>(m_property & 0x00000020u);
}

// Exact high-level arithmetic derived from reference behavior.  VC6/FPU lowering
// is intentionally left to the retail-compiler parity lane.
float VID::CalculateZSpeed(float delta_z,float size) const
{
    float speed;
    if (PropGravity()) {
        speed=size*Const->gravity/m_defaultMaxSpeed*0.5f +
              delta_z*m_defaultMaxSpeed/size;
        speed*=speed>0.0f ? 1.1f : 0.9f;
    }
    else if (PropGravity2()) {
        speed=size*Const->gravity2/m_defaultMaxSpeed*0.5f +
              delta_z*m_defaultMaxSpeed/size;
        speed*=speed>0.0f ? 1.1f : 0.9f;
    }
    else if (PropSelfMoving()) {
        speed=m_maxZSpeed;
    }
    else if (m_groundOffset==0.0f) {
        speed=delta_z*m_defaultMaxSpeed/size;
    }
    else {
        speed=0.0f;
    }

    if (speed>m_maxZSpeed)
        return m_maxZSpeed;
    if (speed<-m_maxZSpeed)
        return -m_maxZSpeed;
    return speed;
}

int WEAPON::PropFrontEye() const { return static_cast<int>(m_property & 0x2u); }
int WEAPON::PropRandomTarget() const { return static_cast<int>(m_property & 0x8u); }
int WEAPON::PropAttackAnyArmy() const { return static_cast<int>(m_property & 0x80u); }
int WEAPON::PropAttackNearOnly() const { return static_cast<int>(m_property & 0x100u); }
int WEAPON::PropAnyDirFire() const { return static_cast<int>(m_property & 0x1u); }

int VID::IsInvulnerable()
{
    return *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(this)+0x28)==0;
}

int VID::PropInvisibleForEnemy()
{
    return static_cast<int>(m_flag & 0x00020000u);
}

int VID::PropRadialDamage()
{
    return static_cast<int>(m_flag & 0x04000000u);
}

int VID::PropNotDamageForFriend()
{
    return static_cast<int>(m_flag & 0x80000000u);
}

int VID::PropRandSpeed()
{
    return static_cast<int>(m_flag & 0x400u);
}

int VID::PropRandZSpeed()
{
    return static_cast<int>(m_flag & 0x00400000u);
}

int VID::PropBounce()
{
    return static_cast<int>(m_flag & 0x10000000u);
}

// ---------------------------------------------------------------------------
// VID_HARDWARE — original MapEdit DX7 hardware-VID owner.
// Layout and all bodies in this block are derived directly from MapEdit.exe
// + debug metadata/debug metadata.  Load/Draw remain in the next renderer-body block; do not
// replace them with base fallbacks or a fabricated vtable.
// ---------------------------------------------------------------------------

namespace {
inline VID*& VidHardwareSharedOwner(VID* vid)
{
    // Retail VID+0x460 is the circular/shared renderer ownership link used by
    // VID_HARDWARE mirrors.  VID's own destructor also walks this field.
    return *reinterpret_cast<VID**>(reinterpret_cast<unsigned char*>(vid)+0x460);
}

}

void WordSet(void* dest,int cword,int noword)
{
    unsigned short* out=static_cast<unsigned short*>(dest);
    const unsigned short word=static_cast<unsigned short>(cword);
    while (noword-- > 0)
        *out++=word;
}

VID_HARDWARE::VID_HARDWARE()
    : VID(), m_texCoor(0), m_noSurf(0), m_textures(0)
{
}

// This is intentionally pointer-taking: debug metadata and the CreateMirror caller
// both prove that the original signature is VID_HARDWARE(VID_HARDWARE*).
VID_HARDWARE::VID_HARDWARE(VID_HARDWARE* source)
    : VID()
{
    VidHardwareSharedOwner(this)=VidHardwareSharedOwner(source);
    VidHardwareSharedOwner(source)=this;
    m_layer=source->m_layer;
    m_extraTypeFlags=source->m_extraTypeFlags;
    m_dotFrameCount=source->m_dotFrameCount;
    m_phaseRandomInterval=source->m_phaseRandomInterval;
    m_regionTileStepX=source->m_regionTileStepX;
    m_regionTileStepY=source->m_regionTileStepY;
    m_texCoor=source->m_texCoor;
    m_textures=source->m_textures;
    m_noSurf=source->m_noSurf;
}

VID_HARDWARE::VID_HARDWARE(int nvid,int size_x,int size_y)
    : VID()
{
    m_footprintWidth=256.0f;
    m_footprintHeight=256.0f;
    m_hitVerticalOffset=1.0f;
    m_regionTileStepX=static_cast<short>(size_x);
    m_regionTileStepY=static_cast<short>(size_y);
    m_noDirections=1;
    m_idx=nvid;
    m_layer=0;
    m_name=STRING("Self Created Hardware Prerendered Ground ");

    // Retail first writes 0x25 then calls VID::SetExtraType() (OR 0x0200).
    m_extraTypeFlags=0x25u;
    SetExtraType();

    const int count=(size_x/256+1)*(size_y/256+1)+1;
    m_texCoor=static_cast<VID_TEXCOOR*>(::operator new(static_cast<unsigned int>(count*0x24)));
    if (!m_texCoor) {
        Error(2,const_cast<char*>("texcoor"),static_cast<unsigned long>(count));
        exit(1);
    }

    m_noSurf=0;
    int index=0;
    for (int y=0;y<size_y;y+=256) {
        for (int x=0;x<size_x;x+=256) {
            if (index)
                m_texCoor[index-1].next_fragment=index;

            VID_TEXCOOR& part=m_texCoor[index];
            part.nsurf=static_cast<int>(m_noSurf);
            part.begx=0;
            part.begy=0;
            part.shiftx=x;
            part.shifty=y;
            const int remainX=size_x-x;
            const int remainY=size_y-y;
            part.sizex=(remainX>256)?256:remainX;
            part.sizey=(remainY>256)?256:remainY;
            part.next_fragment=0;
            m_noSurf=static_cast<short>(m_noSurf+2);
            ++index;
        }
    }

    m_textures=static_cast<TEXTURE**>(::operator new(static_cast<unsigned int>(m_noSurf)*sizeof(TEXTURE*)));
    if (!m_textures) {
        Error(2,const_cast<char*>("textures"),static_cast<unsigned long>(m_noSurf));
        return;
    }
    for (int i=0;i<m_noSurf;i+=2) {
        m_textures[i]=0;
        m_textures[i+1]=0;
    }
}

VID_HARDWARE::~VID_HARDWARE()
{
    if (VidHardwareSharedOwner(this)==this) {
        if (m_texCoor)
            ::operator delete(m_texCoor);
        m_texCoor=0;

        if (m_textures) {
            while (--m_noSurf>=0) {
                TEXTURE* texture=m_textures[m_noSurf];
                if (texture)
                    delete texture;
            }
            ::operator delete(m_textures);
            m_textures=0;
        }
        m_noSurf=0;
    }
}

VID* VID_HARDWARE::CreateMirror()
{
    return new VID_HARDWARE(this);
}

void VID_HARDWARE::DrawVidToVid(const SPRITE* sprite)
{
    if (!IsTextureType() || !IsZBufferType() || m_noDirections!=1)
        return;
    if (sprite->Vid()->PropInvisibleForEnemy())
        return;

    const int savedXMin=g_vidViewXMinRetail;
    const int savedXMax=g_vidViewXMaxRetail;
    const int savedYMin=g_vidViewYMinRetail;
    const int savedYMax=g_vidViewYMaxRetail;

    const int spriteX=static_cast<int>(sprite->X());
    const int spriteY=static_cast<int>(sprite->Y()-sprite->Z());

    VID_TEXCOOR* part=m_texCoor;
    while (part) {
        const int dx=abs(part->shiftx+part->begx+part->sizex/2-spriteX);
        const int spriteHalfX=static_cast<int>(sprite->Vid()->m_regionTileStepX)/2;
        if (dx < spriteHalfX + part->sizex/2) {
            const int dy=abs(part->shifty+part->begy+part->sizey/2-spriteY);
            const int spriteHalfY=static_cast<int>(sprite->Vid()->m_regionTileStepY)/2;
            if (dy < spriteHalfY + part->sizey/2) {
                g_vidViewXMinRetail=part->begx;
                g_vidViewXMaxRetail=part->begx+part->sizex;
                g_vidViewYMinRetail=part->begy;
                g_vidViewYMaxRetail=part->begy+part->sizey;

                const int surface=part->nsurf;
                if (!m_textures[surface]) {
                    m_textures[surface]=new TEXTURE(part->sizex,part->sizey,0x17,0);
                    int pitch=0;
                    unsigned char* bits=m_textures[surface]->Lock(&pitch,0);
                    memset(bits,0,static_cast<unsigned int>(pitch*m_textures[surface]->SizeY()));
                    m_textures[surface]->UnLock();

                    m_textures[surface+1]=new TEXTURE(part->sizex,part->sizey,0x50,2);
                    unsigned short* z= reinterpret_cast<unsigned short*>(m_textures[surface+1]->Lock(&pitch,0));
                    WordSet(z,0x400,(pitch/2)*m_textures[surface+1]->SizeY());
                    m_textures[surface+1]->UnLock();
                }

                sprite->Vid()->DrawToVid(sprite,part,m_textures[surface],m_textures[surface+1]);
            }
        }

        part=part->next_fragment ? m_texCoor+part->next_fragment : 0;
    }

    g_vidViewXMinRetail=savedXMin;
    g_vidViewXMaxRetail=savedXMax;
    g_vidViewYMinRetail=savedYMin;
    g_vidViewYMaxRetail=savedYMax;
}

// derived from the exact MapEdit DX7 owner; no portable UI-scaling path.
void VID_HARDWARE::Draw(const SPRITE* sprite)
{
    if (m_noSurf==0)
        return;

    const int frame=const_cast<SPRITE*>(sprite)->CurrentCadr();
    if (m_texCoor[frame].sizey==0 || PropHide())
        return;

    int x=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenX());
    int y=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenY());
    int z=static_cast<int>(sprite->Z());
    int noStep=1;

    if (PropZeroZ())
        z=3;

    if (PropWave()) {
        const ANGLE phase(static_cast<uint8_t>((CurrentTime>>3)&0xFFu));
        const float shiftZ=m_groundOffset*const_cast<ANGLE&>(phase).Sin();
        z+=static_cast<int>(shiftZ);
        y-=static_cast<int>(shiftZ);
    }

    const unsigned char* raw=reinterpret_cast<const unsigned char*>(this);
    float scaleX=*reinterpret_cast<const float*>(raw+0x2E0);
    float scaleY=*reinterpret_cast<const float*>(raw+0x2E4);

    if (m_propertyBits&2u) {
        const float coeff=const_cast<SPRITE*>(sprite)->ExData()->tableCoeff;
        unsigned char* weapon=reinterpret_cast<unsigned char*>(m_weapon);
        scaleX*=m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0xE4),coeff);
        scaleY*=m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x104),coeff);
    }

    if (m_propertyBits&4u) {
        const float coeff=const_cast<SPRITE*>(sprite)->ExData()->tableCoeff;
        unsigned char* weapon=reinterpret_cast<unsigned char*>(m_weapon);
        x+=static_cast<int>(m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x144),coeff));
        y+=static_cast<int>(m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x164),coeff));
        z+=static_cast<int>(m_weapon->Interpolate(reinterpret_cast<float*>(weapon+0x184),coeff));
    }

    ANGLE dir(static_cast<uint8_t>(0));
    if (PropHardwareDirect()) {
        if (PropVertDir()) {
            EX_SPRITE_DATA* ex=const_cast<SPRITE*>(sprite)->ExData();
            if (sprite->X()!=ex->lastX || sprite->Y()!=ex->lastY) {
                ANGLE actual(sprite->X()-ex->lastX,
                    (sprite->Y()-sprite->Z()-ex->lastY+ex->lastZ)/0.7070602178573608f);
                dir=&actual;
            } else if (sprite->Speed()!=0.0f && sprite->ZSpeed()!=0.0f) {
                ANGLE facing=const_cast<SPRITE*>(sprite)->Direction();
                const float vx=sprite->Speed()*facing.Sin();
                const float vy=-(sprite->ZSpeed()+sprite->Speed()*facing.Cos())/0.7070602178573608f;
                ANGLE actual(vx,vy);
                dir=&actual;
            } else {
                ANGLE facing=const_cast<SPRITE*>(sprite)->Direction();
                const int delta=facing.Int()-const_cast<SPRITE*>(sprite)->RealDirection()*256/static_cast<int>(m_noDirections);
                ANGLE actual(static_cast<uint8_t>(delta));
                dir=&actual;
            }
        } else {
            ANGLE facing=const_cast<SPRITE*>(sprite)->Direction();
            const int delta=facing.Int()-const_cast<SPRITE*>(sprite)->RealDirection()*256/static_cast<int>(m_noDirections);
            ANGLE actual(static_cast<uint8_t>(delta));
            dir=&actual;
        }
    }

    if (PropBlur() && m_texCoor[frame].sizex!=0 && m_texCoor[frame].sizey!=0) {
        EX_SPRITE_DATA* ex=const_cast<SPRITE*>(sprite)->ExData();
        int dx=static_cast<int>(ex->lastX-sprite->X());
        if (dx<0) dx=-dx;
        int dy=static_cast<int>(ex->lastY-ex->lastZ-sprite->Y()+sprite->Z());
        if (dy<0) dy=-dy;
        const int noStepX=dx/m_texCoor[frame].sizex;
        const int noStepY=dy/m_texCoor[frame].sizey;
        noStep+=(noStepX>noStepY ? noStepX*2 : noStepY*2);
    }

    struct RetailVertex {
        float x,y,z,rhw;
        unsigned long diffuse;
        unsigned long specular;
        float tu,tv;
    };
    static_assert(sizeof(RetailVertex)==0x20,"MapEdit hardware VID vertex ABI");

    for (int step=0;step<noStep;++step) {
        if (PropBlur()) {
            EX_SPRITE_DATA* ex=const_cast<SPRITE*>(sprite)->ExData();
            x=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenX()+
                (ex->lastX-sprite->X())*static_cast<float>(step)/static_cast<float>(noStep));
            y=static_cast<int>(const_cast<SPRITE*>(sprite)->ScreenY()+
                (ex->lastY-ex->lastZ-sprite->Y()+sprite->Z())*static_cast<float>(step)/static_cast<float>(noStep));
            z=static_cast<int>(sprite->Z()+
                (ex->lastZ-sprite->Z())*static_cast<float>(step)/static_cast<float>(noStep));
        }

        VID_TEXCOOR* coor=&m_texCoor[frame];
        while (coor) {
            const int halfStepX=static_cast<int>(m_regionTileStepX)/2;
            const int halfStepY=static_cast<int>(m_regionTileStepY)/2;
            int shiftX=x+static_cast<int>(static_cast<float>(coor->shiftx-halfStepX)*scaleX);
            int sizeX=coor->sizex;
            int shiftY=y+static_cast<int>(static_cast<float>(coor->shifty-halfStepY)*scaleY);
            int sizeY=coor->sizey;

            if (!BoxInViewPort(shiftX,shiftY,shiftX+sizeX,shiftY+sizeY)) {
                coor=coor->next_fragment ? &m_texCoor[coor->next_fragment] : 0;
                continue;
            }

            if (IsZBufferType()) {
                if (shiftX+sizeX>g_vidViewXMaxRetail)
                    sizeX=g_vidViewXMaxRetail-shiftX;
                if (shiftY+sizeY>g_vidViewYMaxRetail)
                    sizeY=g_vidViewYMaxRetail-shiftY;
            }

            RECT_OLD screenRect;
            screenRect.left=shiftX;
            screenRect.top=shiftY;
            screenRect.right=shiftX+static_cast<int>(static_cast<float>(sizeX)*scaleX);
            screenRect.bottom=shiftY+static_cast<int>(static_cast<float>(sizeY)*scaleY);

            RECT_OLD textureRect;
            textureRect.left=coor->begx;
            textureRect.top=coor->begy;
            textureRect.right=coor->begx+sizeX;
            textureRect.bottom=coor->begy+sizeY;

            if (IsZBufferType()) {
                const long err=Graph->CopyToZBuffer(&screenRect,&textureRect,m_textures[coor->nsurf+1]);
                if (err)
                    Error(1,const_cast<char*>("zbuffer"),static_cast<unsigned long>(err));
            }

            float z1=static_cast<float>(z)*0.0001220703125f+0.015625f;
            if (PropWave() && m_groundOffset!=0.0f) {
                ANGLE phase(static_cast<uint8_t>((CurrentTime>>3)&0xFFu));
                z1+=m_groundOffset*0.0001220703125f*phase.Sin()/2.0f;
            }

            float z2;
            if (PropAlwaysTop() || IsZBufferType()) {
                z2=z1=0.99999988f;
                Graph->SetRenderState(0x17u,8u);
            } else if (m_hitVerticalOffset>m_footprintHeight) {
                const float d=static_cast<float>(sizeY)*0.0001220703125f;
                z2=z1-d;
                z1+=d;
                Graph->SetRenderState(0x17u,7u);
            } else {
                z2=z1;
                Graph->SetRenderState(0x17u,7u);
            }
            (void)z2;

            if (IsAlphaType()) {
                if (IsTextureType()) Graph->SetAlphaBlend(5u,6u);
                else Graph->SetAlphaBlend(9u,2u);
            } else {
                Graph->SetRenderState(0x1Bu,0u);
            }

            GAMMA spriteGamma=const_cast<SPRITE*>(sprite)->GetGamma();
            GAMMA* baseGamma=reinterpret_cast<GAMMA*>(reinterpret_cast<unsigned char*>(this)+0x2D8);
            GAMMA drawGamma=(*baseGamma)+&spriteGamma;
            if (!PropGamma()) {
                GAMMA graphGamma=Graph->GetGamma();
                drawGamma+=&graphGamma;
            }

            const float w=static_cast<float>(coor->sizex);
            const float h=static_cast<float>(coor->sizey);
            const float textureSizeX=static_cast<float>(m_textures[coor->nsurf]->SizeX());
            const float textureSizeY=static_cast<float>(m_textures[coor->nsurf]->SizeY());
            const float xx[4]={0.0f,w,0.0f,w};
            const float yy[4]={0.0f,0.0f,h,h};
            RetailVertex vertices[4];

            for (int i=0;i<4;++i) {
                vertices[i].x=static_cast<float>(coor->shiftx-halfStepX)+xx[i];
                vertices[i].y=static_cast<float>(coor->shifty-halfStepY)+yy[i];
                vertices[i].z=0.0f;
                vertices[i].tu=(static_cast<float>(coor->begx)+xx[i]+0.5f)/textureSizeX;
                vertices[i].tv=(static_cast<float>(coor->begy)+yy[i]+0.5f)/textureSizeY;
            }

            for (int i=0;i<4;++i) {
                vertices[i].x*=scaleX;
                vertices[i].y*=scaleY;
                if (PropHardwareDirect()) {
                    const float oldX=vertices[i].x;
                    const float oldY=vertices[i].y;
                    vertices[i].x=dir.RotateX(oldX,oldY);
                    vertices[i].y=dir.RotateY(oldX,oldY);
                }
                vertices[i].x+=static_cast<float>(x);
                vertices[i].y+=static_cast<float>(y);
                vertices[i].z=z1;
                vertices[i].rhw=1.0f;
                vertices[i].diffuse=drawGamma.Diffuse();
                vertices[i].specular=drawGamma.Specular();
            }

            m_textures[coor->nsurf]->SetTexture(0);
            Graph->SetRenderState(0x1Du,drawGamma.Specular()!=0 ? 1u : 0u);
            Graph->DrawPrimitive(5u,0x1C4u,vertices,0x20u,4);

            coor=coor->next_fragment ? &m_texCoor[coor->next_fragment] : 0;
        }
    }
}

void VID_HARDWARE::SetLayer()
{
    if (PropGround())
        m_layer=4;
    else if (m_unknown0C==0x40u)
        m_layer=10;
    else if (!m_noSurf)
        m_layer=16;
    else if (IsZBufferType() && IsAlphaType())
        m_layer=9;
    else if (IsZBufferType())
        m_layer=0;
    else if (PropAlwaysTop() && m_noDirections==0xFFu)
        m_layer=15;
    else if (PropAlwaysTop())
        m_layer=14;
    else if (IsAlphaType())
        m_layer=PropWave()?9:12;
    else
        m_layer=8;
}



// Hardware SURF/DATA reader derived against the MapEdit body.  The newer
// engine-family implementation was used only as a naming aid; format gates,
// old/new DATA layout, error routing and resource shifts below follow MapEdit.
void VID_HARDWARE::Load(RESOURCE* res)
{
    QS1_CODER* colorCoder=0;
    QS1_CODER* zCoder=0;

    if (res->GoNext(0x46525553u)) // 'SURF'
        Error(5,const_cast<char*>("SURF"),0);

    res->Read(&m_noSurf,2u);
    if (m_noSurf==0)
        return;

    m_textures=static_cast<TEXTURE**>(operator new(static_cast<unsigned int>(m_noSurf)*sizeof(TEXTURE*)));
    if (!m_textures) {
        Error(2,const_cast<char*>("textures"),static_cast<unsigned long>(m_noSurf));
        return;
    }
    for (int i=0;i<m_noSurf;++i)
        m_textures[i]=0;

    unsigned char* const unpack=static_cast<unsigned char*>(operator new(0x20008u));
    if (!unpack) {
        Error(2,const_cast<char*>("(unpack)"),0);
        return;
    }

    if (IsCompressType()) {
        colorCoder=new QS1_CODER(IsDXTType() ? 1 : 2);
        zCoder=new QS1_CODER(2);
    }

    unsigned char palette[768];
    int i=0;
    while (i<m_noSurf) {
        Graph->DrawLoadBar(Map->Vid(0));

        short width=0;
        short height=0;
        res->Read(&width,2u);
        res->Read(&height,2u);

        if (IsDXTType()) {
            const int format=(IsAlphaType() && IsTextureType()) ? 0x33545844 : 0x31545844; // DXT3 / DXT1
            m_textures[i]=new TEXTURE(width,height,format,0);
        } else if (IsPaletteType()) {
            m_textures[i]=new TEXTURE(width,height,41,0); // P8
        } else if (IsAlphaType() && IsTextureType()) {
            m_textures[i]=new TEXTURE(width,height,26,0); // A4R4G4B4
        } else {
            m_textures[i]=new TEXTURE(width,height,23,0); // R5G6B5
        }

        if (!m_textures[i] || !m_textures[i]->IsExist()) {
            Error(3,const_cast<char*>("texture"),0);
            delete colorCoder;
            delete zCoder;
            operator delete(unpack);
            return;
        }

        if (IsPaletteType()) {
            Error(10,const_cast<char*>("palette %i"),static_cast<unsigned long>(m_textures[i]->IsPaletted()));
            res->Read(palette,768u);
            if (m_textures[i]->IsPaletted()) {
                COLOR colors[256];
                for (int p=0;p<256;++p)
                    colors[p]=COLOR(palette[p*3],palette[p*3+1],palette[p*3+2]);
                m_textures[i]->SetPalette(colors);
            }
        }

        int packedSize=0;
        res->Read(&packedSize,4u);
        const int decodeError=res->ReadPacked(unpack,static_cast<unsigned int>(packedSize),colorCoder);
        if (decodeError)
            Error(5,const_cast<char*>("Can't decode"),static_cast<unsigned long>(i));

        int pitch=0;
        if (IsPaletteType() || packedSize>=2*static_cast<int>(width)*static_cast<int>(height)) {
            unsigned char* dst=m_textures[i]->Lock(&pitch,0);
            if (!dst) {
                Error(0,const_cast<char*>("texture surface"),0);
                delete colorCoder;
                delete zCoder;
                operator delete(unpack);
                return;
            }

            for (int y=0;y<height;++y) {
                unsigned short* const row=reinterpret_cast<unsigned short*>(dst + pitch*y);
                if (IsPaletteType()) {
                    if (!m_textures[i]->IsPaletted()) {
                        for (int x=0;x<width;++x) {
                            const unsigned int pi=unpack[x+y*width];
                            RGB16 pixel(palette[pi*3],palette[pi*3+1],palette[pi*3+2]);
                            row[x]=pixel.color;
                        }
                    } else {
                        memcpy(row,unpack+y*width,static_cast<unsigned int>(width));
                    }
                } else if (m_textures[i]->Format()!=23 && m_textures[i]->Format()!=26) {
                    const unsigned short* const src=reinterpret_cast<const unsigned short*>(unpack)+y*width;
                    for (int x=0;x<width;++x) {
                        const unsigned short value=src[x];
                        row[x]=static_cast<unsigned short>((value & 0x001Fu) | ((value >> 1) & 0x7FE0u));
                    }
                } else {
                    memcpy(row,reinterpret_cast<const unsigned short*>(unpack)+y*width,
                           static_cast<unsigned int>(2*width));
                }
            }
            m_textures[i]->UnLock();
        } else {
            Error(10,const_cast<char*>("Load DXT"),0);
            unsigned char* dst=m_textures[i]->Lock(&pitch,0);
            if (dst) {
                memcpy(dst,unpack,static_cast<unsigned int>(packedSize));
                m_textures[i]->UnLock();
            } else {
                Error(0,const_cast<char*>("DXT texture surface"),0);
            }
        }

        if (IsZBufferType()) {
            ++i;
            m_textures[i]=new TEXTURE(width,height,80,2); // D16
            res->Read(&packedSize,4u);

            if (!m_textures[i] || !m_textures[i]->IsExist()) {
                Error(3,const_cast<char*>("texture z surface"),0);
                res->Shift(packedSize);
                ++i;
                continue;
            }

            unsigned char* const zdst=m_textures[i]->Lock(&pitch,0);
            if (!zdst) {
                Error(0,const_cast<char*>("texture z surface"),0);
                res->Shift(packedSize);
                ++i;
                continue;
            }

            if (packedSize==pitch*static_cast<int>(height)) {
                const int zDecodeError=res->ReadPacked(zdst,static_cast<unsigned int>(packedSize),zCoder);
                if (zDecodeError)
                    Error(5,const_cast<char*>("Can't decode z"),static_cast<unsigned long>(packedSize-zDecodeError));
            } else {
                Error(5,const_cast<char*>("ZBuffer: invalid size"),static_cast<unsigned long>(packedSize));
            }
            m_textures[i]->UnLock();
        }
        ++i;
    }

    if (res->GoNext(0x41544144u)) // 'DATA'
        Error(5,const_cast<char*>("DATA"),0);

    if (IsNewVersionType()) {
        res->SubLoad(reinterpret_cast<void**>(&m_texCoor),0);
        if (!m_texCoor)
            Error(5,const_cast<char*>("tex_coor"),0);
    } else {
        const int count=res->SubSize()/20;
        m_texCoor=static_cast<VID_TEXCOOR*>(operator new(static_cast<unsigned int>(count)*sizeof(VID_TEXCOOR)));
        for (int c=0;c<count;++c)
            m_texCoor[c].Read(res,0);
    }

    res->GoNext(0x44414853u); // 'SHAD'; return value intentionally ignored
    delete colorCoder;
    delete zCoder;
    operator delete(unpack);
}

void VID_TEXCOOR::Read(STREAM* res,int new_version)
{
    if (new_version) {
        res->Read(this,sizeof(*this));
        return;
    }

    res->Read(&shadow_shift,4u);
    short value=0;
    res->Read(&value,2u); nsurf=value;
    res->Read(&value,2u); begx=value;
    res->Read(&value,2u); begy=value;
    res->Read(&value,2u); sizex=value;
    res->Read(&value,2u); sizey=value;
    res->Read(&value,2u); shiftx=value;
    res->Read(&value,2u); shifty=value;
    res->Read(&value,2u); next_fragment=value;
}

void VID::SetChildAndLink()
{
    if (m_linkVidIndex) {
        if (Map->ValidateVid(m_linkVidIndex))
            m_linkVid=Map->Vid(m_linkVidIndex)->m_exchangeVid;
        else
            Error(4,const_cast<char*>("LinkVid"),static_cast<unsigned long>(m_linkVidIndex));
    }

    if (m_weapon) {
        int* p=reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(m_weapon)+0x84);
        for (int i=0;i<8;++i,++p) {
            if (p[-8] || p[0] || p[8] || p[16])
                m_propertyBits|=1u;
            if (p[24]!=0x3F800000 || p[32]!=0x3F800000 || p[40]!=0x3F800000)
                m_propertyBits|=2u;
            if (reinterpret_cast<float*>(p)[48]!=0.0f ||
                reinterpret_cast<float*>(p)[56]!=0.0f ||
                reinterpret_cast<float*>(p)[64]!=0.0f)
                m_propertyBits|=4u;
            if (p[72] || p[80] || p[88])
                m_propertyBits|=8u;
        }
    }

    if (m_defaultDeathTimer!=999999u)
        m_exSpriteData=1;
    else if (PropBlur())
        m_exSpriteData=1;
    else if (m_propertyBits&1u)
        m_exSpriteData=1;
    else if ((m_propertyBits>>1)&1u)
        m_exSpriteData=1;
    else if ((m_propertyBits>>2)&1u)
        m_exSpriteData=1;
    else if ((m_propertyBits>>3)&1u)
        m_exSpriteData=1;
    else if (PropRandSpeed())
        m_exSpriteData=1;
    else if (PropVertDir())
        m_exSpriteData=1;

    for (int i=0;i<17;++i) {
        const int child=m_aniSpawnMode[i];
        if (!child)
            continue;
        const int childIndex=abs(child);
        if (Map->ValidateVid(childIndex)) {
            VID* mirror=Map->Vid(childIndex)->m_exchangeVid;
            m_aniChildVid[i]=mirror;
            if (mirror && (mirror->PropBirthAsSmoke() || mirror->PropVertDir()))
                m_exSpriteData=1;
        } else {
            Error(4,const_cast<char*>("child"),static_cast<unsigned long>(child));
        }
    }
}

// ---- MAP::CreateVid prerequisite family: ABI-proven short virtuals ----

void VID_LIGHT::SetLayer()
{
    m_layer=11;
}

VID_SOFTWARE::VID_SOFTWARE(VID_SOFTWARE* source)
    : VID()
{
    m_mirrorNext=source->m_mirrorNext;
    source->m_mirrorNext=this;
    m_layer=source->m_layer;
    m_extraTypeFlags=source->m_extraTypeFlags;
    m_dotFrameCount=source->m_dotFrameCount;
    m_phaseRandomInterval=source->m_phaseRandomInterval;
    m_regionTileStepX=source->m_regionTileStepX;
    m_regionTileStepY=source->m_regionTileStepY;
    m_cadrShift=source->m_cadrShift;
    m_cadrs=source->m_cadrs;
    m_cadrSize=source->m_cadrSize;

    m_nLinkDots=source->m_nLinkDots;
    if (m_nLinkDots>0) {
        m_linkDots=static_cast<VID_DOT*>(::operator new(static_cast<size_t>(m_nLinkDots)*sizeof(VID_DOT)));
        for (int i=0;i<m_nLinkDots;++i)
            m_linkDots[i]=source->m_linkDots[i];
    }

    if (source->m_dotFrameStarts) {
        m_dotFrameStarts=static_cast<int*>(::operator new(static_cast<size_t>(m_dotFrameCount)*sizeof(int)));
        for (int i=0;i<m_dotFrameCount;++i)
            m_dotFrameStarts[i]=source->m_dotFrameStarts[i];
    } else {
        m_dotFrameStarts=0;
    }
}

VID_SOFTWARE::VID_SOFTWARE()
    : VID(), m_cadrShift(0), m_cadrSize(0), m_cadrs(0)
{
}

VID_SOFTWARE::~VID_SOFTWARE()
{
    if (m_mirrorNext==this) {
        if (m_cadrs)
            ::operator delete(m_cadrs);
        m_cadrs=0;
        if (m_cadrShift)
            ::operator delete(m_cadrShift);
        m_cadrShift=0;
        g_vidMemoryInUse-=m_cadrSize;
        m_cadrSize=0;
    }
}

VID* VID_SOFTWARE::CreateMirror()
{
    return new VID_SOFTWARE(this);
}

int VID_SOFTWARE::HaveShadow()
{
    if (!m_cadrs)
        return 0;
    return *reinterpret_cast<const short*>(m_cadrs+m_cadrShift[0]);
}

int VID_SOFTWARE::PaletteSize()
{
    return 1024;
}

void VID_SOFTWARE::SetLayer()
{
    if ((m_propertyBits>>5)&1u)
        m_layer=IsAlphaType()?2:1;
    else if (PropGround())
        m_layer=3;
    else if (PropAlwaysTop() && m_noDirections==0xFFu)
        m_layer=15;
    else if (PropAlwaysTop())
        m_layer=13;
    else if (IsAlphaType())
        m_layer=7;
    else if (PropBuildSizeToGridZ() || PropBuildVidZToGridZ())
        m_layer=5;
    else
        m_layer=6;
}

void VID_SOFTWARE::SetGammaToPalette(unsigned char* palette,const GAMMA* gamma)
{
    if (!palette || (gamma->subtractive==0 && gamma->additive==0))
        return;
    COLOR* colors=reinterpret_cast<COLOR*>(palette);
    for (int i=0;i<256;++i) {
        COLOR transformed(gamma,&colors[i]);
        colors[i].operator=(&transformed);
    }
}

VID_SOFTWARE16::VID_SOFTWARE16()
    : VID_SOFTWARE()
{
}

VID_SOFTWARE16::VID_SOFTWARE16(VID_SOFTWARE16* source)
    : VID_SOFTWARE(source)
{
}

VID_SOFTWARE16::~VID_SOFTWARE16()
{
}

VID* VID_SOFTWARE16::CreateMirror()
{
    return new VID_SOFTWARE16(this);
}

int VID_SOFTWARE16::PaletteSize()
{
    return (IsAlphaType()?4:2)<<8;
}

void VID_SOFTWARE16::SetGammaToPalette(unsigned char* palette,const GAMMA* gamma)
{
    if (!palette || (gamma->subtractive==0 && gamma->additive==0))
        return;

    if (IsAlphaType()) {
        COLOR* colors=reinterpret_cast<COLOR*>(palette);
        for (int i=0;i<256;++i) {
            COLOR transformed(gamma,&colors[i]);
            colors[i].operator=(&transformed);
        }
        return;
    }

    RGB16* colors=reinterpret_cast<RGB16*>(palette);
    for (int i=0;i<256;++i) {
        COLOR source(&colors[i]);
        COLOR transformed(gamma,&source);
        RGB16 packed(&transformed);
        colors[i].color=packed.color;
    }
}

VID_HARDWARE_Z::VID_HARDWARE_Z()
    : VID_SOFTWARE()
{
}

VID_HARDWARE_Z::VID_HARDWARE_Z(VID_HARDWARE_Z* source)
    : VID_SOFTWARE(source)
{
}

VID_HARDWARE_Z::~VID_HARDWARE_Z()
{
}

VID* VID_HARDWARE_Z::CreateMirror()
{
    return new VID_HARDWARE_Z(this);
}

void VID_HARDWARE_Z::DrawToVid(const SPRITE* /*sprite*/,const VID_TEXCOOR* /*coor*/,TEXTURE* /*video_tex*/,TEXTURE* /*z_tex*/)
{
}

void VID_HARDWARE_Z::SetGamma(const GAMMA* gamma,unsigned int n_gamma)
{
    VID::SetGamma(gamma,n_gamma);
}

void VID_HARDWARE_Z::SetLayer()
{
    if (PropGround())
        m_layer=4;
    else if (m_unknown0C==0x40u)
        m_layer=10;
    else if (IsZBufferType() && IsAlphaType())
        m_layer=12;
    else if (IsAlphaType())
        m_layer=PropWave()?9:12;
    else
        m_layer=8;
}

// debug metadata size 0x488, original vtable 0x004B5324.
VID_FONT::VID_FONT()
    : VID(), m_font(0)
{
}

// Retail copy constructor intentionally only constructs VID and installs the
// VID_FONT vtable; it does not copy/initialize +0x484.
VID_FONT::VID_FONT(VID_FONT* /*source*/)
    : VID()
{
}

VID_FONT::~VID_FONT()
{
}

VID* VID_FONT::CreateMirror()
{
    return new VID_FONT(this);
}

// These are genuine no-op bodies in MapEdit.exe, not implementation stubs.
void VID_FONT::Load(RESOURCE* /*res*/) {}
void VID_FONT::SetLayer() {}
void VID_FONT::Draw(const SPRITE* /*sprite*/) {}
void VID_FONT::RestoreDeviceObjects() {}
void VID_FONT::InvalidateDeviceObjects() {}

// ---------------------------------------------------------------------------
// VID_LIGHT — complete retail owner used by MAP::CreateVid.
// Vtable 0x004B52FC, debug metadata sizeof 0x48C.
// ---------------------------------------------------------------------------

VID_LIGHT::VID_LIGHT()
    : VID(), m_cadrSize(0), m_cadrs(0)
{
}

VID_LIGHT::VID_LIGHT(VID_LIGHT* source)
    : VID()
{
    m_mirrorNext=source->m_mirrorNext;
    source->m_mirrorNext=this;
    m_layer=source->m_layer;
    m_extraTypeFlags=source->m_extraTypeFlags;
    m_dotFrameCount=source->m_dotFrameCount;
    m_phaseRandomInterval=source->m_phaseRandomInterval;
    m_regionTileStepX=source->m_regionTileStepX;
    m_regionTileStepY=source->m_regionTileStepY;
    m_cadrs=source->m_cadrs;
    m_cadrSize=source->m_cadrSize;
}

VID_LIGHT::~VID_LIGHT()
{
    if (m_mirrorNext==this) {
        if (m_cadrs)
            operator delete(m_cadrs);
        m_cadrs=0;
        g_vidMemoryInUse-=m_cadrSize;
        m_cadrSize=0;
    }
}

VID* VID_LIGHT::CreateMirror()
{
    return new VID_LIGHT(this);
}

void VID_LIGHT::Load(RESOURCE* res)
{
    if (res->GoNext(0x41544144u))
        Error(5,const_cast<char*>("DATA"),0);

    // Retail executes the VC6 _ftol helper (0x0049909C) and stores AX.
    // Keep its truncate/overflow boundary semantics instead of relying on the
    // current compiler's float-to-int lowering.
    m_regionTileStepX=static_cast<short>(RetailFtolLow32(m_footprintWidth));
    m_regionTileStepY=static_cast<short>(RetailFtolLow32(m_footprintHeight));
    m_cadrSize=res->SubLoad(reinterpret_cast<void**>(&m_cadrs),0);
    if (!m_cadrSize)
        Error(5,const_cast<char*>("cadr"),0);
    g_vidMemoryInUse+=m_cadrSize;
}

namespace {
void SetRetailLightTextureStageColorOp(unsigned long value)
{
    void* const device=Graph->D3DDevice();
    void** const vtable=*reinterpret_cast<void***>(device);
    typedef long (__stdcall *SetTextureStageStateFn)(void*,unsigned long,unsigned long,unsigned long);
    reinterpret_cast<SetTextureStageStateFn>(vtable[0x94/4])(device,0u,1u,value);
}
}

void VID_LIGHT::Draw(const SPRITE* sprite)
{
    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    COLOR color=m_cadrs[spr->CurrentCadr()];
    if (PropHide())
        return;

    COLOR transparentBlack(0,0,0,0);
    if (color.operator==(&transparentBlack))
        return;
    COLOR opaqueBlack(0,0,0);
    if (color.operator==(&opaqueBlack))
        return;

    if (PropDblLight())
        SetRetailLightTextureStageColorOp(5u);

    GAMMA* const vidGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x2D8);
    if (PropGamma()) {
        GAMMA spriteGamma=spr->GetGamma();
        GAMMA sum=*vidGamma + &spriteGamma;
        COLOR transformed(&sum,&color);
        color.operator=(&transformed);
    } else {
        GAMMA graphGamma=Graph->GetGamma();
        GAMMA spriteGamma=spr->GetGamma();
        GAMMA first=*vidGamma + &spriteGamma;
        GAMMA sum=first + &graphGamma;
        COLOR transformed(&sum,&color);
        color.operator=(&transformed);
    }

    Graph->DrawLightSource(spr->ScreenX(),spr->ScreenY(),spr->Z(),
                           m_footprintWidth,m_footprintHeight,color);

    if (PropDblLight())
        SetRetailLightTextureStageColorOp(4u);
}

// This is the editor-era software VID loader.  In particular, unlike the
// newer portable branch it does not add a per-frame alignment reserve to
// m_cadrSize and it advances frame payloads by the exact packed size.
void VID_SOFTWARE::Load(RESOURCE* res)
{
    COLOR palette[256];
    QS1_CODER* comp=IsCompressType() ? new QS1_CODER(1) : 0;

    if (IsPaletteType()) {
        if (!res->GoNext(0x204C4150u)) { // 'PAL '
            if (IsNewVersionType()) {
                res->Read(palette,1024u);
            } else {
                unsigned char palette24[768];
                res->Read(palette24,768u);
                for (int i=0;i<256;++i) {
                    COLOR color(palette24[i*3],palette24[i*3+1],palette24[i*3+2]);
                    palette[i].operator=(&color);
                }
            }

            GAMMA* const vidGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x2D8);
            for (int i=0;i<256;++i) {
                COLOR transformed(vidGamma,&palette[i]);
                palette[i].operator=(&transformed);
            }
        } else {
            Error(5,const_cast<char*>("PAL "),0);
        }
    }

    if (res->GoNext(0x41544144u)) // 'DATA'
        Error(5,const_cast<char*>("DATA"),0);

    m_cadrSize=res->ResSize();
    if (IsPaletteType())
        m_cadrSize+=2*PaletteSize();

    m_cadrs=static_cast<unsigned char*>(::operator new(static_cast<size_t>(m_cadrSize)));
    if (!m_cadrs) {
        Error(2,const_cast<char*>("cadr"),static_cast<unsigned long>(m_cadrSize));
        return;
    }

    m_cadrShift=static_cast<int*>(::operator new(static_cast<size_t>(m_dotFrameCount)*sizeof(int)));
    if (!m_cadrShift) {
        Error(2,const_cast<char*>("cadrShift"),static_cast<unsigned long>(m_dotFrameCount));
        return;
    }

    int shift=0;
    if (IsPaletteType()) {
        for (int i=0;i<256;++i) {
            if (PaletteSize()==1024) {
                reinterpret_cast<COLOR*>(m_cadrs)[i].operator=(&palette[i]);
            } else {
                RGB16 packed(&palette[i]);
                reinterpret_cast<RGB16*>(m_cadrs)[i].color=packed.color;
            }
        }
        shift=2*PaletteSize();
        memcpy(m_cadrs+PaletteSize(),m_cadrs,static_cast<size_t>(PaletteSize()));
    }

    for (int i=0;i<m_dotFrameCount;++i) {
        int size=0;
        res->Read(&size,4u);
        const int err=res->ReadPacked(m_cadrs+shift,static_cast<unsigned int>(size),comp);
        if (err)
            Error(5,const_cast<char*>("Can't decode software"),static_cast<unsigned long>(size-err));

        if (size!=2) {
            // Retail converts direct 16-bit frame data from 565 to 555 when
            // GRAPH's surface at +0xBE4 reports format 24 and the VID is not
            // an alpha/paletted stream.
            if (!IsPaletteType() && Graph->Is15Bit() && !IsAlphaType()) {
                unsigned char* p=m_cadrs+shift;
                const short noContour=*reinterpret_cast<short*>(p);
                p+=2+6*noContour;
                int y=*reinterpret_cast<short*>(p); p+=2;
                const int endY=y+*reinterpret_cast<short*>(p); p+=2;
                while (y<endY) {
                    while (*reinterpret_cast<short*>(p)) {
                        p+=1;
                        int noDot=*p++;
                        for (int j=0;j<noDot;++j) {
                            unsigned short value=*reinterpret_cast<unsigned short*>(p);
                            value=static_cast<unsigned short>((value&0x001Fu)|((value>>1)&0x7FE0u));
                            *reinterpret_cast<unsigned short*>(p)=value;
                            p+=2;
                        }
                    }
                    p+=2;
                    ++y;
                }
            }
            m_cadrShift[i]=shift;
            shift+=size;
        } else {
            const short frame=*reinterpret_cast<short*>(m_cadrs+shift);
            m_cadrShift[i]=m_cadrShift[frame];
        }
        res->GoNextSub(0x41544144u);
    }

    delete comp;
    g_vidMemoryInUse+=m_cadrSize;
    SetLayer();

    // PropBuildVidZToGridZ generates collision/link dots from the packed
    // software frame.  The 2048x2048 source domain is reduced to a 256x256
    // grid (8x8 cells), exactly matching the original stack buffer walk.
    if (PropBuildVidZToGridZ() && IsPaletteType() && IsTextureType()) {
        float* const scratch=static_cast<float*>(::operator new(0x3000000u));
        m_dotFrameStarts=static_cast<int*>(::operator new(static_cast<size_t>(m_dotFrameCount)*sizeof(int)));

        for (int ncadr=0;ncadr<m_dotFrameCount;++ncadr) {
            short grid[65536];
            for (int i=0;i<65536;++i)
                grid[i]=static_cast<short>(-32000);

            m_dotFrameStarts[ncadr]=m_nLinkDots;
            unsigned char* p=m_cadrs+m_cadrShift[ncadr];
            const short noContour=*reinterpret_cast<short*>(p);
            p+=2+6*noContour;

            if (IsPaletteType() && IsTextureType() && IsZBufferType()) {
                int y=*reinterpret_cast<short*>(p); p+=2;
                const int endY=y+*reinterpret_cast<short*>(p); p+=2;
                while (y<endY) {
                    int x=0;
                    while (*reinterpret_cast<short*>(p)) {
                        x+=*p++;
                        const int noDot=*p++;
                        unsigned char* zdata=p;
                        for (int j=0;j<noDot;++j) {
                            const int z=(static_cast<int>(*reinterpret_cast<unsigned short*>(zdata))>>3)-128;
                            const int zz=z+y;
                            const int xx=x+j;
                            if (zz>=0 && zz<2048 && xx>=0 && xx<2048) {
                                short& cell=grid[(zz/8)*256+(xx/8)];
                                if (z>cell)
                                    cell=static_cast<short>(z);
                            }
                            zdata+=2;
                        }
                        p+=3*noDot;
                        x+=noDot;
                    }
                    ++y;
                    p+=2;
                }
            } else if (IsPaletteType() && IsTextureType()) {
                int y=*reinterpret_cast<short*>(p); p+=2;
                const int endY=y+*reinterpret_cast<short*>(p); p+=2;
                while (y<endY) {
                    int x=0;
                    while (*reinterpret_cast<short*>(p)) {
                        x+=*p++;
                        const int noDot=*p++;
                        for (int j=0;j<noDot;++j) {
                            const int xx=x+j;
                            if (y>=0 && y<2048 && xx>=0 && xx<2048)
                                grid[(y/8)*256+(xx/8)]=0;
                        }
                        p+=noDot;
                        x+=noDot;
                    }
                    ++y;
                    p+=2;
                }
            }

            for (int y=m_regionTileStepY/8-1;y>=0;--y) {
                for (int x=0;x<m_regionTileStepX/8;++x) {
                    const short z=grid[y*256+x];
                    if (z!=static_cast<short>(-32000)) {
                        // Original performs signed integer /2 first (CDQ/SUB/SAR),
                        // then converts the result to float.  Dividing by 2.0f would
                        // incorrectly introduce a half-cell offset for odd extents.
                        const int halfWidth=m_regionTileStepX/2;
                        const int halfHeight=m_regionTileStepY/2;
                        scratch[m_nLinkDots*3+0]=static_cast<float>(x*8-halfWidth);
                        scratch[m_nLinkDots*3+1]=static_cast<float>(y*8-halfHeight);
                        scratch[m_nLinkDots*3+2]=static_cast<float>(z);
                        ++m_nLinkDots;
                    }
                }
            }
        }

        m_linkDots=static_cast<VID_DOT*>(::operator new(static_cast<size_t>(m_nLinkDots)*sizeof(VID_DOT)));
        for (int i=0;i<m_nLinkDots;++i) {
            m_linkDots[i].x=scratch[i*3+0];
            m_linkDots[i].y=scratch[i*3+1];
            m_linkDots[i].z=scratch[i*3+2];
        }
        ::operator delete(scratch);
    }
}


namespace {
// Retail vid.obj work globals used by the software raster helpers.
// MapEdit.exe addresses: Z vector 0x004BF120, active palette 0x004BF138.
// The derived C++ keeps these as module-private state; VID_SOFTWARE::Draw
// supplies the values before dispatching a scanline helper.
short g_softwareRasterZ[4]={0,0,0,0};
void* g_softwareRasterPalette=0;
}

void DrawSpan32(void* data,void* zbuffer,void* video,int no_dot)
{
    unsigned char* src=static_cast<unsigned char*>(data);
    short* z=static_cast<short*>(zbuffer);
    COLOR* dst=static_cast<COLOR*>(video);
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const short depth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        if (depth>z[i]) {
            z[i]=depth;
            dst[i]=palette[src[i]];
        }
    }
}

void DrawSpanAlpha32(unsigned char* data,unsigned short* zbuffer,COLOR* video,int no_dot)
{
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const unsigned short depth=static_cast<unsigned short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        if (depth>=zbuffer[i]) {
            COLOR source=palette[data[i]];
            video[i].AlphaAdd(source,source.Alpha());
        }
    }
}

void DrawSpan16(void* data,void* zbuffer,void* video,int no_dot)
{
    unsigned char* src=static_cast<unsigned char*>(data);
    short* z=static_cast<short*>(zbuffer);
    unsigned short* dst=static_cast<unsigned short*>(video);
    unsigned short* palette=static_cast<unsigned short*>(g_softwareRasterPalette);
    const short depth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        if (depth>z[i]) {
            z[i]=depth;
            dst[i]=palette[src[i]];
        }
    }
}

void DrawSpanAlpha16(unsigned char* data,unsigned short* zbuffer,RGB16* video,int no_dot)
{
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const unsigned short depth=static_cast<unsigned short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        if (depth>=zbuffer[i]) {
            COLOR source=palette[data[i]];
            COLOR destination(&video[i]);
            destination.AlphaAdd(source,source.Alpha());
            RGB16 result(&destination);
            video[i].color=result.color;
        }
    }
}

void DrawSpanAlpha16Mapped(unsigned char* data,unsigned short* zbuffer,RGB16* video,int no_dot,int max_left_shift)
{
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const int depth=static_cast<unsigned short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        int destinationIndex=i;
        const int destinationDepth=static_cast<unsigned short>(zbuffer[i]);
        if (depth<destinationDepth) {
            const int leftShift=(destinationDepth-depth)/16;
            if (leftShift>=max_left_shift)
                continue;
            destinationIndex=i-leftShift;
        }
        COLOR source=palette[data[i]];
        COLOR destination(&video[destinationIndex]);
        destination.AlphaAdd(source,source.Alpha());
        RGB16 result(&destination);
        video[destinationIndex].color=result.color;
    }
}

// Retail uses MMX and processes four 16-bit lanes at a time.  This source
// preserves the per-lane compare/update result; exact VC6 MMX codegen remains
// a comparison tooling tuning item rather than being faked with inline assembly.
void DrawAlphaSpan(void* data,void* zbuffer,void* surface,int no_dot)
{
    unsigned short* src=static_cast<unsigned short*>(data);
    short* z=static_cast<short*>(zbuffer);
    unsigned short* dst=static_cast<unsigned short*>(surface);
    for (int i=0;i<no_dot;++i) {
        const short depth=g_softwareRasterZ[i&3];
        if (depth>z[i]) {
            z[i]=depth;
            dst[i]=src[i];
        } else {
            dst[i]=0;
        }
    }
}

void DrawAlphaSpanWithZ(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,unsigned short* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    const int baseDepth=static_cast<short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        const int depth=baseDepth+static_cast<short>(zdelta[i]);
        const int destinationDepth=static_cast<short>(zbuffer[i]);
        const unsigned int source=data ? reinterpret_cast<unsigned short*>(data)[i] : 0u;
        if (depth<destinationDepth) {
            video[i]=0;
        } else if (depth>destinationDepth+0x7F) {
            video[i]=static_cast<unsigned short>(source);
        } else {
            int alpha=((depth-destinationDepth)*static_cast<int>(source))>>7;
            if (alpha>0xFFFF)
                alpha=0xF000;
            else
                alpha&=0xF000;
            video[i]=static_cast<unsigned short>((source&0x0FFFu)+static_cast<unsigned int>(alpha));
        }
    }
}

void DrawLightSpanWithZ(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,unsigned short* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    unsigned short* source=reinterpret_cast<unsigned short*>(data);
    const int baseDepth=static_cast<short>(g_softwareRasterZ[0]);
    for (int i=0;i<no_dot;++i) {
        const int depth=baseDepth+static_cast<short>(zdelta[i]);
        const int destinationDepth=static_cast<short>(zbuffer[i]);
        if (depth<destinationDepth) {
            video[i]=0;
        } else if (depth>destinationDepth+0x7F) {
            video[i]=static_cast<unsigned short>(source[i]|0xF000u);
        } else {
            const int alpha=(depth-destinationDepth)/8;
            video[i]=static_cast<unsigned short>(source[i]|(alpha<<12));
        }
    }
}

void DrawSpanWithZ32(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,COLOR* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    COLOR* palette=static_cast<COLOR*>(g_softwareRasterPalette);
    const short baseDepth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        const short depth=static_cast<short>(baseDepth+zdelta[i]);
        if (depth>static_cast<short>(zbuffer[i])) {
            zbuffer[i]=static_cast<unsigned short>(depth);
            video[i]=palette[data[i]];
        }
    }
}

void DrawSpanWithZ16(unsigned char* zdata,unsigned char* data,unsigned short* zbuffer,void* video,int no_dot)
{
    short* zdelta=reinterpret_cast<short*>(zdata);
    unsigned short* dst=static_cast<unsigned short*>(video);
    unsigned short* palette=static_cast<unsigned short*>(g_softwareRasterPalette);
    const short baseDepth=g_softwareRasterZ[0];
    for (int i=0;i<no_dot;++i) {
        const short depth=static_cast<short>(baseDepth+zdelta[i]);
        if (depth>static_cast<short>(zbuffer[i])) {
            zbuffer[i]=static_cast<unsigned short>(depth);
            dst[i]=palette[data[i]];
        }
    }
}

// Retail software 32-bit raster owner.  This deliberately follows the
// editor-era packed-frame paths: there is no portable-branch UI scaling.
void VID_SOFTWARE::Draw(const SPRITE* sprite)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);
    int shiftx=spr->ScreenXInt()-width/2;
    int shifty=spr->ScreenYInt()-height/2;
    if (!VID::BoxInViewPort(shiftx,shifty,shiftx+width,shifty+height))
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wave_z=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wave_z;
        // VC6 signed division by 8 is truncation toward zero.
        shifty-=wave_z/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+contours*6;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    int no_row=*reinterpret_cast<short*>(p); p+=2;
    int max_y=y+no_row;
    if (y>=g_vidViewYMaxRetail || max_y<g_vidViewYMinRetail)
        return;
    if (max_y>g_vidViewYMaxRetail)
        max_y=g_vidViewYMaxRetail;

    int z_pitch=0;
    unsigned short* z_buffer=Graph->LockZ(&z_pitch);
    int pitch=0;
    COLOR* video=static_cast<COLOR*>(Graph->Lock(&pitch));

    unsigned char palette[1024];
    if (spr->HaveUniqueGamma()) {
        g_softwareRasterPalette=palette;
        const int palette_size=PaletteSize();
        const int base_offset=IsAltGammaType()?4*palette_size:0;
        memcpy(palette,m_cadrs+base_offset,static_cast<size_t>(palette_size));
        if (PropGamma()) {
            GAMMA gamma=spr->GetGamma();
            SetGammaToPalette(palette,&gamma);
        } else {
            GAMMA sprite_gamma=spr->GetGamma();
            GAMMA graph_gamma=Graph->GetGamma();
            GAMMA gamma=sprite_gamma+&graph_gamma;
            SetGammaToPalette(palette,&gamma);
        }
    } else {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        g_softwareRasterPalette=m_cadrs+palette_offset;
    }

    // Alpha + texture + palette path.
    if (IsAlphaType() && IsTextureType() && IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        COLOR* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawSpanAlpha32(data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            g_softwareRasterZ[0]=static_cast<short>(g_softwareRasterZ[0]+dz);
        }
        return;
    }

    if (!IsTextureType())
        return;

    // Palette + per-pixel Z-delta stream.
    if (IsPaletteType() && IsZBufferType()) {
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=3*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
            }
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        COLOR* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=p[0];
                int nodot=p[1];
                p+=2;
                unsigned char* const zdata=p;
                unsigned char* data=p+2*nodot;
                p=data+nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    data+=source_offset;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawSpanWithZ32(zdata+2*source_offset,data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
        }
        return;
    }

    // Direct RGB16 packed pixels expanded into the 32-bit software surface.
    if (!IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=2*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        COLOR* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=2*nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                for (int i=0;i<nodot;++i) {
                    if (shiftz>=static_cast<int>(z_buffer[draw_x+i])) {
                        z_buffer[draw_x+i]=static_cast<unsigned short>(shiftz);
                        const unsigned int c=*reinterpret_cast<unsigned short*>(data+2*(source_offset+i));
                        video[draw_x+i].color=0xFF000000u |
                            ((c&0x1Fu)<<3) |
                            ((c<<(8-RGB16::gShift))&0x0000FF00u) |
                            ((c<<(16-RGB16::rShift))&0x00FF0000u);
                    }
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
        return;
    }

    // Normal paletted software frame.
    shiftz+=0x400;
    if (shiftz>0x7FFF)
        shiftz=0x7FFF;
    if (y<g_vidViewYMinRetail) {
        while (y<g_vidViewYMinRetail) {
            while (*reinterpret_cast<short*>(p)!=0)
                p+=static_cast<unsigned int>(p[1])+2u;
            p+=2;
            ++y;
            --no_row;
        }
    }
    int dz=0;
    if (m_hitVerticalOffset>m_footprintHeight) {
        shiftz+=8*no_row;
        dz=-8;
    }
    g_softwareRasterZ[0]=static_cast<short>(shiftz);

    COLOR* max_video=video+max_y*pitch;
    video+=y*pitch;
    z_buffer+=y*z_pitch;
    const int unclipped=(shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail);
    while (video<max_video) {
        int x=shiftx;
        if (unclipped) {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                g_softwareRasterZ[0]=static_cast<short>(shiftz);
                DrawSpan32(data,z_buffer+x,video+x,nodot);
                x+=nodot;
            }
        } else {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0) {
                    g_softwareRasterZ[0]=static_cast<short>(shiftz);
                    DrawSpan32(data,z_buffer+draw_x,video+draw_x,nodot);
                }
            }
        }
        p+=2;
        video+=pitch;
        z_buffer+=z_pitch;
        shiftz+=dz;
    }
}

// Retail 16-bit raster path mirrors the packed-frame control flow of the
// 32-bit owner but writes RGB16 pixels and uses the dedicated 16-bit helpers.
void VID_SOFTWARE16::Draw(const SPRITE* sprite)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);
    int shiftx=spr->ScreenXInt()-width/2;
    int shifty=spr->ScreenYInt()-height/2;
    if (!VID::BoxInViewPort(shiftx,shifty,shiftx+width,shifty+height))
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wave_z=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wave_z;
        // VC6 signed division by 8 is truncation toward zero.
        shifty-=wave_z/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+contours*6;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    int no_row=*reinterpret_cast<short*>(p); p+=2;
    int max_y=y+no_row;
    if (y>=g_vidViewYMaxRetail || max_y<g_vidViewYMinRetail)
        return;
    if (max_y>g_vidViewYMaxRetail)
        max_y=g_vidViewYMaxRetail;

    int z_pitch=0;
    unsigned short* z_buffer=Graph->LockZ(&z_pitch);
    int pitch=0;
    RGB16* video=static_cast<RGB16*>(Graph->Lock(&pitch));

    unsigned char palette[1024];
    if (spr->HaveUniqueGamma()) {
        g_softwareRasterPalette=palette;
        const int palette_size=PaletteSize();
        const int base_offset=IsAltGammaType()?4*palette_size:0;
        memcpy(palette,m_cadrs+base_offset,static_cast<size_t>(palette_size));
        if (PropGamma()) {
            GAMMA gamma=spr->GetGamma();
            SetGammaToPalette(palette,&gamma);
        } else {
            GAMMA sprite_gamma=spr->GetGamma();
            GAMMA graph_gamma=Graph->GetGamma();
            GAMMA gamma=sprite_gamma+&graph_gamma;
            SetGammaToPalette(palette,&gamma);
        }
    } else {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        g_softwareRasterPalette=m_cadrs+palette_offset;
    }

    // Alpha + texture + palette path.
    if (IsAlphaType() && IsTextureType() && IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawSpanAlpha16(data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            g_softwareRasterZ[0]=static_cast<short>(g_softwareRasterZ[0]+dz);
        }
        return;
    }

    if (!IsTextureType())
        return;

    // Palette + per-pixel Z-delta stream.
    if (IsPaletteType() && IsZBufferType()) {
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=3*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
            }
        }
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=p[0];
                int nodot=p[1];
                p+=2;
                unsigned char* const zdata=p;
                unsigned char* data=p+2*nodot;
                p=data+nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    data+=source_offset;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawSpanWithZ16(zdata+2*source_offset,data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
        }
        return;
    }

    // Direct RGB16 packed pixels expanded into the 32-bit software surface.
    if (!IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=2*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=2*nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                for (int i=0;i<nodot;++i) {
                    if (shiftz>=static_cast<int>(z_buffer[draw_x+i])) {
                        z_buffer[draw_x+i]=static_cast<unsigned short>(shiftz);
                        const unsigned int c=*reinterpret_cast<unsigned short*>(data+2*(source_offset+i));
                        video[draw_x+i].color=static_cast<unsigned short>(c);
                    }
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
        return;
    }

    // Normal paletted software frame.
    shiftz+=0x400;
    if (shiftz>0x7FFF)
        shiftz=0x7FFF;
    if (y<g_vidViewYMinRetail) {
        while (y<g_vidViewYMinRetail) {
            while (*reinterpret_cast<short*>(p)!=0)
                p+=static_cast<unsigned int>(p[1])+2u;
            p+=2;
            ++y;
            --no_row;
        }
    }
    int dz=0;
    if (m_hitVerticalOffset>m_footprintHeight) {
        shiftz+=8*no_row;
        dz=-8;
    }
    g_softwareRasterZ[0]=static_cast<short>(shiftz);

    RGB16* max_video=video+max_y*pitch;
    video+=y*pitch;
    z_buffer+=y*z_pitch;
    const int unclipped=(shiftx>=g_vidViewXMinRetail && shiftx+width<=g_vidViewXMaxRetail);
    while (video<max_video) {
        int x=shiftx;
        if (unclipped) {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                g_softwareRasterZ[0]=static_cast<short>(shiftz);
                DrawSpan16(data,z_buffer+x,video+x,nodot);
                x+=nodot;
            }
        } else {
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0) {
                    g_softwareRasterZ[0]=static_cast<short>(shiftz);
                    DrawSpan16(data,z_buffer+draw_x,video+draw_x,nodot);
                }
            }
        }
        p+=2;
        video+=pitch;
        z_buffer+=z_pitch;
        shiftz+=dz;
    }
}

// Retail 16-bit prerender path used by VID_HARDWARE::DrawVidToVid.
void VID_SOFTWARE16::DrawToVid(const SPRITE* sprite,const VID_TEXCOOR* coor,TEXTURE* video_tex,TEXTURE* z_tex)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);

    int shiftx=static_cast<int>(spr->X()-static_cast<float>(width/2)-
                                static_cast<float>(coor->shiftx)-static_cast<float>(coor->begx));
    int shifty=static_cast<int>(spr->Y()-spr->Z()-static_cast<float>(height/2)-
                                static_cast<float>(coor->shifty)-static_cast<float>(coor->begy));
    if (!VID::BoxInViewPort(shiftx,shifty,shiftx+width,shifty+height))
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wave_z=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wave_z;
        // VC6 signed divide by 8 truncates toward zero.
        shifty-=wave_z/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+contours*6;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    int no_row=*reinterpret_cast<short*>(p); p+=2;
    int max_y=y+no_row;
    if (y>=g_vidViewYMaxRetail || max_y<g_vidViewYMinRetail)
        return;
    if (max_y>g_vidViewYMaxRetail)
        max_y=g_vidViewYMaxRetail;

    int z_pitch=0;
    unsigned short* z_buffer=reinterpret_cast<unsigned short*>(z_tex->Lock(&z_pitch,0));
    z_pitch/=2;
    int pitch=0;
    RGB16* video=reinterpret_cast<RGB16*>(video_tex->Lock(&pitch,0));
    pitch/=2;

    // The original VC6 body contains an explicitly false legacy mapped-alpha
    // block immediately after the locks (xor edx,edx / test edx,edx).  Its
    // code is unreachable in MapEdit and execution continues at 0x00435DC5.

    if (IsAlphaType() && IsTextureType() && IsPaletteType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        g_softwareRasterPalette=m_cadrs+palette_offset;
        g_softwareRasterZ[0]=static_cast<short>(shiftz);

        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    const int clip=g_vidViewXMinRetail-draw_x;
                    data+=clip;
                    nodot-=clip;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0) {
                    g_softwareRasterZ[0]=static_cast<short>(shiftz);
                    DrawSpanAlpha16(data,z_buffer+draw_x,video+draw_x,nodot);
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
        video_tex->UnLock();
        z_tex->UnLock();
        return;
    }

    if (IsTextureType() && IsPaletteType() && IsZBufferType()) {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        g_softwareRasterPalette=m_cadrs+palette_offset;
        g_softwareRasterZ[0]=static_cast<short>(shiftz);
        g_softwareRasterZ[1]=static_cast<short>(shiftz);
        g_softwareRasterZ[2]=static_cast<short>(shiftz);
        g_softwareRasterZ[3]=static_cast<short>(shiftz);

        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=3*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
            }
        }

        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=p[0];
                int nodot=p[1];
                p+=2;
                unsigned char* const zdata=p;
                unsigned char* data=p+2*nodot;
                p=data+nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    data+=source_offset;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                if (nodot>0)
                    DrawSpanWithZ16(zdata+2*source_offset,data,z_buffer+draw_x,video+draw_x,nodot);
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
        }
        video_tex->UnLock();
        z_tex->UnLock();
        return;
    }

    if (IsTextureType() && IsPaletteType()) {
        const int palette_offset=IsAltGammaType()?spr->Army()*PaletteSize():0;
        unsigned short* const palette=reinterpret_cast<unsigned short*>(m_cadrs+palette_offset);

        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }

        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                for (int i=0;i<nodot;++i) {
                    const int dst=draw_x+i;
                    if (shiftz>=static_cast<int>(z_buffer[dst])) {
                        z_buffer[dst]=static_cast<unsigned short>(shiftz);
                        video[dst].color=palette[data[source_offset+i]];
                    }
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
        video_tex->UnLock();
        z_tex->UnLock();
        return;
    }

    if (IsTextureType()) {
        shiftz+=0x400;
        if (shiftz>0x7FFF)
            shiftz=0x7FFF;
        if (y<g_vidViewYMinRetail) {
            while (y<g_vidViewYMinRetail) {
                while (*reinterpret_cast<short*>(p)!=0)
                    p+=2*static_cast<unsigned int>(p[1])+2u;
                p+=2;
                ++y;
                --no_row;
            }
        }
        int dz=0;
        if (m_hitVerticalOffset>m_footprintHeight) {
            shiftz+=8*no_row;
            dz=-8;
        }

        RGB16* max_video=video+max_y*pitch;
        video+=y*pitch;
        z_buffer+=y*z_pitch;
        while (video<max_video) {
            int x=shiftx;
            while (*reinterpret_cast<short*>(p)!=0) {
                x+=*p++;
                int nodot=*p++;
                unsigned char* data=p;
                p+=2*nodot;
                const int end=x+nodot;
                int draw_x=x;
                int source_offset=0;
                x=end;
                if (draw_x<g_vidViewXMinRetail) {
                    source_offset=g_vidViewXMinRetail-draw_x;
                    nodot-=source_offset;
                    draw_x=g_vidViewXMinRetail;
                }
                if (end>g_vidViewXMaxRetail)
                    nodot=g_vidViewXMaxRetail-draw_x;
                for (int i=0;i<nodot;++i) {
                    const int dst=draw_x+i;
                    if (shiftz>=static_cast<int>(z_buffer[dst])) {
                        z_buffer[dst]=static_cast<unsigned short>(shiftz);
                        video[dst].color=*reinterpret_cast<unsigned short*>(data+2*(source_offset+i));
                    }
                }
            }
            p+=2;
            video+=pitch;
            z_buffer+=z_pitch;
            shiftz+=dz;
        }
    }

    video_tex->UnLock();
    z_tex->UnLock();
}


void VID_SOFTWARE::DrawShadow(const SPRITE* sprite)
{
    struct SHADOWVERTEX {
        float x,y,z,rhw;
        unsigned int diffuse;
        unsigned int specular;
    };
    static_assert(sizeof(SHADOWVERTEX)==0x18,"retail shadow vertex stride");

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    float shadow_z=spr->Z();
    if (!m_cadrs || !m_cadrShift || PropHide())
        return;

    const float shiftx=spr->ScreenX()-static_cast<float>(m_regionTileStepX/2);
    float shifty=spr->ScreenY()-static_cast<float>(m_regionTileStepY/2);
    if (!VID::BoxInViewPort(static_cast<int>(shiftx),static_cast<int>(shifty),
                            static_cast<int>(shiftx)+m_regionTileStepX+200,
                            static_cast<int>(shifty)+m_regionTileStepY+100))
        return;

    shifty+=shadow_z;
    if (PropWave())
        shadow_z=FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset+shadow_z;

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int no_dot=*reinterpret_cast<short*>(p);
    p+=2;
    if (!no_dot)
        return;

    SHADOWVERTEX pnt[514];
    int i=0;
    for (;i<2*no_dot;i+=2) {
        pnt[i].x=static_cast<float>(*reinterpret_cast<short*>(p))+shiftx; p+=2;
        const float y=static_cast<float>(*reinterpret_cast<short*>(p))+shifty; p+=2;
        const float z=static_cast<float>(*reinterpret_cast<short*>(p))+shadow_z; p+=2;
        pnt[i].y=y-z;
        pnt[i].z=z*0.0001220703125f+0.015625f;
        pnt[i].rhw=1.0f;
        pnt[i].diffuse=0x00A4A4A4u;
        pnt[i].specular=0;
        pnt[i+1].x=z*0.35f+pnt[i].x;
        pnt[i+1].y=y-z*0.70f;
        pnt[i+1].z=0.015625f;
        pnt[i+1].rhw=1.0f;
        pnt[i+1].diffuse=0x00A4A4A4u;
        pnt[i+1].specular=0;
    }
    pnt[i]=pnt[0];
    pnt[i+1]=pnt[1];

    Graph->SetRenderState(0x1Du,0u); // D3DRS_SPECULARENABLE
    void* const device=Graph->D3DDevice();
    if (device) {
        void** const vtable=*reinterpret_cast<void***>(device);
        typedef long (__stdcall *SetTextureFn)(void*,unsigned long,void*);
        reinterpret_cast<SetTextureFn>(vtable[0x8C/4])(device,0u,0);
    }
    Graph->SetRenderState(0x16u,3u); // D3DRS_CULLMODE = D3DCULL_CCW
    Graph->SetAlphaBlend(1u,3u);
    const int total=2*no_dot+2;
    for (i=0;i<total;++i)
        pnt[i].diffuse=0x00A4A4A4u;
    Graph->DrawPrimitive(5u,0xC4u,pnt,0x18u,total);

    Graph->SetRenderState(0x16u,2u); // D3DCULL_CW
    Graph->SetAlphaBlend(9u,2u);
    for (i=0;i<total;++i)
        pnt[i].diffuse=0x008F8F8Fu;
    Graph->DrawPrimitive(5u,0xC4u,pnt,0x18u,total);
    Graph->SetRenderState(0x16u,3u);
}

// Hardware-Z owner: builds the temporary A4R4G4B4 alpha/light mask against
// the software Z buffer, then composites that mask through GRAPH::AlphaBuffer.
void VID_HARDWARE_Z::Draw(const SPRITE* sprite)
{
    if (PropHide())
        return;

    SPRITE* const spr=const_cast<SPRITE*>(sprite);
    const int width=static_cast<int>(m_regionTileStepX);
    const int height=static_cast<int>(m_regionTileStepY);
    int shiftx=static_cast<int>(spr->ScreenX()-static_cast<float>(width/2));
    int shifty=static_cast<int>(spr->ScreenY()-static_cast<float>(height/2));
    if (!VID::BoxInViewPort(shiftx,shifty,shiftx+width,shifty+height))
        return;

    int shiftz=static_cast<int>(spr->Z()*8.0f);
    if (PropAlwaysTop() && shiftz<0x3FFF) {
        shiftz+=0x3FFF;
    } else if (PropWave()) {
        const int wavez=static_cast<int>(FSin[(CurrentTime>>3)&0xFFu]*m_groundOffset*8.0f);
        shiftz+=wavez;
        shifty-=wavez/8;
    }

    unsigned char* p=m_cadrs+m_cadrShift[spr->CurrentCadr()];
    const int contours=*reinterpret_cast<short*>(p);
    p+=2+6*contours;
    int y=shifty+*reinterpret_cast<short*>(p); p+=2;
    const int rows=*reinterpret_cast<short*>(p); p+=2;
    int maxy=y+rows;
    if (y>=g_vidViewYMaxRetail || maxy<g_vidViewYMinRetail)
        return;
    if (maxy>g_vidViewYMaxRetail)
        maxy=g_vidViewYMaxRetail;

    if (y<g_vidViewYMinRetail) {
        while (y<g_vidViewYMinRetail) {
            while (*reinterpret_cast<short*>(p)!=0)
                p+=4*static_cast<unsigned int>(p[1])+2u;
            p+=2;
            ++y;
        }
    }

    RECT_OLD screenRect;
    screenRect.left=shiftx;
    screenRect.top=y;
    screenRect.right=shiftx+width;
    screenRect.bottom=maxy;

    RECT_OLD texRect;
    texRect.left=0;
    texRect.top=0;
    texRect.right=width;
    texRect.bottom=maxy-y;

    int zPitch=0;
    unsigned short* zBuffer=Graph->LockZ(&zPitch);
    TEXTURE* alpha=Graph->AlphaBuffer();
    int alphaPitchBytes=0;
    unsigned short* alphaPixels=reinterpret_cast<unsigned short*>(alpha->Lock(&alphaPitchBytes,&texRect));
    const int alphaPitch=alphaPitchBytes/2;
    g_softwareRasterZ[0]=static_cast<short>(shiftz);

    unsigned short* alphaRow=alphaPixels;
    // Retail 0x4398AA computes the end pointer as
    //   alphaPixels + alphaPitch * ((maxy - y) + 1).
    // RECT_OLD bottom is inclusive in this owner, so the packed raster loop
    // processes the final scanline too.  Using `y < maxy` drops that row and
    // desynchronizes the RLE/Z-light mask from the retail renderer.
    unsigned short* const alphaEnd=alphaPixels+alphaPitch*((maxy-y)+1);
    unsigned short* zRow=zBuffer+y*zPitch;
    const int alphaPath=IsAlphaType() && IsTextureType();
    while (alphaRow<alphaEnd) {
        memset(alphaRow,0,static_cast<unsigned int>(alphaPitchBytes));
        int x=shiftx;
        while (*reinterpret_cast<short*>(p)!=0) {
            x+=*p++;
            const int count=*p++;
            unsigned char* const zdata=p;
            unsigned char* const data=p+2*count;
            p+=4*count;
            const int runEnd=x+count;
            int drawX=x;
            int drawEnd=runEnd;
            if (drawX<g_vidViewXMinRetail)
                drawX=g_vidViewXMinRetail;
            if (drawEnd>g_vidViewXMaxRetail)
                drawEnd=g_vidViewXMaxRetail;
            if (drawEnd>drawX) {
                const int sourceOffset=drawX-x;
                if (alphaPath) {
                    DrawAlphaSpanWithZ(zdata+2*sourceOffset,
                                      data+2*sourceOffset,
                                      zRow+drawX,
                                      alphaRow+(drawX-shiftx),
                                      drawEnd-drawX);
                } else {
                    DrawLightSpanWithZ(zdata+2*sourceOffset,
                                      data+2*sourceOffset,
                                      zRow+drawX,
                                      alphaRow+(drawX-shiftx),
                                      drawEnd-drawX);
                }
            }
            x=runEnd;
        }
        p+=2;
        alphaRow+=alphaPitch;
        zRow+=zPitch;
        ++y;
    }

    Graph->SetAlphaBlend(5u,alphaPath ? 6u : 2u);
    alpha->UnLock();
    Graph->SetRenderState(0x17u,8u); // D3DRS_ZFUNC / D3DCMP_ALWAYS

    GAMMA spriteGamma=spr->GetGamma();
    GAMMA* const vidGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x2D8);
    GAMMA drawGamma=(*vidGamma)+&spriteGamma;
    if (!PropGamma()) {
        GAMMA graphGamma=Graph->GetGamma();
        drawGamma+=&graphGamma;
    }
    alpha->Draw(&screenRect,&texRect,&drawGamma);
}

void VID::SetAltGammaType()
{
    m_extraTypeFlags=static_cast<unsigned short>(m_extraTypeFlags|0x0400u);
}

void VID_SOFTWARE::SetGamma(const GAMMA* gamma,unsigned int n_gamma)
{
    const int paletteSize=PaletteSize();
    if (!m_cadrs || !IsPaletteType())
        return;

    GAMMA* const storedGamma=reinterpret_cast<GAMMA*>(VidRaw(this)+0x3E8);

    if (n_gamma==4u) {
        if (IsAltGammaType()) {
            SetGamma(&storedGamma[0],0u);
            SetGamma(&storedGamma[1],1u);
            SetGamma(&storedGamma[2],2u);
            SetGamma(&storedGamma[3],3u);
        } else {
            memcpy(m_cadrs,m_cadrs+paletteSize,static_cast<size_t>(paletteSize));
            if (m_spriteClass!=8u && !PropGamma())
                SetGammaToPalette(m_cadrs,gamma);
        }
        return;
    }

    if (n_gamma>=4u) {
        Error(4,const_cast<char*>("n_gamma in VID_SOFTWARE::SetGamma"),n_gamma);
        return;
    }

    storedGamma[n_gamma].operator=(gamma);

    if (!IsAltGammaType()) {
        unsigned char* const oldCadrs=m_cadrs;
        m_cadrSize+=3*paletteSize;
        g_vidMemoryInUse+=3*paletteSize;
        m_cadrs=static_cast<unsigned char*>(operator new(static_cast<size_t>(m_cadrSize)));
        if (!m_cadrs) {
            Error(2,const_cast<char*>("SetGamma"),static_cast<unsigned long>(m_cadrSize));
            return;
        }

        memcpy(m_cadrs+4*paletteSize,
               oldCadrs+paletteSize,
               static_cast<size_t>(m_cadrSize-4*paletteSize));
        memcpy(m_cadrs,m_cadrs+4*paletteSize,static_cast<size_t>(paletteSize));
        memcpy(m_cadrs+paletteSize,m_cadrs+4*paletteSize,static_cast<size_t>(paletteSize));
        memcpy(m_cadrs+2*paletteSize,m_cadrs,static_cast<size_t>(2*paletteSize));
        operator delete(oldCadrs);

        if (m_cadrShift) {
            for (int i=0;i<m_dotFrameCount;++i)
                m_cadrShift[i]+=3*paletteSize;
        }

        SetAltGammaType();
        for (VID* mirror=m_mirrorNext;mirror!=this;mirror=mirror->m_mirrorNext) {
            mirror->SetAltGammaType();
            static_cast<VID_SOFTWARE*>(mirror)->m_cadrs=m_cadrs;
        }
    }

    memcpy(m_cadrs+static_cast<int>(n_gamma)*paletteSize,
           m_cadrs+4*paletteSize,
           static_cast<size_t>(paletteSize));

    unsigned char* const target=m_cadrs+static_cast<int>(n_gamma)*paletteSize;
    if (m_spriteClass==8u || PropGamma()) {
        SetGammaToPalette(target,gamma);
    } else {
        GAMMA graphGamma=Graph->GetGamma();
        GAMMA sum=const_cast<GAMMA*>(gamma)->operator+(&graphGamma);
        SetGammaToPalette(target,&sum);
    }
}


void VID::SetHpCoeff(int army,int hpPercent)
{
    army&=3;
    const int oldMax=m_maxHp[army];
    if (hpPercent>=0) m_maxHp[army]=hpPercent*m_baseHp/100;
    if (m_baseHp) {
        int iter=0;
        for (SPRITE* spr=Map->FirstSprite(m_layer,&iter);spr;spr=Map->NextSprite(m_layer,&iter)) {
            if (spr->Vid()==this && spr->Army()==army && oldMax)
                spr->ChangeHp(spr->Hp()*m_maxHp[army]/oldMax);
        }
    }
    if (m_linkVid) m_linkVid->SetHpCoeff(army,hpPercent);
}

void VID::SetMaxHp(int army,int newHp)
{
    army&=3;
    const int oldMax=m_maxHp[army];
    if (newHp>=0) m_maxHp[army]=newHp;
    if (m_baseHp) {
        int iter=0;
        for (SPRITE* spr=Map->FirstSprite(m_layer,&iter);spr;spr=Map->NextSprite(m_layer,&iter)) {
            if (spr->Vid()==this && spr->Army()==army && oldMax)
                spr->ChangeHp(spr->Hp()*m_maxHp[army]/oldMax);
        }
    }
    if (m_linkVid) m_linkVid->SetMaxHp(army,newHp);
}
int VID::Killed(int army) { return m_killed[army&3]; }
int VID::Killed() { return m_killed[0]+m_killed[1]+m_killed[2]+m_killed[3]; }
int VID::ReColored(int army) { return m_reColored[army&3]; }
int VID::ReColored() { return m_reColored[0]+m_reColored[1]+m_reColored[2]+m_reColored[3]; }
void VID::SetPropNotCreateAsChild(int on) { m_prop=static_cast<uint32_t>(on); }
