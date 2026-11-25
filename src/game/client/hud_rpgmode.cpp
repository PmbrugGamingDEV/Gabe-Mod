// GabeMod RPG selection HUD - Source SDK Base 2013 Multiplayer
// Client DLL. Uses the server message GabeRPGMode (2 bytes).

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_baseplayer.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Label.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "tier0/memdbgon.h"
#include "c_basecombatweapon.h"

using namespace vgui;

// MUST stay in the same order as g_RPGLaunchItems in weapon_rpg.cpp.
static const char *s_RPGItemNames[] =
{
    "RPG Rocket", "Oil Drum", "Gas Can", "Propane Tank",
    "Wood Crate", "Metal Barrel", "Cardboard Box", "Watermelon",
    "Plastic Bottle", "Paint Can", "Coffee Mug", "Glass Bottle",
    "Cinder Block", "Saw Blade", "Chair", "Office Chair",
    "Table", "Couch", "Filing Cabinet", "Monitor", "Keyboard",
    "Computer Tower", "Radio", "TV", "Traffic Cone", "Trash Bin",
    "Car", "Van", "APC", "Tire", "Skull", "Antlion Gib", "Spine"
};
static const char *s_RPGSpeedNames[] = { "Slow", "Medium", "Fast", "Flash" };

class CHudGabeRPGMode : public CHudElement, public Panel
{
    DECLARE_CLASS_SIMPLE(CHudGabeRPGMode, Panel);
public:
    CHudGabeRPGMode(const char *name)
        : CHudElement(name), BaseClass(g_pClientMode->GetViewport(), "HudGabeRPGMode")
    {
        m_nItem = 0;
        m_nSpeed = 1;
        SetSize(244, 103);
        SetPaintBorderEnabled(false);
        SetPaintBackgroundEnabled(true);
        SetMouseInputEnabled(false);
        SetKeyBoardInputEnabled(false);

        m_pHeading = new Label(this, "Heading", "GABEMOD  |  RPG LAUNCHER");
        m_pItem = new Label(this, "Item", "Launch: RPG Rocket");
        m_pSpeed = new Label(this, "Speed", "Fire rate: Medium");
        m_pHelp = new Label(this, "Help", "MOUSE2  Select    R  Speed");

        Label *labels[] = { m_pHeading, m_pItem, m_pSpeed, m_pHelp };
        for (int i = 0; i < ARRAYSIZE(labels); ++i)
        {
            labels[i]->SetPaintBackgroundEnabled(false);
            labels[i]->SetContentAlignment(Label::a_west);
            labels[i]->SetMouseInputEnabled(false);
        }
        m_pHeading->SetFgColor(Color(170, 218, 255, 255));
        m_pItem->SetFgColor(Color(255, 255, 255, 255));
        m_pSpeed->SetFgColor(Color(235, 235, 235, 255));
        m_pHelp->SetFgColor(Color(190, 200, 208, 255));
        SetVisible(false);
        ivgui()->AddTickSignal(GetVPanel(), 100);
    }

    void Init() override;
	bool ShouldDraw() override 
    { 
        C_BaseCombatWeapon* pWeapon = C_BasePlayer::GetLocalPlayer() ? C_BasePlayer::GetLocalPlayer()->GetActiveWeapon() : NULL; 
    
        return pWeapon && !Q_stricmp(pWeapon->GetClassname(), "weapon_rpg"); 
    }

    void MsgFunc_GabeRPGMode(bf_read &msg)
    {
        m_nItem = clamp(msg.ReadByte(), 0, (int)ARRAYSIZE(s_RPGItemNames) - 1);
        m_nSpeed = clamp(msg.ReadByte(), 0, (int)ARRAYSIZE(s_RPGSpeedNames) - 1);
        char text[128];
        Q_snprintf(text, sizeof(text), "Launch: %s", s_RPGItemNames[m_nItem]);
        m_pItem->SetText(text);
        Q_snprintf(text, sizeof(text), "Fire rate: %s", s_RPGSpeedNames[m_nSpeed]);
        m_pSpeed->SetText(text);
    }

    void PerformLayout() override
    {
        BaseClass::PerformLayout();
        m_pHeading->SetBounds(12, 7, 218, 19);
        m_pItem->SetBounds(12, 32, 218, 20);
        m_pSpeed->SetBounds(12, 52, 218, 20);
        m_pHelp->SetBounds(12, 80, 218, 16);
    }

    void PaintBackground() override
    {
        surface()->DrawSetColor(12, 22, 28, 195);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(100, 165, 205, 180);
        surface()->DrawFilledRect(0, 0, GetWide(), 2);
        surface()->DrawSetColor(115, 145, 160, 150);
        surface()->DrawOutlinedRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(90, 112, 125, 90);
        surface()->DrawFilledRect(12, 75, GetWide()-12, 76);
    }

    void OnTick() override
    {
        SetPos(ScreenWidth() - GetWide() - 14, 14);
        C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();
        C_BaseCombatWeapon *pWeapon = pLocal ? pLocal->GetActiveWeapon() : NULL;
        bool bShow = pWeapon && !Q_stricmp(pWeapon->GetClassname(), "weapon_rpg") &&
                     (!pLocal || pLocal->IsAlive());
        if (IsVisible() != bShow)
            SetVisible(bShow);
    }

private:
    int m_nItem, m_nSpeed;
    Label *m_pHeading;
    Label *m_pItem;
    Label *m_pSpeed;
    Label *m_pHelp;
};

DECLARE_HUDELEMENT(CHudGabeRPGMode);
DECLARE_HUD_MESSAGE(CHudGabeRPGMode, GabeRPGMode);

void CHudGabeRPGMode::Init()
{
    HOOK_HUD_MESSAGE(CHudGabeRPGMode, GabeRPGMode);
}
