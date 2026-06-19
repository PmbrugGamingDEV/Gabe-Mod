#pragma once

///===========================================================================
// Purpose: Gabe mod namespace
///===========================================================================

#include "cbase.h"

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

	// =========================
	// 🔹 Player
	// =========================
#ifndef CLIENT_DLL
	CBasePlayer* GetPlayer();
#else
	C_BasePlayer* GetPlayer();
#endif

	// =========================
	// 🔹 Messaging
	// =========================
	void Print(CBasePlayer* pPlayer, int dest, const char* msg);
	void PrintAll(int dest, const char* msg);

	// =========================
	// 🔹 Debug
	// =========================
	void ScreenText(const char* text, float x, float y, float time, int r, int g, int b, int a);

#ifndef CLIENT_DLL
	void DebugLine(const Vector& start, const Vector& end, float time);
	void DebugBox(const Vector& pos, float size, float time);
#endif

	// =========================
	// 🔹 Math
	// =========================
	Vector Forward(CBasePlayer* pPlayer);
	Vector ShootPos(CBasePlayer* pPlayer);

	// =========================
	// 🔹 Trace
	// =========================
	CBaseEntity* TraceEntity(CBasePlayer* pPlayer, float dist);
	void Trace(CBasePlayer* pPlayer, float dist, trace_t& tr);

#ifndef CLIENT_DLL

	// =========================
	// 🔹 Physics
	// =========================
	void ApplyForce(CBaseEntity* pEnt, const Vector& force);
	void SetMass(CBaseEntity* pEnt, float mass);

	// =========================
	// 🔹 Spawning
	// =========================
	CBaseEntity* SpawnProp(const char* model, const Vector& pos, const QAngle& ang);
	CBaseEntity* SpawnEntity(const char* classname, const Vector& pos, const QAngle& ang);

	// =========================
	// 🔹 Entity helpers
	// =========================
	CBaseEntity* FindByName(const char* name);
	void Remove(CBaseEntity* pEnt);

	// =========================
	// 🔹 Gameplay
	// =========================
	void GiveAllWeapons(CHL2MP_Player* pPlayer);
	void Heal(CBasePlayer* pPlayer, int amount);
	void Damage(CBasePlayer* pPlayer, int amount);

	// =========================
	// 🔹 Advanced helpers
	// =========================
	CBaseEntity* Gabe_FindByName(const char* name);
	CBaseEntity* Gabe_FindByClass(const char* classname);
	CBaseEntity* Gabe_FindClosestEntity(const Vector& origin, const char* classname, float maxDist);

	void Gabe_DealDamage(CBaseEntity* victim, CBaseEntity* attacker, float damage, int damageType);
	void Gabe_RadiusDamage(const Vector& origin, CBaseEntity* attacker, float damage, float radius, int damageType);

	void Gabe_TraceFromPlayer(CBasePlayer* player, float distance, trace_t& tr);

	bool Gabe_HasLineOfSight(CBaseEntity* from, CBaseEntity* to);
	bool Gabe_IsInFOV(CBaseEntity* viewer, CBaseEntity* target, float fovDegrees);

	IPhysicsObject* Gabe_GetPhys(CBaseEntity* ent);

	// =========================
	// 🔹 Constraints
	// =========================
	IPhysicsConstraint* Gabe_CreateFixed(CBaseEntity* refEnt, CBaseEntity* attachedEnt);
	IPhysicsConstraint* Gabe_CreateBreakableFixed(CBaseEntity* refEnt, CBaseEntity* attachedEnt);
	IPhysicsConstraint* Gabe_FixedToWorld(CBaseEntity* ent);

	IPhysicsConstraint* Gabe_CreateBallSocket(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos);
	IPhysicsConstraint* Gabe_CreateRope(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos1, const Vector& worldPos2, bool rigid);

	IPhysicsConstraint* Gabe_CreateSlider(CBaseEntity* refEnt, CBaseEntity* attachedEnt, const Vector& slideAxisWorld, float limitMin, float limitMax);
	void Gabe_SetSliderMotor(IPhysicsConstraint* constraint, float velocity, float maxForce);

	IPhysicsConstraint* Gabe_CreateHinge(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos, const Vector& worldAxis);
	IPhysicsConstraint* Gabe_CreateLimitedHinge(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos, const Vector& worldAxis, float minAngle, float maxAngle);

	void Gabe_SetBreakable(constraint_breakableparams_t& breakParams, float forceLimit, float torqueLimit);
	void Gabe_DestroyConstraint(IPhysicsConstraint* constraint);

	// =========================
	// 🔹 HUD
	// =========================
	void HudText(CBasePlayer* pPlayer,
		const char* pszText,
		int r = 255, int g = 255, int b = 255,
		float x = -1.0f, float y = 0.25f,
		float fadeIn = 0.1f, float fadeOut = 0.5f,
		float hold = 2.0f, int channel = 1,
		int effect = 0);
#endif

#ifdef CLIENT_DLL // VGUI
	void MessageBox(const char* title, const char* message);
#endif

}