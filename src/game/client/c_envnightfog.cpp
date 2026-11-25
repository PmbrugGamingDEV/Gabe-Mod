#include "cbase.h"
#include "c_baseentity.h"

class C_EnvHeightFog : public C_BaseEntity
{
public:
    DECLARE_CLASS(C_EnvHeightFog, C_BaseEntity);
    DECLARE_CLIENTCLASS();

    C_EnvHeightFog() {}
};

IMPLEMENT_CLIENTCLASS_DT(C_EnvHeightFog, DT_EnvHeightFog, CEnvHeightFog)
END_RECV_TABLE();