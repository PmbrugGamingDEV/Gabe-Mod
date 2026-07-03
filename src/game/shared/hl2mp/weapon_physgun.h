#ifndef WEAPON_PHYSGUN_HELPERS_H
#define WEAPON_PHYSGUN_HELPERS_H

#include "basecombatweapon.h"
#include "baseentity.h"

CBaseEntity *PhysGun_GetHeldEntity( CBaseCombatWeapon *pWeapon );
bool PhysGun_IsHoldingObject(CBaseCombatWeapon* pWeapon);
float PhysGun_GetHoldDistance(CBaseCombatWeapon* pWeapon);
bool PhysGun_IsRotatingObject(CBaseCombatWeapon* pWeapon);
void PhysGun_DropHeldObject(CBaseCombatWeapon* pWeapon);
void PhysGun_ReleaseHeldObject(CBaseCombatWeapon* pWeapon);
int PhysGun_GetFrozenObjectCount(CBaseCombatWeapon* pWeapon);
void PhysGun_ReleaseFrozenObjects(CBaseCombatWeapon* pWeapon);

#endif