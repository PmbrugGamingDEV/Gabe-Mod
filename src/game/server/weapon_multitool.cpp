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
#include "GABE.H"
#include "beam_shared.h"
#include "in_buttons.h"
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

// memdbgon must be the last include file in a .cpp file!!!
#include "memdbgon.h"

using GabeMod::HudText;

extern IEffects* g_pEffects;

class CWeaponMultitool;

#define MAX_DUPE_RAGDOLL_BONES 64

ConVar mtool_mode(
	"mtool_mode",
	"remove",          // default
	FCVAR_ARCHIVE,
	"Multitool mode selector"
);

const struct ColorPreset {
	int r, g, b;
	const char* name;
} g_ColorPresets[] = {
	{255, 0, 0, "Red"},
	{255, 165, 0, "Orange"},
	{255, 255, 0, "Yellow"},
	{0, 255, 0, "Green"},
	{0, 0, 255, "Blue"},
	{75, 0, 130, "Indigo"},
	{128, 0, 128, "Purple"},
	{238, 130, 238, "Violet"},
	{255, 192, 203, "Pink"},
	{0, 0, 0, "Black"},
	{255, 255, 255, "White"},
	{0, 0, 0, "Rainbow"}
};
const int COLOR_COUNT = sizeof(g_ColorPresets) / sizeof(g_ColorPresets[0]);

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

ConVar gabeplus_multitool_pmessagetext("gabe+_multitool_pmessagetext", "Customize me with gabe+_multitool_pmessagetext!", FCVAR_ARCHIVE);

class CL_Watermelon : public CBaseAnimating
{
public:
	DECLARE_CLASS(CL_Watermelon, CBaseAnimating);
	DECLARE_DATADESC();

	void Spawn() override;
	void Precache() override;
	void Touch(CBaseEntity* pOther) override;
	void Think() override;
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

	int m_nMultitoolMode;
	int m_nColorIndex;
	int m_nDecalIndex;
	EHANDLE m_hConstraintTarget1;
	EHANDLE m_hConstraintTarget2;
	int m_nConstraintType; // 0: BallSocket, 1: Weld, 2: Axis
	Vector m_vecConstraintPos1;

	int m_nLightWatermelonColor;

	int m_nMaterialIndex;

	CUtlVector<IPhysicsConstraint*> m_Constraints;
	CUtlVector<EHANDLE> m_CreatedRopes;


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

	return MODE_REMOVE;
}

const char* g_szModeNames[] =
{
	"Remove",
	"Distance (Source Engine Units)",
	"Color",
	"Constraints",
	"Ignite",
	"Duplicate",
	"Explode",
	"Point Message",
	"Light Watermelon",
	"Decal",
	"Render FX",
	"Faceposer (NPCs)",
};

const char* g_szConstraintTypes[] = {
	"BallSocket",
	"Weld",
	"Rope"
};
const int CONSTRAINT_TYPE_COUNT = sizeof(g_szConstraintTypes) / sizeof(g_szConstraintTypes[0]);

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

	if (m_nMultitoolMode != MODE_DUPLICATE)
		return;

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
		HudText(pPlayer, "Nothing to copy.", 255, 100, 100);
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
	HudText(
		pPlayer,
		UTIL_VarArgs(
			"Copied: %s",
			STRING(m_DupeData.className)
		),
		150, 255, 150
	);

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
	DispatchSpawn(pEnt);

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

struct FlexPair
{
	const char* flex;
	float weight; // 0..1 (or higher if you want, but keep sane)
};

struct FacePreset
{
	const char* name;
	const FlexPair* pairs;
	int pairCount;
};

// Helper macro
#define FACE_PRESET(_name, _arr) { _name, _arr, (int)(sizeof(_arr)/sizeof(_arr[0])) }

// --- Preset definitions ---
// NOTE: Flex names vary per model. These are common-ish names.
// You can add more aliases later.

static FlexPair g_Face_Smile[] =
{
	{ "smile", 1.0f },
	{ "smile_open", 0.4f },
	{ "happy", 0.6f },
	{ "eyes_smile", 0.6f }
};

static FlexPair g_Face_Sad[] =
{
	{ "sad", 1.0f },
	{ "frown", 0.9f },
	{ "brow_down", 0.6f },
	{ "mouth_down", 0.8f }
};

static FlexPair g_Face_Angry[] =
{
	{ "angry", 1.0f },
	{ "brow_lowerer", 1.0f },
	{ "brow_down", 0.8f },
	{ "sneer", 0.5f }
};

static FlexPair g_Face_Scared[] =
{
	{ "fear", 1.0f },
	{ "scared", 1.0f },
	{ "brow_raiser", 0.8f },
	{ "eyes_wide", 1.0f },
	{ "mouth_open", 0.6f }
};

static FlexPair g_Face_Surprised[] =
{
	{ "surprised", 1.0f },
	{ "brow_raiser", 1.0f },
	{ "eyes_wide", 1.0f },
	{ "jaw_drop", 0.7f }
};

static FlexPair g_Face_Neutral[] =
{
	{ "neutral", 0.0f }
};

static FacePreset g_FacePresets[] =
{
	FACE_PRESET("Smile",     g_Face_Smile),
	FACE_PRESET("Sad",       g_Face_Sad),
	FACE_PRESET("Angry",     g_Face_Angry),
	FACE_PRESET("Scared",    g_Face_Scared),
	FACE_PRESET("Surprised", g_Face_Surprised),
	FACE_PRESET("Neutral",   g_Face_Neutral),
};

const int FACE_COUNT = sizeof(g_FacePresets) / sizeof(g_FacePresets[0]);

#undef FACE_PRESET

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
			HudText(pPlayer, "No players.", 255, 100, 100);
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
		if (m_nColorIndex == COLOR_COUNT - 1) // Rainbow
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
				pThink->Spawn();
				DispatchSpawn(pThink);
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

		HudText(
			pPlayer,
			UTIL_VarArgs("RenderFX: %s", entry.name),
			200, 255, 200
		);

		break;
	}

	case MODE_FACEPOSER:
	{
		if (!pEnt->IsNPC())
		{
			HudText(pPlayer, "Not an NPC.", 255, 100, 100);
			break;
		}

		CBaseFlex* pFlex = dynamic_cast<CBaseFlex*>(pEnt);
		if (!pFlex)
		{
			HudText(pPlayer, "NPC has no facial flexes.", 255, 100, 100);
			break;
		}

		// Store target so we can override every frame
		m_hFaceTarget = pEnt;

		// Clear existing flex values
		int flexCount = pFlex->GetNumFlexControllers();

		for (LocalFlexController_t i; i < flexCount; i++)
		{
			const char* name = pFlex->GetFlexControllerName(i);
			if (name && name[0])
			{
				pFlex->SetFlexWeight(const_cast<char*>(name), 0.0f);
			}
		}

		// Apply preset immediately
		FacePreset& preset = g_FacePresets[m_nFaceIndex];

		for (int i = 0; i < preset.pairCount; i++)
		{
			pFlex->SetFlexWeight(
				const_cast<char*>(preset.pairs[i].flex),
				preset.pairs[i].weight
			);
		}

		HudText(
			pPlayer,
			UTIL_VarArgs("Applied: %s", preset.name),
			150, 255, 150
		);

		break;
	}
	case MODE_CONSTRAINT:
	{
		if (!m_hConstraintTarget1)
		{
			m_hConstraintTarget1 = pEnt;
			m_hConstraintTarget2 = NULL; // Reset second just in case
			HudText(pPlayer, "First point selected.", 200, 255, 255);
		}
		else if (m_hConstraintTarget1 == pEnt)
		{
			HudText(pPlayer, "Cannot constrain an entity to itself.", 255, 100, 100);
			m_hConstraintTarget1 = NULL;
		}
		else
		{
			m_hConstraintTarget2 = pEnt;

			CBaseEntity* pEnt1 = m_hConstraintTarget1;
			CBaseEntity* pEnt2 = m_hConstraintTarget2;

			Vector origin1 = pEnt1->WorldSpaceCenter();
			Vector origin2 = pEnt2->WorldSpaceCenter();
			Vector anchor = (origin1 + origin2) * 0.5f;

			IPhysicsObject* pPhys1 = pEnt1->VPhysicsGetObject();
			IPhysicsObject* pPhys2 = pEnt2->VPhysicsGetObject();

			if (!pPhys1 || !pPhys2)
			{
				HudText(pPlayer, "Both objects must have physics!", 255, 100, 100);
				m_hConstraintTarget1 = NULL;
				m_hConstraintTarget2 = NULL;
				break;
			}

			switch (m_nConstraintType)
			{
			case 0: // BallSocket
			{
				constraint_ballsocketparams_t ballsocket;
				ballsocket.Defaults();
				ballsocket.InitWithCurrentObjectState(pPhys1, pPhys2, anchor);
				IPhysicsConstraint* pConstraint = physenv->CreateBallsocketConstraint(pPhys1, pPhys2, NULL, ballsocket);
				if (pConstraint)
				{
					m_Constraints.AddToTail(pConstraint);
					HudText(pPlayer, "BallSocket constraint created.", 100, 255, 100);
				}

				break;
			}
			case 1: // Weld
			{
				constraint_fixedparams_t weld;
				weld.Defaults();
				weld.InitWithCurrentObjectState(pPhys1, pPhys2);
				IPhysicsConstraint* pConstraint = physenv->CreateFixedConstraint(pPhys1, pPhys2, NULL, weld);
				if (pConstraint)
				{
					m_Constraints.AddToTail(pConstraint);
					HudText(pPlayer, "Weld constraint created.", 100, 255, 100);
				}
				break;
			}
			case 2: // Rope
			{
				// ----------------------------------------
				// FIRST CLICK
				// ----------------------------------------
				if (!m_hConstraintTarget1)
				{
					m_hConstraintTarget1 = pEnt; // store first entity (if any)

					// Store vector position too (trace)
					trace_t tr;
					Vector start = pPlayer->EyePosition();
					Vector forward;
					pPlayer->EyeVectors(&forward);

					UTIL_TraceLine(start, start + forward * 2048,
						MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

					m_vecConstraintPos1 = tr.endpos;

					HudText(pPlayer, "First rope point set.", 200, 255, 255);
					break;
				}

				// ----------------------------------------
				// SECOND CLICK
				// ----------------------------------------
				CBaseEntity* pEnt1 = m_hConstraintTarget1;
				CBaseEntity* pEnt2 = pEnt;

				trace_t tr;
				Vector start = pPlayer->EyePosition();
				Vector forward;
				pPlayer->EyeVectors(&forward);

				UTIL_TraceLine(start, start + forward * 2048,
					MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

				Vector vec2 = tr.endpos;
				Vector vec1 = m_vecConstraintPos1;

				// ----------------------------------------
				// 🟢 VECTOR MODE (no entities)
				// ----------------------------------------
				if (!pEnt1 && !pEnt2)
				{
					CBaseEntity* pStart = CreateEntityByName("move_rope");
					CBaseEntity* pEnd = CreateEntityByName("keyframe_rope");

					if (pStart && pEnd)
					{
						pStart->SetAbsOrigin(vec1);
						pEnd->SetAbsOrigin(vec2);

						char name[64];
						Q_snprintf(name, sizeof(name), "rope_start_%d", gpGlobals->tickcount);

						pStart->SetName(MAKE_STRING(name));
						pEnd->KeyValue("NextKey", name);

						pStart->KeyValue("Slack", "60");
						pStart->KeyValue("Width", "2");

						DispatchSpawn(pStart);
						DispatchSpawn(pEnd);

						HudText(pPlayer, "Vector rope created!", 100, 255, 100);
					}
				}
				else
				{
					// ----------------------------------------
					// 🔵 ENTITY MODE (physics rope)
					// ----------------------------------------
					if (!pEnt1 || !pEnt2 || pEnt1 == pEnt2)
					{
						HudText(pPlayer, "Invalid rope targets!", 255, 100, 100);
						m_hConstraintTarget1 = NULL;
						break;
					}

					IPhysicsObject* pPhys1 = pEnt1->VPhysicsGetObject();
					IPhysicsObject* pPhys2 = pEnt2->VPhysicsGetObject();

					if (!pPhys1 || !pPhys2)
					{
						HudText(pPlayer, "Both objects need physics!", 255, 100, 100);
						m_hConstraintTarget1 = NULL;
						break;
					}

					Vector pos1 = pEnt1->WorldSpaceCenter();
					Vector pos2 = pEnt2->WorldSpaceCenter();

					// ---------------------------
					// VISUAL ROPE
					// ---------------------------
					PrecacheModel("cable/cable.vmt");

					CRopeKeyframe* pRope = CRopeKeyframe::Create(
						pEnt1,
						pEnt2,
						0,
						0,
						2,
						"cable/cable.vmt",
						10
					);

					if (pRope)
					{
						pRope->SetParent(pEnt1, 1);
						pRope->m_Width = 2.0f;
						pRope->m_Slack = 60.0f;
						pRope->m_nSegments = 12;

						m_CreatedRopes.AddToTail(pRope);
					}

					// ---------------------------
					// PHYSICS ROPE
					// ---------------------------
					constraint_lengthparams_t rope;
					rope.Defaults();

					rope.InitWorldspace(pPhys1, pPhys2, pos1, pos2, false);

					rope.totalLength *= 1.2f; // slack
					rope.minLength = 0.0f;

					rope.constraint.strength = 0.8f;
					rope.constraint.forceLimit = 0;
					rope.constraint.torqueLimit = 0;

					IPhysicsConstraint* pConstraint =
						physenv->CreateLengthConstraint(pPhys1, pPhys2, NULL, rope);

					if (pConstraint)
					{
						m_Constraints.AddToTail(pConstraint);
						HudText(pPlayer, "Entity rope created!", 100, 255, 100);
					}
				}

				// ----------------------------------------
				// RESET
				// ----------------------------------------
				m_hConstraintTarget1 = NULL;

				break;
			}
			}

			m_hConstraintTarget1 = NULL;
			m_hConstraintTarget2 = NULL;
		}
		break;
	}
	case MODE_IGNITE:
	{
		if (pEnt->IsWorld())
		{
			HudText(pPlayer, "Cannot ignite the world.", 255, 100, 100);
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
			HudText(pPlayer, "Fire extinguished.", 150, 200, 255);
		}
		else
		{
			// Ignite by attaching an entityflame
			CEntityFlame* pNewFlame = CEntityFlame::Create(pEnt);
			if (pNewFlame)
			{
				pNewFlame->SetLifetime(10.0f); // seconds
				HudText(pPlayer, "Entity ignited.", 255, 150, 50);
			}
		}

		break;
	}
	case MODE_DUPLICATE:
	{
		if (!m_DupeData.valid)
		{
			HudText(pPlayer, "No copied object.", 255, 150, 100);
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
			HudText(pPlayer, "Aim at a surface to place.", 255, 100, 100);
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
				HudText(pPlayer, "Ragdoll spawn failed.", 255, 100, 100);
				break;
			}

			pNew->SetModel(STRING(m_DupeData.modelName));
			pNew->SetAbsOrigin(spawnPos);
			pNew->SetAbsAngles(m_DupeData.angles);

			DispatchSpawn(pNew);
			pNew->Activate();
			pNew->CreateVPhysics();

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
			HudText(pPlayer, "Spawn failed.", 255, 100, 100);
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

		DispatchSpawn(pNew);
		pNew->Activate();

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

		DispatchSpawn(pMsg);
		pMsg->Activate();

		Msg(
			"Multitool: point_message created (text=\"%s\", radius=512).\n",
			gabeplus_multitool_pmessagetext.GetString()
		);

		break;
	}
	case MODE_LIGHT_WATERMELON:
	{
		ThrowLightWatermelon();
		HudText(pPlayer, "Light watermelon thrown.", 200, 255, 200);
		break;
	}

	}
}

void CWeaponMultitool::ShowDistance(CBaseEntity* pEnt)
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	float flDist = (pEnt->GetAbsOrigin() - pPlayer->EyePosition()).Length();
	HudText(pPlayer, UTIL_VarArgs("Distance: %.1f units", flDist), 200, 255, 200);
}

void CWeaponMultitool::ItemPostFrame()
{
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
	if (!pPlayer) return;

	MultitoolMode_t newMode = StringToMode(mtool_mode.GetString());

	if (newMode != m_nMultitoolMode)
	{
		m_nMultitoolMode = newMode;
		SendHudUpdate();
	}

	if ((pPlayer->m_nButtons & IN_RELOAD) && (pPlayer->m_afButtonPressed & IN_RELOAD))
	{
		if (m_nMultitoolMode == MODE_COLOR || m_nMultitoolMode == MODE_DECAL)
		{
			if (m_nMultitoolMode == MODE_COLOR)
			{
				m_nColorIndex = (m_nColorIndex + 1) % COLOR_COUNT;
				const char* name = g_ColorPresets[m_nColorIndex].name;
				HudText(pPlayer, UTIL_VarArgs("Color Mode: %s", name), 255, 255, 255);
				SendHudUpdate();
			}
			else if (m_nMultitoolMode == MODE_DECAL)
			{
				m_nDecalIndex = (m_nDecalIndex + 1) % DECAL_COUNT;
				const char* name = g_DecalNames[m_nDecalIndex];
				HudText(pPlayer, UTIL_VarArgs("Decal: %s", name), 255, 255, 255);
				SendHudUpdate();
			}
		}
	}

	if (m_nMultitoolMode == MODE_MATERIAL)
	{
		if (pPlayer->m_afButtonPressed & IN_RELOAD)
		{
			m_nMaterialIndex = (m_nMaterialIndex + 1) % RENDERFX_COUNT;

			HudText(
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

	if (m_nMultitoolMode == MODE_FACEPOSER)
	{
		if (pPlayer->m_afButtonPressed & IN_RELOAD)
		{
			m_nFaceIndex = (m_nFaceIndex + 1) % FACE_COUNT;

			HudText(
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
			m_nConstraintType = (m_nConstraintType + 1) % CONSTRAINT_TYPE_COUNT;
			HudText(pPlayer, UTIL_VarArgs("Constraint Type: %s", g_szConstraintTypes[m_nConstraintType]), 255, 255, 255);

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
					physenv->DestroyConstraint(m_Constraints[i]);
					removed++;
				}
			}

			m_Constraints.RemoveAll();

			// Remove only ropes created by this tool
			for (int i = 0; i < m_CreatedRopes.Count(); i++)
			{
				if (m_CreatedRopes[i])
					UTIL_Remove(m_CreatedRopes[i]);
			}

			m_CreatedRopes.RemoveAll();

			HudText(pPlayer,
				UTIL_VarArgs("Removed %d constraints.", removed),
				255, 100, 100);
		}
	}

	if (m_nMultitoolMode == MODE_LIGHT_WATERMELON)
	{
		if ((pPlayer->m_afButtonPressed & IN_RELOAD))
		{
			m_nLightWatermelonColor =
				(m_nLightWatermelonColor + 1) % COLOR_COUNT;

			HudText(
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
		CBaseEntity* pTarget = m_hFaceTarget.Get();
		if (!pTarget)
			return;

		CBaseFlex* pFlex = dynamic_cast<CBaseFlex*>(pTarget);
		if (!pFlex)
			return;

		// Reset all flex weights every frame
		LocalFlexController_t flexCount = pFlex->GetNumFlexControllers();

		for (LocalFlexController_t i; i < flexCount; ++i)
		{
			const char* name = pFlex->GetFlexControllerName(i);

			if (name && name[0])
			{
				pFlex->SetFlexWeight(const_cast<char*>(name), 0.0f);
			}
		}

		// Reapply preset
		FacePreset& preset = g_FacePresets[m_nFaceIndex];

		for (int i = 0; i < preset.pairCount; i++)
		{
			pFlex->SetFlexWeight(
				const_cast<char*>(preset.pairs[i].flex),
				preset.pairs[i].weight
			);
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