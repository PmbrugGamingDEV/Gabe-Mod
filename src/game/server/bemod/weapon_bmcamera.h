#ifndef WEAPON_CAMERA_H
#define WEAPON_CAMERA_H

#ifdef _WIN32
#pragma once
#endif

#include "basehlcombatweapon.h"
#include "hl2_player.h"
#include <string>

class CWeaponBMCamera : public CBaseHLCombatWeapon
{
public:
    DECLARE_CLASS(CWeaponBMCamera, CBaseHLCombatWeapon);

    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

    void Precache(void) override;
    void PrimaryAttack(void) override;

private:
    void TakeScreenshot(void);
    std::string GenerateRandomString(size_t length);
};

#endif // WEAPON_CAMERA_H
