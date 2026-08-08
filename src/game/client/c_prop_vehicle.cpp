//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "c_prop_vehicle.h"
#include "hud.h"		
#include <vgui_controls/Controls.h>
#include <Color.h>
#include "view.h"
#include "engine/ivdebugoverlay.h"
#include "movevars_shared.h"
#include "vgui_all.h"
#include "iviewrender.h"
#include "vgui/ISurface.h"
#include "client_virtualreality.h"
#include "../hud_crosshair.h"
#include "sourcevr/isourcevirtualreality.h"
// NVNT haptic utils
#include "haptics/haptic_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

int ScreenTransform( const Vector& point, Vector& screen );

extern ConVar default_fov;
extern ConVar joy_response_move_vehicle;


IMPLEMENT_CLIENTCLASS_DT(C_PropVehicleDriveable, DT_PropVehicleDriveable, CPropVehicleDriveable)
	RecvPropEHandle( RECVINFO(m_hPlayer) ),
	RecvPropInt( RECVINFO( m_nSpeed ) ),
	RecvPropFloat(RECVINFO(m_flFuel)),
	RecvPropInt( RECVINFO( m_nRPM ) ),
	RecvPropFloat( RECVINFO( m_flThrottle ) ),
	RecvPropInt( RECVINFO( m_nBoostTimeLeft ) ),
	RecvPropInt( RECVINFO( m_nHasBoost ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledWeapons ) ),
	RecvPropInt( RECVINFO( m_nScannerDisabledVehicle ) ),
	RecvPropInt( RECVINFO( m_bEnterAnimOn ) ),
	RecvPropInt( RECVINFO( m_bExitAnimOn ) ),
	RecvPropInt( RECVINFO( m_bUnableToFire ) ),
	RecvPropVector( RECVINFO( m_vecEyeExitEndpoint ) ),
	RecvPropBool( RECVINFO( m_bHasGun ) ),
	RecvPropVector( RECVINFO( m_vecGunCrosshair ) ),
END_RECV_TABLE()


BEGIN_DATADESC( C_PropVehicleDriveable )
	DEFINE_EMBEDDED( m_ViewSmoothingData ),
END_DATADESC()

ConVar r_VehicleViewClamp( "r_VehicleViewClamp", "1", FCVAR_CHEAT );

#define ROLL_CURVE_ZERO		20		// roll less than this is clamped to zero
#define ROLL_CURVE_LINEAR	90		// roll greater than this is copied out

#define PITCH_CURVE_ZERO		10	// pitch less than this is clamped to zero
#define PITCH_CURVE_LINEAR		45	// pitch greater than this is copied out
									// spline in between


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::C_PropVehicleDriveable() :
	m_iv_vecGunCrosshair( "C_PropVehicleDriveable::m_iv_vecGunCrosshair" )

{
	m_hPrevPlayer = NULL;

	memset( &m_ViewSmoothingData, 0, sizeof( m_ViewSmoothingData ) );

	m_ViewSmoothingData.pVehicle = this;
	m_ViewSmoothingData.bClampEyeAngles = true;
	m_ViewSmoothingData.bDampenEyePosition = true;

	m_ViewSmoothingData.flPitchCurveZero = PITCH_CURVE_ZERO;
	m_ViewSmoothingData.flPitchCurveLinear = PITCH_CURVE_LINEAR;
	m_ViewSmoothingData.flRollCurveZero = ROLL_CURVE_ZERO;
	m_ViewSmoothingData.flRollCurveLinear = ROLL_CURVE_LINEAR;

	m_ViewSmoothingData.flFOV = m_flFOV = default_fov.GetFloat();

	AddVar( &m_vecGunCrosshair, &m_iv_vecGunCrosshair, LATCH_SIMULATION_VAR );
}

//-----------------------------------------------------------------------------
// Purpose: De-constructor
//-----------------------------------------------------------------------------
C_PropVehicleDriveable::~C_PropVehicleDriveable()
{
}


//-----------------------------------------------------------------------------
// By default all driveable vehicles use the curve defined by the convar.
//-----------------------------------------------------------------------------
int C_PropVehicleDriveable::GetJoystickResponseCurve() const
{
	return joy_response_move_vehicle.GetInt();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter *C_PropVehicleDriveable::GetPassenger( int nRole )
{
	if ( nRole == VEHICLE_ROLE_DRIVER )
		return m_hPlayer.Get();

	return NULL;
}

//-----------------------------------------------------------------------------
// Returns the role of the passenger
//-----------------------------------------------------------------------------
int	C_PropVehicleDriveable::GetPassengerRole( C_BaseCombatCharacter *pPassenger )
{
	if ( m_hPlayer.Get() == pPassenger )
		return VEHICLE_ROLE_DRIVER;

	return VEHICLE_ROLE_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_hPrevPlayer = m_hPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_hPlayer && !m_hPrevPlayer )
	{
		OnEnteredVehicle( m_hPlayer );
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		g_ClientVirtualReality.AlignTorsoAndViewToWeapon();
	}
	else if ( !m_hPlayer && m_hPrevPlayer )
	{
		// NVNT notify haptics system of navigation exit
		OnExitedVehicle( m_hPrevPlayer );
		// They have just exited the vehicle.
		// Sometimes we never reach the end of our exit anim, such as if the
		// animation doesn't have fadeout 0 specified in the QC, so we fail to
		// catch it in VehicleViewSmoothing. Catch it here instead.
		m_ViewSmoothingData.bWasRunningAnim = false;
		SetNextClientThink( CLIENT_THINK_NEVER );
	}
}

//-----------------------------------------------------------------------------
// Should this object cast render-to-texture shadows?
//-----------------------------------------------------------------------------
ShadowType_t C_PropVehicleDriveable::ShadowCastType()
{
	CStudioHdr *pStudioHdr = GetModelPtr();
	if ( !pStudioHdr )
		return SHADOWS_NONE;

	if ( IsEffectActive(EF_NODRAW | EF_NOSHADOW) )
		return SHADOWS_NONE;

	// Always use render-to-texture. We'll always the dirty bits in our think function
	return SHADOWS_RENDER_TO_TEXTURE;
}


//-----------------------------------------------------------------------------
// Mark the shadow as dirty while the vehicle is being driven
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::ClientThink( void )
{
	// The vehicle is always dirty owing to pose parameters while it's being driven.
	g_pClientShadowMgr->MarkRenderToTextureShadowDirty( GetShadowHandle() );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::DampenEyePosition( Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles )
{
}

//-----------------------------------------------------------------------------
// Purpose: Modify the player view/camera while in a vehicle
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*=NULL*/ )
{
	SharedVehicleViewSmoothing( m_hPlayer,
								pAbsOrigin, pAbsAngles,
								m_bEnterAnimOn, m_bExitAnimOn,
								m_vecEyeExitEndpoint, 
								&m_ViewSmoothingData,
								pFOV );
}


//-----------------------------------------------------------------------------
// Futzes with the clip planes
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::GetVehicleClipPlanes( float &flZNear, float &flZFar ) const
{
	// FIXME: Need something a better long-term, this fixes the buggy.
	flZNear = 6;
}

	
//-----------------------------------------------------------------------------
// Renders hud elements
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Simply used to return intensity value based upon current timer passed in
//-----------------------------------------------------------------------------
int GetFlashColorIntensity( int LowIntensity, int HighIntensity, bool Dimming, int Increment, int Timer )
{
	if ( Dimming ) 
		return ( HighIntensity - Timer * Increment );
	else
		return ( LowIntensity + Timer * Increment );
}

#define	TRIANGULATED_CROSSHAIR 1

void C_PropVehicleDriveable::DrawHudElements( )
{
	CHudTexture *pIcon;
	int iIconX, iIconY;

	if (m_bHasGun)
	{
		// draw crosshairs for vehicle gun
		pIcon = gHUD.GetIcon( "gunhair" );

		if ( pIcon != NULL )
		{
			float x, y;

			if( UseVR() )
			{
				C_BasePlayer *pPlayer = (C_BasePlayer *)GetPassenger( VEHICLE_ROLE_DRIVER );
				Vector vecStart, vecDirection;
				pPlayer->EyePositionAndVectors( &vecStart, &vecDirection, NULL, NULL );
				Vector vecEnd = vecStart + vecDirection * MAX_TRACE_LENGTH;

				trace_t tr;
				UTIL_TraceLine( vecStart, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

				Vector screen;
				screen.Init();
				ScreenTransform(tr.endpos, screen);

				int vx, vy, vw, vh;
				vgui::surface()->GetFullscreenViewport( vx, vy, vw, vh );

				float screenWidth = vw;
				float screenHeight = vh;

				x = 0.5f * ( 1.0f + screen[0] ) * screenWidth + 0.5f;
				y = 0.5f * ( 1.0f - screen[1] ) * screenHeight + 0.5f;
			}
			else
			{
				Vector screen;

				x = ScreenWidth()/2;
				y = ScreenHeight()/2;

#if TRIANGULATED_CROSSHAIR
				ScreenTransform( m_vecGunCrosshair, screen );
				x += 0.5 * screen[0] * ScreenWidth() + 0.5;
				y -= 0.5 * screen[1] * ScreenHeight() + 0.5;
#endif
			}


			x -= pIcon->Width() / 2; 
			y -= pIcon->Height() / 2; 
			
			Color	clr = ( m_bUnableToFire ) ? gHUD.m_clrCaution : gHUD.m_clrNormal;
			pIcon->DrawSelf( x, y, clr );
		}

		if ( m_nScannerDisabledWeapons )
		{
			// Draw icons for scanners "weps disabled"  
			pIcon = gHUD.GetIcon( "dmg_bio" );
			if ( pIcon )
			{
				iIconY = 467 - pIcon->Height() / 2;
				iIconX = 385;
				if ( !m_bScannerWepIcon )
				{
					pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
					m_bScannerWepIcon = true;
					m_iScannerWepFlashTimer = 0;
					m_bScannerWepDim = true;
				}
				else
				{
					pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerWepDim, 10, m_iScannerWepFlashTimer), 255 ) );
					m_iScannerWepFlashTimer++;
					m_iScannerWepFlashTimer %= 20;
					if(!m_iScannerWepFlashTimer)
						m_bScannerWepDim ^= 1;
				}
			}
		}
	}

	if ( m_nScannerDisabledVehicle )
	{
		// Draw icons for scanners "vehicle disabled"  
		pIcon = gHUD.GetIcon( "dmg_bio" );
		if ( pIcon )
		{
			iIconY = 467 - pIcon->Height() / 2;
			iIconX = 410;
			if ( !m_bScannerVehicleIcon )
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, 255, 255 ) );
				m_bScannerVehicleIcon = true;
				m_iScannerVehicleFlashTimer = 0;
				m_bScannerVehicleDim = true;
			}
			else
			{
				pIcon->DrawSelf( XRES(iIconX), YRES(iIconY), Color( 0, 0, GetFlashColorIntensity(55, 255, m_bScannerVehicleDim, 10, m_iScannerVehicleFlashTimer), 255 ) );
				m_iScannerVehicleFlashTimer++;
				m_iScannerVehicleFlashTimer %= 20;
				if(!m_iScannerVehicleFlashTimer)
					m_bScannerVehicleDim ^= 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::RestrictView( float *pYawBounds, float *pPitchBounds,
										   float *pRollBounds, QAngle &vecViewAngles )
{
	int eyeAttachmentIndex = LookupAttachment( "vehicle_driver_eyes" );
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetAttachmentLocal( eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles );

	// Limit the yaw.
	if ( pYawBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.y, vehicleEyeAngles.y );
		flAngleDiff = clamp( flAngleDiff, pYawBounds[0], pYawBounds[1] );
		vecViewAngles.y = vehicleEyeAngles.y + flAngleDiff;
	}

	// Limit the pitch.
	if ( pPitchBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.x, vehicleEyeAngles.x );
		flAngleDiff = clamp( flAngleDiff, pPitchBounds[0], pPitchBounds[1] );
		vecViewAngles.x = vehicleEyeAngles.x + flAngleDiff;
	}

	// Limit the roll.
	if ( pRollBounds )
	{
		float flAngleDiff = AngleDiff( vecViewAngles.z, vehicleEyeAngles.z );
		flAngleDiff = clamp( flAngleDiff, pRollBounds[0], pRollBounds[1] );
		vecViewAngles.z = vehicleEyeAngles.z + flAngleDiff;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::UpdateViewAngles( C_BasePlayer *pLocalPlayer, CUserCmd *pCmd )
{
	if ( r_VehicleViewClamp.GetInt() )
	{
		float pitchBounds[2] = { -85.0f, 25.0f };
		RestrictView( NULL, pitchBounds, NULL, pCmd->viewangles );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_PropVehicleDriveable::OnEnteredVehicle( C_BaseCombatCharacter *pPassenger )
{
#if defined( WIN32 ) && !defined( _X360 )
	// NVNT notify haptics system of navigation change
	HapticsEnteredVehicle(this,pPassenger);
#endif
}

// NVNT - added function
void C_PropVehicleDriveable::OnExitedVehicle( C_BaseCombatCharacter *pPassenger )
{
#if defined( WIN32 ) && !defined( _X360 )
	// NVNT notify haptics system of navigation exit
	HapticsExitedVehicle(this,pPassenger);
#endif
}

DECLARE_HUDELEMENT(CHudVehicleStats);


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CHudVehicleStats::CHudVehicleStats(const char* pElementName)
	: CHudElement(pElementName),
	BaseClass(NULL, "HudVehicleStats")
{
	SetParent(g_pClientMode->GetViewport());

	SetHiddenBits(HIDEHUD_PLAYERDEAD);

	m_pVehicle = NULL;
	m_pOwner = NULL;
}


//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------
void CHudVehicleStats::Init()
{
	m_pVehicle = NULL;
	m_pOwner = NULL;
}


//-----------------------------------------------------------------------------
// VidInit
//-----------------------------------------------------------------------------
void CHudVehicleStats::VidInit()
{
	m_pVehicle = NULL;
	m_pOwner = NULL;

	hFont = vgui::scheme()->GetIScheme(
		vgui::scheme()->GetDefaultScheme()
	)->GetFont("Default", true);
}


//-----------------------------------------------------------------------------
// ShouldDraw
//-----------------------------------------------------------------------------
bool CHudVehicleStats::ShouldDraw()
{
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer)
		return false;

	if (!pPlayer->IsInAVehicle())
		return false;

	C_PropVehicleDriveable* pVehicle =
		dynamic_cast<C_PropVehicleDriveable*>(pPlayer->GetVehicle());

	if (!pVehicle)
		return false;

	return CHudElement::ShouldDraw();
}


//-----------------------------------------------------------------------------
// OnThink
//-----------------------------------------------------------------------------
void CHudVehicleStats::OnThink()
{
	m_pVehicle = NULL;
	m_pOwner = NULL;

	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer)
		return;

	if (!pPlayer->IsInAVehicle())
		return;

	m_pVehicle =
		dynamic_cast<C_PropVehicleDriveable*>(pPlayer->GetVehicle());

	if (!m_pVehicle)
		return;

	m_pOwner = pPlayer;
}


//-----------------------------------------------------------------------------
// Paint
//-----------------------------------------------------------------------------
void CHudVehicleStats::Paint()
{
	if (!m_pVehicle)
		return;

	// ---------------------------------------------------------
	// VEHICLE VALUES
	// ---------------------------------------------------------

	float flFuel = m_pVehicle->GetFuelAmt();
	int nSpeed = m_pVehicle->GetSpeed();

	float flFuelFraction = clamp(flFuel / 100.0f, 0.0f, 1.0f);


	// ---------------------------------------------------------
	// PANEL BACKGROUND
	// ---------------------------------------------------------

	int wide, tall;
	GetSize(wide, tall);

	surface()->DrawSetColor(0, 0, 0, 180);
	surface()->DrawFilledRect(
		0,
		0,
		wide,
		tall
	);


	// ---------------------------------------------------------
	// BORDER
	// ---------------------------------------------------------

	surface()->DrawSetColor(255, 255, 255, 180);
	surface()->DrawOutlinedRect(
		0,
		0,
		wide,
		tall
	);


	// ---------------------------------------------------------
	// FONT
	// ---------------------------------------------------------

	if (hFont == INVALID_FONT)
		return;


	// ---------------------------------------------------------
	// SPEED
	// ---------------------------------------------------------

	wchar_t wszSpeed[64];

	V_snwprintf(
		wszSpeed,
		ARRAYSIZE(wszSpeed),
		L"Speed: %d MPH",
		nSpeed
	);

	surface()->DrawSetTextFont(hFont);
	surface()->DrawSetTextColor(255, 255, 255, 255);
	surface()->DrawSetTextPos(10, 10);

	surface()->DrawPrintText(
		wszSpeed,
		V_wcslen(wszSpeed)
	);


	// ---------------------------------------------------------
	// FUEL LABEL
	// ---------------------------------------------------------

	wchar_t wszFuel[64];

	V_snwprintf(
		wszFuel,
		ARRAYSIZE(wszFuel),
		L"Fuel: %.0f%%",
		flFuel
	);

	surface()->DrawSetTextFont(hFont);
	surface()->DrawSetTextColor(255, 255, 255, 255);
	surface()->DrawSetTextPos(10, 40);

	surface()->DrawPrintText(
		wszFuel,
		V_wcslen(wszFuel)
	);


	// ---------------------------------------------------------
	// FUEL BAR
	// ---------------------------------------------------------

	int fuelX = 10;
	int fuelY = 65;

	int fuelBarWide = wide - 20;
	int fuelBarTall = 12;

	// Background
	surface()->DrawSetColor(40, 40, 40, 255);

	surface()->DrawFilledRect(
		fuelX,
		fuelY,
		fuelX + fuelBarWide,
		fuelY + fuelBarTall
	);

	// Filled portion
	int fuelFillWide =
		(int)(fuelBarWide * flFuelFraction);

	surface()->DrawSetColor(255, 255, 255, 255);

	surface()->DrawFilledRect(
		fuelX,
		fuelY,
		fuelX + fuelFillWide,
		fuelY + fuelBarTall
	);

	// Border
	surface()->DrawSetColor(255, 255, 255, 255);

	surface()->DrawOutlinedRect(
		fuelX,
		fuelY,
		fuelX + fuelBarWide,
		fuelY + fuelBarTall
	);
}

//=============================================================================================
// VEHICLE VIEW SMOOTHING. See iclientvehicle.h for details.
//=============================================================================================

//-----------------------------------------------------------------------------
// Purpose: For a given degree of freedom, blends between the raw and clamped
//			view depending on this vehicle's preferences. When vehicles wreck
//			catastrophically, it's often better to lock the view for a little
//			while until things settle down than to keep trying to clamp/flatten
//			the view artificially because we can never really catch up with
//			the chaotic flipping.
//-----------------------------------------------------------------------------
extern float ApplyViewLocking(float flAngleRaw, float flAngleClamped, ViewLockData_t& lockData, RemapAngleRange_CurvePart_t eCurvePart);

extern float RemapAngleRange(float startInterval, float endInterval, float value);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pData - 
//			vehicleEyeAngles - 
//-----------------------------------------------------------------------------
extern void RemapViewAngles(ViewSmoothingData_t* pData, QAngle& vehicleEyeAngles);

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAbsOrigin - 
//			*pAbsAngles - 
//			bEnterAnimOn - 
//			bExitAnimOn - 
//			*vecEyeExitEndpoint - 
//			*pData - 
//-----------------------------------------------------------------------------
void VehicleViewSmoothing(CBasePlayer* pPlayer, Vector* pAbsOrigin, QAngle* pAbsAngles, bool bEnterAnimOn, bool bExitAnimOn, Vector* vecEyeExitEndpoint, ViewSmoothingData_t* pData,
	float* pFOV)
{
	int eyeAttachmentIndex = pData->pVehicle->LookupAttachment("vehicle_driver_eyes");
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	pData->pVehicle->GetAttachment(eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles);
	AngleMatrix(vehicleEyeAngles, vehicleEyePosToWorld);

	// Dampen the eye positional change as we drive around.
	*pAbsAngles = pPlayer->EyeAngles();
	if (r_VehicleViewDampen.GetInt() && pData->bDampenEyePosition)
	{
		C_PropVehicleDriveable* pDriveable = assert_cast<C_PropVehicleDriveable*>(pData->pVehicle);
		pDriveable->DampenEyePosition(vehicleEyeOrigin, vehicleEyeAngles);
	}

	// Started running an entry or exit anim?
	bool bRunningAnim = (bEnterAnimOn || bExitAnimOn);
	if (bRunningAnim && !pData->bWasRunningAnim)
	{
		pData->bRunningEnterExit = true;
		pData->flAnimTimeElapsed = 0.01;
		pData->flEnterExitDuration = pData->pVehicle->SequenceDuration(pData->pVehicle->GetSequence());

		pData->vecAnglesSaved = PrevMainViewAngles();
		pData->vecOriginSaved = PrevMainViewOrigin();

		// Save our initial angular error, which we will blend out over the length of the animation.
		pData->vecAngleDiffSaved.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		pData->vecAngleDiffSaved.y = AngleDiff(vehicleEyeAngles.y, pData->vecAnglesSaved.y);
		pData->vecAngleDiffSaved.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		pData->vecAngleDiffMin = pData->vecAngleDiffSaved;
	}

	pData->bWasRunningAnim = bRunningAnim;

	float frac = 0;
	float flFracFOV = 0;

	// If we're in an enter/exit animation, blend the player's eye angles to the attachment's
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		*pAbsAngles = vehicleEyeAngles;

		// Forward integrate to determine the elapsed time in this entry/exit anim.
		frac = pData->flAnimTimeElapsed / pData->flEnterExitDuration;
		frac = clamp(frac, 0.0f, 1.0f);

		flFracFOV = pData->flAnimTimeElapsed / (pData->flEnterExitDuration * 0.85f);
		flFracFOV = clamp(flFracFOV, 0.0f, 1.0f);

		//Msg("Frac: %f\n", frac );

		if (frac < 1.0)
		{
			// Blend to the desired vehicle eye origin
			//Vector vecToView = (vehicleEyeOrigin - PrevMainViewOrigin());
			//vehicleEyeOrigin = PrevMainViewOrigin() + (vecToView * SimpleSpline(frac));
			//debugoverlay->AddBoxOverlay( vehicleEyeOrigin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 0,255,255, 64, 10 );
		}
		else
		{
			pData->bRunningEnterExit = false;

			// Enter animation has finished, align view with the eye attachment point
			// so they can start mouselooking around.
			if (!bExitAnimOn)
			{
				Vector localEyeOrigin;
				QAngle localEyeAngles;

				pData->pVehicle->GetAttachmentLocal(eyeAttachmentIndex, localEyeOrigin, localEyeAngles);
				engine->SetViewAngles(localEyeAngles);
			}
		}

		pData->flAnimTimeElapsed += gpGlobals->frametime;
	}

	// Compute the relative rotation between the unperturbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix(*pAbsAngles, cameraToWorld);

	matrix3x4_t worldToEyePos;
	MatrixInvert(vehicleEyePosToWorld, worldToEyePos);

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms(worldToEyePos, cameraToWorld, vehicleCameraToEyePos);

	// Damp out some of the vehicle motion (neck/head would do this)
	if (pData->bClampEyeAngles)
	{
		RemapViewAngles(pData, vehicleEyeAngles);
	}

	AngleMatrix(vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld);

	// Now treat the relative eye angles as being relative to this new, perturbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms(vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld);

	// output new view abs angles
	MatrixAngles(newCameraToWorld, *pAbsAngles);

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn(newCameraToWorld, 3, *pAbsOrigin);

	// If we're playing an extry or exit animation...
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		float flSplineFrac = clamp(SimpleSpline(frac), 0, 1);

		// Blend out the error between the player's initial eye angles and the animation's initial
		// eye angles over the duration of the animation. 
		QAngle vecAngleDiffBlend = ((1 - flSplineFrac) * pData->vecAngleDiffSaved);

		// If our current error is less than the error amount that we're blending 
		// out, use that. This lets the angles converge as quickly as possible.
		QAngle vecAngleDiffCur;
		vecAngleDiffCur.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		vecAngleDiffCur.y = AngleDiff(vehicleEyeAngles.y, pData->vecAnglesSaved.y);
		vecAngleDiffCur.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		// In either case, never increase the error, so track the minimum error and clamp to that.
		for (int i = 0; i < 3; i++)
		{
			if (fabs(vecAngleDiffCur[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffCur[i];
			}

			if (fabs(vecAngleDiffBlend[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffBlend[i];
			}
		}

		// Add the error to the animation's eye angles.
		*pAbsAngles -= pData->vecAngleDiffMin;

		// Use this as the basis for the next error calculation.
		pData->vecAnglesSaved = *pAbsAngles;

		//if ( gpGlobals->frametime )
		//{
		//	Msg("Angle : %.2f %.2f %.2f\n", target.x, target.y, target.z );
		//}
		//Msg("Prev: %.2f %.2f %.2f\n", pData->vecAnglesSaved.x, pData->vecAnglesSaved.y, pData->vecAnglesSaved.z );

		Vector vecAbsOrigin = *pAbsOrigin;

		// If we're exiting, our desired position is the server-sent exit position
		if (bExitAnimOn)
		{
			//debugoverlay->AddBoxOverlay( vecEyeExitEndpoint, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 255,255,255, 64, 10 );

			// Blend to the exit position
			*pAbsOrigin = Lerp(flSplineFrac, vecAbsOrigin, *vecEyeExitEndpoint);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, pData->flFOV, default_fov.GetFloat());
			}
		}
		else
		{
			// Blend from our starting position to the desired origin
			*pAbsOrigin = Lerp(flSplineFrac, pData->vecOriginSaved, vecAbsOrigin);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, default_fov.GetFloat(), pData->flFOV);
			}
		}
	}
	else if (pFOV)
	{
		// Not running an entry/exit anim. Just use the vehicle's FOV.
		*pFOV = pData->flFOV;
	}
}
//TheQuartz

void VehicleViewSmoothingAPC(CBasePlayer* pPlayer, Vector* pAbsOrigin, QAngle* pAbsAngles, bool bEnterAnimOn, bool bExitAnimOn, Vector* vecEyeExitEndpoint, ViewSmoothingData_t* pData,
	float* pFOV)
{
	int eyeAttachmentIndex = pData->pVehicle->LookupAttachment("cannon_muzzle");
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	pData->pVehicle->GetAttachment(eyeAttachmentIndex, vehicleEyeOrigin, vehicleEyeAngles);
	Vector up, forward;
	pData->pVehicle->GetVectors(NULL, &forward, &up);
	vehicleEyeOrigin += (forward * 37) + (up * 35);
	AngleMatrix(vehicleEyeAngles, vehicleEyePosToWorld);

	// Dampen the eye positional change as we drive around.
	*pAbsAngles = pPlayer->EyeAngles();
	if (r_VehicleViewDampen.GetInt() && pData->bDampenEyePosition)
	{
		C_PropVehicleDriveable* pDriveable = assert_cast<C_PropVehicleDriveable*>(pData->pVehicle);
		pDriveable->DampenEyePosition(vehicleEyeOrigin, vehicleEyeAngles);
	}

	// Started running an entry or exit anim?
	bool bRunningAnim = pData->bBlendAnglesAPC;
	//bool bRunningAnim = false;

	if (bRunningAnim && !pData->bWasRunningAnim)
	{

		pData->flAPCTimeToStart = gpGlobals->curtime + 2.0f; // Le daremos dos segundos para que piense

		pData->bRunningEnterExit = true;
		pData->flAnimTimeElapsed = 0.01;
		pData->flEnterExitDuration = 2.0f;

		pData->vecAnglesSaved = PrevMainViewAngles();
		pData->vecOriginSaved = PrevMainViewOrigin();

		// Save our initial angular error, which we will blend out over the length of the animation.
		pData->vecAngleDiffSaved.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		pData->vecAngleDiffSaved.y = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		pData->vecAngleDiffSaved.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		pData->vecAngleDiffMin = pData->vecAngleDiffSaved;
	}

	pData->bWasRunningAnim = bRunningAnim;

	float frac = 0;
	float flFracFOV = 0;

	// If we're in an enter/exit animation, blend the player's eye angles to the attachment's
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		*pAbsAngles = vehicleEyeAngles;

		// Forward integrate to determine the elapsed time in this entry/exit anim.
		frac = pData->flAnimTimeElapsed / pData->flEnterExitDuration;
		frac = clamp(frac, 0.0f, 1.0f);

		flFracFOV = pData->flAnimTimeElapsed / (pData->flEnterExitDuration * 0.85f);
		flFracFOV = clamp(flFracFOV, 0.0f, 1.0f);

		//Msg("Frac: %f\n", frac );

		if (frac < 1.0)
		{
			// Blend to the desired vehicle eye origin
			//Vector vecToView = (vehicleEyeOrigin - PrevMainViewOrigin());
			//vehicleEyeOrigin = PrevMainViewOrigin() + (vecToView * SimpleSpline(frac));
			//debugoverlay->AddBoxOverlay( vehicleEyeOrigin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 0,255,255, 64, 10 );
		}
		else
		{
			pData->bRunningEnterExit = false;

			// Enter animation has finished, align view with the eye attachment point
			// so they can start mouselooking around.
			if (!bExitAnimOn)
			{
				Vector localEyeOrigin;
				QAngle localEyeAngles;

				pData->pVehicle->GetAttachmentLocal(eyeAttachmentIndex, localEyeOrigin, localEyeAngles);
				//localEyeAngles.z=0;
				engine->SetViewAngles(localEyeAngles);
			}
		}

		pData->flAnimTimeElapsed += gpGlobals->frametime;
		if (pData->flAPCTimeToStart <= gpGlobals->curtime) {
			pData->bBlendAnglesAPC = false; // Funciona a las mil maravillas
		}
	}

	// Compute the relative rotation between the unperturbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix(*pAbsAngles, cameraToWorld);

	matrix3x4_t worldToEyePos;
	MatrixInvert(vehicleEyePosToWorld, worldToEyePos);

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms(worldToEyePos, cameraToWorld, vehicleCameraToEyePos);

	// Damp out some of the vehicle motion (neck/head would do this)
	if (pData->bClampEyeAngles)
	{
		RemapViewAngles(pData, vehicleEyeAngles);
	}

	AngleMatrix(vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld);

	// Now treat the relative eye angles as being relative to this new, perturbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms(vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld);

	// output new view abs angles
	MatrixAngles(newCameraToWorld, *pAbsAngles);

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn(newCameraToWorld, 3, *pAbsOrigin);

	// If we're playing an extry or exit animation...
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		float flSplineFrac = clamp(SimpleSpline(frac), 0, 1);

		// Blend out the error between the player's initial eye angles and the animation's initial
		// eye angles over the duration of the animation. 
		QAngle vecAngleDiffBlend = ((1 - flSplineFrac) * pData->vecAngleDiffSaved);

		// If our current error is less than the error amount that we're blending 
		// out, use that. This lets the angles converge as quickly as possible.
		QAngle vecAngleDiffCur;
		vecAngleDiffCur.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		vecAngleDiffCur.y = AngleDiff(vehicleEyeAngles.y, pData->vecAnglesSaved.y);
		vecAngleDiffCur.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		// In either case, never increase the error, so track the minimum error and clamp to that.
		for (int i = 0; i < 3; i++)
		{
			if (fabs(vecAngleDiffCur[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffCur[i];
			}

			if (fabs(vecAngleDiffBlend[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffBlend[i];
			}
		}

		// Add the error to the animation's eye angles.
		*pAbsAngles -= pData->vecAngleDiffMin;

		// Use this as the basis for the next error calculation.
		pData->vecAnglesSaved = *pAbsAngles;

		//if ( gpGlobals->frametime )
		//{
		//	Msg("Angle : %.2f %.2f %.2f\n", target.x, target.y, target.z );
		//}
		//Msg("Prev: %.2f %.2f %.2f\n", pData->vecAnglesSaved.x, pData->vecAnglesSaved.y, pData->vecAnglesSaved.z );

		Vector vecAbsOrigin = *pAbsOrigin;

		// If we're exiting, our desired position is the server-sent exit position
		if (bExitAnimOn)
		{
			//debugoverlay->AddBoxOverlay( vecEyeExitEndpoint, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 255,255,255, 64, 10 );

			// Blend to the exit position
			*pAbsOrigin = Lerp(flSplineFrac, vecAbsOrigin, *vecEyeExitEndpoint);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, pData->flFOV, default_fov.GetFloat());
			}
		}
		else
		{
			// Blend from our starting position to the desired origin
			*pAbsOrigin = Lerp(flSplineFrac, pData->vecOriginSaved, vecAbsOrigin);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, default_fov.GetFloat(), pData->flFOV);
			}
		}

	}
	else if (pFOV)
	{
		// Not running an entry/exit anim. Just use the vehicle's FOV.
		*pFOV = pData->flFOV;
	}
}

void VehicleViewSmoothingSTR(CBasePlayer* pPlayer, Vector* pAbsOrigin, QAngle* pAbsAngles, bool bEnterAnimOn, bool bExitAnimOn, Vector* vecEyeExitEndpoint, ViewSmoothingData_t* pData,
	float* pFOV)
{
	int eyeAttachmentIndex = pData->pVehicle->LookupAttachment("vehicle_driver_eyes");
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle no;
	QAngle vehicleEyeAngles;
	//vehicleEyeOrigin=pData->pVehicle->GetAbsOrigin();
	pData->pVehicle->GetAttachment(eyeAttachmentIndex, vehicleEyeOrigin, no);
	Vector up, forward, right;
	pData->pVehicle->GetVectors(&forward, &right, &up);
	vehicleEyeOrigin += (forward * 120) + (up * -10);
	vehicleEyeAngles = pData->pVehicle->GetAbsAngles();
	AngleMatrix(vehicleEyeAngles, vehicleEyePosToWorld);
	pData->pVehicle->InvalidateBoneCache(); // PARA QUE NO TIEMBLE TODO
	// Dampen the eye positional change as we drive around.
	*pAbsAngles = pPlayer->EyeAngles();
	if (r_VehicleViewDampen.GetInt() && pData->bDampenEyePosition)
	{
		C_PropVehicleDriveable* pDriveable = assert_cast<C_PropVehicleDriveable*>(pData->pVehicle);
		pDriveable->DampenEyePosition(vehicleEyeOrigin, vehicleEyeAngles);
	}

	// Started running an entry or exit anim?
	bool bRunningAnim = pData->bBlendAnglesAPC;
	//bool bRunningAnim = false;

	if (bRunningAnim && !pData->bWasRunningAnim)
	{

		pData->flAPCTimeToStart = gpGlobals->curtime + 2.0f; // Le daremos dos segundos para que piense

		pData->bRunningEnterExit = true;
		pData->flAnimTimeElapsed = 0.01;
		pData->flEnterExitDuration = 2.0f;

		pData->vecAnglesSaved = PrevMainViewAngles();
		pData->vecOriginSaved = PrevMainViewOrigin();

		// Save our initial angular error, which we will blend out over the length of the animation.
		pData->vecAngleDiffSaved.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		pData->vecAngleDiffSaved.y = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		pData->vecAngleDiffSaved.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		pData->vecAngleDiffMin = pData->vecAngleDiffSaved;
	}

	pData->bWasRunningAnim = bRunningAnim;

	float frac = 0;
	float flFracFOV = 0;

	// If we're in an enter/exit animation, blend the player's eye angles to the attachment's
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		*pAbsAngles = vehicleEyeAngles;

		// Forward integrate to determine the elapsed time in this entry/exit anim.
		frac = pData->flAnimTimeElapsed / pData->flEnterExitDuration;
		frac = clamp(frac, 0.0f, 1.0f);

		flFracFOV = pData->flAnimTimeElapsed / (pData->flEnterExitDuration * 0.85f);
		flFracFOV = clamp(flFracFOV, 0.0f, 1.0f);

		//Msg("Frac: %f\n", frac );

		if (frac < 1.0)
		{
			// Blend to the desired vehicle eye origin
			//Vector vecToView = (vehicleEyeOrigin - PrevMainViewOrigin());
			//vehicleEyeOrigin = PrevMainViewOrigin() + (vecToView * SimpleSpline(frac));
			//debugoverlay->AddBoxOverlay( vehicleEyeOrigin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 0,255,255, 64, 10 );
		}
		else
		{
			pData->bRunningEnterExit = false;

			// Enter animation has finished, align view with the eye attachment point
			// so they can start mouselooking around.
			if (!bExitAnimOn)
			{
				Vector localEyeOrigin;
				QAngle localEyeAngles;
				QAngle no;
				pData->pVehicle->GetAttachmentLocal(eyeAttachmentIndex, localEyeOrigin, no);
				//localEyeAngles.z=0;
				localEyeAngles = pData->pVehicle->GetAbsAngles();
				engine->SetViewAngles(localEyeAngles);
			}
		}

		pData->flAnimTimeElapsed += gpGlobals->frametime;
		if (pData->flAPCTimeToStart <= gpGlobals->curtime) {
			pData->bBlendAnglesAPC = false; // Funciona a las mil maravillas
		}
	}

	// Compute the relative rotation between the unperturbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix(*pAbsAngles, cameraToWorld);

	matrix3x4_t worldToEyePos;
	MatrixInvert(vehicleEyePosToWorld, worldToEyePos);

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms(worldToEyePos, cameraToWorld, vehicleCameraToEyePos);

	// Damp out some of the vehicle motion (neck/head would do this)
	if (pData->bClampEyeAngles)
	{
		RemapViewAngles(pData, vehicleEyeAngles);
	}

	AngleMatrix(vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld);

	// Now treat the relative eye angles as being relative to this new, perturbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms(vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld);

	// output new view abs angles
	MatrixAngles(newCameraToWorld, *pAbsAngles);

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn(newCameraToWorld, 3, *pAbsOrigin);

	// If we're playing an extry or exit animation...
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		float flSplineFrac = clamp(SimpleSpline(frac), 0, 1);

		// Blend out the error between the player's initial eye angles and the animation's initial
		// eye angles over the duration of the animation. 
		QAngle vecAngleDiffBlend = ((1 - flSplineFrac) * pData->vecAngleDiffSaved);

		// If our current error is less than the error amount that we're blending 
		// out, use that. This lets the angles converge as quickly as possible.
		QAngle vecAngleDiffCur;
		vecAngleDiffCur.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		vecAngleDiffCur.y = AngleDiff(vehicleEyeAngles.y, pData->vecAnglesSaved.y);
		vecAngleDiffCur.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		// In either case, never increase the error, so track the minimum error and clamp to that.
		for (int i = 0; i < 3; i++)
		{
			if (fabs(vecAngleDiffCur[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffCur[i];
			}

			if (fabs(vecAngleDiffBlend[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffBlend[i];
			}
		}

		// Add the error to the animation's eye angles.
		*pAbsAngles -= pData->vecAngleDiffMin;

		// Use this as the basis for the next error calculation.
		pData->vecAnglesSaved = *pAbsAngles;

		//if ( gpGlobals->frametime )
		//{
		//	Msg("Angle : %.2f %.2f %.2f\n", target.x, target.y, target.z );
		//}
		//Msg("Prev: %.2f %.2f %.2f\n", pData->vecAnglesSaved.x, pData->vecAnglesSaved.y, pData->vecAnglesSaved.z );

		Vector vecAbsOrigin = *pAbsOrigin;

		// If we're exiting, our desired position is the server-sent exit position
		if (bExitAnimOn)
		{
			//debugoverlay->AddBoxOverlay( vecEyeExitEndpoint, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 255,255,255, 64, 10 );

			// Blend to the exit position
			*pAbsOrigin = Lerp(flSplineFrac, vecAbsOrigin, *vecEyeExitEndpoint);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, pData->flFOV, default_fov.GetFloat());
			}
		}
		else
		{
			// Blend from our starting position to the desired origin
			*pAbsOrigin = Lerp(flSplineFrac, pData->vecOriginSaved, vecAbsOrigin);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, default_fov.GetFloat(), pData->flFOV);
			}
		}

	}
	else if (pFOV)
	{
		// Not running an entry/exit anim. Just use the vehicle's FOV.
		*pFOV = pData->flFOV;
	}
}

void VehicleViewSmoothingHLC(CBasePlayer* pPlayer, Vector* pAbsOrigin, QAngle* pAbsAngles, bool bEnterAnimOn, bool bExitAnimOn, Vector* vecEyeExitEndpoint, ViewSmoothingData_t* pData,
	float* pFOV)
{
	int eyeAttachmentIndex = pData->pVehicle->LookupAttachment("Bomb");
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle no;
	QAngle vehicleEyeAngles;
	pData->pVehicle->GetAttachment(eyeAttachmentIndex, vehicleEyeOrigin, no);
	Vector up, forward, right;
	pData->pVehicle->GetVectors(&forward, &right, &up);
	vehicleEyeOrigin += (forward * 215) + (up * 10);
	vehicleEyeAngles = pData->pVehicle->GetAbsAngles();
	AngleMatrix(vehicleEyeAngles, vehicleEyePosToWorld);

	// Dampen the eye positional change as we drive around.
	*pAbsAngles = pPlayer->EyeAngles();
	if (r_VehicleViewDampen.GetInt() && pData->bDampenEyePosition)
	{
		C_PropVehicleDriveable* pDriveable = assert_cast<C_PropVehicleDriveable*>(pData->pVehicle);
		pDriveable->DampenEyePosition(vehicleEyeOrigin, vehicleEyeAngles);
	}

	// Started running an entry or exit anim?
	bool bRunningAnim = pData->bBlendAnglesAPC;
	//bool bRunningAnim = false;

	if (bRunningAnim && !pData->bWasRunningAnim)
	{

		pData->flAPCTimeToStart = gpGlobals->curtime + 2.0f; // Le daremos dos segundos para que piense

		pData->bRunningEnterExit = true;
		pData->flAnimTimeElapsed = 0.01;
		pData->flEnterExitDuration = 2.0f;

		pData->vecAnglesSaved = PrevMainViewAngles();
		pData->vecOriginSaved = PrevMainViewOrigin();

		// Save our initial angular error, which we will blend out over the length of the animation.
		pData->vecAngleDiffSaved.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		pData->vecAngleDiffSaved.y = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		pData->vecAngleDiffSaved.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		pData->vecAngleDiffMin = pData->vecAngleDiffSaved;
	}

	pData->bWasRunningAnim = bRunningAnim;

	float frac = 0;
	float flFracFOV = 0;

	// If we're in an enter/exit animation, blend the player's eye angles to the attachment's
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		*pAbsAngles = vehicleEyeAngles;

		// Forward integrate to determine the elapsed time in this entry/exit anim.
		frac = pData->flAnimTimeElapsed / pData->flEnterExitDuration;
		frac = clamp(frac, 0.0f, 1.0f);

		flFracFOV = pData->flAnimTimeElapsed / (pData->flEnterExitDuration * 0.85f);
		flFracFOV = clamp(flFracFOV, 0.0f, 1.0f);

		//Msg("Frac: %f\n", frac );

		if (frac < 1.0)
		{
			// Blend to the desired vehicle eye origin
			//Vector vecToView = (vehicleEyeOrigin - PrevMainViewOrigin());
			//vehicleEyeOrigin = PrevMainViewOrigin() + (vecToView * SimpleSpline(frac));
			//debugoverlay->AddBoxOverlay( vehicleEyeOrigin, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 0,255,255, 64, 10 );
		}
		else
		{
			pData->bRunningEnterExit = false;

			// Enter animation has finished, align view with the eye attachment point
			// so they can start mouselooking around.
			if (!bExitAnimOn)
			{
				Vector localEyeOrigin;
				QAngle localEyeAngles;
				QAngle no;
				pData->pVehicle->GetAttachmentLocal(eyeAttachmentIndex, localEyeOrigin, no);
				//localEyeAngles.z=0;
				localEyeAngles = pData->pVehicle->GetAbsAngles();
				engine->SetViewAngles(localEyeAngles);
			}
		}

		pData->flAnimTimeElapsed += gpGlobals->frametime;
		if (pData->flAPCTimeToStart <= gpGlobals->curtime) {
			pData->bBlendAnglesAPC = false; // Funciona a las mil maravillas
		}
	}

	// Compute the relative rotation between the unperturbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix(*pAbsAngles, cameraToWorld);

	matrix3x4_t worldToEyePos;
	MatrixInvert(vehicleEyePosToWorld, worldToEyePos);

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms(worldToEyePos, cameraToWorld, vehicleCameraToEyePos);

	// Damp out some of the vehicle motion (neck/head would do this)
	if (pData->bClampEyeAngles)
	{
		RemapViewAngles(pData, vehicleEyeAngles);
	}

	AngleMatrix(vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld);

	// Now treat the relative eye angles as being relative to this new, perturbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms(vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld);

	// output new view abs angles
	MatrixAngles(newCameraToWorld, *pAbsAngles);

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn(newCameraToWorld, 3, *pAbsOrigin);

	// If we're playing an extry or exit animation...
	if (bRunningAnim || pData->bRunningEnterExit)
	{
		float flSplineFrac = clamp(SimpleSpline(frac), 0, 1);

		// Blend out the error between the player's initial eye angles and the animation's initial
		// eye angles over the duration of the animation. 
		QAngle vecAngleDiffBlend = ((1 - flSplineFrac) * pData->vecAngleDiffSaved);

		// If our current error is less than the error amount that we're blending 
		// out, use that. This lets the angles converge as quickly as possible.
		QAngle vecAngleDiffCur;
		vecAngleDiffCur.x = AngleDiff(vehicleEyeAngles.x, pData->vecAnglesSaved.x);
		vecAngleDiffCur.y = AngleDiff(vehicleEyeAngles.y, pData->vecAnglesSaved.y);
		vecAngleDiffCur.z = AngleDiff(vehicleEyeAngles.z, pData->vecAnglesSaved.z);

		// In either case, never increase the error, so track the minimum error and clamp to that.
		for (int i = 0; i < 3; i++)
		{
			if (fabs(vecAngleDiffCur[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffCur[i];
			}

			if (fabs(vecAngleDiffBlend[i]) < fabs(pData->vecAngleDiffMin[i]))
			{
				pData->vecAngleDiffMin[i] = vecAngleDiffBlend[i];
			}
		}

		// Add the error to the animation's eye angles.
		*pAbsAngles -= pData->vecAngleDiffMin;

		// Use this as the basis for the next error calculation.
		pData->vecAnglesSaved = *pAbsAngles;

		//if ( gpGlobals->frametime )
		//{
		//	Msg("Angle : %.2f %.2f %.2f\n", target.x, target.y, target.z );
		//}
		//Msg("Prev: %.2f %.2f %.2f\n", pData->vecAnglesSaved.x, pData->vecAnglesSaved.y, pData->vecAnglesSaved.z );

		Vector vecAbsOrigin = *pAbsOrigin;

		// If we're exiting, our desired position is the server-sent exit position
		if (bExitAnimOn)
		{
			//debugoverlay->AddBoxOverlay( vecEyeExitEndpoint, -Vector(1,1,1), Vector(1,1,1), vec3_angle, 255,255,255, 64, 10 );

			// Blend to the exit position
			*pAbsOrigin = Lerp(flSplineFrac, vecAbsOrigin, *vecEyeExitEndpoint);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, pData->flFOV, default_fov.GetFloat());
			}
		}
		else
		{
			// Blend from our starting position to the desired origin
			*pAbsOrigin = Lerp(flSplineFrac, pData->vecOriginSaved, vecAbsOrigin);
			if ((pData->flFOV != 0.0f) && pFOV)
			{
				*pFOV = Lerp(flFracFOV, default_fov.GetFloat(), pData->flFOV);
			}
		}

	}
	else if (pFOV)
	{
		// Not running an entry/exit anim. Just use the vehicle's FOV.
		*pFOV = pData->flFOV;
	}
}
