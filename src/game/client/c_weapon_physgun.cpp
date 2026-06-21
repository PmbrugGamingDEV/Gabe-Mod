//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "in_buttons.h"
#include "beamdraw.h"
#include "c_basehlcombatweapon.h"
#include "c_weapon__stubs.h"
#include "ClientEffectPrecacheSystem.h"
#include "iefx.h"
#include "dlight.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN(PrecacheEffectJBGravityGun)
CLIENTEFFECT_MATERIAL("sprites/physbeam")
CLIENTEFFECT_MATERIAL("sprites/physglow")
CLIENTEFFECT_REGISTER_END()


class C_BeamQuadratic : public CDefaultClientRenderable
{
public:
	C_BeamQuadratic();
	void Update(C_BaseEntity* pOwner);

	// IClientRenderable
	virtual const Vector& GetRenderOrigin(void)
	{
		return m_targetPosition;
	}

	virtual const QAngle& GetRenderAngles(void)
	{
		return vec3_angle;
	}

	virtual bool ShouldDraw(void)
	{
		return true;
	}

	virtual bool IsTransparent(void)
	{
		return true;
	}

	virtual bool ShouldReceiveProjectedTextures(int flags)
	{
		return false;
	}

	virtual int DrawModel(int flags);

	virtual void GetRenderBounds(
		Vector& mins,
		Vector& maxs
	)
	{
		mins.Init(-64, -64, -64);
		maxs.Init(64, 64, 64);
	}

	virtual const matrix3x4_t& RenderableToWorldTransform()
	{
		return m_RenderMatrix;
	}

public:
	C_BaseEntity* m_pOwner;

	Vector m_targetPosition;
	Vector m_worldPosition;

	int m_active;
	int m_glueTouching;
	int m_viewModelIndex;

	matrix3x4_t m_RenderMatrix;
};


class C_JBWeaponGravityGun : public C_BaseHLCombatWeapon
{
	DECLARE_CLASS(
		C_JBWeaponGravityGun,
		C_BaseHLCombatWeapon
	);

public:
	C_JBWeaponGravityGun() {}

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	int KeyInput(
		int down,
		ButtonCode_t keynum,
		const char* pszCurrentBinding
	)
	{
		if (gHUD.m_iKeyBits & IN_ATTACK)
		{
			switch (keynum)
			{
			case MOUSE_WHEEL_UP:
				gHUD.m_iKeyBits |= IN_WEAPON1;
				return 0;

			case MOUSE_WHEEL_DOWN:
				gHUD.m_iKeyBits |= IN_WEAPON2;
				return 0;
			}
		}

		return BaseClass::KeyInput(
			down,
			keynum,
			pszCurrentBinding
		);
	}

	void OnDataChanged(DataUpdateType_t updateType)
	{
		BaseClass::OnDataChanged(updateType);

		m_beam.Update(this);
	}

private:
	C_JBWeaponGravityGun(
		const C_JBWeaponGravityGun&
	);

private:
	C_BeamQuadratic m_beam;
};


STUB_WEAPON_CLASS_IMPLEMENT(
	weapon_jbphysgun,
	C_JBWeaponGravityGun
);


IMPLEMENT_CLIENTCLASS_DT(
	C_JBWeaponGravityGun,
	DT_JBWeaponGravityGun,
	CJBWeaponGravityGun
)

RecvPropVector(
	RECVINFO_NAME(
		m_beam.m_targetPosition,
		m_targetPosition
	)
),

RecvPropVector(
	RECVINFO_NAME(
		m_beam.m_worldPosition,
		m_worldPosition
	)
),

RecvPropInt(
	RECVINFO_NAME(
		m_beam.m_active,
		m_active
	)
),

RecvPropInt(
	RECVINFO_NAME(
		m_beam.m_glueTouching,
		m_glueTouching
	)
),

RecvPropInt(
	RECVINFO_NAME(
		m_beam.m_viewModelIndex,
		m_viewModelIndex
	)
),

END_RECV_TABLE()


C_BeamQuadratic::C_BeamQuadratic()
{
	m_pOwner = NULL;

	SetIdentityMatrix(m_RenderMatrix);
}


void C_BeamQuadratic::Update(
	C_BaseEntity* pOwner
)
{
	m_pOwner = pOwner;

	if (m_active)
	{
		if (
			m_hRenderHandle ==
			INVALID_CLIENT_RENDER_HANDLE
			)
		{
			ClientLeafSystem()->AddRenderable(
				this,
				RENDER_GROUP_TRANSLUCENT_ENTITY
			);
		}
		else
		{
			ClientLeafSystem()->RenderableChanged(
				m_hRenderHandle
			);
		}
	}
	else if (
		!m_active &&
		m_hRenderHandle !=
		INVALID_CLIENT_RENDER_HANDLE
		)
	{
		ClientLeafSystem()->RemoveRenderable(
			m_hRenderHandle
		);
	}
}


int C_BeamQuadratic::DrawModel(
	int flags
)
{
	Vector points[3];
	QAngle ang;

	if (!m_active)
		return 0;

	C_BaseEntity* pEnt =
		cl_entitylist->GetEnt(
			m_viewModelIndex
		);

	if (!pEnt)
		return 0;

	// Viewmodel attachment
	pEnt->GetAttachment(
		1,
		points[0],
		ang
	);

	// Beam follows held object
	points[2] = m_worldPosition;

	// Direction from gun to object
	Vector dir =
		points[2] - points[0];

	float distance =
		VectorNormalize(dir);

	Vector right, up;
	VectorVectors(dir, right, up);

	float t = gpGlobals->curtime;

	// Base midpoint
	points[1] =
		points[0] +
		dir * (distance * 0.5f);

	// Dynamic curve amount
	float curveScale =
		clamp(
			distance * 0.08f,
			12.0f,
			80.0f
		);

	// Sag downward
	points[1].z -=
		curveScale * 0.4f;

	// Side flex
	points[1] +=
		right *
		sin(t * 6.0f) *
		(curveScale * 0.5f);

	// Vertical wave
	points[1] +=
		up *
		cos(t * 7.0f) *
		(curveScale * 0.35f);

	// Chaotic motion
	points[1].x +=
		sin(t * 18.0f) * 3.0f;

	points[1].y +=
		cos(t * 15.0f) * 3.0f;

	points[1].z +=
		sin(t * 12.0f) * 4.0f;

	IMaterial* pMat =
		materials->FindMaterial(
			"sprites/physbeam",
			TEXTURE_GROUP_CLIENT_EFFECTS
		);

	if (!pMat)
		return 0;

	CMatRenderContextPtr pRenderContext(materials);

	pRenderContext->Bind(pMat);

	Vector color;

	if (m_glueTouching)
	{
		color.Init(
			1.0f,
			0.2f,
			0.2f
		);
	}
	else
	{
		color.Init(
			1.0f,
			0.7f,
			0.2f
		);
	}

	float scrollOffset =
		gpGlobals->curtime -
		(int)gpGlobals->curtime;

	float width =
		13.0f +
		sin(
			gpGlobals->curtime *
			20.0f
		) * 2.0f;

	// Main beam
	DrawBeamQuadratic(
		points[0],
		points[1],
		points[2],
		width,
		color,
		scrollOffset
	);

	// Inner bright beam
	Vector innerColor;
	innerColor.Init(1, 1, 1);

	DrawBeamQuadratic(
		points[0],
		points[1],
		points[2],
		width * 0.35f,
		innerColor,
		-scrollOffset
	);

	// Electric arc beams
	for (int i = 0; i < 3; i++)
	{
		Vector arcMid =
			points[1];

		arcMid +=
			right *
			random->RandomFloat(
				-12,
				12
			);

		arcMid +=
			up *
			random->RandomFloat(
				-12,
				12
			);

		arcMid +=
			dir *
			random->RandomFloat(
				-8,
				8
			);

		DrawBeamQuadratic(
			points[0],
			arcMid,
			points[2],
			2.0f,
			color,
			scrollOffset + i
		);
	}

	// Glow sprite
	IMaterial* pGlow =
		materials->FindMaterial(
			"sprites/physglow",
			TEXTURE_GROUP_CLIENT_EFFECTS
		);

	if (pGlow)
	{
		pRenderContext->Bind(pGlow);

		color32 spriteClr;

		spriteClr.r =
			(unsigned char)(color.x * 255);

		spriteClr.g =
			(unsigned char)(color.y * 255);

		spriteClr.b =
			(unsigned char)(color.z * 255);

		spriteClr.a = 255;

		float scale =
			random->RandomFloat(
				3.0f,
				5.0f
			) * 5.0f;

		// Impact glow
		for (int i = 0; i < 3; i++)
		{
			DrawSprite(
				points[2],
				scale,
				scale,
				spriteClr
			);
		}

		// Muzzle glow
		DrawSprite(
			points[0],
			scale * 0.7f,
			scale * 0.7f,
			spriteClr
		);
	}

	return 1;
}