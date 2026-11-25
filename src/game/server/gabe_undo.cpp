#include "cbase.h"
#include "player.h"
#include "util.h"
#include "igamesystem.h"
#include "physics.h"
#include "physics_saverestore.h"
#include "vphysics_interface.h"
#include "gabe_undo.h"
#include "tier1/utlvector.h"
#include "vphysics/constraints.h"
#include "tier0/memdbgon.h"

// Own the VPhysics constraint independently of the weapon lifetime.
class CGabeConstraintOwner : public CBaseEntity, public IPhysicsConstraintEvent
{
public:
    DECLARE_CLASS(CGabeConstraintOwner, CBaseEntity);
    DECLARE_DATADESC();
    CGabeConstraintOwner() : m_pConstraint(NULL) {}
    void Adopt(IPhysicsConstraint* p) { m_pConstraint = p; p->SetGameData(this); }
    void AddVisual(CBaseEntity* p)
    {
        if (!p) return;
        for (int i = 0; i < 3; ++i)
            if (!m_hVisual[i]) { m_hVisual[i] = p; return; }
    }
    void ConstraintBroken(IPhysicsConstraint* p)
    {
        if (p != m_pConstraint) return;
        p->Deactivate();
        PhysCallbackRemove(NetworkProp());
    }
    void UpdateOnRemove()
    {
        if (m_pConstraint)
        {
            m_pConstraint->SetGameData(NULL);
            m_pConstraint->Deactivate();
            physenv->DestroyConstraint(m_pConstraint);
            m_pConstraint = NULL;
        }
        for (int i = 2; i >= 0; --i)
            if (m_hVisual[i].Get()) UTIL_Remove(m_hVisual[i].Get());
        BaseClass::UpdateOnRemove();
    }
private:
    IPhysicsConstraint* m_pConstraint;
    EHANDLE m_hVisual[3];
};
LINK_ENTITY_TO_CLASS(gabe_constraint_owner, CGabeConstraintOwner);
BEGIN_DATADESC(CGabeConstraintOwner)
DEFINE_PHYSPTR(m_pConstraint),
DEFINE_ARRAY(m_hVisual, FIELD_EHANDLE, 3),
END_DATADESC()

CBaseEntity* GabeUndo_OwnConstraint(IPhysicsConstraint* p)
{
    if (!p) return NULL;
    // Never steal ownership from another physics entity.
    if (p->GetGameData()) return NULL;
    CGabeConstraintOwner* owner = static_cast<CGabeConstraintOwner*>(CreateEntityByName("gabe_constraint_owner"));
    if (!owner) { physenv->DestroyConstraint(p); return NULL; }
    owner->Adopt(p);
    return owner;
}
void GabeUndo_AddConstraintVisual(CBaseEntity* owner, CBaseEntity* visual)
{
    if (owner && visual) static_cast<CGabeConstraintOwner*>(owner)->AddVisual(visual);
}

struct UndoAction
{
    EHANDLE player;
    char name[128];
    CUtlVector<EHANDLE> entities;
};
static CUtlVector<UndoAction*> g_actions;
static UndoAction* g_pending = NULL;
static bool Live(CBaseEntity* e) { return e && !e->IsMarkedForDeletion(); }
void SendUndoNotification(CBasePlayer* p, const char* name)
{
    if (!p) return;
    char safe[128], cmd[256];
    Q_strncpy(safe, name && *name ? name : "Entity", sizeof(safe));
    for (int i = 0; safe[i]; ++i)
        if (safe[i] == '"' || safe[i] == ';' || safe[i] == '\\' || safe[i] == '\n' || safe[i] == '\r') safe[i] = ' ';
    Q_snprintf(cmd, sizeof(cmd), "gabe_notify_undo \"%s\"\n", safe);
    engine->ClientCommand(p->edict(), cmd);
}
void GabeUndo_Cancel() { delete g_pending; g_pending = NULL; }
void GabeUndo_Begin(CBasePlayer* p, const char* name)
{
    GabeUndo_Cancel();
    if (!p) return;
    g_pending = new UndoAction;
    g_pending->player = p;
    Q_strncpy(g_pending->name, name && *name ? name : "Entity", sizeof(g_pending->name));
}
void GabeUndo_Add(CBaseEntity* e)
{
    if (!g_pending || !Live(e) || e->IsPlayer() || e->IsWorld()) return;
    for (int i = 0; i < g_pending->entities.Count(); ++i) if (g_pending->entities[i].Get() == e) return;
    g_pending->entities.AddToTail(EHANDLE(e));
}
static void Cleanup()
{
    for (int i = g_actions.Count() - 1; i >= 0; --i)
    {
        UndoAction* a = g_actions[i];
        for (int j = a->entities.Count() - 1; j >= 0; --j) if (!Live(a->entities[j].Get())) a->entities.Remove(j);
        if (!a->player.Get() || !a->entities.Count()) { delete a; g_actions.Remove(i); }
    }
}
void GabeUndo_Finish()
{
    if (!g_pending) return;
    if (!g_pending->entities.Count()) { GabeUndo_Cancel(); return; }
    g_actions.AddToTail(g_pending); g_pending = NULL; Cleanup();
}
void GabeUndo_AddEntity(CBasePlayer* p, CBaseEntity* e, const char* name)
{
    GabeUndo_Begin(p, name); GabeUndo_Add(e); GabeUndo_Finish();
}
void GabeUndo_Last(CBasePlayer* p)
{
    if (!p) return;
    Cleanup();
    for (int i = g_actions.Count() - 1; i >= 0; --i)
    {
        UndoAction* a = g_actions[i];
        if (a->player.Get() != p) continue;
        g_actions.Remove(i);
        for (int j = a->entities.Count() - 1; j >= 0; --j)
            if (Live(a->entities[j].Get())) UTIL_Remove(a->entities[j].Get());
        SendUndoNotification(p, a->name); delete a; return;
    }
    ClientPrint(p, HUD_PRINTCONSOLE, "[Undo] Nothing to undo.\n");
}
void GabeUndo_Clear()
{
    GabeUndo_Cancel();
    for (int i = 0; i < g_actions.Count(); ++i) delete g_actions[i];
    g_actions.RemoveAll();
}
class CGabeUndoSystem : public CAutoGameSystem
{
public:
    CGabeUndoSystem() :CAutoGameSystem("GabeUndoSystem") {}
    void LevelInitPreEntity() { GabeUndo_Clear(); }
    void LevelShutdownPostEntity() { GabeUndo_Clear(); }
};
static CGabeUndoSystem g_undoSystem;
CON_COMMAND(gabe_undo, "Undo your last spawned object.") { GabeUndo_Last(UTIL_GetCommandClient()); }
