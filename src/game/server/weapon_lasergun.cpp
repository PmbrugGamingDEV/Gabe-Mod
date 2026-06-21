#include "cbase.h"
#include "weapon_lasergun.h"
#include "in_buttons.h"
#include "decals.h"
#include "te_effect_dispatch.h"
#include "ieffects.h"
#include "beam_shared.h"
#include "GABE.H"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponLaserGun, DT_WeaponLaserGun)

BEGIN_NETWORK_TABLE(CWeaponLaserGun, DT_WeaponLaserGun)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponLaserGun)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_lasergun, CWeaponLaserGun);
PRECACHE_WEAPON_REGISTER(weapon_lasergun);

CWeaponLaserGun::CWeaponLaserGun()
{
    m_fMinRange1 = 0;
    m_fMaxRange1 = 8192;
    m_bLaserActive = false;
    m_pLaserLoop = NULL;
}

void CWeaponLaserGun::Precache()
{
    BaseClass::Precache();
    PrecacheSound("npc/stalker/go_alert2a.wav");
    PrecacheSound("buttons/button24.wav");
    PrecacheModel("materials/decals/fadingscorch.vmt");
    PrecacheSound("npc/stalker/laser_burn.wav");
    PrecacheSound("npc/stalker/laser_flesh.wav");
}

void CWeaponLaserGun::PrimaryAttack()
{
    FireLaser();

    m_flNextPrimaryAttack = gpGlobals->curtime + 0.01f;
}

void CWeaponLaserGun::FireLaser()
{
    CBasePlayer* pPlayer = GabeMod::GetPlayer();
    if (!pPlayer)
        return;

    Vector vecSrc = pPlayer->Weapon_ShootPosition();

    Vector vecDir;
    pPlayer->EyeVectors(&vecDir);

    Vector vecEnd = vecSrc + (vecDir * 8192);

    trace_t tr;
    UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

    // -------- DAMAGE --------
    if (tr.m_pEnt)
    {
        CTakeDamageInfo dmg(this, pPlayer, 35, DMG_ENERGYBEAM);
        CalculateBulletDamageForce(&dmg, DMG_ENERGYBEAM, vecDir, tr.endpos);
        tr.m_pEnt->DispatchTraceAttack(dmg, vecDir, &tr);
        ApplyMultiDamage();
    }

    CBeam* pBeam = CBeam::BeamCreate("sprites/laserbeam.vmt", 1.0f);
    if (pBeam)
    {
        pBeam->PointEntInit(tr.endpos, this); // attach to weapon
        pBeam->SetEndAttachment(1);           // attachment 1 (muzzle)
        pBeam->SetBrightness(255);
        pBeam->SetColor(0, 255, 0);
        pBeam->RelinkBeam();
        pBeam->LiveForTime(0.01f);
    }

    // -------- SPARKS --------
    if (tr.DidHit())
    {
        g_pEffects->Sparks(tr.endpos);

        // -------- DECAL --------
        UTIL_DecalTrace(&tr, "RedGlowFade");
    }
}

bool CWeaponLaserGun::Holster(CBaseCombatWeapon* pSwitchingTo)
{
#ifndef CLIENT_DLL
    if (m_pLaserLoop)
    {
        CSoundEnvelopeController::GetController().
            SoundDestroy(m_pLaserLoop);
        m_pLaserLoop = NULL;
    }
#endif

    m_bLaserActive = false;

    return BaseClass::Holster(pSwitchingTo);
}

void CWeaponLaserGun::ItemPostFrame()
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
    {
        BaseClass::ItemPostFrame();
        return;
    }

#ifndef CLIENT_DLL

    // ============================================
    // PRIMARY FIRE HOLD LOGIC (LASER MODE ONLY)
    // ============================================

    bool bHoldingPrimary = (pPlayer->m_nButtons & IN_ATTACK) != 0;

    if (bHoldingPrimary && !m_bLaserActive)
    {
        m_bLaserActive = true;

        EmitSound("buttons/button24.wav");

        CPASAttenuationFilter filter(this);
        m_pLaserLoop =
            CSoundEnvelopeController::GetController().
            SoundCreate(filter, entindex(), CHAN_STATIC,
                "npc/stalker/laser_burn.wav", ATTN_NORM);

        CSoundEnvelopeController::GetController().
            Play(m_pLaserLoop, 1.0f, 100);
    }
    else if (!bHoldingPrimary && m_bLaserActive)
    {
        m_bLaserActive = false;

        if (m_pLaserLoop)
        {
            CSoundEnvelopeController::GetController().
                SoundDestroy(m_pLaserLoop);
            m_pLaserLoop = NULL;
        }
    }

    // ============================================
    // RIGHT CLICK — VIEW PUNCH + SCREAM
    // ============================================

    if (pPlayer->m_afButtonPressed & IN_ATTACK2)
    {
        QAngle punch;
        punch.x = -45.0f;
        punch.y = random->RandomFloat(-5.0f, 5.0f);
        punch.z = 0.0f;

        pPlayer->ViewPunch(punch);

        EmitSound("npc/stalker/go_alert2a.wav");

        m_flNextSecondaryAttack = gpGlobals->curtime + 1.5f;
    }

#endif

    BaseClass::ItemPostFrame();
}