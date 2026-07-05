#include "cbase.h"
#include "filesystem.h"
#include "soundent.h"
#include "smoke_trail.h"

void CC_PlayerFart(const CCommand& args)
{
    CBasePlayer* pPlayer = UTIL_GetCommandClient();
    if (!pPlayer)
        return;

    const char* pszSound = "fart.wav";

    char cmd[256];
    Q_snprintf(cmd, sizeof(cmd), "play %s", pszSound);
    engine->ClientCommand(pPlayer->edict(), cmd);

    Vector origin = pPlayer->GetAbsOrigin();

    SmokeTrail* pSmoke = SmokeTrail::CreateSmokeTrail();
    if (pSmoke)
    {
        pSmoke->m_SpawnRate = 256;
        pSmoke->m_ParticleLifetime = 4.0f;
        pSmoke->m_StartSize = 64;
        pSmoke->m_EndSize = 256;
        pSmoke->m_SpawnRadius = 32;
        pSmoke->m_MinSpeed = 10;
        pSmoke->m_MaxSpeed = 60;

        // toxic
        pSmoke->m_StartColor.Init(0.2f, 1.0f, 0.2f);
        pSmoke->m_EndColor.Init(0.05f, 0.4f, 0.05f);

        pSmoke->SetAbsOrigin(origin + Vector(0, 0, 32));
        pSmoke->SetLifetime(1.5f);
    }

    CTakeDamageInfo dmgInfo(pPlayer, pPlayer, 200.0f, DMG_BLAST);

    RadiusDamage(
        dmgInfo,
        origin,
        256.0f,
        CLASS_NONE,
        NULL
    );
    Vector forward, up;
    pPlayer->EyeVectors(&forward, NULL, &up);

    pPlayer->ApplyAbsVelocityImpulse(forward * 200.0f + up * 300.0f);

    // ☠️
    CTakeDamageInfo selfDmg(pPlayer, pPlayer, 1000.0f, DMG_DISSOLVE);
    pPlayer->TakeDamage(selfDmg);
}

ConCommand fart("fart", CC_PlayerFart, "HUGE fart that vaporizes the player", FCVAR_NONE);