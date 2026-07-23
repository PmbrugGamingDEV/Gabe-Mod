#include "cbase.h"
#include "weapon_duplicator.h"
#include "hl2_player.h"
#include "in_buttons.h"
#include "util.h"
#include "physics.h"
#include "engine/IEngineTrace.h"

LINK_ENTITY_TO_CLASS(weapon_duplicator, CWeaponDuplicator);

PRECACHE_WEAPON_REGISTER(weapon_duplicator);

IMPLEMENT_SERVERCLASS_ST(CWeaponDuplicator, DT_WeaponDuplicator)
END_SEND_TABLE()

BEGIN_DATADESC(CWeaponDuplicator)
DEFINE_FIELD(m_CopiedClassname, FIELD_STRING),
DEFINE_FIELD(m_CopiedOrigin, FIELD_VECTOR),
DEFINE_FIELD(m_CopiedAngles, FIELD_VECTOR),
DEFINE_FIELD(m_CopiedModelName, FIELD_STRING),
END_DATADESC()

CWeaponDuplicator::CWeaponDuplicator()
{
    m_CopiedClassname = NULL_STRING;
    m_CopiedOrigin.Init();
    m_CopiedAngles.Init();
    m_CopiedModelName = NULL_STRING;
}

void CWeaponDuplicator::Precache(void)
{
    PrecacheModel("models/weapons/v_duplicator.mdl");
    PrecacheModel("models/weapons/w_duplicator.mdl");
    BaseClass::Precache();
}

void CWeaponDuplicator::PrimaryAttack(void)
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
        return;
    SendWeaponAnim(ACT_VM_PRIMARYATTACK);
    WeaponSound(SINGLE);
    DuplicateEntity(pPlayer);
    m_flNextPrimaryAttack = gpGlobals->curtime + 0.3f;
}

void CWeaponDuplicator::SecondaryAttack(void)
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
        return;

    trace_t tr;
    Vector forward;
    pPlayer->EyeVectors(&forward);
    UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

    if (tr.m_pEnt && !tr.m_pEnt->IsWorld() && !tr.m_pEnt->IsPlayer())
    {
        ClientPrint(pPlayer, 3, "Data Copied!\n");
        CopyEntityData(tr.m_pEnt);
    }
    else
    {
        ClientPrint(pPlayer, 3, "Cannot copy world or player.\n");
    }
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
}

void CWeaponDuplicator::CopyEntityData(CBaseEntity* pEnt)
{
    m_CopiedClassname = AllocPooledString(pEnt->GetClassname());
    m_CopiedOrigin = pEnt->GetAbsOrigin();
    m_CopiedAngles = pEnt->GetAbsAngles();
    m_CopiedModelName = AllocPooledString(pEnt->GetModelName().ToCStr()); // Copy the model name

    // Here you can copy other necessary data from the entity
}

void CWeaponDuplicator::DuplicateEntity(CBasePlayer* pPlayer)
{
    if (m_CopiedClassname == NULL_STRING)
        return;

    Vector forward;
    pPlayer->EyeVectors(&forward);
    Vector spawnPos = pPlayer->EyePosition() + forward * 200; // Adjust the spawn position as needed

    CBaseEntity* pDuplicate = CreateEntityByName(STRING(m_CopiedClassname));
    if (pDuplicate)
    {
        pDuplicate->SetAbsOrigin(spawnPos);
        pDuplicate->SetAbsAngles(m_CopiedAngles);
        pDuplicate->SetModel(STRING(m_CopiedModelName)); // Assign the model name
        pDuplicate->Spawn();

        // Here you can apply the copied data to the new entity
    }
}
