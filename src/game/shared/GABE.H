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
#include "beam_shared.h"
#include "beam_flags.h"

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
#ifdef CLIENT_DLL
	class CGabeClientFuncs
	{
	public:
		DECLARE_CLASS_NOBASE(CGabeClientFuncs);

		static C_BasePlayer* GetPlayer();

		// ============================================================
		// Client player
		// ============================================================

		static bool IsPlayerUnderwater();

		static int GetObserverMode();
		static bool IsObserver();

		static C_BaseEntity* GetObserverTarget();

		static C_BaseCombatWeapon* GetActiveWeapon();
		static C_BaseViewModel* GetViewModel(int index = 0);

		static const char* GetPlayerName();
		static int GetUserID();

		static bool IsFirstPerson();
		static bool ShouldDrawPlayer();

		static float GetPlayerMaxSpeed();
		static float GetStoredMaxSpeed();

		static int GetFOV();
		static int GetDefaultFOV();
		static bool IsZoomed();

		static Vector GetShootPosition();

		static Vector GetEyeForward();
		static Vector GetEyeRight();
		static Vector GetEyeUp();

		static QAngle GetLocalEyeAngles();

		static QAngle GetPunchAngle();

		static bool HasSuit();

		static bool IsInVehicle();
		static C_BaseEntity* GetVehicleEntity();

		static C_BaseEntity* GetUseEntity();

		static float GetTimeBase();
		static float GetFinalPredictedTime();

		static int GetCurrentCommandNumber();

		static bool HasFiredWeapon();

		static float GetDeathTime();

		static void SetPlayerRenderColor(int r, int g, int b, int a);

		inline static color32 GetPlayerRenderColor()
		{
			C_BasePlayer* pPlayer = GetPlayer();

			color32 clr = { 255, 255, 255, 255 };

			if (!pPlayer)
				return clr;

			clr.r = pPlayer->GetRenderColor().r;
			clr.g = pPlayer->GetRenderColor().g;
			clr.b = pPlayer->GetRenderColor().b;
			clr.a = pPlayer->GetRenderColor().a;

			return clr;
		}

		inline static int GetPlayerRenderR()
		{
			return GetPlayerRenderColor().r;
		}

		inline static int GetPlayerRenderG()
		{
			return GetPlayerRenderColor().g;
		}

		inline static int GetPlayerRenderB()
		{
			return GetPlayerRenderColor().b;
		}

		inline static int GetPlayerRenderA()
		{
			return GetPlayerRenderColor().a;
		}

		// ============================================================
		// View / screen
		// ============================================================

		static int GetScreenWidth();
		static int GetScreenHeight();

		static float GetAspectRatio();
		static void SetAspectRatio(float ratio);

		static float GetViewFOV();
		static void SetViewFOV(float fov);

		static QAngle GetViewAngles();
		static void SetViewAngles(const QAngle& angles);

		static const char* GetScreenOverlay();
		static void SetScreenOverlay(const char* material);
		static void ClearScreenOverlay();

		static bool HasScreenOverlay();
	};
#endif

#ifndef CLIENT_DLL

	class CGabeServerFuncs
	{
	public:
		DECLARE_CLASS_NOBASE(CGabeServerFuncs);

		// =========================
		// 🔹 Player
		// =========================

		static CBasePlayer* GetPlayer();

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

		static void DebugLine(const Vector& start, const Vector& end, float time);
		static void DebugBox(const Vector& pos, float size, float time);

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

		// ============================================================
		// Entity state
		// ============================================================

		static Vector GetOrigin(CBaseEntity* pEnt);
		static QAngle GetAngles(CBaseEntity* pEnt);

		static void SetOrigin(CBaseEntity* pEnt, const Vector& pos);
		static void SetAngles(CBaseEntity* pEnt, const QAngle& ang);

		static Vector GetVelocity(CBaseEntity* pEnt);
		static void SetVelocity(CBaseEntity* pEnt, const Vector& velocity);
		static void AddVelocity(CBaseEntity* pEnt, const Vector& velocity);

		static int GetHealth(CBaseEntity* pEnt);
		static int GetMaxHealth(CBaseEntity* pEnt);

		static void SetHealth(CBaseEntity* pEnt, int health);
		static void SetMaxHealth(CBaseEntity* pEnt, int health);

		static bool IsAlive(CBaseEntity* pEnt);
		static bool IsPlayer(CBaseEntity* pEnt);
		static bool IsNPC(CBaseEntity* pEnt);

		static const char* GetClassname(CBaseEntity* pEnt);
		static const char* GetEntityName(CBaseEntity* pEnt);

		static int GetEntIndex(CBaseEntity* pEnt);

		static void SetName(CBaseEntity* pEnt, const char* name);

		static void SetModel(CBaseEntity* pEnt, const char* model);

		static void Activate(CBaseEntity* pEnt);

		static void Teleport(
			CBaseEntity* pEnt,
			const Vector& pos,
			const QAngle& ang,
			const Vector& velocity
		);

		// ============================================================
		// Entity flags / effects
		// ============================================================

		static void AddEffects(CBaseEntity* pEnt, int effects);
		static void RemoveEffects(CBaseEntity* pEnt, int effects);
		static bool HasEffects(CBaseEntity* pEnt, int effects);

		static void AddEFlags(CBaseEntity* pEnt, int flags);
		static void RemoveEFlags(CBaseEntity* pEnt, int flags);
		static bool HasEFlags(CBaseEntity* pEnt, int flags);

		static void SetSolid(CBaseEntity* pEnt, bool solid);

		static void SetMoveType(CBaseEntity* pEnt, int moveType);

		static void SetCollisionGroup(CBaseEntity* pEnt, int group);
		static int GetCollisionGroup(CBaseEntity* pEnt);

		// ============================================================
		// Entity relationship / hierarchy
		// ============================================================

		static void SetParent(CBaseEntity* pEnt, CBaseEntity* pParent);
		static void ClearParent(CBaseEntity* pEnt);

		static CBaseEntity* GetParent(CBaseEntity* pEnt);
		static CBaseEntity* GetOwner(CBaseEntity* pEnt);

		static void SetOwner(CBaseEntity* pEnt, CBaseEntity* pOwner);

		// ============================================================
		// Entity searching
		// ============================================================

		static CBaseEntity* FindByClass(const char* classname);

		static CBaseEntity* FindByNameAfter(
			CBaseEntity* pStart,
			const char* name
		);

		static CBaseEntity* FindByClassAfter(
			CBaseEntity* pStart,
			const char* classname
		);

		static CBaseEntity* FindInSphere(
			CBaseEntity* pStart,
			const Vector& origin,
			float radius
		);

		// ============================================================
		// Player
		// ============================================================

		static CBasePlayer* GetPlayerByIndex(int index);

		static int GetPlayerCount();

		static void SetArmor(CBasePlayer* pPlayer, int armor);
		static int GetArmor(CBasePlayer* pPlayer);

		static void SetFOV(CBasePlayer* pPlayer, int fov);

		static Vector GetEyePosition(CBasePlayer* pPlayer);
		static QAngle GetEyeAngles(CBasePlayer* pPlayer);

		// ============================================================
		// Physics
		// ============================================================

		static bool HasPhysics(CBaseEntity* pEnt);

		static void WakePhysics(CBaseEntity* pEnt);
		static void SleepPhysics(CBaseEntity* pEnt);

		static float GetMass(CBaseEntity* pEnt);

		static Vector GetPhysicsVelocity(CBaseEntity* pEnt);

		static void SetPhysicsVelocity(
			CBaseEntity* pEnt,
			const Vector& velocity
		);

		static void ApplyTorque(
			CBaseEntity* pEnt,
			const Vector& torque
		);

		// ============================================================
		// Damage / fire
		// ============================================================

		static void KillEntity(
			CBaseEntity* victim,
			CBaseEntity* attacker
		);

		static void Ignite(
			CBaseEntity* pEnt,
			float time
		);

		static void Extinguish(CBaseEntity* pEnt);

		// ============================================================
		// Distances / directions
		// ============================================================

		static float Distance(
			const Vector& a,
			const Vector& b
		);

		static float DistanceEntities(
			CBaseEntity* a,
			CBaseEntity* b
		);

		static Vector DirectionTo(
			CBaseEntity* from,
			CBaseEntity* to
		);

		static Vector GetCenter(CBaseEntity* pEnt);

		// ============================================================
		// Sounds
		// ============================================================

		static void EmitSound(
			CBaseEntity* pEnt,
			const char* soundName
		);

		static void StopSound(
			CBaseEntity* pEnt,
			const char* soundName
		);

		// ============================================================
		// Misc
		// ============================================================

		static float RandomFloat(float min, float max);
		static int RandomInt(int min, int max);

		static float GetTime();
		static int GetTickCountA();
		static float GetTickInterval();

		static bool FireInput(
			CBaseEntity* pEnt,
			const char* input
		);

		static bool FireInputString(
			CBaseEntity* pEnt,
			const char* input,
			const char* value
		);

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

		// ============================================================
		// Player helpers
		// ============================================================

		static bool IsPlayerAlive(CBasePlayer* pPlayer);

		static int GetPlayerHealth(CBasePlayer* pPlayer);
		static void SetPlayerHealth(CBasePlayer* pPlayer, int health);

		static int GetPlayerArmor(CBasePlayer* pPlayer);
		static void SetPlayerArmor(CBasePlayer* pPlayer, int armor);

		static Vector GetPlayerOrigin(CBasePlayer* pPlayer);
		static void SetPlayerOrigin(CBasePlayer* pPlayer, const Vector& origin);

		static Vector GetPlayerVelocity(CBasePlayer* pPlayer);
		static void SetPlayerVelocity(CBasePlayer* pPlayer, const Vector& velocity);
		static void AddPlayerVelocity(CBasePlayer* pPlayer, const Vector& velocity);

		static Vector GetPlayerEyePosition(CBasePlayer* pPlayer);
		static QAngle GetPlayerEyeAngles(CBasePlayer* pPlayer);
		static void SetPlayerEyeAngles(CBasePlayer* pPlayer, const QAngle& angles);

		static int GetPlayerTeam(CBasePlayer* pPlayer);
		static void SetPlayerTeam(CBasePlayer* pPlayer, int team);

		static int GetPlayerFOV(CBasePlayer* pPlayer);
		static void SetPlayerFOV(CBasePlayer* pPlayer, int fov);

		static bool IsPlayerOnGround(CBasePlayer* pPlayer);

		static bool IsPlayerDuck(CBasePlayer* pPlayer);

		static void EnableNoclip(CBasePlayer* pPlayer);
		static void DisableNoclip(CBasePlayer* pPlayer);
		static bool IsNoclip(CBasePlayer* pPlayer);

		static void GiveWeapon(CBasePlayer* pPlayer, const char* weapon);
		static void GiveAmmo(CBasePlayer* pPlayer, int amount, const char* ammoName);
		static void StripWeapons(CBasePlayer* pPlayer);

		static CBaseCombatWeapon* GetActiveWeapon(CBasePlayer* pPlayer);

		static void KillPlayer(CBasePlayer* pPlayer);

		static void RespawnPlayer(CBasePlayer* pPlayer);


		static void FreezePlayer(CBasePlayer* pPlayer, bool freeze);

		static void SetPlayerGravity(CBasePlayer* pPlayer, float gravity);
		static float GetPlayerGravity(CBasePlayer* pPlayer);

		static bool HasSuit(CBasePlayer* pPlayer);
		static void GiveSuit(CBasePlayer* pPlayer);
		static void RemoveSuit(CBasePlayer* pPlayer);

		static int GetPlayerFlags(CBasePlayer* pPlayer);
		static void AddPlayerFlags(CBasePlayer* pPlayer, int flags);
		static void RemovePlayerFlags(CBasePlayer* pPlayer, int flags);

		static bool IsPlayerInVehicle(CBasePlayer* pPlayer);
		static CBaseEntity* GetPlayerVehicle(CBasePlayer* pPlayer);

		// Rendering
		static void SetColor(CBaseEntity* pEnt, int r, int g, int b, int a);

		static int GetColorR(CBaseEntity* pEnt);
		static int GetColorG(CBaseEntity* pEnt);
		static int GetColorB(CBaseEntity* pEnt);
		static int GetColorA(CBaseEntity* pEnt);

		static void SetAlpha(CBaseEntity* pEnt, int alpha);
		static int GetAlpha(CBaseEntity* pEnt);

		static void SetRenderMode(CBaseEntity* pEnt, int mode);
		static int GetRenderMode(CBaseEntity* pEnt);

		static void Hide(CBaseEntity* pEnt);
		static void Show(CBaseEntity* pEnt);
		static bool IsHidden(CBaseEntity* pEnt);

		// Model
		static const char* GetModelName(CBaseEntity* pEnt);

		// Generic entity flags
		static int GetFlags(CBaseEntity* pEnt);
		static void AddFlags(CBaseEntity* pEnt, int flags);
		static void RemoveFlags(CBaseEntity* pEnt, int flags);

		// Movement
		static int GetMoveType(CBaseEntity* pEnt);

		// Utility
		static float DistanceToEntity(CBaseEntity* a, CBaseEntity* b);
		static Vector DirectionToEntity(CBaseEntity* from, CBaseEntity* to);
		static bool IsValidEntity(CBaseEntity* pEnt);
	};
#endif
}

#ifndef CLIENT_DLL
void RegisterGabeModVScript();
#endif

#ifdef CLIENT_DLL
void RegisterGabeModClientVScript();
#endif


#ifdef CLIENT_DLL
using GabePlayer_t = C_BasePlayer;
using GabeFuncs_t = GabeMod::CGabeClientFuncs;
#else
using GabePlayer_t = CBasePlayer;
using GabeFuncs_t = GabeMod::CGabeServerFuncs;
#endif