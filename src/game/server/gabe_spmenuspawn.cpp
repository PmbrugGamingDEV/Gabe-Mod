#include "cbase.h"
#include "player.h"
#include "util.h"
#include "util_shared.h"
#include "ieffects.h"
#include "gabe_undo.h"
#include "Sprite.h"
#include "vphysics_interface.h"
#include "tier0/memdbgon.h"

static const char* GetNPCUndoName(const char* classname)
{
    static const struct
    {
        const char* classname;
        const char* name;
    } names[] =
    {
        { "npc_kleiner", "Kleiner" },
        { "npc_gman", "G-Man" },
        { "npc_breen", "Breen" },
        { "npc_barney", "Barney" },
        { "npc_eli", "Eli" },
        { "npc_monk", "Father Grigori" },
        { "npc_alyx", "Alyx" },
        { "npc_mossman", "Mossman" },
        { "npc_dog", "DOG" },
        { "npc_citizen", "Citizen" },
        { "npc_metropolice", "Metropolice" },
        { "npc_combine_s", "Combine Soldier" },
        { "npc_zombie", "Zombie" },
        { "npc_fastzombie", "Fast Zombie" },
        { "npc_poisonzombie", "Poison Zombie" },
        { "npc_zombie_torso", "Zombie Torso" },
        { "npc_headcrab", "Headcrab" },
        { "npc_headcrab_fast", "Fast Headcrab" },
        { "npc_headcrab_poison", "Poison Headcrab" },
        { "npc_vortigaunt", "Vortigaunt" },
        { "npc_antlion", "Antlion" },
        { "npc_antlionguard", "Antlion Guard" },
        { "npc_manhack", "Manhack" },
        { "npc_cscanner", "City Scanner" },
        { "npc_clawscanner", "Claw Scanner" },
        { "npc_rollermine", "Rollermine" },
        { "npc_turret_floor", "Turret" },
        { "npc_helicopter", "Helicopter" },
        { "npc_strider", "Strider" },
        { "npc_combinegunship", "Gunship" },
        { "npc_advisor", "Advisor" },
        { "npc_fisherman", "Fisherman" },
        { "npc_ichthyosaur", "Ichthyosaur" },
        { "npc_barnacle", "Barnacle" },
        { "combine_mine", "Combine Mine" },
        { "npc_crow", "Crow" },
        { "npc_pigeon", "Pigeon" },
        { "npc_seagull", "Seagull" }
    };

    for (int i = 0; i < ARRAYSIZE(names); ++i)
    {
        if (!Q_stricmp(classname, names[i].classname))
            return names[i].name;
    }

    return "NPC";
}

class CFadeInEntity : public CBaseEntity
{
public:
    DECLARE_CLASS(CFadeInEntity, CBaseEntity);
    DECLARE_DATADESC();
    EHANDLE m_hTarget;
    int m_iOriginalMode;
    int m_iOriginalAlpha;
    void Spawn()
    {
        SetThink(&CFadeInEntity::FadeThink);
        SetNextThink(gpGlobals->curtime + 0.02f);
    }
    void FadeThink()
    {
        CBaseEntity* pEnt = m_hTarget.Get();
        if (!pEnt || pEnt->IsMarkedForDeletion()) { UTIL_Remove(this); return; }
        int alpha = MIN(m_iOriginalAlpha, pEnt->GetRenderColor().a + 25);
        pEnt->SetRenderColorA(alpha);
        if (alpha >= m_iOriginalAlpha)
        {
            pEnt->SetRenderMode((RenderMode_t)m_iOriginalMode);
            UTIL_Remove(this); return;
        }
        SetNextThink(gpGlobals->curtime + 0.02f);
    }
};
LINK_ENTITY_TO_CLASS(fadein_helper, CFadeInEntity);
BEGIN_DATADESC(CFadeInEntity)
DEFINE_FIELD(m_hTarget, FIELD_EHANDLE),
DEFINE_FIELD(m_iOriginalMode, FIELD_INTEGER),
DEFINE_FIELD(m_iOriginalAlpha, FIELD_INTEGER),
DEFINE_THINKFUNC(FadeThink),
END_DATADESC()
static void StartFadeIn(CBaseEntity* pEnt)
{
    if (!pEnt || pEnt->IsMarkedForDeletion()) return;
    CFadeInEntity* f = static_cast<CFadeInEntity*>(CreateEntityByName("fadein_helper"));
    if (!f) return;
    f->m_hTarget = pEnt;
    f->m_iOriginalMode = pEnt->GetRenderMode();
    f->m_iOriginalAlpha = pEnt->GetRenderColor().a;
    if (DispatchSpawn(f) < 0) return;
    pEnt->SetRenderMode(kRenderTransTexture);
    pEnt->SetRenderColorA(0);
    f->Activate();
}
static bool FixSpawnPosition(
    CBaseEntity* pEnt,
    const trace_t& spawnTrace)
{
    if (!pEnt || !spawnTrace.DidHit())
        return false;

    if (spawnTrace.startsolid || spawnTrace.allsolid)
        return false;

    // Retrieve the actual collision bounds.
    pEnt->CollisionProp()->UpdatePartition();

    Vector mins, maxs;

    pEnt->CollisionProp()->WorldSpaceAABB(
        &mins, &maxs);

    // Convert world bounds into offsets from
    // the entity's origin.
    Vector origin = pEnt->GetAbsOrigin();

    mins -= origin;
    maxs -= origin;

    Vector normal = spawnTrace.plane.normal;

    if (VectorNormalize(normal) < 0.001f)
        normal.Init(0, 0, 1);

    // Find the bounding-box corner closest
    // to the surface.
    Vector support;

    support.x = normal.x >= 0.0f ? mins.x : maxs.x;
    support.y = normal.y >= 0.0f ? mins.y : maxs.y;
    support.z = normal.z >= 0.0f ? mins.z : maxs.z;

    float offset = -DotProduct(support, normal);

    // Leave a small gap between the prop
    // and the surface.
    const float clearance = 2.0f;

    Vector spawnPos =
        spawnTrace.endpos +
        normal * (offset + clearance);

    // Search outward from the surface until
    // there is enough room for the entire prop.
    for (int i = 0; i <= 32; ++i)
    {
        Vector candidate =
            spawnPos + normal * (i * 4.0f);

        trace_t check;

        UTIL_TraceHull(
            candidate,
            candidate,
            mins,
            maxs,
            MASK_SOLID,
            pEnt,
            COLLISION_GROUP_NONE,
            &check
        );

        if (check.startsolid || check.allsolid)
            continue;

        // Teleport also updates the physics position.
        Vector velocity = vec3_origin;

        pEnt->Teleport(
            &candidate,
            NULL,
            &velocity
        );

        pEnt->CollisionProp()->UpdatePartition();

        IPhysicsObject* pPhysics =
            pEnt->VPhysicsGetObject();

        if (pPhysics)
            pPhysics->Wake();

        return true;
    }

    // No unobstructed position was found.
    return false;
}
static CBaseEntity* SpawnEntity(
    const char* classname,
    const char* model,
    const trace_t& tr)
{
    if (!tr.DidHit() || tr.startsolid || tr.allsolid)
        return NULL;

    if (tr.surface.flags & SURF_SKY)
        return NULL;

    CBaseEntity* p = CreateEntityByName(classname);

    if (!p)
        return NULL;

    if (model && model[0])
    {
        p->PrecacheModel(model);
        p->SetModel(model);
    }

    // Initial position before physics initialization.
    p->SetAbsOrigin(
        tr.endpos + tr.plane.normal * 16.0f);

    p->SetAbsAngles(vec3_angle);

    // Initialize model, collision bounds and physics.
    if (DispatchSpawn(p) < 0)
        return NULL;

    if (p->IsMarkedForDeletion())
        return NULL;

    // Find an unobstructed position.
    if (!FixSpawnPosition(p, tr))
    {
        Warning(
            "[SpawnMenu] No room to spawn %s\n",
            classname);

        UTIL_Remove(p);
        return NULL;
    }

    p->Activate();

    if (p->IsMarkedForDeletion())
        return NULL;

    return p;
}

static CBaseEntity* CreateEffectHandle(const trace_t& tr)
{
    CBaseEntity* pHandle = SpawnEntity(
        "prop_physics",
        "models/props_junk/PopCan01a.mdl",
        tr
    );

    if (!pHandle)
        return NULL;

    IPhysicsObject* pPhysics = pHandle->VPhysicsGetObject();

    if (!pPhysics)
    {
        UTIL_Remove(pHandle);
        return NULL;
    }

    // Hide the physical model.
    pHandle->AddEffects(EF_NODRAW);

    // Allow movement without gravity.
    pPhysics->EnableGravity(false);
    pPhysics->EnableMotion(true);
    pPhysics->SetMass(5.0f);

    // Prevent excessive drifting and spinning.
    float damping = 5.0f;
    float angularDamping = 10.0f;

    pPhysics->SetDamping(&damping, &angularDamping);

    Vector velocity = vec3_origin;
    AngularImpulse angular = vec3_origin;

    pPhysics->SetVelocity(&velocity, &angular);
    pPhysics->Sleep();

    return pHandle;
}

static void SpawnFromCommand(CBasePlayer* pPlayer, const char* type, const char* value)
{
    if (!pPlayer || !type || !value || !*value) return;
    // Commands such as mode changes need no trace hit.
    if (!Q_stricmp(type, "command"))
    {
        if (Q_stristr(value, ";") || Q_stristr(value, "\n") || Q_stristr(value, "\r") || Q_stristr(value, "\"")) return;
        // Legacy spawnmenu NPC commands must use the undoable path.
        if (!Q_strnicmp(value, "npc_create ", 11))
        {
            const char* npcClass = value + 11;
            while (*npcClass == ' ' || *npcClass == '\t')
                ++npcClass;

            char classname[128];
            Q_strncpy(classname, npcClass, sizeof(classname));
            int len = Q_strlen(classname);
            while (len > 0 && (classname[len - 1] == ' ' || classname[len - 1] == '\t'))
                classname[--len] = '\0';

            if (!len || Q_stristr(classname, " ") || Q_stristr(classname, "\t") ||
                Q_strlen(npcClass) >= sizeof(classname))
                return;

            SpawnFromCommand(pPlayer, "npc", classname);
            return;
        }
        if (!Q_strnicmp(value, "mtool_mode ", 11))
        {
            CBaseCombatWeapon* w = pPlayer->Weapon_OwnsThisType("weapon_multitool");
            if (w) pPlayer->Weapon_Switch(w);
        }
        char cmd[512];
        Q_snprintf(cmd, sizeof(cmd), "%s\n", value);
        engine->ClientCommand(pPlayer->edict(), cmd);
        return;
    }
    trace_t tr;
    Vector start = pPlayer->EyePosition();
    Vector end = start + pPlayer->EyeDirection3D() * 1024.0f;
    UTIL_TraceLine(start, end, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &tr);
    if (!tr.DidHit()) return;
    const trace_t& hitPos = tr;
    if (!Q_stricmp(type, "effect"))
    {
        CBaseEntity* carrier = CreateEffectHandle(tr);

        if (!carrier)
            return;

        // Create the visible glow handle.
        const char* spriteMaterial = "sprites/glow01.vmt";

        carrier->PrecacheModel(spriteMaterial);

        CSprite* pGlow = CSprite::SpriteCreate(
            spriteMaterial,
            carrier->GetAbsOrigin(),
            false
        );

        if (!pGlow)
        {
            UTIL_Remove(carrier);
            return;
        }

        pGlow->SetTransparency(
            kRenderGlow,
            80, 255, 120, 255,
            kRenderFxNoDissipation
        );

        pGlow->SetScale(0.25f);
        pGlow->SetParent(carrier);
        pGlow->SetLocalOrigin(vec3_origin);
        pGlow->SetSolid(SOLID_NONE);
        pGlow->TurnOn();

        // Create the actual effect model.
        CBaseEntity* effect = CreateEntityByName("prop_dynamic");

        if (!effect)
        {
            UTIL_Remove(pGlow);
            UTIL_Remove(carrier);
            return;
        }

        effect->PrecacheModel(value);
        effect->SetModel(value);
        effect->SetAbsOrigin(carrier->GetAbsOrigin());
        effect->KeyValue("solid", "0");

        if (DispatchSpawn(effect) < 0)
        {
            UTIL_Remove(pGlow);
            UTIL_Remove(carrier);
            return;
        }

        effect->SetParent(carrier);
        effect->SetLocalOrigin(vec3_origin);
        effect->SetSolid(SOLID_NONE);
        effect->Activate();

        StartFadeIn(effect);

        // Undo removes the entire effect assembly.
        GabeUndo_Begin(pPlayer, "Effect");

        GabeUndo_Add(carrier);
        GabeUndo_Add(pGlow);
        GabeUndo_Add(effect);

        GabeUndo_Finish();

        return;
    }
    const char* classname = value;
    const char* model = NULL;
    const char* name = "Entity";
    if (!Q_stricmp(type, "prop")) { classname = "prop_physics"; model = value; name = "Prop"; }
    else if (!Q_stricmp(type, "ragdoll")) { classname = "prop_ragdoll"; model = value; name = "Ragdoll"; }
    else if (!Q_stricmp(type, "npc")) { if (Q_strnicmp(value, "npc_", 4) && Q_stricmp(value, "combine_mine")) return; name = GetNPCUndoName(value); }
    else if (!Q_stricmp(type, "weapon")) { if (!Q_strnicmp(value, "item_", 5)) name = "Item"; else if (!Q_strnicmp(value, "weapon_", 7)) name = "Weapon"; else return; }
    else if (Q_stricmp(type, "debug")) { return; }
    // This preserves the existing debug spawn path. Restrict access on public servers.
    CBaseEntity* pEnt = SpawnEntity(classname, model, hitPos);
    if (!pEnt) { Warning("[SpawnMenu] Failed to spawn %s\n", value); return; }
    StartFadeIn(pEnt);
    GabeUndo_AddEntity(pPlayer, pEnt, name);
    Msg("[SpawnMenu] Submitted %s (index %d) for undo.\n",
        pEnt->GetClassname(), pEnt->entindex());
}
CON_COMMAND(gabe_spawn, "gabe_spawn <prop|ragdoll|npc|effect|debug|weapon|command> <value>")
{
    CBasePlayer* pPlayer = UTIL_GetCommandClient();
    if (!pPlayer || args.ArgC() < 3) { Msg("Usage: gabe_spawn <type> <value>\n"); return; }
    SpawnFromCommand(pPlayer, args[1], args[2]);
}
