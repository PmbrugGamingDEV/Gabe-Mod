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
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/MessageBox.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/Slider.h>
#include "vgui_controls/imagelist.h"
#include <vgui_controls/Label.h>
#include <vgui_controls/TextEntry.h>
#include <time.h>

#include "filesystem.h"
#include "ienginevgui.h"
#include "vgui/IInput.h"

#ifdef CLIENT_DLL
#include "cdll_client_int.h"
extern IVEngineClient* engine;
#endif

using namespace vgui;

ConVar gabeplus_newgame_useampmclock("gabe_newgame_useampmclock", "1", FCVAR_ARCHIVE, "If set to 1, use the 12-hour clock (AM/PM)");

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
		if (*c == ' ' || *c == ';' || *c == '"')
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
// Map selection tab
// ------------------------------------------------------------
class CGamepadUIMapTab : public Panel
{
	DECLARE_CLASS_SIMPLE(CGamepadUIMapTab, Panel);

public:
	CGamepadUIMapTab(Panel* parent)
		: BaseClass(parent, "GamepadUIMapTab")
	{
		m_pMapList = new ListPanel(this, "MapList");

		// Create image list for thumbnails/icons
		m_pImageList = new ImageList(false);
		m_pMapList->SetImageList(m_pImageList, false);

		m_pMapList->AddColumnHeader(0, "icon", "", 256, ListPanel::COLUMN_IMAGE);
		m_pMapList->AddColumnHeader(1, "map", "Map", 176, 0);
		m_pMapList->AddColumnHeader(2, "category", "Category", 108, 0);
		m_pMapList->AddColumnHeader(3, "lastplayed", "Last Played", 240, 0);

		m_pMapList->SetRowHeight(128);

		m_pMapList->SetMultiselectEnabled(false);
		m_pMapList->SetEmptyListText("No maps found.");

		m_pMapList->AddActionSignalTarget(this);

		m_pFilter = new TextEntry(this, "Filter");
		m_pFilterLabel = new Label(this, "FilterLabel", "Filter:");

		m_szLastFilter[0] = 0;

		LoadMaps();
		ApplyFilter();
	}

	virtual ~CGamepadUIMapTab()
	{
		// 2007-safe cleanup
		if (m_pImageList)
		{
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

		int w, h;
		GetSize(w, h);

		const int pad = 12;
		const int filterH = 22;

		m_pFilterLabel->SetPos(pad, pad);
		m_pFilterLabel->SetSize(44, filterH);

		m_pFilter->SetPos(pad + 48, pad);
		m_pFilter->SetSize(w - (pad + 48) - pad, filterH);

		m_pMapList->SetPos(pad, pad + filterH + 8);
		m_pMapList->SetSize(w - pad * 2, h - (pad + filterH + 8) - pad);
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
	}

	void OnMouseDoublePressed(int itemID)
	{
		if (itemID == -1 || !GetSelectedMap()[0])
			return;

		PostMessage(
			GetParent()->GetParent(),
			new KeyValues("Command", "command", "StartServer")
		);
	}

	const char* GetSelectedMap() const
	{
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
			delete m_pImageList;
			m_pImageList = NULL;
		}

		m_pImageList = new ImageList(false);
		m_pMapList->SetImageList(m_pImageList, false);
	}

	void ApplyFilter()
	{
		m_pMapList->DeleteAllItems();
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

			IImage* img = scheme()->GetImage(finalMat, true);
			int imageIndex = m_pImageList->AddImage(img);

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
	ListPanel* m_pMapList;
	ImageList* m_pImageList;

	TextEntry* m_pFilter;
	Label* m_pFilterLabel;

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
		m_pSingleplayer = new CheckButton(this, "Singleplayer", "Singleplayer Game (Default)");
		m_pSingleplayer->SetSelected(true);

		m_pMaxPlayersLabel = new Label(this, "MaxPlayersLabel", "Max Players (including yourself):");
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

		m_pTeamplay = new CheckButton(this, "Teamplay", "Checked = Team Based, Unchecked = Free for all");
		m_pTeamplay->SetSelected(false);

		m_pFriendlyFire = new CheckButton(this, "FriendlyFire", "Friendly Fire (mp_friendlyfire)");
		m_pFriendlyFire->SetSelected(true);

		m_pCheats = new CheckButton(this, "Cheats", "Allow Cheats (sv_cheats)");
		m_pCheats->SetSelected(true);

		m_pChangelogAtSpawn = new CheckButton(this, "Changelog At Spawn", "Show the changelog every time you spawn (Obsolete)");
		m_pChangelogAtSpawn->SetSelected(false);

		m_pDeathmatchMode = new CheckButton(this, "Deathmatch Mode", "Deathmatch Gamemode (No sandbox, No godmode)");
		m_pDeathmatchMode->SetSelected(false);

		m_pHL1Mode = new CheckButton(this, "Half-Life Mode", "Spawn With Half-Life 1 weapons");
		m_pHL1Mode->SetSelected(false);

		m_pSandboxMode = new CheckButton(this, "Sandbox Mode", "Sandbox Gamemode (Default)");
		m_pSandboxMode->SetSelected(true);

		m_pJBModMode = new CheckButton(this, "JBMod Mode", "JBMod Gamemode (Like the JBMod on Steam)");
		m_pJBModMode->SetSelected(false);

		ApplySingleplayerLock();

		m_bLastSingleplayer = true;
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);

		const int pad = 12;
		const int rowH = 24;
		int y = pad;

		m_pSingleplayer->SetPos(pad, y);
		m_pSingleplayer->SetSize(w - pad * 2, rowH);
		y += rowH + 10;

		LayoutSliderRow(m_pMaxPlayersLabel, m_pMaxPlayers, m_pMaxPlayersValue, pad, y, w, rowH);
		y += rowH + 8;

		LayoutSliderRow(m_pTimeLimitLabel, m_pTimeLimit, m_pTimeLimitValue, pad, y, w, rowH);
		y += rowH + 8;

		LayoutSliderRow(m_pFragLimitLabel, m_pFragLimit, m_pFragLimitValue, pad, y, w, rowH);
		y += rowH + 12;

		m_pTeamplay->SetPos(pad, y);          m_pTeamplay->SetSize(w - pad * 2, rowH); y += rowH + 4;
		m_pFriendlyFire->SetPos(pad, y);      m_pFriendlyFire->SetSize(w - pad * 2, rowH); y += rowH + 4;
		m_pCheats->SetPos(pad, y);            m_pCheats->SetSize(w - pad * 2, rowH); y += rowH + 10;
		m_pChangelogAtSpawn->SetPos(pad, y);  m_pChangelogAtSpawn->SetSize(w - pad * 2, rowH); y += rowH + 4;
		m_pDeathmatchMode->SetPos(pad, y);	  m_pDeathmatchMode->SetSize(w - pad * 2, rowH); y += rowH + 4;
		m_pHL1Mode->SetPos(pad, y);			  m_pHL1Mode->SetSize(w - pad * 2, rowH); y += rowH + 4;
		m_pSandboxMode->SetPos(pad, y);		  m_pSandboxMode->SetSize(w - pad * 2, rowH); y += rowH + 4;
		m_pJBModMode->SetPos(pad, y);		  m_pJBModMode->SetSize(w - pad * 2, rowH); y += rowH + 4;

		UpdateValueLabels();
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
		m_pPassword->SetText("Your Password Goes Here");

		m_pLANOnly = new CheckButton(this, "LANOnly", "LAN Only?");
		m_pLANOnly->SetSelected(true);
		m_pLANOnly->SetEnabled(true);

		m_pPublic = new CheckButton(this, "Public", "Show in Server Browser?");
		m_pPublic->SetSelected(false);
		m_pPublic->SetEnabled(true);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);

		const int pad = 12;
		const int rowH = 22;
		int y = pad;

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
	Label* m_pNameLabel;
	TextEntry* m_pName;

	Label* m_pPasswordLabel;
	TextEntry* m_pPassword;

	CheckButton* m_pLANOnly;
	CheckButton* m_pPublic;
};

// ------------------------------------------------------------
// Main frame (IMPORTANT: VPANEL parenting in 2007)
// ------------------------------------------------------------
class CGamepadUINewServer : public Frame
{
	DECLARE_CLASS_SIMPLE(CGamepadUINewServer, Frame);
public:
	CGamepadUINewServer(VPANEL parent)
		: BaseClass(NULL, "GamepadUINewServer") // 2007: Frame ctor does NOT take VPANEL
	{
		SetParent(parent); // 2007-correct parenting

		SetTitle("NEW GAME", true);
		SetSizeable(false);
		SetMoveable(true);
		SetCloseButtonVisible(true);

		m_pSheet = new PropertySheet(this, "Sheet");
		m_pMapTab = new CGamepadUIMapTab(m_pSheet);
		m_pSettingsTab = new CGamepadUIServerSettingsTab(m_pSheet);

		m_pSheet->AddPage(m_pMapTab, "Select Map");
		m_pSheet->AddPage(m_pSettingsTab, "Settings");

		m_pStart = new Button(this, "Start", "Create New Game", this, "StartServer");
		m_pCancel = new Button(this, "Cancel", "Cancel", this, "Cancel");

		m_pServerInfoTab = new CGamepadUIServerInfoTab(m_pSheet);
		m_pSheet->AddPage(m_pServerInfoTab, "Server Info");

		SetSize(780, 600);
		CenterOnScreen();
		InvalidateLayout(true, true);
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);

		const int pad = 12;
		const int btnH = 26;

		m_pSheet->SetPos(pad, pad + 24);
		m_pSheet->SetSize(w - pad * 2, h - (pad + 24) - (pad + btnH + 6));

		const int btnW = 140;

		m_pCancel->SetSize(btnW, btnH);
		m_pStart->SetSize(btnW + 40, btnH);

		m_pCancel->SetPos(w - pad - btnW, h - pad - btnH);
		m_pStart->SetPos(w - pad - btnW - 10 - (btnW + 40), h - pad - btnH);
	}

	virtual void OnCommand(const char* command)
	{
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
	PropertySheet* m_pSheet;
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
