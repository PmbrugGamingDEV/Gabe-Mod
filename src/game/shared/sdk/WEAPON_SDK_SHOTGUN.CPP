//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"


#if defined( CLIENT_DLL )

	#define CWeaponSDKShotgun C_WeaponSDKShotgun
	#include "c_hl2mp_player.h"

#else

	#include "hl2mp_player.h"
	#include "sdk/te_firebullets.h"

#endif


class CWeaponSDKShotgun : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CWeaponSDKShotgun, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CWeaponSDKShotgun();

	virtual void PrimaryAttack();
	virtual bool Reload();
	virtual void WeaponIdle();

	virtual SDKWeaponID GetWeaponID( void ) const		{ return WEAPON_SHOTGUN; }


private:

	CWeaponSDKShotgun( const CWeaponSDKShotgun & );

	float m_flPumpTime;
	CNetworkVar( int, m_fInSpecialReload );

};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSDKShotgun, DT_WeaponSDKShotgun )

BEGIN_NETWORK_TABLE( CWeaponSDKShotgun, DT_WeaponSDKShotgun )

	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( m_fInSpecialReload ) )
	#else
		SendPropInt( SENDINFO( m_fInSpecialReload ), 2, SPROP_UNSIGNED )
	#endif

END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSDKShotgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_sdk_shotgun, CWeaponSDKShotgun );
PRECACHE_WEAPON_REGISTER( weapon_sdk_shotgun );



CWeaponSDKShotgun::CWeaponSDKShotgun()
{
	m_flPumpTime = 0;
}

void CWeaponSDKShotgun::PrimaryAttack()
{
	CHL2MP_Player *pPlayer = GetPlayerOwner();
	if ( !pPlayer )
		return;

	// don't fire underwater
	if (pPlayer->GetWaterLevel() == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.15;
		return;
	}

	// Out of ammo?
	if ( m_iClip1 <= 0 )
	{
		Reload();
		if ( m_iClip1 == 0 )
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
		}

		return;
	}

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	m_iClip1--;
	pPlayer->DoMuzzleFlash();

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

#ifndef CLIENT_DLL

	FireBulletsInfo_t info;
	info.m_iShots = 6;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	info.m_pAttacker = pPlayer;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_flDamage = GetSDKWpnData().m_iPlayerDamage;
	info.m_nFlags = FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS;
	info.m_iTracerFreq = 1;

	FireBullets(info);
#endif

	if (!m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);
	}

	if (m_iClip1 != 0)
		m_flPumpTime = gpGlobals->curtime + 0.5;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.875;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.875;
	if (m_iClip1 != 0)
		SetWeaponIdleTime( gpGlobals->curtime + 2.5 );
	else
		SetWeaponIdleTime( gpGlobals->curtime + 0.875 );
	m_fInSpecialReload = 0;

	// Update punch angles.
	QAngle angle = pPlayer->GetPunchAngle();

	if ( pPlayer->GetFlags() & FL_ONGROUND )
	{
		angle.x -= SharedRandomInt( "ShotgunPunchAngleGround", 4, 6 );
	}
	else
	{
		angle.x -= SharedRandomInt( "ShotgunPunchAngleAir", 8, 11 );
	}

	pPlayer->SetPunchAngle( angle );
}


bool CWeaponSDKShotgun::Reload()
{
	CHL2MP_Player *pPlayer = GetPlayerOwner();

	if (pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 || m_iClip1 == GetMaxClip1())
		return true;

	// don't reload until recoil is done
	if (m_flNextPrimaryAttack > gpGlobals->curtime)
		return true;
		
	// check to see if we're ready to reload
	if (m_fInSpecialReload == 0)
	{
		pPlayer->SetAnimation( PLAYER_RELOAD );

		SendWeaponAnim( ACT_SHOTGUN_RELOAD_START );
		m_fInSpecialReload = 1;
		pPlayer->m_flNextAttack = gpGlobals->curtime + 0.5;
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
		SetWeaponIdleTime( gpGlobals->curtime + 0.5 );
		return true;
	}
	else if (m_fInSpecialReload == 1)
	{
		if (m_flTimeWeaponIdle > gpGlobals->curtime)
			return true;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		SendWeaponAnim( ACT_VM_RELOAD );
		SetWeaponIdleTime( gpGlobals->curtime + 0.45 );
	}
	else
	{
		// Add them to the clip
		m_iClip1 += 1;
		
#ifdef GAME_DLL
		SendReloadEvents();
#endif
		
		CHL2MP_Player *pPlayer = GetPlayerOwner();

		if ( pPlayer )
			 pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );

		m_fInSpecialReload = 1;
	}

	return true;
}


void CWeaponSDKShotgun::WeaponIdle()
{
	CHL2MP_Player *pPlayer = GetPlayerOwner();

	if (m_flPumpTime && m_flPumpTime < gpGlobals->curtime)
	{
		// play pumping sound
		m_flPumpTime = 0;
	}

	if (m_flTimeWeaponIdle < gpGlobals->curtime)
	{
		if (m_iClip1 == 0 && m_fInSpecialReload == 0 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ))
		{
			Reload( );
		}
		else if (m_fInSpecialReload != 0)
		{
			if (m_iClip1 != 8 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ))
			{
				Reload( );
			}
			else
			{
				// reload debounce has timed out
				//MIKETODO: shotgun anims
				SendWeaponAnim( ACT_SHOTGUN_RELOAD_FINISH );
				
				// play cocking sound
				m_fInSpecialReload = 0;
				SetWeaponIdleTime( gpGlobals->curtime + 1.5 );
			}
		}
		else
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
	}
}
