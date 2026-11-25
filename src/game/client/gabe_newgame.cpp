// gabe_newgamedialog.cpp
// A new game creator panel (Select Map + Settings tabs)

#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Slider.h>
#include "vgui_controls/imagelist.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/HTML.h>
#include <time.h>

#include "vscript_shared.h"

#include "gabe_newgame.h"

#include "filesystem.h"
#include "ienginevgui.h"
#include "vgui/IInput.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
extern IVEngineClient* engine;
#endif

using namespace vgui;

ConVar gabeplus_newgame_useampmclock("gabe_newgame_useampmclock", "1", FCVAR_ARCHIVE, "If set to 1, use the 12-hour clock (AM/PM)");

// GabeMod visual theme. Customize these values in one place.
static const Color NG_BG(24, 24, 24, 255);
static const Color NG_PANEL(35, 35, 35, 255);
static const Color NG_ACCENT(186, 186, 186, 255);
static const Color NG_TEXT(235, 235, 235, 255);
static const Color NG_MUTED(162, 162, 162, 255);

static void NG_FillRect(int x, int y, int w, int h, const Color& col)
{
	surface()->DrawSetColor(col);
	surface()->DrawFilledRect(x, y, x + w, y + h);
}

static void NG_OutlineRect(int x, int y, int w, int h, const Color& col)
{
	surface()->DrawSetColor(col);
	surface()->DrawOutlinedRect(x, y, x + w, y + h);
}

static void NG_StyleLabel(Label* pLabel, bool muted = false)
{
	if (pLabel)
		pLabel->SetFgColor(muted ? NG_MUTED : NG_TEXT);
}

// Custom-painted VGUI button: no external material or scheme required.
class CNewGameButton : public Button
{
	DECLARE_CLASS_SIMPLE(CNewGameButton, Button);
public:
	CNewGameButton(Panel* parent, const char* name, const char* text,
		Panel* target, const char* command, bool primary)
		: BaseClass(parent, name, text, target, command), m_bPrimary(primary)
	{
		SetPaintBorderEnabled(false);
		SetContentAlignment(Label::a_center);
		SetArmedSound("ui/buttonrollover.wav");
		SetDepressedSound("ui/buttonclick.wav");
		SetDefaultColor(NG_TEXT, NG_TEXT);
		SetArmedColor(Color(255, 255, 255, 255), Color(255, 255, 255, 255));
	}

	void PaintBackground()
	{
		Color bg = m_bPrimary ? Color(86, 86, 86, 255) : Color(47, 47, 47, 255);
		if (IsSelected()) bg = Color(88, 88, 88, 255);
		if (IsArmed()) bg = m_bPrimary ? Color(115, 115, 115, 255) : Color(72, 72, 72, 255);
		if (IsDepressed()) bg = Color(48, 48, 48, 255);
		NG_FillRect(0, 0, GetWide(), GetTall(), bg);
		NG_OutlineRect(0, 0, GetWide(), GetTall(),
			m_bPrimary ? NG_ACCENT : Color(92, 92, 92, 255));
	}
private:
	bool m_bPrimary;
};

class CGamePadUIMapTab;

static void StripExtensionInPlace(char* p)
{
	int len = Q_strlen(p);
	for (int i = len - 1; i >= 0; --i)
	{
		if (p[i] == '.')
		{
			p[i] = 0;
			return;
		}
		if (p[i] == '/' || p[i] == '\\')
			return;
	}
}

static bool IsMapNameAllowed(const char* map)
{
	if (!map || !map[0])
		return false;

	// Keep it simple/safe: map name only, no spaces, no semicolons, no quotes.
	for (const char* c = map; *c; ++c)
	{
		if (*c == ' ' || *c == ';' || *c == '"' || *c == '\n' || *c == '\r' || *c == '/' || *c == '\\')
			return false;
	}
	return true;
}

static const char* GetMapLastPlayed(const char* map)
{
	static char out[64];
	out[0] = 0;

	KeyValues* kv = new KeyValues("MapHistory");
	if (kv->LoadFromFile(g_pFullFileSystem, "cfg/gabeplus_map_history.txt", "MOD"))
	{
		const char* val = kv->GetString(map, "");
		if (val && val[0])
		{
			Q_strncpy(out, val, sizeof(out));
			kv->deleteThis();
			return out;
		}
	}

	kv->deleteThis();
	return "Never";
}

// ------------------------------------------------------------
// Particle Simulator tab
// ------------------------------------------------------------
class CGamepadUIParticleSimTab : public Panel
{
	DECLARE_CLASS_SIMPLE(CGamepadUIParticleSimTab, Panel);

public:
	CGamepadUIParticleSimTab(Panel* parent)
		: BaseClass(parent, "GamepadUIParticleSimTab")
	{
		m_pHTML = new HTML(this, "HTMLParticleSim", true, false);

		m_pHTML->SetVisible(true);
		m_pHTML->SetContextMenuEnabled(false);
		m_pHTML->SetScrollbarsEnabled(false);

		// Put the file here:
		// modfolder/resource/html/particlesim.html
		m_pHTML->OpenURL("resource/html/particlesim.html", NULL);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);
		m_pHTML->SetBounds(0, 0, w, h);
	}

private:
	HTML* m_pHTML;
};

// ------------------------------------------------------------
// Map selection tab
// ------------------------------------------------------------
// A row of thumbnail buttons; PanelListPanel supplies vertical scrolling.
class CGabeMapTileRow : public Panel
{
	DECLARE_CLASS_SIMPLE(CGabeMapTileRow, Panel);
public:
	CGabeMapTileRow(Panel* parent) : BaseClass(parent, "MapTileRow"), m_Count(0)
	{
		SetTall(126);
		SetPaintBackgroundEnabled(false);
	}
	void AddMap(Panel* target, const char* map, const char* image, int index)
	{
		if (m_Count >= 3) return;
		char cmd[64]; Q_snprintf(cmd, sizeof(cmd), "SelectTile %d", index);
		Button* b = new CNewGameButton(this, map, map, target, cmd, false);
		b->SetContentAlignment(Label::a_south);
		b->SetArmedSound("ui/buttonrollover.wav");
		b->SetDepressedSound("ui/buttonclick.wav");
		ImagePanel* img = new ImagePanel(b, "Thumbnail");
		img->SetImage(image);
		img->SetShouldScaleImage(true);
		img->SetMouseInputEnabled(false);
		img->SetKeyBoardInputEnabled(false);
		m_Indices[m_Count] = index;
		m_Buttons[m_Count] = b;
		m_Images[m_Count] = img;
		++m_Count;
		InvalidateLayout();
	}
	void UpdateSelection(int index)
	{
		for (int i = 0; i < m_Count; ++i)
			m_Buttons[i]->SetSelected(m_Indices[i] == index);
	}
	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		int cell = MAX(1, (GetWide() - 12) / 3);
		for (int i = 0; i < m_Count; ++i)
		{
			m_Buttons[i]->SetBounds(3 + i * (cell + 3), 3, cell, GetTall() - 6);
			m_Images[i]->SetBounds(4, 4, MAX(1, cell - 8), MAX(1, GetTall() - 35));
		}
	}
private:
	int m_Count;
	int m_Indices[3];
	Button* m_Buttons[3];
	ImagePanel* m_Images[3];
};

class CGamepadUIMapTab : public Panel
{
	DECLARE_CLASS_SIMPLE(CGamepadUIMapTab, Panel);

public:
	CGamepadUIMapTab(Panel* parent)
		: BaseClass(parent, "GamepadUIMapTab")
	{
		m_pMapSheet = new PropertySheet(this, "MapViews");
		m_pTiles = new PanelListPanel(m_pMapSheet, "GameMaps");
		m_pTiles->SetFirstColumnWidth(0);
		m_pMapList = new ListPanel(m_pMapSheet, "MapList");
		m_pMapSheet->AddPage(m_pTiles, "Game");
		m_pMapSheet->AddPage(m_pMapList, "Browse All");

		// Create image list for thumbnails/icons
		m_pImageList = new ImageList(false);
		m_pMapList->SetImageList(m_pImageList, false);

		m_pMapList->AddColumnHeader(0, "map", "Map Name", 150, 60, 1200, ListPanel::COLUMN_RESIZEWITHWINDOW);
		m_pMapList->AddColumnHeader(1, "category", "Category", 100, 50, 300, ListPanel::COLUMN_RESIZEWITHWINDOW);


		m_pMapList->SetRowHeight(24);

		m_pMapList->SetMultiselectEnabled(false);
		m_pMapList->SetEmptyListText("No maps found.");

		m_pMapList->AddActionSignalTarget(this);

		m_pFilter = new TextEntry(this, "Filter");
		m_pFilterLabel = new Label(this, "FilterLabel", "SEARCH MAPS");
		m_pMapSummary = new Label(this, "MapSummary", "Select a map to view its details");
		m_pMapSummary->SetContentAlignment(Label::a_west);
		NG_StyleLabel(m_pFilterLabel, true);
		NG_StyleLabel(m_pMapSummary, false);
		m_pMapList->SetBgColor(NG_PANEL);
		m_pMapList->SetFgColor(NG_TEXT);
		m_pFilter->SetBgColor(Color(47, 47, 47, 255));
		m_pFilter->SetFgColor(NG_TEXT);

		m_szLastFilter[0] = 0;
		m_szSelectedMap[0] = 0;
		m_pHistory = new Label(this, "History", "Last played: -");
		NG_StyleLabel(m_pHistory, true);

		LoadMaps();
		ApplyFilter();
	}

	virtual void OnCommand(const char* command)
	{
		if (!Q_strnicmp(command, "SelectTile ", 11))
		{
			int index = Q_atoi(command + 11);
			if (m_Maps.IsValidIndex(index))
			{
				for (int row = 0; row < m_pMapList->GetItemCount(); ++row)
				{
					int id = m_pMapList->GetItemIDFromRow(row);
					KeyValues* kv = m_pMapList->GetItem(id);
					if (kv && !Q_stricmp(kv->GetString("map"), m_Maps[index].String()))
					{
						m_pMapList->SetSingleSelectedItem(id);
						break;
					}
				}
			}
			return;
		}
		BaseClass::OnCommand(command);
	}

	virtual ~CGamepadUIMapTab()
	{
		// 2007-safe cleanup
		if (m_pImageList)
		{
			m_pMapList->SetImageList(NULL, false);
			delete m_pImageList;
			m_pImageList = NULL;
		}
	}

	virtual void OnMessage(KeyValues* params, VPANEL fromPanel)
	{
		Msg("Signal received: %s\n", params->GetName());

		if (!Q_stricmp(params->GetName(), "OpenContextMenu"))
		{
			int itemID = params->GetInt("itemID", -1);

			if (itemID >= 0)
			{
				KeyValues* kv = m_pMapList->GetItem(itemID);
				if (kv)
				{
					const char* map = kv->GetString("map", "");
					if (map && map[0])
					{
						ShowMapInfo(map);
					}
				}
			}

			return; // swallow the message
		}

		BaseClass::OnMessage(params, fromPanel);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		int w, h; GetSize(w, h);
		m_pFilterLabel->SetBounds(10, 8, w - 20, 18);
		m_pFilter->SetBounds(10, 30, w - 20, 26);
		m_pMapSheet->SetBounds(10, 64, w - 20, MAX(1, h - 126));
		m_pMapSummary->SetBounds(16, h - 58, w - 32, 24);
		m_pHistory->SetBounds(16, h - 32, w - 32, 22);
	}

	void PaintBackground()
	{
		NG_FillRect(0, 0, GetWide(), GetTall(), NG_PANEL);
		NG_FillRect(8, GetTall() - 60, GetWide() - 16, 54, NG_BG);
		NG_FillRect(8, GetTall() - 60, 3, 54, NG_ACCENT);
	}

	void ShowMapInfo(const char* map)
	{
		char path[256];
		Q_snprintf(path, sizeof(path), "maps/%s.txt", map);

		char author[128] = "Unknown";
		char description[512] = "No description provided.";

		KeyValues* kv = new KeyValues(map);

		if (kv->LoadFromFile(g_pFullFileSystem, path, "GAME"))
		{
			KeyValues* sub = kv->FindKey(map);
			if (sub)
			{
				const char* a = sub->GetString("author", "");
				const char* d = sub->GetString("description", "");

				if (a[0])
					Q_strncpy(author, a, sizeof(author));

				if (d[0])
					Q_strncpy(description, d, sizeof(description));
			}
		}

		kv->deleteThis();

		char message[768];
		Q_snprintf(message, sizeof(message),
			"Map: %s\n\nAuthor: %s\n\nDescription:\n%s",
			map,
			author,
			description
		);

		MessageBox* box = new MessageBox("Map Information", message, this);
		box->DoModal();
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();

		char text[256];
		m_pFilter->GetText(text, sizeof(text));

		if (Q_stricmp(text, m_szLastFilter) != 0)
		{
			Q_strncpy(m_szLastFilter, text, sizeof(m_szLastFilter));
			ApplyFilter();
		}

		const char* selected = GetSelectedMap();
		char details[512];
		if (selected && selected[0])
			Q_snprintf(details, sizeof(details), "%s  |  %s",
				selected, GetMapCategory(selected));
		else
			Q_snprintf(details, sizeof(details), "NO MAP SELECTED     |     %d maps available",
				m_pMapList->GetItemCount());
		m_pMapSummary->SetText(details);
		if (Q_stricmp(m_szSelectedMap, selected))
		{
			Q_strncpy(m_szSelectedMap, selected, sizeof(m_szSelectedMap));
			int selectedIndex = -1;
			for (int i = 0; i < m_Maps.Count(); ++i)
				if (!Q_stricmp(m_Maps[i].String(), selected)) { selectedIndex = i; break; }
			for (int i = 0; i < m_TileRows.Count(); ++i)
				m_TileRows[i]->UpdateSelection(selectedIndex);
			char history[160];
			Q_snprintf(history, sizeof(history), "Last played: %s", selected[0] ? GetMapLastPlayed(selected) : "-");
			m_pHistory->SetText(history);
		}
	}

	void OnMouseDoublePressed(int itemID)
	{
		if (itemID == -1 || !GetSelectedMap()[0])
			return;

		PostMessage(
			GetParent(),
			new KeyValues("Command", "command", "StartServer")
		);
	}

	const char* GetSelectedMap() const
	{
		if (m_pMapList->GetSelectedItemsCount() <= 0)
			return "";
		int itemID = m_pMapList->GetSelectedItem(0);
		if (itemID == -1)
			return "";

		KeyValues* kv = m_pMapList->GetItem(itemID);
		if (!kv)
			return "";

		return kv->GetString("map", "");
	}

	void Refresh()
	{
		LoadMaps();
		ApplyFilter();
	}

private:
	const char* GetMapCategory(const char* map) const
	{
		if (!map || !map[0])
			return "Misc";

		if (!Q_strnicmp(map, "dm_", 3) ||
			!Q_strnicmp(map, "ffa_", 4) ||
			!Q_strnicmp(map, "arena_", 6))
			return "Deathmatch";

		if (!Q_strnicmp(map, "ctf_", 4))
			return "Capture The Flag";

		if (!Q_strnicmp(map, "tdm_", 4))
			return "Team Deathmatch";

		if (!Q_strnicmp(map, "koth_", 5))
			return "King of the Hill";

		if (!Q_strnicmp(map, "payload_", 8) ||
			!Q_strnicmp(map, "pl_", 3))
			return "Payload";

		if (!Q_strnicmp(map, "build_", 6) ||
			!Q_strnicmp(map, "gabe_", 5) ||
			!Q_strnicmp(map, "sandbox", 7) ||
			!Q_strnicmp(map, "sb_", 3))
			return "Sandbox";

		if (!Q_strnicmp(map, "d1_", 3) ||
			!Q_strnicmp(map, "d2_", 3) ||
			!Q_strnicmp(map, "d3_", 3))
			return "Half-Life 2";

		if (!Q_strnicmp(map, "ep1_", 4))
			return "Episode 1";

		if (!Q_strnicmp(map, "ep2_", 4))
			return "Episode 2";

		if (!Q_strnicmp(map, "lostcoast", 9))
			return "Lostcoast";

		if (!Q_strnicmp(map, "testchmb_", 9) ||
			!Q_strnicmp(map, "portal_", 7))
			return "Portal";

		if (!Q_strnicmp(map, "de_", 3))
			return "Bomb Defusal";

		if (!Q_strnicmp(map, "cs_", 3))
			return "Counter Strike";

		if (!Q_strnicmp(map, "aim_", 4))
			return "Aim Maps";

		if (!Q_strnicmp(map, "surf_", 5))
			return "Surf";

		if (!Q_strnicmp(map, "bhop_", 5))
			return "Bunny Hop";

		if (!Q_strnicmp(map, "jail_", 5))
			return "Jailbreak";

		if (!Q_strnicmp(map, "zm_", 3) ||
			!Q_strnicmp(map, "zs_", 3))
			return "Zombie Survival";

		if (!Q_strnicmp(map, "coop_", 5))
			return "Cooperative";

		if (!Q_strnicmp(map, "survival_", 9))
			return "Survival";

		if (!Q_strnicmp(map, "background", 10))
			return "Background Maps";

		if (!Q_strnicmp(map, "sdk_", 4) ||
			!Q_strnicmp(map, "dev_", 4) ||
			!Q_strnicmp(map, "test_", 5))
			return "Source SDK";

		if (!Q_strnicmp(map, "phys_", 5))
			return "Physics";

		if (!Q_strnicmp(map, "race_", 5))
			return "Racing";

		if (!Q_strnicmp(map, "parkour_", 8))
			return "Parkour";

		if (!Q_strnicmp(map, "puzzle_", 7))
			return "Puzzle";

		return "Misc";
	}

	void LoadMaps()
	{
		m_Maps.Purge();
		m_pMapList->DeleteAllItems();

		FileFindHandle_t fh = FILESYSTEM_INVALID_FIND_HANDLE;
		const char* pFile = g_pFullFileSystem->FindFirstEx("maps/*.bsp", "GAME", &fh);
		while (pFile)
		{
			char mapname[256];
			Q_strncpy(mapname, pFile, sizeof(mapname));
			StripExtensionInPlace(mapname);

			m_Maps.AddToTail(mapname);

			pFile = g_pFullFileSystem->FindNext(fh);
		}
		g_pFullFileSystem->FindClose(fh);
	}

	void RebuildImageList()
	{
		// 2007: no RemoveAllImages() so recreate the ImageList
		if (m_pImageList)
		{
			m_pMapList->SetImageList(NULL, false);
			delete m_pImageList;
			m_pImageList = NULL;
		}

		m_pImageList = new ImageList(false);
		m_pMapList->SetImageList(m_pImageList, false);
	}

	void ApplyFilter()
	{
		m_pMapList->DeleteAllItems();
		m_szSelectedMap[0] = 0;
		m_TileRows.RemoveAll();
		m_pTiles->DeleteAllItems();
		CGabeMapTileRow* tileRow = NULL;
		int tileCount = 0;
		RebuildImageList();

		char filter[256];
		m_pFilter->GetText(filter, sizeof(filter));
		Q_strlower(filter);

		for (int i = 0; i < m_Maps.Count(); ++i)
		{
			const char* map = m_Maps[i].String();

			if (filter[0])
			{
				char lowerMap[256];
				Q_strncpy(lowerMap, map, sizeof(lowerMap));
				Q_strlower(lowerMap);

				if (!Q_strstr(lowerMap, filter))
					continue;
			}

			// Try loading a per-map thumbnail: materials/vgui/maps/<map>.vmt
			char matPath[256];
			Q_snprintf(matPath, sizeof(matPath), "maps/%s", map);

			// Build real disk path for existence check
			char vmtPath[256];
			Q_snprintf(vmtPath, sizeof(vmtPath),
				"materials/vgui/maps/%s.vmt", map);

			// Let filesystem search normally
			bool exists = g_pFullFileSystem->FileExists(vmtPath);

			// If missing, use fallback logo (no .vmt extension here)
			const char* finalMat = exists ? matPath : "maps/missing";

			if (tileCount % 3 == 0)
			{
				tileRow = new CGabeMapTileRow(m_pTiles);
				m_pTiles->AddItem(NULL, tileRow);
				m_TileRows.AddToTail(tileRow);
			}
			tileRow->AddMap(this, map, finalMat, i);
			++tileCount;

			IImage* img = scheme()->GetImage(finalMat, true);
			// Do not resize shared scheme images; thumbnail panels scale them.
			int imageIndex = img ? m_pImageList->AddImage(img) : 0;

			KeyValues* kv = new KeyValues("item");
			kv->SetInt("icon", imageIndex);
			kv->SetString("map", map);
			kv->SetString("category", GetMapCategory(map));
			kv->SetString("lastplayed", GetMapLastPlayed(map));

			m_pMapList->AddItem(kv, 0, false, false);
			kv->deleteThis();
		}

		if (m_pMapList->GetItemCount() > 0)
			m_pMapList->SetSingleSelectedItem(
				m_pMapList->GetItemIDFromRow(0));
	}

private:
	CUtlVector<CGabeMapTileRow*> m_TileRows;
	PropertySheet* m_pMapSheet;
	PanelListPanel* m_pTiles;
	ListPanel* m_pMapList;
	ImageList* m_pImageList;

	TextEntry* m_pFilter;
	Label* m_pFilterLabel;
	Label* m_pMapSummary;
	Label* m_pHistory;
	char m_szSelectedMap[256];

	CUtlVector<CUtlString> m_Maps;
	char m_szLastFilter[256];
};

// ------------------------------------------------------------
// Settings tab (UPDATED: singleplayer checkbox)
// ------------------------------------------------------------
class CGamepadUIServerSettingsTab : public Panel
{
	DECLARE_CLASS_SIMPLE(CGamepadUIServerSettingsTab, Panel);
public:
	CGamepadUIServerSettingsTab(Panel* parent)
		: BaseClass(parent, "GamepadUIServerSettingsTab")
	{
		m_pSingleplayer = new CheckButton(this, "Singleplayer", "Singleplayer");
		m_pSingleplayer->SetSelected(true);

		m_pMaxPlayersLabel = new Label(this, "MaxPlayersLabel", "Max Players:");
		m_pMaxPlayers = new Slider(this, "MaxPlayers");
		m_pMaxPlayers->SetRange(1, 33);
		m_pMaxPlayers->SetValue(1);

		m_pTimeLimitLabel = new Label(this, "TimeLimitLabel", "Time Limit (min):");
		m_pTimeLimit = new Slider(this, "TimeLimit");
		m_pTimeLimit->SetRange(0, 120);
		m_pTimeLimit->SetValue(0);

		m_pFragLimitLabel = new Label(this, "FragLimitLabel", "Frag Limit:");
		m_pFragLimit = new Slider(this, "FragLimit");
		m_pFragLimit->SetRange(0, 200);
		m_pFragLimit->SetValue(0);

		m_pMaxPlayersValue = new Label(this, "MaxPlayersValue", "");
		m_pTimeLimitValue = new Label(this, "TimeLimitValue", "");
		m_pFragLimitValue = new Label(this, "FragLimitValue", "");

		m_pTeamplay = new CheckButton(this, "Teamplay", "Teamplay");
		m_pTeamplay->SetSelected(false);

		m_pFriendlyFire = new CheckButton(this, "FriendlyFire", "Friendly Fire");
		m_pFriendlyFire->SetSelected(true);

		m_pCheats = new CheckButton(this, "Cheats", "Allow Cheats");
		m_pCheats->SetSelected(true);

		m_pChangelogAtSpawn = new CheckButton(this, "Changelog At Spawn", "Changelog at spawn (obsolete)");
		m_pChangelogAtSpawn->SetSelected(false);

		m_pDeathmatchMode = new CheckButton(this, "Deathmatch Mode", "Deathmatch");
		m_pDeathmatchMode->SetSelected(false);

		m_pHL1Mode = new CheckButton(this, "Half-Life Mode", "Half-Life 1 loadout");
		m_pHL1Mode->SetSelected(false);

		m_pSandboxMode = new CheckButton(this, "Sandbox Mode", "Sandbox");
		m_pSandboxMode->SetSelected(true);

		m_pJBModMode = new CheckButton(this, "JBMod Mode", "JBMod");
		m_pJBModMode->SetSelected(false);

		m_pHL2Mode = new CheckButton(this, "Half-Life 2 Mode", "Half-Life 2 loadout");
		m_pHL2Mode->SetSelected(false);

		m_pBEModMode = new CheckButton(this, "BEMod Mode", "BEMod");
		m_pBEModMode->SetSelected(false);

		ApplySingleplayerLock();

		m_pSessionHeader = new Label(this, "SessionHeader", "SESSION OPTIONS");
		m_pModeHeader = new Label(this, "ModeHeader", "GAME MODE & LOADOUT");
		NG_StyleLabel(m_pSessionHeader);
		NG_StyleLabel(m_pModeHeader);
		NG_StyleLabel(m_pMaxPlayersLabel, true);
		NG_StyleLabel(m_pTimeLimitLabel, true);
		NG_StyleLabel(m_pFragLimitLabel, true);
		NG_StyleLabel(m_pMaxPlayersValue);
		NG_StyleLabel(m_pTimeLimitValue);
		NG_StyleLabel(m_pFragLimitValue);

		m_bLastSingleplayer = true;
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		int w, h; GetSize(w, h);
		const int pad = 12;
		const int row = clamp((h - 16) / 16, 15, 28);
		int y = 8;
		m_pSessionHeader->SetBounds(pad, y, w - 2 * pad, row); y += row;
		m_pSingleplayer->SetBounds(pad, y, w - 2 * pad, row); y += row;
		LayoutSliderRow(m_pMaxPlayersLabel, m_pMaxPlayers, m_pMaxPlayersValue, pad, y, w - pad, row); y += row;
		LayoutSliderRow(m_pTimeLimitLabel, m_pTimeLimit, m_pTimeLimitValue, pad, y, w - pad, row); y += row;
		LayoutSliderRow(m_pFragLimitLabel, m_pFragLimit, m_pFragLimitValue, pad, y, w - pad, row); y += row;
		CheckButton* session[] = { m_pTeamplay, m_pFriendlyFire, m_pCheats };
		for (int i = 0; i < ARRAYSIZE(session); ++i)
		{
			session[i]->SetBounds(pad, y, w - 2 * pad, row); y += row;
		}
		y += 6;
		m_pModeHeader->SetBounds(pad, y, w - 2 * pad, row); y += row;
		CheckButton* modes[] = { m_pSandboxMode, m_pDeathmatchMode, m_pHL1Mode, m_pHL2Mode, m_pJBModMode, m_pBEModMode, m_pChangelogAtSpawn };
		for (int i = 0; i < ARRAYSIZE(modes); ++i)
		{
			modes[i]->SetBounds(pad, y, w - 2 * pad, row); y += row;
		}
		UpdateValueLabels();
	}

	void PaintBackground()
	{
		NG_FillRect(0, 0, GetWide(), GetTall(), NG_BG);
		NG_FillRect(8, 8, GetWide() - 16, 2, NG_ACCENT);
	}

	void UpdateValueLabels()
	{
		char buf[32];

		Q_snprintf(buf, sizeof(buf), "%d", m_pMaxPlayers->GetValue());
		m_pMaxPlayersValue->SetText(buf);

		Q_snprintf(buf, sizeof(buf), "%d", m_pTimeLimit->GetValue());
		m_pTimeLimitValue->SetText(buf);

		Q_snprintf(buf, sizeof(buf), "%d", m_pFragLimit->GetValue());
		m_pFragLimitValue->SetText(buf);
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();

		// Detect checkbox changes without relying on action signals
		const bool now = m_pSingleplayer->IsSelected();
		if (now != m_bLastSingleplayer)
		{
			m_bLastSingleplayer = now;
			ApplySingleplayerLock();
		}

		UpdateValueLabels();
	}

	bool IsSingleplayer() const { return m_pSingleplayer->IsSelected(); }

	int GetMaxPlayers() const
	{
		// Hard force
		if (IsSingleplayer())
			return 1;
		return m_pMaxPlayers->GetValue();
	}
	int GetTimeLimit() const { return m_pTimeLimit->GetValue(); }
	int GetFragLimit() const { return m_pFragLimit->GetValue(); }
	bool GetTeamplay() const { return m_pTeamplay->IsSelected(); }
	bool GetFriendlyFire() const { return m_pFriendlyFire->IsSelected(); }
	bool GetCheats() const { return m_pCheats->IsSelected(); }
	bool GetChangelogAtSpawn() const { return m_pChangelogAtSpawn->IsSelected(); }
	bool GetDeathmatchMode() const { return m_pDeathmatchMode->IsSelected(); }
	bool GetSandboxMode() const { return m_pSandboxMode->IsSelected(); }
	bool GetHL1Mode() const { return m_pHL1Mode->IsSelected(); }
	bool GetJBModMode() const { return m_pJBModMode->IsSelected(); }
	bool GetHL2Mode() const { return m_pHL2Mode->IsSelected(); }
	bool GetBEModMode() const { return m_pBEModMode->IsSelected(); }

private:
	void ApplySingleplayerLock()
	{
		const bool single = m_pSingleplayer->IsSelected();

		if (single)
		{
			m_pMaxPlayers->SetValue(1);

			m_pMaxPlayers->SetEnabled(false);
			m_pMaxPlayersLabel->SetEnabled(false);
			m_pMaxPlayersValue->SetEnabled(false);
		}
		else
		{
			m_pMaxPlayers->SetEnabled(true);
			m_pMaxPlayersLabel->SetEnabled(true);
			m_pMaxPlayersValue->SetEnabled(true);

			if (m_pMaxPlayers->GetValue() <= 1)
				m_pMaxPlayers->SetValue(8);
		}
	}

	void LayoutSliderRow(
		Label* lbl,
		Slider* s,
		Label* value,
		int x, int y, int w, int h)
	{
		const int labelW = 120;
		const int valueW = 36;

		lbl->SetPos(x, y);
		lbl->SetSize(labelW, h);

		value->SetPos(w - valueW - x, y);
		value->SetSize(valueW, h);
		value->SetContentAlignment(Label::a_center);

		s->SetPos(x + labelW + 8, y);
		s->SetSize(w - (labelW + valueW + 16), h);
	}

private:
	Label* m_pSessionHeader;
	Label* m_pModeHeader;
	CheckButton* m_pSingleplayer;
	bool m_bLastSingleplayer;

	Label* m_pMaxPlayersLabel;
	Slider* m_pMaxPlayers;

	Label* m_pTimeLimitLabel;
	Slider* m_pTimeLimit;

	Label* m_pFragLimitLabel;
	Slider* m_pFragLimit;

	Label* m_pMaxPlayersValue;
	Label* m_pTimeLimitValue;
	Label* m_pFragLimitValue;

	CheckButton* m_pTeamplay;
	CheckButton* m_pFriendlyFire;
	CheckButton* m_pCheats;
	CheckButton* m_pChangelogAtSpawn;
	CheckButton* m_pDeathmatchMode;
	CheckButton* m_pHL1Mode;
	CheckButton* m_pSandboxMode;
	CheckButton* m_pJBModMode;
	CheckButton* m_pHL2Mode;
	CheckButton* m_pBEModMode;
};

// ------------------------------------------------------------
// Server Info tab
// ------------------------------------------------------------
class CGamepadUIServerInfoTab : public Panel
{
	DECLARE_CLASS_SIMPLE(CGamepadUIServerInfoTab, Panel);
public:
	CGamepadUIServerInfoTab(Panel* parent)
		: BaseClass(parent, "GamepadUIServerInfoTab")
	{
		m_pNameLabel = new Label(this, "NameLabel", "Server Name:");
		m_pName = new TextEntry(this, "ServerName");
		m_pName->SetText("GabeMod+ Server");

		m_pPasswordLabel = new Label(this, "PasswordLabel", "Password:");
		m_pPassword = new TextEntry(this, "Password");
		m_pPassword->SetText("");

		m_pLANOnly = new CheckButton(this, "LANOnly", "LAN Only?");
		m_pLANOnly->SetSelected(true);
		m_pLANOnly->SetEnabled(true);

		m_pPublic = new CheckButton(this, "Public", "Show in Server Browser?");
		m_pPublic->SetSelected(false);
		m_pPublic->SetEnabled(true);
		m_pInfoHeader = new Label(this, "InfoHeader", "SERVER IDENTITY");
		NG_StyleLabel(m_pInfoHeader);
		NG_StyleLabel(m_pNameLabel, true);
		NG_StyleLabel(m_pPasswordLabel, true);
		m_pName->SetBgColor(NG_PANEL);
		m_pName->SetFgColor(NG_TEXT);
		m_pPassword->SetBgColor(NG_PANEL);
		m_pPassword->SetFgColor(NG_TEXT);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);

		const int pad = 12;
		const int rowH = 22;
		int y = pad;
		m_pInfoHeader->SetBounds(pad, y, w - pad * 2, 23);
		y += 32;

		m_pNameLabel->SetPos(pad, y);
		m_pNameLabel->SetSize(100, rowH);

		m_pName->SetPos(pad + 110, y);
		m_pName->SetSize(w - (pad + 110) - pad, rowH);
		y += rowH + 8;

		m_pPasswordLabel->SetPos(pad, y);
		m_pPasswordLabel->SetSize(100, rowH);

		m_pPassword->SetPos(pad + 110, y);
		m_pPassword->SetSize(w - (pad + 110) - pad, rowH);
		y += rowH + 10;

		m_pLANOnly->SetPos(pad, y);
		m_pLANOnly->SetSize(w - pad * 2, rowH);
		y += rowH + 4;

		m_pPublic->SetPos(pad, y);
		m_pPublic->SetSize(w - pad * 2, rowH);
		y += rowH + 10;
	}

	void PaintBackground()
	{
		NG_FillRect(0, 0, GetWide(), GetTall(), NG_BG);
		NG_FillRect(8, 8, GetWide() - 16, 2, NG_ACCENT);
	}

	// ------------------------------------------------------------
	// Getters
	// ------------------------------------------------------------
	void GetServerName(char* out, int len) const
	{
		m_pName->GetText(out, len);
	}

	void GetPassword(char* out, int len) const
	{
		m_pPassword->GetText(out, len);
	}

	bool IsLANOnly() const { return m_pLANOnly->IsSelected(); }
	bool IsPublic()  const { return m_pPublic->IsSelected(); }

private:
	Label* m_pInfoHeader;
	Label* m_pNameLabel;
	TextEntry* m_pName;

	Label* m_pPasswordLabel;
	TextEntry* m_pPassword;

	CheckButton* m_pLANOnly;
	CheckButton* m_pPublic;
};

class CGamepadUINewServer : public Frame
{
	DECLARE_CLASS_SIMPLE(CGamepadUINewServer, Frame);
public:
	CGamepadUINewServer(VPANEL parent)
		: BaseClass(NULL, "GamepadUINewServer")
	{
		SetParent(parent);

		SetTitle("New Game", false);
		SetSize(1100, 720);
		SetMinimumSize(640, 400);
		SetDeleteSelfOnClose(false);
		SetProportional(false);
		SetSizeable(true);
		SetMoveable(true);
		SetCloseButtonVisible(true);

		m_pMapTab = new CGamepadUIMapTab(this);
		m_pSettingsTab = new CGamepadUIServerSettingsTab(this);
		m_pServerInfoTab = new CGamepadUIServerInfoTab(this);
		m_pSettingsButton = new CNewGameButton(this, "Settings", "Settings", this, "Settings", false);
		m_pInfoButton = new CNewGameButton(this, "ServerInfo", "Server Info", this, "ServerInfo", false);
		m_pStart = new CNewGameButton(this, "Start", "Create Game", this, "StartServer", true);
		m_pCancel = new CNewGameButton(this, "Cancel", "Cancel", this, "Cancel", false);
		m_pServerInfoTab->SetVisible(false);
		m_pSettingsTab->SetVisible(true);
		m_iScreenW = m_iScreenH = 0;
		FitToScreen();
		SetVisible(false);
		CenterOnScreen();
		InvalidateLayout(true, true);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();
		int w, h; GetSize(w, h);
		const int pad = 10, top = 34, bottom = 50;
		int left = (w - 3 * pad) * 54 / 100;
		int rightX = left + 2 * pad;
		int rightW = w - rightX - pad;
		int contentH = MAX(1, h - top - bottom);
		m_pMapTab->SetBounds(pad, top, left, contentH);
		m_pSettingsButton->SetBounds(rightX, top, (rightW - 4) / 2, 28);
		m_pInfoButton->SetBounds(rightX + (rightW + 4) / 2, top, (rightW - 4) / 2, 28);
		m_pSettingsTab->SetBounds(rightX, top + 34, rightW, MAX(1, contentH - 34));
		m_pServerInfoTab->SetBounds(rightX, top + 34, rightW, MAX(1, contentH - 34));
		m_pCancel->SetBounds(w - 110, h - 40, 100, 30);
		m_pStart->SetBounds(w - 282, h - 40, 164, 30);
	}

	void FitToScreen()
	{
		int sw, sh; surface()->GetScreenSize(sw, sh);
		if (sw <= 0 || sh <= 0) return;
		m_iScreenW = sw; m_iScreenH = sh;
		SetMinimumSize(MIN(640, sw - 16), MIN(400, sh - 16));
		SetSize(MIN(1200, sw - 24), MIN(820, sh - 24));
		CenterOnScreen();
		InvalidateLayout(true, true);
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();
		int sw, sh; surface()->GetScreenSize(sw, sh);
		if (sw != m_iScreenW || sh != m_iScreenH) FitToScreen();
		m_pStart->SetEnabled(GetSelectedMap()[0] != 0);
	}

	virtual void PaintBackground()
	{
		NG_FillRect(0, 0, GetWide(), GetTall(), NG_BG);
		NG_FillRect(10, GetTall() - 48, GetWide() - 20, 1, NG_ACCENT);
		NG_OutlineRect(0, 0, GetWide(), GetTall(), Color(92, 92, 92, 255));
	}

	virtual void OnCommand(const char* command)
	{
		if (!Q_stricmp(command, "Settings") || !Q_stricmp(command, "ServerInfo"))
		{
			bool settings = !Q_stricmp(command, "Settings");
			m_pSettingsTab->SetVisible(settings);
			m_pServerInfoTab->SetVisible(!settings);
			return;
		}

		if (!Q_stricmp(command, "Cancel"))
		{
			SetVisible(false);
			return;
		}

		if (!Q_stricmp(command, "StartServer"))
		{
			StartServer();
			return;
		}

		BaseClass::OnCommand(command);
	}

	void CenterOnScreen()
	{
		int sw, sh;
		surface()->GetScreenSize(sw, sh);

		int w, h;
		GetSize(w, h);

		SetPos((sw - w) / 2, (sh - h) / 2);
	}

	static void SaveMapLastPlayed(const char* map)
	{
		time_t now = time(NULL);
		tm* lt = localtime(&now);

		static const char* months[] =
		{
			"January", "February", "March", "April",
			"May", "June", "July", "August",
			"September", "October", "November", "December"
		};

		char datetime[64];

		if (gabeplus_newgame_useampmclock.GetBool())
		{
			int hour = lt->tm_hour % 12;
			if (hour == 0) hour = 12;

			Q_snprintf(
				datetime,
				sizeof(datetime),
				"%s %d, %d at %d:%02d %s",
				months[lt->tm_mon],
				lt->tm_mday,
				lt->tm_year + 1900,
				hour,
				lt->tm_min,
				lt->tm_hour >= 12 ? "PM" : "AM"
			);

		}
		else
		{
			Q_snprintf(
				datetime,
				sizeof(datetime),
				"%s %d, %d at %02d:%02d",
				months[lt->tm_mon],
				lt->tm_mday,
				lt->tm_year + 1900,
				lt->tm_hour,
				lt->tm_min
			);
		}

		KeyValues* kv = new KeyValues("MapHistory");
		kv->LoadFromFile(g_pFullFileSystem, "cfg/gabeplus_map_history.txt", "GAME");
		kv->SetString(map, datetime);
		kv->SaveToFile(g_pFullFileSystem, "cfg/gabeplus_map_history.txt", "GAME");
		kv->deleteThis();
	}

	// ============================================================
	// VScript
	// ============================================================

	bool IsDialogVisible()
	{
		return IsVisible();
	}

	const char* GetSelectedMap() const
	{
		if (!m_pMapTab)
			return "";

		return m_pMapTab->GetSelectedMap();
	}

	bool IsSingleplayer() const
	{
		return m_pSettingsTab && m_pSettingsTab->IsSingleplayer();
	}

	int GetMaxPlayers() const
	{
		return m_pSettingsTab ? m_pSettingsTab->GetMaxPlayers() : 0;
	}

	int GetTimeLimit() const
	{
		return m_pSettingsTab ? m_pSettingsTab->GetTimeLimit() : 0;
	}

	int GetFragLimit() const
	{
		return m_pSettingsTab ? m_pSettingsTab->GetFragLimit() : 0;
	}

	bool GetTeamplay() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetTeamplay();
	}

	bool GetFriendlyFire() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetFriendlyFire();
	}

	bool GetCheats() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetCheats();
	}

	bool GetChangelogAtSpawn() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetChangelogAtSpawn();
	}

	bool GetDeathmatchMode() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetDeathmatchMode();
	}

	bool GetSandboxMode() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetSandboxMode();
	}

	bool GetHL1Mode() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetHL1Mode();
	}

	bool GetJBModMode() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetJBModMode();
	}

	bool GetHL2Mode() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetHL2Mode();
	}

	bool GetBEModMode() const
	{
		return m_pSettingsTab && m_pSettingsTab->GetBEModMode();
	}

	const char* GetServerName() const
	{
		static char name[128];

		name[0] = '\0';

		if (m_pServerInfoTab)
			m_pServerInfoTab->GetServerName(name, sizeof(name));

		return name;
	}

	const char* GetPassword() const
	{
		static char password[128];

		password[0] = '\0';

		if (m_pServerInfoTab)
			m_pServerInfoTab->GetPassword(password, sizeof(password));

		return password;
	}

	bool IsLANOnly() const
	{
		return m_pServerInfoTab && m_pServerInfoTab->IsLANOnly();
	}

	bool IsPublicServer() const
	{
		return m_pServerInfoTab && m_pServerInfoTab->IsPublic();
	}

	int GetDialogWidth()
	{
		int w, h;
		GetSize(w, h);
		return w;
	}

	int GetDialogHeight()
	{
		int w, h;
		GetSize(w, h);
		return h;
	}

	int GetDialogX()
	{
		int x, y;
		GetPos(x, y);
		return x;
	}

	int GetDialogY()
	{
		int x, y;
		GetPos(x, y);
		return y;
	}

private:
	CGamepadUIServerInfoTab* m_pServerInfoTab;

	void StartServer()
	{
#ifdef CLIENT_DLL
		const char* map = m_pMapTab->GetSelectedMap();
		if (!IsMapNameAllowed(map))
			return;

		const int maxplayers = m_pSettingsTab->GetMaxPlayers(); // already forced to 1 in singleplayer
		const int timelimit = m_pSettingsTab->GetTimeLimit();
		const int fraglimit = m_pSettingsTab->GetFragLimit();
		const bool teamplay = m_pSettingsTab->GetTeamplay();
		const bool ff = m_pSettingsTab->GetFriendlyFire();
		const bool cheats = m_pSettingsTab->GetCheats();
		const bool chlogatspawn = m_pSettingsTab->GetChangelogAtSpawn();
		const bool dmmode = m_pSettingsTab->GetDeathmatchMode();
		const bool hl1mode = m_pSettingsTab->GetHL1Mode();
		const bool sandboxmode = m_pSettingsTab->GetSandboxMode();
		const bool jbmodmode = m_pSettingsTab->GetJBModMode();
		const bool hl2mode = m_pSettingsTab->GetHL2Mode();
		const bool bemodmode = m_pSettingsTab->GetBEModMode();

		char hostname[128];
		char password[128];

		m_pServerInfoTab->GetServerName(hostname, sizeof(hostname));
		m_pServerInfoTab->GetPassword(password, sizeof(password));

		const bool lanOnly = m_pServerInfoTab->IsLANOnly();
		const bool publicServer = m_pServerInfoTab->IsPublic();

		{
			char cmd[512];

			Q_snprintf(cmd, sizeof(cmd), "hostname \"%s\"\n", hostname);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "sv_password \"%s\"\n", password);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "sv_lan %d\n", lanOnly ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "sv_allowupload 1\n");
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "sv_allowdownload 1\n");
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "maxplayers %d\n", maxplayers);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "mp_timelimit %d\n", timelimit);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "mp_fraglimit %d\n", fraglimit);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "mp_teamplay %d\n", teamplay ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "mp_friendlyfire %d\n", ff ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "sv_cheats %d\n", cheats ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "gabeplus_chlogatspawn %d\n", chlogatspawn ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "gabe_deathmatch %d\n", dmmode ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "gabe_hl1 %d\n", hl1mode ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "gabe_sandbox %d\n", sandboxmode ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "gabe_jbmod %d\n", jbmodmode ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "gabe_hl2 %d\n", hl2mode ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);

			Q_snprintf(cmd, sizeof(cmd), "gabe_bemod %d\n", bemodmode ? 1 : 0);
			engine->ClientCmd_Unrestricted(cmd);
		}

		{
			char mapCopy[256];
			Q_strncpy(mapCopy, map, sizeof(mapCopy));

			SaveMapLastPlayed(mapCopy);
			m_pMapTab->Refresh();

			char cmd[256];
			engine->ClientCmd_Unrestricted("disconnect\n");
			Q_snprintf(cmd, sizeof(cmd), "map %s\n", mapCopy);
			engine->ClientCmd_Unrestricted(cmd);
		}

		SetVisible(false);
#endif
	}

private:
	Button* m_pSettingsButton;
	Button* m_pInfoButton;
	int m_iScreenW, m_iScreenH;
	CGamepadUIMapTab* m_pMapTab;
	CGamepadUIServerSettingsTab* m_pSettingsTab;

	Button* m_pStart;
	Button* m_pCancel;
};

// ------------------------------------------------------------
// Factory + console command (CLIENT SAFE)
// ------------------------------------------------------------
static CGamepadUINewServer* g_pNewServer = NULL;

static void OpenOrToggleNewServerDialog()
{
	VPANEL root = enginevgui->GetPanel(PANEL_GAMEUIDLL);
	if (!root)
	{
		Warning("gabeplus_newgame: PANEL_GAMEUIDLL root is NULL\n");
		return;
	}

	if (!g_pNewServer)
	{
		g_pNewServer = new CGamepadUINewServer(root);
	}

	// Ensure parent is valid (reconnect/menu changes)
	g_pNewServer->SetParent(root);

	// Toggle visible
	g_pNewServer->SetVisible(!g_pNewServer->IsVisible());

	if (g_pNewServer->IsVisible())
	{
		g_pNewServer->MakePopup();
		g_pNewServer->MoveToFront();
		g_pNewServer->RequestFocus();
		g_pNewServer->SetZPos(10000);
	}
}

void CC_GabeNewGame()
{
	OpenOrToggleNewServerDialog();
}

static ConCommand gabeplus_newgame(
	"gabe_newgame",
	CC_GabeNewGame,
	"Open GabeMod+ new game dialog",
	FCVAR_CLIENTDLL
);

static bool Script_NewGameExists()
{
	return g_pNewServer != NULL;
}

static bool Script_NewGameIsVisible()
{
	return g_pNewServer && g_pNewServer->IsDialogVisible();
}

static const char* Script_NewGameGetSelectedMap()
{
	return g_pNewServer ? g_pNewServer->GetSelectedMap() : "";
}

static bool Script_NewGameIsSingleplayer()
{
	return g_pNewServer && g_pNewServer->IsSingleplayer();
}

static int Script_NewGameGetMaxPlayers()
{
	return g_pNewServer ? g_pNewServer->GetMaxPlayers() : 0;
}

static int Script_NewGameGetTimeLimit()
{
	return g_pNewServer ? g_pNewServer->GetTimeLimit() : 0;
}

static int Script_NewGameGetFragLimit()
{
	return g_pNewServer ? g_pNewServer->GetFragLimit() : 0;
}

static bool Script_NewGameGetTeamplay()
{
	return g_pNewServer && g_pNewServer->GetTeamplay();
}

static bool Script_NewGameGetFriendlyFire()
{
	return g_pNewServer && g_pNewServer->GetFriendlyFire();
}

static bool Script_NewGameGetCheats()
{
	return g_pNewServer && g_pNewServer->GetCheats();
}

static bool Script_NewGameGetChangelogAtSpawn()
{
	return g_pNewServer && g_pNewServer->GetChangelogAtSpawn();
}

static bool Script_NewGameGetDeathmatchMode()
{
	return g_pNewServer && g_pNewServer->GetDeathmatchMode();
}

static bool Script_NewGameGetSandboxMode()
{
	return g_pNewServer && g_pNewServer->GetSandboxMode();
}

static bool Script_NewGameGetHL1Mode()
{
	return g_pNewServer && g_pNewServer->GetHL1Mode();
}

static bool Script_NewGameGetJBModMode()
{
	return g_pNewServer && g_pNewServer->GetJBModMode();
}

static bool Script_NewGameGetHL2Mode()
{
	return g_pNewServer && g_pNewServer->GetHL2Mode();
}

static bool Script_NewGameGetBEModMode()
{
	return g_pNewServer && g_pNewServer->GetBEModMode();
}

static const char* Script_NewGameGetServerName()
{
	return g_pNewServer ? g_pNewServer->GetServerName() : "";
}

static const char* Script_NewGameGetPassword()
{
	return g_pNewServer ? g_pNewServer->GetPassword() : "";
}

static bool Script_NewGameIsLANOnly()
{
	return g_pNewServer && g_pNewServer->IsLANOnly();
}

static bool Script_NewGameIsPublic()
{
	return g_pNewServer && g_pNewServer->IsPublicServer();
}

static int Script_NewGameGetWidth()
{
	return g_pNewServer ? g_pNewServer->GetDialogWidth() : 0;
}

static int Script_NewGameGetHeight()
{
	return g_pNewServer ? g_pNewServer->GetDialogHeight() : 0;
}

static int Script_NewGameGetX()
{
	return g_pNewServer ? g_pNewServer->GetDialogX() : 0;
}

static int Script_NewGameGetY()
{
	return g_pNewServer ? g_pNewServer->GetDialogY() : 0;
}

void RegisterNewGameScriptFuncs()
{
	if (!g_pScriptVM)
		return;

	g_pScriptVM->Run("CL_NewGame <- {};\n");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameExists,
		"NewGame_Exists", "Returns whether the New Game dialog exists.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameIsVisible,
		"NewGame_IsVisible", "Returns whether the New Game dialog is visible.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetSelectedMap,
		"NewGame_GetSelectedMap", "Returns the currently selected map.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameIsSingleplayer,
		"NewGame_IsSingleplayer", "Returns whether singleplayer is selected.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetMaxPlayers,
		"NewGame_GetMaxPlayers", "Returns the selected maximum player count.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetTimeLimit,
		"NewGame_GetTimeLimit", "Returns the selected time limit.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetFragLimit,
		"NewGame_GetFragLimit", "Returns the selected frag limit.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetTeamplay,
		"NewGame_GetTeamplay", "Returns the teamplay setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetFriendlyFire,
		"NewGame_GetFriendlyFire", "Returns the friendly-fire setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetCheats,
		"NewGame_GetCheats", "Returns the cheats setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetChangelogAtSpawn,
		"NewGame_GetChangelogAtSpawn", "Returns the changelog-at-spawn setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetDeathmatchMode,
		"NewGame_GetDeathmatchMode", "Returns the deathmatch mode setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetSandboxMode,
		"NewGame_GetSandboxMode", "Returns the sandbox mode setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetHL1Mode,
		"NewGame_GetHL1Mode", "Returns the Half-Life 1 mode setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetJBModMode,
		"NewGame_GetJBModMode", "Returns the JBMod mode setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetHL2Mode,
		"NewGame_GetHL2Mode", "Returns the Half-Life 2 mode setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetBEModMode,
		"NewGame_GetBEModMode", "Returns the BEMod mode setting.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetServerName,
		"NewGame_GetServerName", "Returns the entered server name.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetPassword,
		"NewGame_GetPassword", "Returns the entered server password.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameIsLANOnly,
		"NewGame_IsLANOnly", "Returns whether LAN-only is enabled.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameIsPublic,
		"NewGame_IsPublic", "Returns whether public server is enabled.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetWidth,
		"NewGame_GetWidth", "Returns the dialog width.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetHeight,
		"NewGame_GetHeight", "Returns the dialog height.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetX,
		"NewGame_GetX", "Returns the dialog X position.");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NewGameGetY,
		"NewGame_GetY", "Returns the dialog Y position.");

	TABLE_FUNC_NAMED(CL_NewGame, NewGame_Exists, Exists);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_IsVisible, IsVisible);

	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetSelectedMap, GetSelectedMap);

	TABLE_FUNC_NAMED(CL_NewGame, NewGame_IsSingleplayer, IsSingleplayer);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetMaxPlayers, GetMaxPlayers);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetTimeLimit, GetTimeLimit);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetFragLimit, GetFragLimit);

	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetTeamplay, GetTeamplay);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetFriendlyFire, GetFriendlyFire);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetCheats, GetCheats);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetChangelogAtSpawn, GetChangelogAtSpawn);

	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetDeathmatchMode, GetDeathmatchMode);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetSandboxMode, GetSandboxMode);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetHL1Mode, GetHL1Mode);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetJBModMode, GetJBModMode);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetHL2Mode, GetHL2Mode);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetBEModMode, GetBEModMode);

	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetServerName, GetServerName);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetPassword, GetPassword);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_IsLANOnly, IsLANOnly);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_IsPublic, IsPublic);

	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetWidth, GetWidth);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetHeight, GetHeight);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetX, GetX);
	TABLE_FUNC_NAMED(CL_NewGame, NewGame_GetY, GetY);
}