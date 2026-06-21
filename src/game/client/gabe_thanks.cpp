//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Gabe Mod Plus - Special Thanks Panel
//
//=============================================================================

#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IScheme.h>

#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>

#include "gabe_events.h"
#include "tier1/fmtstr.h"
#include "ienginevgui.h"

#include <math.h>

using namespace vgui;

ConVar gabeplus_thanks_rbow(
	"gabe_thanks_rbow",
	"0",
	FCVAR_CLIENTDLL,
	"Whether the thanks panel should cycle through rainbow colors"
);

ConVar gabeplus_thanks_glow(
	"gabe_thanks_glow",
	"1",
	FCVAR_CLIENTDLL,
	"Whether the thanks panel should use animated glow"
);

extern ConVar gabe_forceholiday;

static Color GabePlus_RainbowColor(float t, int alpha = 255)
{
	int r = (int)(sinf(t + 0.000f) * 127.0f + 128.0f);
	int g = (int)(sinf(t + 2.094f) * 127.0f + 128.0f);
	int b = (int)(sinf(t + 4.188f) * 127.0f + 128.0f);

	return Color(r, g, b, alpha);
}

static Color GabePlus_PulseColor(Color base, float t, int minA, int maxA)
{
	float p = sinf(t) * 0.5f + 0.5f;
	int a = minA + (int)((float)(maxA - minA) * p);

	return Color(base.r(), base.g(), base.b(), a);
}

// ------------------------------------------------------------
// Shadowed Label Pair
// ------------------------------------------------------------
struct ThanksLabel_t
{
	Label* shadow;
	Label* text;
};

// ------------------------------------------------------------
// Thanks Panel
// ------------------------------------------------------------
class CGabePlusThanksPanel : public Panel
{
	DECLARE_CLASS_SIMPLE(CGabePlusThanksPanel, Panel);

public:
	CGabePlusThanksPanel(VPANEL parent)
		: BaseClass(NULL, "GabePlusThanksPanel")
	{
		SetParent(parent);

		SetPaintBackgroundEnabled(true);
		SetPaintBorderEnabled(false);

		SetMouseInputEnabled(false);
		SetKeyBoardInputEnabled(false);

		SetVisible(true);

		m_flCreateTime = gpGlobals ? gpGlobals->curtime : 0.0f;

		m_iLineCount = 0;

		AddLine("SPECIAL THANKS TO:", true);
		AddLine("");
		AddLine("Garry Newman  - Block spawner in gabe_pond");
		AddLine("PmbrugGaming  - Development, AI prompter");
		AddLine("DasBoSchitt   - HAX");
		AddLine("Adnan Zafar   - Rotation for Physics Gun");
		AddLine("HL2 Sandbox Developers - Updated Physics Gun");

		m_pVersionShadow = new Label(this, "VersionShadow", "Gabe Mod Development Test");
		m_pVersion = new Label(this, "VersionText", "Gabe Mod Development Test");

		m_pHolidayShadow = new Label(this, "HolidayShadow", "");
		m_pHoliday = new Label(this, "HolidayText", "");

		SetupLabel(m_pVersionShadow, Label::a_west);
		SetupLabel(m_pVersion, Label::a_west);
		SetupLabel(m_pHolidayShadow, Label::a_west);
		SetupLabel(m_pHoliday, Label::a_west);

		SetScheme(scheme()->GetScheme("ClientScheme"));

		InvalidateLayout();
	}

	virtual void ApplySchemeSettings(IScheme* pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);

		m_hSmallFont = pScheme->GetFont("DebugFixedSmall", false);
		m_hBigFont = pScheme->GetFont("Default", false);

		for (int i = 0; i < m_iLineCount; i++)
		{
			m_Lines[i].shadow->SetFont(m_hSmallFont);
			m_Lines[i].text->SetFont(m_hSmallFont);
		}

		m_pVersionShadow->SetFont(m_hSmallFont);
		m_pVersion->SetFont(m_hSmallFont);

		m_pHolidayShadow->SetFont(m_hBigFont);
		m_pHoliday->SetFont(m_hBigFont);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int sw, sh;
		surface()->GetScreenSize(sw, sh);

		const int width = 560;
		const int height = 220;
		const int pad = 18;

		SetSize(width, height);
		SetPos(sw - width - pad, sh - height - pad);

		int x = 18;
		int y = 16;

		for (int i = 0; i < m_iLineCount; i++)
		{
			SetShadowedLabelBounds(m_Lines[i], x, y, width - 36, 16);
			y += 18;
		}

		y += 8;

		SetShadowedLabelBounds(m_VersionPair(), x, y, width - 36, 16);
		y += 26;

		SetShadowedLabelBounds(m_HolidayPair(), x, y, width - 36, 28);
	}

	virtual void PaintBackground()
	{
		BaseClass::PaintBackground();

		int wide, tall;
		GetSize(wide, tall);

		float t = gpGlobals ? gpGlobals->curtime : 0.0f;

		Color accent = gabeplus_thanks_rbow.GetBool()
			? GabePlus_RainbowColor(t * 2.0f)
			: Color(80, 220, 255, 255);

		int glowAlpha = gabeplus_thanks_glow.GetBool()
			? 60 + (int)((sinf(t * 4.0f) * 0.5f + 0.5f) * 80.0f)
			: 70;

		// Dark glass background
		surface()->DrawSetColor(0, 0, 0, 165);
		surface()->DrawFilledRect(0, 0, wide, tall);

		// Inner panel tint
		surface()->DrawSetColor(20, 20, 20, 110);
		surface()->DrawFilledRect(4, 4, wide - 4, tall - 4);

		// Outer glow / border
		surface()->DrawSetColor(accent.r(), accent.g(), accent.b(), glowAlpha);
		surface()->DrawOutlinedRect(0, 0, wide, tall);

		surface()->DrawSetColor(accent.r(), accent.g(), accent.b(), glowAlpha / 2);
		surface()->DrawOutlinedRect(1, 1, wide - 1, tall - 1);

		// Animated top scan line
		int scanX = (int)(fabsf(sinf(t * 1.75f)) * (float)(wide - 60));

		surface()->DrawSetColor(accent.r(), accent.g(), accent.b(), 180);
		surface()->DrawFilledRect(scanX, 2, scanX + 60, 4);

		// Divider
		int divY = 145;

		surface()->DrawSetColor(accent.r(), accent.g(), accent.b(), 110);
		surface()->DrawFilledRect(16, divY, wide - 16, divY + 1);
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();

		float t = gpGlobals ? gpGlobals->curtime : 0.0f;

		Color mainColor = gabeplus_thanks_rbow.GetBool()
			? GabePlus_RainbowColor(t * 2.0f)
			: Color(235, 245, 255, 255);

		Color titleColor = gabeplus_thanks_rbow.GetBool()
			? GabePlus_RainbowColor(t * 2.0f + 1.0f)
			: Color(120, 230, 255, 255);

		Color shadowColor(0, 0, 0, 220);

		for (int i = 0; i < m_iLineCount; i++)
		{
			m_Lines[i].shadow->SetFgColor(shadowColor);

			if (i == 0)
				m_Lines[i].text->SetFgColor(titleColor);
			else
				m_Lines[i].text->SetFgColor(mainColor);
		}

		m_pVersionShadow->SetFgColor(shadowColor);
		m_pVersion->SetFgColor(Color(0, 255, 80, 255));

		const char* pszHoliday = GabeEvents_GetHolidayName();

		if (GabeEvents_GetHoliday() != HOLIDAY_NONE && pszHoliday && pszHoliday[0])
		{
			m_pHoliday->SetText(CFmtStr("Happy %s!", pszHoliday));
			m_pHolidayShadow->SetText(CFmtStr("Happy %s!", pszHoliday));
		}
		else
		{
			m_pHoliday->SetText("No holiday active.");
			m_pHolidayShadow->SetText("No holiday active.");
		}

		Color holidayColor = GabeEvents_GetHoliday() != HOLIDAY_NONE
			? GabePlus_PulseColor(mainColor, t * 5.0f, 180, 255)
			: Color(180, 180, 180, 255);

		m_pHolidayShadow->SetFgColor(shadowColor);
		m_pHoliday->SetFgColor(holidayColor);

		Repaint();
	}

private:
	enum
	{
		MAX_THANKS_LINES = 16
	};

	void SetupLabel(Label* pLabel, Label::Alignment align)
	{
		pLabel->SetContentAlignment(align);
		pLabel->SetPaintBackgroundEnabled(false);
		pLabel->SetPaintBorderEnabled(false);
		pLabel->SetMouseInputEnabled(false);
		pLabel->SetKeyBoardInputEnabled(false);
	}

	void AddLine(const char* pszText, bool bTitle = false)
	{
		if (m_iLineCount >= MAX_THANKS_LINES)
			return;

		char name[64];

		Q_snprintf(name, sizeof(name), "ThanksLineShadow%d", m_iLineCount);
		m_Lines[m_iLineCount].shadow = new Label(this, name, pszText);

		Q_snprintf(name, sizeof(name), "ThanksLineText%d", m_iLineCount);
		m_Lines[m_iLineCount].text = new Label(this, name, pszText);

		SetupLabel(m_Lines[m_iLineCount].shadow, bTitle ? Label::a_west : Label::a_northwest);
		SetupLabel(m_Lines[m_iLineCount].text, bTitle ? Label::a_west : Label::a_northwest);

		m_iLineCount++;
	}

	void SetShadowedLabelBounds(ThanksLabel_t pair, int x, int y, int w, int h)
	{
		pair.shadow->SetPos(x + 2, y + 2);
		pair.shadow->SetSize(w, h);

		pair.text->SetPos(x, y);
		pair.text->SetSize(w, h);
	}

	ThanksLabel_t m_VersionPair()
	{
		ThanksLabel_t pair;
		pair.shadow = m_pVersionShadow;
		pair.text = m_pVersion;
		return pair;
	}

	ThanksLabel_t m_HolidayPair()
	{
		ThanksLabel_t pair;
		pair.shadow = m_pHolidayShadow;
		pair.text = m_pHoliday;
		return pair;
	}

private:
	ThanksLabel_t m_Lines[MAX_THANKS_LINES];
	int m_iLineCount;

	Label* m_pVersionShadow;
	Label* m_pVersion;

	Label* m_pHolidayShadow;
	Label* m_pHoliday;

	HFont m_hSmallFont;
	HFont m_hBigFont;

	float m_flCreateTime;
};

// ------------------------------------------------------------
// Console command toggle
// ------------------------------------------------------------
static CGabePlusThanksPanel* g_pThanks = NULL;

CON_COMMAND_F(
	gabe_thanks,
	"Toggle special thanks panel",
	FCVAR_CLIENTDLL
)
{
	VPANEL root = enginevgui->GetPanel(PANEL_GAMEUIDLL);
	if (!root)
		return;

	if (!g_pThanks)
	{
		g_pThanks = new CGabePlusThanksPanel(root);
		g_pThanks->MakePopup(false);
	}
	else
	{
		g_pThanks->SetVisible(!g_pThanks->IsVisible());
	}
}