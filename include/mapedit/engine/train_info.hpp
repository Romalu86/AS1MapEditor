#pragma once
// TRAIN_INFO owner. Included in ABI order by mapedit/runtime.hpp.
struct TRAIN_INFO {
    unsigned int haveAmmo:1;        // +0x00 bit 0
    unsigned int haveRepair:1;      // +0x00 bit 1
    unsigned int reservedFlags:30;
    float maxBattleRange;           // +0x04
    float minBattleRange;           // +0x08
    float power;                    // +0x0C
    float weight;                   // +0x10
    float trainweight;              // +0x14
    int speed;                      // +0x18
    int no;                         // +0x1C
    int hp;                         // +0x20
    int max_hp;                     // +0x24
    int weapon;                     // +0x28
    int build_time;                 // +0x2C
    int noAmmo;                     // +0x30
    int ammo;                       // +0x34
    int maxAmmo;                    // +0x38
    int percentAmmo;                // +0x3C

    explicit TRAIN_INFO(const ENGINE* eng);
    void AddEngine(const ENGINE* eng);
    int CanMove();
    int HaveAmmo();
    int IsDamaged();
    int NeedAmmo();
    int HaveAmmoWagon() const { return haveAmmo != 0; }
    int HaveRepair() const { return haveRepair != 0; }
    int Acceleration();
};
static_assert(sizeof(TRAIN_INFO)==0x40,"debug metadata TRAIN_INFO size");

// MapEdit.exe PLAYER vtable @ 0x004B536C, 12 slots.
// debug metadata omits member names for this class, but player.obj and the original
// executable prove the retail layout below (sizeof=0x28).
