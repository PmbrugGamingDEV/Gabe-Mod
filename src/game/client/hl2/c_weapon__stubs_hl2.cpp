//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "c_weapon__stubs.h"
#include "basehlcombatweapon_shared.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "c_basehlcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

STUB_WEAPON_CLASS( cycler_weapon, WeaponCycler, C_BaseCombatWeapon );

STUB_WEAPON_CLASS( weapon_binoculars, WeaponBinoculars, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_bugbait, WeaponBugBait, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_flaregun, Flaregun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_annabelle, WeaponAnnabelle, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_gauss, WeaponGaussGun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_cubemap, WeaponCubemap, C_BaseCombatWeapon );
STUB_WEAPON_CLASS( weapon_alyxgun, WeaponAlyxGun, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_citizenpackage, WeaponCitizenPackage, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_citizensuitcase, WeaponCitizenSuitcase, C_WeaponCitizenPackage );

/* Gabe Mod */

STUB_WEAPON_CLASS(weapon_hax, WeaponHax, C_BaseHLCombatWeapon);
STUB_WEAPON_CLASS(weapon_multitool, WeaponMultitool, C_BaseHL2MPCombatWeapon);
STUB_WEAPON_CLASS(weapon_stickylauncher, WeaponStickyLauncher, C_BaseHLCombatWeapon);
STUB_WEAPON_CLASS(weapon_eattach, WeaponEAttach, C_BaseHLCombatWeapon);
STUB_WEAPON_CLASS(weapon_lasergun, WeaponLaserGun, C_BaseHLCombatWeapon);
STUB_WEAPON_CLASS(weapon_smp5, WeaponSMP5, C_BaseHLCombatWeapon);

// Half-Life 1

// Row 0
STUB_WEAPON_CLASS(weapon_crowbar_hl1, WeaponHL1Crowbar, C_BaseHLCombatWeapon); // Spot 0
STUB_WEAPON_CLASS(weapon_egon, WeaponEgon, C_BaseHLCombatWeapon); // Spot 1

// Row 1
STUB_WEAPON_CLASS(weapon_glock, WeaponGlock, C_BaseHLCombatWeapon); // Spot 0

// Row 4
STUB_WEAPON_CLASS(weapon_tripmine, WeaponTripMine, C_BaseHLCombatWeapon); // Spot 1
STUB_WEAPON_CLASS(weapon_snark, WeaponSnark, C_BaseHLCombatWeapon); // Spot 3

/* End Gabe Mod */
#ifndef HL2MP
STUB_WEAPON_CLASS( weapon_ar2, WeaponAR2, C_HLMachineGun );
STUB_WEAPON_CLASS( weapon_frag, WeaponFrag, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_rpg, WeaponRPG, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_pistol, WeaponPistol, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_shotgun, WeaponShotgun, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_smg1, WeaponSMG1, C_HLSelectFireMachineGun );
STUB_WEAPON_CLASS( weapon_357, Weapon357, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_crossbow, WeaponCrossbow, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_slam, Weapon_SLAM, C_BaseHLCombatWeapon );
STUB_WEAPON_CLASS( weapon_crowbar, WeaponCrowbar, C_BaseHLBludgeonWeapon );
#ifdef HL2_EPISODIC
STUB_WEAPON_CLASS( weapon_hopwire, WeaponHopwire, C_BaseHLCombatWeapon );
//STUB_WEAPON_CLASS( weapon_proto1, WeaponProto1, C_BaseHLCombatWeapon );
#endif
#ifdef HL2_LOSTCOAST
STUB_WEAPON_CLASS( weapon_oldmanharpoon, WeaponOldManHarpoon, C_WeaponCitizenPackage );
#endif
#endif


