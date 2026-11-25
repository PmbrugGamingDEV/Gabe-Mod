#include "cbase.h"

class CEnvGodRays : public CBaseEntity
{
public:
    DECLARE_CLASS(CEnvGodRays, CBaseEntity);
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

    void Spawn(void)
    {
        SetSolid(SOLID_NONE);
        SetMoveType(MOVETYPE_NONE);
    }
};

LINK_ENTITY_TO_CLASS(env_godrays, CEnvGodRays);

IMPLEMENT_SERVERCLASS_ST(CEnvGodRays, DT_EnvGodRays)
END_SEND_TABLE()

BEGIN_DATADESC(CEnvGodRays)
END_DATADESC()