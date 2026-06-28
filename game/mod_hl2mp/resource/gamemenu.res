"GameMenu"
{
	"1"
	{
		"label" "#GameUI_GameMenu_ResumeGame"
		"command" "ResumeGame"
		"OnlyInGame" "1"
	}
	"2"
	{
		"label" "#GameUI_GameMenu_Disconnect"
		"command" "Disconnect"
		"OnlyInGame" "1"
	}
	"3"
	{
		"label" "#GameUI_GameMenu_PlayerList"
		"command" "OpenPlayerListDialog"
		"OnlyInGame" "1"
	}
	"4"
	{
		"label" ""
		"command" ""
		"OnlyInGame" "1"
	}
	"5"
	{
		"label" "#GameUI_GameMenu_FindServers"
		"command" "OpenServerBrowser"
	}
	"6"
	{
		"label" "#GameUI_GameMenu_CreateServer"
		"command" "engine gabe_newgame"
	}
	"6"
	{
		"label" "LOAD GAME"
		"command" "OpenLoadGameDialog"
	}
	"7"
	{
		"label" "SAVE GAME"
		"command" "OpenSaveGameDialog"
		"OnlyInGame" "1"
	}
	"7"
	{
		"label"	"FRIENDS"
		"command"	"engine gabe_friends"
		"ingameorder"	"7"
	}
	"8"
	{
		"label"	"TOGGLE CREDITS"
		"command"	"engine gabe_thanks"
		"ingameorder"	"8"
	}
	"9"
	{
		"label"	"TUTORIALS"
		"command"	"engine gabe_tutorial"
		"ingameorder"	"9"
	}
	"10"
	{
		"label" "#GameUI_GameMenu_ActivateVR"
		"command" "engine vr_activate"
		"InGameOrder" "40"
		"OnlyWhenVREnabled" "1"
		"OnlyWhenVRInactive" "1"
	}
	"11"
	{
		"label" "#GameUI_GameMenu_DeactivateVR"
		"command" "engine vr_deactivate"
		"InGameOrder" "40"
		"OnlyWhenVREnabled" "1"
		"OnlyWhenVRActive" "1"
	}
	"12"
	{
		"label" "#GameUI_GameMenu_Options"
		"command" "OpenOptionsDialog"
	}
	"13"
	{
		"label" "#GameUI_GameMenu_Quit"
		"command" "Quit"
	}
	"14"
	{
		"label" ""
		"command" ""
	}
	"15"
	{
		"label" "SCRIPT MENU"
		"command" "engine gabe_loadscripts"
	}
}

