///===========================================================================
// Purpose: Gabe mod namespace
///===========================================================================

#include "cbase.h"

#include "GABE.H"

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

#endif

namespace GabeMod
{

#define GABE_PRINTSMALLMESSAGE		1
#define GABE_PRINTTOCONSOLE			2
#define GABE_PRINTTOCHAT			3
#define GABE_PRINTVISUALTEXT		4

#ifndef CLIENT_DLL
	CBasePlayer* GetPlayer()
#else
	C_BasePlayer* GetPlayer()
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

	// =========================
	// 🔹 Messaging
	// =========================
	void PrintToYou(CBasePlayer* pPlayer, int dest, const char* msg)
	{
		if (!pPlayer) return;
		ClientPrint(pPlayer, dest, msg);
	}

	void PrintToAllPlayers(int dest, const char* msg) // INVALID ON CLIENTSIDE
	{
#ifndef CLIENT_DLL
		UTIL_ClientPrintAll(dest, msg);
#endif
	}

	// =========================
	// 🔹 Debug Overlay
	// =========================
	void ScreenText(const char* text, float x, float y, float time, int r, int g, int b, int a)
	{
		if (!debugoverlay) return;
		debugoverlay->AddScreenTextOverlay(x, y, time, r, g, b, a, text);
	}

#ifndef CLIENT_DLL
	void DebugLine(const Vector& start, const Vector& end, float time)
	{
		NDebugOverlay::Line(start, end, 0, 255, 0, true, time);
	}

	void DebugBox(const Vector& pos, float size, float time)
	{
		NDebugOverlay::Box(pos,
			-Vector(size, size, size),
			Vector(size, size, size),
			255, 0, 0, 0,
			time);
	}
#endif

	// =========================
	// 🔹 Math / Player vectors
	// =========================
	Vector Forward(CBasePlayer* pPlayer)
	{
		Vector forward;
		if (!pPlayer) return vec3_origin;

		AngleVectors(pPlayer->EyeAngles(), &forward);
		return forward;
	}

	Vector ShootPos(CBasePlayer* pPlayer)
	{
		if (!pPlayer) return vec3_origin;
		return pPlayer->Weapon_ShootPosition();
	}

	// =========================
	// 🔹 Trace helpers
	// =========================
	CBaseEntity* TraceEntity(CBasePlayer* pPlayer, float dist)
	{
		if (!pPlayer) return NULL;

		trace_t tr;

		Vector start = ShootPos(pPlayer);
		Vector forward = Forward(pPlayer);
		Vector end = start + forward * dist;

		UTIL_TraceLine(start, end, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

		return tr.m_pEnt;
	}

	void Trace(CBasePlayer* pPlayer, float dist, trace_t& tr)
	{
		if (!pPlayer)
			return;

		Vector start = ShootPos(pPlayer);

		Vector forward;
		AngleVectors(pPlayer->EyeAngles(), &forward);

		Vector end = start + forward * dist;

		UTIL_TraceLine(start, end, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);
	}

#ifndef CLIENT_DLL
	// =========================
	// 🔹 Physics
	// =========================
	void ApplyForce(CBaseEntity* pEnt, const Vector& force)
	{
		if (!pEnt) return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
		if (!pPhys) return;

		pPhys->ApplyForceCenter(force);
	}

	void SetMass(CBaseEntity* pEnt, float mass)
	{
		if (!pEnt) return;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
		if (!pPhys) return;

		pPhys->SetMass(mass);
	}

	// =========================
	// 🔹 Spawning
	// =========================
	CBaseEntity* SpawnProp(const char* model, const Vector& pos, const QAngle& ang)
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

	CBaseEntity* SpawnEntity(const char* classname, const Vector& pos, const QAngle& ang)
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
	CBaseEntity* FindByName(const char* name)
	{
		CBaseEntity* pEnt = NULL;

		while ((pEnt = gEntList.FindEntityByName(pEnt, name)) != NULL)
		{
			return pEnt;
		}

		return NULL;
	}

	void Remove(CBaseEntity* pEnt)
	{
		if (!pEnt) return;
		UTIL_Remove(pEnt);
	}

	// =========================
	// 🔹 Gameplay helpers
	// =========================
	void GiveAllWeapons(CHL2MP_Player* pPlayer)
	{
		pPlayer->GiveAllItems();
	}

	void Heal(CBasePlayer* pPlayer, int amount)
	{
		if (!pPlayer) return;

		int newHealth = pPlayer->GetHealth() + amount;
		pPlayer->SetHealth(newHealth);
	}

	void Damage(CBasePlayer* pPlayer, int amount)
	{
		if (!pPlayer) return;

		CTakeDamageInfo dmg;
		dmg.SetDamage(amount);

		pPlayer->TakeDamage(dmg);
	}

	CBaseEntity* Gabe_FindByName(const char* name)
	{
		if (!name)
			return nullptr;

		return gEntList.FindEntityByName(nullptr, name);
	}

	CBaseEntity* Gabe_FindByClass(const char* classname)
	{
		if (!classname)
			return nullptr;

		return gEntList.FindEntityByClassname(nullptr, classname);
	}

	CBaseEntity* Gabe_FindClosestEntity(const Vector& origin, const char* classname, float maxDist)
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

	void Gabe_DealDamage(
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

	void Gabe_RadiusDamage(
		const Vector& origin,
		CBaseEntity* attacker,
		float damage,
		float radius,
		int damageType)
	{
		CTakeDamageInfo info(attacker, attacker, damage, damageType);
		RadiusDamage(info, origin, radius, CLASS_NONE, nullptr);
	}

	void Gabe_TraceFromPlayer(
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

	bool Gabe_HasLineOfSight(CBaseEntity* from, CBaseEntity* to)
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

	bool Gabe_IsInFOV(CBaseEntity* viewer, CBaseEntity* target, float fovDegrees)
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

	IPhysicsObject* Gabe_GetPhys(CBaseEntity* ent)
	{
		if (!ent)
			return nullptr;

		return ent->VPhysicsGetObject();
	}

	/////////////////////////////////////////////////////////////////////
	// CONSTRAINTS
	////////////////////////////////////////////////////////////////////

	IPhysicsConstraint* Gabe_CreateFixed(
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

	IPhysicsConstraint* Gabe_CreateBreakableFixed(
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

	IPhysicsConstraint* Gabe_FixedToWorld(CBaseEntity* ent)
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

	IPhysicsConstraint* Gabe_CreateBallSocket(
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

	IPhysicsConstraint* Gabe_CreateRope(
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

	IPhysicsConstraint* Gabe_CreateSlider(
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

	void Gabe_SetSliderMotor(
		IPhysicsConstraint* constraint,
		float velocity,
		float maxForce)
	{
		if (!constraint)
			return;

		constraint->SetLinearMotor(velocity, maxForce);
	}

	IPhysicsConstraint* Gabe_CreateHinge(
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

	IPhysicsConstraint* Gabe_CreateLimitedHinge(
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

	void Gabe_SetBreakable(
		constraint_breakableparams_t& breakParams,
		float forceLimit,
		float torqueLimit)
	{
		breakParams.forceLimit = forceLimit;
		breakParams.torqueLimit = torqueLimit;
	}

	void Gabe_DestroyConstraint(IPhysicsConstraint* constraint)
	{
		if (!constraint)
			return;

		physenv->DestroyConstraint(constraint);
	}

	////////////////////////
//  Display a message //
////////////////////////
	void HudText(CBasePlayer* pPlayer,
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
	void MessageBox(const char* title, const char* message)
	{
		vgui::MessageBox* pMsgBox = new vgui::MessageBox(title, message);
		pMsgBox->DoModal();
	}
#endif
}

#ifndef CLIENT_DLL

ConVar gabe_healamt("gabe_healamt", "25", FCVAR_CHEAT, "Amount of health to heal");

CON_COMMAND(gabe_heal, "Heal player with specified amount")
{
	CBasePlayer* p = GabeMod::GetPlayer();
	if (!p) return;

	GabeMod::Heal(p, gabe_healamt.GetInt());
}
#endif

