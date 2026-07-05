#include "cbase.h"
#include "tf_weapon_fists.h"

#ifdef CLIENT_DLL
#include "c_baseplayer.h"
#else
#include "player.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponFists, DT_WeaponFists)

BEGIN_NETWORK_TABLE(CWeaponFists, DT_WeaponFists)
#ifdef CLIENT_DLL
RecvPropFloat(RECVINFO(m_flNextMeleeAttack)),
RecvPropFloat(RECVINFO(m_flNextIdle)),
RecvPropInt(RECVINFO(m_iCombo)),
#else
SendPropFloat(SENDINFO(m_flNextMeleeAttack)),
SendPropFloat(SENDINFO(m_flNextIdle)),
SendPropInt(SENDINFO(m_iCombo)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponFists)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_fists, CWeaponFists);
PRECACHE_WEAPON_REGISTER(weapon_fists);

CWeaponFists::CWeaponFists()
{
	m_flNextMeleeAttack = 0.0f;
	m_flNextIdle = 0.0f;
	m_iCombo = 0;
	m_bRightPunch = false;
}

void CWeaponFists::Spawn()
{
	BaseClass::Spawn();
	m_iCombo = 0;
}

void CWeaponFists::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound("WeaponFrag.Throw");
	PrecacheScriptSound("Flesh.ImpactHard");
}

void CWeaponFists::PrimaryAttack()
{
	Punch(false);
}

void CWeaponFists::SecondaryAttack()
{
	Punch(true);
}

void CWeaponFists::Punch(bool bRight)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	m_bRightPunch = bRight;

	pOwner->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
	EmitSound("WeaponFrag.Throw");
#endif

	SendPlayerAnimEvent(pOwner);
	DoViewModelAnimation();

	m_flNextMeleeAttack = gpGlobals->curtime + 0.2f;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.9f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.9f;

	m_flNextIdle = gpGlobals->curtime + 1.0f;
}

void CWeaponFists::ItemPostFrame()
{
	BaseClass::ItemPostFrame();

	if (m_flNextMeleeAttack > 0.0f && gpGlobals->curtime > m_flNextMeleeAttack)
	{
		DealDamage();
		m_flNextMeleeAttack = 0.0f;
	}

	if (gpGlobals->curtime > m_flNextPrimaryAttack + 0.1f)
	{
#ifndef CLIENT_DLL
		m_iCombo = 0;
#endif
	}
}

void CWeaponFists::DealDamage()
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecForward;
	pOwner->EyeVectors(&vecForward);

	Vector vecEnd = vecSrc + vecForward * 48.0f;

	trace_t tr;
	UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tr);

	if (!tr.m_pEnt)
	{
		Vector mins(-10, -10, -8);
		Vector maxs(10, 10, 8);

		UTIL_TraceHull(vecSrc, vecEnd, mins, maxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tr);
	}

#ifndef CLIENT_DLL
	if (tr.DidHit())
	{
		EmitSound("Flesh.ImpactHard");
	}

	CBaseEntity* pHit = tr.m_pEnt;
	if (pHit)
	{
		float flDamage = RandomFloat(8.0f, 12.0f);

		if (m_iCombo >= 2)
		{
			flDamage = RandomFloat(12.0f, 24.0f);
		}

		CTakeDamageInfo info(this, pOwner, flDamage, DMG_CLUB);
		info.SetDamagePosition(tr.endpos);

		Vector vecForce;

		Vector vecForward, vecRight, vecUp;
		pOwner->EyeVectors(&vecForward, &vecRight, &vecUp);

		if (m_iCombo >= 2)
		{
			vecForce = pOwner->GetAbsVelocity() + Vector(0, 0, 5158.0f) + vecForward * 10012.0f;
		}
		else if (m_bRightPunch)
		{
			vecForce = -vecRight * 4912.0f + vecForward * 9989.0f;
		}
		else
		{
			vecForce = vecRight * 4912.0f + vecForward * 9998.0f;
		}

		info.SetDamageForce(vecForce);

		pHit->TakeDamage(info);

		IPhysicsObject* pPhys = pHit->VPhysicsGetObject();
		if (pPhys)
		{
			pPhys->ApplyForceOffset(vecForward * 80.0f * pPhys->GetMass(), tr.endpos);
		}

		if (m_iCombo < 2)
			m_iCombo++;
		else
			m_iCombo = 0;
	}
	else
	{
		m_iCombo = 0;
	}
#endif
}

bool CWeaponFists::Deploy()
{
	m_iCombo = 0;
	m_flNextMeleeAttack = 0.0f;

	bool bResult = BaseClass::Deploy();

	SendWeaponAnim(ACT_VM_DRAW);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
	m_flNextIdle = gpGlobals->curtime + 1.0f;

	return bResult;
}

bool CWeaponFists::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	m_flNextMeleeAttack = 0.0f;
	return BaseClass::Holster(pSwitchingTo);
}

void CWeaponFists::SendPlayerAnimEvent(CBasePlayer* pPlayer)
{
	if (pPlayer)
	{
		pPlayer->SetAnimation( PLAYER_ATTACK1 );
	}
}

void CWeaponFists::DoViewModelAnimation()
{
	if (m_iCombo >= 2)
	{
		SendWeaponAnim(ACT_VM_SWINGHARD);
	}
	else if (m_bRightPunch)
	{
		SendWeaponAnim(ACT_VM_HITRIGHT);
	}
	else
	{
		SendWeaponAnim(ACT_VM_HITLEFT);
	}
}