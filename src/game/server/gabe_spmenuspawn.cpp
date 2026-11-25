#include "cbase.h"
#include "player.h"
#include "util.h"
#include "util_shared.h"
#include "ieffects.h"
#include "gabe_undo.h"
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
static void FixSpawnPosition(CBaseEntity* pEnt, const Vector& hitPos)
{
    pEnt->SetAbsOrigin(hitPos);
    Vector mins, maxs;
    pEnt->CollisionProp()->WorldSpaceAABB(&mins, &maxs);
    Vector pos = hitPos;
    pos.z += hitPos.z - mins.z + 1.0f;
    pEnt->SetAbsOrigin(pos);
    pEnt->CollisionProp()->UpdatePartition();
}
static CBaseEntity* SpawnEntity(const char* classname, const char* model, const Vector& pos)
{
    CBaseEntity* p = CreateEntityByName(classname);
    if (!p) return NULL;
    if (model)
    {
        p->PrecacheModel(model);
        p->SetModel(model);
    }
    p->SetAbsOrigin(pos);
    if (DispatchSpawn(p) < 0) return NULL;
    FixSpawnPosition(p, pos);
    p->Activate();
    if (p->IsMarkedForDeletion()) return NULL;
    return p;
}
static void SpawnFromCommand(CBasePlayer* pPlayer, const char* type, const char* value)
{
    if (!pPlayer || !type || !value || !*value) return;
    // Commands such as mode changes need no trace hit.
    if (!Q_stricmp(type, "command"))
    {
        if (Q_stristr(value, ";") || Q_stristr(value, "\n") || Q_stristr(value, "\r") || Q_stristr(value, "\"")) return;
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
    Vector hitPos = tr.endpos;
    if (!Q_stricmp(type, "effect"))
    {
        CBaseEntity* carrier = SpawnEntity("prop_physics", "models/props_junk/cinderblock01a.mdl", hitPos);
        if (!carrier) return;
        CBaseEntity* effect = CreateEntityByName("prop_dynamic");
        if (!effect) { UTIL_Remove(carrier); return; }
        effect->PrecacheModel(value);
        effect->SetModel(value);
        effect->SetAbsOrigin(carrier->GetAbsOrigin());
        effect->KeyValue("solid", "0");
        if (DispatchSpawn(effect) < 0) { UTIL_Remove(carrier); return; }
        effect->SetParent(carrier);
        effect->SetLocalOrigin(vec3_origin);
        effect->SetSolid(SOLID_NONE);
        effect->Activate();
        StartFadeIn(carrier); StartFadeIn(effect);
        GabeUndo_Begin(pPlayer, "Effect");
        GabeUndo_Add(carrier); GabeUndo_Add(effect);
        GabeUndo_Finish();
        return;
    }
    const char* classname = value;
    const char* model = NULL;
    const char* name = "Entity";
    if (!Q_stricmp(type, "prop")) { classname = "prop_physics"; model = value; name = "Prop"; }
    else if (!Q_stricmp(type, "ragdoll")) { classname = "prop_ragdoll"; model = value; name = "Ragdoll"; }
    else if (!Q_stricmp(type, "npc")) { if (Q_strnicmp(value, "npc_", 4)) return; name = GetNPCUndoName(value); }
    else if (!Q_stricmp(type, "weapon")) { if (!Q_strnicmp(value, "item_", 5)) name = "Item"; else if (!Q_strnicmp(value, "weapon_", 7)) name = "Weapon"; else return; }
    else if (Q_stricmp(type, "debug")) { g_pEffects->Sparks(hitPos, 1, 1, 0); return; }
    // This preserves the existing debug spawn path. Restrict access on public servers.
    CBaseEntity* pEnt = SpawnEntity(classname, model, hitPos);
    if (!pEnt) { Warning("[SpawnMenu] Failed to spawn %s\n", value); return; }
    StartFadeIn(pEnt);
    GabeUndo_AddEntity(pPlayer, pEnt, name);
}
CON_COMMAND(gabe_spawn, "gabe_spawn <prop|ragdoll|npc|effect|debug|weapon|command> <value>")
{
    CBasePlayer* pPlayer = UTIL_GetCommandClient();
    if (!pPlayer || args.ArgC() < 3) { Msg("Usage: gabe_spawn <type> <value>\n"); return; }
    SpawnFromCommand(pPlayer, args[1], args[2]);
}
