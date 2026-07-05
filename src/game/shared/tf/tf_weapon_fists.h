#ifndef WEAPON_FISTS_H
#define WEAPON_FISTS_H
#pragma once

#include "hl2mp/weapon_hl2mpbasebasebludgeon.h"

#ifdef CLIENT_DLL
#define CWeaponFists C_WeaponFists
#endif

class CWeaponFists : public CBaseHL2MPBludgeonWeapon
{
public:
	DECLARE_CLASS(CWeaponFists, CBaseHL2MPBludgeonWeapon);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponFists();

	void Spawn() OVERRIDE;
	void Precache() OVERRIDE;

	void PrimaryAttack() OVERRIDE;
	void SecondaryAttack() OVERRIDE;
	void ItemPostFrame() OVERRIDE;
	bool Deploy() OVERRIDE;
	bool Holster(CBaseCombatWeapon* pSwitchingTo) OVERRIDE;

	void Punch(bool bRight);
	void DealDamage();

	void SendPlayerAnimEvent(CBasePlayer* pPlayer);
	void DoViewModelAnimation();

	float GetRange() { return 48.0f; }
	float GetFireRate() { return 0.9f; }

private:
	CNetworkVar(float, m_flNextMeleeAttack);
	CNetworkVar(float, m_flNextIdle);
	CNetworkVar(int, m_iCombo);

	bool m_bRightPunch;
};

#endif