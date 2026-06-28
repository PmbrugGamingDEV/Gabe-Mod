#include "cbase.h"
#include "beam_shared.h"
#include "basegrenade_shared.h"

extern ConVar sk_plr_dmg_tripmine;

class CMonsterTripmineGrenade : public CBaseGrenade
{
	DECLARE_CLASS(CMonsterTripmineGrenade, CBaseGrenade);
public:
	CMonsterTripmineGrenade();

	void Spawn(void);
	void Precache(void);

	int OnTakeDamage_Alive(const CTakeDamageInfo& inputInfo);

	void WarningThink(void);
	void PowerupThink(void);
	void BeamBreakThink(void);
	void DelayDeathThink(void);
	void Event_Killed(const CTakeDamageInfo& inputInfo);

	void MakeBeam(void);
	void KillBeam(void);

	float		m_flPowerUp;
	Vector		m_vecDir;
	Vector		m_vecEnd;
	float		m_flBeamLength;

	CHandle<CBaseEntity>	m_pRealOwner;
	CHandle<CBeam>			m_hBeam;

	CHandle<CBaseEntity>	m_hStuckOn;
	Vector					m_posStuckOn;
	QAngle					m_angStuckOn;

	int						m_iLaserModel;

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS(monster_tripmine, CMonsterTripmineGrenade);

BEGIN_DATADESC(CMonsterTripmineGrenade)
DEFINE_FIELD(m_flPowerUp, FIELD_TIME),
DEFINE_FIELD(m_vecDir, FIELD_VECTOR),
DEFINE_FIELD(m_vecEnd, FIELD_POSITION_VECTOR),
DEFINE_FIELD(m_flBeamLength, FIELD_FLOAT),
DEFINE_FIELD(m_pRealOwner, FIELD_EHANDLE),
DEFINE_FIELD(m_hStuckOn, FIELD_EHANDLE),
DEFINE_FIELD(m_posStuckOn, FIELD_POSITION_VECTOR),
DEFINE_FIELD(m_angStuckOn, FIELD_VECTOR),
DEFINE_THINKFUNC(WarningThink),
DEFINE_THINKFUNC(PowerupThink),
DEFINE_THINKFUNC(BeamBreakThink),
DEFINE_THINKFUNC(DelayDeathThink),
END_DATADESC()

CMonsterTripmineGrenade::CMonsterTripmineGrenade()
{
	m_vecDir.Init();
	m_vecEnd.Init();
}

void CMonsterTripmineGrenade::Spawn(void)
{
	Precache();
	SetMoveType(MOVETYPE_FLY);
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_SOLID);
	SetModel("models/w_tripmine.mdl");

	SetCollisionGroup(COLLISION_GROUP_WEAPON);

	SetCycle(0);
	SetSequence(SelectWeightedSequence(ACT_TRIPMINE_WORLD));
	ResetSequenceInfo();
	m_flPlaybackRate = 0;

	UTIL_SetSize(this, Vector(-8, -8, -8), Vector(8, 8, 8));

	m_flDamage = sk_plr_dmg_tripmine.GetFloat();
	m_DmgRadius = m_flDamage * 2.5;

	if (m_spawnflags & 1)
	{
		m_flPowerUp = gpGlobals->curtime + 1.0;
	}
	else
	{
		m_flPowerUp = gpGlobals->curtime + 2.5;
	}

	SetThink(&CMonsterTripmineGrenade::PowerupThink);
	SetNextThink(gpGlobals->curtime + 0.2);

	m_takedamage = DAMAGE_YES;

	m_iHealth = 1;

	if (GetOwnerEntity() != NULL)
	{
		EmitSound("MonsterTripmineGrenade.Deploy");
		EmitSound("MonsterTripmineGrenade.Charge");

		m_pRealOwner = GetOwnerEntity();
	}
	AngleVectors(GetAbsAngles(), &m_vecDir);
	m_vecEnd = GetAbsOrigin() + m_vecDir * MAX_TRACE_LENGTH;
}

void CMonsterTripmineGrenade::Precache(void)
{
	PrecacheModel("models/w_tripmine.mdl");
	m_iLaserModel = PrecacheModel("sprites/laserbeam.vmt");

	PrecacheScriptSound("MonsterTripmineGrenade.Deploy");
	PrecacheScriptSound("MonsterTripmineGrenade.Charge");
	PrecacheScriptSound("MonsterTripmineGrenade.Activate");

}

void CMonsterTripmineGrenade::WarningThink(void)
{
	SetThink(&CMonsterTripmineGrenade::PowerupThink);
	SetNextThink(gpGlobals->curtime + 1.0);
}

void CMonsterTripmineGrenade::PowerupThink(void)
{
	if (m_hStuckOn == NULL)
	{
		trace_t tr;
		CBaseEntity* pOldOwner = GetOwnerEntity();

		UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + m_vecDir * 32, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &tr);

		if (tr.m_pEnt && pOldOwner &&
			(tr.m_pEnt == pOldOwner) && pOldOwner->IsPlayer())
		{
			m_flPowerUp += 0.1;
			SetNextThink(gpGlobals->curtime + 0.1f);
			return;
		}

		SetOwnerEntity(NULL);

		UTIL_TraceLine(GetAbsOrigin() + m_vecDir * 8, GetAbsOrigin() - m_vecDir * 32, MASK_SHOT, pOldOwner, COLLISION_GROUP_NONE, &tr);

		if (tr.startsolid)
		{
			SetOwnerEntity(pOldOwner);
			m_flPowerUp += 0.1;
			SetNextThink(gpGlobals->curtime + 0.1f);
			return;
		}
		if (tr.fraction < 1.0)
		{
			SetOwnerEntity(tr.m_pEnt);
			m_hStuckOn = tr.m_pEnt;
			m_posStuckOn = m_hStuckOn->GetAbsOrigin();
			m_angStuckOn = m_hStuckOn->GetAbsAngles();
		}
		else
		{
			StopSound("MonsterTripmineGrenade.Deploy");
			StopSound("MonsterTripmineGrenade.Charge");
			SetThink(&CBaseEntity::SUB_Remove);
			SetNextThink(gpGlobals->curtime + 0.1f);
			KillBeam();
			return;
		}
	}
	else if ((m_posStuckOn != m_hStuckOn->GetAbsOrigin()) || (m_angStuckOn != m_hStuckOn->GetAbsAngles()))
	{

		StopSound("MonsterTripmineGrenade.Deploy");
		StopSound("MonsterTripmineGrenade.Charge");
		CBaseEntity* pMine = Create("weapon_tripmine", GetAbsOrigin() + m_vecDir * 24, GetAbsAngles());
		pMine->AddSpawnFlags(SF_NORESPAWN);

		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(gpGlobals->curtime + 0.1f);
		KillBeam();
		return;
	}

	if (gpGlobals->curtime > m_flPowerUp)
	{
		MakeBeam();
		RemoveSolidFlags(FSOLID_NOT_SOLID);

		m_bIsLive = true;

		EmitSound("MonsterTripmineGrenade.Activate");
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CMonsterTripmineGrenade::KillBeam(void)
{
	if (m_hBeam)
	{
		UTIL_Remove(m_hBeam);
		m_hBeam = NULL;
	}
}

void CMonsterTripmineGrenade::MakeBeam(void)
{
	trace_t tr;

	UTIL_TraceLine(GetAbsOrigin(), m_vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

	m_flBeamLength = tr.fraction;

	SetThink(&CMonsterTripmineGrenade::BeamBreakThink);

	SetNextThink(gpGlobals->curtime + 1.0f);

	Vector vecTmpEnd = GetAbsOrigin() + m_vecDir * MAX_TRACE_LENGTH * m_flBeamLength;

	m_hBeam = CBeam::BeamCreate("sprites/laserbeam.vmt", 1.0);
	m_hBeam->PointEntInit(vecTmpEnd, this);
	m_hBeam->SetColor(0, 214, 198);
	m_hBeam->SetScrollRate(25.5);
	m_hBeam->SetBrightness(64);
	m_hBeam->AddSpawnFlags(SF_BEAM_TEMPORARY);
}

void CMonsterTripmineGrenade::BeamBreakThink(void)
{
	bool bBlowup = false;
	trace_t tr;

	UTIL_TraceLine(GetAbsOrigin(), m_vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

	if (!m_hBeam)
	{
		MakeBeam();

		trace_t stuckOnTrace;
		Vector forward;
		GetVectors(&forward, NULL, NULL);

		UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() - forward * 12.0f, MASK_SOLID, this, COLLISION_GROUP_NONE, &stuckOnTrace);

		if (stuckOnTrace.m_pEnt)
		{
			m_hStuckOn = stuckOnTrace.m_pEnt;
		}
	}

	CBaseEntity* pEntity = tr.m_pEnt;
	CBaseCombatCharacter* pBCC = ToBaseCombatCharacter(pEntity);

	if (pBCC || fabs(m_flBeamLength - tr.fraction) > 0.001)
	{
		bBlowup = true;
	}
	else
	{
		if (m_hStuckOn == NULL)
			bBlowup = true;
		else if (m_posStuckOn != m_hStuckOn->GetAbsOrigin())
			bBlowup = true;
		else if (m_angStuckOn != m_hStuckOn->GetAbsAngles())
			bBlowup = true;
	}

	if (bBlowup)
	{
		SetOwnerEntity(m_pRealOwner);
		m_iHealth = 0;
		Event_Killed(CTakeDamageInfo(this, m_pRealOwner, 100, GIB_NORMAL));
		return;
	}

	SetNextThink(gpGlobals->curtime + 0.1);
}

int CMonsterTripmineGrenade::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	if (gpGlobals->curtime < m_flPowerUp && info.GetDamage() < m_iHealth)
	{
		SetThink(&CBaseEntity::SUB_Remove);
		SetNextThink(gpGlobals->curtime + 0.1f);
		KillBeam();
		return 0;
	}
	return OnTakeDamage_Alive(info);
}

void CMonsterTripmineGrenade::Event_Killed(const CTakeDamageInfo& info)
{
	m_takedamage = DAMAGE_NO;

	if (info.GetAttacker() && (info.GetAttacker()->GetFlags() & FL_CLIENT))
	{
		SetOwnerEntity(info.GetAttacker());
	}

	SetThink(&CMonsterTripmineGrenade::DelayDeathThink);
	SetNextThink(gpGlobals->curtime + random->RandomFloat(0.1, 0.3));

	StopSound("MonsterTripmineGrenade.Charge");
}

void CMonsterTripmineGrenade::DelayDeathThink(void)
{
	KillBeam();
	trace_t tr;
	UTIL_TraceLine(GetAbsOrigin() + m_vecDir * 8, GetAbsOrigin() - m_vecDir * 64, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

	Explode(&tr, DMG_BLAST);
}