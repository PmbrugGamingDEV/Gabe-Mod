#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

void RegisterUserMessagesHL1(void)
{
	usermessages->Register("SPHapWeapEvent", 4);
	usermessages->Register("HapDmg", -1);
	usermessages->Register("HapPunch", -1);
	usermessages->Register("HapSetDrag", -1);
	usermessages->Register("HapSetConst", -1);
	usermessages->Register("HapMeleeContact", 0);
}