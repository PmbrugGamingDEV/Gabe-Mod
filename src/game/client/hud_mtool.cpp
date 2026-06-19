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
    MODE_FACEPOSER
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

    void MsgFunc_MultitoolHUD(bf_read& msg);

private:
    HFont m_hFont;

    int m_iMode;
    int m_iColor;
    int m_iDecal;
    int m_iRenderFx;
    int m_iFace;
    int m_iConstraint;
    int m_iWatermelonColor;
};

DECLARE_HUDELEMENT(CHudMultitool);
DECLARE_HUD_MESSAGE(CHudMultitool, MultitoolHUD);

//=========================================================

CHudMultitool::CHudMultitool(const char* pElementName)
    : CHudElement(pElementName), Panel(NULL, "HudMultitool")
{
    SetParent(g_pClientMode->GetViewport());
    SetHiddenBits(0);
    SetPaintBackgroundEnabled(false);

    HOOK_HUD_MESSAGE(CHudMultitool, MultitoolHUD);

    m_iMode = 0;
    m_iColor = 0;
    m_iDecal = 0;
    m_iRenderFx = 0;
    m_iFace = 0;
    m_iConstraint = 0;
    m_iWatermelonColor = 0;
}

//=========================================================

void CHudMultitool::ApplySchemeSettings(IScheme* pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    m_hFont = surface()->CreateFont();
    surface()->SetFontGlyphSet(
        m_hFont,
        "Consolas",
        18,
        400,
        0,
        0,
        0
    );

    int w, h;
    surface()->GetScreenSize(w, h);
    SetPos(0, 0);
    SetSize(w, h);
}

//=========================================================

bool CHudMultitool::ShouldDraw(void)
{
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
}

//=========================================================

void CHudMultitool::Paint(void)
{
    int sw, sh;
    surface()->GetScreenSize(sw, sh);

    const int padding = 8;
    const int boxWidth = 300;
    const int boxHeight = 420;

    int boxX = sw - boxWidth - 40;
    int boxY = (sh / 2) - (boxHeight / 2);

    // Background
    surface()->DrawSetColor(0, 0, 0, 140);
    surface()->DrawFilledRect(boxX, boxY, boxX + boxWidth, boxY + boxHeight);

    surface()->DrawSetColor(0, 0, 0, 210);
    surface()->DrawOutlinedRect(boxX, boxY, boxX + boxWidth, boxY + boxHeight);

    surface()->DrawSetTextFont(m_hFont);

    int textX = boxX + padding;
    int textY = boxY + padding;

    //---------------------------------------------------------
    // HEADER
    //---------------------------------------------------------

    const wchar_t* modeNames[] =
    {
        L"Remove",
        L"Distance",
        L"Color",
        L"Constraint",
        L"Ignite",
        L"Duplicate",
        L"Explode",
        L"Point Message",
        L"Light Watermelon",
        L"Decal",
        L"Render FX",
        L"Faceposer"
    };

    if (m_iMode >= 0 && m_iMode < 12)
    {
        surface()->DrawSetTextColor(230, 230, 230, 220);
        surface()->DrawSetTextPos(textX, textY);
        surface()->DrawPrintText(modeNames[m_iMode], wcslen(modeNames[m_iMode]));
        textY += 28;
    }

    //---------------------------------------------------------
    // COLOR
    //---------------------------------------------------------

    if (m_iMode == MODE_COLOR || m_iMode == MODE_LIGHT_WATERMELON)
    {
        const wchar_t* colors[] =
        {
            L"Red", L"Orange", L"Yellow", L"Green",
            L"Blue", L"Indigo", L"Purple", L"Violet",
            L"Pink", L"Black", L"White", L"Rainbow"
        };

        for (int i = 0; i < 12; i++)
        {
            surface()->DrawSetTextColor(
                (i == m_iColor) ? 255 : 180,
                (i == m_iColor) ? 220 : 180,
                (i == m_iColor) ? 120 : 180,
                220
            );

            surface()->DrawSetTextPos(textX, textY);
            surface()->DrawPrintText(colors[i], wcslen(colors[i]));
            textY += 22;
        }
    }

    //---------------------------------------------------------
    // DECALS
    //---------------------------------------------------------

    if (m_iMode == MODE_DECAL)
    {
        const wchar_t* decals[] =
        {
            L"YellowBlood", L"Bigshot", L"RedGlowFade", L"BeerSplash",
            L"Blood", L"Scorch", L"ManhackCut", L"FadingScorch",
            L"Rollermine.Crater"
        };

        for (int i = 0; i < 9; i++)
        {
            surface()->DrawSetTextColor(
                (i == m_iDecal) ? 255 : 180,
                (i == m_iDecal) ? 220 : 180,
                (i == m_iDecal) ? 120 : 180,
                220
            );

            surface()->DrawSetTextPos(textX, textY);
            surface()->DrawPrintText(decals[i], wcslen(decals[i]));
            textY += 22;
        }
    }

    //---------------------------------------------------------
    // RENDER FX
    //---------------------------------------------------------

    if (m_iMode == MODE_MATERIAL)
    {
        const wchar_t* fx[] =
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

        for (int i = 0; i < 19; i++)
        {
            surface()->DrawSetTextColor(
                (i == m_iRenderFx) ? 255 : 180,
                (i == m_iRenderFx) ? 220 : 180,
                (i == m_iRenderFx) ? 120 : 180,
                220
            );

            surface()->DrawSetTextPos(textX, textY);
            surface()->DrawPrintText(fx[i], wcslen(fx[i]));
            textY += 22;
        }
    }

    //---------------------------------------------------------
    // FACE POSER
    //---------------------------------------------------------

    if (m_iMode == MODE_FACEPOSER)
    {
        const wchar_t* faces[] =
        {
            L"Smile", L"Sad", L"Angry",
            L"Scared", L"Surprised", L"Neutral"
        };

        for (int i = 0; i < 6; i++)
        {
            surface()->DrawSetTextColor(
                (i == m_iFace) ? 255 : 180,
                (i == m_iFace) ? 220 : 180,
                (i == m_iFace) ? 120 : 180,
                220
            );

            surface()->DrawSetTextPos(textX, textY);
            surface()->DrawPrintText(faces[i], wcslen(faces[i]));
            textY += 22;
        }
    }

    //---------------------------------------------------------
    // CONSTRAINTS
    //---------------------------------------------------------

    if (m_iMode == MODE_CONSTRAINT)
    {
        const wchar_t* constraints[] =
        {
            L"BallSocket", L"Weld", L"Rope"
        };

        for (int i = 0; i < 3; i++)
        {
            surface()->DrawSetTextColor(
                (i == m_iConstraint) ? 255 : 180,
                (i == m_iConstraint) ? 220 : 180,
                (i == m_iConstraint) ? 120 : 180,
                220
            );

            surface()->DrawSetTextPos(textX, textY);
            surface()->DrawPrintText(constraints[i], wcslen(constraints[i]));
            textY += 22;
        }
    }
}