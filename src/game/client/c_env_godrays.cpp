#include "cbase.h"
#include "c_baseentity.h"

class C_EnvGodRays : public C_BaseEntity
{
public:
    DECLARE_CLASS(C_EnvGodRays, C_BaseEntity);
    DECLARE_CLIENTCLASS();

    C_EnvGodRays() {}

    virtual void OnDataChanged(DataUpdateType_t updateType)
    {
        BaseClass::OnDataChanged(updateType);
    }
};

IMPLEMENT_CLIENTCLASS_DT(C_EnvGodRays, DT_EnvGodRays, CEnvGodRays)
END_RECV_TABLE();