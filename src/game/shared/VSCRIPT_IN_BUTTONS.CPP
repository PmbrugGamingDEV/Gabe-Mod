//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "gabe.h"

#include "vscript_shared.h"
#include "vscript_in_buttons.h"

int GetButtons()
{
	CBasePlayer* pPlayer = GabeFuncs_t::GetPlayer();

	if (!pPlayer)
		return 0;

	return pPlayer->m_nButtons;
}

void Register_In_Buttons()
{
	if (!g_pScriptVM)
	{
		return;
	}

	DECLARE_TABLE(Input);

	VSCRIPT_CONST(IN_ATTACK);
	VSCRIPT_CONST(IN_JUMP);
	VSCRIPT_CONST(IN_DUCK);
	VSCRIPT_CONST(IN_FORWARD);
	VSCRIPT_CONST(IN_BACK);
	VSCRIPT_CONST(IN_USE);
	VSCRIPT_CONST(IN_CANCEL);
	VSCRIPT_CONST(IN_LEFT);
	VSCRIPT_CONST(IN_RIGHT);
	VSCRIPT_CONST(IN_MOVELEFT);
	VSCRIPT_CONST(IN_MOVERIGHT);
	VSCRIPT_CONST(IN_ATTACK2);
	VSCRIPT_CONST(IN_RUN);
	VSCRIPT_CONST(IN_RELOAD);
	VSCRIPT_CONST(IN_ALT1);
	VSCRIPT_CONST(IN_ALT2);
	VSCRIPT_CONST(IN_SCORE);
	VSCRIPT_CONST(IN_SPEED);
	VSCRIPT_CONST(IN_WALK);
	VSCRIPT_CONST(IN_ZOOM);
	VSCRIPT_CONST(IN_WEAPON1);
	VSCRIPT_CONST(IN_WEAPON2);
	VSCRIPT_CONST(IN_BULLRUSH);
	VSCRIPT_CONST(IN_GRENADE1);
	VSCRIPT_CONST(IN_GRENADE2);
	VSCRIPT_CONST(IN_ATTACK3);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GetButtons,
		"GetButtons",
		"Done to connect keyboard to VScript. use it on a local named buttons."
	);

	TABLE_FUNC_NAMED(
		Input,
		GetButtons,
		GetButtons
	);
}