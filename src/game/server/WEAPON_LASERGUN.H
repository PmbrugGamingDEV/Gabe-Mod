#ifndef WEAPON_LASERGUN_H
#define WEAPON_LASERGUN_H

#include "basehlcombatweapon.h"

class CWeaponLaserGun : public CBaseHLCombatWeapon
{
public:
    DECLARE_CLASS(CWeaponLaserGun, CBaseHLCombatWeapon);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CWeaponLaserGun();

    void Precache();
    void PrimaryAttack();

private:

    CNetworkVar(bool, m_bLaserActive);
    CSoundPatch* m_pLaserLoop;

    void FireLaser();
    bool Holster(CBaseCombatWeapon* pSwitchingTo);
    void ItemPostFrame();
};

#endif