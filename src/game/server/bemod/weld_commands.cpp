#include "cbase.h"
#include "convar.h"
#include "physics.h"
#include "vphysics_interface.h"
#include "vphysics/constraints.h"
#include "hl2mp_player.h"
#include "engine/IEngineTrace.h"
#include "IEffects.h"
#include "tier0/memdbgon.h"

CBaseEntity* g_pFirstObject = nullptr;
CBaseEntity* g_pSecondObject = nullptr;

void SelectObjectInFrontOfCrosshair(const CCommand& args) {
    CBasePlayer* pPlayer = UTIL_GetCommandClient();
    if (!pPlayer) {
        Msg("Player not found\n");
        return;
    }

    Vector vecStart = pPlayer->EyePosition();
    Vector vecForward;
    pPlayer->EyeVectors(&vecForward);

    Vector vecEnd = vecStart + vecForward * MAX_TRACE_LENGTH;

    trace_t tr;
    UTIL_TraceLine(vecStart, vecEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

    if (tr.fraction == 1.0) {
        Msg("No object in front of crosshair\n");
        return;
    }

    CBaseEntity* pEntity = tr.m_pEnt;
    if (!pEntity) {
        Msg("No valid entity hit\n");
        return;
    }

    if (!g_pFirstObject) {
        g_pFirstObject = pEntity;
        Msg("First object selected: %d\n", pEntity->entindex());
    }
    else if (!g_pSecondObject) {
        g_pSecondObject = pEntity;
        Msg("Second object selected: %d\n", pEntity->entindex());
    }
    else {
        Msg("Both objects already selected. Use weld_objects to weld them or reset_selection to clear selection.\n");
    }
}

void WeldObjects(const CCommand& args) {
    if (!g_pFirstObject || !g_pSecondObject) {
        Msg("Both objects must be selected before welding.\n");
        return;
    }

    IPhysicsObject* pPhysObject1 = g_pFirstObject->VPhysicsGetObject();
    IPhysicsObject* pPhysObject2 = g_pSecondObject->VPhysicsGetObject();

    if (!pPhysObject1 || !pPhysObject2) {
        Msg("Both objects must have physics.\n");
        return;
    }

    constraint_fixedparams_t fixedConstraint;
    fixedConstraint.Defaults();
    fixedConstraint.InitWithCurrentObjectState(pPhysObject1, pPhysObject2);

    physenv->CreateFixedConstraint(pPhysObject1, pPhysObject2, nullptr, fixedConstraint);

    Msg("Objects welded together.\n");

    // Reset selection
    g_pFirstObject = nullptr;
    g_pSecondObject = nullptr;
}

void ResetSelection(const CCommand& args) {
    g_pFirstObject = nullptr;
    g_pSecondObject = nullptr;
    Msg("Object selection reset.\n");
}

CON_COMMAND(bemod_select, "Select the object in front of the crosshair.") {
    SelectObjectInFrontOfCrosshair(args);
}

CON_COMMAND(bemod_weld, "Weld the selected objects together.") {
    WeldObjects(args);
}

CON_COMMAND(bemod_resetweld, "Reset the object selection.") {
    ResetSelection(args);
}
