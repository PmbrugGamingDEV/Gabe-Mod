// GabeMod spawn menu: Source SDK Base 2013 (client DLL)
// Place in game/client/hl2mp/ or another compiled client folder.
#include "cbase.h"
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
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
static ConVar gabe_spawnmenu_debug("gabe_spawnmenu_debug", "0", FCVAR_NONE, "Print spawn menu button and tab events.");

#define SPAWNMENU_W 1024
#define SPAWNMENU_H 768
#define SPAWNMENU_MAX_TABS 32

// Flat dark-tab / translucent button style from the reference image.
class CSpawnFlatButton : public Button
{
    DECLARE_CLASS_SIMPLE(CSpawnFlatButton, Button);
public:
    CSpawnFlatButton(Panel* parent, const char* name,
        const char* text, Panel* target, const char* command,
        bool tab = false)
        : BaseClass(parent, name, text), m_bTab(tab), m_bSelected(false), m_pTarget(target)
    {
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

    void SetSelected(bool selected) { m_bSelected = selected; Repaint(); }

    // Dispatch directly instead of depending on queued VGUI action signals.
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
        int r = 112, g = 112, b = 112, a = 135;
        if (m_bTab) { r = 35; g = 35; b = 35; a = 245; }
        if (m_bTab && m_bSelected) { r = 65; g = 106; b = 150; a = 245; }
        if (IsArmed()) { r = 80; g = 129; b = 181; a = 230; }
        if (IsDepressed()) { r = 47; g = 92; b = 144; a = 245; }
        surface()->DrawSetColor(r, g, b, a);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(195, 195, 195, 65);
        surface()->DrawOutlinedRect(0, 0, GetWide(), GetTall());
    }
private:
    bool m_bTab, m_bSelected;
    Panel* m_pTarget;
    char m_szCommand[64];

public:
    void SetSpawnCommand(const char* command)
    {
        Q_strncpy(m_szCommand, command ? command : "", sizeof(m_szCommand));
    }
};

enum EntryType_t { ENTRY_BUTTON, ENTRY_LABEL };
struct SpawnEntry_t
{
    EntryType_t type;
    CSpawnFlatButton* button;
    Label* label;
    char name[256];
    char value[256];
};

// Per-tab content. ~Label entries begin a fresh column.
class CSpawnButtonColumns : public Panel
{
    DECLARE_CLASS_SIMPLE(CSpawnButtonColumns, Panel);
public:
    CSpawnButtonColumns(Panel* parent, const char* spawnType)
        : BaseClass(parent, "SpawnColumns")
    {
        Q_strncpy(m_szSpawnType, spawnType ? spawnType : "", sizeof(m_szSpawnType));
        m_szSearch[0] = 0;
        m_iButtonWide = 150;
        m_iButtonTall = 23;
        m_iColumnSpacing = 5;
        SetPaintBackgroundEnabled(true);
        SetMouseInputEnabled(true);
    }
    virtual void PaintBackground()
    {
        surface()->DrawSetColor(85, 85, 85, 92);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
    }

    void AddItem(const char* name, const char* value)
    {
        if (!name || !*name || !value) return;
        SpawnEntry_t e = {};
        Q_strncpy(e.name, name, sizeof(e.name));
        Q_strncpy(e.value, value, sizeof(e.value));
        const int index = m_Entries.Count();
        char id[64]; Q_snprintf(id, sizeof(id), "sp_item_%d", index);
        if (name[0] == '~')
        {
            e.type = ENTRY_LABEL;
            e.label = new Label(this, id, name + 1);
            e.label->SetFgColor(Color(255, 220, 155, 255));
        }
        else
        {
            e.type = ENTRY_BUTTON;
            char cmd[64]; Q_snprintf(cmd, sizeof(cmd), "spawn_%d", index);
            e.button = new CSpawnFlatButton(this, id, name, this, cmd);
        }
        m_Entries.AddToTail(e);
        ApplyFilter();
    }

    virtual void OnCommand(const char* cmd)
    {
        if (!Q_strnicmp(cmd, "spawn_", 6))
        {
            int index = atoi(cmd + 6);
            if (index >= 0 && index < m_Entries.Count() &&
                m_Entries[index].type == ENTRY_BUTTON)
            {
                const char* value = m_Entries[index].value;
                // Only a safe, single quoted argument is accepted.
                // Server MUST independently validate type/value/permissions.
                if (strchr(value, '"') || strchr(value, '\\') ||
                    strchr(value, '\n') || strchr(value, '\r') ||
                    strchr(value, ';') || strchr(m_szSpawnType, '"') ||
                    strchr(m_szSpawnType, ';')) return;
                char command[512];
                Q_snprintf(command, sizeof(command), "gabe_spawn %s \"%s\"\n", m_szSpawnType, value);
                if (gabe_spawnmenu_debug.GetBool())
                    Msg("GabeSpawnMenu: sending %s", command);
                engine->ClientCmd_Unrestricted(command);
                surface()->PlaySound("ui/buttonclickrelease.wav");
            }
            return;
        }
        BaseClass::OnCommand(cmd);
    }

    virtual void PerformLayout()
    {
        BaseClass::PerformLayout();
        const int pad = 6, step = m_iButtonWide + m_iColumnSpacing;
        int x = pad, y = pad;
        bool columnHasItems = false;
        for (int i = 0; i < m_Entries.Count(); ++i)
        {
            SpawnEntry_t& e = m_Entries[i];
            Panel* p = (e.type == ENTRY_LABEL) ? (Panel*)e.label : (Panel*)e.button;
            if (!p || !p->IsVisible()) continue;
            if (e.type == ENTRY_LABEL && columnHasItems)
            {
                x += step; y = pad; columnHasItems = false;
            }
            if (y + m_iButtonTall > GetTall() - pad)
            {
                x += step; y = pad; columnHasItems = false;
            }
            p->SetBounds(x, y, m_iButtonWide, m_iButtonTall);
            y += m_iButtonTall + 2;
            columnHasItems = true;
        }
    }

    void ApplyFilter()
    {
        for (int i = 0; i < m_Entries.Count(); ++i)
        {
            SpawnEntry_t& e = m_Entries[i];
            if (e.type == ENTRY_LABEL)
            {
                if (e.label) e.label->SetVisible(!m_szSearch[0]);
            }
            else if (e.button) e.button->SetVisible(!m_szSearch[0] ||
                Q_stristr(e.name, m_szSearch) != NULL ||
                Q_stristr(e.value, m_szSearch) != NULL);
        }
        InvalidateLayout(true);
    }
    void SetSearchText(const char* s)
    {
        Q_strncpy(m_szSearch, s ? s : "", sizeof(m_szSearch)); ApplyFilter();
    }
    void SetButtonSize(int wide, int tall)
    {
        m_iButtonWide = MAX(40, wide); m_iButtonTall = MAX(16, tall);
        InvalidateLayout(true);
    }
    void SetColumnSpacing(int spacing)
    {
        m_iColumnSpacing = MAX(0, spacing); InvalidateLayout(true);
    }
    bool RemoveItem(const char* name)
    {
        for (int i = 0; i < m_Entries.Count(); ++i)
            if (m_Entries[i].type == ENTRY_BUTTON && !Q_stricmp(m_Entries[i].name, name))
            {
                m_Entries[i].button->MarkForDeletion();
                // Keep indices stable for button action commands.
                m_Entries[i].button = NULL;
                m_Entries[i].type = ENTRY_LABEL;
                m_Entries[i].label = NULL;
                m_Entries[i].name[0] = 0;
                InvalidateLayout(true);
                return true;
            }
        return false;
    }
    void ClearItems()
    {
        for (int i = 0; i < m_Entries.Count(); ++i)
        {
            if (m_Entries[i].button) m_Entries[i].button->MarkForDeletion();
            if (m_Entries[i].label) m_Entries[i].label->MarkForDeletion();
        }
        m_Entries.RemoveAll(); InvalidateLayout(true);
    }
    const char* GetSpawnType() const { return m_szSpawnType; }
private:
    CUtlVector<SpawnEntry_t> m_Entries;
    char m_szSpawnType[32], m_szSearch[256];
    int m_iButtonWide, m_iButtonTall, m_iColumnSpacing;
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
        m_iButtonWide = 150; m_iButtonTall = 23; m_iColumnSpacing = 5;
        m_Background = Color(68, 68, 68, 140);
        CenterMenu();
        LoadData();
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
        const int left = 8, top = 31, tabH = 24, bodyY = top + tabH + 3;
        const int tabW = MIN(155, MAX(75, (GetWide() - 16) / MAX(1, m_Tabs.Count())));
        for (int i = 0; i < m_Tabs.Count(); ++i)
        {
            m_Tabs[i].button->SetBounds(left + i * tabW, top, tabW, tabH);
            m_Tabs[i].panel->SetBounds(left, bodyY, GetWide() - 16, GetTall() - bodyY - 9);
        }
    }
    virtual void OnScreenSizeChanged(int oldWide, int oldTall)
    {
        BaseClass::OnScreenSizeChanged(oldWide, oldTall);
        CenterMenu();
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
        BaseClass::OnCommand(command);
    }

    void CenterMenu()
    {
        int sw, sh; surface()->GetScreenSize(sw, sh);
        SetPos((sw - GetWide()) / 2, (sh - GetTall()) / 2);
    }
    void ResizeToScreen()
    {
        SetSize(SPAWNMENU_W, SPAWNMENU_H);
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
        for (KeyValues* section = kv->GetFirstTrueSubKey(); section;
            section = section->GetNextTrueSubKey())
        {
            const char* tabName = section->GetName();
            const char* type = section->GetString("_type", "");
            KeyValues* entries = section->FindKey("entries");
            if (!type[0] || !entries) continue;
            AddTab(tabName, type);
            CSpawnButtonColumns* panel = FindTab(tabName);
            if (!panel) continue;
            for (KeyValues* item = entries->GetFirstValue(); item; item = item->GetNextValue())
                panel->AddItem(item->GetName(), item->GetString());
        }
        kv->deleteThis();
        if (m_iActiveTab < 0 && m_Tabs.Count()) SetActiveTab(0);
    }
    void Reload()
    {
        for (int i = 0; i < m_Tabs.Count(); ++i)
        {
            m_Tabs[i].panel->MarkForDeletion();
            m_Tabs[i].button->MarkForDeletion();
        }
        m_Tabs.RemoveAll(); m_iActiveTab = -1;
        LoadData(); InvalidateLayout(true);
    }

    void Open()
    {
        if (gabe_spawnmenu_debug.GetBool())
            Msg("GabeSpawnMenu: opening; loaded %d tabs\n", m_Tabs.Count());
        CenterMenu(); SetVisible(true); MakePopup(); MoveToFront();
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

    void SetSearchText(const char* text)
    {
        Q_strncpy(m_szSearchText, text ? text : "", sizeof(m_szSearchText));
        for (int i = 0; i < m_Tabs.Count(); ++i)
            m_Tabs[i].panel->SetSearchText(m_szSearchText);
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
    void BringToFront() { MakePopup(); MoveToFront(); }
    int GetTabCount() { return m_Tabs.Count(); }
    void SetActiveTab(int index)
    {
        if (index < 0 || index >= m_Tabs.Count()) return;
        m_iActiveTab = index;
        for (int i = 0; i < m_Tabs.Count(); ++i)
        {
            m_Tabs[i].panel->SetVisible(i == index);
            m_Tabs[i].button->SetSelected(i == index);
        }
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
        tab.panel = new CSpawnButtonColumns(this, type);
        tab.panel->SetButtonSize(m_iButtonWide, m_iButtonTall);
        tab.panel->SetColumnSpacing(m_iColumnSpacing);
        tab.panel->SetSearchText(m_szSearchText);
        tab.button = new CSpawnFlatButton(this, id, name, this, cmd, true);
        tab.panel->SetVisible(false);
        m_Tabs.AddToTail(tab);
        if (m_iActiveTab < 0) SetActiveTab(0);
        InvalidateLayout(true);
    }
    void RemoveTab(const char* name)
    {
        for (int i = 0; i < m_Tabs.Count(); ++i)
            if (!Q_stricmp(m_Tabs[i].name, name))
            {
                m_Tabs[i].button->MarkForDeletion();
                m_Tabs[i].panel->MarkForDeletion();
                m_Tabs.Remove(i);
                // Rebuild tab commands (indices change).
                for (int j = 0; j < m_Tabs.Count(); ++j)
                {
                    char cmd[64]; Q_snprintf(cmd, sizeof(cmd), "tab_%d", j);
                    m_Tabs[j].button->SetSpawnCommand(cmd);
                }
                m_iActiveTab = -1;
                if (m_Tabs.Count()) SetActiveTab(0);
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
    int m_iActiveTab, m_iSpawnCount;
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