#include "cbase.h"
#include "props.h"
#include "physics.h"

static void SpawnExplosiveBarrelAt(const Vector& pos)
{
	CBaseEntity* pBarrel = CreateEntityByName("prop_physics");

	if (!pBarrel)
		return;

	pBarrel->SetAbsOrigin(pos);

	pBarrel->KeyValue("model", "models/props_c17/oildrum001_explosive.mdl");

	DispatchSpawn(pBarrel);

	if (pBarrel->IsMarkedForDeletion())
		return;

	pBarrel->Activate();
}

CON_COMMAND_F(
	litter_barrels,
	"Litter the map with explosive barrels",
	FCVAR_CHEAT)
{
	const int MAX_BARRELS = 80;
	const int MAX_ATTEMPTS = 600;

	int spawned = 0;

	// Rough world-space bounds.
	// Increase these for huge maps.
	const float flRange = 4096.0f;

	for (int i = 0; i < MAX_ATTEMPTS && spawned < MAX_BARRELS; ++i)
	{
		Vector start(
			RandomFloat(-flRange, flRange),
			RandomFloat(-flRange, flRange),
			4096.0f
		);

		Vector end = start;
		end.z = -4096.0f;

		trace_t tr;

		UTIL_TraceLine(
			start,
			end,
			MASK_SOLID,
			NULL,
			COLLISION_GROUP_NONE,
			&tr
		);

		if (tr.fraction >= 1.0f)
			continue;

		if (tr.startsolid)
			continue;

		Vector spawnPos =
			tr.endpos +
			tr.plane.normal * 24.0f;

		SpawnExplosiveBarrelAt(spawnPos);

		++spawned;
	}

	Msg(
		"Spawned %i explosive barrels.\n",
		spawned
	);
}