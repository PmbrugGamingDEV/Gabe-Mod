//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "hud.h"
#include "in_buttons.h"
#include "baseplayer_shared.h"
#include "beamdraw.h"
#include "c_weapon__stubs.h"
#include "clienteffectprecachesystem.h"
#include "c_baseplayer.h"
#include "usercmd.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "glow_overlay.h"
#include "glow_outline_effect.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar physgun_beam_color("jb_physgun_beam_color", "255 255 255", FCVAR_ARCHIVE);
ConVar physgun_model_color("jb_physgun_model_color", "255 0 255", FCVAR_ARCHIVE);


CLIENTEFFECT_REGISTER_BEGIN(PrecacheEffectJBMGravityGun)
CLIENTEFFECT_MATERIAL("sprites/physbeam")
CLIENTEFFECT_REGISTER_END()

class C_JBMBeamQuadratic : public CDefaultClientRenderable
{
public:
	C_JBMBeamQuadratic();
	void			Update(C_BaseEntity* pOwner);

	// IClientRenderable
	virtual const Vector& GetRenderOrigin(void) { return m_worldPosition; }
	virtual const QAngle& GetRenderAngles(void) { return vec3_angle; }
	virtual const					matrix3x4_t& RenderableToWorldTransform();
	virtual bool					ShouldDraw(void) { return true; }
	virtual bool					IsTransparent(void) { return true; }
	virtual bool					ShouldReceiveProjectedTextures(int flags) { return false; }
	virtual int						DrawModel(int flags);

	// Returns the bounds relative to the origin (render bounds)
	virtual void	GetRenderBounds(Vector& mins, Vector& maxs)
	{
		// bogus.  But it should draw if you can see the end point
		mins.Init(-99999, -99999, -99999);
		maxs.Init(99999, 99999, 99999);
	}

	C_BaseEntity* m_pOwner;
	Vector					m_targetPosition;
	Vector					m_worldPosition;
	int						m_active;
	int						m_glueTouching;
};


class C_JBMWeaponGravityGun : public C_BaseCombatWeapon
{
	DECLARE_CLASS(C_JBMWeaponGravityGun, C_BaseCombatWeapon);
public:
	C_JBMWeaponGravityGun() {}

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	int KeyInput(int down, ButtonCode_t keynum, const char* pszCurrentBinding)
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

		// Allow engine to process
		return BaseClass::KeyInput(down, keynum, pszCurrentBinding);
	}

	void CreateMove(float flInputSampleTime, CUserCmd* pCmd, const QAngle& vecOldViewAngles)
	{
		BaseClass::CreateMove(flInputSampleTime, pCmd, vecOldViewAngles);

		if (!pCmd)
			return;

		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if (!pPlayer)
			return;

		if (pPlayer->GetActiveWeapon() != this)
			return;

		if (!m_beam.m_active)
			return;

		if (!(pCmd->buttons & IN_USE))
			return;

		const short nMouseX = pCmd->mousedx;
		const short nMouseY = pCmd->mousedy;
		if (nMouseX == 0 && nMouseY == 0)
			return;

		// Use the raw mouse deltas for server-side physgun rotation, but keep the local
		// camera from turning while we're in rotate mode.
		pCmd->mousedx = nMouseX;
		pCmd->mousedy = nMouseY;
		pCmd->viewangles = vecOldViewAngles;
		QAngle localViewAngles = vecOldViewAngles;
		engine->SetViewAngles(localViewAngles);
	}

	bool OverrideViewAngles(void)
	{
		CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

		if (!pPlayer)
			return false;

		if (m_bIsCurrentlyRotating) {
			return true;
		}

		return false;
	}

	static void ApplyPhysgunModelColor()
	{
		int ir = 255, ig = 0, ib = 255;

		if (sscanf(physgun_model_color.GetString(), "%d %d %d", &ir, &ig, &ib) != 3)
			return;

		ir = clamp(ir, 0, 255);
		ig = clamp(ig, 0, 255);
		ib = clamp(ib, 0, 255);

		float r = ir / 255.0f;
		float g = ig / 255.0f;
		float b = ib / 255.0f;



		// Basically the only way I could really think of doing this:

		IMaterial* pMat = materials->FindMaterial(
			"models/weapons/v_physcannon/v_superphyscannon_sheet",
			TEXTURE_GROUP_MODEL
		);

		if (!pMat || pMat->IsErrorMaterial())

			return;

		bool found = false;
		IMaterialVar* pTint = pMat->FindVar("$selfillumtint", &found);

		if (found && pTint)
		{

			pTint->SetVecValue(r, g, b);
		}
	}





	void OnDataChanged(DataUpdateType_t updateType)
	{


		BaseClass::OnDataChanged(updateType);
		ApplyPhysgunModelColor();

		m_beam.Update(this);
	}

	void UpdateOnRemove(void)
	{
		m_beam.m_active = false;
		m_beam.Update(this);

		BaseClass::UpdateOnRemove();
	}

private:
	C_JBMWeaponGravityGun(const C_JBMWeaponGravityGun&);

	bool m_bIsCurrentlyRotating;

	C_JBMBeamQuadratic	m_beam;
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_jbm_physgun, C_JBMWeaponGravityGun);

IMPLEMENT_CLIENTCLASS_DT(C_JBMWeaponGravityGun, DT_JBMWeaponGravityGun, CJBMWeaponGravityGun)
RecvPropVector(RECVINFO_NAME(m_beam.m_targetPosition, m_targetPosition)),
RecvPropVector(RECVINFO_NAME(m_beam.m_worldPosition, m_worldPosition)),
RecvPropInt(RECVINFO_NAME(m_beam.m_active, m_active)),
RecvPropBool(RECVINFO(m_bIsCurrentlyRotating)),
RecvPropInt(RECVINFO_NAME(m_beam.m_glueTouching, m_glueTouching)),
END_RECV_TABLE()

C_JBMBeamQuadratic::C_JBMBeamQuadratic()
{
	m_pOwner = NULL;
}

void C_JBMBeamQuadratic::Update(C_BaseEntity* pOwner)
{
	m_pOwner = pOwner;
	if (m_active)
	{
		if (m_hRenderHandle == INVALID_CLIENT_RENDER_HANDLE)
		{
			ClientLeafSystem()->AddRenderable(this, RENDER_GROUP_TRANSLUCENT_ENTITY);
		}
		else
		{
			ClientLeafSystem()->RenderableChanged(m_hRenderHandle);
		}
	}
	else if (!m_active && m_hRenderHandle != INVALID_CLIENT_RENDER_HANDLE)
	{
		ClientLeafSystem()->RemoveRenderable(m_hRenderHandle);
	}
}





int C_JBMBeamQuadratic::DrawModel(int flags)
{
	Vector points[3];
	QAngle tmpAngle;

	if (!m_active)
		return 0;

	C_BasePlayer* pOwner =
		ToBasePlayer(m_pOwner->GetOwnerEntity());

	if (!pOwner)
		return 0;

	C_BaseEntity* pEnt =
		pOwner->GetRenderedWeaponModel();

	if (!pEnt)
		return 0;

	pEnt->GetAttachment(
		1,
		points[0],
		tmpAngle
	);

	points[2] = m_worldPosition;

	Vector dir =
		points[2] - points[0];

	float distance =
		VectorNormalize(dir);

	Vector right, up;
	VectorVectors(dir, right, up);

	float t = gpGlobals->curtime;

	points[1] =
		points[0] +
		dir * (distance * 0.5f);

	float curveScale =
		clamp(
			distance * 0.08f,
			12.0f,
			80.0f
		);

	points[1].z -=
		curveScale * 0.4f;

	points[1] +=
		right *
		sin(t * 6.0f) *
		(curveScale * 0.5f);

	points[1] +=
		up *
		cos(t * 7.0f) *
		(curveScale * 0.35f);

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

	Vector color;

	if (m_glueTouching)
	{
		color.Init(
			1.0f,
			0.0f,
			0.0f
		);
	}
	else
	{
		int ir = 255;
		int ig = 255;
		int ib = 255;

		if (sscanf(
			physgun_beam_color.GetString(),
			"%d %d %d",
			&ir,
			&ig,
			&ib
		) != 3)
		{
			ir = ig = ib = 255;
		}

		ir = clamp(ir, 0, 255);
		ig = clamp(ig, 0, 255);
		ib = clamp(ib, 0, 255);

		color.Init(
			ir / 255.0f,
			ig / 255.0f,
			ib / 255.0f
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

	CMatRenderContextPtr pRenderContext(materials);

	pRenderContext->Bind(pMat);

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
	innerColor.Init(1.0f, 1.0f, 1.0f);

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
				-12.0f,
				12.0f
			);

		arcMid +=
			up *
			random->RandomFloat(
				-12.0f,
				12.0f
			);

		arcMid +=
			dir *
			random->RandomFloat(
				-8.0f,
				8.0f
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
			(unsigned char)(color.x * 255.0f);

		spriteClr.g =
			(unsigned char)(color.y * 255.0f);

		spriteClr.b =
			(unsigned char)(color.z * 255.0f);

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

const matrix3x4_t& C_JBMBeamQuadratic::RenderableToWorldTransform()
{
	static matrix3x4_t mat;
	AngleMatrix(GetRenderAngles(), GetRenderOrigin(), mat);
	return mat;
}
