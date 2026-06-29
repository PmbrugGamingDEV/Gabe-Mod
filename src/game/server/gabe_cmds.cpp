#include "cbase.h"
#include "baseentity.h"
#include "baseanimating.h"

ConVar gabe_glowcolor(
	"gabe_glow_color",
	"1.0 1.0 1.0",
	FCVAR_ARCHIVE | FCVAR_NOTIFY,
	"Glow color as 'R G B', each component from 0 to 1."
);

CON_COMMAND(gabe_glowadd, "Add a halo effect to an entity")
{
    trace_t tr;

    CBasePlayer* pPlayer = UTIL_GetCommandClient();

	if (!pPlayer)
		return;

    Vector vecStart = pPlayer->EyePosition();

    Vector vecForward;
    AngleVectors(pPlayer->EyeAngles(), &vecForward);

    Vector vecEnd = vecStart + vecForward * MAX_TRACE_LENGTH;

    UTIL_TraceLine(
        vecStart,
        vecEnd,
        MASK_SHOT,
        pPlayer,
        COLLISION_GROUP_NONE,
        &tr
    );

    CBaseEntity* pObject = tr.m_pEnt;
    if (pObject)
    {

        CBaseAnimating* pAnim = pObject->GetBaseAnimating();
		if (pAnim)
		{
			float r = 1.0f, g = 1.0f, b = 1.0f;

			sscanf(
				gabe_glowcolor.GetString(),
				"%f %f %f",
				&r,
				&g,
				&b
			);

			r = clamp(r, 0.0f, 1.0f);
			g = clamp(g, 0.0f, 1.0f);
			b = clamp(b, 0.0f, 1.0f);

			pAnim->SetGlowEffectColor(r, g, b);
			pAnim->AddGlowEffect();
		}
    }
}

CON_COMMAND(gabe_glowremove, "Remove glow effect from trace ent")
{
	trace_t tr;
	CBasePlayer* pPlayer = UTIL_GetCommandClient();

	if (!pPlayer)
		return;

	Vector vecStart = pPlayer->EyePosition();
	Vector vecForward;
	AngleVectors(pPlayer->EyeAngles(), &vecForward);
	Vector vecEnd = vecStart + vecForward * MAX_TRACE_LENGTH;
	UTIL_TraceLine(
		vecStart,
		vecEnd,
		MASK_SHOT,
		pPlayer,
		COLLISION_GROUP_NONE,
		&tr
	);
	CBaseEntity* pObject = tr.m_pEnt;
	if (pObject)
	{
		CBaseAnimating* pAnim = pObject->GetBaseAnimating();
		if (pAnim)
		{
			pAnim->RemoveGlowEffect();
		}
	}
}