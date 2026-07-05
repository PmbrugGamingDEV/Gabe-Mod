//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: NPC bot that grabs physics props and moves them around
//
//=============================================================================

#include "cbase.h"
#include "ai_basenpc.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "physics.h"
#include "vphysics/friction.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CNPC_PhysBot : public CAI_BaseNPC
{
	DECLARE_CLASS(CNPC_PhysBot, CAI_BaseNPC);
	DECLARE_DATADESC();

public:
	void Spawn(void);
	void Precache(void);
	Class_T Classify(void);

	void BotThink(void);

private:
	CBaseEntity* FindPhysicsObject(void);
	void GrabObject(CBaseEntity* pEnt);
	void MoveHeldObject(void);
	void DropObject(void);

	EHANDLE m_hHeldObject;
	float m_flNextSearchTime;
	float m_flDropTime;
	Vector m_vecMoveOffset;
};

LINK_ENTITY_TO_CLASS(npc_physbot, CNPC_PhysBot);

BEGIN_DATADESC(CNPC_PhysBot)
DEFINE_FIELD(m_hHeldObject, FIELD_EHANDLE),
DEFINE_FIELD(m_flNextSearchTime, FIELD_TIME),
DEFINE_FIELD(m_flDropTime, FIELD_TIME),
DEFINE_FIELD(m_vecMoveOffset, FIELD_VECTOR),

DEFINE_THINKFUNC(BotThink),
END_DATADESC()

void CNPC_PhysBot::Precache(void)
{
	PrecacheModel("models/Humans/Group03/male_07.mdl");
	BaseClass::Precache();
}

void CNPC_PhysBot::Spawn(void)
{
	Precache();

	SetModel("models/Humans/Group03/male_07.mdl");

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid(SOLID_BBOX);
	SetMoveType(MOVETYPE_STEP);
	SetBloodColor(BLOOD_COLOR_RED);

	m_iHealth = 100;
	m_flFieldOfView = 0.5f;
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd(bits_CAP_MOVE_GROUND);

	NPCInit();

	m_hHeldObject = NULL;
	m_flNextSearchTime = gpGlobals->curtime + 1.0f;
	m_flDropTime = 0.0f;
	m_vecMoveOffset = vec3_origin;

	SetThink(&CNPC_PhysBot::BotThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

Class_T CNPC_PhysBot::Classify(void)
{
	return CLASS_PLAYER_ALLY;
}

void CNPC_PhysBot::BotThink(void)
{
	if (m_hHeldObject)
	{
		MoveHeldObject();

		if (gpGlobals->curtime >= m_flDropTime)
		{
			DropObject();
		}
	}
	else if (gpGlobals->curtime >= m_flNextSearchTime)
	{
		CBaseEntity* pObject = FindPhysicsObject();

		if (pObject)
		{
			GrabObject(pObject);
		}

		m_flNextSearchTime = gpGlobals->curtime + 1.0f;
	}

	SetNextThink(gpGlobals->curtime + 0.05f);
}

CBaseEntity* CNPC_PhysBot::FindPhysicsObject(void)
{
	Vector mins = GetAbsOrigin() - Vector(512, 512, 128);
	Vector maxs = GetAbsOrigin() + Vector(512, 512, 256);

	CBaseEntity* pEnts[128];
	int count = UTIL_EntitiesInBox(pEnts, ARRAYSIZE(pEnts), mins, maxs, 0);

	for (int i = 0; i < count; i++)
	{
		CBaseEntity* pEnt = pEnts[i];

		if (!pEnt)
			continue;

		if (pEnt == this)
			continue;

		if (pEnt->IsPlayer() || pEnt->IsNPC())
			continue;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (!pPhys)
			continue;

		if (!pPhys->IsMoveable())
			continue;

		if (pPhys->GetMass() > 150.0f)
			continue;

		return pEnt;
	}

	return NULL;
}

void CNPC_PhysBot::GrabObject(CBaseEntity* pEnt)
{
	if (!pEnt)
		return;

	IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

	if (!pPhys)
		return;

	m_hHeldObject = pEnt;

	pPhys->Wake();

	m_flDropTime = gpGlobals->curtime + random->RandomFloat(3.0f, 6.0f);

	m_vecMoveOffset = Vector(
		random->RandomFloat(-64.0f, 64.0f),
		random->RandomFloat(-64.0f, 64.0f),
		random->RandomFloat(32.0f, 96.0f)
	);
}

void CNPC_PhysBot::MoveHeldObject(void)
{
	CBaseEntity* pEnt = m_hHeldObject.Get();

	if (!pEnt)
	{
		m_hHeldObject = NULL;
		return;
	}

	IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

	if (!pPhys)
	{
		m_hHeldObject = NULL;
		return;
	}

	Vector forward;
	AngleVectors(GetAbsAngles(), &forward);

	Vector targetPos = EyePosition() + forward * 120.0f + m_vecMoveOffset;
	Vector objectPos = pEnt->WorldSpaceCenter();

	Vector velocity = targetPos - objectPos;
	velocity *= 6.0f;

	float maxSpeed = 600.0f;

	if (velocity.Length() > maxSpeed)
	{
		VectorNormalize(velocity);
		velocity *= maxSpeed;
	}

	AngularImpulse angVel(0, 0, 0);
	pPhys->SetVelocity(&velocity, &angVel);

	pPhys->Wake();
}

void CNPC_PhysBot::DropObject(void)
{
	CBaseEntity* pEnt = m_hHeldObject.Get();

	if (pEnt)
	{
		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (pPhys)
		{
			Vector toss = RandomVector(-100.0f, 100.0f);
			toss.z = random->RandomFloat(50.0f, 200.0f);

			AngularImpulse ang = RandomAngularImpulse(-200, 200);
			pPhys->AddVelocity(&toss, &ang);
			pPhys->Wake();
		}
	}

	m_hHeldObject = NULL;
	m_flNextSearchTime = gpGlobals->curtime + random->RandomFloat(1.0f, 3.0f);
}