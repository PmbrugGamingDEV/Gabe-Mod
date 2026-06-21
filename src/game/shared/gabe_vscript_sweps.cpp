#include "cbase.h"
#include "basecombatweapon.h"
#include "baseplayer.h"
#include "vscript_server.h"

class CWeaponVScript : public CBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponVScript, CBaseCombatWeapon);
	DECLARE_SERVERCLASS();

public:
	DECLARE_DATADESC();

	CWeaponVScript()
	{
		m_szScriptFile[0] = 0;
		m_hScriptScope = INVALID_HSCRIPT;
	}

	void Spawn() OVERRIDE
	{
		BaseClass::Spawn();

		Precache();

		if (!m_szScriptFile[0])
			Q_strncpy(m_szScriptFile, "weapons/weapon_test", sizeof(m_szScriptFile));

		RunWeaponScript();

		CallScript("Spawn");
	}

	void Precache() OVERRIDE
	{
		BaseClass::Precache();

		PrecacheModel("models/weapons/w_smg1.mdl");
		PrecacheModel("models/weapons/v_smg1.mdl");
	}

	bool Deploy() OVERRIDE
	{
		bool result = BaseClass::Deploy();
		CallScript("Deploy");
		return result;
	}

	bool Holster(CBaseCombatWeapon* pSwitchingTo) OVERRIDE
	{
		CallScript("Holster");
		return BaseClass::Holster(pSwitchingTo);
	}

	void PrimaryAttack() OVERRIDE
	{
		CallScript("PrimaryAttack");
	}

	void SecondaryAttack() OVERRIDE
	{
		CallScript("SecondaryAttack");
	}

	void Reload() OVERRIDE
	{
		CallScript("Reload");
	}

	void ItemPostFrame() OVERRIDE
	{
		BaseClass::ItemPostFrame();
		CallScript("Think");
	}

	CBasePlayer* GetOwnerPlayerScript()
	{
		return ToBasePlayer(GetOwner());
	}

	void SetNextPrimaryFireScript(float t)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + t;
	}

	void SetNextSecondaryFireScript(float t)
	{
		m_flNextSecondaryAttack = gpGlobals->curtime + t;
	}

	void SetClip1Script(int clip)
	{
		m_iClip1 = clip;
	}

	int GetClip1Script()
	{
		return m_iClip1;
	}

	void SetClip2Script(int clip)
	{
		m_iClip2 = clip;
	}

	int GetClip2Script()
	{
		return m_iClip2;
	}

	void EmitWeaponSoundScript(const char* sound)
	{
		if (sound && sound[0])
			EmitSound(sound);
	}

	void RemoveWeaponScript()
	{
		UTIL_Remove(this);
	}

	void SetWorldModelScript(const char* model)
	{
		if (model && model[0])
		{
			PrecacheModel(model);
			SetModel(model);
		}
	}

	void SetViewModelScript(const char* model)
	{
		if (model && model[0])
		{
			PrecacheModel(model);
			m_iViewModelIndex = modelinfo->GetModelIndex(model);
		}
	}

private:
	char m_szScriptFile[MAX_PATH];
	HSCRIPT m_hScriptScope;

	void RunWeaponScript()
	{
		if (!g_pScriptVM)
			return;

		if (m_hScriptScope != INVALID_HSCRIPT)
			return;

		m_hScriptScope = g_pScriptVM->CreateScope("WeaponScriptScope");

		if (m_hScriptScope == INVALID_HSCRIPT)
			return;

		g_pScriptVM->SetValue(m_hScriptScope, "self", this);

		CFmtStr path("scripts/vscripts/%s.nut", m_szScriptFile);

		CUtlBuffer buf;
		if (!filesystem->ReadFile(path.Access(), "GAME", buf))
		{
			Warning("Weapon VScript: missing %s\n", path.Access());
			return;
		}

		buf.PutChar(0);
		g_pScriptVM->Run((const char*)buf.Base(), true, m_hScriptScope);
	}

	void CallScript(const char* func)
	{
		if (!g_pScriptVM || m_hScriptScope == INVALID_HSCRIPT)
			return;

		HSCRIPT hFunc = g_pScriptVM->LookupFunction(func, m_hScriptScope);
		if (hFunc)
			g_pScriptVM->Call(hFunc, m_hScriptScope, true, NULL, this);
	}
};

IMPLEMENT_SERVERCLASS_ST(CWeaponVScript, DT_WeaponVScript)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_vscript, CWeaponVScript);
PRECACHE_WEAPON_REGISTER(weapon_vscript);

BEGIN_DATADESC(CWeaponVScript)
DEFINE_KEYFIELD(m_szScriptFile, FIELD_STRING, "scriptfile"),
END_DATADESC()

BEGIN_SCRIPTDESC_ROOT(CWeaponVScript, "VScript weapon")
DEFINE_SCRIPTFUNC(GetOwnerPlayerScript, "")
DEFINE_SCRIPTFUNC(SetNextPrimaryFireScript, "")
DEFINE_SCRIPTFUNC(SetNextSecondaryFireScript, "")
DEFINE_SCRIPTFUNC(SetClip1Script, "")
DEFINE_SCRIPTFUNC(GetClip1Script, "")
DEFINE_SCRIPTFUNC(SetClip2Script, "")
DEFINE_SCRIPTFUNC(GetClip2Script, "")
DEFINE_SCRIPTFUNC(EmitWeaponSoundScript, "")
DEFINE_SCRIPTFUNC(RemoveWeaponScript, "")
DEFINE_SCRIPTFUNC(SetWorldModelScript, "")
DEFINE_SCRIPTFUNC(SetViewModelScript, "")
END_SCRIPTDESC()