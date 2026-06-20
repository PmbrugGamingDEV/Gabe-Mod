#include "cbase.h"
#include "player.h"
#include "util.h"
#include "util_shared.h"

#ifndef CLIENT_DLL
#include "spark.h"
#include "ieffects.h"
#endif

#include "utlvector.h"

//-----------------------------------------------------------------------------
// Fade-in system
//-----------------------------------------------------------------------------
class CFadeInEntity : public CBaseEntity
{
public:
	DECLARE_CLASS(CFadeInEntity, CBaseEntity);

	EHANDLE m_hTarget; // Target of the fadein

	void Spawn()
	{
		SetThink(&CFadeInEntity::FadeThink);
		SetNextThink(gpGlobals->curtime + 0.02f);
	}

	void FadeThink()
	{
		CBaseEntity* pEnt = m_hTarget.Get();
		if (!pEnt)
		{
			UTIL_Remove(this);
			return;
		}

		int alpha = pEnt->GetRenderColor().a;
		alpha += 25;

		if (alpha >= 255)
		{
			pEnt->SetRenderColorA(255);
			UTIL_Remove(this);
			return;
		}

		pEnt->SetRenderColorA(alpha);
		SetNextThink(gpGlobals->curtime + 0.02f);
	}
};

LINK_ENTITY_TO_CLASS(fadein_helper, CFadeInEntity);

static void StartFadeIn(CBaseEntity* pEnt)
{
	if (!pEnt)
		return;

	// start invisible
	pEnt->SetRenderMode(kRenderTransTexture);
	pEnt->SetRenderColorA(0);

	CFadeInEntity* pFade = (CFadeInEntity*)CreateEntityByName("fadein_helper");
	if (!pFade)
		return;

	pFade->m_hTarget = pEnt;

	DispatchSpawn(pFade);
	pFade->Activate();
}

static CUtlVector<EHANDLE> g_SpawnStack; // for undo

static void RegisterSpawned(CBaseEntity* pEnt)
{
	if (!pEnt)
		return;

	g_SpawnStack.AddToTail(pEnt);
}

static void FixSpawnPosition(CBaseEntity* pEnt, const Vector& hitPos)
{
	if (!pEnt)
		return;

	// Make sure collision is initialized
	pEnt->SetAbsOrigin(hitPos);
	pEnt->CollisionProp()->UpdatePartition();

	Vector mins, maxs;
	pEnt->CollisionProp()->WorldSpaceAABB(&mins, &maxs);

	Vector pos = hitPos;

	// Move up so bottom sits on hitPos
	float height = maxs.z - mins.z;
	pos.z += (height * 0.5f);

	pEnt->SetAbsOrigin(pos);
}

static void SpawnFromCommand(
	CBasePlayer* pPlayer,
	const char* pszSpawnType,
	const char* pszValue)
{
	if (!pPlayer)
		return;

	trace_t tr;
	Vector start = pPlayer->EyePosition();
	Vector end = start + pPlayer->EyeDirection3D() * 1024.0f;

	UTIL_TraceLine(start, end, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
	if (!tr.DidHit())
		return;

	Vector hitPos = tr.endpos;

	// --------------------------------------------------
	// PROPS
	// --------------------------------------------------
	if (!Q_stricmp(pszSpawnType, "prop"))
	{
		CBaseEntity* pEnt = CreateEntityByName("prop_physics");

		if (pEnt)
		{
			pEnt->PrecacheModel(pszValue);
			pEnt->SetModel(pszValue);

			DispatchSpawn(pEnt);
		}

		if (!pEnt)
			return;

		// Fix position AFTER model is set
		FixSpawnPosition(pEnt, hitPos);

		DispatchSpawn(pEnt);
		pEnt->Activate();
		StartFadeIn(pEnt);
		RegisterSpawned(pEnt);
		return;
	}

	// --------------------------------------------------
	// RAGDOLLS
	// --------------------------------------------------
	if (!Q_stricmp(pszSpawnType, "ragdoll"))
	{
		CBaseEntity* pEnt = CreateEntityByName("prop_ragdoll");
		if (!pEnt)
			return;

		pEnt->PrecacheModel(pszValue);
		pEnt->SetModel(pszValue);

		FixSpawnPosition(pEnt, hitPos);

		DispatchSpawn(pEnt);
		pEnt->Activate();
		StartFadeIn(pEnt);
		RegisterSpawned(pEnt);
		return;
	}

	// --------------------------------------------------
	// NPCs
	// --------------------------------------------------
	if (!Q_stricmp(pszSpawnType, "npc"))
	{
		CBaseEntity* pEnt = CreateEntityByName(pszValue);
		if (!pEnt)
		{
			Warning("Unknown NPC classname: %s\n", pszValue);
			return;
		}

		DispatchSpawn(pEnt);

		FixSpawnPosition(pEnt, hitPos);

		pEnt->Activate();
		StartFadeIn(pEnt);
		RegisterSpawned(pEnt);
		return;
	}

	// --------------------------------------------------
	// EFFECTS
	// --------------------------------------------------
	if (!Q_stricmp(pszSpawnType, "effect"))
	{
		CBaseAnimating* pCarrier =
			static_cast<CBaseAnimating*>(CreateEntityByName("prop_physics"));

		if (!pCarrier)
			return;

		pCarrier->PrecacheModel("models/props_junk/cinderblock01a.mdl");
		pCarrier->SetModel("models/props_junk/cinderblock01a.mdl");
		pCarrier->SetMoveType(MOVETYPE_VPHYSICS);
		pCarrier->SetSolid(SOLID_VPHYSICS);
		pCarrier->VPhysicsInitNormal(SOLID_VPHYSICS, NULL, false);

		FixSpawnPosition(pCarrier, hitPos);

		DispatchSpawn(pCarrier);
		pCarrier->Activate();

		CBaseAnimating* pEffect =
			static_cast<CBaseAnimating*>(CreateEntityByName("prop_dynamic"));

		if (!pEffect)
		{
			pCarrier->SUB_Remove();
			return;
		}

		pEffect->PrecacheModel(pszValue);
		pEffect->SetModel(pszValue);
		pEffect->SetParent(pCarrier);
		pEffect->SetSolid(SOLID_NONE);
		pEffect->SetLocalOrigin(vec3_origin);

		DispatchSpawn(pEffect);
		pEffect->Activate();
		StartFadeIn(pCarrier);
		StartFadeIn(pEffect);
		RegisterSpawned(pCarrier);
		RegisterSpawned(pEffect);
		return;
	}

	// --------------------------------------------------
	// DEBUG ENTITIES
	// --------------------------------------------------
	if (!Q_stricmp(pszSpawnType, "debug"))
	{
		CBaseEntity* pEnt = CreateEntityByName(pszValue);
		if (!pEnt)
		{
			Warning("Unknown debug entity: %s\n", pszValue);
			return;
		}

		DispatchSpawn(pEnt);

		FixSpawnPosition(pEnt, hitPos);

		pEnt->Activate();
		StartFadeIn(pEnt);
		RegisterSpawned(pEnt);
		return;
	}

	// --------------------------------------------------
	// WEAPONS
	// --------------------------------------------------
	if (!Q_stricmp(pszSpawnType, "weapon"))
	{
		CBaseEntity* pEnt = CreateEntityByName(pszValue);
		if (!pEnt)
		{
			Warning("[Spawnmenu] Unknown weapon: %s\n", pszValue);
			return;
		}

		DispatchSpawn(pEnt);

		FixSpawnPosition(pEnt, hitPos);

		pEnt->Activate();
		StartFadeIn(pEnt);
		RegisterSpawned(pEnt);
		return;
	}

	// --------------------------------------------------
	// COMMANDS
	// --------------------------------------------------
	if (!Q_stricmp(pszSpawnType, "command"))
	{
		if (!pszValue || !pszValue[0])
			return;

		Msg("[SpawnMenu] Executing command: %s\n", pszValue);

		char cmd[512];
		Q_snprintf(cmd, sizeof(cmd), "%s\n", pszValue);

		if (pszValue && Q_stristr(pszValue, "mtool_mode"))
		{
			if (!pPlayer)
				return;

			CBaseCombatWeapon* pWep = pPlayer->Weapon_OwnsThisType("weapon_multitool");

			if (pWep)
			{
				pPlayer->Weapon_Switch(pWep);
			}
		}

		engine->ClientCommand(pPlayer->edict(), cmd);
		return;
	}

	// fallback effect
	g_pEffects->Sparks(hitPos, 1, 1, 0);
}

CON_COMMAND(gabe_spawn, "gabe_spawn <prop|ragdoll|npc|effect|debug|weapon|command> <value>")
{
	CBasePlayer* pPlayer = UTIL_GetCommandClient();
	if (!pPlayer || args.ArgC() < 3)
	{
		Msg("Usage: gabe_spawn <type> <model|classname>\n");
		return;
	}

	SpawnFromCommand(pPlayer, args[1], args[2]);
}

CON_COMMAND(gabe_undo, "Undo last spawned entity")
{
	if (g_SpawnStack.Count() == 0)
	{
		Msg("[Undo] Nothing to undo.\n");
		return;
	}

	CBasePlayer* pPlayer = UTIL_GetCommandClient();

	int last = g_SpawnStack.Count() - 1;

	EHANDLE hEnt = g_SpawnStack[last];
	g_SpawnStack.Remove(last);

	CBaseEntity* pEnt = hEnt.Get();

	// 🔥 CHECK FIRST
	if (!pEnt)
	{
		Msg("[Undo] Entity already gone.\n");
		return;
	}

	// safe now
	const char* classname = pEnt->GetClassname();

	char cmd[256];
	Q_snprintf(cmd, sizeof(cmd), "gabe_undoname \"%s\"; show_undo\n", classname);

	engine->ClientCommand(pPlayer->edict(), cmd);

	// dissolve if possible
	CBaseAnimating* pAnim = pEnt->GetBaseAnimating();
	if (pAnim)
	{
		pAnim->Dissolve(NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_ELECTRICAL, pEnt->GetAbsOrigin(), 500);
	}

	int rnd = RandomInt(2, 3);

	char sound[64];
	Q_snprintf(sound, sizeof(sound), "physics/body/body_medium_break%d.wav", rnd);

	pEnt->PrecacheSound(sound);
	pEnt->EmitSound(sound);
}