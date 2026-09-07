#pragma once
// CONSTANT owner. Included in ABI order by mapedit/runtime.hpp.
struct CONSTANT {
    explicit CONSTANT(RESOURCE* res);
    union { float MaxScrollSpeedX; float maxShiftSpeedX; };             // +0x00
    union { float MaxScrollSpeedY; float maxShiftSpeedY; };             // +0x04
    union { float Gravitation; float gravity; };                         // +0x08
    union { float Gravitation2; float gravity2; };                       // +0x0C
    int RepairSpeed;                                                     // +0x10
    int AmmoReloadTime;                                                  // +0x14
    float RailRepairSpeed;                                               // +0x18
    float MasterRepairSpeed;                                             // +0x1C
    float PatrolRadius;                                                   // +0x20
    union { int DepoMillisecondsInSecond; unsigned int buildTimeScale; };// +0x24
    union { int DebugMode; int debugMode; };                             // +0x28
    int DepoAutoRepairTimeInSeconds;                                     // +0x2C
    int MasterAutoRepairTimeInSeconds;                                   // +0x30
    int MouseTipsTime;                                                   // +0x34
    int DepoAutoAddHpPerSecond;                                          // +0x38
    int MasterAutoAddHpPerSecond;                                        // +0x3C
    int FortCannonsAutoAddHpPerSecond;                                   // +0x40
    int RepairSettingMineTime;                                           // +0x44
    int RepairDestroyingMineTime;                                        // +0x48
    int DirijbanAmmoReloadTime;                                          // +0x4C
    int SelectUnitGamma;                                                  // +0x50
    int AttackUnitGamma;                                                  // +0x54
    int LightedUnitGamma;                                                 // +0x58
    int NukeForBirth;                                                     // +0x5C
    float SafeClashSpeed;                                                 // +0x60
    int MessageStartDelay;                                                // +0x64
};
static_assert(offsetof(CONSTANT,MessageStartDelay)==0x64,"CONSTANT MessageStartDelay ABI");
static_assert(sizeof(CONSTANT)==0x68, "MapEdit CONSTANT allocation size");

