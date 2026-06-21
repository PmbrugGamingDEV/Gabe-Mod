//========= Copyright Weapons Expanded, All rights reserved. ============//
//
// Purpose: A camera from another project I was working on, taken from there.
// NOTE: It was BEMod 2.0.5.
//
//=============================================================================

#include "cbase.h"
#include "weapon_camera.h"
#include "hl2mp_player.h"
#include "util.h"
#include "usermessages.h"
#include <random>
#include <string>
#include <in_buttons.h>
#include"fmtstr.h"

LINK_ENTITY_TO_CLASS(weapon_camera, CWeaponCamera);

PRECACHE_WEAPON_REGISTER(weapon_camera);

IMPLEMENT_SERVERCLASS_ST(CWeaponCamera, DT_WeaponCamera)
END_SEND_TABLE()

BEGIN_DATADESC(CWeaponCamera)
END_DATADESC()

CWeaponCamera::CWeaponCamera()
{
    m_iBlurMode = 2; // start at no blur
}

void CWeaponCamera::Precache(void)
{
    PrecacheScriptSound("NPC_CScanner.TakePhoto");

    BaseClass::Precache();
}

bool CWeaponCamera::Deploy(void)
{
    bool bResult = BaseClass::Deploy();

    if (bResult)
    {
        CBasePlayer* pOwner = ToBasePlayer(GetOwner());
        if (pOwner)
        {
            pOwner->ShowViewModel(false);
        }
    }

    return bResult;
}

bool CWeaponCamera::Holster(CBaseCombatWeapon* pSwitchingTo)
{
    bool bResult = BaseClass::Holster(pSwitchingTo);

    if (bResult)
    {
        CBasePlayer* pOwner = ToBasePlayer(GetOwner());
        if (pOwner)
        {
            pOwner->ShowViewModel(true);
        }
    }

    return bResult;
}

void CWeaponCamera::PrimaryAttack(void)
{
    TakeScreenshot();

    m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;
}
void CWeaponCamera::SecondaryAttack()
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
        return;

    float targetFOV = 30.0f;  // Desired zoom level
    float maxFOV = 140.0f;    // FOV reset threshold
    float zoomSpeed = 50.0f;  // Speed at which the zoom is applied
    float currentFOV = pPlayer->GetFOV();

    // Gradually decrease the FOV to the target FOV
    if (currentFOV > targetFOV)
    {
        currentFOV -= zoomSpeed * gpGlobals->frametime;  // Apply zoom over time
        if (currentFOV < targetFOV)
        {
            currentFOV = targetFOV;  // Clamp to the target FOV
        }
        pPlayer->SetFOV(this, currentFOV);

        // Print the FOV change to the client
        ClientPrint(pPlayer, HUD_PRINTCENTER, CFmtStr("FOV: %.1f", currentFOV).Access());
    }

    // Check if FOV reaches or exceeds 140, and reset if so
    if (currentFOV >= maxFOV)
    {
        currentFOV = 90.0f;  // Reset to default FOV
        pPlayer->SetFOV(this, currentFOV);

        // Print the FOV reset to the client
        ClientPrint(pPlayer, HUD_PRINTTALK, "FOV reset to default.");
    }

    // Rotate the camera in a circular motion
    QAngle viewAngles = pPlayer->GetLocalAngles();
    CUserCmd* cmd = pPlayer->GetCurrentCommand();  // Get the current user command
    if (cmd)
    {
        float angleIncrementX = cmd->mousedy * 0.02f;  // Adjust this value for smoothness
        float angleIncrementY = cmd->mousedx * 0.02f;  // Adjust this value for smoothness

        // Rotate around the Yaw axis (left/right) and Pitch axis (up/down)
        viewAngles.y += angleIncrementY;
        viewAngles.x = cos(angleIncrementY) * viewAngles.x + sin(angleIncrementX) * 5.0f; // Small circular adjustment
    }
    pPlayer->SnapEyeAngles(viewAngles);

    m_flNextSecondaryAttack = gpGlobals->curtime + 0.01f;  // Continuous update with a small delay
}

static const char* g_pszCamFXNames[] =
{
    "Calm",
    "Soft Smear",
    "Double Vision",
    "Screen Tear",
    "Pulse Warp",
    "Heavy Blur",
    "Spiral Drift",
    "Horizontal Collapse",
    "Vertical Collapse",
    "Shake Distortion",
    "TV Static",
    "Too Much Beer",
    "Reality Melting"
};

static const int MAX_CAMFX_MODES = 13;

void CWeaponCamera::ItemPostFrame()
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer)
        return;

    // Force-hide the viewmodel every frame
    pPlayer->ShowViewModel(false);

    if (!(pPlayer->m_nButtons & IN_ATTACK2))  // If the right mouse button is not pressed
    {
        // Reset the FOV to the default value (90 degrees) if R is pressed
        if (pPlayer->m_afButtonPressed & IN_RELOAD)  // Pressing R
        {
            float defaultFOV = 90.0f;
            pPlayer->SetFOV(this, defaultFOV);
        }
    }

    if (pPlayer->m_afButtonPressed & IN_USE)
    {
        m_iBlurMode++;

        static const int MAX_CAMFX_MODES = 13;

        if (m_iBlurMode >= MAX_CAMFX_MODES)
            m_iBlurMode = 0;

        CSingleUserRecipientFilter filter(pPlayer);
        filter.MakeReliable();

        UserMessageBegin(filter, "CamBlur");
        WRITE_BYTE(m_iBlurMode);
        MessageEnd();
    }

    BaseClass::ItemPostFrame();
}


void CWeaponCamera::TakeScreenshot(void)
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer || !pPlayer->IsPlayer())
        return;

    std::string randomString = GenerateRandomString(10);
    std::string screenshotFilename = "screenshots/" + randomString + ".jpg";
    std::string screenshotname = randomString;
    std::string screenshotCommand = "jpeg " + screenshotname;

    engine->ClientCommand(pPlayer->edict(), screenshotCommand.c_str());

    // Construct the message with the filename
    std::string message = "Screenshot saved: " + screenshotFilename;

    // Print the message to the player's screen
    ClientPrint(pPlayer, HUD_PRINTCENTER, message.c_str());

    EmitSound("NPC_CScanner.TakePhoto");
}

std::string CWeaponCamera::GenerateRandomString(size_t length)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t max_index = (sizeof(charset) - 1);
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<size_t> distribution(0, max_index);

    std::string randomString;
    for (size_t i = 0; i < length; ++i)
    {
        randomString += charset[distribution(generator)];
    }

    return randomString;
}
