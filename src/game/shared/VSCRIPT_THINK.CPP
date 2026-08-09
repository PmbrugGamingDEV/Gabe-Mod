#include "cbase.h"
#include "vscript_shared.h"
#include "vscript_think.h"

extern IScriptVM* g_pScriptVM;

static bool g_bVScriptThinkEnabled = true;

void GabeVScriptEnableThink()
{
	g_bVScriptThinkEnabled = true;
}

void GabeVScriptDisableThink()
{
	g_bVScriptThinkEnabled = false;
}

bool GabeVScriptIsThinkEnabled()
{
	return g_bVScriptThinkEnabled;
}

void GabeVScriptThink()
{
	if (!g_bVScriptThinkEnabled)
		return;

	if (!g_pScriptVM)
		return;

	HSCRIPT hThink =
		g_pScriptVM->LookupFunction("Think");

	if (!hThink)
		return;

	g_pScriptVM->Call(
		hThink,
		NULL,
		true
	);

	g_pScriptVM->ReleaseFunction(hThink);
}

void RegisterGabeVScriptThink()
{
	if (!g_pScriptVM)
		return;

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeVScriptEnableThink,
		"VScript_EnableThink",
		"Enables the global VScript Think callback."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeVScriptDisableThink,
		"VScript_DisableThink",
		"Disables the global VScript Think callback."
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		GabeVScriptIsThinkEnabled,
		"VScript_IsThinkEnabled",
		"Returns whether VScript Think is enabled."
	);
}