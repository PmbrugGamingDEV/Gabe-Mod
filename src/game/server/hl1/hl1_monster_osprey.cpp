#include	"cbase.h"
#include	"beam_shared.h"
#include	"ai_default.h"
#include	"ai_task.h"
#include	"ai_schedule.h"
#include	"ai_node.h"
#include	"ai_hull.h"
#include	"ai_hint.h"
#include	"ai_route.h"
#include	"ai_senses.h"
#include	"hl1_monster_hgrunt.h"
#include	"soundent.h"
#include	"game.h"
#include	"npcevent.h"
#include	"entitylist.h"
#include	"activitylist.h"
#include	"animation.h"
#include	"engine/IEngineSound.h"
#include	"ammodef.h"
#include	"basecombatweapon.h"
#include	"hl1_basegrenade.h"
#include	"soundenvelope.h"
#include	"CBaseHelicopter.h"
#include	"IEffects.h"
#include	"smoke_trail.h"

extern short	g_sModelIndexFireball;

typedef struct
{
	int isValid;
	EHANDLE hGrunt;
	Vector	vecOrigin;
	Vector  vecAngles;
} t_ospreygrunt;

#define LOADED_WITH_GRUNTS 0
#define UNLOADING_GRUNTS 1
#define GRUNTS_DEPLOYED 2
#define SF_WAITFORTRIGGER	0x40
#define MAX_CARRY	24
#define DEFAULT_SPEED 250
#define HELICOPTER_THINK_INTERVAL		0.1

class CHL1Osprey : public CBaseHelicopter
{
	DECLARE_CLASS(CHL1Osprey, CBaseHelicopter);
public:

	int		ObjectCaps(void) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void Spawn(void);
	void Precache(void);
	Class_T  Classify(void) { return CLASS_NONE; };
	int  BloodColor(void) { return DONT_BLEED; }

	void FindAllThink(void);
	void DeployThink(void);
	bool HasDead(void);
	void Flight(void);
	void HoverThink(void);
	CAI_BaseNPC* MakeGrunt(Vector vecSrc);

	void InitializeRotorSound(void);
	void PrescheduleThink(void);

	void DyingThink(void);

	void CrashTouch(CBaseEntity* pOther);

	void TraceAttack(const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr);

	float m_startTime;

	float m_flIdealtilt;
	float m_flRotortilt;

	float m_flRightHealth;
	float m_flLeftHealth;

	int	m_iUnits;
	EHANDLE m_hGrunt[MAX_CARRY];
	Vector m_vecOrigin[MAX_CARRY];
	EHANDLE m_hRepel[4];

	int m_iSoundState;
	int m_iSpriteTexture;

	int m_iPitch;

	int m_iExplode;
	int	m_iTailGibs;
	int	m_iBodyGibs;
	int	m_iEngineGibs;

	int m_iDoLeftSmokePuff;
	int m_iDoRightSmokePuff;

	int m_iRepelState;
	float m_flPrevGoalVel;

	int m_iRotorAngle;
	int	m_nDebrisModel;

	Vector m_cullBoxMins;
	Vector m_cullBoxMaxs;

	CHandle<SmokeTrail>	m_hLeftSmoke;
	CHandle<SmokeTrail>	m_hRightSmoke;

	DECLARE_DATADESC();

	int m_iNextCrashModel;
};

LINK_ENTITY_TO_CLASS(monster_osprey, CHL1Osprey);

BEGIN_DATADESC(CHL1Osprey)
DEFINE_FIELD(m_startTime, FIELD_TIME),

DEFINE_FIELD(m_flIdealtilt, FIELD_FLOAT),
DEFINE_FIELD(m_flRotortilt, FIELD_FLOAT),

DEFINE_FIELD(m_flRightHealth, FIELD_FLOAT),
DEFINE_FIELD(m_flLeftHealth, FIELD_FLOAT),

DEFINE_FIELD(m_iRepelState, FIELD_INTEGER),

DEFINE_FIELD(m_iUnits, FIELD_INTEGER),
DEFINE_ARRAY(m_hGrunt, FIELD_EHANDLE, MAX_CARRY),
DEFINE_ARRAY(m_vecOrigin, FIELD_POSITION_VECTOR, MAX_CARRY),
DEFINE_ARRAY(m_hRepel, FIELD_EHANDLE, 4),

DEFINE_FIELD(m_flPrevGoalVel, FIELD_FLOAT),
DEFINE_FIELD(m_iRotorAngle, FIELD_INTEGER),

DEFINE_FIELD(m_hLeftSmoke, FIELD_EHANDLE),
DEFINE_FIELD(m_hRightSmoke, FIELD_EHANDLE),

DEFINE_FIELD(m_iNextCrashModel, FIELD_INTEGER),

DEFINE_FIELD(m_iDoLeftSmokePuff, FIELD_INTEGER),
DEFINE_FIELD(m_iDoRightSmokePuff, FIELD_INTEGER),

DEFINE_THINKFUNC(FindAllThink),
DEFINE_THINKFUNC(DeployThink),

DEFINE_ENTITYFUNC(CrashTouch),

END_DATADESC()


	void CHL1Osprey::Spawn(void)
{
	Precache();

	SetModel("models/osprey.mdl");

	BaseClass::Spawn();

	Vector mins, maxs;
	ExtractBbox(0, mins, maxs);
	UTIL_SetSize(this, mins, maxs);
	UTIL_SetOrigin(this, GetAbsOrigin());

	AddFlag(FL_NPC);
	m_takedamage = DAMAGE_YES;
	m_flRightHealth = 200;
	m_flLeftHealth = 200;
	m_iHealth = 400;

	m_flFieldOfView = 0;

	SetSequence(0);
	ResetSequenceInfo();
	SetCycle(random->RandomInt(0, 0xFF));

	m_startTime = gpGlobals->curtime + 1;

	m_flMaxSpeed = (float)BASECHOPPER_MAX_SPEED / 2;

	m_iRepelState = LOADED_WITH_GRUNTS;
	m_flPrevGoalVel = 9999;

	m_iRotorAngle = -1;
	SetBoneController(0, m_iRotorAngle);

	m_hLeftSmoke = NULL;
	m_hRightSmoke = NULL;
}


void CHL1Osprey::Precache(void)
{
	UTIL_PrecacheOther("monster_human_grunt");

	PrecacheModel("models/osprey.mdl");
	PrecacheModel("models/HVR.mdl");

	m_iSpriteTexture = PrecacheModel("sprites/rope.vmt");

	m_iExplode = PrecacheModel("sprites/fexplo.vmt");
	m_iTailGibs = PrecacheModel("models/osprey_tailgibs.mdl");
	m_iBodyGibs = PrecacheModel("models/osprey_bodygibs.mdl");
	m_iEngineGibs = PrecacheModel("models/osprey_enginegibs.mdl");

	m_nDebrisModel = PrecacheModel("models/mechgibs.mdl");

	PrecacheScriptSound("Apache.RotorSpinup");

	BaseClass::Precache();
}

void CHL1Osprey::TraceAttack(const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr)
{
	float flDamage = info.GetDamage();

	if (ptr->hitgroup == 3)
	{
		if (m_flRightHealth <= 0)
			return;
		else
			m_flRightHealth -= flDamage;

		if (m_flRightHealth <= 0)
		{
			Assert(m_hRightSmoke == NULL);

			if ((m_hRightSmoke = SmokeTrail::CreateSmokeTrail()) != NULL)
			{
				m_hRightSmoke->m_Opacity = 1.0f;
				m_hRightSmoke->m_SpawnRate = 60;
				m_hRightSmoke->m_ParticleLifetime = 1.3f;
				m_hRightSmoke->m_StartColor.Init(0.65f, 0.65f, 0.65f);
				m_hRightSmoke->m_EndColor.Init(0.65f, 0.65f, 0.65f);
				m_hRightSmoke->m_StartSize = 12;
				m_hRightSmoke->m_EndSize = 80;
				m_hRightSmoke->m_SpawnRadius = 8;
				m_hRightSmoke->m_MinSpeed = 2;
				m_hRightSmoke->m_MaxSpeed = 24;

				m_hRightSmoke->SetLifetime(1e6);
				m_hRightSmoke->FollowEntity(this, "right");
			}
		}
	}

	if (ptr->hitgroup == 2)
	{
		if (m_flLeftHealth <= 0)
			return;
		else
			m_flLeftHealth -= flDamage;

		if (m_flLeftHealth <= 0)
		{
			Assert(m_hLeftSmoke == NULL);

			if ((m_hLeftSmoke = SmokeTrail::CreateSmokeTrail()) != NULL)
			{
				m_hLeftSmoke->m_Opacity = 1.0f;
				m_hLeftSmoke->m_SpawnRate = 60;
				m_hLeftSmoke->m_ParticleLifetime = 1.3f;
				m_hLeftSmoke->m_StartColor.Init(0.65f, 0.65f, 0.65f);
				m_hLeftSmoke->m_EndColor.Init(0.65f, 0.65f, 0.65f);
				m_hLeftSmoke->m_StartSize = 12;
				m_hLeftSmoke->m_EndSize = 64;
				m_hLeftSmoke->m_SpawnRadius = 8;
				m_hLeftSmoke->m_MinSpeed = 2;
				m_hLeftSmoke->m_MaxSpeed = 24;

				m_hLeftSmoke->SetLifetime(1e6);
				m_hLeftSmoke->FollowEntity(this, "left");
			}
		}
	}

	if (flDamage > 50 || ptr->hitgroup == 1 || ptr->hitgroup == 2 || ptr->hitgroup == 3)
	{
		AddMultiDamage(info, this);
	}
	else
	{
		g_pEffects->Sparks(ptr->endpos);
	}
}

void CHL1Osprey::InitializeRotorSound(void)
{
	CSoundEnvelopeController& controller = CSoundEnvelopeController::GetController();

	CPASAttenuationFilter filter(this);
	m_pRotorSound = controller.SoundCreate(filter, entindex(), CHAN_STATIC, "Apache.RotorSpinup", 0.2);

	BaseClass::InitializeRotorSound();
}

void CHL1Osprey::FindAllThink(void)
{
	CBaseEntity* pEntity = NULL;

	m_iUnits = 0;
	while ((pEntity = gEntList.FindEntityByClassname(pEntity, "monster_human_grunt")) != NULL)
	{
		if (m_iUnits > MAX_CARRY)
			break;

		if (pEntity->IsAlive())
		{
			m_hGrunt[m_iUnits] = pEntity;
			m_vecOrigin[m_iUnits] = pEntity->GetAbsOrigin();
			m_iUnits++;
		}
	}

	if (m_iUnits == 0)
	{
		Msg("osprey error: no grunts to resupply\n");
		UTIL_Remove(this);
		return;
	}

	m_startTime = 0.0f;
}

void CHL1Osprey::DeployThink(void)
{
	Vector vecForward;
	Vector vecRight;
	Vector vecUp;
	Vector vecSrc;

	SetLocalAngularVelocity(QAngle(0, 0, 0));
	SetAbsVelocity(Vector(0, 0, 0));

	AngleVectors(GetAbsAngles(), &vecForward, &vecRight, &vecUp);

	trace_t tr;
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -4096.0), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr);
	CSoundEnt::InsertSound(SOUND_DANGER, tr.endpos, 400, 0.3);

	vecSrc = GetAbsOrigin() + vecForward * 32 + vecRight * 100 + vecUp * -96;
	m_hRepel[0] = MakeGrunt(vecSrc);

	vecSrc = GetAbsOrigin() + vecForward * -64 + vecRight * 100 + vecUp * -96;
	m_hRepel[1] = MakeGrunt(vecSrc);

	vecSrc = GetAbsOrigin() + vecForward * 32 + vecRight * -100 + vecUp * -96;
	m_hRepel[2] = MakeGrunt(vecSrc);

	vecSrc = GetAbsOrigin() + vecForward * -64 + vecRight * -100 + vecUp * -96;
	m_hRepel[3] = MakeGrunt(vecSrc);

	m_iRepelState = GRUNTS_DEPLOYED;

	HoverThink();
}

bool CHL1Osprey::HasDead()
{
	for (int i = 0; i < m_iUnits; i++)
	{
		if (m_hGrunt[i] == NULL || !m_hGrunt[i]->IsAlive())
		{
			return TRUE;
		}
		else
		{
			m_vecOrigin[i] = m_hGrunt[i]->GetAbsOrigin();
		}
	}
	return FALSE;
}

void CHL1Osprey::HoverThink(void)
{
	int i = 0;
	for (i = 0; i < 4; i++)
	{
		CBaseEntity* pRepel = (CBaseEntity*)m_hRepel[i];

		if (pRepel != NULL && pRepel->m_iHealth > 0 && pRepel->GetMoveType() == MOVETYPE_FLYGRAVITY)
		{
			break;
		}
	}

	if (i == 4)
		m_iRepelState = LOADED_WITH_GRUNTS;

	if (m_iRepelState != LOADED_WITH_GRUNTS)
	{
		m_iRotorAngle = UTIL_Approach(0, m_iRotorAngle, 5);
	}
}

CAI_BaseNPC* CHL1Osprey::MakeGrunt(Vector vecSrc)
{
	CBaseEntity* pEntity;
	CAI_BaseNPC* pGrunt;

	trace_t tr;
	UTIL_TraceLine(vecSrc, vecSrc + Vector(0, 0, -4096.0), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr);

	if (tr.m_pEnt && tr.m_pEnt->GetSolid() != SOLID_BSP)
		return NULL;

	for (int i = 0; i < m_iUnits; i++)
	{
		if (m_hGrunt[i] == NULL || !m_hGrunt[i]->IsAlive())
		{
			if (m_hGrunt[i] != NULL && m_hGrunt[i]->m_nRenderMode == kRenderNormal)
			{
				m_hGrunt[i]->SUB_StartFadeOut();
			}
			pEntity = Create("monster_human_grunt", vecSrc, GetAbsAngles());
			pGrunt = pEntity->MyNPCPointer();
			pGrunt->SetMoveType(MOVETYPE_FLYGRAVITY);
			pGrunt->SetGravity(0.0001);

			Vector spd = Vector(0, 0, random->RandomFloat(-196, -128));
			pGrunt->SetLocalVelocity(spd);
			pGrunt->SetActivity(ACT_GLIDE);
			pGrunt->SetGroundEntity(NULL);

			pGrunt->SetOwnerEntity(this);


			CBeam* pBeam = CBeam::BeamCreate("sprites/rope.vmt", 1.0);
			pBeam->PointEntInit(vecSrc + Vector(0, 0, 112), pGrunt);
			pBeam->SetBeamFlags(FBEAM_SOLID);
			pBeam->SetColor(255, 255, 255);
			pBeam->SetThink(&CBaseEntity::SUB_Remove);
			pBeam->SetNextThink(gpGlobals->curtime + -4096.0 * tr.fraction / pGrunt->GetAbsVelocity().z + 0.5);

			pGrunt->m_vecLastPosition = m_vecOrigin[i];
			m_hGrunt[i] = pGrunt;
			return pGrunt;
		}
	}

	return NULL;
}

void CHL1Osprey::Flight(void)
{
	if (m_iRepelState == LOADED_WITH_GRUNTS)
	{
		BaseClass::Flight();

		if (m_angleVelocity > 0)
		{
			m_iRotorAngle = UTIL_Approach(-45, m_iRotorAngle, 5 * (m_angleVelocity / 10));
		}
		else
		{
			m_iRotorAngle = UTIL_Approach(-1, m_iRotorAngle, 5);
		}
		SetBoneController(0, m_iRotorAngle);

	}
}

void CHL1Osprey::PrescheduleThink(void)
{
	BaseClass::PrescheduleThink();

	StudioFrameAdvance();

	if (m_startTime != 0.0 && m_startTime <= gpGlobals->curtime)
		FindAllThink();

	if (GetGoalEnt())
	{
		if (m_flPrevGoalVel != GetGoalEnt()->m_flSpeed)
		{
			if (m_flPrevGoalVel == 0 && GetGoalEnt()->m_flSpeed != 0)
			{
				if (HasDead() && m_iRepelState == LOADED_WITH_GRUNTS)
					m_iRepelState = UNLOADING_GRUNTS;
			}

			m_flPrevGoalVel = GetGoalEnt()->m_flSpeed;
		}
	}

	if (m_iRepelState == UNLOADING_GRUNTS)
		DeployThink();
	else if (m_iRepelState == GRUNTS_DEPLOYED)
		HoverThink();
}

void CHL1Osprey::DyingThink(void)
{
	StudioFrameAdvance();
	SetNextThink(gpGlobals->curtime + 0.1f);

	if (gpGlobals->curtime > m_flNextCrashExplosion)
	{
		CPASFilter filter(GetAbsOrigin());
		Vector pos;
		QAngle dummy;

		int rand = RandomInt(0, 10);

		if (rand < 4)
		{
			int iAttach = LookupAttachment(rand % 2 ? "left" : "right");
			GetAttachment(iAttach, pos, dummy);
		}
		else
		{
			pos = GetAbsOrigin();
			pos.x += random->RandomFloat(-150, 150);
			pos.y += random->RandomFloat(-150, 150);
			pos.z += random->RandomFloat(-150, -50);
		}

		te->Explosion(filter, 0.0, &pos, g_sModelIndexFireball, 10, 15, TE_EXPLFLAG_NONE, 100, 0);
		m_flNextCrashExplosion = gpGlobals->curtime + random->RandomFloat(0.4, 0.7);

		Vector vecSize = Vector(500, 500, 60);

		switch (m_iNextCrashModel)
		{
		case 0:
		{
			te->BreakModel(filter, 0.0, GetAbsOrigin(), vec3_angle,
				vecSize, vec3_origin, m_iTailGibs, 100, 0, 2.5, BREAK_METAL);
			break;
		}
		case 1:
		{
			te->BreakModel(filter, 0.0, GetAbsOrigin(), vec3_angle,
				vecSize, vec3_origin, m_iBodyGibs, 100, 0, 2.5, BREAK_METAL);
			break;
		}
		case 2:
		{
			te->BreakModel(filter, 0.0, GetAbsOrigin(), vec3_angle,
				vecSize, vec3_origin, m_iEngineGibs, 100, 0, 2.5, BREAK_METAL);
			break;
		}
		case 3:
		{
			te->BreakModel(filter, 0.0, GetAbsOrigin(), vec3_angle,
				vecSize, vec3_origin, m_nDebrisModel, 100, 0, 2.5, BREAK_METAL);
			break;
		}
		}

		m_iNextCrashModel++;
		if (m_iNextCrashModel > 3) m_iNextCrashModel = 0;
	}

	QAngle angVel = GetLocalAngularVelocity();
	if (angVel.y < 400)
	{
		angVel.y *= 1.1;
		SetLocalAngularVelocity(angVel);
	}
	Vector vecImpulse(0, 0, 0);
	vecImpulse.z -= 38.4;
	ApplyAbsVelocityImpulse(vecImpulse);

}

void CHL1Osprey::CrashTouch(CBaseEntity* pOther)
{
	Vector vecSize = Vector(120, 120, 30);
	CPVSFilter filter(GetAbsOrigin());

	te->BreakModel(filter, 0.0, GetAbsOrigin(), vec3_angle,
		vecSize, vec3_origin, m_iTailGibs, 100, 0, 2.5, BREAK_METAL);

	if (m_hLeftSmoke)
	{
		m_hLeftSmoke->SetLifetime(0.1f);
		m_hLeftSmoke = NULL;
	}

	if (m_hRightSmoke)
	{
		m_hRightSmoke->SetLifetime(0.1f);
		m_hRightSmoke = NULL;
	}

	BaseClass::CrashTouch(pOther);
}