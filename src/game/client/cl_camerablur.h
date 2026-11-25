#pragma once

#include "cbase.h"
#include "basehlcombatweapon_shared.h"

class CViewSetup;

void CamBlur_Init();
void CamBlur_OnOverrideView(CViewSetup* pSetup);
void CamBlur_PostRender();
void CamBlur_Cycle();

class C_WeaponCamera : public C_BaseHLCombatWeapon
{
    DECLARE_CLASS(C_WeaponCamera, C_BaseHLCombatWeapon);

public:
    DECLARE_PREDICTABLE();
    DECLARE_CLIENTCLASS();

    C_WeaponCamera()
    {
        m_bViewLocked = false;
        m_bLocalViewLocked = false;
        m_angLockedView.Init();
        m_flRollAngle = 0.0f;
    }

    virtual bool ShouldDrawViewModel() { return false; }
    virtual bool ShouldDraw() { return false; }
    virtual bool IsPredicted() const { return true; }
    virtual bool OverrideViewAngles(void) override;
    float GetCameraRoll() const;

private:
    bool m_bViewLocked;
    bool m_bAttack2Held;
    bool m_bLocalViewLocked;
    QAngle m_angLockedView;
    float m_flRollAngle;

    C_WeaponCamera(const C_WeaponCamera&);
};