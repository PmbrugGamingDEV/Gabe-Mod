#ifndef WEAPON_RPG_H
#define WEAPON_RPG_H
#ifdef _WIN32
#pragma once
#endif


#include "basehlcombatweapon_shared.h"

#ifdef CLIENT_DLL
#include "iviewrender_beams.h"
#include "c_smoke_trail.h"
#endif

#ifndef CLIENT_DLL
#include "smoke_trail.h"
#include "Sprite.h"
#include "npcevent.h"
#include "beam_shared.h"
#include "hl1_basegrenade.h"

class CHL1WeaponRPG;

class CHL1RpgRocket : public CHL1BaseGrenade
{
	DECLARE_CLASS(CHL1RpgRocket, CHL1BaseGrenade);
	//DECLARE_SERVERCLASS();

public:
	CHL1RpgRocket();

	Class_T Classify(void) { return CLASS_NONE; }

	void Spawn(void);
	void Precache(void);
	void RocketTouch(CBaseEntity* pOther);
	void IgniteThink(void);
	void SeekThink(void);

	virtual void Detonate(void);

	static CHL1RpgRocket* Create(const Vector& vecOrigin, const QAngle& angAngles, CBasePlayer* pentOwner = NULL);

	CHandle<CHL1WeaponRPG>		m_hOwner;
	float					m_flIgniteTime;
	int						m_iTrail;

	DECLARE_DATADESC();
};


#endif

#ifdef CLIENT_DLL
#define CHL1LaserDot C_HL1LaserDot
#endif

class CHL1LaserDot;

#ifdef CLIENT_DLL
#define CHL1WeaponRPG C_HL1WeaponRPG
#endif

class CHL1WeaponRPG : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CHL1WeaponRPG, CBaseHLCombatWeapon);
public:

	CHL1WeaponRPG(void);
	~CHL1WeaponRPG();

	void	ItemPostFrame(void);
	void	Precache(void);
	bool	Deploy(void);
	void	PrimaryAttack(void);
	void	WeaponIdle(void);
	bool	Holster(CBaseCombatWeapon* pSwitchingTo = NULL);
	void	NotifyRocketDied(void);
	bool	Reload(void);

	void	Drop(const Vector& vecVelocity);

	virtual int	GetDefaultClip1(void) const;

	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

private:
	void	CreateLaserPointer(void);
	void	UpdateSpot(void);
	bool	IsGuiding(void);
	void	StartGuiding(void);
	void	StopGuiding(void);

	CNetworkVar(bool, m_bGuiding);
	CNetworkVar(bool, m_bIntialStateUpdate);
	CNetworkVar(bool, m_bLaserDotSuspended);
	CNetworkVar(float, m_flLaserDotReviveTime);

	CNetworkHandle(CBaseEntity, m_hMissile);

#ifndef CLIENT_DLL
	CHandle<CHL1LaserDot>	m_hLaserDot;
#endif
};


#endif