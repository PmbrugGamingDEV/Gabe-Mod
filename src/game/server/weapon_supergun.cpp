#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "soundent.h"
#include "game.h"
#include "beam_shared.h"
#include "util.h"
#include "vstdlib/random.h"
#include "gamerules.h"
#include "engine/IEngineSound.h"
#include "ammodef.h"
#include "basegrenade_shared.h"

// MP5
class CWeaponSMP5 : public CBaseHLCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponSMP5, CBaseHLCombatWeapon);

	CWeaponSMP5();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	void	Precache(void);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon* pSwitchingTo);

	int		GetMinBurst() { return 1; }
	int		GetMaxBurst() { return 1; }
	float	GetFireRate(void) { return 0.01f; }

	Activity	GetPrimaryAttackActivity(void) { return ACT_VM_PRIMARYATTACK; }

	const Vector& GetBulletSpread(void)
	{
		static Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	void	AddViewKick(void);

private:
	CWeaponSMP5(const CWeaponSMP5&);
};

// Networking macros
IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSMP5, DT_WeaponSMP5)

BEGIN_NETWORK_TABLE(CWeaponSMP5, DT_WeaponSMP5)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponSMP5)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_smp5, CWeaponSMP5);
PRECACHE_WEAPON_REGISTER(weapon_smp5);

// Constructor
CWeaponSMP5::CWeaponSMP5()
{
	m_fMinRange1 = 0;
	m_fMaxRange1 = 1400;
	m_fMinRange2 = 0;
	m_fMaxRange2 = 200;
	m_bFiresUnderwater = false;
}

// Precache models and sounds
void CWeaponSMP5::Precache(void)
{
	PrecacheModel("models/weapons/v_smg1.mdl");
	PrecacheModel("models/weapons/w_smg1.mdl");
	PrecacheScriptSound("Weapon_SMG1.Single");
	BaseClass::Precache();
}

void CWeaponSMP5::PrimaryAttack(void)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	WeaponSound(SINGLE);
	pOwner->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pOwner->SetAnimation(PLAYER_ATTACK1);

	Vector vecAiming = pOwner->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	FireBulletsInfo_t info;
	info.m_iShots = 1;
	info.m_vecSrc = pOwner->Weapon_ShootPosition();
	info.m_vecDirShooting = vecAiming;
	info.m_vecSpread = GetBulletSpread();
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_flDamage = 8.0f;

	pOwner->FireBullets(info);

	// ---- Muzzle position logic ----
	Vector vecSrc;
	QAngle angSrc;

	// First-person local player
	if (pOwner->IsPlayer())
	{
		CBaseViewModel* vm = pOwner->GetViewModel();
		if (vm)
		{
			int vmAttach = vm->LookupAttachment("muzzle");
			if (vmAttach > 0)
				vm->GetAttachment(vmAttach, vecSrc, angSrc);
			else
				vecSrc = pOwner->Weapon_ShootPosition();
		}
	}
	else
	{
		// Third-person / remote players
		int iAttachment = LookupAttachment("muzzle");
		if (iAttachment > 0)
			GetAttachment(iAttachment, vecSrc, angSrc);
		else
			vecSrc = pOwner->Weapon_ShootPosition();
	}

	// Trace to hit point
	Vector end = vecSrc + vecAiming * MAX_TRACE_LENGTH;
	trace_t tr;
	UTIL_TraceLine(vecSrc, end, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

	// Bullet travel time
	const float BULLET_SPEED = 11000.0f;
	float dist = (tr.endpos - vecSrc).Length();
	float life = dist / BULLET_SPEED;
	life = clamp(life, 0.03f, 0.30f);

	// Recoil
	AddViewKick();

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.01f;
}

void CWeaponSMP5::SecondaryAttack()
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	Vector vecSrc = pOwner->Weapon_ShootPosition();
	Vector vecAiming;
	pOwner->EyeVectors(&vecAiming);

	// Launch velocity tweak this value
	Vector vecVelocity = vecAiming * 1000.0f;

	QAngle angThrow;
	VectorAngles(vecVelocity, angThrow);

	// Spawn a grenade_frag
	CBaseGrenade* pGrenade = (CBaseGrenade*)Create("grenade_ar2", vecSrc, angThrow, pOwner);
	if (pGrenade)
	{
		pGrenade->SetAbsVelocity(vecVelocity);
		pGrenade->SetLocalAngularVelocity(QAngle(random->RandomFloat(-600, 600),
			random->RandomFloat(-600, 600),
			random->RandomFloat(-600, 600)));

		pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
		pGrenade->SetThrower(pOwner);

		// Optional: Set fuse time (seconds before detonation)
		pGrenade->SetNextThink(gpGlobals->curtime + 3.0f);
		pGrenade->SetThink(&CBaseGrenade::Detonate);
	}

	WeaponSound(WPN_DOUBLE); // Change to your own sound if needed
	SendWeaponAnim(ACT_VM_SECONDARYATTACK);
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
}

// Small view punch for recoil
void CWeaponSMP5::AddViewKick(void)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	QAngle punch;
	punch.x = random->RandomFloat(0.25f, 0.5f);
	punch.y = random->RandomFloat(-0.6f, 0.6f);
	punch.z = 0;

	pPlayer->ViewPunch(punch);
}

bool CWeaponSMP5::Deploy(void)
{
	return BaseClass::Deploy();
}

bool CWeaponSMP5::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	return BaseClass::Holster(pSwitchingTo);
}
