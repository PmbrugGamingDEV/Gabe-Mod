#pragma once

#include "cbase.h"
#include "basehlcombatweapon.h"

class CStickyPellet;

class CWeaponStickyLauncher : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponStickyLauncher, CBaseHLCombatWeapon);
	DECLARE_SERVERCLASS();
	DECLARE_ACTTABLE();

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual void Precache();
	virtual void Spawn();
	virtual void ItemPostFrame();

	bool m_bFireOnRelease;

	CWeaponStickyLauncher() { m_bFireOnRelease = false; m_bFiresUnderwater = true; }

	CUtlVector<CHandle<CStickyPellet>> m_Pellets;
};
