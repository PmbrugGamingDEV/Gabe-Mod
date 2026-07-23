//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Hides/Shows an entity.
//
//=============================================================================

#include "cbase.h"

class CLogicHide : public CLogicalEntity
{
public:

	DECLARE_CLASS(CLogicHide, CLogicalEntity);
	DECLARE_DATADESC();

	CLogicHide();

	void InputHide(inputdata_t& data);
	void InputShow(inputdata_t& data);

private:
	COutputEvent m_OnHidden;
	COutputEvent m_OnShown;

	string_t m_iszHiddenEntityName;
};

CLogicHide::CLogicHide()
{
	m_iszHiddenEntityName = NULL_STRING;
}

BEGIN_DATADESC(CLogicHide)

DEFINE_INPUTFUNC(FIELD_VOID, "Hide", InputHide),
DEFINE_INPUTFUNC(FIELD_VOID, "Show", InputShow),
DEFINE_OUTPUT(m_OnHidden, "OnHidden"),
DEFINE_OUTPUT(m_OnShown, "OnShown"),
DEFINE_KEYFIELD( m_iszHiddenEntityName, FIELD_STRING, "HiddenTarget" ),

END_DATADESC();

LINK_ENTITY_TO_CLASS(logic_hide, CLogicHide);

void CLogicHide::InputHide(inputdata_t& data)
{
	bool bFoundAny = false;
	CBaseEntity* pEntity = NULL;

	while ((pEntity = gEntList.FindEntityByName(
		pEntity,
		m_iszHiddenEntityName,
		this,
		data.pActivator
	)) != NULL)
	{
		bool bFoundAny = true;

		pEntity->AddEffects(EF_NODRAW);
		pEntity->AddSolidFlags(FSOLID_NOT_SOLID);

		IPhysicsObject* pPhysics = pEntity->VPhysicsGetObject();

		if (pPhysics)
		{
			pPhysics->EnableCollisions(false);
		}
	}

	if (bFoundAny)
	{
		m_OnHidden.FireOutput(data.pActivator, this);
	}
}

void CLogicHide::InputShow(inputdata_t& data)
{
	bool bFoundAny = false;
	CBaseEntity* pEntity = NULL;

	while ((pEntity = gEntList.FindEntityByName(
		pEntity,
		m_iszHiddenEntityName,
		this,
		data.pActivator
	)) != NULL)
	{
		bFoundAny = true;

		pEntity->RemoveEffects(EF_NODRAW);
		pEntity->RemoveSolidFlags(FSOLID_NOT_SOLID);

		IPhysicsObject* pPhysics = pEntity->VPhysicsGetObject();

		if (pPhysics)
		{
			pPhysics->EnableCollisions(true);
		}
	}

	if (bFoundAny)
	{
		m_OnShown.FireOutput(data.pActivator, this);
	}
}