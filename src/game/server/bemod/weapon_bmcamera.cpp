#include "cbase.h"
#include "weapon_bmcamera.h"
#include "hl2_player.h"
#include "util.h"
#include "usermessages.h"
#include <random>
#include <string>

LINK_ENTITY_TO_CLASS(weapon_bmcamera, CWeaponBMCamera);

PRECACHE_WEAPON_REGISTER(weapon_bmcamera);

IMPLEMENT_SERVERCLASS_ST(CWeaponBMCamera, DT_WeaponBMCamera)
END_SEND_TABLE()

BEGIN_DATADESC(CWeaponBMCamera)
END_DATADESC()

void CWeaponBMCamera::Precache(void)
{
    PrecacheModel("models/weapons/v_camera.mdl");
    PrecacheModel("models/weapons/w_camera.mdl");
    PrecacheScriptSound("Weapon_PhysCannon.Launch");

    BaseClass::Precache();
}

void CWeaponBMCamera::PrimaryAttack(void)
{
    TakeScreenshot();

    m_flNextPrimaryAttack = gpGlobals->curtime + 1.0f;
}

void CWeaponBMCamera::TakeScreenshot(void)
{
    CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
    if (!pPlayer || !pPlayer->IsPlayer())
        return;

    std::string randomString = GenerateRandomString(10);
    std::string screenshotFilename = "screenshots/" + randomString + ".tga";
    std::string screenshotname = randomString;
    std::string screenshotCommand = "screenshot " + screenshotname;

    engine->ClientCommand(pPlayer->edict(), screenshotCommand.c_str());

    // Construct the message with the filename
    std::string message = "Screenshot saved: " + screenshotFilename;

    // Print the message to the player's screen
    ClientPrint(pPlayer, 3, message.c_str());

    EmitSound("Weapon_PhysCannon.Launch");
}

std::string CWeaponBMCamera::GenerateRandomString(size_t length)
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
