//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "gabe_gamerules.h"
#include "hl2mp_gamerules.h"
#include "ammodef.h"

#ifdef CLIENT_DLL
#include "../client/hl2mp/c_hl2mp_player.h"
#else
#include "../server/hl2mp/hl2mp_player.h"
#endif

REGISTER_GAMERULES_CLASS(CGabeGameRules);

BEGIN_NETWORK_TABLE_NOBASE(CGabeGameRules, DT_GabeGameRules)
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(gabe_gamerules, CGabeGameRulesProxy);
IMPLEMENT_NETWORKCLASS_ALIASED(GabeGameRulesProxy, DT_GabeGameRulesProxy)

#ifdef CLIENT_DLL
void RecvProxy_GabeGameRules(const RecvProp* pProp, void** pOut, void* pData, int objectID)
{
	CGabeGameRules* pRules = GabeGameRules();
	Assert(pRules);
	*pOut = pRules;
}

BEGIN_RECV_TABLE(CGabeGameRulesProxy, DT_GabeGameRulesProxy)
RecvPropDataTable("gabe_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE(DT_GabeGameRules), RecvProxy_GabeGameRules)
END_RECV_TABLE()
#else
void* SendProxy_GabeGameRules(const SendProp* pProp, const void* pStructBase, const void* pData, CSendProxyRecipients* pRecipients, int objectID)
{
	CGabeGameRules* pRules = GabeGameRules();
	Assert(pRules);
	return pRules;
}

BEGIN_SEND_TABLE(CGabeGameRulesProxy, DT_GabeGameRulesProxy)
SendPropDataTable("gabe_gamerules_data", 0, &REFERENCE_SEND_TABLE(DT_GabeGameRules), SendProxy_GabeGameRules)
END_SEND_TABLE()
#endif

CGabeGameRules::CGabeGameRules()
{}

CGabeGameRules::~CGabeGameRules()
{}

bool CGabeGameRules::ClientCommand(CBaseEntity* pEdict, const CCommand& args)
{
#ifndef CLIENT_DLL
	if (BaseClass::ClientCommand(pEdict, args))
		return true;


	CHL2MP_Player* pPlayer = (CHL2MP_Player*)pEdict;

	if (pPlayer->ClientCommand(args))
		return true;
#endif

	return false;
}

void CGabeGameRules::CreateStandardEntities(void)
{

#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.
	BaseClass::CreateStandardEntities();

#ifdef DBGFLAG_ASSERT
	CBaseEntity* pEnt =
#endif

		CBaseEntity::Create("gabe_gamerules", vec3_origin, vec3_angle);
	Assert(pEnt);
#endif

}

void CGabeGameRules::Think(void)
{

#ifndef CLIENT_DLL
	BaseClass::Think();
#endif

}

#ifndef CLIENT_DLL

//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CGabeGameRules::CanHavePlayerItem(CBasePlayer* pPlayer, CBaseCombatWeapon* pItem)
{
	return true;
}

#endif

void CGabeGameRules::ClientDisconnected(edict_t* pClient)
{}

const char* CGabeGameRules::GetGameDescription(void)
{
	return "Gabe Mod";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGabeGameRules::Precache(void)
{}

bool CGabeGameRules::ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	if (collisionGroup0 > collisionGroup1)
	{
		// swap so that lowest is always first
		V_swap(collisionGroup0, collisionGroup1);
	}

	if ((collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_WEAPON)
	{
		return false;
	}

	return BaseClass::ShouldCollide(collisionGroup0, collisionGroup1);

}


#ifdef CLIENT_DLL

#else

bool CGabeGameRules::FShouldSwitchWeapon(CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon)
{
	if (pPlayer->GetActiveWeapon() && pPlayer->IsNetClient())
	{
		// Player has an active item, so let's check cl_autowepswitch.
		const char* cl_autowepswitch = engine->GetClientConVarValue(engine->IndexOfEdict(pPlayer->edict()), "cl_autowepswitch");
		if (cl_autowepswitch && atoi(cl_autowepswitch) <= 0)
		{
			return false;
		}
	}

	return BaseClass::FShouldSwitchWeapon(pPlayer, pWeapon);
}

#endif

#ifndef CLIENT_DLL
const char* CGabeGameRules::GetChatFormat(bool bTeamOnly, CBasePlayer* pPlayer)
{
	if (!pPlayer)  // dedicated server output
	{
		return NULL;
	}

	const char* pszFormat = NULL;

	// team only
	if (bTeamOnly == TRUE)
	{
		if (pPlayer->GetTeamNumber() == TEAM_SPECTATOR)
		{
			pszFormat = "HL2MP_Chat_Spec";
		}
		else
		{
			const char* chatLocation = GetChatLocation(bTeamOnly, pPlayer);
			if (chatLocation && *chatLocation)
			{
				pszFormat = "HL2MP_Chat_Team_Loc";
			}
			else
			{
				pszFormat = "HL2MP_Chat_Team";
			}
		}
	}
	// everyone
	else
	{
		if (pPlayer->GetTeamNumber() != TEAM_SPECTATOR)
		{
			pszFormat = "HL2MP_Chat_All";
		}
		else
		{
			pszFormat = "HL2MP_Chat_AllSpec";
		}
	}

	return pszFormat;
}

#endif

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef* GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;

	if (!bInitted)
	{
		bInitted = true;

		def.AddAmmoType("9mmRound", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE, "sk_plr_dmg_9mm_bullet", "sk_npc_dmg_9mm_bullet", "sk_max_9mm_bullet", BULLET_IMPULSE(500, 1325), 0);
		def.AddAmmoType("357Round", DMG_BULLET | DMG_NEVERGIB, TRACER_NONE, "sk_plr_dmg_357_bullet", NULL, "sk_max_357_bullet", BULLET_IMPULSE(650, 6000), 0);
		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, "sk_plr_dmg_buckshot", NULL, "sk_max_buckshot", BULLET_IMPULSE(200, 1200), 0);
		def.AddAmmoType("XBowBolt", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE, "sk_plr_dmg_xbow_bolt_plr", NULL, "sk_max_xbow_bolt", BULLET_IMPULSE(200, 1200), 0);
		def.AddAmmoType("MP5_Grenade", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_mp5_grenade", NULL, "sk_max_mp5_grenade", 0, 0);
		def.AddAmmoType("RPG_Rocket", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_rpg", NULL, "sk_max_rpg_rocket", 0, 0);
		def.AddAmmoType("Uranium", DMG_ENERGYBEAM, TRACER_NONE, NULL, NULL, "sk_max_uranium", 0, 0);
		def.AddAmmoType("Grenade", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_grenade", NULL, "sk_max_grenade", 0, 0);
		def.AddAmmoType("Hornet", DMG_BULLET, TRACER_NONE, "sk_plr_dmg_hornet", "sk_npc_dmg_hornet", "sk_max_hornet", BULLET_IMPULSE(100, 1200), 0);
		def.AddAmmoType("Snark", DMG_SLASH, TRACER_NONE, "sk_snark_dmg_bite", NULL, "sk_max_snark", 0, 0);
		def.AddAmmoType("TripMine", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_tripmine", NULL, "sk_max_tripmine", 0, 0);
		def.AddAmmoType("Satchel", DMG_BURN | DMG_BLAST, TRACER_NONE, "sk_plr_dmg_satchel", NULL, "sk_max_satchel", 0, 0);
		def.AddAmmoType("12mmRound", DMG_BULLET | DMG_NEVERGIB, TRACER_LINE, NULL, "sk_npc_dmg_12mm_bullet", NULL, BULLET_IMPULSE(300, 1200), 0);
		def.AddAmmoType("Gravity", DMG_CLUB, TRACER_NONE, 0, 0, 8, 0, 0);
		def.AddAmmoType("AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 60, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("AR2AltFire", DMG_DISSOLVE, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 150, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 225, BULLET_IMPULSE(200, 1225), 0);
		def.AddAmmoType("357", DMG_BULLET, TRACER_LINE_AND_WHIZ, 0, 0, 12, BULLET_IMPULSE(800, 5000), 0);
		def.AddAmmoType("XBowBolt", DMG_BULLET, TRACER_LINE, 0, 0, 10, BULLET_IMPULSE(800, 8000), 0);
		def.AddAmmoType("Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE, 0, 0, 30, BULLET_IMPULSE(400, 1200), 0);
		def.AddAmmoType("RPG_Round", DMG_BURN, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("SMG1_Grenade", DMG_BURN, TRACER_NONE, 0, 0, 3, 0, 0);
		def.AddAmmoType("Grenade", DMG_BURN, TRACER_NONE, 0, 0, 5, 0, 0);
		def.AddAmmoType("slam", DMG_BURN, TRACER_NONE, 0, 0, 5, 0, 0);
	}

	return &def;
}
