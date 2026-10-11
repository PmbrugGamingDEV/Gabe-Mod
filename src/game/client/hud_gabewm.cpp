#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/Panel.h"

// Memdbgon.h MUST be the last include in a .cpp file!!!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern IClientMode* g_pClientMode;

#define FONTFLAG_ANTIALIAS 0x010

ConVar watermark_r("watermark_r", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark color Red.");
ConVar watermark_g("watermark_g", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark color Green.");
ConVar watermark_b("watermark_b", "255", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark color Blue.");
ConVar watermark_a("watermark_a", "200", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Watermark alpha.");

// ----------------------------------------------------------------------

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
    : CHudElement(pElementName), Panel(NULL, "HudWatermark")
{
    SetParent(g_pClientMode->GetViewport());
    SetHiddenBits(0);

    int w, h;
    surface()->GetScreenSize(w, h);
    SetPos(0, 0);
    SetSize(w, h);
}

void CHudWatermark::ApplySchemeSettings(IScheme* pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    m_hFontTitle = pScheme->GetFont("Tahoma", true);
    if (!m_hFontTitle)
    {
        m_hFontTitle = surface()->CreateFont();
        surface()->SetFontGlyphSet(m_hFontTitle, "Tahoma", 24, 700, 0, 0, FONTFLAG_ANTIALIAS);
    }

    m_hFontWebsite = pScheme->GetFont("Verdana", true);
    if (!m_hFontWebsite)
    {
        m_hFontWebsite = surface()->CreateFont();
        surface()->SetFontGlyphSet(m_hFontWebsite, "Verdana", 16, 500, 0, 0, FONTFLAG_ANTIALIAS);
    }

    int w, h;
    surface()->GetScreenSize(w, h);
    SetPos(0, 0);
    SetSize(w, h);
}

bool CHudWatermark::ShouldDraw(void)
{
    return true;
}

void CHudWatermark::Paint(void)
{
    int screenW, screenH;
    surface()->GetScreenSize(screenW, screenH);

    wchar_t wszTitle[256];
    g_pVGuiLocalize->ConvertANSIToUnicode("Gabe Mod", wszTitle, sizeof(wszTitle));

    wchar_t wszWebsite[256] = L"";
    V_wcsncpy(
        wszWebsite,
        L"https://sites.google.com/view/pmbruggaming",
        sizeof(wszWebsite)
    );

    int r = watermark_r.GetInt();
    int g = watermark_g.GetInt();
    int b = watermark_b.GetInt();
    int a = watermark_a.GetInt();

    int titleW, titleH;
    surface()->GetTextSize(m_hFontTitle, wszTitle, titleW, titleH);

    int websiteW = 0, websiteH = 0;
    if (wcslen(wszWebsite) > 0)
        surface()->GetTextSize(m_hFontWebsite, wszWebsite, websiteW, websiteH);

    const int padding = 6;
    const int fadeSize = 8;

    int boxW = max(titleW, websiteW) + padding * 2;
    int boxH = titleH + (websiteH ? websiteH + 2 : 0) + padding * 2;

    int x = screenW - boxW - 10;
    int y = 10;

    // --------------------------------------------------
    // Fade-out edges
    // --------------------------------------------------
    for (int i = fadeSize; i > 0; --i)
    {
        int alpha = (a * i) / (fadeSize * 2);

        surface()->DrawSetColor(0, 0, 0, alpha);
        surface()->DrawFilledRect(
            x - i,
            y - i,
            x + boxW + i,
            y + boxH + i
        );
    }

    // --------------------------------------------------
    // Solid center box
    // --------------------------------------------------
    surface()->DrawSetColor(0, 0, 0, a);
    surface()->DrawFilledRect(
        x,
        y,
        x + boxW,
        y + boxH
    );

    // --------------------------------------------------
    // Text Positions
    // --------------------------------------------------
    int titleX = x + boxW - padding - titleW;
    int titleY = y + padding;

    int websiteX = x + boxW - padding - websiteW;
    int websiteY = titleY + titleH + 2;

    // --------------------------------------------------
    // Title
    // --------------------------------------------------
    surface()->DrawSetTextFont(m_hFontTitle);
    surface()->DrawSetTextColor(r, g, b, a);
    surface()->DrawSetTextPos(titleX, titleY);
    surface()->DrawPrintText(wszTitle, wcslen(wszTitle));

    // --------------------------------------------------
    // Website
    // --------------------------------------------------
    if (wcslen(wszWebsite) > 0)
    {
        surface()->DrawSetTextFont(m_hFontWebsite);
        surface()->DrawSetTextColor(r, g, b, a);
        surface()->DrawSetTextPos(websiteX, websiteY);
        surface()->DrawPrintText(wszWebsite, wcslen(wszWebsite));
    }
}