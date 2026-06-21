// sticky_pellet.cpp
#include "cbase.h"
#include "sticky_pellet.h"
#include "explode.h"
#include "entitylist.h"
#include "baseanimating.h"
#include "te_effect_dispatch.h"
#include "soundent.h"
#include "shake.h"
#include "util.h"
#include "physics.h"
#include "vphysics_interface.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"

LINK_ENTITY_TO_CLASS(sticky_pellet, CStickyPellet);

BEGIN_DATADESC(CStickyPellet)
	DEFINE_THINKFUNC(Think),
END_DATADESC()

void CStickyPellet::Spawn()
{
	SetModel("models/weapons/w_bugbait.mdl");
	SetMoveType(MOVETYPE_FLYGRAVITY);
	SetSolid(SOLID_BBOX);
	SetSize(Vector(-4, -4, -4), Vector(4, 4, 4));
	SetRenderColor(255, 0, 0);

	SetThink(&CStickyPellet::Think);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CStickyPellet::StickToSurface(trace_t& tr)
{
	if (m_bStuck)
		return;

	m_bStuck = true;

	// Stop movement
	SetMoveType(MOVETYPE_NONE);
	SetAbsVelocity(vec3_origin);

	// Disable collisions so we don't retrigger Touch
	SetSolid(SOLID_NONE);

	m_hStuckTo = tr.m_pEnt;

	if (m_hStuckTo)
	{
		// Parent to the entity we hit
		SetParent(m_hStuckTo);

		matrix3x4_t worldToParent;

		MatrixInvert(m_hStuckTo->EntityToWorldTransform(), worldToParent);

		// Store local offset
		VectorITransform(GetAbsOrigin(), worldToParent, m_vOffset);

		// Store orientation
		m_qAngleOffset = GetAbsAngles();
	}
}

void CStickyPellet::Think()
{
	if (m_bStuck && m_hStuckTo)
	{
		matrix3x4_t mat = m_hStuckTo->EntityToWorldTransform();
		Vector worldPos;
		VectorTransform(m_vOffset, mat, worldPos);
		SetAbsOrigin(worldPos);
		SetAbsAngles(m_qAngleOffset);
	}
	SetNextThink(gpGlobals->curtime + 0.05f);
}

void CStickyPellet::Detonate()
{
	// FIXED: Implementation is in the weapon's code
}
