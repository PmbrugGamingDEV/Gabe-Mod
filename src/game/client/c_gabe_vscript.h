#pragma once

#include "interface.h"
#include "vscript/ivscript.h"

extern IScriptVM* g_pScriptVM;

void VScriptClientSetFactory(CreateInterfaceFn factory);

bool VScriptClientInit();
void VScriptClientTerm();
bool VScriptClientRunScript(const char* pszScriptName);