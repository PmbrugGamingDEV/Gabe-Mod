#pragma once
#ifndef WEAPON_CAMERA_H
#define WEAPON_CAMERA_H

#ifdef _WIN32
#pragma once
#endif

#include "basehlcombatweapon.h"
#include "hl2mp_player.h"
#include <string>

class CWeaponCamera : public CBaseHLCombatWeapon
{
public:
    DECLARE_CLASS(CWeaponCamera, CBaseHLCombatWeapon);

    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

    CWeaponCamera();
    ~CWeaponCamera() {}

    void Precache(void) override;
    virtual bool Deploy(void) override;
    virtual bool Holster(CBaseCombatWeapon* pSwitchingTo);
    void PrimaryAttack(void) override;
    void SecondaryAttack(void) override;
private:
    void ItemPostFrame();
    void TakeScreenshot(void);

    std::string GenerateRandomString(size_t length);
    int m_iBlurMode;
};

#endif // WEAPON_CAMERA_H
