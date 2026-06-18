//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HL2MP SMG1 with grenade selector
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#else
#include "grenade_ar2.h"
#include "hl2mp_player.h"
#include "basegrenade_shared.h"
#include "npc_combine.h"
#include "prop_combine_ball.h"
#include "grenade_frag.h"
#endif

#include "weapon_hl2mpbase.h"
#include "weapon_hl2mpbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponSMG1 C_WeaponSMG1
#endif

#include "tier0/memdbgon.h"

#ifndef CLIENT_DLL // Server specific.
//extern ConVar gabeplus_deathmatch;
#endif

#define SMG1_GRENADE_DAMAGE 100.0f
#define SMG1_GRENADE_RADIUS 250.0f

#define COMBINE_MIN_GRENADE_CLEAR_DIST 256.0f

#define SMG1_GRENADE_CYCLE_DELAY 0.6f

ConVar smg1_ball_radius("gabe+_smg1_ball_radius", "200", FCVAR_CHEAT);
ConVar smg1_ball_mass("gabe+_smg1_ball_mass", "150", FCVAR_CHEAT);
ConVar smg1_ball_duration("gabe+_smg1_ball_duration", "3.0", FCVAR_CHEAT);
ConVar smg1_frag_damage("gabe+_smg1_frag_damage", "125", FCVAR_CHEAT);
ConVar smg1_frag_radius("gabe+_smg1_frag_radius", "275", FCVAR_CHEAT);
ConVar smg1_frag_timer("gabe+_smg1_frag_timer", "2.5", FCVAR_CHEAT);
ConVar smg1_grenade_delay("gabe+_smg1_grenade_delay", "1.0", FCVAR_CHEAT);

class CWeaponSMG1 : public CHL2MPMachineGun
{
public:
	DECLARE_CLASS(CWeaponSMG1, CHL2MPMachineGun);

	CWeaponSMG1();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void	Precache(void);
	void	AddViewKick(void);
	void	SecondaryAttack(void);
	bool	Reload(void);

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	float	GetFireRate(void) { return 0.075f; }
	int		WeaponRangeAttack2Condition(float flDot, float flDist);
	Activity	GetPrimaryAttackActivity(void);

	virtual const Vector& GetBulletSpread(void)
	{
		static Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	void FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, Vector& vecShootOrigin, Vector& vecShootDir);
	void Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary);
	void Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator);

	const WeaponProficiencyInfo_t* GetProficiencyValues();

	DECLARE_ACTTABLE();

protected:
	int		m_iGrenadeMode;
	float	m_flNextGrenadeModeSwitch;
	float  m_flNextGrenadeCheck;
	Vector m_vecTossVelocity;

private:
	CWeaponSMG1(const CWeaponSMG1&);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSMG1, DT_WeaponSMG1)
BEGIN_NETWORK_TABLE(CWeaponSMG1, DT_WeaponSMG1)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponSMG1)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_smg1, CWeaponSMG1);
PRECACHE_WEAPON_REGISTER(weapon_smg1);

acttable_t	CWeaponSMG1::m_acttable[] =
{
	{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
	{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
	{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
	{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

	{ ACT_WALK,						ACT_WALK_RIFLE,					true },
	{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },

	// Readiness activities (not aiming)
		{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
		{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
		{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

		{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
		{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
		{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

		{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
		{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
		{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

		// Readiness activities (aiming)
			{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims	
			{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
			{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

			{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
			{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
			{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

			{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
			{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
			{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
			//End readiness activities

				{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
				{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
				{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
				{ ACT_RUN,						ACT_RUN_RIFLE,					true },
				{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
				{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
				{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
				{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
				{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
				{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
				{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
				{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
				{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
};

IMPLEMENT_ACTTABLE(CWeaponSMG1);

//=========================================================
CWeaponSMG1::CWeaponSMG1()
{
	m_fMinRange1 = 0;
	m_fMaxRange1 = 1400;

	m_iGrenadeMode = 1;
	m_flNextGrenadeModeSwitch = 0.0f;
	m_flNextGrenadeCheck = 0.0f;
	m_vecTossVelocity = vec3_origin;
}

//-----------------------------------------------------------------------------
// Precache
//-----------------------------------------------------------------------------
void CWeaponSMG1::Precache(void)
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther("grenade_ar2");
	UTIL_PrecacheOther("prop_combine_ball");
	UTIL_PrecacheOther("npc_grenade_frag");
#endif

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Reload (SHIFT + Reload cycles grenade type)
//-----------------------------------------------------------------------------
bool CWeaponSMG1::Reload(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return false;

	// SHIFT + Reload = cycle grenade type
	if ((pPlayer->m_nButtons & IN_RELOAD) &&
		(pPlayer->m_nButtons & IN_SPEED))
	{
		if (gpGlobals->curtime >= m_flNextGrenadeModeSwitch)
		{
			m_iGrenadeMode++;
			if (m_iGrenadeMode > 3)
				m_iGrenadeMode = 1;

#ifndef CLIENT_DLL
			const char* pszName = "AR2";
			if (m_iGrenadeMode == 2) pszName = "ENERGY BALL";
			else if (m_iGrenadeMode == 3) pszName = "FRAG";

			ClientPrint(pPlayer, HUD_PRINTCENTER,
				UTIL_VarArgs("SMG1 GRENADE: %s", pszName));
#endif

			m_flNextGrenadeModeSwitch = gpGlobals->curtime + SMG1_GRENADE_CYCLE_DELAY;
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
		}
		return false;
	}

	// Normal reload
	bool fRet;
	float flCache = m_flNextSecondaryAttack;

	fRet = DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD);
	if (fRet)
	{
		m_flNextSecondaryAttack = GetOwner()->m_flNextAttack = flCache;
		WeaponSound(RELOAD);
	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: view kick
//-----------------------------------------------------------------------------
void CWeaponSMG1::AddViewKick(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

#define EASY_DAMPEN			0.5f
#define MAX_VERTICAL_KICK	1.0f
#define SLIDE_LIMIT			2.0f

	DoMachineGunKick(pPlayer,
		EASY_DAMPEN,
		MAX_VERTICAL_KICK,
		m_fFireDuration,
		SLIDE_LIMIT);
}

//-----------------------------------------------------------------------------
// SecondaryAttack (throw selected grenade)
//-----------------------------------------------------------------------------
void CWeaponSMG1::SecondaryAttack(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	if (pPlayer->GetAmmoCount(m_iSecondaryAmmoType) <= 0)
	{
		SendWeaponAnim(ACT_VM_DRYFIRE);
		WeaponSound(EMPTY);
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
		return;
	}

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecThrow;
	AngleVectors(pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecThrow);
	VectorScale(vecThrow, 1000.0f, vecThrow);

#ifndef CLIENT_DLL
	if (m_iGrenadeMode == 1)
	{
		CGrenadeAR2* pGrenade =
			(CGrenadeAR2*)Create("grenade_ar2", vecSrc, vec3_angle, pPlayer);

		if (pGrenade)
		{
			pGrenade->SetAbsVelocity(vecThrow);
			pGrenade->SetLocalAngularVelocity(RandomAngle(-400, 400));
			pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
			pGrenade->SetThrower(pPlayer);
			pGrenade->SetDamage(SMG1_GRENADE_DAMAGE);
			pGrenade->SetDamageRadius(SMG1_GRENADE_RADIUS);
		}
	}
	else if (m_iGrenadeMode == 2)
	{
		Vector vecVel = vecThrow;
		VectorNormalize(vecVel);
		VectorScale(vecVel, 1100.0f, vecVel);

		// Fire the combine ball (engine-native)
		CreateCombineBall(
			vecSrc,
			vecVel,
			smg1_ball_radius.GetFloat(),
			smg1_ball_mass.GetFloat(),
			smg1_ball_duration.GetFloat(),
			pPlayer
		);
	}
	else
	{
		Vector vecVel = vecThrow;
		VectorNormalize(vecVel);
		VectorScale(vecVel, 1000.0f, vecVel);

		AngularImpulse angVel(RandomFloat(-400, 400),
			RandomFloat(-400, 400),
			RandomFloat(-400, 400));

		// Create frag grenade the Valve way
		CBaseGrenade* pFrag = Fraggrenade_Create(
			vecSrc,                     // position
			vec3_angle,                 // angles
			vecVel,                     // velocity
			angVel,                     // angular velocity
			pPlayer,                    // owner
			smg1_frag_timer.GetFloat(), // fuse time
			false                        // combineSpawned (false = normal frag)
		);

		if (pFrag)
		{
			pFrag->SetDamage(smg1_frag_damage.GetFloat());
			pFrag->SetDamageRadius(smg1_frag_radius.GetFloat());
		}
	}
#endif

	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);
	WeaponSound(WPN_DOUBLE);

/*#ifndef CLIENT_DLL
	if (gabeplus_deathmatch.GetBool())
	{
		pPlayer->RemoveAmmo(1, m_iSecondaryAmmoType);
	}
#endif */

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
	m_flNextSecondaryAttack = gpGlobals->curtime + smg1_grenade_delay.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int
//-----------------------------------------------------------------------------
int CWeaponSMG1::WeaponRangeAttack2Condition(float flDot, float flDist)
{
#ifndef CLIENT_DLL
	CAI_BaseNPC* npcOwner = GetOwner()->MyNPCPointer();

	return COND_NONE;

	// --------------------------------------------------------
	// Assume things haven't changed too much since last time
	// --------------------------------------------------------
	//if (gpGlobals->curtime < m_flNextGrenadeCheck )
		//return m_lastGrenadeCondition;

// -----------------------
// If moving, don't check.
// -----------------------
	if (npcOwner->IsMoving())
		return COND_NONE;

	CBaseEntity* pEnemy = npcOwner->GetEnemy();

	if (!pEnemy)
		return COND_NONE;

	Vector vecEnemyLKP = npcOwner->GetEnemyLKP();
	if (!(pEnemy->GetFlags() & FL_ONGROUND) && pEnemy->GetWaterLevel() == 0 && vecEnemyLKP.z > (GetAbsOrigin().z + WorldAlignMaxs().z))
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to 
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		return COND_NONE;
	}

	// --------------------------------------
	//  Get target vector
	// --------------------------------------
	Vector vecTarget;
	if (random->RandomInt(0, 1))
	{
		// magically know where they are
		vecTarget = pEnemy->WorldSpaceCenter();
	}
	else
	{
		// toss it to where you last saw them
		vecTarget = vecEnemyLKP;
	}
	// vecTarget = m_vecEnemyLKP + (pEnemy->BodyTarget( GetLocalOrigin() ) - pEnemy->GetLocalOrigin());
	// estimate position
	// vecTarget = vecTarget + pEnemy->m_vecVelocity * 2;


	if ((vecTarget - npcOwner->GetLocalOrigin()).Length2D() <= COMBINE_MIN_GRENADE_CLEAR_DIST)
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return (COND_NONE);
	}

	// ---------------------------------------------------------------------
	// Are any friendlies near the intended grenade impact area?
	// ---------------------------------------------------------------------
	CBaseEntity* pTarget = NULL;

	while ((pTarget = gEntList.FindEntityInSphere(pTarget, vecTarget, COMBINE_MIN_GRENADE_CLEAR_DIST)) != NULL)
	{
		//Check to see if the default relationship is hatred, and if so intensify that
		if (npcOwner->IRelationType(pTarget) == D_LI)
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
			return (COND_WEAPON_BLOCKED_BY_FRIEND);
		}
	}

	// ---------------------------------------------------------------------
	// Check that throw is legal and clear
	// ---------------------------------------------------------------------
	// FIXME: speed is based on difficulty...

	Vector vecToss = VecCheckThrow(this, npcOwner->GetLocalOrigin() + Vector(0, 0, 60), vecTarget, 600.0, 0.5);
	if (vecToss != vec3_origin)
	{
		m_vecTossVelocity = vecToss;

		// don't check again for a while.
		// JAY: HL1 keeps checking - test?
		//m_flNextGrenadeCheck = gpGlobals->curtime;
		m_flNextGrenadeCheck = gpGlobals->curtime + 0.3; // 1/3 second.
		return COND_CAN_RANGE_ATTACK2;
	}
	else
	{
		// don't check again for a while.
		m_flNextGrenadeCheck = gpGlobals->curtime + 1; // one full second.
		return COND_WEAPON_SIGHT_OCCLUDED;
	}
#endif

	return 0;
}



//-----------------------------------------------------------------------------
Activity CWeaponSMG1::GetPrimaryAttackActivity(void)
{
	if (m_nShotsFired < 2) return ACT_VM_PRIMARYATTACK;
	if (m_nShotsFired < 3) return ACT_VM_RECOIL1;
	if (m_nShotsFired < 4) return ACT_VM_RECOIL2;
	return ACT_VM_RECOIL3;
}

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::FireNPCPrimaryAttack(CBaseCombatCharacter* pOperator, Vector& vecShootOrigin, Vector& vecShootDir)
{
	// FIXME: use the returned number of bullets to account for >10hz firerate
	WeaponSoundRealtime(SINGLE_NPC);

	CSoundEnt::InsertSound(SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator, SOUNDENT_CHANNEL_WEAPON, pOperator->GetEnemy());
	pOperator->FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
		MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 2, entindex(), 0);

	pOperator->DoMuzzleFlash();
	m_iClip1 = m_iClip1 - 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::Operator_ForceNPCFire(CBaseCombatCharacter* pOperator, bool bSecondary)
{
	// Ensure we have enough rounds in the clip
	m_iClip1++;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment(LookupAttachment("muzzle"), vecShootOrigin, angShootDir);
	AngleVectors(angShootDir, &vecShootDir);
	FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSMG1::Operator_HandleAnimEvent(animevent_t* pEvent, CBaseCombatCharacter* pOperator)
{
	switch (pEvent->event)
	{
	case EVENT_WEAPON_SMG1:
	{
		Vector vecShootOrigin, vecShootDir;
		QAngle angDiscard;

		// Support old style attachment point firing
		if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!pOperator->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
		{
			vecShootOrigin = pOperator->Weapon_ShootPosition();
		}

		CAI_BaseNPC* npc = pOperator->MyNPCPointer();
		ASSERT(npc != NULL);
		vecShootDir = npc->GetActualShootTrajectory(vecShootOrigin);

		FireNPCPrimaryAttack(pOperator, vecShootOrigin, vecShootDir);
	}
	break;
	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}
}
#endif 

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t* CWeaponSMG1::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t table[] =
	{
		{ 7.0, 0.75 },
		{ 5.0, 0.75 },
		{ 10.0 / 3.0, 0.75 },
		{ 5.0 / 3.0, 0.75 },
		{ 1.0, 1.0 },
	};
	return table;
}
