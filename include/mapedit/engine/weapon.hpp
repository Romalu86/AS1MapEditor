#pragma once
// WEAPON owner. Included in ABI order by mapedit/runtime.hpp.
class WEAPON {
public:
    uint32_t m_attackMask;       // +0x00 SpriteType; target VID type-mask
    uint32_t m_property;         // +0x04 Property; PropInTurn / PropSelfDirecting flags
    float m_length;              // +0x08 Length
    float m_weight;              // +0x0C Weight
    float m_power;               // +0x10 Power; train command-all gate
    float m_detectRange;         // +0x14 DetectRange
    float m_battleRange;         // +0x18 BattleRange
    float m_targetRadius;        // +0x1C WeaponAim; random target radius in CreateChild
    int m_reloadTime;           // +0x20 ReloadTime
    int m_buildTime;            // +0x24 BuildTime
    int m_maxAmmo;              // +0x28 MaxAmmo
    int m_army;                 // +0x2C DefaultArmy
    int m_defaultBehave;        // +0x30 DefaultBehave
    int m_icon;                 // +0x34 Icon
    int m_enemyRating;          // +0x38 EnemyRating
    float m_deadZone;           // +0x3C DeadZone
    unsigned long m_period;     // +0x40 Period
    int PropInTurn() const;
    int PropSelfDirecting() const;
    int PropFrontEye() const;
    int PropRandomTarget() const;
    int PropAttackAnyArmy() const;
    int PropAttackNearOnly() const;
    int PropAnyDirFire() const;
    int PropMoved() const;
    float Interpolate(float* data,float coeff);
    int Interpolate(int* data,float coeff);
};
static_assert(offsetof(WEAPON,m_attackMask)==0x00, "WEAPON attack-mask ABI");
static_assert(offsetof(WEAPON,m_property)==0x04, "WEAPON property ABI");
static_assert(offsetof(WEAPON,m_power)==0x10, "WEAPON power ABI");
static_assert(offsetof(WEAPON,m_targetRadius)==0x1C, "WEAPON target-radius ABI");
static_assert(offsetof(WEAPON,m_army)==0x2C, "WEAPON army ABI");
static_assert(offsetof(WEAPON,m_enemyRating)==0x38, "WEAPON enemy-rating ABI");
static_assert(offsetof(WEAPON,m_deadZone)==0x3C, "WEAPON dead-zone ABI");
static_assert(offsetof(WEAPON,m_period)==0x40, "WEAPON period ABI");

