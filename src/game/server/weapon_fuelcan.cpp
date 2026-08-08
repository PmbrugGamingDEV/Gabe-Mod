#include "cbase.h"
#include "basecombatweapon.h"
#include "vehicle_base.h"
#include "debugoverlay_shared.h"
#include "ieffects.h"
#include "beam_shared.h"
#include "Sprite.h"

#define PHYSGUN_BEAM_SPRITE1	"sprites/strider_bluebeam.vmt"
#define PHYSGUN_BEAM_SPRITE		"sprites/bluebeam.vmt"
#define PHYSGUN_BEAM_GLOW		"sprites/physglow.vmt"

class CWeaponFuelCan : public CBaseCombatWeapon
{
public:
    DECLARE_CLASS(CWeaponFuelCan, CBaseCombatWeapon);
    DECLARE_DATADESC();
    DECLARE_SERVERCLASS();

    virtual void Precache() override;
    void PrimaryAttack() override;
    virtual void ItemPostFrame() override;
};

LINK_ENTITY_TO_CLASS(weapon_fuelcan, CWeaponFuelCan);
PRECACHE_WEAPON_REGISTER(weapon_fuelcan);

BEGIN_DATADESC(CWeaponFuelCan)
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CWeaponFuelCan, DT_WeaponFuelCan)
END_SEND_TABLE()

void CWeaponFuelCan::Precache()
{
    PrecacheScriptSound("Weapon_Pistol.Empty");
    PrecacheScriptSound("Buttons.snd37");
    PrecacheModel(PHYSGUN_BEAM_SPRITE);
    PrecacheModel(PHYSGUN_BEAM_GLOW);

    BaseClass::Precache();
}

void CWeaponFuelCan::PrimaryAttack()
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

    if (!pPlayer)
        return;

    Vector vecSrc = pPlayer->Weapon_ShootPosition();

    Vector vecForward;
    pPlayer->EyeVectors(&vecForward);

    trace_t tr;

    UTIL_TraceLine(
        vecSrc,
        vecSrc + vecForward * 128.0f,
        MASK_SOLID,
        pPlayer,
        COLLISION_GROUP_NONE,
        &tr
    );

    CPropVehicleDriveable* pVehicle =
        dynamic_cast<CPropVehicleDriveable*>(tr.m_pEnt);

    if (pVehicle)
    {
        float flOldFuel = pVehicle->GetFuelAmt();

        pVehicle->AddFuel(1.0f);

        if (pVehicle->GetFuelAmt() > flOldFuel)
        {
            SendWeaponAnim(ACT_VM_PRIMARYATTACK);

            pPlayer->SetAnimation(PLAYER_ATTACK1);

            EmitSound("Buttons.snd37");

            CBeam* h_Beam = CBeam::BeamCreate(PHYSGUN_BEAM_SPRITE, 13.0f);

            if (h_Beam)
            {
                h_Beam->EntsInit(this, pVehicle);

                h_Beam->SetStartAttachment(1);

                h_Beam->SetEndAttachment(0);

                h_Beam->SetBrightness(255);

                h_Beam->SetColor(0, 0, 255);

                h_Beam->LiveForTime(0.1f);

                h_Beam->RelinkBeam();
            }

            CSprite* pGlow = CSprite::SpriteCreate(PHYSGUN_BEAM_GLOW, pVehicle->GetAbsOrigin(), true);

            if (pGlow)
            {
                pGlow->SetAttachment(pVehicle, 0);
                pGlow->SetTransparency(kRenderGlow, 0, 0, 255, 255, kRenderFxNoDissipation);
                pGlow->SetScale(3.0f);
                pGlow->SetBrightness(255);
                pGlow->TurnOn();

                pGlow->FadeAndDie(0.1f);
            }

            g_pEffects->Sparks(pVehicle->GetAbsOrigin());

            m_flNextPrimaryAttack = gpGlobals->curtime + 0.01f;
        }
    }
    else
    {
        SendWeaponAnim(ACT_VM_PRIMARYATTACK);

        EmitSound("Weapon_Pistol.Empty");

        m_flNextPrimaryAttack = gpGlobals->curtime + 0.25f;
    }
}

void CWeaponFuelCan::ItemPostFrame()
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

    if (pPlayer)
    {
        char szText[256];

        V_strncpy(
            szText,
            "Fuel Can\n\n"
            "Primary Fire: Refuel vehicle\n"
            "Aim at a vehicle and hold/use primary fire.",
            sizeof(szText)
        );

        Vector vecSrc = pPlayer->Weapon_ShootPosition();

        Vector vecForward;
        pPlayer->EyeVectors(&vecForward);

        trace_t tr;

        UTIL_TraceLine(
            vecSrc,
            vecSrc + vecForward * 128.0f,
            MASK_SOLID,
            pPlayer,
            COLLISION_GROUP_NONE,
            &tr
        );

        CPropVehicleDriveable* pVehicle =
            dynamic_cast<CPropVehicleDriveable*>(tr.m_pEnt);

        if (pVehicle)
        {
            V_snprintf(
                szText,
                sizeof(szText),
                "Fuel Can\n\n"
                "Primary Fire: Refuel vehicle\n"
                "Vehicle Fuel: %.1f%%",
                pVehicle->GetFuelAmt()
            );
        }

        NDebugOverlay::ScreenText(
            0.02f,
            0.02f,
            szText,
            255,
            255,
            255,
            255,
            0.0f
        );
    }

    BaseClass::ItemPostFrame();
}
