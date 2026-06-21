// gabeplus_friendslist.cpp
// Expanded 2007-compatible friends list panel with Steam + fake friend support

#include "cbase.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>

#include <vgui_controls/Frame.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/CheckButton.h>

#include "ienginevgui.h"

#include "steam/steam_api.h"
#include "steam/isteamfriends.h"
#include "steam/isteamutils.h"

using namespace vgui;

static uint64 Gabe_Q_atoui64(const char* s)
{
	if (!s || !s[0])
		return 0;

	uint64 out = 0;

	while (*s)
	{
		if (*s < '0' || *s > '9')
			break;

		out = out * 10 + (*s - '0');
		++s;
	}

	return out;
}

static const char* PersonaStateToString(EPersonaState state)
{
	switch (state)
	{
	case k_EPersonaStateOffline: return "Offline";
	case k_EPersonaStateOnline: return "Online";
	case k_EPersonaStateBusy: return "Busy";
	case k_EPersonaStateAway: return "Away";
	case k_EPersonaStateSnooze: return "Snooze";
	case k_EPersonaStateLookingToTrade: return "Trading";
	case k_EPersonaStateLookingToPlay: return "Looking To Play";
	default: return "Unknown";
	}
}

class CSteamAvatarImage : public vgui::IImage
{
public:
	CSteamAvatarImage(int texID, int w, int h)
		: m_iTexture(texID),
		m_iWide(w),
		m_iTall(h),
		m_iX(0),
		m_iY(0),
		m_Color(255, 255, 255, 255)
	{}

	virtual void Paint()
	{
		surface()->DrawSetColor(m_Color);
		surface()->DrawSetTexture(m_iTexture);
		surface()->DrawTexturedRect(m_iX, m_iY, m_iX + m_iWide, m_iY + m_iTall);
	}

	virtual void SetPos(int x, int y)
	{
		m_iX = x;
		m_iY = y;
	}

	virtual void GetContentSize(int& w, int& h)
	{
		w = m_iWide;
		h = m_iTall;
	}

	virtual void GetSize(int& w, int& h)
	{
		w = m_iWide;
		h = m_iTall;
	}

	virtual void SetSize(int w, int h)
	{
		m_iWide = w;
		m_iTall = h;
	}

	virtual void SetColor(Color col)
	{
		m_Color = col;
	}

	virtual bool Evict() { return false; }

	virtual int GetNumFrames()
	{
		return 1;
	}

	virtual void SetFrame(int nFrame) {}

	virtual HTexture GetID()
	{
		return 0;
	}

	virtual void SetRotation(int iRotation) {}

	virtual Color GetColor()
	{
		return Color(255, 255, 255, 255);
	}

	virtual bool IsError()
	{
		return false;
	}

private:
	int m_iTexture;
	int m_iWide;
	int m_iTall;
	int m_iX;
	int m_iY;
	Color m_Color;
};

class CGabePlusFriendsPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CGabePlusFriendsPanel, Frame);

public:
	CGabePlusFriendsPanel(VPANEL parent)
		: BaseClass(NULL, "GabePlusFriendsPanel")
	{
		SetParent(parent);

		SetTitle("Friends", true);
		SetSize(760, 620);
		SetMoveable(true);
		SetSizeable(false);
		SetCloseButtonVisible(true);
		SetDeleteSelfOnClose(false);

		m_pSearchLabel = new Label(this, "SearchLabel", "Search:");
		m_pSearch = new TextEntry(this, "Search");
		m_pSearch->SetText("");

		m_pOnlineOnly = new CheckButton(this, "OnlineOnly", "Online only");
		m_pOnlineOnly->SetSelected(false);

		m_pSummary = new Label(this, "Summary", "");

		m_pList = new ListPanel(this, "FriendsList");

		m_pList->AddColumnHeader(0, "icon", "", 72, ListPanel::COLUMN_IMAGE);
		m_pList->AddColumnHeader(1, "name", "Friend", 250, ListPanel::COLUMN_RESIZEWITHWINDOW);
		m_pList->AddColumnHeader(2, "status", "Status", 110, ListPanel::COLUMN_FIXEDSIZE);
		m_pList->AddColumnHeader(3, "type", "Type", 80, ListPanel::COLUMN_FIXEDSIZE);
		m_pList->AddColumnHeader(4, "steamid", "SteamID", 160, ListPanel::COLUMN_FIXEDSIZE);

		m_pList->SetRowHeight(72);
		m_pList->SetMultiselectEnabled(false);
		m_pList->SetEmptyListText("No friends found.");
		m_pList->AddActionSignalTarget(this);

		m_pImageList = new ImageList(false);
		m_pList->SetImageList(m_pImageList, false);

		m_pOverlay = new Button(this, "OverlayButton", "Steam Friends", this, "OpenOverlay");
		m_pChat = new Button(this, "ChatButton", "Chat", this, "OpenChat");
		m_pProfile = new Button(this, "ProfileButton", "Profile", this, "OpenProfile");
		m_pRefresh = new Button(this, "RefreshButton", "Refresh", this, "Refresh");
		m_pRemoveFake = new Button(this, "RemoveFakeButton", "Remove Fake", this, "RemoveFake");
		m_pClose = new Button(this, "CloseButton", "Close", this, "ClosePanel");

		m_szLastSearch[0] = 0;
		m_bLastOnlineOnly = false;

		PopulateFriends();
		CenterOnScreen();
	}

	virtual ~CGabePlusFriendsPanel()
	{
		if (m_pImageList)
		{
			delete m_pImageList;
			m_pImageList = NULL;
		}
	}

	struct FakeFriend_t
	{
		CUtlString name;
		bool online;
	};

	void AddFakeFriend(const char* name, bool online)
	{
		if (!name || !name[0])
			return;

		FakeFriend_t ff;
		ff.name = name;
		ff.online = online;

		m_FakeFriends.AddToTail(ff);
		PopulateFriends();
	}

	void ClearFakeFriends()
	{
		m_FakeFriends.Purge();
		PopulateFriends();
	}

	virtual void PerformLayout()
	{
		BaseClass::PerformLayout();

		int w, h;
		GetSize(w, h);

		const int pad = 12;
		const int topY = pad + 28;
		const int rowH = 24;
		const int btnH = 26;
		const int btnW = 105;
		const int gap = 8;

		m_pSearchLabel->SetBounds(pad, topY, 55, rowH);
		m_pSearch->SetBounds(pad + 60, topY, 260, rowH);
		m_pOnlineOnly->SetBounds(pad + 330, topY, 120, rowH);
		m_pSummary->SetBounds(pad + 460, topY, w - (pad + 460) - pad, rowH);

		int buttonY = h - pad - btnH;
		int x = pad;

		m_pOverlay->SetBounds(x, buttonY, btnW + 25, btnH);
		x += btnW + 25 + gap;

		m_pChat->SetBounds(x, buttonY, btnW - 25, btnH);
		x += btnW - 25 + gap;

		m_pProfile->SetBounds(x, buttonY, btnW - 15, btnH);
		x += btnW - 15 + gap;

		m_pRefresh->SetBounds(x, buttonY, btnW - 15, btnH);
		x += btnW - 15 + gap;

		m_pRemoveFake->SetBounds(x, buttonY, btnW + 5, btnH);

		m_pClose->SetBounds(w - pad - btnW, buttonY, btnW, btnH);

		m_pList->SetBounds(
			pad,
			topY + rowH + 8,
			w - pad * 2,
			h - (topY + rowH + 8) - btnH - pad * 2
		);
	}

	virtual void OnThink()
	{
		BaseClass::OnThink();

		char search[256];
		m_pSearch->GetText(search, sizeof(search));

		bool onlineOnly = m_pOnlineOnly->IsSelected();

		if (Q_stricmp(search, m_szLastSearch) != 0 || onlineOnly != m_bLastOnlineOnly)
		{
			Q_strncpy(m_szLastSearch, search, sizeof(m_szLastSearch));
			m_bLastOnlineOnly = onlineOnly;
			PopulateFriends();
		}
	}

	virtual void OnCommand(const char* cmd)
	{
		if (!Q_stricmp(cmd, "Refresh"))
		{
			PopulateFriends();
			return;
		}

		if (!Q_stricmp(cmd, "OpenOverlay"))
		{
			if (steamapicontext && steamapicontext->SteamFriends())
				steamapicontext->SteamFriends()->ActivateGameOverlay("Friends");

			return;
		}

		if (!Q_stricmp(cmd, "OpenChat"))
		{
			OpenSelectedSteamOverlay("chat");
			return;
		}

		if (!Q_stricmp(cmd, "OpenProfile"))
		{
			OpenSelectedSteamOverlay("steamid");
			return;
		}

		if (!Q_stricmp(cmd, "RemoveFake"))
		{
			RemoveSelectedFakeFriend();
			return;
		}

		if (!Q_stricmp(cmd, "ClosePanel"))
		{
			SetVisible(false);
			return;
		}

		BaseClass::OnCommand(cmd);
	}

private:
	CUtlVector<FakeFriend_t> m_FakeFriends;

	ListPanel* m_pList;
	ImageList* m_pImageList;

	TextEntry* m_pSearch;
	Label* m_pSearchLabel;
	Label* m_pSummary;
	CheckButton* m_pOnlineOnly;

	Button* m_pRefresh;
	Button* m_pOverlay;
	Button* m_pChat;
	Button* m_pProfile;
	Button* m_pRemoveFake;
	Button* m_pClose;

	char m_szLastSearch[256];
	bool m_bLastOnlineOnly;

	void RebuildImageList()
	{
		if (m_pImageList)
			delete m_pImageList;

		m_pImageList = new ImageList(false);
		m_pList->SetImageList(m_pImageList, false);
	}

	KeyValues* GetSelectedFriend()
	{
		int itemID = m_pList->GetSelectedItem(0);
		if (itemID == -1)
			return NULL;

		return m_pList->GetItem(itemID);
	}

	bool PassesFilter(const char* name, bool online)
	{
		if (m_pOnlineOnly && m_pOnlineOnly->IsSelected() && !online)
			return false;

		char filter[256];
		filter[0] = 0;

		if (m_pSearch)
			m_pSearch->GetText(filter, sizeof(filter));

		if (!filter[0])
			return true;

		char lowerName[256];
		char lowerFilter[256];

		Q_strncpy(lowerName, name ? name : "", sizeof(lowerName));
		Q_strncpy(lowerFilter, filter, sizeof(lowerFilter));

		Q_strlower(lowerName);
		Q_strlower(lowerFilter);

		return Q_strstr(lowerName, lowerFilter) != NULL;
	}

	int CreateColorAvatar(const char* seedName, bool online)
	{
		const int w = 32;
		const int h = 32;
		const int imageSize = w * h * 4;

		unsigned char* rgba = new unsigned char[imageSize];

		unsigned int hash = 2166136261u;
		const char* s = seedName ? seedName : "friend";

		while (*s)
		{
			hash ^= (unsigned char)(*s);
			hash *= 16777619u;
			++s;
		}

		unsigned char r = 60 + (hash & 127);
		unsigned char g = 60 + ((hash >> 8) & 127);
		unsigned char b = 60 + ((hash >> 16) & 127);

		if (online)
			g = 180;
		else
		{
			r = 80;
			g = 80;
			b = 80;
		}

		for (int p = 0; p < w * h; ++p)
		{
			rgba[p * 4 + 0] = r;
			rgba[p * 4 + 1] = g;
			rgba[p * 4 + 2] = b;
			rgba[p * 4 + 3] = 255;
		}

		int texID = surface()->CreateNewTextureID(true);

		surface()->DrawSetTextureRGBA(texID, rgba, w, h, 1, true);

		CSteamAvatarImage* img = new CSteamAvatarImage(texID, w, h);
		int imageIndex = m_pImageList->AddImage(img);

		if (imageIndex != -1)
			img->SetSize(64, 64);

		delete[] rgba;
		return imageIndex;
	}

	int CreateSteamAvatar(CSteamID id)
	{
		if (!steamapicontext || !steamapicontext->SteamFriends() || !steamapicontext->SteamUtils())
			return -1;

		ISteamFriends* pFriends = steamapicontext->SteamFriends();
		ISteamUtils* pUtils = steamapicontext->SteamUtils();

		int avatar = pFriends->GetMediumFriendAvatar(id);
		if (avatar == -1)
			return -1;

		uint32 w = 0;
		uint32 h = 0;

		if (!pUtils->GetImageSize(avatar, &w, &h))
			return -1;

		const int imageSize = w * h * 4;
		unsigned char* rgba = new unsigned char[imageSize];

		int imageIndex = -1;

		if (pUtils->GetImageRGBA(avatar, rgba, imageSize))
		{
			int texID = surface()->CreateNewTextureID(true);

			surface()->DrawSetTextureRGBA(texID, rgba, w, h, 1, true);

			CSteamAvatarImage* img = new CSteamAvatarImage(texID, w, h);
			imageIndex = m_pImageList->AddImage(img);

			if (imageIndex != -1)
				img->SetSize(64, 64);
		}

		delete[] rgba;
		return imageIndex;
	}

	void AddFriendRow(
		const char* name,
		const char* status,
		bool online,
		const char* type,
		const char* steamid,
		int imageIndex)
	{
		if (!PassesFilter(name, online))
			return;

		KeyValues* kv = new KeyValues("item");
		kv->SetInt("icon", imageIndex);
		kv->SetString("name", name ? name : "Unknown");
		kv->SetString("status", status ? status : "Unknown");
		kv->SetString("type", type ? type : "");
		kv->SetString("steamid", steamid ? steamid : "");

		m_pList->AddItem(kv, 0, false, false);
		kv->deleteThis();
	}

	void PopulateFriends()
	{
		m_pList->DeleteAllItems();
		RebuildImageList();

		int steamTotal = 0;
		int steamOnline = 0;
		int fakeTotal = m_FakeFriends.Count();
		int fakeOnline = 0;

		if (steamapicontext && steamapicontext->SteamFriends())
		{
			ISteamFriends* pFriends = steamapicontext->SteamFriends();

			int count = pFriends->GetFriendCount(k_EFriendFlagImmediate);
			steamTotal = count;

			for (int i = 0; i < count; ++i)
			{
				CSteamID id = pFriends->GetFriendByIndex(i, k_EFriendFlagImmediate);

				const char* name = pFriends->GetFriendPersonaName(id);
				EPersonaState state = pFriends->GetFriendPersonaState(id);

				bool online = state != k_EPersonaStateOffline;
				if (online)
					steamOnline++;

				int imageIndex = CreateSteamAvatar(id);
				if (imageIndex == -1)
					imageIndex = CreateColorAvatar(name, online);

				char steamid[64];
				Q_snprintf(steamid, sizeof(steamid), "%llu", id.ConvertToUint64());

				AddFriendRow(
					name,
					PersonaStateToString(state),
					online,
					"Steam",
					steamid,
					imageIndex
				);
			}
		}

		for (int i = 0; i < m_FakeFriends.Count(); ++i)
		{
			const FakeFriend_t& ff = m_FakeFriends[i];

			if (ff.online)
				fakeOnline++;

			int imageIndex = CreateColorAvatar(ff.name.String(), ff.online);

			char fakeID[64];
			Q_snprintf(fakeID, sizeof(fakeID), "fake:%d", i);

			AddFriendRow(
				ff.name.String(),
				ff.online ? "Online" : "Offline",
				ff.online,
				"Fake",
				fakeID,
				imageIndex
			);
		}

		char summary[256];
		Q_snprintf(
			summary,
			sizeof(summary),
			"Steam: %d/%d online   Fake: %d/%d online",
			steamOnline,
			steamTotal,
			fakeOnline,
			fakeTotal
		);

		m_pSummary->SetText(summary);

		if (m_pList->GetItemCount() > 0)
		{
			m_pList->SetSingleSelectedItem(
				m_pList->GetItemIDFromRow(0)
			);
		}
	}

	void OpenSelectedSteamOverlay(const char* mode)
	{
		KeyValues* kv = GetSelectedFriend();

		if (!kv || !steamapicontext || !steamapicontext->SteamFriends())
			return;

		const char* type = kv->GetString("type", "");
		if (Q_stricmp(type, "Steam"))
			return;

		const char* idText = kv->GetString("steamid", "");
		uint64 id64 = Gabe_Q_atoui64(idText);

		if (!id64)
			return;

		CSteamID id(id64);
		steamapicontext->SteamFriends()->ActivateGameOverlayToUser(mode, id);
	}

	void RemoveSelectedFakeFriend()
	{
		KeyValues* kv = GetSelectedFriend();
		if (!kv)
			return;

		const char* type = kv->GetString("type", "");
		if (Q_stricmp(type, "Fake"))
			return;

		const char* idText = kv->GetString("steamid", "");
		if (!idText || Q_strnicmp(idText, "fake:", 5))
			return;

		int index = atoi(idText + 5);

		if (index < 0 || index >= m_FakeFriends.Count())
			return;

		m_FakeFriends.Remove(index);
		PopulateFriends();
	}

	void CenterOnScreen()
	{
		int sw, sh;
		surface()->GetScreenSize(sw, sh);

		int w, h;
		GetSize(w, h);

		SetPos((sw - w) / 2, (sh - h) / 2);
	}
};

CGabePlusFriendsPanel* g_pFriends = NULL;

CON_COMMAND_F(gabe_friends, "Open friends list", FCVAR_CLIENTDLL)
{
	VPANEL root = enginevgui->GetPanel(PANEL_GAMEUIDLL);
	if (!root)
		return;

	if (!g_pFriends)
		g_pFriends = new CGabePlusFriendsPanel(root);

	g_pFriends->SetParent(root);
	g_pFriends->SetVisible(true);
	g_pFriends->MakePopup();
	g_pFriends->MoveToFront();
	g_pFriends->RequestFocus();
}

CON_COMMAND_F(gabe_newfriend, "Create fake friend: gabe_newfriend <name> <0/1>", FCVAR_CLIENTDLL)
{
	if (!g_pFriends)
	{
		Msg("Open the friends panel first.\n");
		return;
	}

	if (args.ArgC() < 2)
	{
		Msg("Usage: gabe_newfriend <name> <online 0/1>\n");
		return;
	}

	const char* name = args[1];
	bool online = true;

	if (args.ArgC() >= 3)
		online = atoi(args[2]) != 0;

	g_pFriends->AddFakeFriend(name, online);

	Msg("Added fake friend: %s\n", name);
}

CON_COMMAND_F(gabe_clearfakefriends, "Clear all fake friends", FCVAR_CLIENTDLL)
{
	if (!g_pFriends)
	{
		Msg("Open the friends panel first.\n");
		return;
	}

	g_pFriends->ClearFakeFriends();
	Msg("Cleared fake friends.\n");
}