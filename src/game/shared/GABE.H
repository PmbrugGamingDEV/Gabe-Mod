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

#define BEGIN_LIST(name) const char* name[] = { // Begin a simple string list
#define LIST_ENTRY(text) #text // Add a text object to the list
#define END_LIST(name) \
}; \
const int name##_COUNT = sizeof(name) / sizeof(name[0]);

#define BEGIN_STRUCT_LIST(type, name) const type name[] = {
#define STRUCT_ENTRY(...) { __VA_ARGS__ },
#define END_STRUCT_LIST(name) \
}; \
const int name##_COUNT = ARRAYSIZE(name);

namespace GabeMod
{

	class CGabeScriptFuncs
	{
	public:
		DECLARE_CLASS_NOBASE(CGabeScriptFuncs);
#ifndef CLIENT_DLL
		DECLARE_ENT_SCRIPTDESC();
#endif

		// =========================
		// 🔹 Player
		// =========================
#ifndef CLIENT_DLL
		static CBasePlayer* GetPlayer();
#else
		static C_BasePlayer* GetPlayer();
#endif

		// =========================
		// 🔹 Messaging
		// =========================

		static void PrintToYou(CBasePlayer* pPlayer, int dest, const char* msg);
		static void PrintToAllPlayers(int dest, const char* msg);
		static void Print(CBasePlayer* pPlayer, int dest, const char* msg);
		static void PrintAll(int dest, const char* msg);

		// =========================
		// 🔹 Debug
		// =========================
		static void ScreenText(const char* text, float x, float y, float time, int r, int g, int b, int a);

#ifndef CLIENT_DLL
		static void DebugLine(const Vector& start, const Vector& end, float time);
		static void DebugBox(const Vector& pos, float size, float time);
#endif

		// =========================
		// 🔹 Math
		// =========================
		static Vector Forward(CBasePlayer* pPlayer);
		static Vector ShootPos(CBasePlayer* pPlayer);

		// =========================
		// 🔹 Trace
		// =========================
		static CBaseEntity* TraceEntity(CBasePlayer* pPlayer, float dist);
		static void Trace(CBasePlayer* pPlayer, float dist, trace_t& tr);

#ifndef CLIENT_DLL

		// =========================
		// 🔹 Physics
		// =========================
		static void ApplyForce(CBaseEntity* pEnt, const Vector& force);
		static void SetMass(CBaseEntity* pEnt, float mass);

		// =========================
		// 🔹 Spawning
		// =========================
		static CBaseEntity* SpawnProp(const char* model, const Vector& pos, const QAngle& ang);
		static CBaseEntity* SpawnEntity(const char* classname, const Vector& pos, const QAngle& ang);

		// =========================
		// 🔹 Entity helpers
		// =========================
		static CBaseEntity* FindByName(const char* name);
		static void Remove(CBaseEntity* pEnt);

		// =========================
		// 🔹 Gameplay
		// =========================
		static void GiveAllWeapons(CHL2MP_Player* pPlayer);
		static void Heal(CBasePlayer* pPlayer, int amount);
		static void Damage(CBasePlayer* pPlayer, int amount);

		// =========================
		// 🔹 Advanced helpers
		// =========================
		static CBaseEntity* Gabe_FindByName(const char* name);
		static CBaseEntity* Gabe_FindByClass(const char* classname);
		static CBaseEntity* Gabe_FindClosestEntity(const Vector& origin, const char* classname, float maxDist);

		static void Gabe_DealDamage(CBaseEntity* victim, CBaseEntity* attacker, float damage, int damageType);
		static void Gabe_RadiusDamage(const Vector& origin, CBaseEntity* attacker, float damage, float radius, int damageType);

		static void Gabe_TraceFromPlayer(CBasePlayer* player, float distance, trace_t& tr);

		static bool Gabe_HasLineOfSight(CBaseEntity* from, CBaseEntity* to);
		static bool Gabe_IsInFOV(CBaseEntity* viewer, CBaseEntity* target, float fovDegrees);

		static IPhysicsObject* Gabe_GetPhys(CBaseEntity* ent);

		// =========================
		// 🔹 Constraints
		// =========================
		static IPhysicsConstraint* Gabe_CreateFixed(CBaseEntity* refEnt, CBaseEntity* attachedEnt);
		static IPhysicsConstraint* Gabe_CreateBreakableFixed(CBaseEntity* refEnt, CBaseEntity* attachedEnt);
		static IPhysicsConstraint* Gabe_FixedToWorld(CBaseEntity* ent);

		static IPhysicsConstraint* Gabe_CreateBallSocket(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos);
		static IPhysicsConstraint* Gabe_CreateRope(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos1, const Vector& worldPos2, bool rigid);

		static IPhysicsConstraint* Gabe_CreateSlider(CBaseEntity* refEnt, CBaseEntity* attachedEnt, const Vector& slideAxisWorld, float limitMin, float limitMax);
		static void Gabe_SetSliderMotor(IPhysicsConstraint* constraint, float velocity, float maxForce);

		static IPhysicsConstraint* Gabe_CreateHinge(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos, const Vector& worldAxis);
		static IPhysicsConstraint* Gabe_CreateLimitedHinge(CBaseEntity* ent1, CBaseEntity* ent2, const Vector& worldPos, const Vector& worldAxis, float minAngle, float maxAngle);

		static void Gabe_SetBreakable(constraint_breakableparams_t& breakParams, float forceLimit, float torqueLimit);
		static void Gabe_DestroyConstraint(IPhysicsConstraint* constraint);

		// =========================
		// 🔹 HUD
		// =========================
		static void HudText(CBasePlayer* pPlayer,
			const char* pszText,
			int r = 255, int g = 255, int b = 255,
			float x = -1.0f, float y = 0.25f,
			float fadeIn = 0.1f, float fadeOut = 0.5f,
			float hold = 2.0f, int channel = 1,
			int effect = 0);
#endif

#ifdef CLIENT_DLL // VGUI
		static void MessageBox(const char* title, const char* message);
#endif
	};

	void RegisterVScriptFunctions();
}