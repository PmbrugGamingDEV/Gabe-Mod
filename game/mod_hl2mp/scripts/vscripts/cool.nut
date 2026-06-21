// Cool animated HUD test
// local Author = "Phil"
// local Description = "Animated neon VScript HUD"
// local Version = "1.0"
// local DateMade = "2026"

::CoolHud <- null
::CoolHudFont <- 0
::CoolHudSmallFont <- 0
::CoolHudStart <- Time()

function CoolHud_Init()
{
	if (::CoolHud != null)
	{
		::CoolHud.HUD_Remove()
		::CoolHud = null
	}

	::CoolHudFont = Surface_CreateFont()
	Surface_SetFontGlyphSet(::CoolHudFont, "Tahoma", 22, 800, 0, 0, 0)

	::CoolHudSmallFont = Surface_CreateFont()
	Surface_SetFontGlyphSet(::CoolHudSmallFont, "Tahoma", 14, 600, 0, 0, 0)

	::CoolHud = HUD_CreateElement("CoolVScriptHUD")
	::CoolHud.SetPaintFunction("CoolHud_Paint")
	::CoolHud.HUD_SetBounds(0, 0, Surface_GetScreenWide(), Surface_GetScreenTall())
	::CoolHud.HUD_ForceShow()

	printl("[CoolHud] loaded")
}

function DrawText(font, x, y, r, g, b, a, text)
{
	Surface_SetTextFont(font)
	Surface_SetTextColor(r, g, b, a)
	Surface_SetTextPos(x, y)
	Surface_PrintText(text)
}

function CoolHud_Paint(hud)
{
	local sw = Surface_GetScreenWide()
	local sh = Surface_GetScreenTall()

	local t = Time() - ::CoolHudStart
	local pulse = 120 + sin(t * 4.0) * 80
	local x = 36
	local y = sh - 150
	local w = 330
	local h = 92

	// shadow
	Surface_SetDrawColor(0, 0, 0, 180)
	Surface_DrawFilledRect(x + 5, y + 5, x + w + 5, y + h + 5)

	// main dark glass panel
	Surface_SetDrawColor(8, 14, 22, 210)
	Surface_DrawFilledRect(x, y, x + w, y + h)

	// neon outline
	Surface_SetDrawColor(0, 180, 255, pulse.tointeger())
	Surface_DrawOutlinedRect(x, y, x + w, y + h)

	// top glow strip
	Surface_SetDrawColor(0, 220, 255, 180)
	Surface_DrawFilledRect(x, y, x + w, y + 3)

	// animated scanner line
	local scanX = x + 10 + ((t * 90).tointeger() % (w - 20))
	Surface_SetDrawColor(0, 255, 255, 160)
	Surface_DrawLine(scanX, y + 8, scanX + 35, y + h - 8)

	// fake power bar
	local barW = ((sin(t * 2.5) + 1.0) * 0.5 * 210).tointeger()
	Surface_SetDrawColor(20, 40, 55, 230)
	Surface_DrawFilledRect(x + 92, y + 56, x + 305, y + 70)

	Surface_SetDrawColor(0, 210, 120, 230)
	Surface_DrawFilledRect(x + 94, y + 58, x + 94 + barW, y + 68)

	Surface_SetDrawColor(255, 255, 255, 40)
	Surface_DrawFilledRectFade(x, y, x + w, y + h, 70, 0, false)

	DrawText(::CoolHudFont, x + 18, y + 15, 0, 230, 255, 255, "VScript HUD ONLINE")
	DrawText(::CoolHudSmallFont, x + 20, y + 50, 220, 240, 255, 230, "client paint callback active")
	DrawText(::CoolHudSmallFont, x + 20, y + 68, 120, 255, 190, 230, "runtime: " + t.tointeger() + " sec")
}

CoolHud_Init()