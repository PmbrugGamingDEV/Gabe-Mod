#pragma once
#include "cbase.h"

ConVar gabe_forceholiday(
	"gabe_forceholiday",
	"0",
	FCVAR_CLIENTDLL,
	"Force a specific holiday mode (0 for none, 1-11 for specific holidays):\n 1 - New Year\n 2 - Valentine's Day\n 3 - St. Patrick's Day\n 4 - Easter Season\n 5 - Memorial Season\n 6 - Independence Day\n 7 - Labor Season\n 8 - Halloween\n 9 - Thanksgiving Season\n 10 - Christmas\n 11 - No Holiday"
);
