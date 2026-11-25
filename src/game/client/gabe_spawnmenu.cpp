// GabeMod spawn menu: Source SDK Base 2013 (client DLL)
// Place in game/client/hl2mp/ or another compiled client folder.
#include "cbase.h"
#include <math.h>
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>
#include <vgui/IPanel.h>
#include "matsys_controls/mdlpanel.h"
#include "matsys_controls/matsyscontrols.h"
#include "filesystem.h"
#include "ienginevgui.h"
#include "tier1/KeyValues.h"
#include "tier1/strtools.h"
#include "tier1/utlvector.h"
#include "vscript_shared.h"
#include "gabe_spawnmenu.h"
#include "vgui_all.h"
#include <stdlib.h>
#include <string.h>

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
extern IVEngineClient* engine;
#endif

// memdbgon must be the last include!
#include "tier0/memdbgon.h"

using namespace vgui;
static bool g_bSpawnMenuOpen = false;
static ConVar gabe_spawnicon_zoom("gabe_spawnicon_zoom", "1.35", FCVAR_ARCHIVE, "Model icon magnification.", true, 0.5f, true, 2.5f);
static ConVar gabe_spawnmenu_debug("gabe_spawnmenu_debug", "0", FCVAR_NONE, "Print spawn menu button and tab events.");

#define SPAWNMENU_W 1024
#define SPAWNMENU_H 768
#define SPAWNMENU_MAX_TABS 32

// Live 3D icon. Suppress model animation ticks while its tab is hidden.
class CSpawnModelIcon : public CMDLPanel
{
public:
    CSpawnModelIcon(Panel* parent, const char* name) : CMDLPanel(parent, name) {}
    virtual void OnTick()
    {
        if (ipanel()->IsFullyVisible(GetVPanel())) CMDLPanel::OnTick();
    }
};

// Flat dark-tab / translucent button style from the reference image.
class CSpawnFlatButton : public Button
{
    DECLARE_CLASS_SIMPLE(CSpawnFlatButton, Button);
public:
    CSpawnFlatButton(Panel* parent, const char* name,
        const char* text, Panel* target, const char* command,
        bool tab = false)
        : BaseClass(parent, name, text), m_bTab(tab), m_bSelected(false), m_pTarget(target), m_bTile(false), m_pModel(NULL), m_bModelTried(false), m_pAuthor(NULL)
    {
        m_szModel[0] = 0;
        SetPaintBorderEnabled(false);
        SetMouseInputEnabled(true);
        SetEnabled(true);
        Q_strncpy(m_szCommand, command ? command : "", sizeof(m_szCommand));
        // The spawn menu performs actions on mouse press. Source VGUI's
        // default Button requires a complete press/release over the control,
        // which can fail when the game or another panel changes mouse focus.
        SetButtonActivationType(ACTIVATE_ONPRESSED);
        SetUseCaptureMouse(false);
        SetContentAlignment(Label::a_west);
        SetTextInset(5, 0);
        SetDefaultColor(Color(238, 238, 238, 255), Color(238, 238, 238, 255));
        SetArmedColor(Color(255, 255, 255, 255), Color(255, 255, 255, 255));
    }

    virtual void ApplySchemeSettings(IScheme* pScheme)
    {
        BaseClass::ApplySchemeSettings(pScheme);
        SetPaintBorderEnabled(false);

        const Color white(255, 255, 255, 255);
        SetFgColor(white);
        SetDefaultColor(white, white);
        SetArmedColor(white, white);
        SetDepressedColor(white, white);
        SetSelectedColor(white, white);
        if (!m_bTile) return;

        // Create our own font so no ClientScheme.res changes are required.
        static HFont tileFont = INVALID_FONT;
        if (tileFont == INVALID_FONT)
        {
            tileFont = surface()->CreateFont();
            surface()->SetFontGlyphSet(tileFont, "Tahoma", 10, 700, 0, 0,
                ISurface::FONTFLAG_ANTIALIAS | ISurface::FONTFLAG_DROPSHADOW);
        }
        if (tileFont != INVALID_FONT) SetFont(tileFont);
        UpdateTileAlignment();
    }

    void SetWeaponAuthor(const char* author)
    {
        m_pAuthor = new Label(this, "WeaponAuthor", author);
        m_pAuthor->SetMouseInputEnabled(false);
        m_pAuthor->SetKeyBoardInputEnabled(false);
        m_pAuthor->SetContentAlignment(Label::a_west);
        InvalidateLayout();
    }

    void UpdateTileAlignment()
    {
        // A tile is text-only until its model preview actually loads.
        SetContentAlignment(m_pModel ? Label::a_south : Label::a_center);
        SetTextInset(3, m_pModel ? 4 : 0);
    }

    void SetTile(const char* value)
    {
        m_bTile = true;
        UpdateTileAlignment();
        InvalidateLayout(false, true);
        Q_strncpy(m_szModel, value ? value : "", sizeof(m_szModel));
        const int len = Q_strlen(m_szModel);
        if (len < 5 || Q_stricmp(m_szModel + len - 4, ".mdl") ||
            Q_strnicmp(m_szModel, "models/", 7))
            m_szModel[0] = 0;
    }

    virtual void OnThink()
    {
        BaseClass::OnThink();
        if (!m_bTile || !ipanel()->IsFullyVisible(GetVPanel())) return;
        if (m_bModelTried) return;
        // Never construct CMDLPanel before its material interfaces are ready.
        if (!vgui::MaterialSystem() || !vgui::MDLCache() ||
            !vgui::StudioRender() || !vgui::MatSystemSurface() ||
            !vgui::MaterialSystemHardwareConfig()) return;
        m_bModelTried = true;
        if (!m_szModel[0] || !filesystem->FileExists(m_szModel, "GAME")) return;

        // Validate the model before handing it to the viewer.
        MDLHandle_t handle = vgui::MDLCache()->FindMDL(m_szModel);
        if (handle == MDLHANDLE_INVALID) return;
        if (vgui::MDLCache()->IsErrorModel(handle))
        {
            vgui::MDLCache()->Release(handle);
            return;
        }
        m_pModel = new CSpawnModelIcon(this, "ModelIcon");
        m_pModel->SetMouseInputEnabled(false);
        m_pModel->SetKeyBoardInputEnabled(false);
        m_pModel->SetPaintBorderEnabled(false);
        m_pModel->SetGroundGrid(false);
        m_pModel->SetLookAtCamera(false);
        m_pModel->SetMDL(handle);
        vgui::MDLCache()->Release(handle);
        m_pModel->SetModelAnglesAndPosition(QAngle(0, 35, 0), vec3_origin);
        m_pModel->SetBounds(3, 3, MAX(1, GetWide() - 6), MAX(1, GetTall() - 28));
        m_pModel->LookAtMDL();
        const float fov = DEG2RAD((float)m_pModel->GetCameraFOV());
        m_pModel->SetCameraFOV(RAD2DEG(2.0f * atanf(tanf(fov * 0.5f) / gabe_spawnicon_zoom.GetFloat())));
        m_pModel->SetLockView(true);
        m_pModel->SetVisible(true);
        UpdateTileAlignment();
        InvalidateLayout();
        Repaint();
    }
    virtual void PerformLayout()
    {
        BaseClass::PerformLayout();
        if (m_pAuthor)
        {
            int split = GetWide() * 65 / 100;
            m_pAuthor->SetBounds(split + 6, 0, MAX(1, GetWide() - split - 8), GetTall());
            m_pAuthor->SetFgColor(Color(255, 255, 255, 255));
        }
        if (m_pModel)
        {
            m_pModel->SetBounds(3, 3, MAX(1, GetWide() - 6), MAX(1, GetTall() - 28));
        }
    }
    void SetSelected(bool selected) { m_bSelected = selected; Repaint(); }

    // Dispatch directly instead of depending on queued VGUI action signals.
    virtual void OnMouseWheeled(int delta) { char cmd[64]; Q_snprintf(cmd, sizeof(cmd), "wheel_%d", delta); if (m_pTarget) m_pTarget->OnCommand(cmd); }
    virtual void OnMousePressed(MouseCode code)
    {
        if (code != MOUSE_LEFT || !IsEnabled())
        {
            BaseClass::OnMousePressed(code);
            return;
        }

        if (gabe_spawnmenu_debug.GetBool())
            Msg("GabeSpawnMenu: pressed %s (%s)\n", GetName(), m_szCommand);

        if (m_pTarget && m_szCommand[0])
            m_pTarget->OnCommand(m_szCommand);
    }

    virtual void PaintBackground()
    {
        int shade = m_bTile ? 105 : (m_bTab ? 125 : 65);
        if (m_bSelected) shade = 150;
        if (IsArmed()) shade += 25;
        if (IsDepressed()) shade = 75;
        surface()->DrawSetColor(shade, shade, shade, 255);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        if (m_bTile)
        {
            for (int y = 1; y < GetTall() - 1; ++y)
            {
                int v = shade + 22 - 44 * y / MAX(1, GetTall());
                surface()->DrawSetColor(v, v, v, 255);
                surface()->DrawFilledRect(1, y, GetWide() - 1, y + 1);
            }
        }
        surface()->DrawSetColor(170, 170, 170, 255);
        surface()->DrawOutlinedRect(0, 0, GetWide(), GetTall());
    }
private:
    bool m_bTab, m_bSelected;
    Panel* m_pTarget;
    char m_szCommand[64];
    bool m_bTile;
    CMDLPanel* m_pModel;
    bool m_bModelTried;
    char m_szModel[256];
    Label* m_pAuthor;

public:
    void SetSpawnCommand(const char* command)
    {
        Q_strncpy(m_szCommand, command ? command : "", sizeof(m_szCommand));
    }
};



class CToolSettingsPanel : public Panel
{
    DECLARE_CLASS_SIMPLE(CToolSettingsPanel, Panel);
public:
    struct SettingEntry_t
    {
        CSpawnFlatButton* button;
        char command[128];
    };

    CToolSettingsPanel(Panel* parent) : BaseClass(parent, "ToolSettings")
    {
        SetPaintBackgroundEnabled(true);
        m_pTitle = new Label(this, "ToolSettingsTitle", "Tool Settings");
        m_pDesc = new Label(this, "ToolSettingsDesc", "Click a tool on the left to configure it here.");
        m_pSection = new Label(this, "ToolSettingsSection", "Settings");
        m_pTitle->SetContentAlignment(Label::a_west);
        m_pDesc->SetContentAlignment(Label::a_northwest);
        m_pSection->SetContentAlignment(Label::a_west);
        m_pDesc->SetWrap(true);
        m_szToolName[0] = 0;
        m_szToolCommand[0] = 0;
    }

    virtual void ApplySchemeSettings(IScheme* pScheme)
    {
        BaseClass::ApplySchemeSettings(pScheme);
        static HFont titleFont = INVALID_FONT;
        static HFont sectionFont = INVALID_FONT;
        static HFont textFont = INVALID_FONT;
        if (titleFont == INVALID_FONT)
        {
            titleFont = surface()->CreateFont();
            surface()->SetFontGlyphSet(titleFont, "Tahoma", 16, 700, 0, 0,
                ISurface::FONTFLAG_ANTIALIAS | ISurface::FONTFLAG_DROPSHADOW);
            sectionFont = surface()->CreateFont();
            surface()->SetFontGlyphSet(sectionFont, "Tahoma", 12, 700, 0, 0,
                ISurface::FONTFLAG_ANTIALIAS);
            textFont = surface()->CreateFont();
            surface()->SetFontGlyphSet(textFont, "Tahoma", 12, 500, 0, 0,
                ISurface::FONTFLAG_ANTIALIAS);
        }
        m_pTitle->SetFont(titleFont);
        m_pDesc->SetFont(textFont);
        m_pSection->SetFont(sectionFont);
        m_pTitle->SetFgColor(Color(255, 255, 255, 255));
        m_pDesc->SetFgColor(Color(230, 230, 230, 255));
        m_pSection->SetFgColor(Color(255, 255, 255, 255));
    }

    virtual void PaintBackground()
    {
        surface()->DrawSetColor(48, 48, 48, 245);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(145, 145, 145, 255);
        surface()->DrawOutlinedRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(72, 72, 72, 255);
        surface()->DrawFilledRect(0, 34, GetWide(), 35);
    }

    void ClearSettings()
    {
        for (int i = 0; i < m_Settings.Count(); ++i)
        {
            if (m_Settings[i].button)
            {
                m_Settings[i].button->SetVisible(false);
                m_Settings[i].button->MarkForDeletion();
            }
        }
        m_Settings.RemoveAll();
    }

    void AddSetting(const char* text, const char* command)
    {
        SettingEntry_t entry = {};
        char id[64], cmd[32];
        Q_snprintf(id, sizeof(id), "ToolSetting_%d", m_Settings.Count());
        Q_snprintf(cmd, sizeof(cmd), "setting_%d", m_Settings.Count());
        entry.button = new CSpawnFlatButton(this, id, text, this, cmd);
        Q_strncpy(entry.command, command ? command : "", sizeof(entry.command));
        m_Settings.AddToTail(entry);
    }

    void AddCommonToolButtons(const char* toolCommand)
    {
        if (toolCommand && toolCommand[0])
        {
            char cmd[192];
            Q_snprintf(cmd, sizeof(cmd), "gabe_spawn command \"%s\"", toolCommand);
            AddSetting("Select Tool", cmd);
        }
        AddSetting("Undo Last", "gabe_undo");
    }

    void ShowTool(const char* toolName, const char* toolCommand)
    {
        ClearSettings();
        Q_strncpy(m_szToolName, toolName ? toolName : "Tool Settings", sizeof(m_szToolName));
        Q_strncpy(m_szToolCommand, toolCommand ? toolCommand : "", sizeof(m_szToolCommand));
        m_pTitle->SetText(m_szToolName[0] ? m_szToolName : "Tool Settings");
        m_pSection->SetText("Settings");

        const char* desc = "Choose a tool on the left. Any settings for that tool appear here.";
        if (!Q_strnicmp(m_szToolCommand, "mtool_mode ", 11))
        {
            const char* mode = m_szToolCommand + 11;
            AddCommonToolButtons(m_szToolCommand);
            if (!Q_stricmp(mode, "remove"))
            {
                desc = "Remove deletes the entity you click. It does not use extra settings.";
            }
            else if (!Q_stricmp(mode, "distance"))
            {
                desc = "Distance measures how far away the target is. It does not use extra settings.";
            }
            else if (!Q_stricmp(mode, "color"))
            {
                desc = "Set the color preset used by the Color tool.";
                AddSetting("Red", "gabe_mtool_setting color red");
                AddSetting("Orange", "gabe_mtool_setting color orange");
                AddSetting("Yellow", "gabe_mtool_setting color yellow");
                AddSetting("Green", "gabe_mtool_setting color green");
                AddSetting("Blue", "gabe_mtool_setting color blue");
                AddSetting("Purple", "gabe_mtool_setting color purple");
                AddSetting("Pink", "gabe_mtool_setting color pink");
                AddSetting("White", "gabe_mtool_setting color white");
                AddSetting("Black", "gabe_mtool_setting color black");
                AddSetting("Rainbow", "gabe_mtool_setting color rainbow");
            }
            else if (!Q_stricmp(mode, "constraint"))
            {
                desc = "Choose which constraint type the Multitool will create.";
                AddSetting("Ball Socket", "gabe_mtool_setting constraint ballsocket");
                AddSetting("Weld", "gabe_mtool_setting constraint weld");
                AddSetting("Rope", "gabe_mtool_setting constraint rope");
                AddSetting("Slider", "gabe_mtool_setting constraint slider");
                AddSetting("Axis", "gabe_mtool_setting constraint hinge");
                AddSetting("Clear Constraints", "gabe_mtool_setting action clearconstraints");
            }
            else if (!Q_stricmp(mode, "ignite"))
            {
                desc = "Ignite sets the clicked entity on fire. No extra settings are needed.";
            }
            else if (!Q_stricmp(mode, "duplicate"))
            {
                desc = "Duplicate copies the targeted entity. No extra settings are needed here.";
            }
            else if (!Q_stricmp(mode, "explode"))
            {
                desc = "Explode detonates the selected entity. No extra settings are needed.";
            }
            else if (!Q_stricmp(mode, "pointmessage"))
            {
                desc = "Point Message creates map messages in the world.";
                AddSetting("Clear Point Messages", "gabe_mtool_setting action clearpointmessages");
            }
            else if (!Q_stricmp(mode, "light_watermelon"))
            {
                desc = "Choose the light color used when spawning the light watermelon.";
                AddSetting("Red", "gabe_mtool_setting lightcolor red");
                AddSetting("Green", "gabe_mtool_setting lightcolor green");
                AddSetting("Blue", "gabe_mtool_setting lightcolor blue");
                AddSetting("Yellow", "gabe_mtool_setting lightcolor yellow");
                AddSetting("Purple", "gabe_mtool_setting lightcolor purple");
                AddSetting("White", "gabe_mtool_setting lightcolor white");
                AddSetting("Rainbow", "gabe_mtool_setting lightcolor rainbow");
            }
            else if (!Q_stricmp(mode, "decal"))
            {
                desc = "Choose which decal the Decal tool places.";
                AddSetting("Blood", "gabe_mtool_setting decal blood");
                AddSetting("Scorch", "gabe_mtool_setting decal scorch");
                AddSetting("Concrete", "gabe_mtool_setting decal impact_concrete");
                AddSetting("Metal", "gabe_mtool_setting decal impact_metal");
                AddSetting("Glass", "gabe_mtool_setting decal impact_glass");
                AddSetting("Sand", "gabe_mtool_setting decal impact_sand");
                AddSetting("Yellow Blood", "gabe_mtool_setting decal yellowblood");
            }
            else if (!Q_stricmp(mode, "material"))
            {
                desc = "Choose the render effect used by the Render Effects tool.";
                AddSetting("None", "gabe_mtool_setting material none");
                AddSetting("Pulse Slow", "gabe_mtool_setting material pulse_slow");
                AddSetting("Pulse Fast", "gabe_mtool_setting material pulse_fast");
                AddSetting("Fade Slow", "gabe_mtool_setting material fade_slow");
                AddSetting("Fade Fast", "gabe_mtool_setting material fade_fast");
                AddSetting("Flicker Slow", "gabe_mtool_setting material flicker_slow");
                AddSetting("Flicker Fast", "gabe_mtool_setting material flicker_fast");
                AddSetting("Hologram", "gabe_mtool_setting material hologram");
                AddSetting("Glow Shell", "gabe_mtool_setting material glowshell");
            }
            else if (!Q_stricmp(mode, "faceposer"))
            {
                desc = "Choose the face preset applied by the Faceposer tool.";
                AddSetting("Smile", "gabe_mtool_setting face smile");
                AddSetting("Sad", "gabe_mtool_setting face sad");
                AddSetting("Angry", "gabe_mtool_setting face angry");
                AddSetting("Scared", "gabe_mtool_setting face scared");
                AddSetting("Surprised", "gabe_mtool_setting face surprised");
                AddSetting("Neutral", "gabe_mtool_setting face neutral");
            }
            else if (!Q_stricmp(mode, "balloon"))
            {
                desc = "Choose the lift force used by the Balloon tool.";
                AddSetting("100", "gabe_mtool_setting balloon 100");
                AddSetting("300", "gabe_mtool_setting balloon 300");
                AddSetting("500", "gabe_mtool_setting balloon 500");
                AddSetting("700", "gabe_mtool_setting balloon 700");
                AddSetting("1000", "gabe_mtool_setting balloon 1000");
            }
            else if (!Q_stricmp(mode, "thruster"))
            {
                desc = "Choose the power level used by the Thruster tool.";
                AddSetting("Low", "gabe_mtool_setting thruster low");
                AddSetting("Medium", "gabe_mtool_setting thruster medium");
                AddSetting("High", "gabe_mtool_setting thruster high");
            }
            else if (!Q_stricmp(mode, "elastic"))
            {
                desc = "Spring strength.";
                AddSetting("Strength: 0.5", "gabe_mtool_setting strength 0.5");
                AddSetting("Strength: 1", "gabe_mtool_setting strength 1");
                AddSetting("Strength: 2", "gabe_mtool_setting strength 2");
                AddSetting("Strength: 4", "gabe_mtool_setting strength 4");
            }
            else if (!Q_stricmp(mode, "hydraulic"))
            {
                desc = "Hydraulic strength.";
                AddSetting("Strength: 0.5", "gabe_mtool_setting strength 0.5");
                AddSetting("Strength: 1", "gabe_mtool_setting strength 1");
                AddSetting("Strength: 2", "gabe_mtool_setting strength 2");
                AddSetting("Strength: 4", "gabe_mtool_setting strength 4");
            }
            else if (!Q_stricmp(mode, "muscle"))
            {
                desc = "Muscle strength.";
                AddSetting("Strength: 0.5", "gabe_mtool_setting strength 0.5");
                AddSetting("Strength: 1", "gabe_mtool_setting strength 1");
                AddSetting("Strength: 2", "gabe_mtool_setting strength 2");
                AddSetting("Strength: 4", "gabe_mtool_setting strength 4");
            }
            else if (!Q_stricmp(mode, "winch"))
            {
                desc = "Winch strength.";
                AddSetting("Strength: 0.5", "gabe_mtool_setting strength 0.5");
                AddSetting("Strength: 1", "gabe_mtool_setting strength 1");
                AddSetting("Strength: 2", "gabe_mtool_setting strength 2");
                AddSetting("Strength: 4", "gabe_mtool_setting strength 4");
            }
            else if (!Q_stricmp(mode, "motor"))
            {
                desc = "Motor speed in degrees per second.";
                AddSetting("Motor speed: -360", "gabe_mtool_setting motor_speed -360");
                AddSetting("Motor speed: -180", "gabe_mtool_setting motor_speed -180");
                AddSetting("Motor speed: 90", "gabe_mtool_setting motor_speed 90");
                AddSetting("Motor speed: 180", "gabe_mtool_setting motor_speed 180");
                AddSetting("Motor speed: 360", "gabe_mtool_setting motor_speed 360");
                AddSetting("Motor speed: 720", "gabe_mtool_setting motor_speed 720");
            }
            else if (!Q_stricmp(mode, "buoyancy"))
            {
                desc = "Buoyancy strength.";
                AddSetting("Strength: 0.5", "gabe_mtool_setting strength 0.5");
                AddSetting("Strength: 1", "gabe_mtool_setting strength 1");
                AddSetting("Strength: 2", "gabe_mtool_setting strength 2");
                AddSetting("Strength: 4", "gabe_mtool_setting strength 4");
            }
            else if (!Q_stricmp(mode, "hoverball"))
            {
                desc = "Height stabilization strength.";
                AddSetting("Strength: 0.5", "gabe_mtool_setting strength 0.5");
                AddSetting("Strength: 1", "gabe_mtool_setting strength 1");
                AddSetting("Strength: 2", "gabe_mtool_setting strength 2");
                AddSetting("Strength: 4", "gabe_mtool_setting strength 4");
            }
            else if (!Q_stricmp(mode, "keepupright"))
            {
                desc = "Orientation stabilization strength.";
                AddSetting("Strength: 0.5", "gabe_mtool_setting strength 0.5");
                AddSetting("Strength: 1", "gabe_mtool_setting strength 1");
                AddSetting("Strength: 2", "gabe_mtool_setting strength 2");
                AddSetting("Strength: 4", "gabe_mtool_setting strength 4");
            }
            else if (!Q_stricmp(mode, "magnetise"))
            {
                desc = "Magnet radius in units.";
                AddSetting("Magnet radius: 64", "gabe_mtool_setting magnet_radius 64");
                AddSetting("Magnet radius: 128", "gabe_mtool_setting magnet_radius 128");
                AddSetting("Magnet radius: 256", "gabe_mtool_setting magnet_radius 256");
                AddSetting("Magnet radius: 512", "gabe_mtool_setting magnet_radius 512");
            }
            else if (!Q_stricmp(mode, "weight"))
            {
                desc = "Target mass in kilograms.";
                AddSetting("Weight: 1", "gabe_mtool_setting weight 1");
                AddSetting("Weight: 10", "gabe_mtool_setting weight 10");
                AddSetting("Weight: 50", "gabe_mtool_setting weight 50");
                AddSetting("Weight: 100", "gabe_mtool_setting weight 100");
                AddSetting("Weight: 500", "gabe_mtool_setting weight 500");
                AddSetting("Weight: 1000", "gabe_mtool_setting weight 1000");
                AddSetting("Weight: 5000", "gabe_mtool_setting weight 5000");
            }
            else if (!Q_stricmp(mode, "physicalproperties"))
            {
                desc = "Gravity and damping.";
                AddSetting("Damping: 0", "gabe_mtool_setting damping 0");
                AddSetting("Damping: 0.5", "gabe_mtool_setting damping 0.5");
                AddSetting("Damping: 1", "gabe_mtool_setting damping 1");
                AddSetting("Damping: 5", "gabe_mtool_setting damping 5");
                AddSetting("Damping: 10", "gabe_mtool_setting damping 10");
                AddSetting("Damping: 20", "gabe_mtool_setting damping 20");
                AddSetting("Gravity on", "gabe_mtool_setting gravity 1");
                AddSetting("Gravity off", "gabe_mtool_setting gravity 0");
            }
            else if (!Q_stricmp(mode, "stacker"))
            {
                desc = "Number of copies and spacing.";
                AddSetting("Stack count: 1", "gabe_mtool_setting stack_count 1");
                AddSetting("Stack count: 2", "gabe_mtool_setting stack_count 2");
                AddSetting("Stack count: 4", "gabe_mtool_setting stack_count 4");
                AddSetting("Stack count: 8", "gabe_mtool_setting stack_count 8");
                AddSetting("Stack count: 12", "gabe_mtool_setting stack_count 12");
                AddSetting("Gap: 2", "gabe_mtool_setting stack_gap 2");
                AddSetting("Gap: 8", "gabe_mtool_setting stack_gap 8");
                AddSetting("Gap: 16", "gabe_mtool_setting stack_gap 16");
                AddSetting("Gap: 32", "gabe_mtool_setting stack_gap 32");
            }
            else
            {
                desc = "This tool currently has no extra panel settings.";
            }
        }
        else if (!Q_strnicmp(m_szToolCommand, "gabe_glow_color ", 16))
        {
            desc = "This glow color command is ready to use. Click Select Tool to apply that color, then use Add Glow or Remove Glow.";
            AddCommonToolButtons(m_szToolCommand);
            AddSetting("Add Glow", "gabe_spawn command \"gabe_glowadd\"");
            AddSetting("Remove Glow", "gabe_spawn command \"gabe_glowremove\"");
        }
        else if (!Q_stricmp(m_szToolCommand, "gabe_glowadd") || !Q_stricmp(m_szToolCommand, "gabe_glowremove"))
        {
            desc = "Glow functions operate on the entity you are aiming at.";
            AddCommonToolButtons(m_szToolCommand);
        }
        else
        {
            AddCommonToolButtons(m_szToolCommand);
            desc = "This command does not expose extra settings in the panel.";
        }

        m_pDesc->SetText(desc);
        InvalidateLayout(true);
    }

    virtual void OnCommand(const char* command)
    {
        if (!Q_strnicmp(command, "setting_", 8))
        {
            int index = atoi(command + 8);
            if (index >= 0 && index < m_Settings.Count())
            {
                const char* exec = m_Settings[index].command;
                if (exec && exec[0])
                {
                    char full[160];
                    Q_snprintf(full, sizeof(full), "%s\n", exec);
                    engine->ClientCmd_Unrestricted(full);
                    surface()->PlaySound("ui/buttonclickrelease.wav");
                }
            }
            return;
        }
        BaseClass::OnCommand(command);
    }

    virtual void PerformLayout()
    {
        BaseClass::PerformLayout();
        const int pad = 8;
        const int w = GetWide();
        m_pTitle->SetBounds(pad, 8, w - pad * 2, 20);
        m_pDesc->SetBounds(pad, 40, w - pad * 2, 60);
        m_pSection->SetBounds(pad, 108, w - pad * 2, 18);
        int y = 132;
        int cols = (w >= 300) ? 2 : 1;
        int gap = 4;
        int bw = (w - pad * 2 - gap * (cols - 1)) / cols;
        int bh = 24;
        for (int i = 0; i < m_Settings.Count(); ++i)
        {
            int row = i / cols;
            int col = i % cols;
            int bx = pad + col * (bw + gap);
            int by = y + row * (bh + gap);
            m_Settings[i].button->SetBounds(bx, by, bw, bh);
        }
    }

private:
    Label* m_pTitle;
    Label* m_pDesc;
    Label* m_pSection;
    CUtlVector<SettingEntry_t> m_Settings;
    char m_szToolName[128];
    char m_szToolCommand[256];
};

// Tool headings are labels, not buttons; they never execute commands.
class CSpawnToolHeading : public Label
{
    DECLARE_CLASS_SIMPLE(CSpawnToolHeading, Label);
public:
    CSpawnToolHeading(Panel* parent, const char* name, const char* text)
        : BaseClass(parent, name, text), m_pOwner(parent)
    {
        SetContentAlignment(Label::a_west);
        SetTextInset(6, 0);
        SetPaintBackgroundEnabled(true);
        SetPaintBorderEnabled(false);
        SetKeyBoardInputEnabled(false);
        SetMouseInputEnabled(true);
    }
    virtual void ApplySchemeSettings(IScheme* scheme)
    {
        BaseClass::ApplySchemeSettings(scheme);
        static HFont font = INVALID_FONT;
        if (font == INVALID_FONT)
        {
            font = surface()->CreateFont();
            surface()->SetFontGlyphSet(font, "Tahoma", 12, 700, 0, 0,
                ISurface::FONTFLAG_ANTIALIAS);
        }
        if (font != INVALID_FONT) SetFont(font);
        SetFgColor(Color(255, 255, 255, 255));
        SetPaintBackgroundEnabled(true);
        SetPaintBorderEnabled(false);
    }
    virtual void PaintBackground()
    {
        surface()->DrawSetColor(82, 82, 82, 255);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(150, 150, 150, 255);
        surface()->DrawFilledRect(0, GetTall() - 2, GetWide(), GetTall());
    }
    virtual void OnMouseWheeled(int delta)
    {
        if (m_pOwner) m_pOwner->OnMouseWheeled(delta);
    }
private:
    Panel* m_pOwner;
};

enum EntryType_t { ENTRY_BUTTON, ENTRY_LABEL };
struct SpawnEntry_t
{
    EntryType_t type;
    CSpawnFlatButton* button;
    Label* label;
    char name[256];
    char value[256];
    int group;
    char spawnType[32];
};

// Per-tab content. ~Label entries begin a fresh column.
class CSpawnButtonColumns : public Panel
{
    DECLARE_CLASS_SIMPLE(CSpawnButtonColumns, Panel);
public:
    CSpawnButtonColumns(Panel* parent, const char* type, bool tools) : BaseClass(parent, "SpawnColumns")
    {
        Q_strncpy(m_szSpawnType, type, sizeof(m_szSpawnType)); m_szSearch[0] = 0;
        m_iGroup = -1; m_iScroll = 0; m_iCategoryScroll = 0; m_iButtonWide = 112; m_iButtonTall = 112; m_iColumnSpacing = 5;
        m_bTools = tools;
        m_iLastSelectedIndex = -1;
        m_bWeaponList = !Q_stricmp(type, "weapon");
        m_pWeaponName = new Label(this, "WeaponNameHeader", "Name");
        m_pWeaponAuthor = new Label(this, "WeaponAuthorHeader", "Author");
        m_pWeaponName->SetVisible(m_bWeaponList);
        m_pWeaponAuthor->SetVisible(m_bWeaponList);
        m_pCatPrev = new CSpawnFlatButton(this, "CatPrev", "^", this, "catprev");
        m_pCatNext = new CSpawnFlatButton(this, "CatNext", "v", this, "catnext");
        m_pPrev = new CSpawnFlatButton(this, "Prev", "<", this, "prev");
        m_pNext = new CSpawnFlatButton(this, "Next", ">", this, "next");
        m_pPage = new Label(this, "Page", "");
        SetPaintBackgroundEnabled(true);
    }
    bool IsTools() const { return m_bTools; }
    virtual void PaintBackground()
    {
        surface()->DrawSetColor(48, 48, 48, 245); surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        if (!m_bTools && !m_bWeaponList)
        {
            bool groups = false;
            for (int i = 0; i < m_Entries.Count(); ++i) if (m_Entries[i].type == ENTRY_LABEL && m_Entries[i].button) { groups = true; break; }
            if (groups) { int y = MIN(180, GetTall() / 3); surface()->DrawSetColor(145, 145, 145, 255); surface()->DrawFilledRect(0, y, GetWide(), y + 3); }
        }
        surface()->DrawSetColor(145, 145, 145, 255); surface()->DrawOutlinedRect(0, 0, GetWide(), GetTall());
    }
    void AddItem(const char* name, const char* value, const char* spawnType = NULL)
    {
        if (!name || !*name || !value) return;
        SpawnEntry_t e = {}; Q_strncpy(e.name, name, sizeof(e.name)); Q_strncpy(e.value, value, sizeof(e.value));
        Q_strncpy(e.spawnType, spawnType ? spawnType : m_szSpawnType, sizeof(e.spawnType));
        int i = m_Entries.Count(); char id[64], cmd[64]; Q_snprintf(id, sizeof(id), "item_%d", i);
        e.group = -1;
        for (int j = i - 1; j >= 0; --j) if (m_Entries[j].type == ENTRY_LABEL && (m_Entries[j].button || m_Entries[j].label)) { e.group = j; break; }
        if (name[0] == '~')
        {
            e.type = ENTRY_LABEL; Q_snprintf(cmd, sizeof(cmd), "group_%d", i);
            if (m_bTools)
                e.label = new CSpawnToolHeading(this, id, name + 1);
            else
            {
                e.button = new CSpawnFlatButton(this, id, name + 1, this, cmd);
                if (m_iGroup < 0) m_iGroup = i;
            }
        }
        else
        {
            e.type = ENTRY_BUTTON; Q_snprintf(cmd, sizeof(cmd), "spawn_%d", i);
            e.button = new CSpawnFlatButton(this, id, name, this, cmd);
            if (m_bWeaponList)
            {
                const bool custom = !Q_stricmp(value, "weapon_eattach") ||
                    !Q_stricmp(value, "weapon_stickylauncher") ||
                    !Q_stricmp(value, "weapon_lasergun");
                e.button->SetWeaponAuthor(custom ? "PmbrugGaming" : "Valve");
            }
            else if (!m_bTools) e.button->SetTile(value);
        }
        m_Entries.AddToTail(e); InvalidateLayout();
    }
    virtual void OnCommand(const char* cmd)
    {
        if (!Q_strnicmp(cmd, "wheel_", 6)) { OnMouseWheeled(atoi(cmd + 6)); return; }
        if (!Q_strnicmp(cmd, "group_", 6))
        {
            if (m_bTools) return;
            int group = atoi(cmd + 6); m_iGroup = group; m_iScroll = 0; ApplyFilter(); return;
        }
        if (!Q_stricmp(cmd, "catprev")) { m_iCategoryScroll = MAX(0, m_iCategoryScroll - 1); InvalidateLayout(true); return; }
        if (!Q_stricmp(cmd, "catnext")) { ++m_iCategoryScroll; InvalidateLayout(true); return; }
        if (!Q_stricmp(cmd, "prev")) { m_iScroll = MAX(0, m_iScroll - 1); InvalidateLayout(true); return; }
        if (!Q_stricmp(cmd, "next")) { ++m_iScroll; InvalidateLayout(true); return; }
        if (!Q_strnicmp(cmd, "spawn_", 6))
        {
            int index = atoi(cmd + 6);
            if (index >= 0 && index < m_Entries.Count() &&
                m_Entries[index].type == ENTRY_BUTTON)
            {
                char normalized[256];
                Q_strncpy(normalized, m_Entries[index].value, sizeof(normalized));
                for (char* c = normalized; *c; ++c) if (*c == '\\') *c = '/';
                const char* value = normalized;
                const char* spawnType = m_Entries[index].spawnType;
                // Only a safe, single quoted argument is accepted.
                // Server MUST independently validate type/value/permissions.
                if (strchr(value, '"') || strchr(value, '\\') ||
                    strchr(value, '\n') || strchr(value, '\r') ||
                    strchr(value, ';') || strchr(spawnType, '"') ||
                    strchr(spawnType, ';')) return;
                char command[512];
                Q_snprintf(command, sizeof(command), "gabe_spawn %s \"%s\"\n", spawnType, value);
                if (gabe_spawnmenu_debug.GetBool())
                    Msg("GabeSpawnMenu: sending %s", command);
                engine->ClientCmd_Unrestricted(command);
                surface()->PlaySound("ui/buttonclickrelease.wav");
                if (m_bTools)
                {
                    m_iLastSelectedIndex = index;
                    InvalidateLayout(true);
                    if (GetParent())
                        GetParent()->OnCommand("toolpick");
                }
            }
            return;
        }
        BaseClass::OnCommand(cmd);
    }

    virtual void OnMouseWheeled(int delta)
    {
        int x, y; input()->GetCursorPos(x, y); ScreenToLocal(x, y);
        bool hasGroups = false;
        for (int i = 0; i < m_Entries.Count(); ++i) if (m_Entries[i].type == ENTRY_LABEL && m_Entries[i].button) { hasGroups = true; break; }
        if (!m_bTools && !m_bWeaponList && hasGroups && y < MIN(180, GetTall() / 3)) m_iCategoryScroll = MAX(0, m_iCategoryScroll - delta);
        else m_iScroll = MAX(0, m_iScroll - delta);
        InvalidateLayout(true);
    }
    virtual void PerformLayout()
    {
        BaseClass::PerformLayout();
        int w = GetWide(), h = GetTall(); if (w < 20 || h < 40)return;
        int categoryH = (m_bTools || m_bWeaponList) ? 0 : MIN(180, h / 3);
        int toolW = w;
        int groups = 0, actualGroups = 0;
        for (int k = 0; k < m_Entries.Count(); ++k) if (m_Entries[k].type == ENTRY_LABEL && m_Entries[k].button)++actualGroups;
        int categoryRows = MAX(1, (categoryH - 8) / 18);
        m_iCategoryScroll = clamp(m_iCategoryScroll, 0, MAX(0, actualGroups - categoryRows));
        m_pCatPrev->SetVisible(!m_bTools && !m_bWeaponList && actualGroups > categoryRows);
        m_pCatNext->SetVisible(!m_bTools && !m_bWeaponList && actualGroups > categoryRows);
        m_pCatPrev->SetBounds(w - 23, 4, 19, 18);
        m_pCatNext->SetBounds(w - 23, categoryH - 22, 19, 18);
        for (int i = 0; i < m_Entries.Count(); ++i)
        {
            SpawnEntry_t& e = m_Entries[i];
            if (e.label) e.label->SetVisible(false);
            if (!e.button) continue;
            e.button->SetVisible(false);
            if (e.type == ENTRY_LABEL && !m_bTools && !m_bWeaponList)
            {
                int count = 0; for (int j = 0; j < m_Entries.Count(); ++j) if (m_Entries[j].type == ENTRY_BUTTON && m_Entries[j].button && m_Entries[j].group == i)++count;
                char text[300]; Q_snprintf(text, sizeof(text), "%s   (%d)", e.name + 1, count); e.button->SetText(text);
                int row = groups++ - m_iCategoryScroll;
                e.button->SetSelected(m_iGroup == i);
                if (row >= 0 && row < categoryRows)
                {
                    e.button->SetBounds(4, 4 + row * 18, w - (actualGroups > categoryRows ? 30 : 8), 17);
                    e.button->SetVisible(true);
                }
            }
        }
        int top = (!m_bTools && groups) ? categoryH + 4 : 4;
        if (m_bWeaponList)
        {
            int split = (w - 8) * 65 / 100;
            m_pWeaponName->SetBounds(9, 4, MAX(1, split - 5), 22);
            m_pWeaponAuthor->SetBounds(10 + split, 4, MAX(1, w - split - 14), 22);
            m_pWeaponName->SetFgColor(Color(255, 255, 255, 255));
            m_pWeaponAuthor->SetFgColor(Color(255, 255, 255, 255));
            top = 28;
        }
        int cellW = (m_bTools || m_bWeaponList) ? toolW - 8 : MAX(40, m_iButtonWide);
        int cellH = (m_bWeaponList || m_bTools) ? 24 : MAX(40, m_iButtonTall);
        int gap = (m_bTools || m_bWeaponList) ? 2 : MAX(1, m_iColumnSpacing), cols = MAX(1, (toolW - 8 + gap) / (cellW + gap));
        if (m_bTools || m_bWeaponList) cols = 1;
        int rows = MAX(1, (h - top - 28) / (cellH + gap)); int count = 0;
        for (int i = 0; i < m_Entries.Count(); ++i) if (Matches(m_Entries[i])) ++count;
        m_iScroll = clamp(m_iScroll, 0, MAX(0, (count + cols - 1) / cols - rows));
        int n = 0;
        for (int i = 0; i < m_Entries.Count(); ++i)
        {
            SpawnEntry_t& e = m_Entries[i]; if (!Matches(e))continue;
            int pos = n++ - m_iScroll * cols; if (pos < 0 || pos >= rows * cols)continue;
            int y = top + (pos / cols) * (cellH + gap);
            if (e.label)
            {
                e.label->SetBounds(4, y + 2, cellW, cellH - 2);
                e.label->SetVisible(true);
            }
            else if (e.button)
            {
                if (m_bTools) e.button->SetSelected(i == m_iLastSelectedIndex);
                int inset = m_bTools ? 8 : 0;
                e.button->SetBounds(4 + (pos % cols) * (cellW + gap) + inset,
                    y, MAX(1, cellW - inset), m_bTools ? 22 : cellH);
                e.button->SetVisible(true);
            }
        }
        m_pPrev->SetBounds(4, h - 23, 24, 19); m_pNext->SetBounds(toolW - 28, h - 23, 24, 19);
        m_pPage->SetBounds(32, h - 23, MAX(1, toolW - 64), 19);
        int itemCount = count;
        if (m_bTools)
            for (int i = 0; i < m_Entries.Count(); ++i)
                if (m_Entries[i].type == ENTRY_LABEL && Matches(m_Entries[i])) --itemCount;
        char text[80]; Q_snprintf(text, sizeof(text), "%d items", itemCount); m_pPage->SetText(text);
    }
    bool Matches(const SpawnEntry_t& e) const
    {
        if (e.type == ENTRY_LABEL) return m_bTools && e.label && !m_szSearch[0];
        if (!e.button) return false;
        if (m_szSearch[0])return Q_stristr(e.name, m_szSearch) || Q_stristr(e.value, m_szSearch);
        return m_bTools || m_bWeaponList || m_iGroup < 0 || e.group == m_iGroup;
    }
    bool GetLastSelected(char* outName, int outNameSize, char* outValue, int outValueSize) const
    {
        if (m_iLastSelectedIndex < 0 || m_iLastSelectedIndex >= m_Entries.Count())
            return false;
        const SpawnEntry_t& e = m_Entries[m_iLastSelectedIndex];
        if (e.type != ENTRY_BUTTON)
            return false;
        if (outName && outNameSize > 0) Q_strncpy(outName, e.name, outNameSize);
        if (outValue && outValueSize > 0) Q_strncpy(outValue, e.value, outValueSize);
        return true;
    }
    void ApplyFilter() { InvalidateLayout(true); }
    void SetSearchText(const char* s) { Q_strncpy(m_szSearch, s ? s : "", sizeof(m_szSearch)); m_iScroll = 0; ApplyFilter(); }
    void SetButtonSize(int w, int h) { m_iButtonWide = MAX(40, w); m_iButtonTall = MAX(40, h); InvalidateLayout(); }
    void SetColumnSpacing(int n) { m_iColumnSpacing = MAX(1, n); InvalidateLayout(); }
    bool RemoveItem(const char* name) { for (int i = 0; i < m_Entries.Count(); ++i)if (m_Entries[i].type == ENTRY_BUTTON && m_Entries[i].button && !Q_stricmp(m_Entries[i].name, name)) { m_Entries[i].button->SetVisible(false); m_Entries[i].button->MarkForDeletion(); m_Entries[i].button = NULL; InvalidateLayout(); return true; }return false; }
    void ClearItems()
    {
        for (int i = 0; i < m_Entries.Count(); ++i)
        {
            if (m_Entries[i].button) { m_Entries[i].button->SetVisible(false); m_Entries[i].button->MarkForDeletion(); }
            if (m_Entries[i].label) { m_Entries[i].label->SetVisible(false); m_Entries[i].label->MarkForDeletion(); }
        }
        m_Entries.RemoveAll(); m_iLastSelectedIndex = -1; m_iGroup = -1; m_iScroll = 0; m_iCategoryScroll = 0; InvalidateLayout();
    }
    const char* GetSpawnType() const { return m_szSpawnType; }
private:
    CUtlVector<SpawnEntry_t> m_Entries;
    char m_szSpawnType[32], m_szSearch[256];
    int m_iButtonWide, m_iButtonTall, m_iColumnSpacing, m_iGroup, m_iScroll, m_iCategoryScroll;
    bool m_bTools;
    bool m_bWeaponList;
    int m_iLastSelectedIndex;
    Label* m_pWeaponName;
    Label* m_pWeaponAuthor;
    CSpawnFlatButton* m_pPrev, * m_pNext, * m_pCatPrev, * m_pCatNext;
    Label* m_pPage;
};

class CGabeSpawnMenu : public Frame
{
    DECLARE_CLASS_SIMPLE(CGabeSpawnMenu, Frame);
public:
    struct SpawnTab_t
    {
        char name[128];
        CSpawnButtonColumns* panel;
        CSpawnFlatButton* button;
    };
    struct SpawnFavorite_t
    {
        char type[32], value[256];
    };

    CGabeSpawnMenu(VPANEL parent)
        : BaseClass(NULL, "GabeSpawnMenu")
    {
        SetParent(parent);
        SetTitle("GabeMod - Spawn Menu", true);
        SetMoveable(true);
        SetSize(SPAWNMENU_W, SPAWNMENU_H);
        SetCloseButtonVisible(true);
        SetMouseInputEnabled(true);
        SetKeyBoardInputEnabled(true);
        SetVisible(false);
        m_iActiveTab = -1;
        m_iSpawnCount = 0;
        m_szSearchText[0] = 0;
        m_iButtonWide = 112; m_iButtonTall = 112; m_iColumnSpacing = 5;
        m_iActiveTool = -1;
        m_pToolTitle = new Label(this, "ToolTitle", "Tools");
        m_pToolSettings = new CToolSettingsPanel(this);
        m_pToolSettings->SetVisible(false);
        m_pToolSettings->ShowTool("Tool Settings", "");
        SetTitleBarVisible(false);
        SetMoveable(false);
        SetProportional(false);
        m_Background = Color(68, 68, 68, 140);
        ResizeToScreen();
        LoadData();
    }

    void ScanMountedModels(
        CSpawnButtonColumns* panel,
        const char* directory,
        const char* pathID,
        const char* gameName,
        int depth = 0)
    {
        if (!panel || depth > 16)
            return;

        char wildcard[512];

        Q_snprintf(
            wildcard,
            sizeof(wildcard),
            "%s/*",
            directory
        );

        FileFindHandle_t handle;

        const char* file = filesystem->FindFirstEx(
            wildcard,
            pathID,
            &handle
        );

        if (!file)
            return;

        bool bCreatedCategory = false;

        while (file)
        {
            char path[512];

            Q_snprintf(
                path,
                sizeof(path),
                "%s/%s",
                directory,
                file
            );

            if (filesystem->FindIsDirectory(handle))
            {
                if (Q_stricmp(file, ".") &&
                    Q_stricmp(file, ".."))
                {
                    ScanMountedModels(
                        panel,
                        path,
                        pathID,
                        gameName,
                        depth + 1
                    );
                }
            }
            else
            {
                int len = Q_strlen(file);

                if (len > 4 &&
                    !Q_stricmp(file + len - 4, ".mdl"))
                {
                    // Prefer models with physics data.
                    char physicsPath[512];

                    Q_strncpy(
                        physicsPath,
                        path,
                        sizeof(physicsPath)
                    );

                    Q_strncpy(
                        physicsPath + Q_strlen(physicsPath) - 4,
                        ".phy",
                        5
                    );

                    if (filesystem->FileExists(
                        physicsPath, pathID))
                    {
                        if (!bCreatedCategory)
                        {
                            char category[256];

                            Q_snprintf(
                                category,
                                sizeof(category),
                                "~%s - %s",
                                gameName,
                                directory
                            );

                            panel->AddItem(
                                category, "", "prop");

                            bCreatedCategory = true;
                        }

                        char label[256];

                        Q_strncpy(
                            label,
                            file,
                            sizeof(label)
                        );

                        label[Q_strlen(label) - 4] = '\0';

                        panel->AddItem(
                            label,
                            path,
                            "prop"
                        );
                    }
                }
            }

            file = filesystem->FindNext(handle);
        }

        filesystem->FindClose(handle);
    }

    void LoadMountedGames()
    {
        CSpawnButtonColumns* panel = FindTab("Props");

        if (!panel)
            return;

        static const struct
        {
            const char* pathID;
            const char* name;
        } games[] =
        {
            { "HL2",       "Half-Life 2" },
            { "CSS",       "Counter-Strike Source" },
            { "HLS",       "Half-Life Source" },
            { "DODS",      "Day of Defeat Source" },
            { "LOSTCOAST", "Lost Coast" },
            { "HL1MP",     "Half-Life Deathmatch" },
            { "EP1",       "Episode One" },
            { "PORTAL",    "Portal" },
            { "EP2",       "Episode Two" },
            { "TF2",       "Team Fortress 2" }
        };

        for (int i = 0; i < ARRAYSIZE(games); ++i)
        {
            char paths[8192] = {};

            filesystem->GetSearchPath(
                games[i].pathID,
                true,
                paths,
                sizeof(paths)
            );

            if (!paths[0])
                continue;

            Msg(
                "[SpawnMenu] Loading mounted content: %s\n",
                games[i].name
            );

            ScanMountedModels(
                panel,
                "models",
                games[i].pathID,
                games[i].name
            );
        }

        panel->InvalidateLayout(true);
    }

    virtual void PaintBackground()
    {
        surface()->DrawSetColor(m_Background);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(205, 205, 205, 100);
        surface()->DrawOutlinedRect(0, 0, GetWide(), GetTall());
    }
    virtual void PerformLayout()
    {
        BaseClass::PerformLayout();
        int w = GetWide(), h = GetTall(), left = 8, top = 5, tabH = 23;
        int split = w * 57 / 100, bodyY = top + tabH + 3;
        int leftCount = 0, rightCount = 0;
        for (int i = 0; i < m_Tabs.Count(); ++i) if (m_Tabs[i].panel->IsTools())++rightCount; else ++leftCount;
        int lw = MAX(1, (split - 12) / MAX(1, leftCount));
        int rw = MAX(1, (w - split - 12) / MAX(1, rightCount));
        int toolAreaX = split + 4;
        int toolAreaW = w - split - 12;
        int toolListW = MAX(120, toolAreaW * 38 / 100);
        if (toolListW > toolAreaW - 80) toolListW = MAX(100, toolAreaW / 2);
        int toolSettingsX = toolAreaX + toolListW + 6;
        int toolSettingsW = MAX(1, toolAreaW - toolListW - 6);
        int li = 0, ri = 0;
        for (int i = 0; i < m_Tabs.Count(); ++i)
        {
            bool tool = m_Tabs[i].panel->IsTools();
            m_Tabs[i].button->SetBounds(tool ? split + 4 + ri++ * rw : left + li++ * lw, top, tool ? rw : lw, tabH);
            m_Tabs[i].panel->SetBounds(tool ? toolAreaX : left, bodyY, tool ? toolListW : split - 12, h - bodyY - 8);
        }
        m_pToolSettings->SetBounds(toolSettingsX, bodyY, toolSettingsW, h - bodyY - 8);
        m_pToolSettings->SetVisible(m_iActiveTool >= 0 && toolSettingsW > 80);
        m_pToolTitle->SetBounds(split + 4, bodyY, toolAreaW, 22);
        m_pToolTitle->SetVisible(rightCount == 0);
    }
    virtual void OnScreenSizeChanged(int oldWide, int oldTall)
    {
        BaseClass::OnScreenSizeChanged(oldWide, oldTall);
        ResizeToScreen();
    }
    virtual void OnClose()
    {
        Close();
    }
    virtual void OnKeyCodeTyped(KeyCode code)
    {
        if (code == KEY_ESCAPE) { Close(); return; }
        BaseClass::OnKeyCodeTyped(code);
    }
    virtual void OnCommand(const char* command)
    {
        if (!Q_strnicmp(command, "tab_", 4))
        {
            if (gabe_spawnmenu_debug.GetBool())
                Msg("GabeSpawnMenu: tab click: %s\n", command);
            SetActiveTab(atoi(command + 4));
            surface()->PlaySound("ui/buttonclickrelease.wav");
            return;
        }
        if (!Q_stricmp(command, "toolpick"))
        {
            UpdateToolSettingsFromActivePanel();
            return;
        }
        BaseClass::OnCommand(command);
    }

    void CenterMenu()
    {
        int sw, sh; surface()->GetScreenSize(sw, sh);
        SetPos((sw - GetWide()) / 2, (sh - GetTall()) / 2);
    }
    void ResizeToScreen()
    {
        int sw, sh; surface()->GetScreenSize(sw, sh);
        SetSize(MAX(1, sw - 40), MAX(1, sh - 40));
        CenterMenu(); InvalidateLayout(true);
    }
    void LoadData()
    {
        KeyValues* kv = new KeyValues("SpawnMenu");
        if (!kv->LoadFromFile(filesystem, "settings/spawnmenu.txt", "MOD"))
        {
            Warning("GabeSpawnMenu: failed to load settings/spawnmenu.txt\n");
            kv->deleteThis(); return;
        }
        // Fixed GMod-style tabs; configuration sections become categories.
        AddTab("Props", "prop");
        AddTab("NPCs", "command");
        AddTab("Weapons", "weapon");
        AddTab("Vehicles", "command");
        AddTab("Entities", "debug");
        AddTab("Tools", "command");

        for (KeyValues* section = kv->GetFirstTrueSubKey(); section;
            section = section->GetNextTrueSubKey())
        {
            const char* name = section->GetName();
            const char* type = section->GetString("_type", "");
            KeyValues* entries = section->FindKey("entries");
            if (!type[0] || !entries) continue;
            const char* tab = "Entities";
            if (!Q_stricmp(type, "prop") || !Q_stricmp(type, "ragdoll") ||
                (!Q_stricmp(type, "effect") && !Q_stricmp(name, "Special Effects"))) tab = "Props";
            else if (!Q_stricmp(name, "NPCs") || !Q_stricmp(type, "npc")) tab = "NPCs";
            else if (!Q_stricmp(type, "weapon")) tab = "Weapons";
            else if (!Q_stricmp(name, "Interactive")) tab = "Vehicles";
            else if (!Q_stricmp(name, "Glow")) tab = "Tools";
            else if (!Q_stricmp(type, "command")) tab = "Tools";
            CSpawnButtonColumns* panel = FindTab(tab);
            if (!panel) continue;
            char group[256];
            bool needGroup = true;
            for (KeyValues* item = entries->GetFirstValue(); item; item = item->GetNextValue())
            {
                const char* label = item->GetName();
                if (label[0] == '~')
                {
                    if (panel->IsTools())
                        Q_snprintf(group, sizeof(group), "~%s", label + 1);
                    else
                        Q_snprintf(group, sizeof(group), "~%s - %s", name, label + 1);
                    panel->AddItem(group, "", type);
                    needGroup = false;
                    continue;
                }
                if (needGroup)
                {
                    Q_snprintf(group, sizeof(group), "~%s", name);
                    panel->AddItem(group, "", type);
                    needGroup = false;
                }
                panel->AddItem(label, item->GetString(), type);
            }
        }
        kv->deleteThis();
        LoadMountedGames();
        if (m_iActiveTab < 0) for (int i = 0; i < m_Tabs.Count(); ++i) if (!m_Tabs[i].panel->IsTools()) { SetActiveTab(i); break; }
    }
    void Reload()
    {
        for (int i = 0; i < m_Tabs.Count(); ++i)
        {
            m_Tabs[i].panel->SetVisible(false); m_Tabs[i].panel->MarkForDeletion();
            m_Tabs[i].button->SetVisible(false); m_Tabs[i].button->MarkForDeletion();
        }
        m_Tabs.RemoveAll(); m_iActiveTab = -1; m_iActiveTool = -1;
        if (m_pToolSettings) m_pToolSettings->ShowTool("Tool Settings", "");
        LoadData(); InvalidateLayout(true);
    }

    void Open()
    {
        if (gabe_spawnmenu_debug.GetBool())
            Msg("GabeSpawnMenu: opening; loaded %d tabs\n", m_Tabs.Count());
        ResizeToScreen(); SetVisible(true); MakePopup(); MoveToFront();
        SetMouseInputEnabled(true); SetKeyBoardInputEnabled(true);
        g_bSpawnMenuOpen = true;
    }
    void Close()
    {
        SetVisible(false); SetMouseInputEnabled(false);
        SetKeyBoardInputEnabled(false); g_bSpawnMenuOpen = false;
    }
    void Toggle() { if (IsOpen()) Close(); else Open(); }
    bool IsOpen() { return g_bSpawnMenuOpen && IsVisible(); }

    virtual void OnThink()
    {
        BaseClass::OnThink();
        if (!IsVisible()) return;
    }
    void SetSearchText(const char* text)
    {
        Q_strncpy(m_szSearchText, text ? text : "", sizeof(m_szSearchText));
        for (int i = 0; i < m_Tabs.Count(); ++i)
            if (!m_Tabs[i].panel->IsTools()) m_Tabs[i].panel->SetSearchText(m_szSearchText);
    }
    void ClearSearch() { SetSearchText(""); }
    const char* GetSearchText() { return m_szSearchText; }
    void SetBackgroundColor(int r, int g, int b, int a)
    {
        m_Background = Color(clamp(r, 0, 255), clamp(g, 0, 255), clamp(b, 0, 255), clamp(a, 0, 255)); Repaint();
    }
    void SetButtonSize(int wide, int tall)
    {
        m_iButtonWide = wide; m_iButtonTall = tall;
        for (int i = 0; i < m_Tabs.Count(); ++i) m_Tabs[i].panel->SetButtonSize(wide, tall);
    }
    void SetColumnSpacing(int spacing)
    {
        m_iColumnSpacing = spacing;
        for (int i = 0; i < m_Tabs.Count(); ++i) m_Tabs[i].panel->SetColumnSpacing(spacing);
    }
    void AddFavorite(const char* type, const char* value)
    {
        if (!type || !*type || !value || !*value) return;
        for (int i = 0; i < m_Favorites.Count(); ++i)
            if (!Q_stricmp(m_Favorites[i].type, type) && !Q_stricmp(m_Favorites[i].value, value)) return;
        SpawnFavorite_t f = {};
        Q_strncpy(f.type, type, sizeof(f.type)); Q_strncpy(f.value, value, sizeof(f.value));
        m_Favorites.AddToTail(f);
    }
    void ClearFavorites() { m_Favorites.RemoveAll(); }
    void RecordSpawn(const char* type, const char* value)
    {
        if (!type || !value) return;
        ++m_iSpawnCount;
        SpawnFavorite_t f = {}; Q_strncpy(f.type, type, sizeof(f.type));
        Q_strncpy(f.value, value, sizeof(f.value)); m_History.AddToTail(f);
    }
    int GetSpawnCount() { return m_iSpawnCount; }
    void ClearHistory() { m_History.RemoveAll(); }
    void SetMenuTitle(const char* title) { if (title) SetTitle(title, true); }
    // The requested fixed size is maintained even through VScript calls.
    void SetMenuSize(int, int) { ResizeToScreen(); }
    void SetMenuPos(int x, int y) { SetPos(x, y); }
    int GetMenuWide() { return GetWide(); }
    int GetMenuTall() { return GetTall(); }
    int GetMenuX() { int x, y; GetPos(x, y); return x; }
    int GetMenuY() { int x, y; GetPos(x, y); return y; }
    void MakeFullscreen() { ResizeToScreen(); }
    void Center() { CenterMenu(); }
    void UpdateToolSettingsFromActivePanel()
    {
        if (!m_pToolSettings)
            return;
        if (m_iActiveTool < 0 || m_iActiveTool >= m_Tabs.Count())
        {
            m_pToolSettings->ShowTool("Tool Settings", "");
            return;
        }
        char name[128] = "";
        char value[256] = "";
        if (!m_Tabs[m_iActiveTool].panel->GetLastSelected(name, sizeof(name), value, sizeof(value)))
        {
            m_pToolSettings->ShowTool("Tool Settings", "");
            return;
        }
        m_pToolSettings->ShowTool(name, value);
    }
    void BringToFront() { MakePopup(); MoveToFront(); }
    int GetTabCount() { return m_Tabs.Count(); }
    void SetActiveTab(int index)
    {
        if (index < 0 || index >= m_Tabs.Count()) return;
        if (m_Tabs[index].panel->IsTools()) m_iActiveTool = index; else m_iActiveTab = index;
        for (int i = 0; i < m_Tabs.Count(); ++i)
        {
            bool active = i == m_iActiveTab || i == m_iActiveTool;
            m_Tabs[i].panel->SetVisible(active); m_Tabs[i].button->SetSelected(active);
        }
        UpdateToolSettingsFromActivePanel();
        InvalidateLayout(true);
    }
    int GetActiveTab() { return m_iActiveTab; }
    CSpawnButtonColumns* FindTab(const char* name)
    {
        if (!name) return NULL;
        for (int i = 0; i < m_Tabs.Count(); ++i)
            if (!Q_stricmp(m_Tabs[i].name, name)) return m_Tabs[i].panel;
        return NULL;
    }
    void AddTab(const char* name, const char* type)
    {
        if (!name || !*name || !type || !*type || FindTab(name) ||
            m_Tabs.Count() >= SPAWNMENU_MAX_TABS) return;
        SpawnTab_t tab = {};
        Q_strncpy(tab.name, name, sizeof(tab.name));
        char id[64], cmd[64];
        Q_snprintf(id, sizeof(id), "spawn_tab_%d", m_Tabs.Count());
        Q_snprintf(cmd, sizeof(cmd), "tab_%d", m_Tabs.Count());
        tab.panel = new CSpawnButtonColumns(this, type, !Q_stricmp(name, "Tools") || (!Q_stricmp(type, "command") && Q_stricmp(name, "NPCs") && Q_stricmp(name, "Vehicles") && Q_stricmp(name, "Entities")));
        tab.panel->SetButtonSize(m_iButtonWide, m_iButtonTall);
        tab.panel->SetColumnSpacing(m_iColumnSpacing);
        tab.panel->SetSearchText(m_szSearchText);
        tab.button = new CSpawnFlatButton(this, id, name, this, cmd, true);
        tab.panel->SetVisible(false);
        m_Tabs.AddToTail(tab);
        if (tab.panel->IsTools() ? m_iActiveTool < 0 : m_iActiveTab < 0) SetActiveTab(m_Tabs.Count() - 1);
        InvalidateLayout(true);
    }
    void RemoveTab(const char* name)
    {
        for (int i = 0; i < m_Tabs.Count(); ++i)
            if (!Q_stricmp(m_Tabs[i].name, name))
            {
                m_Tabs[i].button->SetVisible(false); m_Tabs[i].button->MarkForDeletion();
                m_Tabs[i].panel->SetVisible(false); m_Tabs[i].panel->MarkForDeletion();
                m_Tabs.Remove(i);
                // Rebuild tab commands (indices change).
                for (int j = 0; j < m_Tabs.Count(); ++j)
                {
                    char cmd[64]; Q_snprintf(cmd, sizeof(cmd), "tab_%d", j);
                    m_Tabs[j].button->SetSpawnCommand(cmd);
                }
                m_iActiveTab = -1; m_iActiveTool = -1;
                for (int k = 0; k < m_Tabs.Count(); ++k) if (m_Tabs[k].panel->IsTools() ? m_iActiveTool < 0 : m_iActiveTab < 0) SetActiveTab(k);
                InvalidateLayout(true);
                return;
            }
    }
    const char* GetTabName(int index)
    {
        return (index >= 0 && index < m_Tabs.Count()) ? m_Tabs[index].name : "";
    }
    void AddItem(const char* tab, const char* name, const char* value)
    {
        CSpawnButtonColumns* p = FindTab(tab); if (p)p->AddItem(name, value);
    }
    void RemoveItem(const char* tab, const char* name)
    {
        CSpawnButtonColumns* p = FindTab(tab); if (p)p->RemoveItem(name);
    }
    void ClearTab(const char* tab)
    {
        CSpawnButtonColumns* p = FindTab(tab); if (p)p->ClearItems();
    }
private:
    CUtlVector<SpawnTab_t> m_Tabs;
    CUtlVector<SpawnFavorite_t> m_Favorites, m_History;
    int m_iActiveTab, m_iSpawnCount, m_iActiveTool;
    Label* m_pToolTitle;
    CToolSettingsPanel* m_pToolSettings;
    int m_iButtonWide, m_iButtonTall, m_iColumnSpacing;
    char m_szSearchText[256];
    Color m_Background;
};

static CGabeSpawnMenu* g_pSpawnMenu = NULL;

// ============================================================
// Spawn menu VScript wrappers
// ============================================================

static CGabeSpawnMenu* Script_GetSpawnMenu()
{
    return g_pSpawnMenu;
}

static void Script_SpawnMenuOpen()
{
    VPANEL root = enginevgui->GetPanel(PANEL_INGAMESCREENS);

    if (!root)
        return;

    if (!g_pSpawnMenu)
        g_pSpawnMenu = new CGabeSpawnMenu(root);

    g_pSpawnMenu->Open();
}

static void Script_SpawnMenuClose()
{
    if (!g_pSpawnMenu)
        return;

    g_pSpawnMenu->Close();
}

static void Script_SpawnMenuToggle()
{
    if (!g_pSpawnMenu)
    {
        Script_SpawnMenuOpen();
        return;
    }

    g_pSpawnMenu->Toggle();
}

static bool Script_SpawnMenuIsOpen()
{
    return g_pSpawnMenu && g_pSpawnMenu->IsOpen();
}

static bool Script_SpawnMenuExists()
{
    return g_pSpawnMenu != NULL;
}

static void Script_SpawnMenuReload()
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->Reload();
}

static void Script_SpawnMenuSetTitle(const char* title)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetMenuTitle(title);
}

static void Script_SpawnMenuSetSize(int wide, int tall)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetMenuSize(wide, tall);
}

static void Script_SpawnMenuSetPos(int x, int y)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetMenuPos(x, y);
}

static int Script_SpawnMenuGetWide()
{
    if (!g_pSpawnMenu)
        return 0;

    return g_pSpawnMenu->GetMenuWide();
}

static int Script_SpawnMenuGetTall()
{
    if (!g_pSpawnMenu)
        return 0;

    return g_pSpawnMenu->GetMenuTall();
}

static int Script_SpawnMenuGetX()
{
    if (!g_pSpawnMenu)
        return 0;

    return g_pSpawnMenu->GetMenuX();
}

static int Script_SpawnMenuGetY()
{
    if (!g_pSpawnMenu)
        return 0;

    return g_pSpawnMenu->GetMenuY();
}

static void Script_SpawnMenuCenter()
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->Center();
}

static void Script_SpawnMenuFullscreen()
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->MakeFullscreen();
}

static void Script_SpawnMenuBringToFront()
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->BringToFront();
}

static int Script_SpawnMenuGetTabCount()
{
    if (!g_pSpawnMenu)
        return 0;

    return g_pSpawnMenu->GetTabCount();
}

static int Script_SpawnMenuGetActiveTab()
{
    if (!g_pSpawnMenu)
        return -1;

    return g_pSpawnMenu->GetActiveTab();
}

static void Script_SpawnMenuSetActiveTab(int index)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetActiveTab(index);
}

static void Script_SpawnMenuSpawn(
    const char* type,
    const char* value)
{
    if (!type || !*type)
        return;

    if (!value || !*value)
        return;

    engine->ClientCmd_Unrestricted(
        VarArgs(
            "gabe_spawn %s \"%s\"\n",
            type,
            value
        )
    );
}

static void Script_SpawnMenuPlayClickSound()
{
    surface()->PlaySound(
        "ui/buttonclickrelease.wav"
    );
}

static void Script_SpawnMenuPlaySound(const char* sound)
{
    if (!sound || !*sound)
        return;

    surface()->PlaySound(sound);
}

static void Script_SpawnMenuEnableMouse(bool enabled)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetMouseInputEnabled(enabled);
}

static void Script_SpawnMenuEnableKeyboard(bool enabled)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetKeyBoardInputEnabled(enabled);
}

static bool Script_SpawnMenuHasFocus()
{
    if (!g_pSpawnMenu)
        return false;

    VPANEL focus = input()->GetFocus();

    return focus == g_pSpawnMenu->GetVPanel();
}

static void Script_SpawnMenuSetSearchText(const char* text)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetSearchText(text);
}

static void Script_SpawnMenuClearSearch()
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->ClearSearch();
}

static const char* Script_SpawnMenuGetSearchText()
{
    return g_pSpawnMenu ? g_pSpawnMenu->GetSearchText() : "";
}

static void Script_SpawnMenuAddTab(
    const char* name,
    const char* type)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->AddTab(name, type);
}

static void Script_SpawnMenuRemoveTab(const char* name)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->RemoveTab(name);
}

static const char* Script_SpawnMenuGetTabName(int index)
{
    return g_pSpawnMenu ? g_pSpawnMenu->GetTabName(index) : "";
}

static void Script_SpawnMenuAddItem(
    const char* tab,
    const char* name,
    const char* value)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->AddItem(tab, name, value);
}

static void Script_SpawnMenuRemoveItem(
    const char* tab,
    const char* name)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->RemoveItem(tab, name);
}

static void Script_SpawnMenuClearTab(const char* tab)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->ClearTab(tab);
}

static void Script_SpawnMenuSetBackgroundColor(
    int r, int g, int b, int a)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetBackgroundColor(r, g, b, a);
}

static void Script_SpawnMenuSetButtonSize(int w, int h)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetButtonSize(w, h);
}

static void Script_SpawnMenuSetColumnSpacing(int spacing)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->SetColumnSpacing(spacing);
}

static void Script_SpawnMenuAddFavorite(
    const char* type,
    const char* value)
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->AddFavorite(type, value);
}

static void Script_SpawnMenuClearFavorites()
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->ClearFavorites();
}

static int Script_SpawnMenuGetSpawnCount()
{
    return g_pSpawnMenu ? g_pSpawnMenu->GetSpawnCount() : 0;
}

static void Script_SpawnMenuClearHistory()
{
    if (g_pSpawnMenu)
        g_pSpawnMenu->ClearHistory();
}

// ============================================================
// COMMAND
// ============================================================

CON_COMMAND(gabe_spawnmenu, "Toggle the spawn menu")
{
    VPANEL root = enginevgui->GetPanel(PANEL_INGAMESCREENS);
    if (!root) return;
    if (!g_pSpawnMenu) g_pSpawnMenu = new CGabeSpawnMenu(root);
    g_pSpawnMenu->Toggle();
}

CON_COMMAND(gabe_closespmenu, "Close the spawn menu")
{
    if (g_pSpawnMenu) g_pSpawnMenu->Close();
}

CON_COMMAND(gabe_reloadspawnmenu, "Reload settings/spawnmenu.txt")
{
    if (g_pSpawnMenu) g_pSpawnMenu->Reload();
}

void RegisterSpawnMenuScriptFuncs()
{
    if (!g_pScriptVM)
        return;

    g_pScriptVM->Run("CL_SpawnMenu <- {};\n");

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuOpen,
        "SpawnMenu_Open",
        "Opens the spawn menu."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuClose,
        "SpawnMenu_Close",
        "Closes the spawn menu."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuToggle,
        "SpawnMenu_Toggle",
        "Toggles the spawn menu."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuIsOpen,
        "SpawnMenu_IsOpen",
        "Returns whether the spawn menu is open."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuExists,
        "SpawnMenu_Exists",
        "Returns whether the spawn menu has been created."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuReload,
        "SpawnMenu_Reload",
        "Reloads the spawn menu."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuSetTitle,
        "SpawnMenu_SetTitle",
        "Sets the spawn menu title."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuSetSize,
        "SpawnMenu_SetSize",
        "Sets the spawn menu size."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuSetPos,
        "SpawnMenu_SetPos",
        "Sets the spawn menu position."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuGetWide,
        "SpawnMenu_GetWide",
        "Returns the menu width."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuGetTall,
        "SpawnMenu_GetTall",
        "Returns the menu height."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuGetX,
        "SpawnMenu_GetX",
        "Returns the menu X position."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuGetY,
        "SpawnMenu_GetY",
        "Returns the menu Y position."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuCenter,
        "SpawnMenu_Center",
        "Centers the spawn menu."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuFullscreen,
        "SpawnMenu_Fullscreen",
        "Makes the spawn menu fullscreen."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuBringToFront,
        "SpawnMenu_BringToFront",
        "Brings the spawn menu to the front."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuGetTabCount,
        "SpawnMenu_GetTabCount",
        "Returns the number of tabs."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuGetActiveTab,
        "SpawnMenu_GetActiveTab",
        "Returns the active tab index."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuSetActiveTab,
        "SpawnMenu_SetActiveTab",
        "Sets the active tab index."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuSpawn,
        "SpawnMenu_Spawn",
        "Spawns an item using gabe_spawn."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuPlayClickSound,
        "SpawnMenu_PlayClickSound",
        "Plays the spawn menu click sound."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuPlaySound,
        "SpawnMenu_PlaySound",
        "Plays a VGUI sound."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuEnableMouse,
        "SpawnMenu_EnableMouse",
        "Enables or disables mouse input."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuEnableKeyboard,
        "SpawnMenu_EnableKeyboard",
        "Enables or disables keyboard input."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuHasFocus,
        "SpawnMenu_HasFocus",
        "Returns whether the spawn menu has focus."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuSetSearchText,
        "SpawnMenu_SetSearchText",
        "Sets the spawn menu search text."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuClearSearch,
        "SpawnMenu_ClearSearch",
        "Clears the spawn menu search."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuAddTab,
        "SpawnMenu_AddTab",
        "Adds a spawn menu tab."
    );

    ScriptRegisterFunctionNamed(
        g_pScriptVM,
        Script_SpawnMenuAddItem,
        "SpawnMenu_AddItem",
        "Adds an item to a spawn menu tab."
    );

    // ============================================================
    // CL_SpawnMenu
    // ============================================================

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Open, Open);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Close, Close);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Toggle, Toggle);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_IsOpen, IsOpen);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Exists, Exists);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Reload, Reload);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetTitle, SetTitle);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetSize, SetSize);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetPos, SetPos);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetWide, GetWide);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetTall, GetTall);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetX, GetX);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetY, GetY);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Center, Center);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Fullscreen, Fullscreen);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_BringToFront, BringToFront);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetTabCount, GetTabCount);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetActiveTab, GetActiveTab);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetActiveTab, SetActiveTab);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_Spawn, Spawn);

    TABLE_FUNC_NAMED(
        CL_SpawnMenu,
        SpawnMenu_PlayClickSound,
        PlayClickSound
    );

    TABLE_FUNC_NAMED(
        CL_SpawnMenu,
        SpawnMenu_PlaySound,
        PlaySound
    );

    TABLE_FUNC_NAMED(
        CL_SpawnMenu,
        SpawnMenu_EnableMouse,
        EnableMouse
    );

    TABLE_FUNC_NAMED(
        CL_SpawnMenu,
        SpawnMenu_EnableKeyboard,
        EnableKeyboard
    );

    TABLE_FUNC_NAMED(
        CL_SpawnMenu,
        SpawnMenu_HasFocus,
        HasFocus
    );

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetSearchText, SetSearchText);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_ClearSearch, ClearSearch);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetSearchText, GetSearchText);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_AddTab, AddTab);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_RemoveTab, RemoveTab);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetTabName, GetTabName);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_AddItem, AddItem);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_RemoveItem, RemoveItem);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_ClearTab, ClearTab);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetBackgroundColor, SetBackgroundColor);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetButtonSize, SetButtonSize);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_SetColumnSpacing, SetColumnSpacing);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_AddFavorite, AddFavorite);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_ClearFavorites, ClearFavorites);

    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_GetSpawnCount, GetSpawnCount);
    TABLE_FUNC_NAMED(CL_SpawnMenu, SpawnMenu_ClearHistory, ClearHistory);
}