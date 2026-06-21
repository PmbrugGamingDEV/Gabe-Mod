#pragma once

#include "cbase.h"
#include "baseanimating.h"

class CStickyPellet : public CBaseAnimating
{
public:
	DECLARE_CLASS(CStickyPellet, CBaseAnimating);
	DECLARE_DATADESC();

	void Spawn();
	void StickToSurface(trace_t& tr);
	void Detonate();
	void Think();


	CHandle<CBaseEntity> m_hStuckTo;
	Vector m_vOffset;
	QAngle m_qAngleOffset;
	bool m_bStuck;
	void DelayedDetonate();
};
