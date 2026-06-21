// weapon_stickylauncher.cpp
#include "cbase.h"
#include "weapon_stickylauncher.h"
#include "sticky_pellet.h"
#include "basehlcombatweapon.h"
#include "hl2_player.h"
#include "in_buttons.h"
#include "engine/IEngineSound.h"
#include "soundent.h"
#include "gamestats.h"
#include "explode.h"
#include "vstdlib/random.h"
#include "eventlist.h"

IMPLEMENT_SERVERCLASS_ST(CWeaponStickyLauncher, DT_WeaponStickyLauncher)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_stickylauncher, CWeaponStickyLauncher);
PRECACHE_WEAPON_REGISTER(weapon_stickylauncher);

acttable_t CWeaponStickyLauncher::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_PISTOL, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_PISTOL, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_PISTOL, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PISTOL, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_PISTOL, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_PISTOL, false },
};
IMPLEMENT_ACTTABLE(CWeaponStickyLauncher);

void CWeaponStickyLauncher::Precache()
{
	BaseClass::Precache();
	PrecacheModel("models/weapons/w_bugbait.mdl");
	PrecacheScriptSound("Weapon_Pistol.Single");
}

void CWeaponStickyLauncher::Spawn()
{
	BaseClass::Spawn();
	SetModel("models/weapons/w_pistol.mdl");
}

void CWeaponStickyLauncher::ItemPostFrame()
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if (!pOwner)
		return;

	if (pOwner->m_nButtons & IN_ATTACK)
	{
		if (!m_bFireOnRelease && m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			PrimaryAttack();
			m_bFireOnRelease = true;
		}
	}
	else
	{
		m_bFireOnRelease = false;
	}

	BaseClass::ItemPostFrame();
}

void CWeaponStickyLauncher::PrimaryAttack()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming;
	pPlayer->EyeVectors(&vecAiming);

	CStickyPellet* pPellet = static_cast<CStickyPellet*>(CreateEntityByName("sticky_pellet"));
	if (pPellet)
	{
		Vector vecVelocity = vecAiming * 1200.0f;

		pPellet->SetAbsOrigin(vecSrc + vecAiming * 16);
		pPellet->SetAbsAngles(pPlayer->EyeAngles());
		pPellet->SetAbsVelocity(vecVelocity);
		pPellet->SetOwnerEntity(pPlayer);
		pPellet->Spawn();

		trace_t tr;
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 2048, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		WeaponSound(SINGLE);
	    SendWeaponAnim(ACT_VM_PRIMARYATTACK);

		m_Pellets.AddToTail(pPellet);
	}

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
}

void CStickyPellet::DelayedDetonate()
{
	Vector vecOrigin = GetAbsOrigin();

	EmitSound("BaseExplosionEffect.Sound");

	ExplosionCreate(
		vecOrigin + Vector(0, 0, 4),
		QAngle(0, 0, 0),
		GetOwnerEntity(),
		150,
		256,
		true,
		1.0f,
		false,
		true,
		DMG_BLAST
	);

	float flRadius = 256.0f;
	float flForce = 1200.0f;

	for (CBaseEntity* pEnt = gEntList.FirstEnt(); pEnt; pEnt = gEntList.NextEnt(pEnt))
	{
		if (!pEnt->VPhysicsGetObject())
			continue;

		Vector vecDir = pEnt->GetAbsOrigin() - vecOrigin;
		float flDist = vecDir.Length();

		if (flDist > flRadius || flDist <= 1.0f)
			continue;

		VectorNormalize(vecDir);

		float falloff = 1.0f - (flDist / flRadius);

		Vector vecImpulse = vecDir * flForce * falloff;

		pEnt->ApplyAbsVelocityImpulse(vecImpulse); // Strong push
	}

	UTIL_ScreenShake(
		vecOrigin,
		5.0f,   // amplitude
		150.0f, // frequency
		0.75f,  // duration
		256.0f, // radius
		SHAKE_START,
		false    // no air shake
	);

	UTIL_Remove(this);
}


void CWeaponStickyLauncher::SecondaryAttack()
{
#ifndef CLIENT_DLL

	float delay = 0.0f;

	for (int i = 0; i < m_Pellets.Count(); ++i)
	{
		CStickyPellet* pPellet = m_Pellets[i];
		if (!pPellet)
			continue;

		pPellet->SetContextThink(
			&CStickyPellet::DelayedDetonate,
			gpGlobals->curtime + delay,
			"StickyExplode"
		);

		delay += 0.05f; // 50ms spacing
	}

	m_Pellets.Purge();

#endif

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
}
