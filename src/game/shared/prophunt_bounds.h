#ifndef PROPHUNT_BOUNDS_H
#define PROPHUNT_BOUNDS_H

#include "engine/ivmodelinfo.h"

#define PROPHUNT_MODEL "models/props_junk/watermelon01.mdl"

// Model identity is already networked, so both movement DLLs can use it.
inline bool PropHuntBounds(const model_t *model, Vector &mins, Vector &maxs)
{
    if (!model || Q_stricmp(modelinfo->GetModelName(model), PROPHUNT_MODEL))
        return false;

    modelinfo->GetModelBounds(model, mins, maxs);
    // Keep the horizontal hull independent of the player's yaw.
    float x = MAX(fabsf(mins.x), fabsf(maxs.x));
    float y = MAX(fabsf(mins.y), fabsf(maxs.y));
    float radius = MAX(2.0f, sqrtf(x*x + y*y));
    mins.x = mins.y = -radius;
    maxs.x = maxs.y = radius;
    return maxs.z > mins.z;
}

#endif
