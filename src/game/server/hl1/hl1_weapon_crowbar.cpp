#include "cbase.h"
#include "basehlcombatweapon_shared.h"
#include "player.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "soundent.h"
#include "decals.h"
#include "vstdlib/random.h"

#include "util.h"              // UTIL_TraceLine / UTIL_TraceHull
#include "takedamageinfo.h"   // CTakeDamageInfo
#include "baseentity.h"       // GetAbsOrigin / entindex
#include "ai_basenpc.h"       // CLASS_MACHINE / CLASS_NONE
#include "basecombatweapon.h"     // TraceAttackToTriggers
#include "engine/IEngineSound.h"

//Crowbar Sounds
ConVar hl1_crowbar_sound("hl1_crowbar_sound", "1");

ConVar hl1_crowbar_concrete("hl1_crowbar_concrete", "1");
ConVar hl1_crowbar_concrete_vol("hl1_crowbar_concrete_vol", "10");

ConVar hl1_crowbar_metal("hl1_crowbar_metal", "1");
ConVar hl1_crowbar_metal_vol("hl1_crowbar_metal_vol", "10");

ConVar hl1_crowbar_dirt("hl1_crowbar_dirt", "1");
ConVar hl1_crowbar_dirt_vol("hl1_crowbar_dirt_vol", "10");

ConVar hl1_crowbar_vent("hl1_crowbar_vent", "1");
ConVar hl1_crowbar_vent_vol("hl1_crowbar_vent_vol", "10");

ConVar hl1_crowbar_grate("hl1_crowbar_grate", "1");
ConVar hl1_crowbar_grate_vol("hl1_crowbar_grate_vol", "10");

ConVar hl1_crowbar_tile("hl1_crowbar_tile", "1");
ConVar hl1_crowbar_tile_vol("hl1_crowbar_tile_vol", "10");

ConVar hl1_crowbar_wood("hl1_crowbar_wood", "1");
ConVar hl1_crowbar_wood_vol("hl1_crowbar_wood_vol", "10");

ConVar hl1_crowbar_glass("hl1_crowbar_glass", "1");
ConVar hl1_crowbar_glass_vol("hl1_crowbar_glass_vol", "10");

ConVar hl1_crowbar_computer("hl1_crowbar_computer", "1");
ConVar hl1_crowbar_computer_vol("hl1_crowbar_computer_vol", "10");

#define	CROWBAR_RANGE		32.0f
#define	CROWBAR_REFIRE_MISS	0.5f
#define	CROWBAR_REFIRE_HIT	0.25f

float GetCrowbarVolume(trace_t& ptr)
{
	if (!hl1_crowbar_sound.GetBool())
		return 0.0f;

	float fvol = 0.0;
	char mType = TEXTURETYPE_Find(&ptr);

	CBaseEntity* pEntity = ptr.m_pEnt;

	switch (mType)
	{
	case CHAR_TEX_CONCRETE:
		if (hl1_crowbar_concrete.GetBool())
			fvol = hl1_crowbar_concrete_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_METAL:
		if (hl1_crowbar_metal.GetBool())
			fvol = hl1_crowbar_metal_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_DIRT:
		if (hl1_crowbar_dirt.GetBool())
			fvol = hl1_crowbar_dirt_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_VENT:
		if (hl1_crowbar_vent.GetBool())
			fvol = hl1_crowbar_vent_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_GRATE:
		if (hl1_crowbar_grate.GetBool())
			fvol = hl1_crowbar_grate_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_TILE:
		if (hl1_crowbar_tile.GetBool())
			fvol = hl1_crowbar_tile_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_WOOD:
		if (hl1_crowbar_wood.GetBool())
			fvol = hl1_crowbar_wood_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_GLASS:
		if (hl1_crowbar_glass.GetBool())
			fvol = hl1_crowbar_glass_vol.GetFloat() / 10;
		break;
	case CHAR_TEX_COMPUTER:
		if (hl1_crowbar_computer.GetBool())
			fvol = hl1_crowbar_computer_vol.GetFloat() / 10;
		break;
	default: fvol = 0.0;
		break;
	}

	if (pEntity && FClassnameIs(pEntity, "func_breakable"))
	{
		fvol /= 2.0;
	}

	return fvol;
}

class CWeaponHL1Crowbar : public CBaseHLCombatWeapon
{
	DECLARE_CLASS(CWeaponHL1Crowbar, CBaseHLCombatWeapon);
public:

	CWeaponHL1Crowbar();

	void			Precache(void);
	virtual void	ItemPostFrame(void);
	void			PrimaryAttack(void);

public:
	trace_t		m_traceHit;
	Activity	m_nHitActivity;

private:
	virtual void		Swing(void);
	virtual	void		Hit(void);
	virtual	void		ImpactEffect(void);
	virtual	void		ImpactSound(bool isWorld, trace_t& hitTrace);
	virtual Activity	ChooseIntersectionPointAndActivity(trace_t& hitTrace, const Vector& mins, const Vector& maxs, CBasePlayer* pOwner);

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
};

IMPLEMENT_SERVERCLASS_ST(CWeaponHL1Crowbar, DT_WeaponHL1Crowbar)
END_SEND_TABLE()
PRECACHE_WEAPON_REGISTER(weapon_crowbar_hl1);

LINK_ENTITY_TO_CLASS(weapon_crowbar_hl1, CWeaponHL1Crowbar);

BEGIN_DATADESC(CWeaponHL1Crowbar)
DEFINE_FUNCTION(Hit),
END_DATADESC()

static const Vector g_bludgeonMins(-16, -16, -16);
static const Vector g_bludgeonMaxs(16, 16, 16);

CWeaponHL1Crowbar::CWeaponHL1Crowbar()
{
	m_bFiresUnderwater = true;
}

void CWeaponHL1Crowbar::Precache(void)
{
	PrecacheSound("weapons/cbar_hit1.wav");
	PrecacheSound("weapons/cbar_hit2.wav");

	BaseClass::Precache();
}

void CWeaponHL1Crowbar::ItemPostFrame(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());

	if (pOwner == NULL)
		return;

	if ((pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		PrimaryAttack();
	}
	else
	{
		WeaponIdle();
		return;
	}
}

void CWeaponHL1Crowbar::PrimaryAttack()
{
	Swing();
}

void CWeaponHL1Crowbar::Hit(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 400, 0.2);
	CSoundEnt::InsertSound(SOUND_BULLET_IMPACT, m_traceHit.endpos, 400, 0.2f, pPlayer);

	CBaseEntity* pHitEntity = m_traceHit.m_pEnt;

	if (pHitEntity != NULL)
	{
		Vector hitDirection;
		pPlayer->EyeVectors(&hitDirection, NULL, NULL);
		VectorNormalize(hitDirection);

		ClearMultiDamage();
		CTakeDamageInfo info(GetOwner(), GetOwner(), 9.0f, DMG_CLUB);
		CalculateMeleeDamageForce(&info, hitDirection, m_traceHit.endpos);
		pHitEntity->DispatchTraceAttack(info, hitDirection, &m_traceHit);
		ApplyMultiDamage();

		TraceAttackToTriggers(CTakeDamageInfo(GetOwner(), GetOwner(), 9.0f, DMG_CLUB), m_traceHit.startpos, m_traceHit.endpos, hitDirection);

		ImpactSound(pHitEntity->IsWorld(), m_traceHit);
	}

	ImpactEffect();
}

void CWeaponHL1Crowbar::ImpactSound(bool isWorld, trace_t& hitTrace)
{
	if (isWorld)
	{
		float flvol = GetCrowbarVolume(hitTrace);
		switch (random->RandomInt(0, 1))
		{
		case 0:
			UTIL_EmitAmbientSound(GetOwner()->entindex(), GetOwner()->GetAbsOrigin(), "weapons/cbar_hit1.wav", flvol, SNDLVL_GUNFIRE, 0, 98 + random->RandomInt(0, 3));
			break;
		case 1:
			UTIL_EmitAmbientSound(GetOwner()->entindex(), GetOwner()->GetAbsOrigin(), "weapons/cbar_hit2.wav", flvol, SNDLVL_GUNFIRE, 0, 98 + random->RandomInt(0, 3));
			break;
		}
	}
	else
	{
		WeaponSound(MELEE_HIT);
	}
}

Activity CWeaponHL1Crowbar::ChooseIntersectionPointAndActivity(trace_t& hitTrace, const Vector& mins, const Vector& maxs, CBasePlayer* pOwner)
{
	int			i, j, k;
	float		distance;
	const float* minmaxs[2] = { mins.Base(), maxs.Base() };
	trace_t		tmpTrace;
	Vector		vecHullEnd = hitTrace.endpos;
	Vector		vecEnd;

	distance = 1e6f;
	Vector vecSrc = hitTrace.startpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace);
	if (tmpTrace.fraction == 1.0)
	{
		for (i = 0; i < 2; i++)
		{
			for (j = 0; j < 2; j++)
			{
				for (k = 0; k < 2; k++)
				{
					vecEnd.x = vecHullEnd.x + minmaxs[i][0];
					vecEnd.y = vecHullEnd.y + minmaxs[j][1];
					vecEnd.z = vecHullEnd.z + minmaxs[k][2];

					UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace);
					if (tmpTrace.fraction < 1.0)
					{
						float thisDistance = (tmpTrace.endpos - vecSrc).Length();
						if (thisDistance < distance)
						{
							hitTrace = tmpTrace;
							distance = thisDistance;
						}
					}
				}
			}
		}
	}
	else
	{
		hitTrace = tmpTrace;
	}


	return ACT_VM_HITCENTER;
}

void CWeaponHL1Crowbar::ImpactEffect(void)
{
	UTIL_ImpactTrace(&m_traceHit, DMG_CLUB);
}

void CWeaponHL1Crowbar::Swing(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition();
	Vector forward;

	pOwner->EyeVectors(&forward, NULL, NULL);

	UTIL_TraceLine(swingStart, swingStart + forward * CROWBAR_RANGE, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &m_traceHit);
	m_nHitActivity = ACT_VM_HITCENTER;

	if (m_traceHit.fraction == 1.0)
	{
		UTIL_TraceHull(swingStart, swingStart + forward * CROWBAR_RANGE, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &m_traceHit);
		if (m_traceHit.fraction < 1.0)
		{
			m_nHitActivity = ChooseIntersectionPointAndActivity(m_traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner);
		}
	}

	if (m_traceHit.fraction == 1.0f)
	{
		m_nHitActivity = ACT_VM_MISSCENTER;

		WeaponSound(SINGLE);

		m_flNextPrimaryAttack = gpGlobals->curtime + CROWBAR_REFIRE_MISS;
	}
	else
	{
		Hit();

		m_flNextPrimaryAttack = gpGlobals->curtime + CROWBAR_REFIRE_HIT;
	}

	SendWeaponAnim(m_nHitActivity);
}