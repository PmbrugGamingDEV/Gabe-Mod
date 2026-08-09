#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui/iinput.h>

#include "filesystem.h"
#include "ienginevgui.h"

#include "vscript_shared.h"

#include "gabe_spawnmenu.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
extern IVEngineClient* engine;
#endif

using namespace vgui;

static bool g_bSpawnMenuOpen = false;

// ============================================================
// Button data (SAFE STORAGE)
// ============================================================

struct SpawnButton_t
{
	Button* button;
	char value[256];
};

// ============================================================
// COLUMN
// ============================================================

enum EntryType_t
{
	ENTRY_BUTTON,
	ENTRY_LABEL
};

struct SpawnEntry_t
{
	EntryType_t type;

	Button* button;
	Label* label;

	char value[256]; // only used for buttons
};

class CSpawnButtonColumns : public Panel
{
	DECLARE_CLASS_SIMPLE(CSpawnButtonColumns, Panel);

public:
	CSpawnButtonColumns(Panel* parent, const char* spawnType)
		: BaseClass(parent, "SpawnColumns")
	{
		Q_strncpy(m_szSpawnType, spawnType, sizeof(m_szSpawnType));

		SetPaintBackgroundEnabled(true);
		SetBgColor(Color(60, 60, 60, 220));
	}

	void AddItem(const char* name, const char* value)
	{
		SpawnEntry_t entry;

		if (name[0] == '~')
		{
			entry.type = ENTRY_LABEL;
			entry.label = new Label(this, name, name + 1);
			entry.label->SetFgColor(Color(255, 200, 100, 255));
		}
		else
		{
			entry.type = ENTRY_BUTTON;

			entry.button = new Button(this, name, name, this, "spawn");
			entry.button->SetContentAlignment(Label::a_west);

			Q_strncpy(entry.value, value, sizeof(entry.value));
		}

		m_Entries.AddToTail(entry);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);

		const int pad = 6;
		const int btnW = 140;
		const int btnH = 20;

		int x = pad;
		int y = pad;

		for (int i = 0; i < m_Entries.Count(); i++)
		{
			SpawnEntry_t& e = m_Entries[i];

			// 🔥 LABEL = FORCE NEW COLUMN
			if (e.type == ENTRY_LABEL)
			{
				y = pad;                // reset Y
				if (i != 0)             // don't shift first column
					x += btnW + pad;    // move to next column

				e.label->SetPos(x, y);
				e.label->SetSize(btnW, btnH);

				y += btnH + 4;
				continue;
			}

			// BUTTON
			if (y + btnH > h - 15) // 🔥 15px threshold from bottom
			{
				y = pad;
				x += btnW + pad;
			}

			e.button->SetPos(x, y);
			e.button->SetSize(btnW, btnH);

			y += btnH + 2;
		}
	}

	virtual void OnCommand(const char* cmd)
	{
		if (!Q_stricmp(cmd, "spawn"))
		{
#ifdef CLIENT_DLL
			VPANEL focused = input()->GetFocus();

			for (int i = 0; i < m_Entries.Count(); i++)
			{
				if (m_Entries[i].type != ENTRY_BUTTON)
					continue;

				if (m_Entries[i].button->GetVPanel() == focused)
				{
					const char* value = m_Entries[i].value;

					engine->ClientCmd_Unrestricted(
						VarArgs("gabe_spawn %s \"%s\"\n", m_szSpawnType, value)
					);

					surface()->PlaySound("ui/buttonclickrelease.wav");

					break;
				}
			}
#endif
			return;
		}

		BaseClass::OnCommand(cmd);
	}

private:
	CUtlVector<SpawnEntry_t> m_Entries;
	char m_szSpawnType[16];
};

// ============================================================
// MAIN MENU
// ============================================================

class CGabeSpawnMenu : public Frame
{
	DECLARE_CLASS_SIMPLE(CGabeSpawnMenu, Frame);

public:
	CGabeSpawnMenu(VPANEL parent)
		: BaseClass(NULL, "GabeSpawnMenu")
	{
		SetParent(parent);

		SetTitle("Spawn Menu", true);
		SetMoveable(true);
		SetCloseButtonVisible(true);
		SetMouseInputEnabled(true);
		SetKeyBoardInputEnabled(true);
		int sw, sh;
		surface()->GetScreenSize(sw, sh);
		SetSize(sw, sh);

		m_pTabs = new PropertySheet(this, "Tabs");

		LoadData();
		CenterMenu();
	}

	void ResizeToScreen()
	{
		int sw, sh;
		surface()->GetScreenSize(sw, sh);

		// FULLSCREEN
		SetPos(0, 0);
		SetSize(sw, sh);

		InvalidateLayout(true);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);

		m_pTabs->SetPos(10, 30);
		m_pTabs->SetSize(w, h);
	}

	virtual void OnScreenSizeChanged(int oldWide, int oldTall)
	{
		BaseClass::OnScreenSizeChanged(oldWide, oldTall);

		ResizeToScreen();
	}

	void LoadData()
	{
		KeyValues* kv = new KeyValues("SpawnMenu");

		if (!kv->LoadFromFile(filesystem, "settings/sm_populate.txt", "MOD"))
		{
			Warning("FAILED TO LOAD sm_populate.txt\n");
			kv->deleteThis();
			return;
		}

		for (KeyValues* section = kv->GetFirstSubKey();
			section;
			section = section->GetNextKey())
		{
			const char* tabName = section->GetName();
			const char* type = section->GetString("_type", "");

			KeyValues* entries = section->FindKey("entries");
			if (!entries || !type[0])
				continue;

			CSpawnButtonColumns* panel = new CSpawnButtonColumns(m_pTabs, type);
			m_pTabs->AddPage(panel, tabName);

			for (KeyValues* item = entries->GetFirstSubKey();
				item;
				item = item->GetNextKey())
			{
				panel->AddItem(item->GetName(), item->GetString());
			}
		}

		kv->deleteThis();
	}

	void CenterMenu()
	{
		int sw, sh;
		surface()->GetScreenSize(sw, sh);

		int w, h;
		GetSize(w, h);

		SetPos((sw - w) / 2, (sh - h) / 2);
	}

	// ============================================================
	// Script
	// ============================================================

	void Open()
	{
		SetVisible(true);
		MakePopup();
		MoveToFront();

		SetMouseInputEnabled(true);
		SetKeyBoardInputEnabled(true);

		g_bSpawnMenuOpen = true;
	}

	void Close()
	{
		SetVisible(false);

		SetMouseInputEnabled(false);
		SetKeyBoardInputEnabled(false);

		g_bSpawnMenuOpen = false;
	}

	void Toggle()
	{
		if (g_bSpawnMenuOpen)
			Close();
		else
			Open();
	}

	bool IsOpen()
	{
		return g_bSpawnMenuOpen && IsVisible();
	}

	void Reload()
	{
		if (!m_pTabs)
			return;

		m_pTabs->RemoveAllPages();

		LoadData();

		InvalidateLayout(true);
	}

	void SetMenuTitle(const char* title)
	{
		if (!title)
			return;

		SetTitle(title, true);
	}

	void SetMenuSize(int wide, int tall)
	{
		SetSize(wide, tall);
		InvalidateLayout(true);
	}

	void SetMenuPos(int x, int y)
	{
		SetPos(x, y);
	}

	int GetMenuWide()
	{
		int w, h;
		GetSize(w, h);

		return w;
	}

	int GetMenuTall()
	{
		int w, h;
		GetSize(w, h);

		return h;
	}

	int GetMenuX()
	{
		int x, y;
		GetPos(x, y);

		return x;
	}

	int GetMenuY()
	{
		int x, y;
		GetPos(x, y);

		return y;
	}

	void MakeFullscreen()
	{
		ResizeToScreen();
	}

	void Center()
	{
		CenterMenu();
	}

	void BringToFront()
	{
		MakePopup();
		MoveToFront();
	}

	int GetTabCount()
	{
		if (!m_pTabs)
			return 0;

		return m_pTabs->GetNumPages();
	}

	void SetActiveTab(int index)
	{
		if (!m_pTabs)
			return;

		if (index < 0 || index >= m_pTabs->GetNumPages())
			return;

		m_pTabs->SetActivePage(m_pTabs->GetPage(index));
	}

	int GetActiveTab()
	{
		if (!m_pTabs)
			return -1;

		Panel* active = m_pTabs->GetActivePage();

		if (!active)
			return -1;

		for (int i = 0; i < m_pTabs->GetNumPages(); i++)
		{
			if (m_pTabs->GetPage(i) == active)
				return i;
		}

		return -1;
	}

private:
	PropertySheet* m_pTabs;
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

// ============================================================
// COMMAND
// ============================================================

CON_COMMAND(gabe_spawnmenu, "Open spawn menu")
{
	VPANEL root = enginevgui->GetPanel(PANEL_INGAMESCREENS);
	if (!root)
		return;

	if (!g_pSpawnMenu)
		g_pSpawnMenu = new CGabeSpawnMenu(root);

	if (!g_bSpawnMenuOpen)
	{
		g_bSpawnMenuOpen = true;

		g_pSpawnMenu->SetVisible(true);
		g_pSpawnMenu->MakePopup();
		g_pSpawnMenu->MoveToFront();

		g_pSpawnMenu->SetMouseInputEnabled(true);
	}
}

CON_COMMAND(gabe_closespmenu, "Close spawn menu")
{
	if (g_pSpawnMenu && g_bSpawnMenuOpen)
	{
		g_bSpawnMenuOpen = false;

		g_pSpawnMenu->SetVisible(false);
		g_pSpawnMenu->SetMouseInputEnabled(false);
	}
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
}