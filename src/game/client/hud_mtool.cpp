//=========================================================
// Gabe Mod – Multitool HUD (All Modes)
// Source SDK Base 2007 – HL2MP
//=========================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Panel.h"
#include "vgui/ILocalize.h"

#include "tier0/memdbgon.h"

using namespace vgui;

extern IClientMode* g_pClientMode;

//=========================================================
// MODE ENUM (must match server order)
//=========================================================

enum
{
    MODE_REMOVE = 0,
    MODE_DISTANCE,
    MODE_COLOR,
    MODE_CONSTRAINT,
    MODE_IGNITE,
    MODE_DUPLICATE,
    MODE_EXPLODE,
    MODE_POINTMESSAGE,
    MODE_LIGHT_WATERMELON,
    MODE_DECAL,
    MODE_MATERIAL,
    MODE_FACEPOSER,
    MODE_BALLOON,
    MODE_THRUSTER
};

//=========================================================

class CHudMultitool : public CHudElement, public Panel
{
public:
    DECLARE_CLASS_SIMPLE(CHudMultitool, Panel);

    CHudMultitool(const char* pElementName);

    virtual void ApplySchemeSettings(IScheme* pScheme);
    virtual bool ShouldDraw(void);
    virtual void Paint(void);
    virtual void OnScreenSizeChanged(int oldWide, int oldTall);

    void MsgFunc_MultitoolHUD(bf_read& msg);

private:
    HFont m_hTitleFont;
    HFont m_hFont;
    void DrawLine(HFont font, int x, int y, const wchar_t* text, Color color);
    void DrawSelection(int x, int& y, const wchar_t* label,
        const wchar_t* const* names, int count, int index);

    int m_iMode;
    int m_iColor;
    int m_iDecal;
    int m_iRenderFx;
    int m_iFace;
    int m_iConstraint;
    int m_iWatermelonColor;
    int m_iBalloonLift;
    int m_iThrusterPower;
};

DECLARE_HUDELEMENT(CHudMultitool);
DECLARE_HUD_MESSAGE(CHudMultitool, MultitoolHUD);

//=========================================================

CHudMultitool::CHudMultitool(const char* pElementName)
    : CHudElement(pElementName), Panel(NULL, "HudMultitool")
{
    SetParent(g_pClientMode->GetViewport());
    SetHiddenBits(HIDEHUD_PLAYERDEAD);
    SetMouseInputEnabled(false);
    SetKeyBoardInputEnabled(false);
    m_hTitleFont = INVALID_FONT;
    m_hFont = INVALID_FONT;
    SetPaintBackgroundEnabled(false);

    HOOK_HUD_MESSAGE(CHudMultitool, MultitoolHUD);

    m_iMode = 0;
    m_iColor = 0;
    m_iDecal = 0;
    m_iRenderFx = 0;
    m_iFace = 0;
    m_iConstraint = 0;
    m_iWatermelonColor = 0;
    m_iBalloonLift = 0;
    m_iThrusterPower = 0;
}

//=========================================================

void CHudMultitool::ApplySchemeSettings(IScheme* pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    // Use scheme fonts so video mode changes do not invalidate custom glyphs.
    m_hTitleFont = pScheme->GetFont("Trebuchet24", true);
    if (m_hTitleFont == INVALID_FONT)
        m_hTitleFont = pScheme->GetFont("DefaultLarge", true);
    m_hFont = pScheme->GetFont("DefaultSmall", true);
    if (m_hFont == INVALID_FONT)
        m_hFont = pScheme->GetFont("Default", true);
    if (m_hTitleFont == INVALID_FONT)
        m_hTitleFont = m_hFont;
    SetPaintBackgroundEnabled(false);
    SetPaintBorderEnabled(false);

    int w, h;
    surface()->GetScreenSize(w, h);
    SetPos(0, 0);
    SetSize(w, h);
}

//=========================================================

bool CHudMultitool::ShouldDraw(void)
{
    if (!CHudElement::ShouldDraw())
        return false;

    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
    if (!pPlayer)
        return false;

    C_BaseCombatWeapon* pWeapon = pPlayer->GetActiveWeapon();
    if (!pWeapon)
        return false;

    if (FClassnameIs(pWeapon, "weapon_multitool"))
        return true;

    return false;
}

//=========================================================

void CHudMultitool::MsgFunc_MultitoolHUD(bf_read& msg)
{
    m_iMode = msg.ReadByte();
    m_iColor = msg.ReadByte();
    m_iDecal = msg.ReadByte();
    m_iRenderFx = msg.ReadByte();
    m_iFace = msg.ReadByte();
    m_iConstraint = msg.ReadByte();
    m_iWatermelonColor = msg.ReadByte();
    m_iBalloonLift = msg.ReadByte();
    m_iThrusterPower = msg.ReadByte();
}

//=========================================================

void CHudMultitool::DrawLine(HFont font, int x, int y,
    const wchar_t* text, Color color)
{
    if (font == INVALID_FONT || !text || !text[0])
        return;
    surface()->DrawSetTextFont(font);
    surface()->DrawSetTextColor(0, 0, 0, 210);
    surface()->DrawSetTextPos(x + 1, y + 1);
    surface()->DrawPrintText(text, wcslen(text));
    surface()->DrawSetTextColor(color.r(), color.g(), color.b(), color.a());
    surface()->DrawSetTextPos(x, y);
    surface()->DrawPrintText(text, wcslen(text));
}

void CHudMultitool::DrawSelection(int x, int& y, const wchar_t* label,
    const wchar_t* const* names, int count, int index)
{
    DrawLine(m_hFont, x, y, label, Color(230, 230, 230, 255));
    int labelW, labelH;
    surface()->GetTextSize(m_hFont, label, labelW, labelH);
    DrawLine(m_hFont, x + labelW + 6, y,
        (index >= 0 && index < count) ? names[index] : L"Unknown",
        Color(255, 255, 255, 255));
    y += surface()->GetFontTall(m_hFont) + 3;
}

void CHudMultitool::Paint(void)
{
    if (m_hFont == INVALID_FONT || m_hTitleFont == INVALID_FONT)
        return;

    struct ToolInfo { const wchar_t* title; const wchar_t* description; };
    static const ToolInfo tools[] =
    {
        { L"Remove", L"Remove objects from the world." },
        { L"Distance", L"Measure distances between points." },
        { L"Color", L"Change the color of objects." },
        { L"Constraint", L"Connect objects using physics constraints." },
        { L"Ignite", L"Set objects on fire." },
        { L"Duplicator", L"Duplicate objects." },
        { L"Explode", L"Create explosions." },
        { L"Point Message", L"Place messages in the world." },
        { L"Light Watermelon", L"Create a colored watermelon light." },
        { L"Decal", L"Apply decals to surfaces." },
        { L"Render FX", L"Change the rendering effect of objects." },
        { L"Faceposer", L"Change facial expressions." },
        { L"Balloon", L"Create balloons with adjustable lift." },
        { L"Thruster", L"Create thrusters with adjustable power." }
    };

    if (m_iMode < 0 || m_iMode >= ARRAYSIZE(tools))
        return;

    int sw, sh;
    surface()->GetScreenSize(sw, sh);
    const int x = MAX(12, sw / 50);
    int y = MAX(10, sh / 45);
    DrawLine(m_hTitleFont, x, y, tools[m_iMode].title, Color(255, 255, 255, 255));
    y += surface()->GetFontTall(m_hTitleFont) + 3;
    DrawLine(m_hFont, x, y, tools[m_iMode].description, Color(240, 240, 240, 255));
    y += surface()->GetFontTall(m_hFont) + 5;

    static const wchar_t* colors[] =
    {
        L"Red", L"Orange", L"Yellow", L"Green",
        L"Blue", L"Indigo", L"Purple", L"Violet",
        L"Pink", L"Black", L"White", L"Rainbow"
    };

    static const wchar_t* decals[] =
    {
        L"YellowBlood",
        L"Bigshot",
        L"RedGlowFade",
        L"BeerSplash",
        L"Blood",
        L"Scorch",
        L"ManhackCut",
        L"FadingScorch",
        L"Rollermine.Crater",
        L"Impact.Concrete",
        L"Impact.Metal",
        L"Impact.Glass",
        L"Impact.Sand"
    };

    static const wchar_t* fx[] =
    {
        L"None",
        L"Pulse Slow",
        L"Pulse Fast",
        L"Pulse Wide",
        L"Fade Slow",
        L"Fade Fast",
        L"Solid Slow",
        L"Solid Fast",
        L"Strobe Slow",
        L"Strobe Fast",
        L"Strobe Faster",
        L"Flicker Slow",
        L"Flicker Fast",
        L"No Dissipation",
        L"Distort",
        L"Hologram",
        L"Explode",
        L"Glow Shell",
        L"Clamp Min Scale"
    };

    static const wchar_t* faces[] =
    {
        L"Smile", L"Sad", L"Angry",
        L"Scared", L"Surprised", L"Neutral"
    };

    static const wchar_t* constraints[] =
    {
        L"BallSocket", L"Weld", L"Rope", L"Slider", L"Hinge/Axis"
    };

    static const wchar_t* liftNames[] =
    {
        L"100", L"200", L"300", L"400", L"500",
        L"600", L"700", L"800", L"900", L"1000"
    };

    static const wchar_t* powerNames[] =
    {
        L"Low - Red (500)",
        L"Medium - Green (1500)",
        L"High - Blue (4000)"
    };

    // Show only the currently selected option, not the entire list.
    switch (m_iMode)
    {
    case MODE_COLOR: DrawSelection(x, y, L"Color:", colors, ARRAYSIZE(colors), m_iColor); break;
    case MODE_LIGHT_WATERMELON: DrawSelection(x, y, L"Light color:", colors, ARRAYSIZE(colors), m_iWatermelonColor); break;
    case MODE_DECAL: DrawSelection(x, y, L"Decal:", decals, ARRAYSIZE(decals), m_iDecal); break;
    case MODE_MATERIAL: DrawSelection(x, y, L"Effect:", fx, ARRAYSIZE(fx), m_iRenderFx); break;
    case MODE_FACEPOSER: DrawSelection(x, y, L"Expression:", faces, ARRAYSIZE(faces), m_iFace); break;
    case MODE_CONSTRAINT: DrawSelection(x, y, L"Constraint:", constraints, ARRAYSIZE(constraints), m_iConstraint); break;
    case MODE_BALLOON: DrawSelection(x, y, L"Lift:", liftNames, ARRAYSIZE(liftNames), m_iBalloonLift); break;
    case MODE_THRUSTER:
        DrawSelection(x, y, L"Power:", powerNames, ARRAYSIZE(powerNames), m_iThrusterPower);
        DrawLine(m_hFont, x, y, L"Reload: cycle thruster power", Color(230, 230, 230, 255));
        break;
    }
}

void CHudMultitool::OnScreenSizeChanged(int oldWide, int oldTall)
{
    BaseClass::OnScreenSizeChanged(oldWide, oldTall);
    int w, h;
    surface()->GetScreenSize(w, h);
    SetBounds(0, 0, w, h);
    InvalidateLayout(true, true);
}
