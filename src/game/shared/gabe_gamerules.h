#ifndef GABE_GAMERULES_H
#define GABE_GAMERULES_H
#pragma once
#include "multiplay_gamerules.h"
#include "ammodef.h"

#ifdef CLIENT_DLL
#define CGabeGameRules C_GabeGameRules
#define CGabeGameRulesProxy C_GabeGameRulesProxy
#endif

class CGabeGameRulesProxy : public CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS(CGabeGameRulesProxy, CHL2MPGameRulesProxy);
	DECLARE_NETWORKCLASS();
};

class CGabeGameRules : public CHL2MPRules
{
public:
	DECLARE_CLASS(CGabeGameRules, CHL2MPRules);

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.
#else
	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif

	CGabeGameRules();
	virtual ~CGabeGameRules();

	virtual void Precache(void);
	virtual bool ShouldCollide(int collisionGroup0, int collisionGroup1);
	virtual bool ClientCommand(CBaseEntity* pEdict, const CCommand& args);
	virtual void Think(void);
	virtual void CreateStandardEntities(void);
	virtual const char* GetGameDescription(void);
	virtual void ClientDisconnected(edict_t* pClient);

#ifndef CLIENT_DLL

	virtual bool CanHavePlayerItem(CBasePlayer* pPlayer, CBaseCombatWeapon* pItem);
	virtual bool FShouldSwitchWeapon(CBasePlayer* pPlayer, CBaseCombatWeapon* pWeapon);

	void AddLevelDesignerPlacedObject(CBaseEntity* pEntity);
	void RemoveLevelDesignerPlacedObject(CBaseEntity* pEntity);
	const char* GetChatFormat(bool bTeamOnly, CBasePlayer* pPlayer);

#endif

};

inline CGabeGameRules* GabeGameRules()
{
	return static_cast<CGabeGameRules*>(g_pGameRules);
}

#endif