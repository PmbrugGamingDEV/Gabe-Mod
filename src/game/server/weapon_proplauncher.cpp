#include "cbase.h"
#include "weapon_proplauncher.h"
#include "player.h"
#include "gamerules.h"
#include "in_buttons.h"
#include "te_effect_dispatch.h"
#include "basecombatweapon_shared.h"
#include "fmtstr.h"
#include "physics.h"
#include "vphysics/constraints.h"

LINK_ENTITY_TO_CLASS(weapon_proplauncher, CWeaponPropLauncher);
PRECACHE_WEAPON_REGISTER(weapon_proplauncher);

BEGIN_DATADESC(CWeaponPropLauncher)
    DEFINE_FIELD(m_iCurrentPropIndex, FIELD_INTEGER),
    DEFINE_FIELD(m_iCurrentMode, FIELD_INTEGER),
    DEFINE_FIELD(m_hFirstEntity, FIELD_EHANDLE),
    DEFINE_FIELD(m_vecLastHitPos, FIELD_VECTOR),
    DEFINE_FIELD(m_bFirstPointSet, FIELD_BOOLEAN),
    DEFINE_FIELD(m_flNextModeSwitchTime, FIELD_TIME),
    DEFINE_FIELD(m_hPhysgunTarget, FIELD_EHANDLE),
    DEFINE_FIELD(m_pPhysgunTargetObject, FIELD_CLASSPTR),
    DEFINE_FIELD(m_pPhysgunConstraint, FIELD_CLASSPTR),
    DEFINE_FIELD(m_bHoldingRotateKey, FIELD_BOOLEAN),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponPropLauncher, DT_WeaponPropLauncher)
END_SEND_TABLE()

CWeaponPropLauncher::CWeaponPropLauncher()
{
    m_iCurrentPropIndex = 0;
    m_iCurrentMode = MODE_LAUNCH;
    m_bFirstPointSet = false;
    m_flNextModeSwitchTime = 0.0f;
    m_hPhysgunTarget = NULL;
    m_pPhysgunTargetObject = NULL;
    m_pPhysgunConstraint = NULL;
    m_bHoldingRotateKey = false;

    m_PropModels.AddToTail("models/props_c17/oildrum001.mdl");
    m_PropModels.AddToTail("models/props_junk/wood_crate001a.mdl");
    m_PropModels.AddToTail("models/props_c17/FurnitureChair001a.mdl");
    m_PropModels.AddToTail("models/props_c17/FurnitureTable001a.mdl");
    m_PropModels.AddToTail("models/props_lab/desklamp01.mdl");

    // Additional props
    m_PropModels.AddToTail("models/props_c17/oildrum002.mdl");
    m_PropModels.AddToTail("models/props_junk/wood_crate002a.mdl");
    m_PropModels.AddToTail("models/props_c17/FurnitureChair002a.mdl");
    m_PropModels.AddToTail("models/props_c17/FurnitureTable002a.mdl");
    m_PropModels.AddToTail("models/props_lab/filecabinet02.mdl");
    m_PropModels.AddToTail("models/props_lab/monitor02.mdl");
    m_PropModels.AddToTail("models/props_lab/tpswitch.mdl");
    m_PropModels.AddToTail("models/props_junk/metalbucket01a.mdl");
    m_PropModels.AddToTail("models/props_junk/plasticcrate01a.mdl");
    m_PropModels.AddToTail("models/props_junk/popcan01a.mdl");
    m_PropModels.AddToTail("models/props_c17/metalpot001a.mdl");
    m_PropModels.AddToTail("models/props_c17/pottery02a.mdl");
    m_PropModels.AddToTail("models/props_c17/metalpot002a.mdl");
    m_PropModels.AddToTail("models/props_junk/garbage_metalcan001a.mdl");
    m_PropModels.AddToTail("models/props_c17/metalpot003a.mdl");
    m_PropModels.AddToTail("models/props_c17/bench01a.mdl");
    m_PropModels.AddToTail("models/props_c17/chair_office01a.mdl");
    m_PropModels.AddToTail("models/props_c17/clock01.mdl");
    m_PropModels.AddToTail("models/props_interiors/Radiator01a.mdl");
    m_PropModels.AddToTail("models/props_interiors/SinkKitchen01a.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01a.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01b.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01c.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01d.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01e.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01f.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01g.mdl");
    m_PropModels.AddToTail("models/props_interiors/BathTub01h.mdl");
}

void CWeaponPropLauncher::Precache(void)
{
    for (int i = 0; i < m_PropModels.Count(); i++)
    {
        PrecacheModel(m_PropModels[i]);
    }
    BaseClass::Precache();
}

bool CWeaponPropLauncher::Deploy(void)
{
    return BaseClass::Deploy();
}

void CWeaponPropLauncher::PrimaryAttack(void)
{
    if (m_iCurrentMode == MODE_LAUNCH)
    {
        if (m_iCurrentPropIndex >= 0 && m_iCurrentPropIndex < m_PropModels.Count())
        {
            LaunchProp(m_PropModels[m_iCurrentPropIndex]);
        }
    }
    else if (m_iCurrentMode == MODE_WELD)
    {
        WeldEntities();
    }
    else if (m_iCurrentMode == MODE_PHYSGUN)
    {
        if (m_hPhysgunTarget == NULL)
        {
            PhysgunPrimary(); // Pick up object
        }
        else
        {
            PhysgunSecondary(); // Drop object
        }
    }
    m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;
}

void CWeaponPropLauncher::SecondaryAttack(void)
{
    if (m_iCurrentMode == MODE_PHYSGUN)
    {
        FreezeProp();
    }
    else
    {
        CycleProps();
    }
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.3f;
}

void CWeaponPropLauncher::LaunchProp(const char* modelName)
{
    CBasePlayer* pOwner = ToBasePlayer(GetOwner());
    if (!pOwner)
        return;

    Vector vecSrc = pOwner->Weapon_ShootPosition();
    QAngle angAiming = pOwner->EyeAngles();

    CBaseEntity* pProp = CreateEntityByName("prop_physics_override");
    if (pProp)
    {
        pProp->SetModel(modelName);
        pProp->SetAbsOrigin(vecSrc);
        pProp->SetAbsAngles(angAiming);
        DispatchSpawn(pProp);
        pProp->Activate();

        IPhysicsObject* pPhysics = pProp->VPhysicsGetObject();
        if (pPhysics)
        {
            Vector vecVelocity;
            AngleVectors(angAiming, &vecVelocity);
            vecVelocity *= 1000.0f;
            pPhysics->AddVelocity(&vecVelocity, NULL);
        }
    }

    ClientPrint(pOwner, HUD_PRINTCENTER, CFmtStr("Spawned: %s", modelName));
}

void CWeaponPropLauncher::WeldEntities()
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
        return;

    if (!m_bFirstPointSet)
    {
        trace_t tr;
        Vector vecStart = pPlayer->Weapon_ShootPosition();
        Vector vecDir = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
        Vector vecEnd = vecStart + (vecDir * MAX_TRACE_LENGTH);
        UTIL_TraceLine(vecStart, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

        if (tr.DidHit() && tr.m_pEnt && tr.m_pEnt->GetMoveType() == MOVETYPE_VPHYSICS)
        {
            m_hFirstEntity = tr.m_pEnt;
            m_vecLastHitPos = tr.endpos;
            m_bFirstPointSet = true;
            ClientPrint(pPlayer, HUD_PRINTCENTER, "First point set");
        }
        else
        {
            ClientPrint(pPlayer, HUD_PRINTCENTER, "No valid target for the first point");
        }
    }
    else
    {
        trace_t tr;
        Vector vecStart = pPlayer->Weapon_ShootPosition();
        Vector vecDir = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
        Vector vecEnd = vecStart + (vecDir * MAX_TRACE_LENGTH);
        UTIL_TraceLine(vecStart, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

        if (tr.DidHit() && tr.m_pEnt && tr.m_pEnt->GetMoveType() == MOVETYPE_VPHYSICS)
        {
            IPhysicsObject* pPhys1 = m_hFirstEntity->VPhysicsGetObject();
            IPhysicsObject* pPhys2 = tr.m_pEnt->VPhysicsGetObject();

            if (pPhys1 && pPhys2)
            {
                constraint_fixedparams_t fixed;
                fixed.Defaults();
                fixed.InitWithCurrentObjectState(pPhys1, pPhys2);

                IPhysicsConstraint* pConstraint = physenv->CreateFixedConstraint(pPhys1, pPhys2, NULL, fixed);
                if (pConstraint)
                {
                    pConstraint->SetGameData((void*)this);
                }
                ClientPrint(pPlayer, HUD_PRINTCENTER, "Weld created");
            }

            m_bFirstPointSet = false;
        }
        else
        {
            ClientPrint(pPlayer, HUD_PRINTCENTER, "No valid target for the second point");
        }
    }
}

void CWeaponPropLauncher::CycleProps()
{
    m_iCurrentPropIndex = (m_iCurrentPropIndex + 1) % m_PropModels.Count();
    CBasePlayer* pOwner = ToBasePlayer(GetOwner());
    if (pOwner)
    {
        ClientPrint(pOwner, HUD_PRINTCENTER, CFmtStr("Current Prop: %s", m_PropModels[m_iCurrentPropIndex]));
    }
}

void CWeaponPropLauncher::ItemPostFrame(void)
{
    BaseClass::ItemPostFrame();

    CBasePlayer* pOwner = ToBasePlayer(GetOwner());
    if (!pOwner)
        return;

    HandleKeyPresses();

    if (m_iCurrentMode == MODE_PHYSGUN && m_hPhysgunTarget != NULL)
    {
        Vector vecStart = pOwner->Weapon_ShootPosition();
        Vector vecDir = pOwner->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
        Vector vecEnd = vecStart + (vecDir * 128.0f); // Adjust distance as needed

        if (m_pPhysgunTargetObject)
        {
            m_pPhysgunTargetObject->Wake();
            Vector targetPos = vecEnd;
            Vector targetVel = (targetPos - m_hPhysgunTarget->GetAbsOrigin()) * 10.0f;
            m_pPhysgunTargetObject->SetVelocityInstantaneous(&targetVel, NULL);
        }
    }
}

bool CWeaponPropLauncher::Reload(void)
{
    if (m_flNextPrimaryAttack <= gpGlobals->curtime)
    {
        if (gpGlobals->curtime >= m_flNextModeSwitchTime)
        {
            SwitchMode();
            m_flNextModeSwitchTime = gpGlobals->curtime + 1.0f; // 1 second cooldown
        }
        return true;
    }
    return false;
}

void CWeaponPropLauncher::SwitchMode()
{
    m_iCurrentMode = static_cast<PropLauncherMode>((m_iCurrentMode + 1) % 3);
    CBasePlayer* pOwner = ToBasePlayer(GetOwner());
    if (pOwner)
    {
        const char* modeName = (m_iCurrentMode == MODE_LAUNCH) ? "Launch" :
                               (m_iCurrentMode == MODE_WELD) ? "Weld" : "Physgun";
        ClientPrint(pOwner, HUD_PRINTCENTER, CFmtStr("Mode: %s", modeName));
    }
}

void CWeaponPropLauncher::PhysgunPrimary()
{
    CBasePlayer* pOwner = ToBasePlayer(GetOwner());
    if (!pOwner)
        return;

    if (m_hPhysgunTarget == NULL)
    {
        trace_t tr;
        Vector vecStart = pOwner->Weapon_ShootPosition();
        Vector vecDir = pOwner->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);
        Vector vecEnd = vecStart + (vecDir * MAX_TRACE_LENGTH);
        UTIL_TraceLine(vecStart, vecEnd, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr);

        if (tr.DidHit() && tr.m_pEnt && tr.m_pEnt->GetMoveType() == MOVETYPE_VPHYSICS)
        {
            m_hPhysgunTarget = tr.m_pEnt;
            m_pPhysgunTargetObject = tr.m_pEnt->VPhysicsGetObject();
            ClientPrint(pOwner, HUD_PRINTCENTER, "Object grabbed");
        }
    }
    else
    {
        PhysgunSecondary();
    }
}

void CWeaponPropLauncher::PhysgunSecondary()
{
    if (m_hPhysgunTarget != NULL)
    {
        m_hPhysgunTarget = NULL;
        m_pPhysgunTargetObject = NULL;
        CBasePlayer* pOwner = ToBasePlayer(GetOwner());
        if (pOwner)
        {
            ClientPrint(pOwner, HUD_PRINTCENTER, "Object released");
        }
    }
}

void CWeaponPropLauncher::FreezeProp()
{
    if (m_hPhysgunTarget != NULL && m_pPhysgunTargetObject)
    {
        m_pPhysgunTargetObject->EnableMotion(false);
        m_hPhysgunTarget = NULL;
        m_pPhysgunTargetObject = NULL;
        ClientPrint(ToBasePlayer(GetOwner()), HUD_PRINTCENTER, "Prop frozen");
    }
}

void CWeaponPropLauncher::HandleKeyPresses()
{
    CBasePlayer* pOwner = ToBasePlayer(GetOwner());
    if (!pOwner)
        return;

    if (m_hPhysgunTarget != NULL)
    {
        Vector pos;
        QAngle angles;
        m_pPhysgunTargetObject->GetPosition(&pos, &angles);

        if (pOwner->m_nButtons & IN_MOVELEFT)
        {
            angles[YAW] -= 1.0f;  // Adjust rotation speed as needed
        }
        if (pOwner->m_nButtons & IN_MOVERIGHT)
        {
            angles[YAW] += 1.0f;  // Adjust rotation speed as needed
        }
        if (pOwner->m_nButtons & IN_FORWARD)
        {
            angles[PITCH] -= 1.0f; // Adjust rotation speed as needed
        }
        if (pOwner->m_nButtons & IN_BACK)
        {
            angles[PITCH] += 1.0f; // Adjust rotation speed as needed

            m_pPhysgunTargetObject->SetPosition(pos, angles, true);
        }
    }
}
