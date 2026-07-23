#ifndef WEAPON_PROPLAUNCHER_H
#define WEAPON_PROPLAUNCHER_H

#include "basecombatweapon_shared.h"
#include "cbase.h"
#include "vphysics/constraints.h"
#include "physics_prop_ragdoll.h"

enum PropLauncherMode
{
    MODE_LAUNCH = 0,
    MODE_WELD,
    MODE_PHYSGUN
};

class CWeaponPropLauncher : public CBaseCombatWeapon
{
    DECLARE_CLASS(CWeaponPropLauncher, CBaseCombatWeapon);
public:
    CWeaponPropLauncher();

    void Precache(void);
    void PrimaryAttack(void);
    void SecondaryAttack(void);
    bool Deploy(void);
    void ItemPostFrame(void);
    bool Reload(void);
    void SwitchMode(void);

    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

private:
    void LaunchProp(const char* modelName);
    void CycleProps();
    void WeldEntities(void);
    void PhysgunPrimary(void);
    void PhysgunSecondary(void);
    void FreezeProp(void);
    void HandleKeyPresses(void);

    int m_iCurrentPropIndex;
    CUtlVector<const char*> m_PropModels;
    PropLauncherMode m_iCurrentMode;
    EHANDLE m_hFirstEntity;
    Vector m_vecLastHitPos;
    bool m_bFirstPointSet;
    float m_flNextModeSwitchTime;
    EHANDLE m_hPhysgunTarget;
    IPhysicsObject* m_pPhysgunTargetObject;
    IPhysicsConstraint* m_pPhysgunConstraint;
    bool m_bHoldingRotateKey;
};

#endif // WEAPON_PROPLAUNCHER_H
