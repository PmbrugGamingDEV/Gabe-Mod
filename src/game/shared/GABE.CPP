///===========================================================================
// Purpose: Gabe mod namespace
///===========================================================================

#include "cbase.h"

#include "GABE.H"

#include "vscript_shared.h"
#include "baseentity_shared.h"

// =========================
// 🔹 Shared (client + server)
// =========================

#include "shareddefs.h"

#include "collisionutils.h"

// =========================
// 🔹 Client only
// =========================
#ifdef CLIENT_DLL

#include "debugoverlay_shared.h"
#include "vgui_all.h"
#include "IVehicle.h"
#include "iclientvehicle.h"

#else // ================== SERVER ==================

#include "game.h"

#include "hl2mp_player.h"
#include "player.h"

#include "util.h"
#include "entitylist.h"

#include "ndebugoverlay.h"
#include "debugoverlay_shared.h"

#include "soundent.h"
#include "engine/IEngineSound.h"

#include "props.h"
#include "ai_basenpc.h"

#include "vphysics_interface.h"
#include "vphysics/constraints.h"
#include "physics.h"

#include "IVehicle.h"
#include "vehicle_base.h"

#endif

#define FUNC_REGISTER(name, func, description) ScriptRegisterFunctionNamed(g_pScriptVM, func, name, description)

namespace GabeMod
{

#define GABE_PRINTSMALLMESSAGE		1
#define GABE_PRINTTOCONSOLE			2
#define GABE_PRINTTOCHAT			3
#define GABE_PRINTVISUALTEXT		4

#ifndef CLIENT_DLL
	CBasePlayer* CGabeServerFuncs::GetPlayer()
#else
	C_BasePlayer* CGabeClientFuncs::GetPlayer()
#endif
	{
		if (gpGlobals->maxClients == 1)
		{
#ifdef CLIENT_DLL
			return C_BasePlayer::GetLocalPlayer();
#else
			return UTIL_GetLocalPlayer();
#endif
		}

#ifdef CLIENT_DLL
		return C_BasePlayer::GetLocalPlayer();
#else
		CBasePlayer* pPlayer = UTIL_GetCommandClient();

		if (pPlayer)
			return pPlayer;

		for (int i = 1; i <= gpGlobals->maxClients; i++)
		{
			CBasePlayer* p = UTIL_PlayerByIndex(i);
			if (p)
				return p;
		}

		return NULL;
#endif
	}

#ifndef CLIENT_DLL

	// =========================
	// 🔹 Messaging
	// =========================
	void CGabeServerFuncs::PrintToYou(CBasePlayer* pPlayer, int dest, const char* msg)
	{
		if (!pPlayer) return;
		ClientPrint(pPlayer, dest, msg);
	}

	void CGabeServerFuncs::PrintToAllPlayers(int dest, const char* msg) // INVALID ON CLIENTSIDE
	{
		UTIL_ClientPrintAll(dest, msg);
	}

	// =========================
	// 🔹 Debug Overlay
	// =========================
	void CGabeServerFuncs::ScreenText(const char* text, float x, float y, float time, int r, int g, int b, int a)
	{
		if (!debugoverlay) return;
		debugoverlay->AddScreenTextOverlay(x, y, time, r, g, b, a, text);
	}


	void CGabeServerFuncs::DebugLine(const Vector& start, const Vector& end, float time)
	{
		NDebugOverlay::Line(start, end, 0, 255, 0, true, time);
	}

	void CGabeServerFuncs::DebugBox(const Vector& pos, float size, float time)
	{
		NDebugOverlay::Box(pos,
			-Vector(size, size, size),
			Vector(size, size, size),
			255, 0, 0, 0,
			time);
	}

	// =========================
	// 🔹 Math / Player vectors
	// =========================
	Vector CGabeServerFuncs::Forward(CBasePlayer* pPlayer)
	{
		Vector forward;
		if (!pPlayer) return vec3_origin;

		AngleVectors(pPlayer->EyeAngles(), &forward);
		return forward;
	}

	Vector CGabeServerFuncs::ShootPos(CBasePlayer* pPlayer)
	{
		if (!pPlayer) return vec3_origin;
		return pPlayer->Weapon_ShootPosition();
	}

	// =========================
	// 🔹 Trace helpers
	// =========================
	CBaseEntity* CGabeServerFuncs::TraceEntity(CBasePlayer* pPlayer, float dist)
	{
		if (!pPlayer) return NULL;

		trace_t tr;

		Vector start = ShootPos(pPlayer);
		Vector forward = Forward(pPlayer);
		Vector end = start + forward * dist;

		UTIL_TraceLine(start, end, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

		return tr.m_pEnt;
	}

	void CGabeServerFuncs::Trace(CBasePlayer* pPlayer, float dist, trace_t& tr)
	{
		if (!pPlayer)
			return;

		Vector start = ShootPos(pPlayer);

		Vector forward;
		AngleVectors(pPlayer->EyeAngles(), &forward);

		Vector end = start + forward * dist;

		UTIL_TraceLine(start, end, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);
	}

	// =========================
	// 🔹 Physics
	// =========================
	void CGabeServerFuncs::ApplyForce(CBaseEntity* pEnt, const Vector& force)
	{
		if (!pEnt) return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
		if (!pPhys) return;

		pPhys->ApplyForceCenter(force);
	}

	void CGabeServerFuncs::SetMass(CBaseEntity* pEnt, float mass)
	{
		if (!pEnt) return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
		if (!pPhys) return;

		pPhys->SetMass(mass);
	}

	// =========================
	// 🔹 Spawning
	// =========================
	CBaseEntity* CGabeServerFuncs::SpawnProp(const char* model, const Vector& pos, const QAngle& ang)
	{
		CBaseEntity* pProp = CreateEntityByName("prop_physics");

		if (!pProp)
			return NULL;

		pProp->PrecacheModel(model);
		pProp->SetModel(model);
		pProp->SetAbsOrigin(pos);
		pProp->SetAbsAngles(ang);

		DispatchSpawn(pProp);
		pProp->Activate();

		return pProp;
	}

	CBaseEntity* CGabeServerFuncs::SpawnEntity(const char* classname, const Vector& pos, const QAngle& ang)
	{
		CBaseEntity* pEnt = CreateEntityByName(classname);

		if (!pEnt)
			return NULL;

		pEnt->SetAbsOrigin(pos);
		pEnt->SetAbsAngles(ang);

		DispatchSpawn(pEnt);
		pEnt->Activate();

		return pEnt;
	}

	// =========================
	// 🔹 Entity helpers
	// =========================
	CBaseEntity* CGabeServerFuncs::FindByName(const char* name)
	{
		CBaseEntity* pEnt = NULL;

		while ((pEnt = gEntList.FindEntityByName(pEnt, name)) != NULL)
		{
			return pEnt;
		}

		return NULL;
	}

	void CGabeServerFuncs::Remove(CBaseEntity* pEnt)
	{
		if (!pEnt) return;
		UTIL_Remove(pEnt);
	}

	// =========================
	// 🔹 Gameplay helpers
	// =========================
	void CGabeServerFuncs::GiveAllWeapons(CHL2MP_Player* pPlayer)
	{
		pPlayer->GiveAllItems();
	}

	void CGabeServerFuncs::Heal(CBasePlayer* pPlayer, int amount)
	{
		if (!pPlayer) return;

		int newHealth = pPlayer->GetHealth() + amount;
		pPlayer->SetHealth(newHealth);
	}

	void CGabeServerFuncs::Damage(CBasePlayer* pPlayer, int amount)
	{
		if (!pPlayer) return;

		CTakeDamageInfo dmg;
		dmg.SetDamage(amount);

		pPlayer->TakeDamage(dmg);
	}

	CBaseEntity* CGabeServerFuncs::Gabe_FindByName(const char* name)
	{
		if (!name)
			return nullptr;

		return gEntList.FindEntityByName(nullptr, name);
	}

	CBaseEntity* CGabeServerFuncs::Gabe_FindByClass(const char* classname)
	{
		if (!classname)
			return nullptr;

		return gEntList.FindEntityByClassname(nullptr, classname);
	}

	CBaseEntity* CGabeServerFuncs::Gabe_FindClosestEntity(const Vector& origin, const char* classname, float maxDist)
	{
		CBaseEntity* pEntity = nullptr;
		CBaseEntity* pBest = nullptr;
		float bestDist = maxDist;

		while ((pEntity = gEntList.FindEntityByClassname(pEntity, classname)) != nullptr)
		{
			float dist = origin.DistTo(pEntity->GetAbsOrigin());

			if (dist < bestDist)
			{
				bestDist = dist;
				pBest = pEntity;
			}
		}

		return pBest;
	}

	void CGabeServerFuncs::Gabe_DealDamage(
		CBaseEntity* victim,
		CBaseEntity* attacker,
		float damage,
		int damageType)
	{
		if (!victim)
			return;

		CTakeDamageInfo info(attacker, attacker, damage, damageType);
		victim->TakeDamage(info);
	}

	void CGabeServerFuncs::Gabe_RadiusDamage(
		const Vector& origin,
		CBaseEntity* attacker,
		float damage,
		float radius,
		int damageType)
	{
		CTakeDamageInfo info(attacker, attacker, damage, damageType);
		RadiusDamage(info, origin, radius, CLASS_NONE, nullptr);
	}

	void CGabeServerFuncs::Gabe_TraceFromPlayer(
		CBasePlayer* player,
		float distance,
		trace_t& tr)
	{
		if (!player)
			return;

		Vector start = player->EyePosition();
		Vector forward;
		player->EyeVectors(&forward);

		Vector end = start + forward * distance;

		UTIL_TraceLine(
			start,
			end,
			MASK_SHOT,
			player,
			COLLISION_GROUP_NONE,
			&tr);
	}

	bool CGabeServerFuncs::Gabe_HasLineOfSight(CBaseEntity* from, CBaseEntity* to)
	{
		if (!from || !to)
			return false;

		trace_t tr;

		UTIL_TraceLine(
			from->EyePosition(),
			to->WorldSpaceCenter(),
			MASK_SOLID,
			from,
			COLLISION_GROUP_NONE,
			&tr);

		return (tr.m_pEnt == to || tr.fraction == 1.0f);
	}

	bool CGabeServerFuncs::Gabe_IsInFOV(CBaseEntity* viewer, CBaseEntity* target, float fovDegrees)
	{
		if (!viewer || !target)
			return false;

		Vector forward;

		Vector toTarget = (target->WorldSpaceCenter() - viewer->EyePosition());
		toTarget.NormalizeInPlace();

		float dot = DotProduct(forward, toTarget);

		float cosHalfFOV = cosf(DEG2RAD(fovDegrees * 0.5f));

		return dot >= cosHalfFOV;
	}

	IPhysicsObject* CGabeServerFuncs::Gabe_GetPhys(CBaseEntity* ent)
	{
		if (!ent)
			return nullptr;

		return ent->VPhysicsGetObject();
	}

	/////////////////////////////////////////////////////////////////////
	// CONSTRAINTS
	////////////////////////////////////////////////////////////////////

	IPhysicsConstraint* CGabeServerFuncs::Gabe_CreateFixed(
		CBaseEntity* refEnt,
		CBaseEntity* attachedEnt)
	{
		if (!refEnt || !attachedEnt)
			return nullptr;

		IPhysicsObject* physRef = refEnt->VPhysicsGetObject();
		IPhysicsObject* physAttached = attachedEnt->VPhysicsGetObject();

		if (!physRef || !physAttached)
			return nullptr;

		constraint_fixedparams_t params;
		params.Defaults();

		params.InitWithCurrentObjectState(physRef, physAttached);

		return physenv->CreateFixedConstraint(
			physRef,
			physAttached,
			nullptr,
			params);
	}

	IPhysicsConstraint* CGabeServerFuncs::Gabe_CreateBreakableFixed(
		CBaseEntity* refEnt,
		CBaseEntity* attachedEnt)
	{
		if (!refEnt || !attachedEnt)
			return nullptr;

		IPhysicsObject* physRef = refEnt->VPhysicsGetObject();
		IPhysicsObject* physAttached = attachedEnt->VPhysicsGetObject();

		if (!physRef || !physAttached)
			return nullptr;

		constraint_fixedparams_t params;
		params.Defaults();

		params.InitWithCurrentObjectState(physRef, physAttached);

		return physenv->CreateFixedConstraint(
			physRef,
			physAttached,
			nullptr,
			params);
	}

	IPhysicsConstraint* CGabeServerFuncs::Gabe_FixedToWorld(CBaseEntity* ent)
	{
		if (!ent)
			return nullptr;

		IPhysicsObject* phys = ent->VPhysicsGetObject();
		if (!phys)
			return nullptr;

		constraint_fixedparams_t params;
		params.Defaults();

		params.InitWithCurrentObjectState(
			g_PhysWorldObject,
			phys);

		return physenv->CreateFixedConstraint(
			g_PhysWorldObject,
			phys,
			nullptr,
			params);
	}

	IPhysicsConstraint* CGabeServerFuncs::Gabe_CreateBallSocket(
		CBaseEntity* ent1,
		CBaseEntity* ent2,
		const Vector& worldPos)
	{
		IPhysicsObject* phys1 = Gabe_GetPhys(ent1);
		IPhysicsObject* phys2 = Gabe_GetPhys(ent2);

		if (!phys1 || !phys2)
			return nullptr;

		constraint_ballsocketparams_t params;
		params.Defaults();

		params.InitWithCurrentObjectState(
			phys1,
			phys2,
			worldPos);

		return physenv->CreateBallsocketConstraint(
			phys1,
			phys2,
			nullptr,
			params);
	}

	IPhysicsConstraint* CGabeServerFuncs::Gabe_CreateRope(
		CBaseEntity* ent1,
		CBaseEntity* ent2,
		const Vector& worldPos1,
		const Vector& worldPos2,
		bool rigid = false)
	{
		IPhysicsObject* phys1 = Gabe_GetPhys(ent1);
		IPhysicsObject* phys2 = Gabe_GetPhys(ent2);

		if (!phys1 || !phys2)
			return nullptr;

		constraint_lengthparams_t params;
		params.Defaults();

		params.InitWorldspace(
			phys1,
			phys2,
			worldPos1,
			worldPos2,
			rigid);

		return physenv->CreateLengthConstraint(
			phys1,
			phys2,
			nullptr,
			params);
	}

	IPhysicsConstraint* CGabeServerFuncs::Gabe_CreateSlider(
		CBaseEntity* refEnt,
		CBaseEntity* attachedEnt,
		const Vector& slideAxisWorld,
		float limitMin,
		float limitMax)
	{
		IPhysicsObject* physRef = Gabe_GetPhys(refEnt);
		IPhysicsObject* physAttached = Gabe_GetPhys(attachedEnt);

		if (!physRef || !physAttached)
			return nullptr;

		constraint_slidingparams_t params;
		params.Defaults();

		params.InitWithCurrentObjectState(
			physRef,
			physAttached,
			slideAxisWorld);

		params.limitMin = limitMin;
		params.limitMax = limitMax;

		return physenv->CreateSlidingConstraint(
			physRef,
			physAttached,
			nullptr,
			params);
	}

	void CGabeServerFuncs::Gabe_SetSliderMotor(
		IPhysicsConstraint* constraint,
		float velocity,
		float maxForce)
	{
		if (!constraint)
			return;

		constraint->SetLinearMotor(velocity, maxForce);
	}

	IPhysicsConstraint* CGabeServerFuncs::Gabe_CreateHinge(
		CBaseEntity* ent1,
		CBaseEntity* ent2,
		const Vector& worldPos,
		const Vector& worldAxis)
	{
		IPhysicsObject* phys1 = Gabe_GetPhys(ent1);
		IPhysicsObject* phys2 = Gabe_GetPhys(ent2);

		if (!phys1 || !phys2)
			return nullptr;

		constraint_hingeparams_t params;
		params.Defaults();

		params.worldPosition = worldPos;
		params.worldAxisDirection = worldAxis;

		return physenv->CreateHingeConstraint(
			phys1,
			phys2,
			nullptr,
			params);
	}

	IPhysicsConstraint* CGabeServerFuncs::Gabe_CreateLimitedHinge(
		CBaseEntity* ent1,
		CBaseEntity* ent2,
		const Vector& worldPos,
		const Vector& worldAxis,
		float minAngle,
		float maxAngle)
	{
		IPhysicsObject* phys1 = Gabe_GetPhys(ent1);
		IPhysicsObject* phys2 = Gabe_GetPhys(ent2);

		if (!phys1 || !phys2)
			return nullptr;

		constraint_limitedhingeparams_t params;
		params.Defaults();

		params.worldPosition = worldPos;
		params.worldAxisDirection = worldAxis;

		params.hingeAxis.minRotation = minAngle;
		params.hingeAxis.maxRotation = maxAngle;

		return physenv->CreateHingeConstraint(
			phys1,
			phys2,
			nullptr,
			params);
	}

	void CGabeServerFuncs::Gabe_SetBreakable(
		constraint_breakableparams_t& breakParams,
		float forceLimit,
		float torqueLimit)
	{
		breakParams.forceLimit = forceLimit;
		breakParams.torqueLimit = torqueLimit;
	}

	void CGabeServerFuncs::Gabe_DestroyConstraint(IPhysicsConstraint* constraint)
	{
		if (!constraint)
			return;

		physenv->DestroyConstraint(constraint);
	}

	// ============================================================
	// Entity state
	// ============================================================

	Vector CGabeServerFuncs::GetOrigin(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return vec3_origin;

		return pEnt->GetAbsOrigin();
	}

	QAngle CGabeServerFuncs::GetAngles(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return vec3_angle;

		return pEnt->GetAbsAngles();
	}

	void CGabeServerFuncs::SetOrigin(
		CBaseEntity* pEnt,
		const Vector& pos)
	{
		if (!pEnt)
			return;

		pEnt->SetAbsOrigin(pos);
	}

	void CGabeServerFuncs::SetAngles(
		CBaseEntity* pEnt,
		const QAngle& ang)
	{
		if (!pEnt)
			return;

		pEnt->SetAbsAngles(ang);
	}

	Vector CGabeServerFuncs::GetVelocity(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return vec3_origin;

		return pEnt->GetAbsVelocity();
	}

	void CGabeServerFuncs::SetVelocity(
		CBaseEntity* pEnt,
		const Vector& velocity)
	{
		if (!pEnt)
			return;

		pEnt->SetAbsVelocity(velocity);
	}

	void CGabeServerFuncs::AddVelocity(
		CBaseEntity* pEnt,
		const Vector& velocity)
	{
		if (!pEnt)
			return;

		pEnt->ApplyAbsVelocityImpulse(velocity);
	}

	int CGabeServerFuncs::GetHealth(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 0;

		return pEnt->GetHealth();
	}

	int CGabeServerFuncs::GetMaxHealth(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 0;

		return pEnt->GetMaxHealth();
	}

	void CGabeServerFuncs::SetHealth(
		CBaseEntity* pEnt,
		int health)
	{
		if (!pEnt)
			return;

		pEnt->SetHealth(health);
	}

	void CGabeServerFuncs::SetMaxHealth(
		CBaseEntity* pEnt,
		int health)
	{
		if (!pEnt)
			return;

		pEnt->SetMaxHealth(health);
	}

	bool CGabeServerFuncs::IsAlive(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return false;

		return pEnt->IsAlive();
	}

	bool CGabeServerFuncs::IsPlayer(CBaseEntity* pEnt)
	{
		return pEnt && pEnt->IsPlayer();
	}

	bool CGabeServerFuncs::IsNPC(CBaseEntity* pEnt)
	{
		return pEnt && pEnt->IsNPC();
	}

	const char* CGabeServerFuncs::GetClassname(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return "";

		return pEnt->GetClassname();
	}

	const char* CGabeServerFuncs::GetEntityName(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return "";

		return STRING(pEnt->GetEntityName());
	}

	int CGabeServerFuncs::GetEntIndex(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return -1;

		return pEnt->entindex();
	}

	void CGabeServerFuncs::SetName(
		CBaseEntity* pEnt,
		const char* name)
	{
		if (!pEnt || !name)
			return;

		pEnt->SetName(AllocPooledString(name));
	}

	void CGabeServerFuncs::SetModel(
		CBaseEntity* pEnt,
		const char* model)
	{
		if (!pEnt || !model)
			return;

		pEnt->PrecacheModel(model);
		pEnt->SetModel(model);
	}

	void CGabeServerFuncs::Activate(CBaseEntity* pEnt)
	{
		if (pEnt)
			pEnt->Activate();
	}

	void CGabeServerFuncs::Teleport(
		CBaseEntity* pEnt,
		const Vector& pos,
		const QAngle& ang,
		const Vector& velocity)
	{
		if (!pEnt)
			return;

		pEnt->Teleport(&pos, &ang, &velocity);
	}

	void CGabeServerFuncs::AddEffects(
		CBaseEntity* pEnt,
		int effects)
	{
		if (pEnt)
			pEnt->AddEffects(effects);
	}

	void CGabeServerFuncs::RemoveEffects(
		CBaseEntity* pEnt,
		int effects)
	{
		if (pEnt)
			pEnt->RemoveEffects(effects);
	}

	bool CGabeServerFuncs::HasEffects(
		CBaseEntity* pEnt,
		int effects)
	{
		if (!pEnt)
			return false;

		return pEnt->IsEffectActive(effects);
	}

	void CGabeServerFuncs::AddEFlags(
		CBaseEntity* pEnt,
		int flags)
	{
		if (pEnt)
			pEnt->AddEFlags(flags);
	}

	void CGabeServerFuncs::RemoveEFlags(
		CBaseEntity* pEnt,
		int flags)
	{
		if (pEnt)
			pEnt->RemoveEFlags(flags);
	}

	bool CGabeServerFuncs::HasEFlags(
		CBaseEntity* pEnt,
		int flags)
	{
		if (!pEnt)
			return false;

		return pEnt->IsEFlagSet(flags);
	}

	void CGabeServerFuncs::SetSolid(
		CBaseEntity* pEnt,
		bool solid)
	{
		if (!pEnt)
			return;

		if (solid)
			pEnt->RemoveSolidFlags(FSOLID_NOT_SOLID);
		else
			pEnt->AddSolidFlags(FSOLID_NOT_SOLID);
	}

	void CGabeServerFuncs::SetMoveType(
		CBaseEntity* pEnt,
		int moveType)
	{
		if (!pEnt)
			return;

		pEnt->SetMoveType((MoveType_t)moveType);
	}

	void CGabeServerFuncs::SetCollisionGroup(
		CBaseEntity* pEnt,
		int group)
	{
		if (pEnt)
			pEnt->SetCollisionGroup(group);
	}

	int CGabeServerFuncs::GetCollisionGroup(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return COLLISION_GROUP_NONE;

		return pEnt->GetCollisionGroup();
	}

	void CGabeServerFuncs::SetParent(
		CBaseEntity* pEnt,
		CBaseEntity* pParent)
	{
		if (!pEnt)
			return;

		pEnt->SetParent(pParent);
	}

	void CGabeServerFuncs::ClearParent(CBaseEntity* pEnt)
	{
		if (pEnt)
			pEnt->SetParent(NULL);
	}

	CBaseEntity* CGabeServerFuncs::GetParent(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return NULL;

		return pEnt->GetMoveParent();
	}

	CBaseEntity* CGabeServerFuncs::GetOwner(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return NULL;

		return pEnt->GetOwnerEntity();
	}

	void CGabeServerFuncs::SetOwner(
		CBaseEntity* pEnt,
		CBaseEntity* pOwner)
	{
		if (pEnt)
			pEnt->SetOwnerEntity(pOwner);
	}

	CBaseEntity* CGabeServerFuncs::FindByClass(
		const char* classname)
	{
		if (!classname)
			return NULL;

		return gEntList.FindEntityByClassname(
			NULL,
			classname
		);
	}

	CBaseEntity* CGabeServerFuncs::FindByNameAfter(
		CBaseEntity* pStart,
		const char* name)
	{
		if (!name)
			return NULL;

		return gEntList.FindEntityByName(
			pStart,
			name
		);
	}

	CBaseEntity* CGabeServerFuncs::FindByClassAfter(
		CBaseEntity* pStart,
		const char* classname)
	{
		if (!classname)
			return NULL;

		return gEntList.FindEntityByClassname(
			pStart,
			classname
		);
	}

	CBaseEntity* CGabeServerFuncs::FindInSphere(
		CBaseEntity* pStart,
		const Vector& origin,
		float radius)
	{
		return gEntList.FindEntityInSphere(
			pStart,
			origin,
			radius
		);
	}

	CBasePlayer* CGabeServerFuncs::GetPlayerByIndex(int index)
	{
		return UTIL_PlayerByIndex(index);
	}

	int CGabeServerFuncs::GetPlayerCount()
	{
		int count = 0;

		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			if (UTIL_PlayerByIndex(i))
				++count;
		}

		return count;
	}

	void CGabeServerFuncs::SetArmor(
		CBasePlayer* pPlayer,
		int armor)
	{
		if (!pPlayer)
			return;

		pPlayer->SetArmorValue(armor);
	}

	int CGabeServerFuncs::GetArmor(CBasePlayer* pPlayer)
	{
		if (!pPlayer)
			return 0;

		return pPlayer->ArmorValue();
	}

	void CGabeServerFuncs::SetFOV(
		CBasePlayer* pPlayer,
		int fov)
	{
		if (!pPlayer)
			return;

		pPlayer->SetFOV(pPlayer, fov);
	}

	void CGabeServerFuncs::StripWeapons(CBasePlayer* pPlayer)
	{
		if (!pPlayer)
			return;

		pPlayer->RemoveAllItems(false);
	}

	Vector CGabeServerFuncs::GetEyePosition(CBasePlayer* pPlayer)
	{
		if (!pPlayer)
			return vec3_origin;

		return pPlayer->EyePosition();
	}

	QAngle CGabeServerFuncs::GetEyeAngles(CBasePlayer* pPlayer)
	{
		if (!pPlayer)
			return vec3_angle;

		return pPlayer->EyeAngles();
	}

	bool CGabeServerFuncs::HasPhysics(CBaseEntity* pEnt)
	{
		return pEnt && pEnt->VPhysicsGetObject();
	}

	void CGabeServerFuncs::WakePhysics(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (pPhys)
			pPhys->Wake();
	}

	void CGabeServerFuncs::SleepPhysics(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (pPhys)
			pPhys->Sleep();
	}

	float CGabeServerFuncs::GetMass(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 0.0f;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (!pPhys)
			return 0.0f;

		return pPhys->GetMass();
	}

	Vector CGabeServerFuncs::GetPhysicsVelocity(
		CBaseEntity* pEnt)
	{
		if (!pEnt)
			return vec3_origin;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (!pPhys)
			return vec3_origin;

		Vector velocity;
		AngularImpulse angular;

		pPhys->GetVelocity(
			&velocity,
			&angular
		);

		return velocity;
	}

	void CGabeServerFuncs::SetPhysicsVelocity(
		CBaseEntity* pEnt,
		const Vector& velocity)
	{
		if (!pEnt)
			return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (!pPhys)
			return;

		pPhys->SetVelocity(
			&velocity,
			NULL
		);
	}

	void CGabeServerFuncs::ApplyTorque(
		CBaseEntity* pEnt,
		const Vector& torque)
	{
		if (!pEnt)
			return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();

		if (!pPhys)
			return;

		AngularImpulse impulse(
			torque.x,
			torque.y,
			torque.z
		);

		pPhys->ApplyTorqueCenter(impulse);
	}

	void CGabeServerFuncs::KillEntity(
		CBaseEntity* victim,
		CBaseEntity* attacker)
	{
		if (!victim)
			return;

		if (!attacker)
			attacker = victim;

		CTakeDamageInfo info(
			attacker,
			attacker,
			FLT_MAX,
			DMG_GENERIC
		);

		victim->TakeDamage(info);
	}

	void CGabeServerFuncs::Ignite(
		CBaseEntity* pEnt,
		float time)
	{
		if (!pEnt)
			return;

		CBaseAnimating* pAnimating =
			dynamic_cast<CBaseAnimating*>(pEnt);

		if (!pAnimating)
			return;

		pAnimating->Ignite(time);
	}

	void CGabeServerFuncs::Extinguish(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return;

		CBaseAnimating* pAnimating =
			dynamic_cast<CBaseAnimating*>(pEnt);

		if (pAnimating)
			pAnimating->Extinguish();
	}

	float CGabeServerFuncs::Distance(
		const Vector& a,
		const Vector& b)
	{
		return a.DistTo(b);
	}

	float CGabeServerFuncs::DistanceEntities(
		CBaseEntity* a,
		CBaseEntity* b)
	{
		if (!a || !b)
			return -1.0f;

		return a->GetAbsOrigin().DistTo(
			b->GetAbsOrigin()
		);
	}

	Vector CGabeServerFuncs::DirectionTo(
		CBaseEntity* from,
		CBaseEntity* to)
	{
		if (!from || !to)
			return vec3_origin;

		Vector dir =
			to->WorldSpaceCenter() -
			from->WorldSpaceCenter();

		dir.NormalizeInPlace();

		return dir;
	}

	Vector CGabeServerFuncs::GetCenter(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return vec3_origin;

		return pEnt->WorldSpaceCenter();
	}

	void CGabeServerFuncs::EmitSound(
		CBaseEntity* pEnt,
		const char* soundName)
	{
		if (!pEnt || !soundName)
			return;

		pEnt->EmitSound(soundName);
	}

	void CGabeServerFuncs::StopSound(
		CBaseEntity* pEnt,
		const char* soundName)
	{
		if (!pEnt || !soundName)
			return;

		pEnt->StopSound(soundName);
	}

	float CGabeServerFuncs::RandomFloat(
		float min,
		float max)
	{
		return random->RandomFloat(min, max);
	}

	int CGabeServerFuncs::RandomInt(
		int min,
		int max)
	{
		return random->RandomInt(min, max);
	}

	float CGabeServerFuncs::GetTime()
	{
		return gpGlobals->curtime;
	}

	int CGabeServerFuncs::GetTickCountA()
	{
		return gpGlobals->tickcount;
	}

	float CGabeServerFuncs::GetTickInterval()
	{
		return gpGlobals->interval_per_tick;
	}

	bool CGabeServerFuncs::FireInput(
		CBaseEntity* pEnt,
		const char* input)
	{
		if (!pEnt || !input)
			return false;

		variant_t value;

		return pEnt->AcceptInput(
			input,
			NULL,
			NULL,
			value,
			0
		);
	}

	bool CGabeServerFuncs::FireInputString(
		CBaseEntity* pEnt,
		const char* input,
		const char* value)
	{
		if (!pEnt || !input)
			return false;

		variant_t var;
		var.SetString(
			AllocPooledString(value ? value : "")
		);

		return pEnt->AcceptInput(
			input,
			NULL,
			NULL,
			var,
			0
		);
	}

	bool CGabeServerFuncs::IsPlayerAlive(CBasePlayer* pPlayer)
	{
		return pPlayer && pPlayer->IsAlive();
	}

	int CGabeServerFuncs::GetPlayerHealth(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetHealth() : 0;
	}

	void CGabeServerFuncs::SetPlayerHealth(CBasePlayer* pPlayer, int health)
	{
		if (pPlayer)
			pPlayer->SetHealth(health);
	}

	int CGabeServerFuncs::GetPlayerArmor(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->ArmorValue() : 0;
	}

	void CGabeServerFuncs::SetPlayerArmor(CBasePlayer* pPlayer, int armor)
	{
		if (pPlayer)
			pPlayer->SetArmorValue(armor);
	}

	Vector CGabeServerFuncs::GetPlayerOrigin(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetAbsOrigin() : vec3_origin;
	}

	void CGabeServerFuncs::SetPlayerOrigin(
		CBasePlayer* pPlayer,
		const Vector& origin)
	{
		if (pPlayer)
			pPlayer->SetAbsOrigin(origin);
	}

	Vector CGabeServerFuncs::GetPlayerVelocity(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetAbsVelocity() : vec3_origin;
	}

	void CGabeServerFuncs::SetPlayerVelocity(
		CBasePlayer* pPlayer,
		const Vector& velocity)
	{
		if (pPlayer)
			pPlayer->SetAbsVelocity(velocity);
	}

	void CGabeServerFuncs::AddPlayerVelocity(
		CBasePlayer* pPlayer,
		const Vector& velocity)
	{
		if (pPlayer)
			pPlayer->ApplyAbsVelocityImpulse(velocity);
	}

	Vector CGabeServerFuncs::GetPlayerEyePosition(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->EyePosition() : vec3_origin;
	}

	QAngle CGabeServerFuncs::GetPlayerEyeAngles(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->EyeAngles() : vec3_angle;
	}

	void CGabeServerFuncs::SetPlayerEyeAngles(
		CBasePlayer* pPlayer,
		const QAngle& angles)
	{
		if (pPlayer)
			pPlayer->SnapEyeAngles(angles);
	}

	int CGabeServerFuncs::GetPlayerTeam(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetTeamNumber() : TEAM_INVALID;
	}

	void CGabeServerFuncs::SetPlayerTeam(
		CBasePlayer* pPlayer,
		int team)
	{
		if (pPlayer)
			pPlayer->ChangeTeam(team);
	}

	int CGabeServerFuncs::GetPlayerFOV(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetFOV() : 0;
	}

	void CGabeServerFuncs::SetPlayerFOV(
		CBasePlayer* pPlayer,
		int fov)
	{
		if (pPlayer)
			pPlayer->SetFOV(pPlayer, fov);
	}

	bool CGabeServerFuncs::IsPlayerOnGround(CBasePlayer* pPlayer)
	{
		return pPlayer && (pPlayer->GetFlags() & FL_ONGROUND);
	}

	bool CGabeServerFuncs::IsPlayerDuck(CBasePlayer* pPlayer)
	{
		return pPlayer && (pPlayer->GetFlags() & FL_DUCKING);
	}

	void CGabeServerFuncs::EnableNoclip(CBasePlayer* pPlayer)
	{
		if (!pPlayer)
			return;

		pPlayer->SetMoveType(MOVETYPE_NOCLIP);
	}

	void CGabeServerFuncs::DisableNoclip(CBasePlayer* pPlayer)
	{
		if (!pPlayer)
			return;

		pPlayer->SetMoveType(MOVETYPE_WALK);
	}

	void CGabeServerFuncs::GiveWeapon(
		CBasePlayer* pPlayer,
		const char* weapon)
	{
		if (!pPlayer || !weapon)
			return;

		pPlayer->GiveNamedItem(weapon);
	}

	void CGabeServerFuncs::GiveAmmo(
		CBasePlayer* pPlayer,
		int amount,
		const char* ammoName)
	{
		if (!pPlayer || !ammoName)
			return;

		pPlayer->GiveAmmo(
			amount,
			ammoName,
			true
		);
	}

	CBaseCombatWeapon* CGabeServerFuncs::GetActiveWeapon(
		CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetActiveWeapon() : NULL;
	}

	bool CGabeServerFuncs::IsNoclip(CBasePlayer* pPlayer)
	{
		return pPlayer &&
			pPlayer->GetMoveType() == MOVETYPE_NOCLIP;
	}

	void CGabeServerFuncs::KillPlayer(CBasePlayer* pPlayer)
	{
		if (pPlayer)
			pPlayer->CommitSuicide();
	}

	void CGabeServerFuncs::RespawnPlayer(CBasePlayer* pPlayer)
	{
		CHL2MP_Player* pHL2Player =
			dynamic_cast<CHL2MP_Player*>(pPlayer);

		if (!pHL2Player)
			return;

		pHL2Player->Spawn();
	}

	void CGabeServerFuncs::FreezePlayer(
		CBasePlayer* pPlayer,
		bool freeze)
	{
		if (!pPlayer)
			return;

		if (freeze)
			pPlayer->AddFlag(FL_FROZEN);
		else
			pPlayer->RemoveFlag(FL_FROZEN);
	}

	void CGabeServerFuncs::SetPlayerGravity(
		CBasePlayer* pPlayer,
		float gravity)
	{
		if (pPlayer)
			pPlayer->SetGravity(gravity);
	}

	float CGabeServerFuncs::GetPlayerGravity(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetGravity() : 1.0f;
	}

	bool CGabeServerFuncs::HasSuit(CBasePlayer* pPlayer)
	{
		return pPlayer && pPlayer->IsSuitEquipped();
	}

	void CGabeServerFuncs::GiveSuit(CBasePlayer* pPlayer)
	{
		if (pPlayer)
			pPlayer->EquipSuit();
	}

	void CGabeServerFuncs::RemoveSuit(CBasePlayer* pPlayer)
	{
		if (pPlayer)
			pPlayer->RemoveSuit();
	}

	int CGabeServerFuncs::GetPlayerFlags(CBasePlayer* pPlayer)
	{
		return pPlayer ? pPlayer->GetFlags() : 0;
	}

	void CGabeServerFuncs::AddPlayerFlags(
		CBasePlayer* pPlayer,
		int flags)
	{
		if (pPlayer)
			pPlayer->AddFlag(flags);
	}

	void CGabeServerFuncs::RemovePlayerFlags(
		CBasePlayer* pPlayer,
		int flags)
	{
		if (pPlayer)
			pPlayer->RemoveFlag(flags);
	}

	bool CGabeServerFuncs::IsPlayerInVehicle(CBasePlayer* pPlayer)
	{
		return pPlayer && pPlayer->IsInAVehicle();
	}

	CBaseEntity* CGabeServerFuncs::GetPlayerVehicle(CBasePlayer* pPlayer)
	{
		if (!pPlayer || !pPlayer->IsInAVehicle())
			return NULL;

		IServerVehicle* pVehicle = pPlayer->GetServerVehicle();

		if (!pVehicle)
			return NULL;

		return pVehicle->GetVehicleEnt();
	}

	void CGabeServerFuncs::SetColor(
		CBaseEntity* pEnt,
		int r,
		int g,
		int b,
		int a)
	{
		if (!pEnt)
			return;

		pEnt->SetRenderColor(r, g, b, a);
	}

	int CGabeServerFuncs::GetColorR(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 255;

		return pEnt->GetRenderColor().r;
	}

	int CGabeServerFuncs::GetColorG(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 255;

		return pEnt->GetRenderColor().g;
	}

	int CGabeServerFuncs::GetColorB(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 255;

		return pEnt->GetRenderColor().b;
	}

	int CGabeServerFuncs::GetColorA(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 255;

		return pEnt->GetRenderColor().a;
	}

	void CGabeServerFuncs::SetAlpha(
		CBaseEntity* pEnt,
		int alpha)
	{
		if (!pEnt)
			return;

		const color32& color = pEnt->GetRenderColor();

		pEnt->SetRenderColor(
			color.r,
			color.g,
			color.b,
			alpha
		);
	}

	int CGabeServerFuncs::GetAlpha(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 255;

		return pEnt->GetRenderColor().a;
	}

	void CGabeServerFuncs::SetRenderMode(
		CBaseEntity* pEnt,
		int mode)
	{
		if (!pEnt)
			return;

		pEnt->SetRenderMode((RenderMode_t)mode);
	}

	int CGabeServerFuncs::GetRenderMode(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return 0;

		return (int)pEnt->GetRenderMode();
	}

	void CGabeServerFuncs::Hide(CBaseEntity* pEnt)
	{
		if (pEnt)
			pEnt->AddEffects(EF_NODRAW);
	}

	void CGabeServerFuncs::Show(CBaseEntity* pEnt)
	{
		if (pEnt)
			pEnt->RemoveEffects(EF_NODRAW);
	}

	bool CGabeServerFuncs::IsHidden(CBaseEntity* pEnt)
	{
		return pEnt &&
			pEnt->IsEffectActive(EF_NODRAW);
	}

	const char* CGabeServerFuncs::GetModelName(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return "";

		return STRING(pEnt->GetModelName());
	}

	int CGabeServerFuncs::GetFlags(CBaseEntity* pEnt)
	{
		return pEnt ? pEnt->GetFlags() : 0;
	}

	void CGabeServerFuncs::AddFlags(
		CBaseEntity* pEnt,
		int flags)
	{
		if (pEnt)
			pEnt->AddFlag(flags);
	}

	void CGabeServerFuncs::RemoveFlags(
		CBaseEntity* pEnt,
		int flags)
	{
		if (pEnt)
			pEnt->RemoveFlag(flags);
	}

	int CGabeServerFuncs::GetMoveType(CBaseEntity* pEnt)
	{
		if (!pEnt)
			return MOVETYPE_NONE;

		return (int)pEnt->GetMoveType();
	}

	float CGabeServerFuncs::DistanceToEntity(
		CBaseEntity* a,
		CBaseEntity* b)
	{
		if (!a || !b)
			return -1.0f;

		return a->WorldSpaceCenter().DistTo(
			b->WorldSpaceCenter()
		);
	}

	Vector CGabeServerFuncs::DirectionToEntity(
		CBaseEntity* from,
		CBaseEntity* to)
	{
		if (!from || !to)
			return vec3_origin;

		Vector direction =
			to->WorldSpaceCenter() -
			from->WorldSpaceCenter();

		direction.NormalizeInPlace();

		return direction;
	}

	bool CGabeServerFuncs::IsValidEntity(CBaseEntity* pEnt)
	{
		return pEnt != NULL;
	}

	////////////////////////
	//  Display a message //
	////////////////////////
	void CGabeServerFuncs::HudText(CBasePlayer* pPlayer,
		const char* pszText,
		int r, int g, int b,
		float x, float y,
		float fadeIn, float fadeOut,
		float hold, int channel,
		int effect)
	{
		if (!pPlayer || !pszText)
			return;

		hudtextparms_t params;
		params.channel = channel;
		params.x = x;
		params.y = y;
		params.effect = effect;
		params.r1 = r;
		params.g1 = g;
		params.b1 = b;
		params.a1 = 255;
		params.r2 = 255;
		params.g2 = 255;
		params.b2 = 255;
		params.a2 = 255;
		params.fadeinTime = fadeIn;
		params.fadeoutTime = fadeOut;
		params.holdTime = hold;
		params.fxTime = 0.5f;

		UTIL_HudMessage(pPlayer, params, pszText);
	}
#endif

#ifdef CLIENT_DLL

	bool CGabeClientFuncs::IsPlayerUnderwater()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->IsPlayerUnderwater();
	}

	int CGabeClientFuncs::GetObserverMode()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetObserverMode()
			: OBS_MODE_NONE;
	}

	bool CGabeClientFuncs::IsObserver()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->IsObserver();
	}

	C_BaseEntity* CGabeClientFuncs::GetObserverTarget()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetObserverTarget()
			: NULL;
	}

	C_BaseCombatWeapon* CGabeClientFuncs::GetActiveWeapon()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetActiveWeapon()
			: NULL;
	}

	C_BaseViewModel* CGabeClientFuncs::GetViewModel(int index)
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetViewModel(index)
			: NULL;
	}

	const char* CGabeClientFuncs::GetPlayerName()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetPlayerName()
			: "";
	}

	int CGabeClientFuncs::GetUserID()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetUserID()
			: -1;
	}

	bool CGabeClientFuncs::IsFirstPerson()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->InFirstPersonView();
	}

	bool CGabeClientFuncs::ShouldDrawPlayer()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->ShouldDrawThisPlayer();
	}

	float CGabeClientFuncs::GetPlayerMaxSpeed()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetPlayerMaxSpeed()
			: 0.0f;
	}

	float CGabeClientFuncs::GetStoredMaxSpeed()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->MaxSpeed()
			: 0.0f;
	}

	int CGabeClientFuncs::GetFOV()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? (int)pPlayer->GetFOV()
			: 0;
	}

	int CGabeClientFuncs::GetDefaultFOV()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetDefaultFOV()
			: 0;
	}

	bool CGabeClientFuncs::IsZoomed()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->IsZoomed();
	}

	Vector CGabeClientFuncs::GetShootPosition()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->Weapon_ShootPosition()
			: vec3_origin;
	}

	Vector CGabeClientFuncs::GetEyeForward()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return vec3_origin;

		Vector forward;
		pPlayer->EyeVectors(&forward);

		return forward;
	}

	Vector CGabeClientFuncs::GetEyeRight()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return vec3_origin;

		Vector right;
		pPlayer->EyeVectors(NULL, &right, NULL);

		return right;
	}

	Vector CGabeClientFuncs::GetEyeUp()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return vec3_origin;

		Vector up;
		pPlayer->EyeVectors(NULL, NULL, &up);

		return up;
	}

	QAngle CGabeClientFuncs::GetLocalEyeAngles()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->LocalEyeAngles()
			: vec3_angle;
	}

	QAngle CGabeClientFuncs::GetPunchAngle()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetPunchAngle()
			: vec3_angle;
	}

	bool CGabeClientFuncs::HasSuit()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->IsSuitEquipped();
	}

	bool CGabeClientFuncs::IsInVehicle()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->IsInAVehicle();
	}

	C_BaseEntity* CGabeClientFuncs::GetVehicleEntity()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer || !pPlayer->IsInAVehicle())
			return NULL;

		IClientVehicle* pVehicle = pPlayer->GetVehicle();

		if (!pVehicle)
			return NULL;

		return pVehicle->GetVehicleEnt();
	}

	C_BaseEntity* CGabeClientFuncs::GetUseEntity()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetUseEntity()
			: NULL;
	}

	float CGabeClientFuncs::GetTimeBase()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetTimeBase()
			: 0.0f;
	}

	float CGabeClientFuncs::GetFinalPredictedTime()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetFinalPredictedTime()
			: 0.0f;
	}

	int CGabeClientFuncs::GetCurrentCommandNumber()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->CurrentCommandNumber()
			: -1;
	}

	bool CGabeClientFuncs::HasFiredWeapon()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer &&
			pPlayer->HasFiredWeapon();
	}

	float CGabeClientFuncs::GetDeathTime()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetDeathTime()
			: 0.0f;
	}

	int CGabeClientFuncs::GetScreenWidth()
	{
		int width, height;
		engine->GetScreenSize(width, height);

		return width;
	}

	int CGabeClientFuncs::GetScreenHeight()
	{
		int width, height;
		engine->GetScreenSize(width, height);

		return height;
	}

	static float g_flGabeAspectRatio = 0.0f;

	float CGabeClientFuncs::GetAspectRatio()
	{
		if (g_flGabeAspectRatio > 0.0f)
			return g_flGabeAspectRatio;

		int width, height;
		engine->GetScreenSize(width, height);

		if (height <= 0)
			return 1.0f;

		return (float)width / (float)height;
	}

	void CGabeClientFuncs::SetAspectRatio(float ratio)
	{
		if (ratio <= 0.0f)
		{
			g_flGabeAspectRatio = 0.0f;
			return;
		}

		g_flGabeAspectRatio = ratio;
	}

	float CGabeClientFuncs::GetViewFOV()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		return pPlayer
			? pPlayer->GetFOV()
			: 90.0f;
	}

	void CGabeClientFuncs::SetViewFOV(float fov)
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return;

		pPlayer->SetFOV(
			pPlayer,
			(int)fov
		);
	}

	QAngle CGabeClientFuncs::GetViewAngles()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return vec3_angle;

		return pPlayer->pl.v_angle;
	}

	void CGabeClientFuncs::SetPlayerRenderColor(int r, int g, int b, int a)
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return;

		pPlayer->SetRenderColor(
			clamp(r, 0, 255),
			clamp(g, 0, 255),
			clamp(b, 0, 255),
			clamp(a, 0, 255)
		);
	}

	void CGabeClientFuncs::SetViewAngles(
		const QAngle& angles)
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return;

		pPlayer->pl.v_angle = angles;
	}

	const char* CGabeClientFuncs::GetScreenOverlay()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return "";

		return pPlayer->GetScriptOverlayMaterial();
	}

	bool CGabeClientFuncs::HasScreenOverlay()
	{
		const char* overlay = GetScreenOverlay();

		return overlay &&
			overlay[0] != '\0';
	}

	void CGabeClientFuncs::SetScreenOverlay(
		const char* material)
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return;

		pPlayer->SetScriptOverlayMaterial(material);
	}

	void CGabeClientFuncs::ClearScreenOverlay()
	{
		C_BasePlayer* pPlayer = GetPlayer();

		if (!pPlayer)
			return;

		pPlayer->SetScriptOverlayMaterial("");
	}
#endif

}

#ifndef CLIENT_DLL

class CGabeModVScript
{
public:
	DECLARE_ENT_SCRIPTDESC();
	DECLARE_CLASS_NOBASE(CGabeModVScript);

public:
	// =========================
	// Player
	// =========================

	HSCRIPT GetPlayer()
	{
		return ToHScript(GabeMod::CGabeServerFuncs::GetPlayer());
	}

	// =========================
	// Messaging
	// =========================

	void PrintToYou(HSCRIPT hPlayer, int dest, const char* msg)
	{
		CBasePlayer* pPlayer = ScriptToEntClass<CBasePlayer>(hPlayer);

		if (!pPlayer)
			return;

		GabeMod::CGabeServerFuncs::PrintToYou(pPlayer, dest, msg);
	}

	void PrintToAllPlayers(int dest, const char* msg)
	{
		GabeMod::CGabeServerFuncs::PrintToAllPlayers(dest, msg);
	}

	// =========================
	// Debug
	// =========================

	void ScreenText(
		const char* text,
		float x,
		float y,
		float time,
		int r,
		int g,
		int b,
		int a)
	{
		GabeMod::CGabeServerFuncs::ScreenText(
			text, x, y, time, r, g, b, a);
	}

	void DebugLine(
		const Vector& start,
		const Vector& end,
		float time)
	{
		GabeMod::CGabeServerFuncs::DebugLine(
			start, end, time);
	}

	void DebugBox(
		const Vector& pos,
		float size,
		float time)
	{
		GabeMod::CGabeServerFuncs::DebugBox(
			pos, size, time);
	}

	// =========================
	// Player vectors
	// =========================

	Vector Forward(HSCRIPT hPlayer)
	{
		CBasePlayer* pPlayer =
			ScriptToEntClass<CBasePlayer>(hPlayer);

		if (!pPlayer)
			return vec3_origin;

		return GabeMod::CGabeServerFuncs::Forward(pPlayer);
	}

	Vector ShootPos(HSCRIPT hPlayer)
	{
		CBasePlayer* pPlayer =
			ScriptToEntClass<CBasePlayer>(hPlayer);

		if (!pPlayer)
			return vec3_origin;

		return GabeMod::CGabeServerFuncs::ShootPos(pPlayer);
	}

	// =========================
	// Trace
	// =========================

	HSCRIPT TraceEntity(HSCRIPT hPlayer, float dist)
	{
		CBasePlayer* pPlayer =
			ScriptToEntClass<CBasePlayer>(hPlayer);

		if (!pPlayer)
			return NULL;

		return ToHScript(
			GabeMod::CGabeServerFuncs::TraceEntity(
				pPlayer,
				dist));
	}

	// =========================
	// Physics
	// =========================

	void ApplyForce(
		HSCRIPT hEntity,
		const Vector& force)
	{
		CBaseEntity* pEnt = ToEnt(hEntity);

		if (!pEnt)
			return;

		GabeMod::CGabeServerFuncs::ApplyForce(
			pEnt,
			force);
	}

	void SetMass(
		HSCRIPT hEntity,
		float mass)
	{
		CBaseEntity* pEnt = ToEnt(hEntity);

		if (!pEnt)
			return;

		GabeMod::CGabeServerFuncs::SetMass(
			pEnt,
			mass);
	}

	// =========================
	// Spawning
	// =========================

	HSCRIPT SpawnProp(
		const char* model,
		const Vector& pos,
		const QAngle& ang)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::SpawnProp(
				model,
				pos,
				ang));
	}

	HSCRIPT SpawnEntity(
		const char* classname,
		const Vector& pos,
		const QAngle& ang)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::SpawnEntity(
				classname,
				pos,
				ang));
	}

	// =========================
	// Entity helpers
	// =========================

	HSCRIPT FindByName(const char* name)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::FindByName(name));
	}

	void Remove(HSCRIPT hEntity)
	{
		CBaseEntity* pEnt = ToEnt(hEntity);

		if (!pEnt)
			return;

		GabeMod::CGabeServerFuncs::Remove(pEnt);
	}

	// =========================
	// Gameplay
	// =========================

	void GiveAllWeapons(HSCRIPT hPlayer)
	{
		CHL2MP_Player* pPlayer =
			ScriptToEntClass<CHL2MP_Player>(hPlayer);

		if (!pPlayer)
			return;

		GabeMod::CGabeServerFuncs::GiveAllWeapons(pPlayer);
	}

	void Heal(HSCRIPT hPlayer, int amount)
	{
		CBasePlayer* pPlayer =
			ScriptToEntClass<CBasePlayer>(hPlayer);

		if (!pPlayer)
			return;

		GabeMod::CGabeServerFuncs::Heal(
			pPlayer,
			amount);
	}

	void Damage(HSCRIPT hPlayer, int amount)
	{
		CBasePlayer* pPlayer =
			ScriptToEntClass<CBasePlayer>(hPlayer);

		if (!pPlayer)
			return;

		GabeMod::CGabeServerFuncs::Damage(
			pPlayer,
			amount);
	}

	// =========================
	// Advanced entity helpers
	// =========================

	HSCRIPT Gabe_FindByName(const char* name)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::Gabe_FindByName(
				name));
	}

	HSCRIPT Gabe_FindByClass(const char* classname)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::Gabe_FindByClass(
				classname));
	}

	HSCRIPT Gabe_FindClosestEntity(
		const Vector& origin,
		const char* classname,
		float maxDist)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::Gabe_FindClosestEntity(
				origin,
				classname,
				maxDist));
	}

	// =========================
	// Damage
	// =========================

	void Gabe_DealDamage(
		HSCRIPT hVictim,
		HSCRIPT hAttacker,
		float damage,
		int damageType)
	{
		CBaseEntity* victim = ToEnt(hVictim);
		CBaseEntity* attacker = ToEnt(hAttacker);

		if (!victim)
			return;

		GabeMod::CGabeServerFuncs::Gabe_DealDamage(
			victim,
			attacker,
			damage,
			damageType);
	}

	void Gabe_RadiusDamage(
		const Vector& origin,
		HSCRIPT hAttacker,
		float damage,
		float radius,
		int damageType)
	{
		CBaseEntity* attacker = ToEnt(hAttacker);

		GabeMod::CGabeServerFuncs::Gabe_RadiusDamage(
			origin,
			attacker,
			damage,
			radius,
			damageType);
	}

	// =========================
	// Visibility
	// =========================

	bool Gabe_HasLineOfSight(
		HSCRIPT hFrom,
		HSCRIPT hTo)
	{
		CBaseEntity* from = ToEnt(hFrom);
		CBaseEntity* to = ToEnt(hTo);

		if (!from || !to)
			return false;

		return GabeMod::CGabeServerFuncs::Gabe_HasLineOfSight(
			from,
			to);
	}

	bool Gabe_IsInFOV(
		HSCRIPT hViewer,
		HSCRIPT hTarget,
		float fovDegrees)
	{
		CBaseEntity* viewer = ToEnt(hViewer);
		CBaseEntity* target = ToEnt(hTarget);

		if (!viewer || !target)
			return false;

		return GabeMod::CGabeServerFuncs::Gabe_IsInFOV(
			viewer,
			target,
			fovDegrees);
	}

	// =========================
	// HUD
	// =========================

	void HudText(
		HSCRIPT hPlayer,
		const char* text,
		int r,
		int g,
		int b,
		float x,
		float y,
		float fadeIn,
		float fadeOut,
		float hold,
		int channel,
		int effect)
	{
		CBasePlayer* pPlayer =
			ScriptToEntClass<CBasePlayer>(hPlayer);

		if (!pPlayer)
			return;

		GabeMod::CGabeServerFuncs::HudText(
			pPlayer,
			text,
			r,
			g,
			b,
			x,
			y,
			fadeIn,
			fadeOut,
			hold,
			channel,
			effect);
	}

	// ============================================================
// Entity state
// ============================================================

	Vector GetOrigin(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetOrigin(ToEnt(hEnt));
	}

	QAngle GetAngles(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetAngles(ToEnt(hEnt));
	}

	void SetOrigin(HSCRIPT hEnt, const Vector& pos)
	{
		GabeMod::CGabeServerFuncs::SetOrigin(ToEnt(hEnt), pos);
	}

	void SetAngles(HSCRIPT hEnt, const QAngle& ang)
	{
		GabeMod::CGabeServerFuncs::SetAngles(ToEnt(hEnt), ang);
	}

	Vector GetVelocity(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetVelocity(ToEnt(hEnt));
	}

	void SetVelocity(HSCRIPT hEnt, const Vector& velocity)
	{
		GabeMod::CGabeServerFuncs::SetVelocity(ToEnt(hEnt), velocity);
	}

	void AddVelocity(HSCRIPT hEnt, const Vector& velocity)
	{
		GabeMod::CGabeServerFuncs::AddVelocity(ToEnt(hEnt), velocity);
	}

	int GetHealth(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetHealth(ToEnt(hEnt));
	}

	int GetMaxHealth(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetMaxHealth(ToEnt(hEnt));
	}

	void SetHealth(HSCRIPT hEnt, int health)
	{
		GabeMod::CGabeServerFuncs::SetHealth(ToEnt(hEnt), health);
	}

	void SetMaxHealth(HSCRIPT hEnt, int health)
	{
		GabeMod::CGabeServerFuncs::SetMaxHealth(ToEnt(hEnt), health);
	}

	bool IsAlive(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::IsAlive(ToEnt(hEnt));
	}

	bool IsPlayer(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::IsPlayer(ToEnt(hEnt));
	}

	bool IsNPC(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::IsNPC(ToEnt(hEnt));
	}

	const char* GetClassname(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetClassname(ToEnt(hEnt));
	}

	const char* GetEntityName(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetEntityName(ToEnt(hEnt));
	}

	int GetEntIndex(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetEntIndex(ToEnt(hEnt));
	}

	void SetName(HSCRIPT hEnt, const char* name)
	{
		GabeMod::CGabeServerFuncs::SetName(ToEnt(hEnt), name);
	}

	void SetModel(HSCRIPT hEnt, const char* model)
	{
		GabeMod::CGabeServerFuncs::SetModel(ToEnt(hEnt), model);
	}

	void Activate(HSCRIPT hEnt)
	{
		GabeMod::CGabeServerFuncs::Activate(ToEnt(hEnt));
	}

	void Teleport(
		HSCRIPT hEnt,
		const Vector& pos,
		const QAngle& ang,
		const Vector& velocity)
	{
		GabeMod::CGabeServerFuncs::Teleport(
			ToEnt(hEnt),
			pos,
			ang,
			velocity);
	}

	// ============================================================
	// Effects / flags
	// ============================================================

	void AddEffects(HSCRIPT hEnt, int effects)
	{
		GabeMod::CGabeServerFuncs::AddEffects(ToEnt(hEnt), effects);
	}

	void RemoveEffects(HSCRIPT hEnt, int effects)
	{
		GabeMod::CGabeServerFuncs::RemoveEffects(ToEnt(hEnt), effects);
	}

	bool HasEffects(HSCRIPT hEnt, int effects)
	{
		return GabeMod::CGabeServerFuncs::HasEffects(ToEnt(hEnt), effects);
	}

	void AddEFlags(HSCRIPT hEnt, int flags)
	{
		GabeMod::CGabeServerFuncs::AddEFlags(ToEnt(hEnt), flags);
	}

	void RemoveEFlags(HSCRIPT hEnt, int flags)
	{
		GabeMod::CGabeServerFuncs::RemoveEFlags(ToEnt(hEnt), flags);
	}

	bool HasEFlags(HSCRIPT hEnt, int flags)
	{
		return GabeMod::CGabeServerFuncs::HasEFlags(ToEnt(hEnt), flags);
	}

	void SetSolid(HSCRIPT hEnt, bool solid)
	{
		GabeMod::CGabeServerFuncs::SetSolid(ToEnt(hEnt), solid);
	}

	void SetMoveType(HSCRIPT hEnt, int moveType)
	{
		GabeMod::CGabeServerFuncs::SetMoveType(ToEnt(hEnt), moveType);
	}

	void SetCollisionGroup(HSCRIPT hEnt, int group)
	{
		GabeMod::CGabeServerFuncs::SetCollisionGroup(ToEnt(hEnt), group);
	}

	int GetCollisionGroup(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetCollisionGroup(ToEnt(hEnt));
	}

	// ============================================================
	// Parenting / ownership
	// ============================================================

	void SetParent(HSCRIPT hEnt, HSCRIPT hParent)
	{
		GabeMod::CGabeServerFuncs::SetParent(
			ToEnt(hEnt),
			ToEnt(hParent));
	}

	void ClearParent(HSCRIPT hEnt)
	{
		GabeMod::CGabeServerFuncs::ClearParent(ToEnt(hEnt));
	}

	HSCRIPT GetParent(HSCRIPT hEnt)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::GetParent(ToEnt(hEnt)));
	}

	HSCRIPT GetOwner(HSCRIPT hEnt)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::GetOwner(ToEnt(hEnt)));
	}

	void SetOwner(HSCRIPT hEnt, HSCRIPT hOwner)
	{
		GabeMod::CGabeServerFuncs::SetOwner(
			ToEnt(hEnt),
			ToEnt(hOwner));
	}

	// ============================================================
	// Entity searching
	// ============================================================

	HSCRIPT FindByClass(const char* classname)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::FindByClass(classname));
	}

	HSCRIPT FindByNameAfter(HSCRIPT hStart, const char* name)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::FindByNameAfter(
				ToEnt(hStart),
				name));
	}

	HSCRIPT FindByClassAfter(HSCRIPT hStart, const char* classname)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::FindByClassAfter(
				ToEnt(hStart),
				classname));
	}

	HSCRIPT FindInSphere(
		HSCRIPT hStart,
		const Vector& origin,
		float radius)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::FindInSphere(
				ToEnt(hStart),
				origin,
				radius));
	}

	// ============================================================
	// Player
	// ============================================================

	HSCRIPT GetPlayerByIndex(int index)
	{
		return ToHScript(
			GabeMod::CGabeServerFuncs::GetPlayerByIndex(index));
	}

	int GetPlayerCount()
	{
		return GabeMod::CGabeServerFuncs::GetPlayerCount();
	}

	void SetArmor(HSCRIPT hPlayer, int armor)
	{
		GabeMod::CGabeServerFuncs::SetArmor(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			armor);
	}

	int GetArmor(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetArmor(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetFOV(HSCRIPT hPlayer, int fov)
	{
		GabeMod::CGabeServerFuncs::SetFOV(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			fov);
	}

	Vector GetEyePosition(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetEyePosition(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	QAngle GetEyeAngles(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetEyeAngles(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	// ============================================================
	// Physics
	// ============================================================

	bool HasPhysics(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::HasPhysics(ToEnt(hEnt));
	}

	void WakePhysics(HSCRIPT hEnt)
	{
		GabeMod::CGabeServerFuncs::WakePhysics(ToEnt(hEnt));
	}

	void SleepPhysics(HSCRIPT hEnt)
	{
		GabeMod::CGabeServerFuncs::SleepPhysics(ToEnt(hEnt));
	}

	float GetMass(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetMass(ToEnt(hEnt));
	}

	Vector GetPhysicsVelocity(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetPhysicsVelocity(ToEnt(hEnt));
	}

	void SetPhysicsVelocity(
		HSCRIPT hEnt,
		const Vector& velocity)
	{
		GabeMod::CGabeServerFuncs::SetPhysicsVelocity(
			ToEnt(hEnt),
			velocity);
	}

	void ApplyTorque(
		HSCRIPT hEnt,
		const Vector& torque)
	{
		GabeMod::CGabeServerFuncs::ApplyTorque(
			ToEnt(hEnt),
			torque);
	}

	// ============================================================
	// Damage / fire
	// ============================================================

	void KillEntity(
		HSCRIPT hVictim,
		HSCRIPT hAttacker)
	{
		GabeMod::CGabeServerFuncs::KillEntity(
			ToEnt(hVictim),
			ToEnt(hAttacker));
	}

	void Ignite(HSCRIPT hEnt, float time)
	{
		GabeMod::CGabeServerFuncs::Ignite(
			ToEnt(hEnt),
			time);
	}

	void Extinguish(HSCRIPT hEnt)
	{
		GabeMod::CGabeServerFuncs::Extinguish(ToEnt(hEnt));
	}

	// ============================================================
	// Math / spatial
	// ============================================================

	float Distance(
		const Vector& a,
		const Vector& b)
	{
		return GabeMod::CGabeServerFuncs::Distance(a, b);
	}

	float DistanceEntities(
		HSCRIPT hA,
		HSCRIPT hB)
	{
		return GabeMod::CGabeServerFuncs::DistanceEntities(
			ToEnt(hA),
			ToEnt(hB));
	}

	Vector DirectionTo(
		HSCRIPT hFrom,
		HSCRIPT hTo)
	{
		return GabeMod::CGabeServerFuncs::DirectionTo(
			ToEnt(hFrom),
			ToEnt(hTo));
	}

	Vector GetCenter(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetCenter(ToEnt(hEnt));
	}

	// ============================================================
	// Sound
	// ============================================================

	void EmitSound(
		HSCRIPT hEnt,
		const char* soundName)
	{
		GabeMod::CGabeServerFuncs::EmitSound(
			ToEnt(hEnt),
			soundName);
	}

	void StopSound(
		HSCRIPT hEnt,
		const char* soundName)
	{
		GabeMod::CGabeServerFuncs::StopSound(
			ToEnt(hEnt),
			soundName);
	}

	// ============================================================
	// Entity I/O
	// ============================================================

	bool FireInput(
		HSCRIPT hEnt,
		const char* input)
	{
		return GabeMod::CGabeServerFuncs::FireInput(
			ToEnt(hEnt),
			input);
	}

	bool FireInputString(
		HSCRIPT hEnt,
		const char* input,
		const char* value)
	{
		return GabeMod::CGabeServerFuncs::FireInputString(
			ToEnt(hEnt),
			input,
			value);
	}

	// ============================================================
	// Misc
	// ============================================================

	float RandomFloat(float min, float max)
	{
		return GabeMod::CGabeServerFuncs::RandomFloat(min, max);
	}

	int RandomInt(int min, int max)
	{
		return GabeMod::CGabeServerFuncs::RandomInt(min, max);
	}

	float GetTime()
	{
		return GabeMod::CGabeServerFuncs::GetTime();
	}

	int GetTickCountA()
	{
		return GabeMod::CGabeServerFuncs::GetTickCountA();
	}

	float GetTickInterval()
	{
		return GabeMod::CGabeServerFuncs::GetTickInterval();
	}

	// ============================================================
// Player helpers
// ============================================================

	bool IsPlayerAlive(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::IsPlayerAlive(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	int GetPlayerHealth(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerHealth(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetPlayerHealth(HSCRIPT hPlayer, int health)
	{
		GabeMod::CGabeServerFuncs::SetPlayerHealth(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			health);
	}

	int GetPlayerArmor(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerArmor(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetPlayerArmor(HSCRIPT hPlayer, int armor)
	{
		GabeMod::CGabeServerFuncs::SetPlayerArmor(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			armor);
	}

	Vector GetPlayerOrigin(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerOrigin(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetPlayerOrigin(
		HSCRIPT hPlayer,
		const Vector& origin)
	{
		GabeMod::CGabeServerFuncs::SetPlayerOrigin(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			origin);
	}

	Vector GetPlayerVelocity(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerVelocity(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetPlayerVelocity(
		HSCRIPT hPlayer,
		const Vector& velocity)
	{
		GabeMod::CGabeServerFuncs::SetPlayerVelocity(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			velocity);
	}

	void AddPlayerVelocity(
		HSCRIPT hPlayer,
		const Vector& velocity)
	{
		GabeMod::CGabeServerFuncs::AddPlayerVelocity(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			velocity);
	}

	Vector GetPlayerEyePosition(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerEyePosition(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	QAngle GetPlayerEyeAngles(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerEyeAngles(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetPlayerEyeAngles(
		HSCRIPT hPlayer,
		const QAngle& angles)
	{
		GabeMod::CGabeServerFuncs::SetPlayerEyeAngles(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			angles);
	}

	int GetPlayerTeam(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerTeam(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetPlayerTeam(
		HSCRIPT hPlayer,
		int team)
	{
		GabeMod::CGabeServerFuncs::SetPlayerTeam(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			team);
	}

	int GetPlayerFOV(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerFOV(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void SetPlayerFOV(
		HSCRIPT hPlayer,
		int fov)
	{
		GabeMod::CGabeServerFuncs::SetPlayerFOV(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			fov);
	}

	bool IsPlayerOnGround(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::IsPlayerOnGround(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	bool IsPlayerDuck(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::IsPlayerDuck(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void EnableNoclip(HSCRIPT hPlayer)
	{
		GabeMod::CGabeServerFuncs::EnableNoclip(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void DisableNoclip(HSCRIPT hPlayer)
	{
		GabeMod::CGabeServerFuncs::DisableNoclip(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	bool IsNoclip(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::IsNoclip(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void GiveWeapon(
		HSCRIPT hPlayer,
		const char* weapon)
	{
		GabeMod::CGabeServerFuncs::GiveWeapon(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			weapon);
	}

	void GiveAmmo(
		HSCRIPT hPlayer,
		int amount,
		const char* ammoName)
	{
		GabeMod::CGabeServerFuncs::GiveAmmo(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			amount,
			ammoName);
	}

	void StripWeapons(HSCRIPT hPlayer)
	{
		GabeMod::CGabeServerFuncs::StripWeapons(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	HSCRIPT GetActiveWeapon(HSCRIPT hPlayer)
	{
		CBaseCombatWeapon* pWeapon =
			GabeMod::CGabeServerFuncs::GetActiveWeapon(
				ScriptToEntClass<CBasePlayer>(hPlayer));

		return ToHScript(pWeapon);
	}

	void KillPlayer(HSCRIPT hPlayer)
	{
		GabeMod::CGabeServerFuncs::KillPlayer(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void RespawnPlayer(HSCRIPT hPlayer)
	{
		GabeMod::CGabeServerFuncs::RespawnPlayer(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void FreezePlayer(
		HSCRIPT hPlayer,
		bool freeze)
	{
		GabeMod::CGabeServerFuncs::FreezePlayer(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			freeze);
	}

	void SetPlayerGravity(
		HSCRIPT hPlayer,
		float gravity)
	{
		GabeMod::CGabeServerFuncs::SetPlayerGravity(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			gravity);
	}

	float GetPlayerGravity(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerGravity(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	bool HasSuit(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::HasSuit(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void GiveSuit(HSCRIPT hPlayer)
	{
		GabeMod::CGabeServerFuncs::GiveSuit(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void RemoveSuit(HSCRIPT hPlayer)
	{
		GabeMod::CGabeServerFuncs::RemoveSuit(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	int GetPlayerFlags(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::GetPlayerFlags(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	void AddPlayerFlags(
		HSCRIPT hPlayer,
		int flags)
	{
		GabeMod::CGabeServerFuncs::AddPlayerFlags(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			flags);
	}

	void RemovePlayerFlags(
		HSCRIPT hPlayer,
		int flags)
	{
		GabeMod::CGabeServerFuncs::RemovePlayerFlags(
			ScriptToEntClass<CBasePlayer>(hPlayer),
			flags);
	}

	bool IsPlayerInVehicle(HSCRIPT hPlayer)
	{
		return GabeMod::CGabeServerFuncs::IsPlayerInVehicle(
			ScriptToEntClass<CBasePlayer>(hPlayer));
	}

	HSCRIPT GetPlayerVehicle(HSCRIPT hPlayer)
	{
		CBaseEntity* pVehicle =
			GabeMod::CGabeServerFuncs::GetPlayerVehicle(
				ScriptToEntClass<CBasePlayer>(hPlayer));

		return ToHScript(pVehicle);
	}

	// ============================================================
// Render / color
// ============================================================

	void SetColor(
		HSCRIPT hEnt,
		int r,
		int g,
		int b,
		int a)
	{
		GabeMod::CGabeServerFuncs::SetColor(
			ToEnt(hEnt),
			r,
			g,
			b,
			a);
	}

	int GetColorR(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetColorR(
			ToEnt(hEnt));
	}

	int GetColorG(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetColorG(
			ToEnt(hEnt));
	}

	int GetColorB(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetColorB(
			ToEnt(hEnt));
	}

	int GetColorA(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetColorA(
			ToEnt(hEnt));
	}

	void SetAlpha(
		HSCRIPT hEnt,
		int alpha)
	{
		GabeMod::CGabeServerFuncs::SetAlpha(
			ToEnt(hEnt),
			alpha);
	}

	int GetAlpha(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetAlpha(
			ToEnt(hEnt));
	}

	void SetRenderMode(
		HSCRIPT hEnt,
		int mode)
	{
		GabeMod::CGabeServerFuncs::SetRenderMode(
			ToEnt(hEnt),
			mode);
	}

	int GetRenderMode(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetRenderMode(
			ToEnt(hEnt));
	}

	void Hide(HSCRIPT hEnt)
	{
		GabeMod::CGabeServerFuncs::Hide(
			ToEnt(hEnt));
	}

	void Show(HSCRIPT hEnt)
	{
		GabeMod::CGabeServerFuncs::Show(
			ToEnt(hEnt));
	}

	bool IsHidden(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::IsHidden(
			ToEnt(hEnt));
	}

	const char* GetModelName(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetModelName(
			ToEnt(hEnt));
	}

	int GetFlags(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetFlags(
			ToEnt(hEnt));
	}

	void AddFlags(
		HSCRIPT hEnt,
		int flags)
	{
		GabeMod::CGabeServerFuncs::AddFlags(
			ToEnt(hEnt),
			flags);
	}

	void RemoveFlags(
		HSCRIPT hEnt,
		int flags)
	{
		GabeMod::CGabeServerFuncs::RemoveFlags(
			ToEnt(hEnt),
			flags);
	}

	int GetMoveType(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::GetMoveType(
			ToEnt(hEnt));
	}

	float DistanceToEntity(
		HSCRIPT hA,
		HSCRIPT hB)
	{
		return GabeMod::CGabeServerFuncs::DistanceToEntity(
			ToEnt(hA),
			ToEnt(hB));
	}

	Vector DirectionToEntity(
		HSCRIPT hFrom,
		HSCRIPT hTo)
	{
		return GabeMod::CGabeServerFuncs::DirectionToEntity(
			ToEnt(hFrom),
			ToEnt(hTo));
	}

	bool IsValidEntity(HSCRIPT hEnt)
	{
		return GabeMod::CGabeServerFuncs::IsValidEntity(
			ToEnt(hEnt));
	}
};
#endif

#ifndef CLIENT_DLL

	BEGIN_ENT_SCRIPTDESC_ROOT_NAMED(CGabeModVScript, "Gabe_Server", SCRIPT_SINGLETON "Gabe Mod server-side utility funcs")

		// Player
		DEFINE_SCRIPTFUNC(GetPlayer,
			"Returns a player.");

	// Messaging
	DEFINE_SCRIPTFUNC(PrintToYou,
		"Prints a message to a player.");

	DEFINE_SCRIPTFUNC(PrintToAllPlayers,
		"Prints a message to all players.");

	// Debug
	DEFINE_SCRIPTFUNC(ScreenText,
		"Draws debug text on the screen.");

	DEFINE_SCRIPTFUNC(DebugLine,
		"Draws a debug line.");

	DEFINE_SCRIPTFUNC(DebugBox,
		"Draws a debug box.");

	// Player vectors
	DEFINE_SCRIPTFUNC(Forward,
		"Returns the player's forward vector.");

	DEFINE_SCRIPTFUNC(ShootPos,
		"Returns the player's weapon shoot position.");

	// Tracing
	DEFINE_SCRIPTFUNC(TraceEntity,
		"Traces from a player and returns the hit entity.");

	// Physics
	DEFINE_SCRIPTFUNC(ApplyForce,
		"Applies force to an entity.");

	DEFINE_SCRIPTFUNC(SetMass,
		"Sets an entity's physics mass.");

	// Spawning
	DEFINE_SCRIPTFUNC(SpawnProp,
		"Spawns a prop_physics.");

	DEFINE_SCRIPTFUNC(SpawnEntity,
		"Spawns an entity by classname.");

	// Entity helpers
	DEFINE_SCRIPTFUNC(FindByName,
		"Finds an entity by targetname.");

	DEFINE_SCRIPTFUNC(Remove,
		"Removes an entity.");

	// Gameplay
	DEFINE_SCRIPTFUNC(GiveAllWeapons,
		"Gives an HL2MP player all weapons.");

	DEFINE_SCRIPTFUNC(Heal,
		"Heals a player.");

	DEFINE_SCRIPTFUNC(Damage,
		"Damages a player.");

	// Advanced entity searching
	DEFINE_SCRIPTFUNC(Gabe_FindByName,
		"Finds an entity by targetname.");

	DEFINE_SCRIPTFUNC(Gabe_FindByClass,
		"Finds an entity by classname.");

	DEFINE_SCRIPTFUNC(Gabe_FindClosestEntity,
		"Finds the closest entity of a classname.");

	// Damage
	DEFINE_SCRIPTFUNC(Gabe_DealDamage,
		"Deals damage to an entity.");

	DEFINE_SCRIPTFUNC(Gabe_RadiusDamage,
		"Deals radius damage.");

	// Visibility
	DEFINE_SCRIPTFUNC(Gabe_HasLineOfSight,
		"Checks whether one entity has line of sight to another.");

	DEFINE_SCRIPTFUNC(Gabe_IsInFOV,
		"Checks whether an entity is inside another entity's field of view.");

	// HUD
	DEFINE_SCRIPTFUNC(HudText,
		"Displays HUD text to a player.");

	// =========================================================
// Entity state
// =========================================================

	DEFINE_SCRIPTFUNC(GetOrigin,
		"Returns an entity's world origin.");

	DEFINE_SCRIPTFUNC(GetAngles,
		"Returns an entity's world angles.");

	DEFINE_SCRIPTFUNC(SetOrigin,
		"Sets an entity's world origin.");

	DEFINE_SCRIPTFUNC(SetAngles,
		"Sets an entity's world angles.");

	DEFINE_SCRIPTFUNC(GetVelocity,
		"Returns an entity's velocity.");

	DEFINE_SCRIPTFUNC(SetVelocity,
		"Sets an entity's velocity.");

	DEFINE_SCRIPTFUNC(AddVelocity,
		"Adds velocity to an entity.");

	DEFINE_SCRIPTFUNC(GetHealth,
		"Returns an entity's health.");

	DEFINE_SCRIPTFUNC(GetMaxHealth,
		"Returns an entity's maximum health.");

	DEFINE_SCRIPTFUNC(SetHealth,
		"Sets an entity's health.");

	DEFINE_SCRIPTFUNC(SetMaxHealth,
		"Sets an entity's maximum health.");

	DEFINE_SCRIPTFUNC(IsAlive,
		"Returns whether an entity is alive.");

	DEFINE_SCRIPTFUNC(IsPlayer,
		"Returns whether an entity is a player.");

	DEFINE_SCRIPTFUNC(IsNPC,
		"Returns whether an entity is an NPC.");

	DEFINE_SCRIPTFUNC(GetClassname,
		"Returns an entity's classname.");

	DEFINE_SCRIPTFUNC(GetEntityName,
		"Returns an entity's targetname.");

	DEFINE_SCRIPTFUNC(GetEntIndex,
		"Returns an entity's index.");

	DEFINE_SCRIPTFUNC(SetName,
		"Sets an entity's targetname.");

	DEFINE_SCRIPTFUNC(SetModel,
		"Sets an entity's model.");

	DEFINE_SCRIPTFUNC(Activate,
		"Activates an entity.");

	DEFINE_SCRIPTFUNC(Teleport,
		"Teleports an entity.");


	// =========================================================
	// Effects / flags
	// =========================================================

	DEFINE_SCRIPTFUNC(AddEffects,
		"Adds effect flags to an entity.");

	DEFINE_SCRIPTFUNC(RemoveEffects,
		"Removes effect flags from an entity.");

	DEFINE_SCRIPTFUNC(HasEffects,
		"Checks effect flags on an entity.");

	DEFINE_SCRIPTFUNC(AddEFlags,
		"Adds EFlags to an entity.");

	DEFINE_SCRIPTFUNC(RemoveEFlags,
		"Removes EFlags from an entity.");

	DEFINE_SCRIPTFUNC(HasEFlags,
		"Checks EFlags on an entity.");

	DEFINE_SCRIPTFUNC(SetSolid,
		"Enables or disables entity solidity.");

	DEFINE_SCRIPTFUNC(SetMoveType,
		"Sets an entity's movement type.");

	DEFINE_SCRIPTFUNC(SetCollisionGroup,
		"Sets an entity's collision group.");

	DEFINE_SCRIPTFUNC(GetCollisionGroup,
		"Returns an entity's collision group.");


	// =========================================================
	// Parenting / ownership
	// =========================================================

	DEFINE_SCRIPTFUNC(SetParent,
		"Parents an entity.");

	DEFINE_SCRIPTFUNC(ClearParent,
		"Clears an entity's parent.");

	DEFINE_SCRIPTFUNC(GetParent,
		"Returns an entity's parent.");

	DEFINE_SCRIPTFUNC(GetOwner,
		"Returns an entity's owner.");

	DEFINE_SCRIPTFUNC(SetOwner,
		"Sets an entity's owner.");


	// =========================================================
	// Entity searching
	// =========================================================

	DEFINE_SCRIPTFUNC(FindByClass,
		"Finds an entity by classname.");

	DEFINE_SCRIPTFUNC(FindByNameAfter,
		"Finds the next entity with a targetname.");

	DEFINE_SCRIPTFUNC(FindByClassAfter,
		"Finds the next entity with a classname.");

	DEFINE_SCRIPTFUNC(FindInSphere,
		"Finds the next entity inside a sphere.");


	// =========================================================
	// Player
	// =========================================================

	DEFINE_SCRIPTFUNC(GetPlayerByIndex,
		"Returns a player by index.");

	DEFINE_SCRIPTFUNC(GetPlayerCount,
		"Returns the number of connected players.");

	DEFINE_SCRIPTFUNC(SetArmor,
		"Sets a player's armor.");

	DEFINE_SCRIPTFUNC(GetArmor,
		"Returns a player's armor.");

	DEFINE_SCRIPTFUNC(SetFOV,
		"Sets a player's FOV.");

	DEFINE_SCRIPTFUNC(GiveWeapon,
		"Gives a weapon to a player.");

	DEFINE_SCRIPTFUNC(StripWeapons,
		"Removes a player's weapons.");

	DEFINE_SCRIPTFUNC(KillPlayer,
		"Kills a player.");

	DEFINE_SCRIPTFUNC(GetEyePosition,
		"Returns a player's eye position.");

	DEFINE_SCRIPTFUNC(GetEyeAngles,
		"Returns a player's eye angles.");

	DEFINE_SCRIPTFUNC(SetPlayerVelocity,
		"Sets a player's velocity.");


	// =========================================================
	// Physics
	// =========================================================

	DEFINE_SCRIPTFUNC(HasPhysics,
		"Checks whether an entity has a physics object.");

	DEFINE_SCRIPTFUNC(WakePhysics,
		"Wakes an entity's physics object.");

	DEFINE_SCRIPTFUNC(SleepPhysics,
		"Puts an entity's physics object to sleep.");

	DEFINE_SCRIPTFUNC(GetMass,
		"Returns an entity's physics mass.");

	DEFINE_SCRIPTFUNC(GetPhysicsVelocity,
		"Returns an entity's physics velocity.");

	DEFINE_SCRIPTFUNC(SetPhysicsVelocity,
		"Sets an entity's physics velocity.");

	DEFINE_SCRIPTFUNC(ApplyTorque,
		"Applies torque to an entity.");


	// =========================================================
	// Damage / fire
	// =========================================================

	DEFINE_SCRIPTFUNC(KillEntity,
		"Kills an entity.");

	DEFINE_SCRIPTFUNC(Ignite,
		"Ignites an entity.");

	DEFINE_SCRIPTFUNC(Extinguish,
		"Extinguishes an entity.");


	// =========================================================
	// Spatial / math
	// =========================================================

	DEFINE_SCRIPTFUNC(Distance,
		"Returns the distance between two vectors.");

	DEFINE_SCRIPTFUNC(DistanceEntities,
		"Returns the distance between two entities.");

	DEFINE_SCRIPTFUNC(DirectionTo,
		"Returns the direction from one entity to another.");

	DEFINE_SCRIPTFUNC(GetCenter,
		"Returns an entity's world-space center.");


	// =========================================================
	// Sound
	// =========================================================

	DEFINE_SCRIPTFUNC(EmitSound,
		"Emits a sound from an entity.");

	DEFINE_SCRIPTFUNC(StopSound,
		"Stops a sound on an entity.");


	// =========================================================
	// Entity I/O
	// =========================================================

	DEFINE_SCRIPTFUNC(FireInput,
		"Fires an input on an entity.");

	DEFINE_SCRIPTFUNC(FireInputString,
		"Fires an input with a string parameter.");


	// =========================================================
	// Utility
	// =========================================================

	DEFINE_SCRIPTFUNC(RandomFloat,
		"Returns a random float.");

	DEFINE_SCRIPTFUNC(RandomInt,
		"Returns a random integer.");

	DEFINE_SCRIPTFUNC(GetTime,
		"Returns the current game time.");

	DEFINE_SCRIPTFUNC(GetTickCountA,
		"Returns the current server tick.");

	DEFINE_SCRIPTFUNC(GetTickInterval,
		"Returns the server tick interval.");

	// =========================================================
// Player helpers
// =========================================================

	DEFINE_SCRIPTFUNC(IsPlayerAlive,
		"Returns whether a player is alive.");

	DEFINE_SCRIPTFUNC(GetPlayerHealth,
		"Returns a player's health.");

	DEFINE_SCRIPTFUNC(SetPlayerHealth,
		"Sets a player's health.");

	DEFINE_SCRIPTFUNC(GetPlayerArmor,
		"Returns a player's armor.");

	DEFINE_SCRIPTFUNC(SetPlayerArmor,
		"Sets a player's armor.");

	DEFINE_SCRIPTFUNC(GetPlayerOrigin,
		"Returns a player's position.");

	DEFINE_SCRIPTFUNC(SetPlayerOrigin,
		"Sets a player's position.");

	DEFINE_SCRIPTFUNC(GetPlayerVelocity,
		"Returns a player's velocity.");

	DEFINE_SCRIPTFUNC(SetPlayerVelocity,
		"Sets a player's velocity.");

	DEFINE_SCRIPTFUNC(AddPlayerVelocity,
		"Adds velocity to a player.");

	DEFINE_SCRIPTFUNC(GetPlayerEyePosition,
		"Returns a player's eye position.");

	DEFINE_SCRIPTFUNC(GetPlayerEyeAngles,
		"Returns a player's eye angles.");

	DEFINE_SCRIPTFUNC(SetPlayerEyeAngles,
		"Sets a player's eye angles.");

	DEFINE_SCRIPTFUNC(GetPlayerTeam,
		"Returns a player's team.");

	DEFINE_SCRIPTFUNC(SetPlayerTeam,
		"Changes a player's team.");

	DEFINE_SCRIPTFUNC(GetPlayerFOV,
		"Returns a player's field of view.");

	DEFINE_SCRIPTFUNC(SetPlayerFOV,
		"Sets a player's field of view.");

	DEFINE_SCRIPTFUNC(IsPlayerOnGround,
		"Returns whether a player is on the ground.");

	DEFINE_SCRIPTFUNC(IsPlayerDuck,
		"Returns whether a player is ducking.");

	DEFINE_SCRIPTFUNC(EnableNoclip,
		"Enables noclip for a player.");

	DEFINE_SCRIPTFUNC(DisableNoclip,
		"Disables noclip for a player.");

	DEFINE_SCRIPTFUNC(IsNoclip,
		"Returns whether a player is in noclip mode.");

	DEFINE_SCRIPTFUNC(GiveWeapon,
		"Gives a weapon to a player.");

	DEFINE_SCRIPTFUNC(GiveAmmo,
		"Gives ammo to a player.");

	DEFINE_SCRIPTFUNC(StripWeapons,
		"Removes all weapons from a player.");

	DEFINE_SCRIPTFUNC(GetActiveWeapon,
		"Returns the player's active weapon.");

	DEFINE_SCRIPTFUNC(KillPlayer,
		"Kills a player.");

	DEFINE_SCRIPTFUNC(RespawnPlayer,
		"Respawns a player.");

	DEFINE_SCRIPTFUNC(FreezePlayer,
		"Freezes or unfreezes a player.");

	DEFINE_SCRIPTFUNC(SetPlayerGravity,
		"Sets a player's gravity multiplier.");

	DEFINE_SCRIPTFUNC(GetPlayerGravity,
		"Returns a player's gravity multiplier.");

	DEFINE_SCRIPTFUNC(HasSuit,
		"Returns whether a player has the HEV suit.");

	DEFINE_SCRIPTFUNC(GiveSuit,
		"Equips the HEV suit.");

	DEFINE_SCRIPTFUNC(RemoveSuit,
		"Removes the HEV suit.");

	DEFINE_SCRIPTFUNC(GetPlayerFlags,
		"Returns a player's flags.");

	DEFINE_SCRIPTFUNC(AddPlayerFlags,
		"Adds flags to a player.");

	DEFINE_SCRIPTFUNC(RemovePlayerFlags,
		"Removes flags from a player.");

	DEFINE_SCRIPTFUNC(IsPlayerInVehicle,
		"Returns whether a player is in a vehicle.");

	DEFINE_SCRIPTFUNC(GetPlayerVehicle,
		"Returns the vehicle occupied by a player.");

	// ============================================================
// Render / color
// ============================================================

	DEFINE_SCRIPTFUNC(SetColor,
		"Sets an entity's render color.");

	DEFINE_SCRIPTFUNC(GetColorR,
		"Returns an entity's render red channel.");

	DEFINE_SCRIPTFUNC(GetColorG,
		"Returns an entity's render green channel.");

	DEFINE_SCRIPTFUNC(GetColorB,
		"Returns an entity's render blue channel.");

	DEFINE_SCRIPTFUNC(GetColorA,
		"Returns an entity's render alpha channel.");

	DEFINE_SCRIPTFUNC(SetAlpha,
		"Sets an entity's render alpha.");

	DEFINE_SCRIPTFUNC(GetAlpha,
		"Returns an entity's render alpha.");

	DEFINE_SCRIPTFUNC(SetRenderMode,
		"Sets an entity's render mode.");

	DEFINE_SCRIPTFUNC(GetRenderMode,
		"Returns an entity's render mode.");

	DEFINE_SCRIPTFUNC(Hide,
		"Hides an entity.");

	DEFINE_SCRIPTFUNC(Show,
		"Shows an entity.");

	DEFINE_SCRIPTFUNC(IsHidden,
		"Returns whether an entity is hidden.");

	DEFINE_SCRIPTFUNC(GetModelName,
		"Returns an entity's model name.");

	DEFINE_SCRIPTFUNC(GetFlags,
		"Returns an entity's flags.");

	DEFINE_SCRIPTFUNC(AddFlags,
		"Adds flags to an entity.");

	DEFINE_SCRIPTFUNC(RemoveFlags,
		"Removes flags from an entity.");

	DEFINE_SCRIPTFUNC(GetMoveType,
		"Returns an entity's move type.");

	DEFINE_SCRIPTFUNC(DistanceToEntity,
		"Returns the distance between two entities.");

	DEFINE_SCRIPTFUNC(DirectionToEntity,
		"Returns the normalized direction from one entity to another.");

	DEFINE_SCRIPTFUNC(IsValidEntity,
		"Returns whether an entity handle is valid.");

	END_SCRIPTDESC()

	static CGabeModVScript g_GabeModVScript;

	void RegisterGabeModVScript()
	{
		if (g_pScriptVM)
			g_pScriptVM->RegisterInstance(
				&g_GabeModVScript,
				"GabeMod"
			);

		// =========================================================
		// HUD print destinations
		// =========================================================
		VSCRIPT_CONST(HUD_PRINTNOTIFY);
		VSCRIPT_CONST(HUD_PRINTCONSOLE);
		VSCRIPT_CONST(HUD_PRINTTALK);
		VSCRIPT_CONST(HUD_PRINTCENTER);

		// =========================================================
		// Teams
		// =========================================================
		VSCRIPT_CONST(TEAM_ANY);
		VSCRIPT_CONST(TEAM_INVALID);
		VSCRIPT_CONST(TEAM_UNASSIGNED);
		VSCRIPT_CONST(TEAM_SPECTATOR);
		VSCRIPT_CONST(LAST_SHARED_TEAM);
		VSCRIPT_CONST(FIRST_GAME_TEAM);

		// =========================================================
		// Skill levels
		// =========================================================
		VSCRIPT_CONST(SKILL_EASY);
		VSCRIPT_CONST(SKILL_MEDIUM);
		VSCRIPT_CONST(SKILL_HARD);

		// =========================================================
		// Weapon state
		// =========================================================
		VSCRIPT_CONST(WEAPON_NOT_CARRIED);
		VSCRIPT_CONST(WEAPON_IS_CARRIED_BY_PLAYER);
		VSCRIPT_CONST(WEAPON_IS_ACTIVE);
		VSCRIPT_CONST(WEAPON_IS_ONTARGET);
		VSCRIPT_CONST(WEAPON_NOCLIP);

		// =========================================================
		// Weapon item flags
		// =========================================================
		VSCRIPT_CONST(ITEM_FLAG_SELECTONEMPTY);
		VSCRIPT_CONST(ITEM_FLAG_NOAUTORELOAD);
		VSCRIPT_CONST(ITEM_FLAG_NOAUTOSWITCHEMPTY);
		VSCRIPT_CONST(ITEM_FLAG_LIMITINWORLD);
		VSCRIPT_CONST(ITEM_FLAG_EXHAUSTIBLE);
		VSCRIPT_CONST(ITEM_FLAG_DOHITLOCATIONDMG);
		VSCRIPT_CONST(ITEM_FLAG_NOAMMOPICKUPS);
		VSCRIPT_CONST(ITEM_FLAG_NOITEMPICKUP);

		// =========================================================
		// Hitgroups
		// =========================================================
		VSCRIPT_CONST(HITGROUP_GENERIC);
		VSCRIPT_CONST(HITGROUP_HEAD);
		VSCRIPT_CONST(HITGROUP_CHEST);
		VSCRIPT_CONST(HITGROUP_STOMACH);
		VSCRIPT_CONST(HITGROUP_LEFTARM);
		VSCRIPT_CONST(HITGROUP_RIGHTARM);
		VSCRIPT_CONST(HITGROUP_LEFTLEG);
		VSCRIPT_CONST(HITGROUP_RIGHTLEG);
		VSCRIPT_CONST(HITGROUP_GEAR);

		// =========================================================
		// Damage types
		// =========================================================
		VSCRIPT_CONST(DMG_GENERIC);
		VSCRIPT_CONST(DMG_CRUSH);
		VSCRIPT_CONST(DMG_BULLET);
		VSCRIPT_CONST(DMG_SLASH);
		VSCRIPT_CONST(DMG_BURN);
		VSCRIPT_CONST(DMG_VEHICLE);
		VSCRIPT_CONST(DMG_FALL);
		VSCRIPT_CONST(DMG_BLAST);
		VSCRIPT_CONST(DMG_CLUB);
		VSCRIPT_CONST(DMG_SHOCK);
		VSCRIPT_CONST(DMG_SONIC);
		VSCRIPT_CONST(DMG_ENERGYBEAM);

		VSCRIPT_CONST(DMG_PREVENT_PHYSICS_FORCE);
		VSCRIPT_CONST(DMG_NEVERGIB);
		VSCRIPT_CONST(DMG_ALWAYSGIB);
		VSCRIPT_CONST(DMG_DROWN);

		VSCRIPT_CONST(DMG_PARALYZE);
		VSCRIPT_CONST(DMG_NERVEGAS);
		VSCRIPT_CONST(DMG_POISON);
		VSCRIPT_CONST(DMG_RADIATION);
		VSCRIPT_CONST(DMG_DROWNRECOVER);
		VSCRIPT_CONST(DMG_ACID);
		VSCRIPT_CONST(DMG_SLOWBURN);

		VSCRIPT_CONST(DMG_REMOVENORAGDOLL);
		VSCRIPT_CONST(DMG_PHYSGUN);
		VSCRIPT_CONST(DMG_PLASMA);
		VSCRIPT_CONST(DMG_AIRBOAT);
		VSCRIPT_CONST(DMG_DISSOLVE);
		VSCRIPT_CONST(DMG_BLAST_SURFACE);
		VSCRIPT_CONST(DMG_DIRECT);
		VSCRIPT_CONST(DMG_BUCKSHOT);

		// =========================================================
		// Take-damage states
		// =========================================================
		VSCRIPT_CONST(DAMAGE_NO);
		VSCRIPT_CONST(DAMAGE_EVENTS_ONLY);
		VSCRIPT_CONST(DAMAGE_YES);
		VSCRIPT_CONST(DAMAGE_AIM);

		// =========================================================
		// HUD hiding flags
		// =========================================================
		VSCRIPT_CONST(HIDEHUD_WEAPONSELECTION);
		VSCRIPT_CONST(HIDEHUD_FLASHLIGHT);
		VSCRIPT_CONST(HIDEHUD_ALL);
		VSCRIPT_CONST(HIDEHUD_HEALTH);
		VSCRIPT_CONST(HIDEHUD_PLAYERDEAD);
		VSCRIPT_CONST(HIDEHUD_NEEDSUIT);
		VSCRIPT_CONST(HIDEHUD_MISCSTATUS);
		VSCRIPT_CONST(HIDEHUD_CHAT);
		VSCRIPT_CONST(HIDEHUD_CROSSHAIR);
		VSCRIPT_CONST(HIDEHUD_VEHICLE_CROSSHAIR);
		VSCRIPT_CONST(HIDEHUD_INVEHICLE);
		VSCRIPT_CONST(HIDEHUD_BONUS_PROGRESS);

		// =========================================================
		// Suit devices
		// =========================================================
		VSCRIPT_CONST(bits_SUIT_DEVICE_SPRINT);
		VSCRIPT_CONST(bits_SUIT_DEVICE_FLASHLIGHT);
		VSCRIPT_CONST(bits_SUIT_DEVICE_BREATHER);

		// =========================================================
		// Tracers
		// =========================================================
		VSCRIPT_CONST(TRACER_TYPE_DEFAULT);
		VSCRIPT_CONST(TRACER_TYPE_GUNSHIP);
		VSCRIPT_CONST(TRACER_TYPE_STRIDER);
		VSCRIPT_CONST(TRACER_TYPE_GAUSS);
		VSCRIPT_CONST(TRACER_TYPE_WATERBULLET);

		VSCRIPT_CONST(TRACER_FLAG_WHIZ);
		VSCRIPT_CONST(TRACER_FLAG_USEATTACHMENT);
		VSCRIPT_CONST(TRACER_DONT_USE_ATTACHMENT);

		// =========================================================
		// Muzzle flashes
		// =========================================================
		VSCRIPT_CONST(MUZZLEFLASH_TYPE_DEFAULT);
		VSCRIPT_CONST(MUZZLEFLASH_TYPE_GUNSHIP);
		VSCRIPT_CONST(MUZZLEFLASH_TYPE_STRIDER);

		VSCRIPT_CONST(MUZZLEFLASH_AR2);
		VSCRIPT_CONST(MUZZLEFLASH_SHOTGUN);
		VSCRIPT_CONST(MUZZLEFLASH_SMG1);
		VSCRIPT_CONST(MUZZLEFLASH_SMG2);
		VSCRIPT_CONST(MUZZLEFLASH_PISTOL);
		VSCRIPT_CONST(MUZZLEFLASH_COMBINE);
		VSCRIPT_CONST(MUZZLEFLASH_357);
		VSCRIPT_CONST(MUZZLEFLASH_RPG);
		VSCRIPT_CONST(MUZZLEFLASH_COMBINE_TURRET);
		VSCRIPT_CONST(MUZZLEFLASH_FIRSTPERSON);

		// =========================================================
		// Entity dissolve types
		// =========================================================
		VSCRIPT_CONST(ENTITY_DISSOLVE_NORMAL);
		VSCRIPT_CONST(ENTITY_DISSOLVE_ELECTRICAL);
		VSCRIPT_CONST(ENTITY_DISSOLVE_ELECTRICAL_LIGHT);
		VSCRIPT_CONST(ENTITY_DISSOLVE_CORE);

		// =========================================================
		// Observer modes
		// =========================================================
		VSCRIPT_CONST(OBS_MODE_NONE);
		VSCRIPT_CONST(OBS_MODE_DEATHCAM);
		VSCRIPT_CONST(OBS_MODE_FREEZECAM);
		VSCRIPT_CONST(OBS_MODE_FIXED);
		VSCRIPT_CONST(OBS_MODE_IN_EYE);
		VSCRIPT_CONST(OBS_MODE_CHASE);
		VSCRIPT_CONST(OBS_MODE_POI);
		VSCRIPT_CONST(OBS_MODE_ROAMING);

		VSCRIPT_CONST(OBS_ALLOW_ALL);
		VSCRIPT_CONST(OBS_ALLOW_TEAM);
		VSCRIPT_CONST(OBS_ALLOW_NONE);

		// =========================================================
		// USE_TYPE
		// =========================================================
		VSCRIPT_CONST(USE_OFF);
		VSCRIPT_CONST(USE_ON);
		VSCRIPT_CONST(USE_SET);
		VSCRIPT_CONST(USE_TOGGLE);

		// =========================================================
		// Vehicle roles
		// =========================================================
		VSCRIPT_CONST(VEHICLE_ROLE_NONE);
		VSCRIPT_CONST(VEHICLE_ROLE_DRIVER);

		// =========================================================
		// Blood colors
		// =========================================================
		VSCRIPT_CONST(DONT_BLEED);
		VSCRIPT_CONST(BLOOD_COLOR_RED);
		VSCRIPT_CONST(BLOOD_COLOR_YELLOW);
		VSCRIPT_CONST(BLOOD_COLOR_GREEN);
		VSCRIPT_CONST(BLOOD_COLOR_MECH);

#ifdef HL2_EPISODIC
		VSCRIPT_CONST(BLOOD_COLOR_ANTLION);
		VSCRIPT_CONST(BLOOD_COLOR_ZOMBIE);
		VSCRIPT_CONST(BLOOD_COLOR_ANTLION_WORKER);
#endif

		// =========================================================
		// Entity EFlags
		// =========================================================
		VSCRIPT_CONST(EFL_KILLME);
		VSCRIPT_CONST(EFL_DORMANT);
		VSCRIPT_CONST(EFL_NOCLIP_ACTIVE);
		VSCRIPT_CONST(EFL_KEEP_ON_RECREATE_ENTITIES);
		VSCRIPT_CONST(EFL_FORCE_CHECK_TRANSMIT);
		VSCRIPT_CONST(EFL_BOT_FROZEN);
		VSCRIPT_CONST(EFL_SERVER_ONLY);
		VSCRIPT_CONST(EFL_NO_AUTO_EDICT_ATTACH);

		VSCRIPT_CONST(EFL_DIRTY_ABSTRANSFORM);
		VSCRIPT_CONST(EFL_DIRTY_ABSVELOCITY);
		VSCRIPT_CONST(EFL_DIRTY_ABSANGVELOCITY);
		VSCRIPT_CONST(EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS);
		VSCRIPT_CONST(EFL_DIRTY_SPATIAL_PARTITION);
		VSCRIPT_CONST(EFL_FORCE_ALLOW_MOVEPARENT);

		VSCRIPT_CONST(EFL_IN_SKYBOX);
		VSCRIPT_CONST(EFL_USE_PARTITION_WHEN_NOT_SOLID);
		VSCRIPT_CONST(EFL_TOUCHING_FLUID);

		VSCRIPT_CONST(EFL_IS_BEING_LIFTED_BY_BARNACLE);
		VSCRIPT_CONST(EFL_NO_ROTORWASH_PUSH);
		VSCRIPT_CONST(EFL_NO_THINK_FUNCTION);
		VSCRIPT_CONST(EFL_NO_GAME_PHYSICS_SIMULATION);
		VSCRIPT_CONST(EFL_CHECK_UNTOUCH);
		VSCRIPT_CONST(EFL_DONTBLOCKLOS);
		VSCRIPT_CONST(EFL_DONTWALKON);
		VSCRIPT_CONST(EFL_NO_DISSOLVE);
		VSCRIPT_CONST(EFL_NO_MEGAPHYSCANNON_RAGDOLL);
		VSCRIPT_CONST(EFL_NO_WATER_VELOCITY_CHANGE);
		VSCRIPT_CONST(EFL_NO_PHYSCANNON_INTERACTION);
		VSCRIPT_CONST(EFL_NO_DAMAGE_FORCES);

		// =========================================================
		// FireBullets flags
		// =========================================================
		VSCRIPT_CONST(FIRE_BULLETS_FIRST_SHOT_ACCURATE);
		VSCRIPT_CONST(FIRE_BULLETS_DONT_HIT_UNDERWATER);
		VSCRIPT_CONST(FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS);
		VSCRIPT_CONST(FIRE_BULLETS_TEMPORARY_DANGER_SOUND);

		// =========================================================
		// Misc useful constants
		// =========================================================
		VSCRIPT_CONST(MAX_FOV);
		VSCRIPT_CONST(MAX_WEAPON_SLOTS);
		VSCRIPT_CONST(MAX_WEAPON_POSITIONS);
		VSCRIPT_CONST(MAX_WEAPONS);
		VSCRIPT_CONST(MAX_AMMO_TYPES);

		VSCRIPT_CONST(MAX_VIEWMODELS);
		VSCRIPT_CONST(MAX_BEAM_ENTS);
		VSCRIPT_CONST(MAX_SCREEN_OVERLAYS);

		VSCRIPT_CONST(NO_THINK_CONTEXT);
		VSCRIPT_CONST(TICK_NEVER_THINK);
	}

ConVar gabe_healamt("gabe_healamt", "25", FCVAR_CHEAT, "Amount of health to heal");

CON_COMMAND(gabe_heal, "Heal player with specified amount")
{
	CBasePlayer* p = GabeMod::CGabeServerFuncs::GetPlayer();
	if (!p) return;

	GabeMod::CGabeServerFuncs::Heal(p, gabe_healamt.GetInt());
}
#endif


#ifdef CLIENT_DLL

// ============================================================
// Entity-returning wrappers
// ============================================================

static C_BaseEntity* ClientEntFromHandle(int handle)
{
	if (handle <= 0)
		return NULL;

	return cl_entitylist->GetEnt(handle);
}

static C_BasePlayer* ClientPlayerFromHandle(int handle)
{
	if (handle <= 0)
		return NULL;

	return ToBasePlayer(
		cl_entitylist->GetEnt(handle)
	);
}

static int VScript_Gabe_GetPlayer()
{
	C_BasePlayer* pPlayer =
		GabeMod::CGabeClientFuncs::GetPlayer();

	if (!pPlayer)
		return -1;

	return pPlayer->entindex();
}

static int VScript_Gabe_GetObserverTarget()
{
	C_BaseEntity* pEnt =
		GabeMod::CGabeClientFuncs::GetObserverTarget();

	return pEnt ? pEnt->entindex() : -1;
}

static int VScript_Gabe_GetActiveWeapon()
{
	C_BaseCombatWeapon* pWeapon =
		GabeMod::CGabeClientFuncs::GetActiveWeapon();

	return pWeapon ? pWeapon->entindex() : -1;
}

static int VScript_Gabe_GetViewModel(int index)
{
	C_BaseViewModel* pViewModel =
		GabeMod::CGabeClientFuncs::GetViewModel(index);

	return pViewModel ? pViewModel->entindex() : -1;
}

static int VScript_Gabe_GetVehicleEntity()
{
	C_BaseEntity* pVehicle =
		GabeMod::CGabeClientFuncs::GetVehicleEntity();

	return pVehicle ? pVehicle->entindex() : -1;
}

static int VScript_Gabe_GetUseEntity()
{
	C_BaseEntity* pEnt =
		GabeMod::CGabeClientFuncs::GetUseEntity();

	return pEnt ? pEnt->entindex() : -1;
}


// ============================================================
// Registration
// ============================================================

void RegisterGabeModClientVScript()
{
	if (!g_pScriptVM)
		return;

	g_pScriptVM->Run(
		"Gabe_Client <- {};",
		true
	);

	// Player/entity handles
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		VScript_Gabe_GetPlayer,
		"Gabe_GetPlayer",
		"Returns the local player."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		VScript_Gabe_GetObserverTarget,
		"Gabe_GetObserverTarget",
		"Returns the current observer target."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		VScript_Gabe_GetActiveWeapon,
		"Gabe_GetActiveWeapon",
		"Returns the local player's active weapon."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		VScript_Gabe_GetViewModel,
		"Gabe_GetViewModel",
		"Returns one of the local player's viewmodels."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		VScript_Gabe_GetVehicleEntity,
		"Gabe_GetVehicleEntity",
		"Returns the local player's vehicle entity."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		VScript_Gabe_GetUseEntity,
		"Gabe_GetUseEntity",
		"Returns the local player's use entity."
	);


	// Player state
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::IsPlayerUnderwater,
		"Gabe_IsPlayerUnderwater",
		"Returns whether the local player is underwater."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetObserverMode,
		"Gabe_GetObserverMode",
		"Returns the current observer mode."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::IsObserver,
		"Gabe_IsObserver",
		"Returns whether the local player is an observer."
	);


	// Identity
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetPlayerName,
		"Gabe_GetPlayerName",
		"Returns the local player's name."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetUserID,
		"Gabe_GetUserID",
		"Returns the local player's user ID."
	);


	// View state
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::IsFirstPerson,
		"Gabe_IsFirstPerson",
		"Returns whether the local player is in first-person view."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::ShouldDrawPlayer,
		"Gabe_ShouldDrawPlayer",
		"Returns whether the local player model should be drawn."
	);


	// Speed
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetPlayerMaxSpeed,
		"Gabe_GetPlayerMaxSpeed",
		"Returns the player's calculated maximum speed."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetStoredMaxSpeed,
		"Gabe_GetStoredMaxSpeed",
		"Returns the player's stored maximum speed."
	);


	// FOV
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetFOV,
		"Gabe_GetFOV",
		"Returns the current field of view."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetDefaultFOV,
		"Gabe_GetDefaultFOV",
		"Returns the default field of view."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::IsZoomed,
		"Gabe_IsZoomed",
		"Returns whether the player is zoomed."
	);


	// Eye / weapon position
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetShootPosition,
		"Gabe_GetShootPosition",
		"Returns the weapon shoot position."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetEyeForward,
		"Gabe_GetEyeForward",
		"Returns the player's eye forward vector."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetEyeRight,
		"Gabe_GetEyeRight",
		"Returns the player's eye right vector."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetEyeUp,
		"Gabe_GetEyeUp",
		"Returns the player's eye up vector."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetLocalEyeAngles,
		"Gabe_GetLocalEyeAngles",
		"Returns the player's local eye angles."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetPunchAngle,
		"Gabe_GetPunchAngle",
		"Returns the player's view punch angle."
	);


	// Suit / vehicle
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::HasSuit,
		"Gabe_HasSuit",
		"Returns whether the player has the HEV suit."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::IsInVehicle,
		"Gabe_IsInVehicle",
		"Returns whether the player is in a vehicle."
	);


	// Prediction / timing
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetTimeBase,
		"Gabe_GetTimeBase",
		"Returns the player's time base."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetFinalPredictedTime,
		"Gabe_GetFinalPredictedTime",
		"Returns the final predicted time."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetCurrentCommandNumber,
		"Gabe_GetCurrentCommandNumber",
		"Returns the current predicted command number."
	);


	// Misc player state
	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::HasFiredWeapon,
		"Gabe_HasFiredWeapon",
		"Returns whether the player has fired a weapon."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetDeathTime,
		"Gabe_GetDeathTime",
		"Returns the player's death time."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetScreenWidth,
		"Gabe_GetScreenWidth",
		"Returns the screen width."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetScreenHeight,
		"Gabe_GetScreenHeight",
		"Returns the screen height."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetAspectRatio,
		"Gabe_GetAspectRatio",
		"Returns the current view aspect ratio."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::SetAspectRatio,
		"Gabe_SetAspectRatio",
		"Sets the view aspect-ratio override."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetViewFOV,
		"Gabe_GetViewFOV",
		"Returns the current view FOV."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::SetViewFOV,
		"Gabe_SetViewFOV",
		"Sets the current view FOV."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetViewAngles,
		"Gabe_GetViewAngles",
		"Returns the current view angles."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::SetViewAngles,
		"Gabe_SetViewAngles",
		"Sets the current view angles."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetScreenOverlay,
		"Gabe_GetScreenOverlay",
		"Returns the current screen overlay material."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::SetScreenOverlay,
		"Gabe_SetScreenOverlay",
		"Sets the screen overlay material."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::ClearScreenOverlay,
		"Gabe_ClearScreenOverlay",
		"Clears the screen overlay."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::HasScreenOverlay,
		"Gabe_HasScreenOverlay",
		"Returns whether a screen overlay is active."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetPlayerRenderR,
		"Gabe_GetPlayerRenderR",
		"Gets player render red."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetPlayerRenderG,
		"Gabe_GetPlayerRenderG",
		"Gets player render green."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetPlayerRenderB,
		"Gabe_GetPlayerRenderB",
		"Gets player render blue."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeMod::CGabeClientFuncs::GetPlayerRenderA,
		"Gabe_GetPlayerRenderA",
		"Gets player render alpha."
	);
	FUNC_REGISTER("Gabe_SetPlayerRenderColor", GabeMod::CGabeClientFuncs::SetPlayerRenderColor, "Set the player's color, integer r, g, b, a");

	// ============================================================
	// Gabe_Client table
	// ============================================================

// Entity/player handles
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPlayer, GetPlayer);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetObserverTarget, GetObserverTarget);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetActiveWeapon, GetActiveWeapon);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetViewModel, GetViewModel);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetVehicleEntity, GetVehicleEntity);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetUseEntity, GetUseEntity);

	// Player state
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_IsPlayerUnderwater, IsPlayerUnderwater);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetObserverMode, GetObserverMode);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_IsObserver, IsObserver);

	// Identity
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPlayerName, GetPlayerName);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetUserID, GetUserID);

	// View state
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_IsFirstPerson, IsFirstPerson);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_ShouldDrawPlayer, ShouldDrawPlayer);

	// Speed
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPlayerMaxSpeed, GetPlayerMaxSpeed);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetStoredMaxSpeed, GetStoredMaxSpeed);

	// FOV
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetFOV, GetFOV);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetDefaultFOV, GetDefaultFOV);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_IsZoomed, IsZoomed);

	// Eye / weapon position
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetShootPosition, GetShootPosition);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetEyeForward, GetEyeForward);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetEyeRight, GetEyeRight);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetEyeUp, GetEyeUp);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetLocalEyeAngles, GetLocalEyeAngles);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPunchAngle, GetPunchAngle);

	// Suit / vehicle
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_HasSuit, HasSuit);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_IsInVehicle, IsInVehicle);

	// Prediction / timing
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetTimeBase, GetTimeBase);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetFinalPredictedTime, GetFinalPredictedTime);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetCurrentCommandNumber, GetCurrentCommandNumber);

	// Misc player state
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_HasFiredWeapon, HasFiredWeapon);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetDeathTime, GetDeathTime);

	// Screen
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetScreenWidth, GetScreenWidth);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetScreenHeight, GetScreenHeight);

	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetAspectRatio, GetAspectRatio);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_SetAspectRatio, SetAspectRatio);

	// View
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetViewFOV, GetViewFOV);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_SetViewFOV, SetViewFOV);

	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetViewAngles, GetViewAngles);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_SetViewAngles, SetViewAngles);

	// Screen overlay
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetScreenOverlay, GetScreenOverlay);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_SetScreenOverlay, SetScreenOverlay);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_ClearScreenOverlay, ClearScreenOverlay);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_HasScreenOverlay, HasScreenOverlay);

	// Player rendering
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPlayerRenderR, GetPlayerRenderR);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPlayerRenderG, GetPlayerRenderG);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPlayerRenderB, GetPlayerRenderB);
	TABLE_FUNC_NAMED(Gabe_Client, Gabe_GetPlayerRenderA, GetPlayerRenderA);

	TABLE_FUNC_NAMED(
		Gabe_Client,
		Gabe_SetPlayerRenderColor,
		SetPlayerRenderColor
	);
}

#endif
