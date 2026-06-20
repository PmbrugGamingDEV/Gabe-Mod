#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "hud_killfeed.h"

using namespace vgui;

DECLARE_HUDELEMENT(CHudKillFeed);
DECLARE_HUD_MESSAGE(CHudKillFeed, KillFeed);
DECLARE_HUD_MESSAGE(CHudKillFeed, DamageFeed);

CHudKillFeed::CHudKillFeed(const char* pElementName)
	: CHudElement(pElementName), Panel(NULL, "HudKillFeed")
{
	SetParent(g_pClientMode->GetViewport());
	SetPaintBackgroundEnabled(false);
	SetBounds(0, 0, ScreenWidth(), ScreenHeight());
}

void CHudKillFeed::Init()
{
	HOOK_HUD_MESSAGE(CHudKillFeed, KillFeed);
	HOOK_HUD_MESSAGE(CHudKillFeed, DamageFeed);
	m_Lines.Purge();
}

void CHudKillFeed::Reset()
{
	m_Lines.Purge();
}

void CHudKillFeed::MsgFunc_KillFeed(bf_read& msg)
{
	char attacker[64], victim[64];
	msg.ReadString(attacker, sizeof(attacker));
	msg.ReadString(victim, sizeof(victim));

	char buffer[128];

	// combine everything
	Q_snprintf(
		buffer,
		sizeof(buffer),
		"%s %s %s",
		attacker,
		"killed",
		victim
	);

	KillFeedLine line;
	g_pVGuiLocalize->ConvertANSIToUnicode(buffer, line.text, sizeof(line.text));
	line.timeAdded = gpGlobals->curtime;
	line.isDamage = false;

	m_Lines.AddToHead(line);

	// keep only last 5 entries
	if (m_Lines.Count() > 5)
		m_Lines.Remove(m_Lines.Count() - 1);
}

void CHudKillFeed::MsgFunc_DamageFeed(bf_read& msg)
{
	char attacker[64], victim[64], dmgType[32];
	int dmg = 0;

	msg.ReadString(attacker, sizeof(attacker));
	msg.ReadString(victim, sizeof(victim));
	dmg = (int)msg.ReadShort();
	msg.ReadString(dmgType, sizeof(dmgType));

	char buffer[160];
	Q_snprintf(buffer, sizeof(buffer), "%s gave +%d %s to %s", attacker, dmg, dmgType, victim);

	KillFeedLine line;
	g_pVGuiLocalize->ConvertANSIToUnicode(buffer, line.text, sizeof(line.text));
	line.timeAdded = gpGlobals->curtime;
	line.isDamage = true;

	m_Lines.AddToHead(line);
	if (m_Lines.Count() > 5)
		m_Lines.Remove(m_Lines.Count() - 1);
}

void CHudKillFeed::Paint()
{
	int screenW, screenH;
	surface()->GetScreenSize(screenW, screenH);

	HFont font = scheme()->GetIScheme(GetScheme())->GetFont("DefaultVerySmall", true);

	int baseY = screenH * 0.18f;
	int spacing = 30;

	for (int i = m_Lines.Count() - 1; i >= 0; i--)
	{
		float age = gpGlobals->curtime - m_Lines[i].timeAdded;

		// remove expired safely
		if (age > 5.0f)
		{
			m_Lines.Remove(i);
			continue;
		}

		int drawIndex = (m_Lines.Count() - 1) - i;

		// =========================
		// ANIMATION
		// =========================

		int alpha = RemapValClamped(age, 0.0f, 5.0f, 255.0f, 0.0f);
		float slide = RemapValClamped(age, 0.0f, 0.25f, 120.0f, 0.0f);
		float yOffset = RemapValClamped(age, 0.0f, 0.2f, -10.0f, 0.0f);

		// =========================
		// SYMBOL (SAFE ASCII)
		// =========================

		const wchar_t* symbol = m_Lines[i].isDamage ? L"+" : L"x";

		int symW, symH;
		surface()->GetTextSize(font, symbol, symW, symH);

		// =========================
		// TEXT SIZE
		// =========================

		int textW, textH;
		surface()->GetTextSize(font, m_Lines[i].text, textW, textH);

		int totalW = textW + symW + 15;

		int x = screenW - totalW - 60 + slide;
		int y = baseY + (drawIndex * spacing) + yOffset;

		// =========================
		// BACKGROUND
		// =========================

		int pad = 8;

		surface()->DrawSetColor(0, 0, 0, alpha / 2);
		surface()->DrawFilledRect(
			x - pad,
			y - 3,
			x + totalW + pad,
			y + textH + 3
		);

		// highlight strip
		if (m_Lines[i].isDamage)
			surface()->DrawSetColor(255, 140, 0, alpha);
		else
			surface()->DrawSetColor(255, 60, 60, alpha);

		surface()->DrawFilledRect(
			x - pad,
			y - 3,
			x - pad + 3,
			y + textH + 3
		);

		// =========================
		// SYMBOL
		// =========================

		if (m_Lines[i].isDamage)
			surface()->DrawSetTextColor(255, 180, 0, alpha);
		else
			surface()->DrawSetTextColor(255, 255, 255, alpha);

		surface()->DrawSetTextFont(font);
		surface()->DrawSetTextPos(x, y);
		surface()->DrawPrintText(symbol, wcslen(symbol));

		x += symW + 5;

		// =========================
		// TEXT
		// =========================

		surface()->DrawSetTextColor(255, 255, 255, alpha);

		surface()->DrawSetTextPos(x, y);
		surface()->DrawPrintText(m_Lines[i].text, wcslen(m_Lines[i].text));
	}
}