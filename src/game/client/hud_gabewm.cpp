
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/Panel.h"
#include "tier1/strtools.h"

// memdbgon.h must be the last include.
#include "tier0/memdbgon.h"

using namespace vgui;

extern IClientMode* g_pClientMode;

ConVar watermark_r("watermark_r", "150",
    FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark red.");

ConVar watermark_g("watermark_g", "205",
    FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark green.");

ConVar watermark_b("watermark_b", "255",
    FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark blue.");

ConVar watermark_a("watermark_a", "210",
    FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark opacity.");

ConVar watermark_enable("gabe_watermark_enable", "1",
    FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Show watermark.");

class CHudWatermark : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE(CHudWatermark, Panel);

    CHudWatermark(const char* pElementName);

    virtual void ApplySchemeSettings(IScheme* pScheme);
    virtual bool ShouldDraw(void);
    virtual void Paint(void);

private:
    HFont m_hFontTitle;
    HFont m_hFontWebsite;
};

DECLARE_HUDELEMENT(CHudWatermark);

CHudWatermark::CHudWatermark(const char* pElementName)
    : CHudElement(pElementName),
    Panel(NULL, "HudWatermark")
{
    SetParent(g_pClientMode->GetViewport());
    SetHiddenBits(0);

    m_hFontTitle = INVALID_FONT;
    m_hFontWebsite = INVALID_FONT;

    SetPaintBackgroundEnabled(false);
    SetPaintBorderEnabled(false);
    SetMouseInputEnabled(false);
    SetKeyBoardInputEnabled(false);

    int w, h;
    surface()->GetScreenSize(w, h);

    SetPos(0, 0);
    SetSize(w, h);
}

void CHudWatermark::ApplySchemeSettings(IScheme* pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    SetPaintBackgroundEnabled(false);
    SetPaintBorderEnabled(false);

    m_hFontTitle = surface()->CreateFont();
    surface()->SetFontGlyphSet(
        m_hFontTitle,
        "Tahoma",
        16,
        700,
        0,
        0,
        ISurface::EFontFlags::FONTFLAG_ANTIALIAS
    );

    m_hFontWebsite = surface()->CreateFont();
    surface()->SetFontGlyphSet(
        m_hFontWebsite,
        "Verdana",
        11,
        500,
        0,
        0,
        ISurface::EFontFlags::FONTFLAG_ANTIALIAS
    );

    int w, h;
    surface()->GetScreenSize(w, h);

    SetPos(0, 0);
    SetSize(w, h);
}

bool CHudWatermark::ShouldDraw(void)
{
    return watermark_enable.GetBool()
        && CHudElement::ShouldDraw();
}

void CHudWatermark::Paint(void)
{
    if (!watermark_enable.GetBool())
        return;

    int screenW, screenH;
    surface()->GetScreenSize(screenW, screenH);

    // ---------------------------------
    // Text
    // ---------------------------------

    const wchar_t* title = L"GABE MOD";
    const wchar_t* website =
        L"sites.google.com/view/pmbruggaming";

    int titleW, titleH;
    int websiteW, websiteH;

    surface()->GetTextSize(
        m_hFontTitle,
        title,
        titleW,
        titleH
    );

    surface()->GetTextSize(
        m_hFontWebsite,
        website,
        websiteW,
        websiteH
    );

    // ---------------------------------
    // Layout
    // ---------------------------------

    const int paddingX = 11;
    const int paddingY = 7;
    const int spacing = 2;
    const int margin = 10;

    const int contentW = max(titleW, websiteW);

    const int boxW = contentW + paddingX * 2;
    const int boxH =
        paddingY * 2 +
        titleH +
        spacing +
        websiteH +
        4;

    const int x = screenW - boxW - margin;
    const int y = margin;

    const int a = clamp(watermark_a.GetInt(), 0, 255);
    const int r = clamp(watermark_r.GetInt(), 0, 255);
    const int g = clamp(watermark_g.GetInt(), 0, 255);
    const int b = clamp(watermark_b.GetInt(), 0, 255);

    // ---------------------------------
    // Subtle shadow
    // ---------------------------------

    surface()->DrawSetColor(0, 0, 0, a / 3);
    surface()->DrawFilledRect(
        x + 2, y + 3,
        x + boxW + 2,
        y + boxH + 3
    );

    // ---------------------------------
    // Main dark background
    // ---------------------------------

    surface()->DrawSetColor(17, 21, 27, a);
    surface()->DrawFilledRect(
        x, y,
        x + boxW,
        y + boxH
    );

    // ---------------------------------
    // Thin border
    // ---------------------------------

    surface()->DrawSetColor(r, g, b, a / 3);
    surface()->DrawOutlinedRect(
        x, y,
        x + boxW,
        y + boxH
    );

    // ---------------------------------
    // Title
    // ---------------------------------

    const int titleX =
        x + boxW - paddingX - titleW;

    const int titleY = y + paddingY;

    surface()->DrawSetTextFont(m_hFontTitle);
    surface()->DrawSetTextColor(r, g, b, 255);
    surface()->DrawSetTextPos(titleX, titleY);
    surface()->DrawPrintText(
        title,
        wcslen(title)
    );

    // ---------------------------------
    // Website
    // ---------------------------------

    const int websiteX =
        x + boxW - paddingX - websiteW;

    const int websiteY =
        titleY + titleH + spacing;

    surface()->DrawSetTextFont(m_hFontWebsite);
    surface()->DrawSetTextColor(
        175, 185, 195, 235
    );

    surface()->DrawSetTextPos(
        websiteX,
        websiteY
    );

    surface()->DrawPrintText(
        website,
        wcslen(website)
    );

    // ---------------------------------
    // Bottom accent line
    // ---------------------------------

    surface()->DrawSetColor(r, g, b, 220);

    surface()->DrawFilledRect(
        x + 1,
        y + boxH - 2,
        x + boxW - 1,
        y + boxH - 1
    );
}
