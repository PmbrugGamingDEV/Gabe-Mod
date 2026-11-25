//===== Copyright (c) 1996-2026 Valve Corporation. All rights reserved. =====//
// 
//
// Purpose: A versatile tool for manipulating entities in various ways.
//
// $NoKeywords: $
//=================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "explode.h"
#include "hl2_player.h"
#include "game.h"
#include "entityflame.h"
#include "IEffects.h"
#include "beam_shared.h"
#include "GABE_SHARED.H"
#include "in_buttons.h"
#include "imath.h"
#include "baseanimating.h"
#include "vphysics/constraints.h"
#include "rope.h"
#include "props.h"
#include "physics_prop_ragdoll.h"
#include "usermessages.h"
#include "sprite.h"
#include "ai_basenpc.h"
#include "baseflex.h"
#include "vphysics_interface.h"
#include "te_effect_dispatch.h"
#include "gabe_undo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "memdbgon.h"

using GabeMod::CGabeServerFuncs;

extern IEffects* g_pEffects;

class CWeaponMultitool;

#define MAX_DUPE_RAGDOLL_BONES 64

ConVar mtool_mode(
	"mtool_mode",
	"remove",          // default
	FCVAR_ARCHIVE,
	"Multitool mode selector"
);

struct ColorPreset
{
	int r, g, b;
	const char* name;
};

BEGIN_STRUCT_LIST(ColorPreset, g_ColorPresets)
STRUCT_ENTRY(255, 0, 0, "Red")
STRUCT_ENTRY(255, 165, 0, "Orange")
STRUCT_ENTRY(255, 255, 0, "Yellow")
STRUCT_ENTRY(0, 255, 0, "Green")
STRUCT_ENTRY(0, 0, 255, "Blue")
STRUCT_ENTRY(75, 0, 130, "Indigo")
STRUCT_ENTRY(128, 0, 128, "Purple")
STRUCT_ENTRY(238, 130, 238, "Violet")
STRUCT_ENTRY(255, 192, 203, "Pink")
STRUCT_ENTRY(0, 0, 0, "Black")
STRUCT_ENTRY(255, 255, 255, "White")
STRUCT_ENTRY(0, 0, 0, "Rainbow")
END_STRUCT_LIST(g_ColorPresets)

const char* g_DecalNames[] =
{
	"YellowBlood",
	"Bigshot",
	"RedGlowFade",
	"BeerSplash",
	"Blood",
	"Scorch",
	"ManhackCut",
	"FadingScorch",
	"Rollermine.Crater",
	"Impact.Concrete",
	"Impact.Metal",
	"Impact.Glass",
	"Impact.Sand"
};
const int DECAL_COUNT = sizeof(g_DecalNames) / sizeof(g_DecalNames[0]);

struct RenderFxEntry
{
	RenderFx_t fx;
	const char* name;
};

RenderFxEntry g_RenderFxModes[] =
{
	{ kRenderFxNone,             "None" },
	{ kRenderFxPulseSlow,        "Pulse Slow" },
	{ kRenderFxPulseFast,        "Pulse Fast" },
	{ kRenderFxPulseSlowWide,    "Pulse Wide" },
	{ kRenderFxFadeSlow,         "Fade Slow" },
	{ kRenderFxFadeFast,         "Fade Fast" },
	{ kRenderFxSolidSlow,        "Solid Slow" },
	{ kRenderFxSolidFast,        "Solid Fast" },
	{ kRenderFxStrobeSlow,       "Strobe Slow" },
	{ kRenderFxStrobeFast,       "Strobe Fast" },
	{ kRenderFxStrobeFaster,     "Strobe Faster" },
	{ kRenderFxFlickerSlow,      "Flicker Slow" },
	{ kRenderFxFlickerFast,      "Flicker Fast" },
	{ kRenderFxNoDissipation,    "No Dissipation" },
	{ kRenderFxDistort,          "Distort" },
	{ kRenderFxHologram,         "Hologram" },
	{ kRenderFxExplode,          "Explode" },
	{ kRenderFxGlowShell,        "Glow Shell" },
	{ kRenderFxClampMinScale,    "Clamp Min Scale" }
};

const int RENDERFX_COUNT = sizeof(g_RenderFxModes) / sizeof(g_RenderFxModes[0]);

class CToolBalloon : public CBaseAnimating
{
public:
	DECLARE_CLASS(CToolBalloon, CBaseAnimating);
	DECLARE_DATADESC();

	void Spawn() override;
	void Precache() override;
	void BalloonThink();

	void SetLift(float flLift)
	{
		m_flLift = flLift;
	}

private:
	float m_flLift;
};

LINK_ENTITY_TO_CLASS(tool_balloon, CToolBalloon);

BEGIN_DATADESC(CToolBalloon)
DEFINE_FIELD(m_flLift, FIELD_FLOAT),
DEFINE_THINKFUNC(BalloonThink),
END_DATADESC()

void CToolBalloon::Precache()
{
	// Replace this with whatever balloon model you actually have.
	PrecacheModel("models/props_junk/watermelon01.mdl");

	BaseClass::Precache();
}

void CToolBalloon::Spawn()
{
	Precache();

	SetModel("models/props_junk/watermelon01.mdl");

	SetSolid(SOLID_VPHYSICS);
	SetMoveType(MOVETYPE_VPHYSICS);

	VPhysicsInitNormal(
		SOLID_VPHYSICS,
		0,
		false
	);

	m_flLift = 300.0f;

	SetThink(&CToolBalloon::BalloonThink);
	SetNextThink(gpGlobals->curtime + 0.02f);
}

void CToolBalloon::BalloonThink()
{
	IPhysicsObject* pPhysics = VPhysicsGetObject();

	if (pPhysics)
	{
		// Apply upward force continuously.
		Vector force(0, 0, m_flLift);

		pPhysics->ApplyForceCenter(force);
		pPhysics->Wake();
	}

	SetNextThink(gpGlobals->curtime + 0.02f);
}

class CRainbowThinker : public CBaseEntity
{
public:
	DECLARE_CLASS(CRainbowThinker, CBaseEntity);
	DECLARE_DATADESC();

	EHANDLE m_hTargetEnt;

	void Spawn()
	{
		SetNextThink(gpGlobals->curtime + 0.05f);
	}

	void Think()
	{
		CBaseEntity* pTarget = m_hTargetEnt;
		if (pTarget)
		{
			float t = gpGlobals->curtime * 2.0f;
			int r = static_cast<int>(sin(t + 0.0f) * 127 + 128);
			int g = static_cast<int>(sin(t + 2.0f) * 127 + 128);
			int b = static_cast<int>(sin(t + 4.0f) * 127 + 128);
			pTarget->SetRenderColor(r, g, b);
			SetNextThink(gpGlobals->curtime + 0.05f);
		}
		else
		{
			UTIL_Remove(this);
		}
	}
};

LINK_ENTITY_TO_CLASS(env_rainbowthink, CRainbowThinker);

BEGIN_DATADESC(CRainbowThinker)
DEFINE_FIELD(m_hTargetEnt, FIELD_EHANDLE),
DEFINE_FUNCTION(Think),
END_DATADESC()

static Vector GetDuplicateSpawnPos(CBasePlayer* pPlayer)
{
	Vector forward;
	pPlayer->EyeVectors(&forward);
	return pPlayer->EyePosition() + forward * 96.0f;
}

ConVar gabeplus_multitool_pmessagetext("gabe_multitool_pmessagetext", "Customize me with gabe+_multitool_pmessagetext!", FCVAR_ARCHIVE);

class CL_Watermelon : public CBaseAnimating
{
public:
	DECLARE_CLASS(CL_Watermelon, CBaseAnimating);
	DECLARE_DATADESC();

	void Spawn() override;
	void Precache() override;
	void Touch(CBaseEntity* pOther) override;
	void Think() override;
	void UpdateOnRemove()
	{
		if (m_hDynamicLight.Get()) UTIL_Remove(m_hDynamicLight.Get());
		BaseClass::UpdateOnRemove();
	}
	void SetLightColor(int r, int g, int b); // Function to set the light color

private:
	float m_flDamageAmount; // Damage to inflict on NPCs
	CHandle<CBaseEntity> m_hDynamicLight; // Handle to the dynamic light entity
};

LINK_ENTITY_TO_CLASS(l_watermelon, CL_Watermelon);

BEGIN_DATADESC(CL_Watermelon)
DEFINE_FIELD(m_flDamageAmount, FIELD_FLOAT),
DEFINE_FIELD(m_hDynamicLight, FIELD_EHANDLE),
END_DATADESC()

void CL_Watermelon::Precache()
{
	PrecacheModel("models/props_junk/watermelon01.mdl");
	BaseClass::Precache();

	// Precache the burn sound effect
	PrecacheScriptSound("Watermelon.Burn");
}

void CL_Watermelon::Spawn()
{
	Precache();

	SetModel("models/props_junk/watermelon01.mdl");
	SetSolid(SOLID_VPHYSICS);
	SetMoveType(MOVETYPE_VPHYSICS);

	// Initialize physics
	VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);

	// Set the amount of damage the watermelon will inflict
	m_flDamageAmount = 25.0f; // You can adjust the damage amount here

	// Create and configure the dynamic light
	m_hDynamicLight = CreateEntityByName("light_dynamic");
	if (m_hDynamicLight)
	{
		m_hDynamicLight->SetParent(this);
		m_hDynamicLight->SetLocalOrigin(vec3_origin);
		m_hDynamicLight->KeyValue("brightness", "5");  // Adjust brightness as needed
		m_hDynamicLight->KeyValue("distance", "500");  // Adjust light distance as needed
		m_hDynamicLight->Spawn();
	}

	SetThink(&CL_Watermelon::Think);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CL_Watermelon::Think()
{
	// Perform any necessary actions
	SetNextThink(gpGlobals->curtime + 0.1f); // Keep thinking
}

void CL_Watermelon::Touch(CBaseEntity* pOther)
{
	if (pOther->IsNPC())
	{
		// Inflict damage on the NPC or player
		CTakeDamageInfo damageInfo(this, this, m_flDamageAmount, DMG_BURN);
		pOther->TakeDamage(damageInfo);
		// Set up the effect data
		CEffectData data;
		data.m_vOrigin = GetAbsOrigin();
		data.m_vNormal = Vector(0, 0, 1);
		data.m_fFlags = 0;
		data.m_nEntIndex = entindex();

		// Dispatch the sparks effect
		DispatchEffect("Sparks", data);
		// Play burn sound effect
		EmitSound("WeaponDissolve.Dissolve");
	}
}

void CL_Watermelon::SetLightColor(int r, int g, int b)
{
	if (m_hDynamicLight)
	{
		m_hDynamicLight->SetRenderColor(r, g, b);
	}
}

class CToolThruster : public CBaseAnimating
{
public:
	DECLARE_CLASS(CToolThruster, CBaseAnimating);
	DECLARE_DATADESC();

	void Precache()
	{
		PrecacheModel("models/props_junk/watermelon01.mdl");
		BaseClass::Precache();
	}

	void Spawn()
	{
		Precache();

		SetModel("models/props_junk/watermelon01.mdl");

		// The watermelon is visual only.
		// Physics force is applied to the target prop.
		SetSolid(SOLID_NONE);
		SetMoveType(MOVETYPE_NONE);

		m_flForce = 500.0f;
		m_bEnabled = true;
		m_nPowerLevel = 0;

		SetRenderColor(255, 0, 0);

		SetThink(&CToolThruster::ThrusterThink);
		SetNextThink(gpGlobals->curtime + 0.02f);
	}

	void SetupThruster(
		CBaseEntity* pTarget,
		const Vector& vecWorldPos,
		const Vector& vecDirection,
		int nPowerLevel)
	{
		if (!pTarget)
			return;

		m_hTarget = pTarget;

		// Store thrust direction relative to the target.
		VectorIRotate(
			vecDirection,
			pTarget->EntityToWorldTransform(),
			m_vecLocalDirection
		);

		VectorNormalize(m_vecLocalDirection);

		VectorITransform(
			vecWorldPos,
			pTarget->EntityToWorldTransform(),
			m_vecLocalOffset
		);

		// Attach the watermelon visually.
		SetParent(pTarget);
		SetLocalOrigin(m_vecLocalOffset);
		SetLocalAngles(vec3_angle);

		SetPowerLevel(nPowerLevel);
	}

	void SetPowerLevel(int nLevel)
	{
		m_nPowerLevel = clamp(nLevel, 0, 2);

		switch (m_nPowerLevel)
		{
		case 0: // LOW - RED
			m_flForce = 5000.0f;
			SetRenderColor(255, 0, 0);
			break;

		case 1: // MEDIUM - GREEN
			m_flForce = 15000.0f;
			SetRenderColor(0, 255, 0);
			break;

		case 2: // HIGH - BLUE
			m_flForce = 40000.0f;
			SetRenderColor(0, 100, 255);
			break;
		}
	}

	void ThrusterThink()
	{
		CBaseEntity* pTarget = m_hTarget.Get();

		if (!pTarget)
		{
			UTIL_Remove(this);
			return;
		}

		IPhysicsObject* pPhysics =
			pTarget->VPhysicsGetObject();

		if (pPhysics && m_bEnabled)
		{
			Vector vecDirection;

			// Rotate thrust direction with the prop.
			VectorRotate(
				m_vecLocalDirection,
				pTarget->EntityToWorldTransform(),
				vecDirection
			);

			Vector vecWorldPos;

			VectorTransform(
				m_vecLocalOffset,
				pTarget->EntityToWorldTransform(),
				vecWorldPos
			);

			// Approximate continuous force using a fixed think interval.
			Vector vecForce =
				vecDirection * m_flForce * 0.02f;

			pPhysics->ApplyForceOffset(
				vecForce,
				vecWorldPos
			);

			pPhysics->Wake();
		}

		SetNextThink(gpGlobals->curtime + 0.02f);
	}

	void Toggle()
	{
		m_bEnabled = !m_bEnabled;
	}

	CBaseEntity* GetTarget()
	{
		return m_hTarget.Get();
	}

private:
	EHANDLE m_hTarget;

	Vector m_vecLocalOffset;
	Vector m_vecLocalDirection;

	float m_flForce;
	bool m_bEnabled;
	int m_nPowerLevel;
};

LINK_ENTITY_TO_CLASS(tool_thruster, CToolThruster);

BEGIN_DATADESC(CToolThruster)
DEFINE_FIELD(m_hTarget, FIELD_EHANDLE),
DEFINE_FIELD(m_vecLocalOffset, FIELD_VECTOR),
DEFINE_FIELD(m_vecLocalDirection, FIELD_VECTOR),
DEFINE_FIELD(m_flForce, FIELD_FLOAT),
DEFINE_FIELD(m_bEnabled, FIELD_BOOLEAN),
DEFINE_FIELD(m_nPowerLevel, FIELD_INTEGER),
DEFINE_THINKFUNC(ThrusterThink),
END_DATADESC()

class CWeaponMultitool : public CBaseHL2MPCombatWeapon
{
public:
	DECLARE_CLASS(CWeaponMultitool, CBaseHL2MPCombatWeapon);
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CWeaponMultitool();

	virtual void Precache();
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual void ItemPostFrame();
	virtual bool Deploy();

private:
	CBaseEntity* FindEntityInFront();
	void SendHudUpdate();
	void ApplyToolAction(CBaseEntity* pEnt);
	void ShowDistance(CBaseEntity* pEnt);
	void ThrowLightWatermelon();
	void CreateBalloon(CBaseEntity* pTarget);

	int m_nMultitoolMode;
	int m_nColorIndex;
	int m_nDecalIndex;
	EHANDLE m_hConstraintTarget1;
	EHANDLE m_hConstraintTarget2;
	int m_nConstraintType; // 0: BallSocket, 1: Weld, 2: Axis
	Vector m_vecConstraintPos1;

	int m_nLightWatermelonColor;

	int m_nMaterialIndex;

	CUtlVector<EHANDLE> m_Constraints;


	struct DupedRagdollBone_t
	{
		Vector  localPos;   // offset from entity origin
		QAngle  ang;        // world angles are fine
	};

	struct DupedEntityData_t
	{
		string_t className;
		string_t modelName;

		QAngle   angles;
		color32  color;

		Vector   mins;
		Vector   maxs;

		float    mass;
		bool     valid;

		bool     isRagdoll;
		int      boneCount;

		DupedRagdollBone_t bones[MAX_DUPE_RAGDOLL_BONES];
	};

	int m_nFaceIndex;
	EHANDLE m_hFaceTarget;

	DupedEntityData_t m_DupeData;

	// Balloon lift forces
	float m_BalloonLiftForces[10] = { 100.0f, 200.0f, 300.0f, 400.0f, 500.0f, 600.0f, 700.0f, 800.0f, 900.0f, 1000.0f };
	int m_nBalloonLiftIndex;

	int m_nThrusterPower;
};

IMPLEMENT_SERVERCLASS_ST(CWeaponMultitool, DT_WeaponMultitool)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS(weapon_multitool, CWeaponMultitool);
PRECACHE_WEAPON_REGISTER(weapon_multitool);

BEGIN_DATADESC(CWeaponMultitool)
DEFINE_FIELD(m_hConstraintTarget1, FIELD_EHANDLE),
DEFINE_FIELD(m_hConstraintTarget2, FIELD_EHANDLE),
DEFINE_FIELD(m_nConstraintType, FIELD_INTEGER),
DEFINE_FIELD(m_nMultitoolMode, FIELD_INTEGER),
DEFINE_FIELD(m_nLightWatermelonColor, FIELD_INTEGER),
DEFINE_FIELD(m_nColorIndex, FIELD_INTEGER),
DEFINE_FIELD(m_nDecalIndex, FIELD_INTEGER),
DEFINE_FIELD(m_nFaceIndex, FIELD_INTEGER),
DEFINE_FIELD(m_hFaceTarget, FIELD_EHANDLE),
END_DATADESC()

CWeaponMultitool::CWeaponMultitool()
{
	m_nMultitoolMode = 0;
	m_nColorIndex = 0;
	m_nDecalIndex = 0;
	m_hConstraintTarget1 = NULL;
	m_hConstraintTarget2 = NULL;
	m_nConstraintType = 0;
	m_DupeData.valid = false;
	m_bFiresUnderwater = true;
	m_nLightWatermelonColor = 0;
	m_nMaterialIndex = 0;
	m_nFaceIndex = 0;
	m_hFaceTarget = nullptr;
	m_nBalloonLiftIndex = 2;
	m_nThrusterPower = 0; // Red / Low
}

enum MultitoolMode_t
{
	MODE_REMOVE = 0,
	MODE_DISTANCE,
	MODE_COLOR,
	MODE_CONSTRAINT,
	MODE_IGNITE,
	MODE_DUPLICATE,
	MODE_EXPLODE,
	MODE_POINTMESSAGE,
	MODE_LIGHT_WATERMELON,
	MODE_DECAL,
	MODE_MATERIAL,
	MODE_FACEPOSER,
	MODE_BALLOON,
	MODE_THRUSTER,
	MODE_MAX
};

MultitoolMode_t StringToMode(const char* psz)
{
	if (!psz)
		return MODE_REMOVE;

	if (!Q_stricmp(psz, "remove")) return MODE_REMOVE;
	if (!Q_stricmp(psz, "distance")) return MODE_DISTANCE;
	if (!Q_stricmp(psz, "color")) return MODE_COLOR;
	if (!Q_stricmp(psz, "constraint")) return MODE_CONSTRAINT;
	if (!Q_stricmp(psz, "ignite")) return MODE_IGNITE;
	if (!Q_stricmp(psz, "duplicate")) return MODE_DUPLICATE;
	if (!Q_stricmp(psz, "explode")) return MODE_EXPLODE;
	if (!Q_stricmp(psz, "pointmessage")) return MODE_POINTMESSAGE;
	if (!Q_stricmp(psz, "light_watermelon")) return MODE_LIGHT_WATERMELON;
	if (!Q_stricmp(psz, "decal")) return MODE_DECAL;
	if (!Q_stricmp(psz, "material")) return MODE_MATERIAL;
	if (!Q_stricmp(psz, "faceposer")) return MODE_FACEPOSER;
	if (!Q_stricmp(psz, "balloon")) return MODE_BALLOON;
	if (!Q_stricmp(psz, "thruster")) return MODE_THRUSTER;

	return MODE_REMOVE;
}

BEGIN_LIST(g_szModeNames)
LIST_ENTRY(Remove),
LIST_ENTRY(Distance),
LIST_ENTRY(Color),
LIST_ENTRY(Constraints),
LIST_ENTRY(Ignite),
LIST_ENTRY(Duplicate),
LIST_ENTRY(Explode),
LIST_ENTRY(Point_Message),
LIST_ENTRY(Light_Watermelon),
LIST_ENTRY(Decal),
LIST_ENTRY(Render_FX),
LIST_ENTRY(Faceposer_NPCs)
LIST_ENTRY(Balloon)
LIST_ENTRY(Thruster)
END_LIST(g_szModeNames)

BEGIN_LIST(g_szConstraintTypes)
LIST_ENTRY(BallSocket),
LIST_ENTRY(Weld),
LIST_ENTRY(Rope),
LIST_ENTRY(Slider),
LIST_ENTRY(Hinge)
END_LIST(g_szConstraintTypes)

CBaseEntity* CWeaponMultitool::FindEntityInFront()
{
	trace_t tr;
	Vector start = GetOwner()->EyePosition();
	Vector forward;
	ToBasePlayer(GetOwner())->EyeVectors(&forward);

	UTIL_TraceLine(start, start + forward * 2000, MASK_SOLID, GetOwner(), COLLISION_GROUP_NONE, &tr);
	return tr.m_pEnt;
}

bool CWeaponMultitool::Deploy()
{
	bool result = BaseClass::Deploy();
	SendHudUpdate();
	return result;
}

void CWeaponMultitool::Precache()
{
	BaseClass::Precache();
	PrecacheModel("cable/cable.vmt");
}

void CWeaponMultitool::PrimaryAttack()
{
	CBaseEntity* pEnt = FindEntityInFront();
	if (!pEnt || pEnt == GetOwner())
		return;

	ApplyToolAction(pEnt);

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	int r = random->RandomInt(1, 2);

	char snd[128];
	Q_snprintf(snd, sizeof(snd), "multitool/airboat_gun_lastshot%d.wav", r);

	PrecacheSound(snd);
	EmitSound(snd);

	trace_t tr;
	Vector vecStart = pPlayer->EyePosition();
	Vector vecForward;
	pPlayer->EyeVectors(&vecForward);
	Vector vecEnd = vecStart + vecForward * 2048;

	UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

	PrecacheModel("effects/tool_tracer.vmt");
	CBeam* pBeam = CBeam::BeamCreate("effects/tool_tracer.vmt", 5);
	if (pBeam)
	{
		pBeam->PointEntInit(tr.endpos, this);
		pBeam->SetEndAttachment(1);
		pBeam->SetBrightness(255);
		pBeam->SetColor(255, 255, 255);
		pBeam->RelinkBeam();
		pBeam->LiveForTime(0.1);
	}

	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();

	CSprite* pSprite = CSprite::SpriteCreate("sprites/select_dot.vmt", tr.endpos, false);
	if (pSprite)
	{
		// align to surface
		QAngle ang;
		VectorAngles(tr.plane.normal, ang);

		pSprite->SetAbsAngles(ang);

		// push slightly off surface to avoid z-fighting
		pSprite->SetAbsOrigin(tr.endpos + tr.plane.normal * 0.5f);

		// visuals
		pSprite->SetScale(0.5f);
		pSprite->SetBrightness(255);
		pSprite->SetColor(255, 255, 255);

		// make it fade quickly like a tracer
		pSprite->FadeAndDie(0.1f);
	}

	if (tr.fraction < 1.0f)
	{
		g_pEffects->Sparks(tr.endpos, 1, 1);
	}

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.15f;
}

void CWeaponMultitool::SecondaryAttack()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	if (m_nMultitoolMode == MODE_DUPLICATE)
	{

		CBaseEntity* pEnt = FindEntityInFront();

		if (!pEnt)
		{
			Warning("[Duplicator] could not copy whatever's in-front!\n");
			return;
		}

		if (pEnt->GetClassname() && (Q_stricmp(pEnt->GetClassname(), "prop_vehicle_jeep") == 0 || Q_stricmp(pEnt->GetClassname(), "prop_vehicle_airboat") == 0))
		{
			pEnt->AddTimedOverlay("Cannot dupe vehicles, sorry!", 2);
			return;
		}

		if (pEnt->IsWorld())
		{
			CGabeServerFuncs::HudText(pPlayer, "Nothing to copy.", 255, 100, 100);
			return;
		}

		// -----------------------------------------
		// Base info (always safe)
		// -----------------------------------------
		m_DupeData.className = AllocPooledString(pEnt->GetClassname());
		m_DupeData.modelName = pEnt->GetModelName();
		m_DupeData.angles = pEnt->GetAbsAngles();
		m_DupeData.color = pEnt->GetRenderColor();
		m_DupeData.valid = true;

		// -----------------------------------------
		// Collision bounds (SAFE)
		// -----------------------------------------
		if (pEnt->CollisionProp())
		{
			m_DupeData.mins = pEnt->CollisionProp()->OBBMins();
			m_DupeData.maxs = pEnt->CollisionProp()->OBBMaxs();
		}
		else
		{
			m_DupeData.mins = Vector(-8, -8, -8);
			m_DupeData.maxs = Vector(8, 8, 8);
		}

		// -----------------------------------------
		// Physics mass (SAFE)
		// -----------------------------------------
		m_DupeData.mass = 0.0f;

		IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
		if (pPhys)
		{
			m_DupeData.mass = pPhys->GetMass();
		}

		// -----------------------------------------
		// Ragdoll handling (SAFE)
		// -----------------------------------------
		m_DupeData.isRagdoll = false;
		m_DupeData.boneCount = 0;

		CBaseAnimating* pAnim = pEnt->GetBaseAnimating();

		if (pAnim && pAnim->IsRagdoll())
		{
			CRagdollProp* ragdollProp =
				dynamic_cast<CRagdollProp*>(pAnim);

			if (ragdollProp)
			{
				ragdoll_t* pRagdoll =
					ragdollProp->GetRagdoll();

				if (pRagdoll && pRagdoll->listCount > 0)
				{
					m_DupeData.isRagdoll = true;

					Vector baseOrigin =
						pEnt->GetAbsOrigin();

					m_DupeData.boneCount =
						MIN(pRagdoll->listCount,
							MAX_DUPE_RAGDOLL_BONES);

					for (int i = 0;
						i < m_DupeData.boneCount;
						i++)
					{
						IPhysicsObject* pBonePhys =
							pRagdoll->list[i].pObject;

						if (pBonePhys)
						{
							Vector worldPos;
							QAngle worldAng;

							pBonePhys->GetPosition(
								&worldPos,
								&worldAng
							);

							m_DupeData.bones[i].localPos =
								worldPos - baseOrigin;

							m_DupeData.bones[i].ang =
								worldAng;
						}
					}
				}
			}
		}

		// -----------------------------------------
		// Success feedback
		// -----------------------------------------
		CGabeServerFuncs::HudText(
			pPlayer,
			UTIL_VarArgs(
				"Copied: %s",
				STRING(m_DupeData.className)
			),
			150, 255, 150
		);
	}

	else if (m_nMultitoolMode == MODE_THRUSTER)
	{
		CBaseEntity* pTarget = FindEntityInFront();

		if (!pTarget || pTarget->IsWorld())
			return;

		CBaseEntity* pEnt = NULL;
		bool bFoundThruster = false;

		while ((pEnt = gEntList.FindEntityByClassname(
			pEnt, "tool_thruster")) != NULL)
		{
			CToolThruster* pThruster =
				static_cast<CToolThruster*>(pEnt);

			if (pThruster->GetTarget() == pTarget)
			{
				pThruster->Toggle();
				bFoundThruster = true;

				CGabeServerFuncs::HudText(
					pPlayer,
					"Thruster toggled.",
					150, 255, 150
				);

				break;
			}
		}

		if (!bFoundThruster)
		{
			CGabeServerFuncs::HudText(
				pPlayer,
				"No thruster attached to this object.",
				255, 150, 100
			);
		}

		m_flNextSecondaryAttack =
			gpGlobals->curtime + 0.3f;

		return;
	}
	else
	{
		return;
	}

	// -----------------------------------------
	// Visual trace feedback
	// -----------------------------------------
	trace_t tr;
	Vector vecStart = pPlayer->EyePosition();
	Vector vecForward;
	pPlayer->EyeVectors(&vecForward);
	Vector vecEnd = vecStart + vecForward * 2048;

	UTIL_TraceLine(
		vecStart,
		vecEnd,
		MASK_SOLID,
		pPlayer,
		COLLISION_GROUP_NONE,
		&tr
	);

	if (tr.fraction < 1.0f)
	{
		g_pEffects->Sparks(tr.endpos, 1, 1);
		g_pEffects->Ricochet(tr.endpos, tr.plane.normal);
		g_pEffects->EnergySplash(tr.endpos, tr.plane.normal);
	}

	SendWeaponAnim(ACT_VM_RELOAD);
	WeaponSound(RELOAD);

	m_flNextSecondaryAttack =
		gpGlobals->curtime + 0.3f;
}

void CWeaponMultitool::ThrowLightWatermelon()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecDir;
	pPlayer->EyeVectors(&vecDir);

	CBaseEntity* pEnt = CreateEntityByName("l_watermelon");
	if (!pEnt)
		return;

	pEnt->SetAbsOrigin(vecSrc);
	pEnt->SetAbsAngles(pPlayer->EyeAngles());
	if (DispatchSpawn(pEnt) < 0) return;
	pEnt->Activate();
	GabeUndo_AddEntity(pPlayer, pEnt, "Light");

	IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();
	if (pPhys)
	{
		Vector vel = vecDir * 1000.0f;
		Vector ang = RandomAngularImpulse(-200, 200);
		pPhys->AddVelocity(&vel, &ang);
	}

	// Apply color
	const ColorPreset& col = g_ColorPresets[m_nLightWatermelonColor];
	pEnt->SetRenderColor(col.r, col.g, col.b);
}

void CWeaponMultitool::CreateBalloon(CBaseEntity* pTarget)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	trace_t tr;

	Vector start = pPlayer->EyePosition();
	Vector forward;
	pPlayer->EyeVectors(&forward);

	UTIL_TraceLine(
		start,
		start + forward * 2048.0f,
		MASK_SOLID,
		pPlayer,
		COLLISION_GROUP_NONE,
		&tr
	);

	if (tr.fraction == 1.0f)
		return;

	Vector balloonPos =
		tr.endpos + Vector(0, 0, 64.0f);

	CToolBalloon* pBalloon =
		static_cast<CToolBalloon*>(
			CreateEntityByName("tool_balloon")
			);

	if (!pBalloon)
		return;

	pBalloon->SetAbsOrigin(balloonPos);
	pBalloon->SetAbsAngles(vec3_angle);
	pBalloon->SetRenderColor(g_pRandom->RandomInt(0, 255), g_pRandom->RandomInt(0, 255), g_pRandom->RandomInt(0, 255), 255);

	if (DispatchSpawn(pBalloon) < 0) return;
	pBalloon->Activate();
	GabeUndo_Begin(pPlayer, "Balloon");
	GabeUndo_Add(pBalloon);

	CGabeServerFuncs::HudText(
		pPlayer,
		"Balloon created.",
		150, 255, 150
	);

	IPhysicsObject* pTargetPhys =
		pTarget->VPhysicsGetObject();

	IPhysicsObject* pBalloonPhys =
		pBalloon->VPhysicsGetObject();

	if (pTargetPhys && pBalloonPhys)
	{
		Vector targetPos = tr.endpos;
		Vector balloonAttach =
			pBalloon->WorldSpaceCenter();

		constraint_lengthparams_t rope;
		rope.Defaults();

		rope.InitWorldspace(
			pTargetPhys,
			pBalloonPhys,
			targetPos,
			balloonAttach,
			false
		);

		rope.totalLength =
			(balloonAttach - targetPos).Length();

		rope.minLength = 0.0f;

		IPhysicsConstraint* pConstraint =
			physenv->CreateLengthConstraint(
				pTargetPhys,
				pBalloonPhys,
				NULL,
				rope
			);

		CBaseEntity* pConstraintOwner = GabeUndo_OwnConstraint(pConstraint);
		if (pConstraintOwner)
		{
			m_Constraints.AddToTail(EHANDLE(pConstraintOwner));
			GabeUndo_Add(pConstraintOwner);
		}

		PrecacheModel("cable/cable.vmt");

		CRopeKeyframe* pRope = pConstraintOwner ?
			CRopeKeyframe::Create(
				pTarget,
				pBalloon,
				0,
				0,
				2,
				"cable/cable.vmt",
				10
			) : NULL;

		if (pRope)
		{
			pRope->m_Width = 1.5f;
			pRope->m_Slack = 20.0f;
			pRope->m_nSegments = 12;

			GabeUndo_AddConstraintVisual(pConstraintOwner, pRope);
		}
	}


	float mass = pTargetPhys ? pTargetPhys->GetMass() : 0.0f;

	pBalloon->SetLift(m_BalloonLiftForces[m_nBalloonLiftIndex] + mass * 25.0f);
	GabeUndo_Finish();
}


void RainbowThink(CBaseEntity* pEnt)
{
	if (!pEnt)
		return;

	float t = gpGlobals->curtime * 2.0f;
	int r = static_cast<int>(sin(t + 0.0f) * 127 + 128);
	int g = static_cast<int>(sin(t + 2.0f) * 127 + 128);
	int b = static_cast<int>(sin(t + 4.0f) * 127 + 128);

	pEnt->SetRenderColor(r, g, b);
	pEnt->SetNextThink(gpGlobals->curtime + 0.05f);
}

// ------------------------------------------------------------
// Faceposer: use actual model flex controller names, not
// imaginary preset names. Each model may expose different controls.
// ------------------------------------------------------------
ConVar mtool_face_debug("mtool_face_debug", "0", FCVAR_NONE,
	"Print available flex controllers on faceposer click");

struct FlexPair
{
	const char* flex;
	float weight;
};

struct FacePreset
{
	const char* name;
	const FlexPair* pairs;
	int pairCount;
};

#define FACE_PRESET(_name, _arr) { _name, _arr, ARRAYSIZE(_arr) }

// Left/right pairs are intentional. These are names used by
// common Source facial models; unsupported names are skipped.
static const FlexPair g_Face_Smile[] =
{
	{ "left_smile", 0.95f }, { "right_smile", 0.95f },
	{ "smile", 0.95f },
	{ "left_cheek_raiser", 0.45f }, { "right_cheek_raiser", 0.45f },
	{ "left_lid_closer", 0.15f }, { "right_lid_closer", 0.15f }
};
static const FlexPair g_Face_Sad[] =
{
	{ "left_frown", 0.9f }, { "right_frown", 0.9f },
	{ "frown", 0.9f },
	{ "left_brow_lower", 0.35f }, { "right_brow_lower", 0.35f },
	{ "left_brow_raiser", 0.30f }, { "right_brow_raiser", 0.30f }
};
static const FlexPair g_Face_Angry[] =
{
	{ "left_brow_lower", 1.0f }, { "right_brow_lower", 1.0f },
	{ "left_brow_lowerer", 1.0f }, { "right_brow_lowerer", 1.0f },
	{ "left_frown", 0.65f }, { "right_frown", 0.65f },
	{ "left_sneer", 0.35f }, { "right_sneer", 0.35f },
	{ "jaw_clencher", 0.65f }
};
static const FlexPair g_Face_Scared[] =
{
	{ "left_brow_raiser", 0.95f }, { "right_brow_raiser", 0.95f },
	{ "left_lid_raiser", 0.8f }, { "right_lid_raiser", 0.8f },
	{ "jaw_drop", 0.55f },
	{ "left_stretcher", 0.4f }, { "right_stretcher", 0.4f }
};
static const FlexPair g_Face_Surprised[] =
{
	{ "left_brow_raiser", 0.9f }, { "right_brow_raiser", 0.9f },
	{ "left_lid_raiser", 0.7f }, { "right_lid_raiser", 0.7f },
	{ "jaw_drop", 0.85f }, { "mouth_open", 0.85f }
};
static const FlexPair g_Face_Neutral[] =
{
	{ "", 0.0f }
};
static const FacePreset g_FacePresets[] =
{
	FACE_PRESET("Smile", g_Face_Smile),
	FACE_PRESET("Sad", g_Face_Sad),
	FACE_PRESET("Angry", g_Face_Angry),
	FACE_PRESET("Scared", g_Face_Scared),
	FACE_PRESET("Surprised", g_Face_Surprised),
	FACE_PRESET("Neutral", g_Face_Neutral)
};
static const int FACE_COUNT = ARRAYSIZE(g_FacePresets);
#undef FACE_PRESET

static void ResetFaceControllers(CBaseFlex* pFlex)
{
	if (!pFlex) return;
	const int count = (int)pFlex->GetNumFlexControllers();
	for (int i = 0; i < count; ++i)
		pFlex->SetFlexWeight((LocalFlexController_t)i, 0.0f);
}

// Returns how many controller names in the selected preset
// actually exist on this NPC's studio model.
static int ApplyFacePreset(CBaseFlex* pFlex, int presetIndex, bool reset)
{
	if (!pFlex || presetIndex < 0 || presetIndex >= FACE_COUNT)
		return 0;

	if (reset)
		ResetFaceControllers(pFlex);

	if (presetIndex == FACE_COUNT - 1) // Neutral
		return (int)pFlex->GetNumFlexControllers();

	const FacePreset& preset = g_FacePresets[presetIndex];
	const int count = (int)pFlex->GetNumFlexControllers();
	int applied = 0;

	for (int i = 0; i < count; ++i)
	{
		const char* controller = pFlex->GetFlexControllerName((LocalFlexController_t)i);
		if (!controller || !*controller) continue;

		for (int j = 0; j < preset.pairCount; ++j)
		{
			if (!Q_stricmp(controller, preset.pairs[j].flex))
			{
				pFlex->SetFlexWeight((LocalFlexController_t)i,
					clamp(preset.pairs[j].weight, 0.0f, 1.0f));
				++applied;
				break;
			}
		}
	}
	return applied;
}

static void DebugFaceControllers(CBaseFlex* pFlex)
{
	if (!pFlex || !mtool_face_debug.GetBool()) return;
	Msg("[Multitool Faceposer] %s controllers:\n", pFlex->GetClassname());
	for (int i = 0; i < (int)pFlex->GetNumFlexControllers(); ++i)
	{
		const char* name = pFlex->GetFlexControllerName((LocalFlexController_t)i);
		Msg("  %d: %s\n", i, name ? name : "(null)");
	}
}


void CWeaponMultitool::SendHudUpdate()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer)
		return;

	CSingleUserRecipientFilter filter(pPlayer);
	filter.MakeReliable();

	UserMessageBegin(filter, "MultitoolHUD");

	WRITE_BYTE(m_nMultitoolMode);
	WRITE_BYTE(m_nColorIndex);
	WRITE_BYTE(m_nDecalIndex);
	WRITE_BYTE(m_nMaterialIndex);
	WRITE_BYTE(m_nFaceIndex);
	WRITE_BYTE(m_nConstraintType);
	WRITE_BYTE(m_nLightWatermelonColor);
	WRITE_BYTE(m_nBalloonLiftIndex);
	WRITE_BYTE(m_nThrusterPower);

	MessageEnd();
}


void CWeaponMultitool::ApplyToolAction(CBaseEntity* pEnt)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	switch (m_nMultitoolMode)
	{
	case MODE_REMOVE:
	{
		if (pEnt->IsWorld())
			break;

		// Don’t dissolve players
		if (pEnt->IsPlayer())
		{
			CGabeServerFuncs::HudText(pPlayer, "No players.", 255, 100, 100);
			break;
		}

		CBaseAnimating* pAnim = pEnt->GetBaseAnimating();
		if (pAnim)
		{
			pAnim->Dissolve(
				NULL,
				gpGlobals->curtime,
				false,
				ENTITY_DISSOLVE_CORE,
				GetAbsOrigin(),
				500
			);

		}
		else
		{
			// fallback if it can't dissolve
			UTIL_Remove(pEnt);
		}

		break;
	}

	case MODE_DISTANCE:
	{
		ShowDistance(pEnt);
		break;
	}

	case MODE_EXPLODE:
	{
		trace_t tr;
		Vector vecStart = pPlayer->EyePosition();
		Vector vecForward;
		pPlayer->EyeVectors(&vecForward);
		Vector vecEnd = vecStart + vecForward * 2048;

		UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		ExplosionCreate(tr.endpos, QAngle(0, 0, 0), this, 150, 128, true);
		break;
	}

	case MODE_DECAL:
	{
		trace_t tr;
		Vector vecStart = pPlayer->EyePosition();
		Vector vecForward;
		pPlayer->EyeVectors(&vecForward);
		Vector vecEnd = vecStart + vecForward * 2048;

		UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.fraction < 1.0f && tr.m_pEnt)
		{
			CBroadcastRecipientFilter filter;
			UTIL_DecalTrace(&tr, g_DecalNames[m_nDecalIndex]);
		}
		break;
	}

	case MODE_COLOR:
	{
		if (m_nColorIndex == g_ColorPresets_COUNT - 1) // Rainbow
		{

			CBaseEntity* pChild = NULL;
			while ((pChild = gEntList.FindEntityByClassname(pChild, "env_rainbowthink")) != NULL)
			{
				CRainbowThinker* pThinker = dynamic_cast<CRainbowThinker*>(pChild);
				if (pThinker && pThinker->m_hTargetEnt == pEnt)
				{
					UTIL_Remove(pThinker);
				}
			}


			CRainbowThinker* pThink = static_cast<CRainbowThinker*>(CreateEntityByName("env_rainbowthink"));
			if (pThink)
			{
				pThink->m_hTargetEnt = pEnt;
				if (DispatchSpawn(pThink) >= 0)
					GabeUndo_AddEntity(pPlayer, pThink, "Rainbow");
			}
		}
		else
		{

			CBaseEntity* pChild = NULL;
			while ((pChild = gEntList.FindEntityByClassname(pChild, "env_rainbowthink")) != NULL)
			{
				CRainbowThinker* pThinker = dynamic_cast<CRainbowThinker*>(pChild);
				if (pThinker && pThinker->m_hTargetEnt == pEnt)
				{
					UTIL_Remove(pThinker);
				}
			}

			const ColorPreset& color = g_ColorPresets[m_nColorIndex];
			pEnt->SetRenderColor(color.r, color.g, color.b);
		}
		break;
	}

	case MODE_MATERIAL:
	{
		if (!pEnt || pEnt->IsWorld())
			break;

		RenderFxEntry& entry = g_RenderFxModes[m_nMaterialIndex];

		// Always reset first
		pEnt->m_nRenderFX = kRenderFxNone;
		pEnt->m_nRenderMode = kRenderNormal;
		pEnt->SetRenderColor(255, 255, 255, 255);
		pEnt->RemoveEffects(EF_NODRAW);

		// Apply selected FX
		pEnt->m_nRenderFX = entry.fx;

		// FX that need alpha blending to be visible
		switch (entry.fx)
		{
		case kRenderFxPulseSlow:
		case kRenderFxPulseFast:
		case kRenderFxPulseSlowWide:
		case kRenderFxFadeSlow:
		case kRenderFxFadeFast:
		case kRenderFxSolidSlow:
		case kRenderFxSolidFast:
		case kRenderFxStrobeSlow:
		case kRenderFxStrobeFast:
		case kRenderFxStrobeFaster:
		case kRenderFxFlickerSlow:
		case kRenderFxFlickerFast:
		case kRenderFxDistort:
		{
			pEnt->m_nRenderMode = kRenderTransAlpha;
			pEnt->SetRenderColor(255, 255, 255, 180);
			break;
		}

		case kRenderFxGlowShell:
		{
			pEnt->SetRenderColor(0, 255, 0);
			break;
		}

		case kRenderFxHologram:
		{
			pEnt->m_nRenderMode = kRenderTransColor;
			pEnt->SetRenderColor(0, 150, 255, 160);
			break;
		}

		case kRenderFxExplode:
		{
			pEnt->m_nRenderMode = kRenderTransAlpha;
			pEnt->SetRenderColor(255, 120, 0, 200);
			break;
		}

		case kRenderFxNoDissipation:
		case kRenderFxClampMinScale:
		case kRenderFxNone:
		default:
		{
			// These don't need special handling
			break;
		}
		}

		CGabeServerFuncs::HudText(
			pPlayer,
			UTIL_VarArgs("RenderFX: %s", entry.name),
			200, 255, 200
		);

		break;
	}

	case MODE_FACEPOSER:
	{
		CBaseFlex* pFlex = dynamic_cast<CBaseFlex*>(pEnt);
		if (!pFlex || pFlex->GetNumFlexControllers() <= 0)
		{
			CGabeServerFuncs::HudText(pPlayer,
				"Target has no usable facial controllers.", 255, 100, 100);
			break;
		}

		DebugFaceControllers(pFlex);
		int applied = ApplyFacePreset(pFlex, m_nFaceIndex, true);
		if (applied == 0)
		{
			CGabeServerFuncs::HudText(pPlayer,
				"Preset not supported by this model (mtool_face_debug 1).",
				255, 150, 100);
			// Do not bind future updates to an unsupported model.
			m_hFaceTarget = NULL;
			break;
		}

		m_hFaceTarget = pEnt;
		CGabeServerFuncs::HudText(pPlayer,
			UTIL_VarArgs("Face: %s (%d controllers)",
				g_FacePresets[m_nFaceIndex].name, applied), 150, 255, 150);
		break;
	}

	case MODE_CONSTRAINT:
	{
		// Every constraint uses the same two-click state machine.
		// Save the EXACT hit point, not an entity origin.
		trace_t tr;
		Vector start = pPlayer->EyePosition();
		Vector forward;
		pPlayer->EyeVectors(&forward);
		UTIL_TraceLine(start, start + forward * 2048.0f,
			MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

		if (tr.fraction == 1.0f || (tr.surface.flags & SURF_SKY))
		{
			CGabeServerFuncs::HudText(pPlayer, "Aim at an object or the world.", 255, 100, 100);
			break;
		}

		CBaseEntity* pHit = tr.m_pEnt;
		if (!pHit)
		{
			CGabeServerFuncs::HudText(pPlayer, "No target.", 255, 100, 100);
			break;
		}

		// First click must be a real movable physics object.
		if (!m_hConstraintTarget1)
		{
			if (pHit->IsWorld() || !pHit->VPhysicsGetObject())
			{
				CGabeServerFuncs::HudText(pPlayer, "First target needs VPhysics.", 255, 100, 100);
				break;
			}
			m_hConstraintTarget1 = pHit;
			m_hConstraintTarget2 = NULL;
			m_vecConstraintPos1 = tr.endpos;
			CGabeServerFuncs::HudText(pPlayer, "First point selected. Select second point.", 200, 255, 255);
			break;
		}

		CBaseEntity* pEnt1 = m_hConstraintTarget1.Get();
		CBaseEntity* pEnt2 = pHit;
		Vector pos1 = m_vecConstraintPos1;
		Vector pos2 = tr.endpos;

		// Always clear pending selection on a completed second click.
		m_hConstraintTarget1 = NULL;
		m_hConstraintTarget2 = NULL;

		if (!pEnt1 || !pEnt2 || pEnt1 == pEnt2)
		{
			CGabeServerFuncs::HudText(pPlayer, "Invalid second target.", 255, 100, 100);
			break;
		}

		IPhysicsObject* pPhys1 = pEnt1->VPhysicsGetObject();
		IPhysicsObject* pPhys2 = pEnt2->IsWorld() ? g_PhysWorldObject : pEnt2->VPhysicsGetObject();
		if (!pPhys1 || !pPhys2)
		{
			CGabeServerFuncs::HudText(pPlayer, "Second target needs VPhysics or world.", 255, 100, 100);
			break;
		}

		IPhysicsConstraint* pConstraint = NULL;
		const char* pszCreated = "Constraint";
		switch (m_nConstraintType)
		{
		case 0: // Ballsocket -- pivot is the second clicked location.
		{
			constraint_ballsocketparams_t params;
			params.Defaults();
			params.InitWithCurrentObjectState(pPhys1, pPhys2, pos2);
			pConstraint = physenv->CreateBallsocketConstraint(pPhys1, pPhys2, NULL, params);
			pszCreated = "Ballsocket";
			break;
		}
		case 1: // Weld -- preserve existing relative transform.
		{
			constraint_fixedparams_t params;
			params.Defaults();
			params.InitWithCurrentObjectState(pPhys1, pPhys2);
			pConstraint = physenv->CreateFixedConstraint(pPhys1, pPhys2, NULL, params);
			pszCreated = "Weld";
			break;
		}
		case 2: // Rope -- physics anchors are the selected impact points.
		{
			constraint_lengthparams_t params;
			params.Defaults();
			params.InitWorldspace(pPhys1, pPhys2, pos1, pos2, false);
			params.totalLength = (pos2 - pos1).Length() + 8.0f;
			params.minLength = 0.0f;
			pConstraint = physenv->CreateLengthConstraint(pPhys1, pPhys2, NULL, params);
			pszCreated = "Rope";
			break;
		}
		case 3: // Slider
		{
			Vector axis = pos2 - pos1;
			if (VectorNormalize(axis) < 0.001f)
				pPlayer->EyeVectors(&axis);
			pConstraint = CGabeServerFuncs::Gabe_CreateSlider(pEnt1, pEnt2, axis, -128.0f, 128.0f);
			pszCreated = "Slider";
			break;
		}
		case 4: // Hinge -- second point is the hinge pivot.
		{
			Vector axis = tr.plane.normal;
			if (VectorNormalize(axis) < 0.001f)
				axis.Init(0, 0, 1);
			pConstraint = CGabeServerFuncs::Gabe_CreateHinge(pEnt1, pEnt2, pos2, axis);
			pszCreated = "Hinge";
			break;
		}
		}

		if (!pConstraint)
		{
			CGabeServerFuncs::HudText(pPlayer, "Failed to create constraint.", 255, 100, 100);
			break;
		}

		CBaseEntity* pConstraintOwner = GabeUndo_OwnConstraint(pConstraint);
		if (!pConstraintOwner) break;
		m_Constraints.AddToTail(EHANDLE(pConstraintOwner));
		GabeUndo_AddEntity(pPlayer, pConstraintOwner, pszCreated);
		pPhys1->Wake();
		pPhys2->Wake();

		if (m_nConstraintType == 2)
		{
			// Visual-only anchors are attached at the actual hit points.
			// They follow prop movement; an unparented endpoint stays on the world.
			CBaseEntity* pAnchor1 = CreateEntityByName("info_target");
			CBaseEntity* pAnchor2 = CreateEntityByName("info_target");
			if (pAnchor1 && pAnchor2)
			{
				pAnchor1->SetAbsOrigin(pos1);
				pAnchor2->SetAbsOrigin(pos2);
				DispatchSpawn(pAnchor1);
				DispatchSpawn(pAnchor2);
				pAnchor1->AddEffects(EF_NODRAW);
				pAnchor2->AddEffects(EF_NODRAW);
				pAnchor1->SetParent(pEnt1);
				if (!pEnt2->IsWorld()) pAnchor2->SetParent(pEnt2);
				pAnchor1->SetAbsOrigin(pos1);
				pAnchor2->SetAbsOrigin(pos2);

				CRopeKeyframe* pRope = CRopeKeyframe::Create(
					pAnchor1, pAnchor2, 0, 0, 2, "cable/cable.vmt", 10);
				if (pRope)
				{
					pRope->m_Width = 2.0f;
					pRope->m_Slack = 8.0f;
					pRope->m_nSegments = 12;
					GabeUndo_AddConstraintVisual(pConstraintOwner, pRope);
				}
				GabeUndo_AddConstraintVisual(pConstraintOwner, pAnchor1);
				GabeUndo_AddConstraintVisual(pConstraintOwner, pAnchor2);
			}
			else
			{
				if (pAnchor1) UTIL_Remove(pAnchor1);
				if (pAnchor2) UTIL_Remove(pAnchor2);
			}
		}

		CGabeServerFuncs::HudText(pPlayer,
			UTIL_VarArgs("%s constraint created.", pszCreated), 100, 255, 100);
		break;
	}
	case MODE_IGNITE:
	{
		if (pEnt->IsWorld())
		{
			CGabeServerFuncs::HudText(pPlayer, "Cannot ignite the world.", 255, 100, 100);
			break;
		}

		// Check if entity already has a flame attached
		CEntityFlame* pFlame = dynamic_cast<CEntityFlame*>(
			gEntList.FindEntityByClassname(NULL, "entityflame")
			);

		bool bAlreadyOnFire = false;

		while (pFlame)
		{
			if (pFlame->GetMoveParent() == pEnt)
			{
				bAlreadyOnFire = true;
				break;
			}
			pFlame = dynamic_cast<CEntityFlame*>(
				gEntList.FindEntityByClassname(pFlame, "entityflame")
				);
		}

		if (bAlreadyOnFire)
		{
			// Extinguish = remove flame entity
			UTIL_Remove(pFlame);
			CGabeServerFuncs::HudText(pPlayer, "Fire extinguished.", 150, 200, 255);
		}
		else
		{
			// Ignite by attaching an entityflame
			CEntityFlame* pNewFlame = CEntityFlame::Create(pEnt);
			if (pNewFlame)
			{
				GabeUndo_AddEntity(pPlayer, pNewFlame, "Fire");
				pNewFlame->SetLifetime(10.0f); // seconds
				CGabeServerFuncs::HudText(pPlayer, "Entity ignited.", 255, 150, 50);
			}
		}

		break;
	}
	case MODE_THRUSTER:
	{
		if (!pEnt || pEnt->IsWorld())
			break;

		IPhysicsObject* pPhysics = pEnt->VPhysicsGetObject();

		if (!pPhysics)
		{
			CGabeServerFuncs::HudText(
				pPlayer,
				"Target has no physics.",
				255, 100, 100
			);
			break;
		}

		trace_t tr;

		Vector vecStart = pPlayer->EyePosition();
		Vector vecForward;
		pPlayer->EyeVectors(&vecForward);

		UTIL_TraceLine(
			vecStart,
			vecStart + vecForward * 2048.0f,
			MASK_SOLID,
			pPlayer,
			COLLISION_GROUP_NONE,
			&tr
		);

		CToolThruster* pThruster =
			static_cast<CToolThruster*>(
				CreateEntityByName("tool_thruster")
				);

		if (!pThruster)
			break;

		if (DispatchSpawn(pThruster) < 0) break;
		// Push in the direction the player is aiming.
		pThruster->SetupThruster(
			pEnt,
			tr.endpos,
			vecForward,
			m_nThrusterPower
		);



		pThruster->Activate();
		GabeUndo_AddEntity(pPlayer, pThruster, "Thruster");

		CGabeServerFuncs::HudText(
			pPlayer,
			"Thruster attached!",
			100, 255, 100
		);

		break;
	}
	case MODE_DUPLICATE:
	{
		if (!m_DupeData.valid)
		{
			CGabeServerFuncs::HudText(pPlayer, "No copied object.", 255, 150, 100);
			break;
		}

		trace_t tr;
		Vector start = pPlayer->EyePosition();
		Vector forward;
		pPlayer->EyeVectors(&forward);

		Vector end = start + forward * 4096.0f;

		UTIL_TraceLine(
			start,
			end,
			MASK_SOLID,
			pPlayer,
			COLLISION_GROUP_NONE,
			&tr
		);

		if (tr.fraction == 1.0f)
		{
			CGabeServerFuncs::HudText(pPlayer, "Aim at a surface to place.", 255, 100, 100);
			break;
		}

		Vector spawnPos = tr.endpos + tr.plane.normal * 8.0f;

		// =====================================
		// RAGDOLL
		// =====================================
		if (m_DupeData.isRagdoll)
		{
			CRagdollProp* pNew =
				dynamic_cast<CRagdollProp*>(
					CreateEntityByName("prop_ragdoll")
					);

			if (!pNew)
			{
				CGabeServerFuncs::HudText(pPlayer, "Ragdoll spawn failed.", 255, 100, 100);
				break;
			}

			pNew->SetModel(STRING(m_DupeData.modelName));
			pNew->SetAbsOrigin(spawnPos);
			pNew->SetAbsAngles(m_DupeData.angles);

			if (DispatchSpawn(pNew) < 0) break;
			pNew->Activate();
			GabeUndo_AddEntity(pPlayer, pNew, "Ragdoll");

			ragdoll_t* pData = pNew->GetRagdoll();

			if (pData && pData->listCount > 0)
			{
				int count =
					MIN(m_DupeData.boneCount,
						pData->listCount);

				for (int i = 0; i < count; i++)
				{
					IPhysicsObject* pBonePhys =
						pData->list[i].pObject;

					if (pBonePhys)
					{
						Vector worldPos =
							spawnPos +
							m_DupeData.bones[i].localPos;

						pBonePhys->SetPosition(
							worldPos,
							m_DupeData.bones[i].ang,
							true
						);

						pBonePhys->Wake();
					}
				}
			}

			break;
		}

		// =====================================
		// NORMAL ENTITY
		// =====================================
		CBaseEntity* pNew =
			CreateEntityByName(
				STRING(m_DupeData.className)
			);

		if (!pNew)
		{
			CGabeServerFuncs::HudText(pPlayer, "Spawn failed.", 255, 100, 100);
			break;
		}

		pNew->SetAbsOrigin(spawnPos);
		pNew->SetAbsAngles(m_DupeData.angles);

		if (m_DupeData.modelName != NULL_STRING)
			pNew->SetModel(STRING(m_DupeData.modelName));

		pNew->SetRenderColor(
			m_DupeData.color.r,
			m_DupeData.color.g,
			m_DupeData.color.b
		);

		if (DispatchSpawn(pNew) < 0) break;
		pNew->Activate();
		GabeUndo_AddEntity(pPlayer, pNew, "Duplication");

		IPhysicsObject* pPhys = pNew->VPhysicsGetObject();
		if (pPhys && m_DupeData.mass > 0.0f)
		{
			pPhys->SetMass(m_DupeData.mass);
			pPhys->Wake();
		}

		break;
	}

	case MODE_POINTMESSAGE:
	{
		trace_t tr;
		Vector start = pPlayer->EyePosition();
		Vector forward;
		pPlayer->EyeVectors(&forward);

		Vector end = start + forward * 4096.0f;

		UTIL_TraceLine(
			start,
			end,
			MASK_SOLID,
			pPlayer,
			COLLISION_GROUP_NONE,
			&tr
		);

		if (tr.fraction == 1.0f)
		{
			Msg("Multitool: aim at a surface to place point_message.\n");
			break;
		}

		Vector spawnPos = tr.endpos + tr.plane.normal * 4.0f;

		CBaseEntity* pMsg = CreateEntityByName("point_message");
		if (!pMsg)
		{
			Msg("Multitool: failed to create point_message.\n");
			break;
		}

		pMsg->SetAbsOrigin(spawnPos);

		// Apply keyvalues BEFORE spawn
		pMsg->KeyValue("targetname", "gabeplus_mtool_pmessage");
		pMsg->KeyValue("message", gabeplus_multitool_pmessagetext.GetString());
		pMsg->KeyValue("radius", "512");

		if (DispatchSpawn(pMsg) < 0) break;
		pMsg->Activate();
		GabeUndo_AddEntity(pPlayer, pMsg, "Point Message");

		Msg(
			"Multitool: point_message created (text=\"%s\", radius=512).\n",
			gabeplus_multitool_pmessagetext.GetString()
		);

		break;
	}
	case MODE_LIGHT_WATERMELON:
	{
		ThrowLightWatermelon();
		CGabeServerFuncs::HudText(pPlayer, "Light watermelon thrown.", 200, 255, 200);
		break;
	}

	case MODE_BALLOON:
	{
		if (pEnt->IsWorld())
			break;

		CreateBalloon(pEnt);

		break;
	}

	}
}

void CWeaponMultitool::ShowDistance(CBaseEntity* pEnt)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	float flDist = (pEnt->GetAbsOrigin() - pPlayer->EyePosition()).Length();
	CGabeServerFuncs::HudText(pPlayer, UTIL_VarArgs("Distance: %.1f units", flDist), 200, 255, 200);
}

void CWeaponMultitool::ItemPostFrame()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	MultitoolMode_t newMode = StringToMode(mtool_mode.GetString());

	if (newMode != m_nMultitoolMode)
	{
		m_hConstraintTarget1 = NULL;
		m_hConstraintTarget2 = NULL;
		m_nMultitoolMode = newMode;
		SendHudUpdate();
	}

	if ((pPlayer->m_nButtons & IN_RELOAD) && (pPlayer->m_afButtonPressed & IN_RELOAD))
	{
		if (m_nMultitoolMode == MODE_COLOR || m_nMultitoolMode == MODE_DECAL || m_nMultitoolMode == MODE_THRUSTER)
		{
			if (m_nMultitoolMode == MODE_COLOR)
			{
				m_nColorIndex = (m_nColorIndex + 1) % g_ColorPresets_COUNT;
				const char* name = g_ColorPresets[m_nColorIndex].name;
				CGabeServerFuncs::HudText(pPlayer, UTIL_VarArgs("Color Mode: %s", name), 255, 255, 255);
				SendHudUpdate();
			}
			else if (m_nMultitoolMode == MODE_DECAL)
			{
				m_nDecalIndex = (m_nDecalIndex + 1) % DECAL_COUNT;
				const char* name = g_DecalNames[m_nDecalIndex];
				CGabeServerFuncs::HudText(pPlayer, UTIL_VarArgs("Decal: %s", name), 255, 255, 255);
				SendHudUpdate();
			}

			// -----------------------------------------
			// Thruster power selection (R)
			// -----------------------------------------
			else if (m_nMultitoolMode == MODE_THRUSTER)
			{
				// Red -> Green -> Blue -> Red
				m_nThrusterPower = (m_nThrusterPower + 1) % 3;

				const char* pszPowerNames[] =
				{
					"LOW (RED)",
					"MEDIUM (GREEN)",
					"HIGH (BLUE)"
				};

				const int colors[3][3] =
				{
					{ 255, 0, 0 },
					{ 0, 255, 0 },
					{ 0, 100, 255 }
				};

				CGabeServerFuncs::HudText(
					pPlayer,
					UTIL_VarArgs(
						"Thruster Power: %s",
						pszPowerNames[m_nThrusterPower]
					),
					colors[m_nThrusterPower][0],
					colors[m_nThrusterPower][1],
					colors[m_nThrusterPower][2]
				);

				SendHudUpdate();
			}
		}
	}

	if (m_nMultitoolMode == MODE_MATERIAL)
	{
		if (pPlayer->m_afButtonPressed & IN_RELOAD)
		{
			m_nMaterialIndex = (m_nMaterialIndex + 1) % RENDERFX_COUNT;

			CGabeServerFuncs::HudText(
				pPlayer,
				UTIL_VarArgs(
					"RenderFX: %s",
					g_RenderFxModes[m_nMaterialIndex].name
				),
				255, 255, 255
			);

			SendHudUpdate();
		}
	}

	if (m_nMultitoolMode == MODE_BALLOON)
	{
		if (pPlayer->m_afButtonPressed & IN_RELOAD)
		{
			m_nBalloonLiftIndex++;

			if (m_nBalloonLiftIndex >= 10)
				m_nBalloonLiftIndex = 0;

			CGabeServerFuncs::HudText(
				pPlayer,
				UTIL_VarArgs(
					"Balloon lift: %.0f",
					m_BalloonLiftForces[m_nBalloonLiftIndex]
				),
				150, 255, 150
			);

			SendHudUpdate();
		}
	}

	if (m_nMultitoolMode == MODE_FACEPOSER)
	{
		if (pPlayer->m_afButtonPressed & IN_RELOAD)
		{
			m_nFaceIndex = (m_nFaceIndex + 1) % FACE_COUNT;

			CBaseFlex* pSelectedFlex = dynamic_cast<CBaseFlex*>(m_hFaceTarget.Get());
			if (pSelectedFlex)
				ApplyFacePreset(pSelectedFlex, m_nFaceIndex, true);

			CGabeServerFuncs::HudText(
				pPlayer,
				UTIL_VarArgs("Face: %s", g_FacePresets[m_nFaceIndex].name),
				255, 255, 255
			);

			SendHudUpdate();
		}
	}


	if (m_nMultitoolMode == MODE_CONSTRAINT)
	{
		if ((pPlayer->m_nButtons & IN_RELOAD) && (pPlayer->m_afButtonPressed & IN_RELOAD))
		{
			m_hConstraintTarget1 = NULL;
			m_hConstraintTarget2 = NULL;
			m_nConstraintType = (m_nConstraintType + 1) % g_szConstraintTypes_COUNT;
			CGabeServerFuncs::HudText(pPlayer, UTIL_VarArgs("Constraint Type: %s", g_szConstraintTypes[m_nConstraintType]), 255, 255, 255);

			SendHudUpdate();
		}

		if ((pPlayer->m_nButtons & IN_USE) &&
			(pPlayer->m_afButtonPressed & IN_USE))
		{
			int removed = 0;

			// Destroy physics constraints
			for (int i = 0; i < m_Constraints.Count(); i++)
			{
				if (m_Constraints[i])
				{
					if (!m_Constraints[i]->IsMarkedForDeletion())
						UTIL_Remove(m_Constraints[i].Get());
					removed++;
				}
			}

			m_Constraints.RemoveAll();
			m_hConstraintTarget1 = NULL;
			m_hConstraintTarget2 = NULL;


			CGabeServerFuncs::HudText(pPlayer,
				UTIL_VarArgs("Removed %d constraints.", removed),
				255, 100, 100);
		}
	}

	if (m_nMultitoolMode == MODE_LIGHT_WATERMELON)
	{
		if ((pPlayer->m_afButtonPressed & IN_RELOAD))
		{
			m_nLightWatermelonColor =
				(m_nLightWatermelonColor + 1) % g_ColorPresets_COUNT;

			CGabeServerFuncs::HudText(
				pPlayer,
				UTIL_VarArgs(
					"Watermelon Light Color: %s",
					g_ColorPresets[m_nLightWatermelonColor].name
				),
				255, 255, 255
			);

			SendHudUpdate();
		}
	}

	if (m_nMultitoolMode == MODE_POINTMESSAGE)
	{
		if ((pPlayer->m_nButtons & IN_RELOAD) &&
			(pPlayer->m_afButtonPressed & IN_RELOAD))
		{
			int removed = 0;

			CBaseEntity* pEnt = NULL;
			while ((pEnt = gEntList.NextEnt(pEnt)) != NULL)
			{
				if (Q_stricmp(
					STRING(pEnt->GetEntityName()),
					"gabeplus_mtool_pmessage"
				) == 0)
				{
					UTIL_Remove(pEnt);
					removed++;
				}
			}

			Msg(
				"Multitool: removed %d point_message entities named \"gabeplus_mtool_pmessage\".\n",
				removed
			);
		}
	}

	if (m_nMultitoolMode == MODE_FACEPOSER && m_hFaceTarget)
	{
		CBaseFlex* pFlex = dynamic_cast<CBaseFlex*>(m_hFaceTarget.Get());
		if (pFlex)
		{
			// Reapply only the selected channels for scene/animation updates.
			// Do NOT zero every flex controller every frame: that wipes
			// speech/blinking and causes visible flicker.
			if (m_nFaceIndex != FACE_COUNT - 1)
				ApplyFacePreset(pFlex, m_nFaceIndex, false);
		}
		else
		{
			m_hFaceTarget = NULL;
		}
	}

	if (m_nMultitoolMode == MODE_DUPLICATE && m_DupeData.valid)
	{
		trace_t tr;

		Vector start = pPlayer->EyePosition();
		Vector forward;
		pPlayer->EyeVectors(&forward);

		Vector end = start + forward * 4096.0f;

		UTIL_TraceLine(
			start,
			end,
			MASK_SOLID,
			pPlayer,
			COLLISION_GROUP_NONE,
			&tr
		);

		if (tr.fraction < 1.0f)
		{
			Vector renderOrigin = tr.endpos + tr.plane.normal * 2.0f;

			// Draw bounding box preview
			NDebugOverlay::BoxAngles(
				renderOrigin,
				m_DupeData.mins,
				m_DupeData.maxs,
				m_DupeData.angles,
				0, 255, 0,   // Green
				40,          // Alpha
				0.0f         // Duration 0 = draw for one frame (we redraw every frame)
			);
		}
	}

	BaseClass::ItemPostFrame();
}