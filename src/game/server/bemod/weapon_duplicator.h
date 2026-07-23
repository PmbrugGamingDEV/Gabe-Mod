#pragma once

#include "basehlcombatweapon.h"

class CWeaponDuplicator : public CBaseHLCombatWeapon
{
public:
    DECLARE_CLASS(CWeaponDuplicator, CBaseHLCombatWeapon);
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

    CWeaponDuplicator();

    void Precache(void);
    void PrimaryAttack(void); // Left-click: Duplicate the entity
    void SecondaryAttack(void); // Right-click: Copy the entity data
    void CopyEntityData(CBaseEntity* pEnt);
    void DuplicateEntity(CBasePlayer* pPlayer);

private:
    string_t m_CopiedClassname;
    Vector m_CopiedOrigin;
    QAngle m_CopiedAngles;
    string_t m_CopiedModelName;
};
