//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "takedamageinfo.h"
#include "weapon_sdkbase.h"
#include "ammodef.h"


#if defined( CLIENT_DLL )

	#include "c_hl2mp_player.h"

#else

	#include "hl2mp_player.h"

#endif


// ----------------------------------------------------------------------------- //
// Global functions.
// ----------------------------------------------------------------------------- //

//--------------------------------------------------------------------------------------------------------
static const char * s_WeaponAliasInfo[] = 
{
	"none",		// WEAPON_NONE
	"mp5",		// WEAPON_MP5
	"shotgun",	// WEAPON_SHOTGUN
	"grenade",	// WEAPON_GRENADE
	NULL,		// WEAPON_NONE
};

//--------------------------------------------------------------------------------------------------------
//
// Given an alias, return the associated weapon ID
//
int AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i=0; s_WeaponAliasInfo[i] != NULL; ++i )
			if (!Q_stricmp( s_WeaponAliasInfo[i], alias ))
				return i;
	}

	return WEAPON_NONE;
}

//--------------------------------------------------------------------------------------------------------
//
// Given a weapon ID, return its alias
//
const char *WeaponIDToAlias( int id )
{
	if ( (id >= WEAPON_MAX) || (id < 0) )
		return NULL;

	return s_WeaponAliasInfo[id];
}

// ----------------------------------------------------------------------------- //
// CWeaponSDKBase tables.
// ----------------------------------------------------------------------------- //

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSDKBase, DT_WeaponSDKBase )

BEGIN_NETWORK_TABLE( CWeaponSDKBase, DT_WeaponSDKBase )
#ifdef CLIENT_DLL
  
#else
	// world weapon models have no animations
  	SendPropExclude( "DT_AnimTimeMustBeFirst", "m_flAnimTime" ),
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponSDKBase )
	DEFINE_PRED_FIELD( m_flTimeWeaponIdle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_sdk_base, CWeaponSDKBase );


#ifdef GAME_DLL

	BEGIN_DATADESC( CWeaponSDKBase )

		// New weapon Think and Touch Functions go here..

	END_DATADESC()

#endif

// ----------------------------------------------------------------------------- //
// CWeaponCSBase implementation. 
// ----------------------------------------------------------------------------- //
CWeaponSDKBase::CWeaponSDKBase()
{
	SetPredictionEligible( true );

	AddSolidFlags( FSOLID_TRIGGER ); // Nothing collides with these but it gets touches.
}

const CHL2MPSWeaponInfo &CWeaponSDKBase::GetSDKWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CHL2MPSWeaponInfo *pSDKInfo;

	#ifdef _DEBUG
		pSDKInfo = dynamic_cast< const CHL2MPSWeaponInfo* >( pWeaponInfo );
		Assert( pSDKInfo );
	#else
		pSDKInfo = static_cast< const CHL2MPSWeaponInfo* >( pWeaponInfo );
	#endif

	return *pSDKInfo;
}

bool CWeaponSDKBase::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );
	
	return 0;
}

CHL2MP_Player* CWeaponSDKBase::GetPlayerOwner() const
{
	return dynamic_cast< CHL2MP_Player* >( GetOwner() );
}

#ifdef GAME_DLL

void CWeaponSDKBase::SendReloadEvents()
{
	CHL2MP_Player *pPlayer = dynamic_cast< CHL2MP_Player* >( GetOwner() );
	if ( !pPlayer )
		return;

	// Send a message to any clients that have this entity to play the reload.
	CPASFilter filter( pPlayer->GetAbsOrigin() );
	filter.RemoveRecipient( pPlayer );

	UserMessageBegin( filter, "ReloadEffect" );
	WRITE_SHORT( pPlayer->entindex() );
	MessageEnd();

	// Make the player play his reload animation.
	SendWeaponAnim(ACT_VM_RELOAD);
}

#endif
