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

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
extern IVEngineClient* engine;
#endif

using namespace vgui;

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

private:
	PropertySheet* m_pTabs;
};

// ============================================================
// COMMAND
// ============================================================

static CGabeSpawnMenu* g_pSpawnMenu = NULL;

static bool g_bSpawnMenuOpen = false;

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