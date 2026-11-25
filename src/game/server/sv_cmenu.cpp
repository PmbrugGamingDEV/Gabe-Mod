// GabeMod context menu - server DLL, Source SDK 2013 MP
#include "cbase.h"
#include "player.h"
#include "util.h"
#include "vphysics_interface.h"
#include "gabe_undo.h"
#include "baseanimating.h"
#include "tier0/memdbgon.h"

// Default: listen-server host or server admin only. Enable for trusted sandbox players.
static ConVar gabe_context_allowall("gabe_context_allowall", "0", FCVAR_ARCHIVE, "Allow all players to edit nearby physics props with the context menu.");

static bool ContextTargetAllowed(CBasePlayer* player, CBaseEntity* e)
{
    if (!player || !e || e->IsWorld() || e->IsPlayer() || e->IsNPC() || e->IsMarkedForDeletion()) return false;
    if (!gabe_context_allowall.GetBool() && !UTIL_IsCommandIssuedByServerAdmin()) return false;
    // Restrict mutation to physics props and ragdolls, not map logic/doors.
    if (!FClassnameIs(e, "prop_physics") && !FClassnameIs(e, "prop_physics_multiplayer") && !FClassnameIs(e, "prop_ragdoll")) return false;
    if ((e->WorldSpaceCenter() - player->EyePosition()).LengthSqr() > 2048.0f * 2048.0f) return false;
    trace_t tr;
    UTIL_TraceLine(player->EyePosition(), e->WorldSpaceCenter(), MASK_SHOT, player, COLLISION_GROUP_NONE, &tr);
    return tr.m_pEnt == e || !tr.DidHit();
}
CON_COMMAND(gabe_context_action, "Internal context menu property command")
{
    CBasePlayer* player = UTIL_GetCommandClient();
    if (!player || args.ArgC() != 3) return;
    int index = atoi(args[1]);
    if (index <= gpGlobals->maxClients || index >= gpGlobals->maxEntities) return;
    CBaseEntity* e = UTIL_EntityByIndex(index);
    if (!ContextTargetAllowed(player, e))
    {
        ClientPrint(player, HUD_PRINTCENTER, "Context menu: target not permitted.");
        return;
    }
    const char* action = args[2];
    if (!Q_stricmp(action, "undo")) { GabeUndo_Last(player); return; }
    if (!Q_stricmp(action, "inspect"))
    {
        IPhysicsObject* phys = e->VPhysicsGetObject();
        char info[256]; Q_snprintf(info, sizeof(info), "%s | #%d | Mass: %.1f kg | Health: %d", e->GetClassname(), index, phys ? phys->GetMass() : 0.0f, e->GetHealth());
        ClientPrint(player, HUD_PRINTCONSOLE, info);
        Vector pos = e->GetAbsOrigin(); QAngle ang = e->GetAbsAngles();
        Q_snprintf(info, sizeof(info), "Model: %s | Origin: %.1f %.1f %.1f | Angles: %.1f %.1f %.1f\n", STRING(e->GetModelName()), pos.x, pos.y, pos.z, ang.x, ang.y, ang.z);
        ClientPrint(player, HUD_PRINTCONSOLE, info);
        ClientPrint(player, HUD_PRINTCENTER, info);
        return;
    }
    if (!Q_stricmp(action, "remove")) { UTIL_Remove(e); return; }
    if (!Q_stricmp(action, "extinguish")) { CBaseAnimating* anim = e->GetBaseAnimating(); if (anim) anim->Extinguish(); return; }
    if (!Q_stricmp(action, "white")) { e->SetRenderColor(255, 255, 255); return; }
    if (!Q_stricmp(action, "red")) { e->SetRenderColor(255, 0, 0); return; }
    if (!Q_stricmp(action, "green")) { e->SetRenderColor(0, 255, 0); return; }
    if (!Q_stricmp(action, "blue")) { e->SetRenderColor(0, 100, 255); return; }
    if (!Q_stricmp(action, "yellow")) { e->SetRenderColor(255, 255, 0); return; }
    if (!Q_stricmp(action, "purple")) { e->SetRenderColor(160, 50, 255); return; }
    if (!Q_stricmp(action, "black")) { e->SetRenderColor(0, 0, 0); return; }
    if (!Q_stricmp(action, "orange")) { e->SetRenderColor(255, 128, 0); return; }
    if (!Q_stricmp(action, "opaque")) { e->SetRenderMode(kRenderNormal); e->SetRenderColorA(255); return; }
    if (!Q_stricmp(action, "transparent")) { e->SetRenderMode(kRenderTransColor); e->SetRenderColorA(128); return; }
    CBaseAnimating* anim = e->GetBaseAnimating();
    if (!Q_stricmp(action, "ignite")) { if (anim) anim->Ignite(5.0f, false); return; }
    if (!Q_stricmp(action, "skinreset")) { if (anim) anim->m_nSkin = 0; return; }
    if (!Q_stricmp(action, "bodygroup")) { if (anim && anim->GetNumBodyGroups() > 1) anim->SetBodygroup(1, (anim->GetBodygroup(1) + 1) % MAX(1, anim->GetBodygroupCount(1))); return; }
    if (!Q_stricmp(action, "bodyreset")) { if (anim) for (int i = 0; i < anim->GetNumBodyGroups(); ++i) anim->SetBodygroup(i, 0); return; }
    if (!Q_stricmp(action, "collisionoff")) { e->SetCollisionGroup(COLLISION_GROUP_DEBRIS); return; }
    if (!Q_stricmp(action, "collisionon")) { e->SetCollisionGroup(COLLISION_GROUP_NONE); return; }
    if (!Q_stricmp(action, "rotate") || !Q_stricmp(action, "duplicate"))
    {
        // Whole-entity transforms are restricted to regular physics props.
        if (FClassnameIs(e, "prop_ragdoll")) { ClientPrint(player, HUD_PRINTCENTER, "This action requires a regular prop."); return; }
        Vector origin = e->GetAbsOrigin(); QAngle angles = e->GetAbsAngles();
        if (!Q_stricmp(action, "rotate")) { angles.y += 45; e->Teleport(NULL, &angles, NULL); return; }
        Vector mins, maxs; e->CollisionProp()->WorldSpaceAABB(&mins, &maxs);
        Vector dest = origin + Vector(0, 0, maxs.z - mins.z + 8.0f);
        trace_t tr; UTIL_TraceHull(dest, dest, mins - origin, maxs - origin, MASK_SOLID, e, COLLISION_GROUP_NONE, &tr);
        if (tr.startsolid || tr.allsolid) { ClientPrint(player, HUD_PRINTCENTER, "No room to duplicate prop."); return; }
        CBaseEntity* copy = CreateEntityByName(e->GetClassname()); if (!copy) return;
        copy->KeyValue("model", STRING(e->GetModelName())); copy->SetAbsOrigin(dest); copy->SetAbsAngles(angles);
        if (DispatchSpawn(copy) < 0) return;
        copy->Activate(); color32 col = e->GetRenderColor(); copy->SetRenderColor(col.r, col.g, col.b, col.a);
        copy->SetRenderMode(e->GetRenderMode());
        CBaseAnimating* other = copy->GetBaseAnimating(); if (anim && other) { other->m_nSkin = anim->m_nSkin; for (int i = 0; i < anim->GetNumBodyGroups(); ++i) other->SetBodygroup(i, anim->GetBodygroup(i)); }
        if (copy->VPhysicsGetObject() && e->VPhysicsGetObject()) copy->VPhysicsGetObject()->SetMass(e->VPhysicsGetObject()->GetMass());
        GabeUndo_AddEntity(player, copy, "Context Duplicate"); return;
    }
    IPhysicsObject* list[VPHYSICS_MAX_OBJECT_LIST_COUNT];
    int count = e->VPhysicsGetObjectList(list, ARRAYSIZE(list));
    for (int i = 0; i < count; ++i)
    {
        IPhysicsObject* phys = list[i]; if (!phys) continue;
        if (!Q_stricmp(action, "freeze")) { phys->EnableMotion(false); phys->Sleep(); }
        else if (!Q_stricmp(action, "unfreeze")) { phys->EnableMotion(true); phys->Wake(); }
        else if (!Q_stricmp(action, "gravityoff")) { phys->EnableGravity(false); phys->Wake(); }
        else if (!Q_stricmp(action, "gravityon")) { phys->EnableGravity(true); phys->Wake(); }
        else if (!Q_stricmp(action, "stop")) { Vector zero = vec3_origin; AngularImpulse angular = vec3_origin; phys->SetVelocity(&zero, &angular); }
        else if (!Q_stricmp(action, "wake")) phys->Wake();
        else if (!Q_stricmp(action, "damping0") || !Q_stricmp(action, "damping5")) { float d = !Q_stricmp(action, "damping5") ? 5.0f : 0.0f; phys->SetDamping(&d, &d); }
        else if (!Q_stricmp(action, "mass1")) phys->SetMass(MAX(0.1f, 1.0f / MAX(1, count)));
        else if (!Q_stricmp(action, "mass1000")) phys->SetMass(1000.0f / MAX(1, count));
        else if (!Q_stricmp(action, "mass10")) phys->SetMass(10.0f / MAX(1, count));
        else if (!Q_stricmp(action, "mass50")) phys->SetMass(50.0f / MAX(1, count));
        else if (!Q_stricmp(action, "mass200")) phys->SetMass(200.0f / MAX(1, count));
    }
}
