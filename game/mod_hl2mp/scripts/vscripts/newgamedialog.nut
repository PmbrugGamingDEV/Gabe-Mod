// scripts/vscripts/newgamedialog.nut

::NG_Frame <- null
::NG_SelectedMap <- ""
::NG_MapCards <- []

::NG_Singleplayer <- null
::NG_MaxPlayers <- null
::NG_TimeLimit <- null
::NG_FragLimit <- null
::NG_Teamplay <- null
::NG_FriendlyFire <- null
::NG_Cheats <- null
::NG_Deathmatch <- null
::NG_HL1 <- null
::NG_Sandbox <- null
::NG_Hostname <- null
::NG_Password <- null
::NG_LAN <- null

function NG_Bool(v)
{
	return v ? "1" : "0"
}

function NG_Close()
{
	if (::NG_Frame != null)
	{
		::NG_Frame.Delete()
		::NG_Frame = null
	}

	::NG_MapCards.clear()
}

function NG_SafeMapName(map)
{
	if (map == null || map == "")
		return false

	if (map.find(" ") != null) return false
	if (map.find(";") != null) return false
	if (map.find("\"") != null) return false
	if (map.find("\\") != null) return false
	if (map.find("/") != null) return false

	return true
}

function NG_SplitLines(str)
{
	local out = []
	local start = 0

	for (local i = 0; i < str.len(); i++)
	{
		if (str.slice(i, i + 1) == "\n")
		{
			local line = str.slice(start, i)
			if (line != "")
				out.append(line)

			start = i + 1
		}
	}

	local last = str.slice(start)
	if (last != "")
		out.append(last)

	return out
}

function NG_SelectMap(map)
{
	::NG_SelectedMap = map
	printl("Selected map: " + map)
}

function NG_UpdateSingleplayer()
{
	if (::NG_Singleplayer == null || ::NG_MaxPlayers == null)
		return

	if (::NG_Singleplayer.IsChecked())
	{
		::NG_MaxPlayers.SetText("1")
		::NG_MaxPlayers.SetEnabled(false)
	}
	else
	{
		::NG_MaxPlayers.SetEnabled(true)

		if (::NG_MaxPlayers.GetText() == "1")
			::NG_MaxPlayers.SetText("8")
	}
}

function NG_Start()
{
	if (!NG_SafeMapName(::NG_SelectedMap))
	{
		MessageBox("New Game", "Select a valid map first.")
		return
	}

	local maxplayers = ::NG_MaxPlayers.GetText()
	if (::NG_Singleplayer.IsChecked())
		maxplayers = "1"

	if (maxplayers == "")
		maxplayers = "1"

	local timelimit = ::NG_TimeLimit.GetText()
	if (timelimit == "")
		timelimit = "0"

	local fraglimit = ::NG_FragLimit.GetText()
	if (fraglimit == "")
		fraglimit = "0"

	local hostname = ::NG_Hostname.GetText()
	local password = ::NG_Password.GetText()

	ClientCmdUnrestricted("hostname \"" + hostname + "\"")
	ClientCmdUnrestricted("sv_password \"" + password + "\"")
	ClientCmdUnrestricted("sv_lan " + NG_Bool(::NG_LAN.IsChecked()))

	ClientCmdUnrestricted("sv_allowupload 1")
	ClientCmdUnrestricted("sv_allowdownload 1")

	ClientCmdUnrestricted("maxplayers " + maxplayers)
	ClientCmdUnrestricted("mp_timelimit " + timelimit)
	ClientCmdUnrestricted("mp_fraglimit " + fraglimit)

	ClientCmdUnrestricted("mp_teamplay " + NG_Bool(::NG_Teamplay.IsChecked()))
	ClientCmdUnrestricted("mp_friendlyfire " + NG_Bool(::NG_FriendlyFire.IsChecked()))
	ClientCmdUnrestricted("sv_cheats " + NG_Bool(::NG_Cheats.IsChecked()))

	ClientCmdUnrestricted("gabeplus_deathmatch " + NG_Bool(::NG_Deathmatch.IsChecked()))
	ClientCmdUnrestricted("gabeplus_hl1 " + NG_Bool(::NG_HL1.IsChecked()))
	ClientCmdUnrestricted("gabeplus_sandbox " + NG_Bool(::NG_Sandbox.IsChecked()))

	ClientCmdUnrestricted("disconnect")
	ClientCmdUnrestricted("map " + ::NG_SelectedMap)

	NG_Close()
}

function NG_CreateMapCard(parent, mapname, index)
{
	local cardW = 155
	local cardH = 125
	local gapX = 18
	local gapY = 10
	local startX = 15
	local startY = 55

	local col = index % 3
	local row = index / 3

	local x = startX + col * (cardW + gapX)
	local y = startY + row * (cardH + gapY)

	local card = parent.CreatePanel(x, y, cardW, cardH)
	card.SetBgColor(90, 98, 110, 255)

	local imgPath = "vgui/maps/" + mapname
	card.CreateImagePanel(imgPath, 5, 5, cardW - 10, 88)

	local name = card.CreateLabel(mapname, 7, 94, cardW - 14, 16)
	name.SetFgColor(255, 255, 255, 255)

	local author = card.CreateLabel("Local Map", 7, 110, cardW - 14, 14)
	author.SetFgColor(170, 175, 185, 255)

	local btn = card.CreateButton("", 0, 0, cardW, cardH)
	btn.SetAlpha(0)
	btn.SetCommand("NG_SelectMap(\"" + mapname + "\")")

	::NG_MapCards.append(card)
}

function ShowNewGameDialog()
{
	NG_Close()

	::NG_SelectedMap = ""

	::NG_Frame = VGUI_CreateFrame("Start Single-Player Game", 160, 40, 760, 660)
	::NG_Frame.SetSizeable(false)

	::NG_Frame.CreateLabel("Maps", 15, 30, 200, 20)

	local maps = NG_SplitLines(GetMapListString())

	if (maps.len() <= 0)
	{
		::NG_Frame.CreateLabel("No maps found in maps/*.bsp", 20, 70, 300, 24)
	}
	else
	{
		for (local i = 0; i < maps.len(); i++)
		{
			if (i >= 12)
				break

			NG_CreateMapCard(::NG_Frame, maps[i], i)

			if (::NG_SelectedMap == "")
				::NG_SelectedMap = maps[i]
		}
	}

	local sx = 545
	local y = 55

	::NG_Frame.CreateLabel("Settings", sx, y, 170, 22)
	y += 28

	::NG_Singleplayer = ::NG_Frame.CreateCheckButton("Singleplayer Game", sx, y, 180, 22)
	::NG_Singleplayer.SetChecked(true)
	::NG_Singleplayer.SetCommand("NG_UpdateSingleplayer()")
	y += 30

	::NG_Frame.CreateLabel("Max Players:", sx, y, 100, 22)
	::NG_MaxPlayers = ::NG_Frame.CreateTextEntry("1", sx + 105, y, 60, 22)
	y += 30

	::NG_Frame.CreateLabel("Time Limit:", sx, y, 100, 22)
	::NG_TimeLimit = ::NG_Frame.CreateTextEntry("0", sx + 105, y, 60, 22)
	y += 30

	::NG_Frame.CreateLabel("Frag Limit:", sx, y, 100, 22)
	::NG_FragLimit = ::NG_Frame.CreateTextEntry("0", sx + 105, y, 60, 22)
	y += 35

	::NG_Teamplay = ::NG_Frame.CreateCheckButton("Team Based", sx, y, 180, 22)
	::NG_Teamplay.SetChecked(false)
	y += 26

	::NG_FriendlyFire = ::NG_Frame.CreateCheckButton("Friendly Fire", sx, y, 180, 22)
	::NG_FriendlyFire.SetChecked(true)
	y += 26

	::NG_Cheats = ::NG_Frame.CreateCheckButton("Allow Cheats", sx, y, 180, 22)
	::NG_Cheats.SetChecked(true)
	y += 26

	::NG_Deathmatch = ::NG_Frame.CreateCheckButton("Deathmatch Mode", sx, y, 180, 22)
	::NG_Deathmatch.SetChecked(false)
	y += 26

	::NG_HL1 = ::NG_Frame.CreateCheckButton("HL1 Weapons", sx, y, 180, 22)
	::NG_HL1.SetChecked(false)
	y += 26

	::NG_Sandbox = ::NG_Frame.CreateCheckButton("Sandbox Mode", sx, y, 180, 22)
	::NG_Sandbox.SetChecked(true)
	y += 40

	::NG_Frame.CreateLabel("Server Info", sx, y, 170, 22)
	y += 28

	::NG_Frame.CreateLabel("Name:", sx, y, 80, 22)
	::NG_Hostname = ::NG_Frame.CreateTextEntry("GabeMod+ Server", sx + 65, y, 150, 22)
	y += 30

	::NG_Frame.CreateLabel("Password:", sx, y, 80, 22)
	::NG_Password = ::NG_Frame.CreateTextEntry("", sx + 65, y, 150, 22)
	y += 30

	::NG_LAN = ::NG_Frame.CreateCheckButton("LAN Only", sx, y, 180, 22)
	::NG_LAN.SetChecked(true)

	local start = ::NG_Frame.CreateButton("Create New Game", 510, 610, 140, 28)
	start.SetCommand("NG_Start()")

	local cancel = ::NG_Frame.CreateButton("Cancel", 660, 610, 75, 28)
	cancel.SetCommand("NG_Close()")

	NG_UpdateSingleplayer()
}

ShowNewGameDialog()