#include "cbase.h"
#include "basehlcombatweapon.h"
#include "npc_manhack.h"
#include "vphysics/constraints.h"
#include "physics.h"
#include "ieffects.h"

class CWeaponEAttach : public CBaseHLCombatWeapon
{
public:
    DECLARE_CLASS(CWeaponEAttach, CBaseHLCombatWeapon);
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    void Precache() override;
    void PrimaryAttack() override;
    void SecondaryAttack() override;

private:
    void AttachNPC(const char* pClassname);
    void CreateFixedConstraint(CBaseEntity* pA, CBaseEntity* pB, const Vector& attachPos);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponEAttach, DT_WeaponEAttach)

BEGIN_NETWORK_TABLE(CWeaponEAttach, DT_WeaponEAttach)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponEAttach)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_eattach, CWeaponEAttach);
PRECACHE_WEAPON_REGISTER(weapon_eattach);


//---------------------------------------------------------
void CWeaponEAttach::Precache()
{
#ifndef CLIENT_DLL
    UTIL_PrecacheOther("npc_manhack");
    UTIL_PrecacheOther("npc_cscanner");
#endif
    BaseClass::Precache();
}


//---------------------------------------------------------
void CWeaponEAttach::PrimaryAttack()
{
#ifndef CLIENT_DLL
    AttachNPC("npc_manhack");
#endif
    WeaponSound(SINGLE);
    SendWeaponAnim(ACT_VM_PRIMARYATTACK);
    m_flNextPrimaryAttack = gpGlobals->curtime + 0.4f;
}


//---------------------------------------------------------
void CWeaponEAttach::SecondaryAttack()
{
#ifndef CLIENT_DLL
    AttachNPC("npc_cscanner");
#endif
    WeaponSound(SINGLE);
    SendWeaponAnim(ACT_VM_PRIMARYATTACK);
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.4f;
}


//---------------------------------------------------------
void CWeaponEAttach::AttachNPC(const char* pClassname)
{
#ifndef CLIENT_DLL

    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
        return;

    trace_t tr;
    Vector vecStart = pPlayer->Weapon_ShootPosition();
    Vector vecEnd = vecStart + pPlayer->EyeDirection3D() * 2000.0f;

    UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

    if (!tr.DidHit() || !tr.m_pEnt)
        return;

    CBaseEntity* pTarget = tr.m_pEnt;

    CBaseEntity* pNPC = CreateEntityByName(pClassname);
    if (!pNPC)
        return;

    pNPC->SetAbsOrigin(tr.endpos);
    pNPC->SetAbsAngles(QAngle(0, 0, 0));
    DispatchSpawn(pNPC);

    // Ensure both have physics
    IPhysicsObject* pPhysNPC = pNPC->VPhysicsGetObject();
    if (!pPhysNPC)
        return;

    IPhysicsObject* pPhysTarget = pTarget->VPhysicsGetObject();

    // If target has no physics (like world), use world physics
    if (!pPhysTarget)
        pPhysTarget = g_PhysWorldObject;

    CreateFixedConstraint(pNPC, pTarget, tr.endpos);

    g_pEffects->Smoke(tr.endpos, 0, 10.0f, 25.0f);
    g_pEffects->Sparks(tr.endpos);
#endif
}


//---------------------------------------------------------
void CWeaponEAttach::CreateFixedConstraint(CBaseEntity* pA, CBaseEntity* pB, const Vector& attachPos)
{
#ifndef CLIENT_DLL

    if (!pA)
        return;

    IPhysicsObject* pPhysA = pA->VPhysicsGetObject();
    IPhysicsObject* pPhysB = pB->VPhysicsGetObject();

    if (!pPhysA)
        return;

    if (!pPhysB)
        pPhysB = g_PhysWorldObject;

    constraint_fixedparams_t fixed;
    fixed.Defaults();

    fixed.InitWithCurrentObjectState(pPhysA, pPhysB);

    physenv->CreateFixedConstraint(pPhysA, pPhysB, nullptr, fixed);

#endif
}
