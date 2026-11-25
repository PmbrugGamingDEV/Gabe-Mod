#ifndef GABE_UNDO_H
#define GABE_UNDO_H
class CBasePlayer;
class CBaseEntity;
class IPhysicsConstraint;
void GabeUndo_AddEntity(CBasePlayer*, CBaseEntity*, const char*);
void GabeUndo_Begin(CBasePlayer*, const char*);
void GabeUndo_Add(CBaseEntity*);
void GabeUndo_Finish();
void GabeUndo_Cancel();
void GabeUndo_Last(CBasePlayer*);
void GabeUndo_Clear();
void SendUndoNotification(CBasePlayer*, const char*);
CBaseEntity* GabeUndo_OwnConstraint(IPhysicsConstraint*);
void GabeUndo_AddConstraintVisual(CBaseEntity* owner, CBaseEntity* visual);
#endif
