//========= Gabe Mod Custom Main Menu ====================//
//
// Purpose: Custom GameUI main menu replacement
//
//========================================================//

#include "cbase.h"
#include "GameUI/IGameUI.h"
#include "ienginevgui.h"
#include "tier1/interface.h"

#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui/IScheme.h>

#include <vgui_controls/Panel.h>
#include <vgui_controls/Button.h>

#include "tier0/memdbgon.h"

using namespace vgui;

static IGameUI* g_pGameUI = NULL;
static CSysModule* g_pGameUIModule = NULL;

//=========================================================
// Configuration
//=========================================================

ConVar gabe_menu_r("gabe_menu_r", "180", FCVAR_ARCHIVE);
ConVar gabe_menu_g("gabe_menu_g", "190", FCVAR_ARCHIVE);
ConVar gabe_menu_b("gabe_menu_b", "160", FCVAR_ARCHIVE);

ConVar gabe_menu_alpha(
    "gabe_menu_alpha", "180", FCVAR_ARCHIVE);

ConVar gabe_menu_speed(
    "gabe_menu_speed", "9.0", FCVAR_ARCHIVE);

ConVar gabe_menu_x(
    "gabe_menu_x", "0.045", FCVAR_ARCHIVE);

ConVar gabe_menu_y(
    "gabe_menu_y", "0.24", FCVAR_ARCHIVE);

ConVar gabe_menu_gradient(
    "gabe_menu_gradient", "1", FCVAR_ARCHIVE);

ConVar gabe_menu_sounds(
    "gabe_menu_sounds", "1", FCVAR_ARCHIVE,
    "Enable main menu sound effects.");

ConVar gabe_menu_sound_rollover(
    "gabe_menu_sound_rollover",
    "ui/buttonrollover.wav", FCVAR_ARCHIVE);

ConVar gabe_menu_sound_click(
    "gabe_menu_sound_click",
    "ui/buttonclick.wav", FCVAR_ARCHIVE);

ConVar gabe_menu_sound_release(
    "gabe_menu_sound_release",
    "ui/buttonclickrelease.wav", FCVAR_ARCHIVE);

//=========================================================
// Accent color
//=========================================================

static Color GetAccentColor()
{
    return Color(
        clamp(gabe_menu_r.GetInt(), 0, 255),
        clamp(gabe_menu_g.GetInt(), 0, 255),
        clamp(gabe_menu_b.GetInt(), 0, 255),
        255);
}

//=========================================================
// Menu definitions
//=========================================================

struct GabeMenuItem_t
{
    const char* label;
    const char* command;
    bool onlyInGame;
    bool separator;
};

static const GabeMenuItem_t g_MenuItems[] =
{
    { "Resume Game",
      "ResumeGame", true, false },

    { "Disconnect",
      "Disconnect", true, false },

    { "Player List",
      "OpenPlayerListDialog", true, false },

    { "", "", true, true },

    { "Find Servers",
      "OpenServerBrowser", false, false },

    { "New Game",
      "engine gabe_newgame", false, false },

    { "Load Game",
      "OpenLoadGameDialog", false, false },

    { "Save Game",
      "OpenSaveGameDialog", true, false },

    { "Friends",
      "engine gabe_friends", false, false },

    { "Toggle Credits",
      "engine gabe_thanks", false, false },

    { "Tutorials",
      "engine gabe_tutorial", false, false },

    { "Achievements",
      "OpenAchievementsDialog", false, false },

    { "Options",
      "OpenOptionsDialog", false, false },

    { "Quit",
      "Quit", false, false },

    { "", "", false, true },

    { "Script Menu",
      "engine gabe_loadscripts", false, false },

    { "Web Browser",
      "engine gabe_showbrowser", false, false },

    { "Mount Games",
      "engine gabe_mountgames", false, false }
};

static float g_flMenuScale = 1.0f;

static int MenuScale(int value)
{
    return MAX(1, (int)(value * g_flMenuScale + 0.5f));
}

//=========================================================
// Custom animated menu button
//=========================================================

class CGabeMenuButton : public Button
{
    DECLARE_CLASS_SIMPLE(CGabeMenuButton, Button);

public:

    CGabeMenuButton(
        Panel* parent,
        const char* name,
        const char* label,
        const char* command)
        : BaseClass(parent, name, label, parent, command)
    {
        V_UTF8ToUnicode(
            label, m_wszLabel, sizeof(m_wszLabel));

        m_hMenuFont = INVALID_FONT;

        m_flHover = 0.0f;
        m_flPress = 0.0f;

        m_flLastTime = Plat_FloatTime();

        SetPaintBackgroundEnabled(false);
        SetPaintBorderEnabled(false);

        SetMouseInputEnabled(true);
        SetKeyBoardInputEnabled(true);

        // Disable the default VGUI sounds.
        // We handle sounds ourselves so ConVars update live.

        SetArmedSound(NULL);
        SetDepressedSound(NULL);
        SetReleasedSound(NULL);

        m_bWasArmed = false;
        m_bWasDepressed = false;
    }

    //=====================================================
    // Font
    //=====================================================

    void SetMenuFont(HFont font)
    {
        m_hMenuFont = font;
    }

    //=====================================================
    // Sound playback
    //=====================================================

    void PlayMenuSound(const char* pszSound)
    {
        if (!gabe_menu_sounds.GetBool())
            return;

        if (!pszSound || !pszSound[0])
            return;

        surface()->PlaySound(pszSound);
    }

    //=====================================================
    // Animation and sounds
    //=====================================================

    virtual void OnThink()
    {
        BaseClass::OnThink();

        double now = Plat_FloatTime();

        float dt = clamp(
            (float)(now - m_flLastTime),
            0.0f, 0.1f);

        m_flLastTime = now;

        bool bArmed = IsArmed();
        bool bDepressed = IsDepressed();

        // Play sound once when entering the button.

        if (bArmed && !m_bWasArmed)
        {
            PlayMenuSound(
                gabe_menu_sound_rollover.GetString());
        }

        // Play sound once when pressing the button.

        if (bDepressed && !m_bWasDepressed)
        {
            PlayMenuSound(
                gabe_menu_sound_click.GetString());
        }

        // Play sound when releasing over the button.

        if (!bDepressed && m_bWasDepressed && bArmed)
        {
            PlayMenuSound(
                gabe_menu_sound_release.GetString());
        }

        m_bWasArmed = bArmed;
        m_bWasDepressed = bDepressed;

        float speed = MAX(
            0.0f, gabe_menu_speed.GetFloat());

        m_flHover = Approach(
            bArmed ? 1.0f : 0.0f,
            m_flHover,
            dt * speed);

        m_flPress = Approach(
            bDepressed ? 1.0f : 0.0f,
            m_flPress,
            dt * 15.0f);

        Repaint();
    }

    //=====================================================
    // Custom rendering
    //=====================================================

    virtual void Paint()
    {
        int w, h;
        GetSize(w, h);

        Color accent = GetAccentColor();

        int alpha = clamp(
            gabe_menu_alpha.GetInt(), 0, 255);

        // Main charcoal background.

        surface()->DrawSetColor(
            22, 24, 22, alpha);

        surface()->DrawFilledRect(
            0, 0, w, h);

        // Subtle internal gradient.

        for (int y = 0; y < h; y++)
        {
            float fraction =
                (float)y / (float)MAX(1, h);

            int shade =
                (int)((1.0f - fraction) * 20.0f);

            surface()->DrawSetColor(
                255, 255, 255, shade);

            surface()->DrawFilledRect(
                0, y, w, y + 1);
        }

        // Animated hover highlight.

        surface()->DrawSetColor(
            accent.r(),
            accent.g(),
            accent.b(),
            (int)(m_flHover * 105.0f));

        surface()->DrawFilledRect(
            0, 0, w, h);

        // Darken the button when pressed.

        surface()->DrawSetColor(
            0, 0, 0,
            (int)(m_flPress * 90.0f));

        surface()->DrawFilledRect(
            0, 0, w, h);

        // Top bevel.

        surface()->DrawSetColor(
            255, 255, 255,
            20 + (int)(m_flHover * 45));

        surface()->DrawFilledRect(
            0, 0, w, 1);

        // Bottom shadow.

        surface()->DrawSetColor(
            0, 0, 0, 110);

        surface()->DrawFilledRect(
            0, h - 1, w, h);

        // Right border.

        surface()->DrawSetColor(
            0, 0, 0, 65);

        surface()->DrawFilledRect(
            w - 1, 0, w, h);

        // Animated left accent bar.

        int edgeWidth =
            2 + (int)(m_flHover * 2.0f);

        surface()->DrawSetColor(
            accent.r(),
            accent.g(),
            accent.b(),
            50 + (int)(m_flHover * 205));

        surface()->DrawFilledRect(
            0, 0, edgeWidth, h);

        // Draw the button label.

        if (m_hMenuFont == INVALID_FONT)
            return;

        int fontTall =
            surface()->GetFontTall(m_hMenuFont);

        int textX =
            12 + (int)(m_flHover * 6.0f);

        int textY =
            (h - fontTall) / 2;

        int brightness =
            205 + (int)(m_flHover * 50.0f);

        // Move the text slightly when pressed.

        textY += (int)m_flPress;

        surface()->DrawSetTextFont(m_hMenuFont);

        // Text shadow.

        surface()->DrawSetTextColor(
            0, 0, 0, 160);

        surface()->DrawSetTextPos(
            textX + 1, textY + 1);

        surface()->DrawPrintText(
            m_wszLabel,
            V_wcslen(m_wszLabel));

        // Main text.

        surface()->DrawSetTextColor(
            brightness,
            brightness,
            brightness,
            255);

        surface()->DrawSetTextPos(
            textX, textY);

        surface()->DrawPrintText(
            m_wszLabel,
            V_wcslen(m_wszLabel));

        // Small animated hover arrow.

        if (m_flHover > 0.01f)
        {
            int ax = w - 20 +
                (int)(m_flHover * 3.0f);

            int ay = h / 2;

            surface()->DrawSetColor(
                255, 255, 255,
                (int)(m_flHover * 220.0f));

            surface()->DrawLine(
                ax - 3, ay - 4,
                ax + 1, ay);

            surface()->DrawLine(
                ax + 1, ay,
                ax - 3, ay + 4);
        }
    }

private:

    wchar_t m_wszLabel[128];

    HFont m_hMenuFont;

    float m_flHover;
    float m_flPress;

    double m_flLastTime;

    bool m_bWasArmed;
    bool m_bWasDepressed;
};

//=========================================================
// Menu separator
//=========================================================

class CGabeMenuSeparator : public Panel
{
    DECLARE_CLASS_SIMPLE(CGabeMenuSeparator, Panel);

public:

    CGabeMenuSeparator(Panel* parent)
        : BaseClass(parent, "MenuSeparator")
    {
        SetPaintBackgroundEnabled(false);
        SetPaintBorderEnabled(false);

        SetMouseInputEnabled(false);
        SetKeyBoardInputEnabled(false);
    }

    virtual void Paint()
    {
        int w, h;
        GetSize(w, h);

        surface()->DrawSetColor(
            0, 0, 0, 100);

        surface()->DrawFilledRect(
            4, h / 2,
            w - 4, h / 2 + 1);

        surface()->DrawSetColor(
            180, 185, 175, 45);

        surface()->DrawFilledRect(
            4, h / 2 + 1,
            w - 4, h / 2 + 2);
    }
};

//=========================================================
// Main menu panel
//=========================================================

class CGabeMainMenu : public Panel
{
    DECLARE_CLASS_SIMPLE(CGabeMainMenu, Panel);

public:

    CGabeMainMenu(VPANEL parent)
        : BaseClass(NULL, "GabeMainMenu")
    {
        SetParent(parent);

        SetVisible(true);
        SetProportional(false);

        SetMouseInputEnabled(true);
        SetKeyBoardInputEnabled(true);

        SetPaintBackgroundEnabled(false);
        SetPaintBorderEnabled(false);

        m_hFont = INVALID_FONT;
        m_iFontSize = 0;

        m_bInGame = false;

        m_flLastMenuX = gabe_menu_x.GetFloat();
        m_flLastMenuY = gabe_menu_y.GetFloat();

        // Create all menu entries.

        for (int i = 0;
            i < ARRAYSIZE(g_MenuItems);
            i++)
        {
            const GabeMenuItem_t& item =
                g_MenuItems[i];

            if (item.separator)
            {
                m_pItems[i] =
                    new CGabeMenuSeparator(this);
            }
            else
            {
                char name[64];

                Q_snprintf(
                    name, sizeof(name),
                    "MenuButton%d", i);

                m_pItems[i] = new CGabeMenuButton(
                    this,
                    name,
                    item.label,
                    item.command);
            }

            m_pItems[i]->SetVisible(
                !item.onlyInGame);
        }

        ivgui()->AddTickSignal(
            GetVPanel(), 50);
    }

    virtual ~CGabeMainMenu()
    {
        ivgui()->RemoveTickSignal(GetVPanel());
    }

    //=====================================================
    // Resolution-dependent font
    //=====================================================

    void UpdateFonts(int buttonH)
    {
        IScheme* pScheme =
            scheme()->GetIScheme(GetScheme());

        if (!pScheme)
            return;

        const char* pszFont;

        if (buttonH >= 36)
            pszFont = "GabeMenuLarge";
        else if (buttonH >= 28)
            pszFont = "GabeMenuMedium";
        else if (buttonH >= 20)
            pszFont = "GabeMenuSmall";
        else
            pszFont = "GabeMenuTiny";

        m_hFont = pScheme->GetFont(pszFont, false);

        if (m_hFont == INVALID_FONT)
            m_hFont = pScheme->GetFont("Default", false);

        for (int i = 0; i < ARRAYSIZE(g_MenuItems); i++)
        {
            if (g_MenuItems[i].separator)
                continue;

            CGabeMenuButton* pButton =
                static_cast<CGabeMenuButton*>(m_pItems[i]);

            pButton->SetMenuFont(m_hFont);
            pButton->Repaint();
        }
    }

    virtual void ApplySchemeSettings(IScheme* pScheme)
    {
        BaseClass::ApplySchemeSettings(pScheme);

        m_iFontSize = 0;

        InvalidateLayout();
    }

    //=====================================================
    // Menu layout
    //=====================================================

    virtual void PerformLayout()
    {
        BaseClass::PerformLayout();

        int w, h;
        GetSize(w, h);

        if (w <= 0 || h <= 0)
            return;

        // Scale relative to 1920x1080.
        // Use the smaller dimension to support different
        // aspect ratios without stretching the interface.

        float scaleX = (float)w / 1920.0f;
        float scaleY = (float)h / 1080.0f;

        g_flMenuScale = MIN(scaleX, scaleY);

        int margin = (int)(w * clamp(
            gabe_menu_x.GetFloat(), 0.0f, 0.8f));

        // Keep the original logo area clear.

        int logoBottom = (int)(h * 0.35f);

        int y = MAX(logoBottom, (int)(h * clamp(
            gabe_menu_y.GetFloat(), 0.0f, 0.8f)));

        int buttonW = MenuScale(320);
        int buttonH = MenuScale(36);
        int spacing = MenuScale(4);
        int separatorH = MenuScale(12);

        buttonW = MAX(1, MIN(
            buttonW, w - margin - MenuScale(8)));

        int visibleButtons = 0;
        int visibleSeparators = 0;

        for (int i = 0; i < ARRAYSIZE(g_MenuItems); i++)
        {
            if (!m_pItems[i]->IsVisible())
                continue;

            if (g_MenuItems[i].separator)
                visibleSeparators++;
            else
                visibleButtons++;
        }

        // Fit the entire menu below the logo.

        int availableHeight = MAX(
            1, h - y - MenuScale(16));

        int totalHeight =
            visibleButtons * (buttonH + spacing) +
            visibleSeparators * separatorH;

        if (totalHeight > availableHeight)
        {
            float fitScale =
                (float)availableHeight /
                (float)totalHeight;

            buttonH = MAX(1, (int)(buttonH * fitScale));
            spacing = MAX(1, (int)(spacing * fitScale));

            separatorH = MAX(
                1, (int)(separatorH * fitScale));
        }

        // Scale font according to final button height.

        UpdateFonts(buttonH);

        // Apply new dimensions to every visible entry.

        for (int i = 0; i < ARRAYSIZE(g_MenuItems); i++)
        {
            Panel* pPanel = m_pItems[i];

            if (!pPanel->IsVisible())
                continue;

            if (g_MenuItems[i].separator)
            {
                pPanel->SetBounds(
                    margin, y,
                    buttonW, separatorH);

                y += separatorH;
            }
            else
            {
                pPanel->SetBounds(
                    margin, y,
                    buttonW, buttonH);

                y += buttonH + spacing;
            }

            pPanel->InvalidateLayout();
            pPanel->Repaint();
        }

        Repaint();
    }

    //=====================================================
    // Background rendering
    //=====================================================

    virtual void Paint()
    {
        if (!gabe_menu_gradient.GetBool())
            return;

        int w, h;
        GetSize(w, h);

        if (w <= 0 || h <= 0)
            return;

        int gradientWidth = MAX(
            1, (int)(w * 0.42f));

        // Soft gradient behind the buttons.
        // No additional text or graphics.

        for (int x = 0;
            x < gradientWidth;
            x += 4)
        {
            float t =
                (float)x /
                (float)gradientWidth;

            int alpha =
                (int)((1.0f - t) * 110.0f);

            surface()->DrawSetColor(
                0, 0, 0, alpha);

            surface()->DrawFilledRect(
                x, 0,
                MIN(x + 4, gradientWidth),
                h);
        }
    }

    //=====================================================
    // Update menu
    //=====================================================

    virtual void OnTick()
    {
        BaseClass::OnTick();

        VPANEL parent = GetVParent();

        if (parent)
        {
            int w, h;

            ipanel()->GetSize(
                parent, w, h);

            if (w != GetWide() ||
                h != GetTall())
            {
                SetBounds(0, 0, w, h);

                InvalidateLayout();
            }
        }

        bool bInGame =
            engine->IsInGame() &&
            !engine->IsLevelMainMenuBackground();

        // Show or hide in-game-only entries.

        if (m_bInGame != bInGame)
        {
            m_bInGame = bInGame;

            for (int i = 0;
                i < ARRAYSIZE(g_MenuItems);
                i++)
            {
                bool visible =
                    !g_MenuItems[i].onlyInGame ||
                    m_bInGame;

                m_pItems[i]->SetVisible(
                    visible);
            }

            InvalidateLayout();
        }

        // Update menu positioning live.

        if (m_flLastMenuX !=
            gabe_menu_x.GetFloat() ||
            m_flLastMenuY !=
            gabe_menu_y.GetFloat())
        {
            m_flLastMenuX =
                gabe_menu_x.GetFloat();

            m_flLastMenuY =
                gabe_menu_y.GetFloat();

            InvalidateLayout();
        }
    }

    //=====================================================
    // Command handling
    //=====================================================

    virtual void OnCommand(
        const char* command)
    {
        if (!command || !command[0])
            return;

        // Resume the game.

        if (!Q_stricmp(
            command, "ResumeGame"))
        {
            engine->ClientCmd_Unrestricted(
                "gameui_hide\n");

            return;
        }

        // Execute custom Gabe Mod commands.

        if (!Q_strnicmp(
            command, "engine ", 7))
        {
            char buffer[256];

            Q_snprintf(
                buffer,
                sizeof(buffer),
                "%s\n",
                command + 7);

            engine->ClientCmd_Unrestricted(
                buffer);

            return;
        }

        // Forward the remaining commands
        // to the original GameUI module.

        if (g_pGameUI)
        {
            g_pGameUI->SendMainMenuCommand(
                command);

            return;
        }

        BaseClass::OnCommand(command);
    }

private:

    Panel* m_pItems[
        ARRAYSIZE(g_MenuItems)];

    HFont m_hFont;

    int m_iFontSize;

    bool m_bInGame;

    float m_flLastMenuX;
    float m_flLastMenuY;
};

//=========================================================
// Main menu instance
//=========================================================

static CGabeMainMenu* g_pMainMenu = NULL;

//=========================================================
// Initialization
//=========================================================

void CustomMainMenu_Init()
{
    if (g_pMainMenu)
        return;

    if (!enginevgui)
        return;

    // Obtain the GameUI module.

    g_pGameUIModule =
        Sys_LoadModule("GameUI");

    if (!g_pGameUIModule)
    {
        Warning(
            "Failed to load GameUI.\n");

        return;
    }

    // Obtain the GameUI factory.

    CreateInterfaceFn factory =
        Sys_GetFactory(g_pGameUIModule);

    if (factory)
    {
        g_pGameUI =
            static_cast<IGameUI*>(
                factory(
                    GAMEUI_INTERFACE_VERSION,
                    NULL));
    }

    // Obtain the GameUI root panel.

    VPANEL parent =
        enginevgui->GetPanel(
            PANEL_GAMEUIDLL);

    if (!g_pGameUI || !parent)
    {
        Warning(
            "Failed to initialize custom menu.\n");

        g_pGameUI = NULL;

        Sys_UnloadModule(
            g_pGameUIModule);

        g_pGameUIModule = NULL;

        return;
    }

    // Create the new main menu.

    g_pMainMenu =
        new CGabeMainMenu(parent);

    int w, h;

    ipanel()->GetSize(
        parent, w, h);

    g_pMainMenu->SetBounds(
        0, 0, w, h);

    // Replace the original GameUI main menu.

    g_pGameUI->SetMainMenuOverride(
        g_pMainMenu->GetVPanel());

    g_pMainMenu->InvalidateLayout();

    Msg("Custom main menu installed.\n");
}

//=========================================================
// Shutdown
//=========================================================

void CustomMainMenu_Shutdown()
{
    // Restore the original GameUI main menu.

    if (g_pGameUI && g_pMainMenu)
    {
        g_pGameUI->SetMainMenuOverride(0);
    }

    // Delete the custom panel.

    delete g_pMainMenu;

    g_pMainMenu = NULL;
    g_pGameUI = NULL;

    // Release the GameUI module reference.

    if (g_pGameUIModule)
    {
        Sys_UnloadModule(
            g_pGameUIModule);

        g_pGameUIModule = NULL;
    }
}

//=========================================================
// Console commands
//=========================================================

CON_COMMAND(
    gabe_menu_install,
    "Installs the custom main menu.")
{
    CustomMainMenu_Init();
}

CON_COMMAND(
    gabe_menu_reload,
    "Reloads the custom main menu.")
{
    CustomMainMenu_Shutdown();
    CustomMainMenu_Init();
}