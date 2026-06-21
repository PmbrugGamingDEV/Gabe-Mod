#include "cbase.h"
#include "hl1_player.h"
#include "gamerules.h"
#include "trains.h"
#include "hl1_baseweapon_shared.h"
#include "vcollide_parse.h"
#include "in_buttons.h"
#include "igamemovement.h"
#include "ai_hull.h"
#include "hl2_shareddefs.h"
#include "info_camera_link.h"
#include "point_camera.h"
#include "ndebugoverlay.h"
#include "globals.h"
#include "ai_interactions.h"
#include "engine/IEngineSound.h"
#include "vphysics/player_controller.h"
#include "vphysics/constraints.h"
#include "predicted_viewmodel.h"
#include "physics_saverestore.h"
#include "gamestats.h"
#include "player_pickup.h"
#include "props.h"
#include "vphysics/friction.h"
#include "movevars_shared.h"
#include "te_effect_dispatch.h"
#include "soundent.h"

#define DMG_FREEZE		DMG_VEHICLE
#define DMG_SLOWFREEZE	DMG_DISSOLVE

#define PARALYZE_DURATION	2
#define PARALYZE_DAMAGE		1.0

#define NERVEGAS_DURATION	2
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION		5
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION	2
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION		2
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION	2
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION	2
#define SLOWFREEZE_DAMAGE	1.0

#define	FLASH_DRAIN_TIME	 1.2
#define	FLASH_CHARGE_TIME	 0.2

#define PLAYER_HOLD_LEVEL_EYES	-8

#define PLAYER_HOLD_DOWN_FEET	2

#define PLAYER_HOLD_UP_EYES		24

#define PLAYER_LOOK_PITCH_RANGE	30

#define PLAYER_REACH_DOWN_DISTANCE	24

#define MASS_SPEED_SCALE	60
#define MAX_MASS			40

extern ConVar player_showpredictedposition;
extern ConVar player_showpredictedposition_timestep;

ConVar sv_hl1_allowpickup("sv_hl1_allowpickup", "0");

extern ConVar hl2_normspeed;
extern ConVar player_throwforce;
extern ConVar physcannon_maxmass;

ConVar hl1_movement("hl1_movement", "0", FCVAR_REPLICATED, "Enable HL1 movement.");
ConVar hl1_move_sounds("hl1_move_sounds", "0", FCVAR_REPLICATED, "Enable HL1 movement sounds.");

LINK_ENTITY_TO_CLASS(player, CHL1_Player);
PRECACHE_REGISTER(player);

BEGIN_DATADESC(CHL1_Player)
DEFINE_FIELD(m_nControlClass, FIELD_INTEGER),
DEFINE_AUTO_ARRAY(m_vecMissPositions, FIELD_POSITION_VECTOR),
DEFINE_FIELD(m_nNumMissPositions, FIELD_INTEGER),

DEFINE_FIELD(m_flIdleTime, FIELD_TIME),
DEFINE_FIELD(m_flMoveTime, FIELD_TIME),
DEFINE_FIELD(m_flLastDamageTime, FIELD_TIME),
DEFINE_FIELD(m_flTargetFindTime, FIELD_TIME),
DEFINE_FIELD(m_bHasLongJump, FIELD_BOOLEAN),
DEFINE_FIELD(m_nFlashBattery, FIELD_INTEGER),
DEFINE_FIELD(m_flFlashLightTime, FIELD_TIME),

DEFINE_FIELD(m_flStartCharge, FIELD_FLOAT),
DEFINE_FIELD(m_flAmmoStartCharge, FIELD_FLOAT),
DEFINE_FIELD(m_flPlayAftershock, FIELD_FLOAT),
DEFINE_FIELD(m_flNextAmmoBurn, FIELD_FLOAT),

DEFINE_PHYSPTR(m_pPullConstraint),
DEFINE_FIELD(m_hPullObject, FIELD_EHANDLE),
DEFINE_FIELD(m_bIsPullingObject, FIELD_BOOLEAN),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST(CHL1_Player, DT_HL1Player)
SendPropInt(SENDINFO(m_bHasLongJump), 1, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_nFlashBattery), 8, SPROP_UNSIGNED),
SendPropBool(SENDINFO(m_bIsPullingObject)),

SendPropFloat(SENDINFO(m_flStartCharge)),
SendPropFloat(SENDINFO(m_flAmmoStartCharge)),
SendPropFloat(SENDINFO(m_flPlayAftershock)),
SendPropFloat(SENDINFO(m_flNextAmmoBurn))

END_SEND_TABLE()

CHL1_Player::CHL1_Player()
{
	m_nNumMissPositions = 0;
}

void CHL1_Player::Precache(void)
{
	BaseClass::Precache();

	PrecacheScriptSound("Player.FlashlightOn");
	PrecacheScriptSound("Player.FlashlightOff");
	PrecacheScriptSound("Player.UseTrain");
}

void CHL1_Player::PreThink(void)
{
	if (player_showpredictedposition.GetBool())
	{
		Vector	predPos;

		UTIL_PredictedPosition(this, player_showpredictedposition_timestep.GetFloat(), &predPos);

		NDebugOverlay::Box(predPos, NAI_Hull::Mins(GetHullType()), NAI_Hull::Maxs(GetHullType()), 0, 255, 0, 0, 0.01f);
		NDebugOverlay::Line(GetAbsOrigin(), predPos, 0, 255, 0, 0, 0.01f);
	}

	int buttonsChanged;
	buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	UpdatePullingObject();

	g_pGameRules->PlayerThink(this);

	if (g_fGameOver || IsPlayerLockedInPlace())
		return;

	ItemPreFrame();
	WaterMove();

	if (g_pGameRules && g_pGameRules->FAllowFlashlight())
		m_Local.m_iHideHUD &= ~HIDEHUD_FLASHLIGHT;
	else
		m_Local.m_iHideHUD |= HIDEHUD_FLASHLIGHT;

	UpdateClientData();

	CheckTimeBasedDamage();

	CheckSuitUpdate();

	if (m_lifeState >= LIFE_DYING)
	{
		PlayerDeathThink();
		return;
	}

	if (m_afPhysicsFlags & PFLAG_DIROVERRIDE)
		AddFlag(FL_ONTRAIN);
	else
		RemoveFlag(FL_ONTRAIN);

	if (m_afPhysicsFlags & PFLAG_DIROVERRIDE)
	{
		CBaseEntity* pTrain = GetGroundEntity();
		float vel;

		if (pTrain)
		{
			if (!(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE))
				pTrain = NULL;
		}

		if (!pTrain)
		{
			if (GetActiveWeapon() && (GetActiveWeapon()->ObjectCaps() & FCAP_DIRECTIONAL_USE))
			{
				m_iTrain = TRAIN_ACTIVE | TRAIN_NEW;

				if (m_nButtons & IN_FORWARD)
				{
					m_iTrain |= TRAIN_FAST;
				}
				else if (m_nButtons & IN_BACK)
				{
					m_iTrain |= TRAIN_BACK;
				}
				else
				{
					m_iTrain |= TRAIN_NEUTRAL;
				}
				return;
			}
			else
			{
				trace_t trainTrace;

				UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -38),
					MASK_PLAYERSOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trainTrace);

				if (trainTrace.fraction != 1.0 && trainTrace.m_pEnt)
					pTrain = trainTrace.m_pEnt;


				if (!pTrain || !(pTrain->ObjectCaps() & FCAP_DIRECTIONAL_USE) || !pTrain->OnControls(this))
				{
					m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
					m_iTrain = TRAIN_NEW | TRAIN_OFF;
					return;
				}
			}
		}
		else if (!(GetFlags() & FL_ONGROUND) || pTrain->HasSpawnFlags(SF_TRACKTRAIN_NOCONTROL) || (m_nButtons & (IN_MOVELEFT | IN_MOVERIGHT)))
		{
			m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
			return;
		}

		SetAbsVelocity(vec3_origin);
		vel = 0;
		if (m_afButtonPressed & IN_FORWARD)
		{
			vel = 1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}
		else if (m_afButtonPressed & IN_BACK)
		{
			vel = -1;
			pTrain->Use(this, this, USE_SET, (float)vel);
		}
		else if (m_afButtonPressed & IN_JUMP)
		{
			m_afPhysicsFlags &= ~PFLAG_DIROVERRIDE;
			m_iTrain = TRAIN_NEW | TRAIN_OFF;
		}

		if (vel)
		{
			m_iTrain = TrainSpeed(pTrain->m_flSpeed, ((CFuncTrackTrain*)pTrain)->GetMaxSpeed());
			m_iTrain |= TRAIN_ACTIVE | TRAIN_NEW;
		}
	}
	else if (m_iTrain & TRAIN_ACTIVE)
	{
		m_iTrain = TRAIN_NEW;
	}

	if (m_nButtons & IN_JUMP)
	{
		if (IsPullingObject())
		{
			StopPullingObject();
		}

		Jump();
	}

	if ((m_nButtons & IN_DUCK) || (GetFlags() & FL_DUCKING) || (m_afPhysicsFlags & PFLAG_DUCKING))
		Duck();

	if (!(GetFlags() & FL_ONGROUND))
	{
		m_Local.m_flFallVelocity = -GetAbsVelocity().z;
	}

	if (m_afPhysicsFlags & PFLAG_ONBARNACLE)
	{
		SetAbsVelocity(vec3_origin);
	}
	FindMissTargets();

	if (hl1_movement.GetBool())
	{
		SetMaxSpeed(400);
	}
	else
	{
		SetMaxSpeed(320);
	}
}

Class_T  CHL1_Player::Classify(void)
{
	if (m_nControlClass != CLASS_NONE)
	{
		return m_nControlClass;
	}
	else
	{
		return CLASS_PLAYER;
	}
}

bool CHL1_Player::HandleInteraction(int interactionType, void* data, CBaseCombatCharacter* sourceEnt)
{
	if (interactionType == g_interactionBarnacleVictimDangle)
	{
		TakeDamage(CTakeDamageInfo(sourceEnt, sourceEnt, m_iHealth + ArmorValue(), DMG_SLASH | DMG_ALWAYSGIB));
		return true;
	}

	if (interactionType == g_interactionBarnacleVictimReleased)
	{
		m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
		SetMoveType(MOVETYPE_WALK);
		return true;
	}
	else if (interactionType == g_interactionBarnacleVictimGrab)
	{
		m_afPhysicsFlags |= PFLAG_ONBARNACLE;
		ClearUseEntity();
		return true;
	}
	return false;
}


void CHL1_Player::PlayerRunCommand(CUserCmd* ucmd, IMoveHelper* moveHelper)
{
	if (m_afPhysicsFlags & PFLAG_ONBARNACLE)
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
	}

	if ((ucmd->forwardmove != 0) || (ucmd->sidemove != 0) || (ucmd->upmove != 0))
	{
		m_flIdleTime -= TICK_INTERVAL * 2.0f;

		if (m_flIdleTime < 0.0f)
		{
			m_flIdleTime = 0.0f;
		}

		m_flMoveTime += TICK_INTERVAL;

		if (m_flMoveTime > 4.0f)
		{
			m_flMoveTime = 4.0f;
		}
	}
	else
	{
		m_flIdleTime += TICK_INTERVAL;

		if (m_flIdleTime > 4.0f)
		{
			m_flIdleTime = 4.0f;
		}

		m_flMoveTime -= TICK_INTERVAL * 2.0f;

		if (m_flMoveTime < 0.0f)
		{
			m_flMoveTime = 0.0f;
		}
	}

	BaseClass::PlayerRunCommand(ucmd, moveHelper);
}

CBaseEntity* CHL1_Player::GiveNamedItem(const char* pszName, int iSubType)
{
	if (Weapon_OwnsThisType(pszName, iSubType))
		return NULL;

	EHANDLE pent;

	pent = CreateEntityByName(pszName);
	if (pent == NULL)
	{
		Msg("NULL Ent in GiveNamedItem!\n");
		return NULL;
	}

	pent->SetLocalOrigin(GetLocalOrigin());
	pent->AddSpawnFlags(SF_NORESPAWN);

	if (iSubType)
	{
		CBaseCombatWeapon* pWeapon = dynamic_cast<CBaseCombatWeapon*>((CBaseEntity*)pent);
		if (pWeapon)
		{
			pWeapon->SetSubType(iSubType);
		}
	}

	DispatchSpawn(pent);

	if (pent != NULL && !(pent->IsMarkedForDeletion()))
	{
		pent->Touch(this);
	}

	return pent;
}

void CHL1_Player::StartPullingObject(CBaseEntity* pObject)
{
	if (pObject->VPhysicsGetObject() == NULL || VPhysicsGetObject() == NULL)
	{
		return;
	}

	if (!(GetFlags() & FL_ONGROUND))
	{
		return;
	}

	if (GetGroundEntity() == pObject)
	{
		return;
	}

	constraint_ballsocketparams_t ballsocket;
	ballsocket.Defaults();
	ballsocket.constraint.Defaults();
	ballsocket.constraint.forceLimit = lbs2kg(1000);
	ballsocket.constraint.torqueLimit = lbs2kg(1000);
	ballsocket.InitWithCurrentObjectState(VPhysicsGetObject(), pObject->VPhysicsGetObject(), WorldSpaceCenter());
	m_pPullConstraint = physenv->CreateBallsocketConstraint(VPhysicsGetObject(), pObject->VPhysicsGetObject(), NULL, ballsocket);

	m_hPullObject.Set(pObject);
	m_bIsPullingObject = true;
}

void CHL1_Player::StopPullingObject()
{
	if (m_pPullConstraint)
	{
		physenv->DestroyConstraint(m_pPullConstraint);
	}

	m_hPullObject.Set(NULL);
	m_pPullConstraint = NULL;
	m_bIsPullingObject = false;
}

void CHL1_Player::UpdatePullingObject()
{
	if (!IsPullingObject())
		return;

	CBaseEntity* pObject = m_hPullObject.Get();

	if (!pObject || !pObject->VPhysicsGetObject())
	{
		StopPullingObject();
		return;
	}

	if (m_afButtonReleased & IN_USE)
	{
		StopPullingObject();
		return;
	}


	float flMaxDistSqr = Square(PLAYER_USE_RADIUS + 1.0f);

	Vector objectPos;
	QAngle angle;

	pObject->VPhysicsGetObject()->GetPosition(&objectPos, &angle);

	if (!FInViewCone(objectPos))
	{
		StopPullingObject();
	}
	else if (objectPos.DistToSqr(WorldSpaceCenter()) > flMaxDistSqr)
	{
		StopPullingObject();
	}
}

void CHL1_Player::Spawn(void)
{
	SetModel("models/player.mdl");

	BaseClass::Spawn();

	SetMaxSpeed(400);

	SetDefaultFOV(0);

	m_nFlashBattery = 99;
	m_flFlashLightTime = 1;

	m_flFieldOfView = 0.5;

	StopPullingObject();

	m_Local.m_iHideHUD = 0;
}

void CHL1_Player::Event_Killed(const CTakeDamageInfo& info)
{
	if (m_iHealth < -40 && !(info.GetDamageType() & DMG_NEVERGIB) || info.GetDamageType() & DMG_ALWAYSGIB)
	{
		CEffectData	data;

		data.m_vOrigin = WorldSpaceCenter();
		data.m_vNormal = data.m_vOrigin - info.GetDamagePosition();
		VectorNormalize(data.m_vNormal);

		data.m_flScale = RemapVal(m_iHealth, 0, -500, 1, 3);
		data.m_flScale = clamp(data.m_flScale, 1, 3);

		data.m_nMaterial = 1;

		data.m_nColor = BloodColor();

		DispatchEffect("HL1Gib", data);

		EmitSound("Player.FallGib");

		CSoundEnt::InsertSound(SOUND_MEAT, GetAbsOrigin(), 256, 0.5f, this);
	}

	StopPullingObject();
	BaseClass::Event_Killed(info);
}

void CHL1_Player::CheckTimeBasedDamage(void)
{
	int i;
	byte bDuration = 0;

	static float gtbdPrev = 0.0;

	if (!g_pGameRules->Damage_IsTimeBased(m_bitsDamageType))
		return;

	if (abs(gpGlobals->curtime - m_tbdPrev) < 2.0)
		return;

	m_tbdPrev = gpGlobals->curtime;

	for (i = 0; i < CDMG_TIMEBASED; i++)
	{
		int iDamage = (DMG_PARALYZE << i);
		if (!g_pGameRules->Damage_IsTimeBased(iDamage))
			continue;

		if (m_bitsDamageType & iDamage)
		{
			switch (i)
			{
			case itbd_Paralyze:
				bDuration = PARALYZE_DURATION;
				break;
			case itbd_NerveGas:
				bDuration = NERVEGAS_DURATION;
				break;
			case itbd_PoisonRecover:
				OnTakeDamage(CTakeDamageInfo(this, this, POISON_DAMAGE, DMG_GENERIC));
				bDuration = POISON_DURATION;
				break;
			case itbd_Radiation:
				bDuration = RADIATION_DURATION;
				break;
			case itbd_DrownRecover:
				if (m_idrowndmg > m_idrownrestored)
				{
					int idif = min(m_idrowndmg - m_idrownrestored, 10);

					TakeHealth(idif, DMG_GENERIC);
					m_idrownrestored += idif;
				}
				bDuration = 4;
				break;

			case itbd_Acid:
				bDuration = ACID_DURATION;
				break;
			case itbd_SlowBurn:
				bDuration = SLOWBURN_DURATION;
				break;
			case itbd_SlowFreeze:
				bDuration = SLOWFREEZE_DURATION;
				break;
			default:
				bDuration = 0;
			}

			if (m_rgbTimeBasedDamage[i])
			{
				if (!m_rgbTimeBasedDamage[i] || --m_rgbTimeBasedDamage[i] == 0)
				{
					m_rgbTimeBasedDamage[i] = 0;
					m_bitsDamageType &= ~(DMG_PARALYZE << i);
				}
			}
			else
				m_rgbTimeBasedDamage[i] = bDuration;
		}
	}
}


class CPhysicsPlayerCallback : public IPhysicsPlayerControllerEvent
{
public:
	int ShouldMoveTo(IPhysicsObject* pObject, const Vector& position)
	{
		CHL1_Player* pPlayer = (CHL1_Player*)pObject->GetGameData();
		if (pPlayer)
		{
			if (pPlayer->TouchedPhysics())
			{
				return 0;
			}
		}
		return 1;
	}
};

static CPhysicsPlayerCallback playerCallback;

void CHL1_Player::InitVCollision(const Vector& vecAbsOrigin, const Vector& vecAbsVelocity)
{
	BaseClass::InitVCollision(vecAbsOrigin, vecAbsVelocity);

	GetPhysicsController()->SetEventHandler(&playerCallback);
}


CHL1_Player::~CHL1_Player(void)
{
}

extern int gEvilImpulse101;
void CHL1_Player::CheatImpulseCommands(int iImpulse)
{
	if (!sv_cheats->GetBool())
	{
		return;
	}

	switch (iImpulse)
	{
	case 101:
		gEvilImpulse101 = true;

		GiveNamedItem("item_suit");
		GiveNamedItem("item_battery");
		GiveNamedItem("weapon_crowbar");
		GiveNamedItem("weapon_glock");
		GiveNamedItem("ammo_9mmclip");
		GiveNamedItem("weapon_shotgun");
		GiveNamedItem("ammo_buckshot");
		GiveNamedItem("weapon_mp5");
		GiveNamedItem("ammo_9mmar");
		GiveNamedItem("ammo_argrenades");
		GiveNamedItem("weapon_handgrenade");
		GiveNamedItem("weapon_tripmine");
		GiveNamedItem("weapon_357");
		GiveNamedItem("ammo_357");
		GiveNamedItem("weapon_crossbow");
		GiveNamedItem("ammo_crossbow");
		GiveNamedItem("weapon_egon");
		GiveNamedItem("weapon_gauss");
		GiveNamedItem("ammo_gaussclip");
		GiveNamedItem("weapon_rpg");
		GiveNamedItem("ammo_rpgclip");
		GiveNamedItem("weapon_satchel");
		GiveNamedItem("weapon_snark");
		GiveNamedItem("weapon_hornetgun");

		gEvilImpulse101 = false;
		break;

	case 0:
	default:
		BaseClass::CheatImpulseCommands(iImpulse);
	}
}

void CHL1_Player::SetupVisibility(CBaseEntity* pViewEntity, unsigned char* pvs, int pvssize)
{
	int area = pViewEntity ? pViewEntity->NetworkProp()->AreaNum() : NetworkProp()->AreaNum();
	BaseClass::SetupVisibility(pViewEntity, pvs, pvssize);
	PointCameraSetupVisibility(this, area, pvs, pvssize);
}


#define ARMOR_RATIO	 0.2
#define ARMOR_BONUS  0.5


int	CHL1_Player::OnTakeDamage(const CTakeDamageInfo& inputInfo)
{
	int bitsDamage = inputInfo.GetDamageType();
	int ffound = true;
	int fmajor;
	int fcritical;
	int fTookDamage;
	int ftrivial;
	float flRatio;
	float flBonus;
	float flHealthPrev = m_iHealth;

	CTakeDamageInfo info = inputInfo;

	if (info.GetDamage() > 0.0f)
	{
		m_flLastDamageTime = gpGlobals->curtime;
	}

	flBonus = ARMOR_BONUS;
	flRatio = ARMOR_RATIO;

	if ((info.GetDamageType() & DMG_BLAST) && g_pGameRules->IsMultiplayer())
	{
		flBonus *= 2;
	}

	if (!IsAlive())
		return 0;

	if (!g_pGameRules->FPlayerCanTakeDamage(this, info.GetAttacker()))
	{
		return 0;
	}

	m_lastDamageAmount = info.GetDamage();

	if (ArmorValue() &&
		!(info.GetDamageType() & (DMG_FALL | DMG_DROWN | DMG_POISON)) && !(GetFlags() & FL_GODMODE))
	{
		float flNew = info.GetDamage() * flRatio;

		float flArmor;

		flArmor = (info.GetDamage() - flNew) * flBonus;

		if (flArmor > ArmorValue())
		{
			flArmor = ArmorValue();
			flArmor *= (1 / flBonus);
			flNew = info.GetDamage() - flArmor;
			SetArmorValue(0);
		}
		else
			SetArmorValue(ArmorValue() - flArmor);

		info.SetDamage(flNew);
	}

	info.SetDamage((int)info.GetDamage());
	fTookDamage = CBaseCombatCharacter::OnTakeDamage(info);

	if (fTookDamage)
	{
		if (info.GetInflictor() && info.GetInflictor()->edict())
			m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();

		m_DmgTake += (int)info.GetDamage();
	}

	for (int i = 0; i < CDMG_TIMEBASED; i++)
	{
		int iDamage = (DMG_PARALYZE << i);
		if ((info.GetDamageType() & iDamage) && g_pGameRules->Damage_IsTimeBased(iDamage))
		{
			m_rgbTimeBasedDamage[i] = 0;
		}
	}

	DamageEffect(info.GetDamage(), bitsDamage);

	ftrivial = (m_iHealth > 75 || m_lastDamageAmount < 5);
	fmajor = (m_lastDamageAmount > 25);
	fcritical = (m_iHealth < 30);

	m_bitsDamageType |= bitsDamage;
	m_bitsHUDDamage = -1;

	bool bTimeBasedDamage = g_pGameRules->Damage_IsTimeBased(bitsDamage);
	while (fTookDamage && (!ftrivial || bTimeBasedDamage) && ffound && bitsDamage)
	{
		ffound = false;

		if (bitsDamage & DMG_CLUB)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC);
			bitsDamage &= ~DMG_CLUB;
			ffound = true;
		}
		if (bitsDamage & (DMG_FALL | DMG_CRUSH))
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG5", false, SUIT_NEXT_IN_30SEC);
			else
				SetSuitUpdate("!HEV_DMG4", false, SUIT_NEXT_IN_30SEC);

			bitsDamage &= ~(DMG_FALL | DMG_CRUSH);
			ffound = true;
		}

		if (bitsDamage & DMG_BULLET)
		{
			if (m_lastDamageAmount > 5)
				SetSuitUpdate("!HEV_DMG6", false, SUIT_NEXT_IN_30SEC);

			bitsDamage &= ~DMG_BULLET;
			ffound = true;
		}

		if (bitsDamage & DMG_SLASH)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG1", false, SUIT_NEXT_IN_30SEC);
			else
				SetSuitUpdate("!HEV_DMG0", false, SUIT_NEXT_IN_30SEC);

			bitsDamage &= ~DMG_SLASH;
			ffound = true;
		}

		if (bitsDamage & DMG_SONIC)
		{
			if (fmajor)
				SetSuitUpdate("!HEV_DMG2", false, SUIT_NEXT_IN_1MIN);
			bitsDamage &= ~DMG_SONIC;
			ffound = true;
		}

		if (bitsDamage & (DMG_POISON | DMG_PARALYZE))
		{
			if (bitsDamage & DMG_POISON)
			{
				m_nPoisonDmg += info.GetDamage();
				m_tbdPrev = gpGlobals->curtime;
				m_rgbTimeBasedDamage[itbd_PoisonRecover] = 0;
			}

			SetSuitUpdate("!HEV_DMG3", false, SUIT_NEXT_IN_1MIN);
			bitsDamage &= ~(DMG_POISON | DMG_PARALYZE);
			ffound = true;
		}

		if (bitsDamage & DMG_ACID)
		{
			SetSuitUpdate("!HEV_DET1", false, SUIT_NEXT_IN_1MIN);
			bitsDamage &= ~DMG_ACID;
			ffound = true;
		}

		if (bitsDamage & DMG_NERVEGAS)
		{
			SetSuitUpdate("!HEV_DET0", false, SUIT_NEXT_IN_1MIN);
			bitsDamage &= ~DMG_NERVEGAS;
			ffound = true;
		}

		if (bitsDamage & DMG_RADIATION)
		{
			SetSuitUpdate("!HEV_DET2", false, SUIT_NEXT_IN_1MIN);
			bitsDamage &= ~DMG_RADIATION;
			ffound = true;
		}
		if (bitsDamage & DMG_SHOCK)
		{
			bitsDamage &= ~DMG_SHOCK;
			ffound = true;
		}
	}

	m_Local.m_vecPunchAngle.SetX(-2);

	if (fTookDamage && !ftrivial && fmajor && flHealthPrev >= 75)
	{
		SetSuitUpdate("!HEV_MED1", false, SUIT_NEXT_IN_30MIN);

		SetSuitUpdate("!HEV_HEAL7", false, SUIT_NEXT_IN_30MIN);
	}

	if (fTookDamage && !ftrivial && fcritical && flHealthPrev < 75)
	{
		if (m_iHealth < 6)
			SetSuitUpdate("!HEV_HLTH3", false, SUIT_NEXT_IN_10MIN);
		else if (m_iHealth < 20)
			SetSuitUpdate("!HEV_HLTH2", false, SUIT_NEXT_IN_10MIN);

		if (!random->RandomInt(0, 3) && flHealthPrev < 50)
			SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN);
	}

	if (fTookDamage && g_pGameRules->Damage_IsTimeBased(info.GetDamageType()) && flHealthPrev < 75)
	{
		if (flHealthPrev < 50)
		{
			if (!random->RandomInt(0, 3))
				SetSuitUpdate("!HEV_DMG7", false, SUIT_NEXT_IN_5MIN);
		}
		else
			SetSuitUpdate("!HEV_HLTH1", false, SUIT_NEXT_IN_10MIN);
	}

	if (bitsDamage & DMG_BLAST)
	{
		OnDamagedByExplosion(info);
	}

	gamestats->Event_PlayerDamage(this, info);

	return fTookDamage;
}

int CHL1_Player::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	int nRet;
	int nSavedHealth = m_iHealth;

	nRet = BaseClass::OnTakeDamage_Alive(info);

	if (GetFlags() & FL_GODMODE)
	{
		m_iHealth = nSavedHealth;
	}
	else if (m_debugOverlays & OVERLAY_BUDDHA_MODE)
	{
		if (m_iHealth <= 0)
		{
			m_iHealth = 1;
		}
	}

	return nRet;
}

void CHL1_Player::FindMissTargets(void)
{
	if (m_flTargetFindTime > gpGlobals->curtime)
		return;

	m_flTargetFindTime = gpGlobals->curtime + 1.0f;
	m_nNumMissPositions = 0;

	CBaseEntity* pEnts[256];
	Vector		radius(80, 80, 80);

	int numEnts = UTIL_EntitiesInBox(pEnts, 256, GetAbsOrigin() - radius, GetAbsOrigin() + radius, 0);

	for (int i = 0; i < numEnts; i++)
	{
		if (pEnts[i] == NULL)
			continue;

		if (m_nNumMissPositions >= 16)
			return;

		if (FClassnameIs(pEnts[i], "prop_dynamic") ||
			FClassnameIs(pEnts[i], "dynamic_prop") ||
			FClassnameIs(pEnts[i], "prop_physics") ||
			FClassnameIs(pEnts[i], "physics_prop"))
		{
			m_vecMissPositions[m_nNumMissPositions++] = pEnts[i]->WorldSpaceCenter();
			continue;
		}
	}
}

bool CHL1_Player::GetMissPosition(Vector* position)
{
	if (m_nNumMissPositions == 0)
		return false;

	(*position) = m_vecMissPositions[random->RandomInt(0, m_nNumMissPositions - 1)];

	return true;
}

int CHL1_Player::FlashlightIsOn(void)
{
	return IsEffectActive(EF_DIMLIGHT);
}

void CHL1_Player::FlashlightTurnOn(void)
{
	if (IsSuitEquipped())
	{
		AddEffects(EF_DIMLIGHT);
		CPASAttenuationFilter filter(this);
		EmitSound(filter, entindex(), "Player.FlashlightOn");

		m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->curtime;
	}
}

void CHL1_Player::FlashlightTurnOff(void)
{
	RemoveEffects(EF_DIMLIGHT);
	CPASAttenuationFilter filter(this);
	EmitSound(filter, entindex(), "Player.FlashlightOff");
	m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->curtime;
}


void CHL1_Player::UpdateClientData(void)
{
	if (m_DmgTake || m_DmgSave || m_bitsHUDDamage != m_bitsDamageType)
	{
		Vector damageOrigin = GetLocalOrigin();
		damageOrigin = m_DmgOrigin;

		int iShowHudDamage = g_pGameRules->Damage_GetShowOnHud();
		int visibleDamageBits = m_bitsDamageType & iShowHudDamage;

		m_DmgTake = clamp(m_DmgTake, 0, 255);
		m_DmgSave = clamp(m_DmgSave, 0, 255);

		CSingleUserRecipientFilter user(this);
		user.MakeReliable();
		UserMessageBegin(user, "Damage");
		WRITE_BYTE(m_DmgSave);
		WRITE_BYTE(m_DmgTake);
		WRITE_LONG(visibleDamageBits);
		WRITE_FLOAT(damageOrigin.x);
		WRITE_FLOAT(damageOrigin.y);
		WRITE_FLOAT(damageOrigin.z);
		MessageEnd();

		m_DmgTake = 0;
		m_DmgSave = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		int iDamage = g_pGameRules->Damage_GetTimeBased();
		m_bitsDamageType &= iDamage;
	}

	if ((m_flFlashLightTime) && (m_flFlashLightTime <= gpGlobals->curtime))
	{
		if (FlashlightIsOn())
		{
			if (m_nFlashBattery)
			{
				m_flFlashLightTime = FLASH_DRAIN_TIME + gpGlobals->curtime;
				m_nFlashBattery--;

				if (!m_nFlashBattery)
					FlashlightTurnOff();
			}
		}
		else
		{
			if (m_nFlashBattery < 100)
			{
				m_flFlashLightTime = FLASH_CHARGE_TIME + gpGlobals->curtime;
				m_nFlashBattery++;
			}
			else
				m_flFlashLightTime = 0;
		}
	}

	BaseClass::UpdateClientData();
}

void CHL1_Player::OnSave(IEntitySaveUtils* pUtils)
{
	StopPullingObject();

	BaseClass::OnSave(pUtils);
}

void CHL1_Player::CreateViewModel(int index)
{
	Assert(index >= 0 && index < MAX_VIEWMODELS);

	if (GetViewModel(index))
		return;

	CPredictedViewModel* vm = (CPredictedViewModel*)CreateEntityByName("predicted_viewmodel");
	if (vm)
	{
		vm->SetAbsOrigin(GetAbsOrigin());
		vm->SetOwner(this);
		vm->SetIndex(index);
		DispatchSpawn(vm);
		vm->FollowEntity(this);
		m_hViewModel.Set(index, vm);
	}
}

void CHL1_Player::OnRestore(void)
{
	BaseClass::OnRestore();

	if (!FBitSet(m_iTrain, TRAIN_OFF))
	{
		m_iTrain |= TRAIN_NEW;
	}
}

const char* CHL1_Player::GetOverrideStepSound(const char* pszBaseStepSoundName)
{
	if (hl1_move_sounds.GetBool())
	{
		CSoundParameters params;
		char hl1step[50] = "hl1";
		strcat_s(hl1step, pszBaseStepSoundName);
		if (CBaseEntity::GetParametersForSound(hl1step, params, NULL))
			pszBaseStepSoundName = hl1step;
	}

	return pszBaseStepSoundName;
}