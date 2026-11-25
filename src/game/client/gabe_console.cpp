//========= Gabe Mod Server Browser ======================//
//
// Purpose: Custom Steam server browser.
//
//========================================================//

#include "cbase.h"
#include "ienginevgui.h"

#include "steam/steam_api.h"
#include "steam/isteammatchmaking.h"

#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ListPanel.h>
#include <vgui_controls/TextEntry.h>

#include "tier1/utlvector.h"
#include <time.h>

#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include "tier0/memdbgon.h"

using namespace vgui;

//=========================================================
// Configuration
//=========================================================

ConVar gabe_browser_appid(
    "gabe_browser_appid", "243750", FCVAR_ARCHIVE);

ConVar gabe_browser_gamedir(
    "gabe_browser_gamedir", "", FCVAR_ARCHIVE);

ConVar gabe_browser_sounds(
    "gabe_browser_sounds", "1", FCVAR_ARCHIVE);

ConVar gabe_browser_hideempty(
    "gabe_browser_hideempty", "0", FCVAR_ARCHIVE);

ConVar gabe_browser_hidefull(
    "gabe_browser_hidefull", "0", FCVAR_ARCHIVE);

ConVar gabe_browser_hidepassworded(
    "gabe_browser_hidepassworded", "0", FCVAR_ARCHIVE);

//=========================================================
// Server list types
//=========================================================

enum GabeServerListType
{
    GABE_SERVERS_INTERNET = 0,
    GABE_SERVERS_FAVORITES,
    GABE_SERVERS_HISTORY,
    GABE_SERVERS_LAN
};

//=========================================================
// Stored server data
//=========================================================

struct GabeServerInfo_t
{
    char name[128];
    char map[64];
    char address[64];
    char gameDir[64];

    uint32 ip;
    uint16 connectionPort;
    uint16 queryPort;

    int players;
    int maxPlayers;
    int ping;

    bool passworded;
    bool secure;
};

class CGabeServerList : public ListPanel
{
    DECLARE_CLASS_SIMPLE(CGabeServerList, ListPanel);

public:

    CGabeServerList(Panel* parent, const char* name)
        : BaseClass(parent, name)
    {}

    virtual void OnKeyCodeTyped(KeyCode code)
    {
        if (code == KEY_ENTER)
        {
            if (GetSelectedItemsCount() > 0)
            {
                PostActionSignal(
                    new KeyValues("Command",
                        "command", "Connect"));
            }

            return;
        }

        BaseClass::OnKeyCodeTyped(code);
    }
};

//=========================================================
// Server browser
//=========================================================

class CGabeServerBrowser :
    public Frame,
    public ISteamMatchmakingServerListResponse
{
    DECLARE_CLASS_SIMPLE(CGabeServerBrowser, Frame);

public:

    CGabeServerBrowser(VPANEL parent)
        : BaseClass(NULL, "GabeServerBrowser")
    {
        SetParent(parent);

        SetTitle("Server Browser", false);

        SetSize(900, 560);
        SetMinimumSize(450, 300);

        SetProportional(false);
        SetSizeable(true);
        SetMoveable(true);

        SetCloseButtonVisible(true);
        SetMinimizeButtonVisible(false);
        SetMaximizeButtonVisible(false);

        SetDeleteSelfOnClose(false);

        SetMouseInputEnabled(true);
        SetKeyBoardInputEnabled(true);

        m_hRequest = NULL;

        m_pMatchmaking = NULL;

        m_ListType = GABE_SERVERS_INTERNET;

        m_bRefreshing = false;

        m_iScreenW = 0;
        m_iScreenH = 0;

        m_szLastFilter[0] = '\0';

        // Tabs.

        m_pInternet = new Button(
            this, "Internet",
            "Internet", this, "Internet");

        m_pFavorites = new Button(
            this, "Favorites",
            "Favorites", this, "Favorites");

        m_pHistory = new Button(
            this, "History",
            "History", this, "History");

        m_pLAN = new Button(
            this, "LAN",
            "LAN", this, "LAN");

        // Search box.

        m_pFilter = new TextEntry(
            this, "ServerFilter");

        m_pFilter->SetMultiline(false);
        m_pFilter->SetMaximumCharCount(128);

        // Refresh button.

        m_pRefresh = new Button(
            this, "Refresh",
            "Refresh", this, "Refresh");

        // Main server list.

        m_pServers = new CGabeServerList(
            this, "Servers");

        m_pServers->AddColumnHeader(
            0, "name", "Server Name",
            280, 100, 1000, 0);

        m_pServers->AddColumnHeader(
            1, "map", "Map",
            150, 70, 400, 0);

        m_pServers->AddColumnHeader(
            2, "players", "Players",
            70, 50, 100, 0);

        m_pServers->AddColumnHeader(
            3, "ping", "Ping",
            60, 40, 100, 0);

        m_pServers->AddColumnHeader(
            4, "secure", "VAC",
            45, 35, 80, 0);

        m_pServers->AddColumnHeader(
            5, "password", "Password",
            75, 50, 100, 0);

        m_pServers->SetMultiselectEnabled(false);

        m_pServers->SetEmptyListText(
            "No servers found.");

        m_pServers->SetSortColumn(3);
        m_pServers->SetSortFunc(3, SortPing);

        m_pServers->AddActionSignalTarget(this);



        // Bottom controls.

        m_pStatus = new Label(
            this, "Status", "0 servers");

        m_pConnect = new Button(
            this, "Connect",
            "Connect", this, "Connect");

        m_pAddFavorite = new Button(
            this, "AddFavorite",
            "Add Favorite", this, "AddFavorite");

        m_pRemoveFavorite = new Button(
            this, "RemoveFavorite",
            "Remove Favorite", this, "RemoveFavorite");

        SetVisible(false);

        ivgui()->AddTickSignal(
            GetVPanel(), 100);
    }

    virtual ~CGabeServerBrowser()
    {
        StopRefresh();

        ivgui()->RemoveTickSignal(
            GetVPanel());
    }

    //=====================================================
    // Numeric ping sorting
    //=====================================================

    static int SortPing(
        ListPanel* pPanel,
        const ListPanelItem& a,
        const ListPanelItem& b)
    {
        int pingA = a.kv->GetInt("ping");
        int pingB = b.kv->GetInt("ping");

        if (pingA < pingB)
            return -1;

        if (pingA > pingB)
            return 1;

        return 0;
    }

    //=====================================================
    // Scheme
    //=====================================================

    virtual void ApplySchemeSettings(
        IScheme* pScheme)
    {
        BaseClass::ApplySchemeSettings(pScheme);

        HFont font =
            pScheme->GetFont(
                "GabeMenuTiny", false);

        if (font == INVALID_FONT)
        {
            font = pScheme->GetFont(
                "GabeMenuLarge", false);
        }

        m_pServers->SetFont(font);
        m_pFilter->SetFont(font);
        m_pStatus->SetFont(font);

        m_pServers->SetBgColor(
            Color(15, 18, 15, 245));

        m_pServers->SetFgColor(
            Color(220, 225, 215, 255));

        m_pFilter->SetBgColor(
            Color(18, 22, 18, 255));

        m_pFilter->SetFgColor(
            Color(235, 240, 230, 255));

        m_pStatus->SetFgColor(
            Color(180, 190, 160, 255));

        Button* buttons[] =
        {
            m_pInternet,
            m_pFavorites,
            m_pHistory,
            m_pLAN,
            m_pRefresh,
            m_pConnect,
            m_pAddFavorite,
            m_pRemoveFavorite
        };

        for (int i = 0;
            i < ARRAYSIZE(buttons);
            i++)
        {
            Button* pButton = buttons[i];

            pButton->SetFont(font);

            pButton->SetDefaultColor(
                Color(220, 225, 215, 255),
                Color(38, 44, 36, 255));

            pButton->SetArmedColor(
                Color(255, 255, 255, 255),
                Color(82, 95, 69, 255));

            pButton->SetDepressedColor(
                Color(255, 255, 255, 255),
                Color(25, 31, 24, 255));

            pButton->SetPaintBorderEnabled(false);

            if (gabe_browser_sounds.GetBool())
            {
                pButton->SetArmedSound(
                    "ui/buttonrollover.wav");

                pButton->SetDepressedSound(
                    "ui/buttonclick.wav");

                pButton->SetReleasedSound(
                    "ui/buttonclickrelease.wav");
            }
        }
    }

    //=====================================================
    // Responsive layout
    //=====================================================

    virtual void PerformLayout()
    {
        BaseClass::PerformLayout();

        int w, h;
        GetSize(w, h);

        int margin = 10;
        int tabY = 34;

        int tabH = 28;
        int searchH = 28;

        int tabW = (w - 50) / 4;

        m_pInternet->SetBounds(
            margin,
            tabY, tabW, tabH);

        m_pFavorites->SetBounds(
            margin + tabW + 10,
            tabY, tabW, tabH);

        m_pHistory->SetBounds(
            margin + (tabW + 10) * 2,
            tabY, tabW, tabH);

        m_pLAN->SetBounds(
            margin + (tabW + 10) * 3,
            tabY, tabW, tabH);

        int searchY = tabY + tabH + 8;

        m_pFilter->SetBounds(
            margin,
            searchY,
            w - 120,
            searchH);

        m_pRefresh->SetBounds(
            w - 100,
            searchY,
            90,
            searchH);

        int listY =
            searchY + searchH + 8;

        m_pServers->SetBounds(
            margin,
            listY,
            MAX(1, w - 20),
            MAX(1, h - listY - 50));

        m_pStatus->SetBounds(
            margin,
            h - 35,
            MAX(1, w - 350),
            25);

        m_pRemoveFavorite->SetBounds(
            w - 320,
            h - 38,
            125,
            28);

        m_pAddFavorite->SetBounds(
            w - 190,
            h - 38,
            100,
            28);

        m_pConnect->SetBounds(
            w - 85,
            h - 38,
            75,
            28);

        // Distribute column widths.

        m_pServers->AddColumnHeader(
            0, "name", "Server Name",
            280, 100, 2000,
            ListPanel::COLUMN_RESIZEWITHWINDOW);

        m_pServers->AddColumnHeader(
            1, "map", "Map",
            150, 70, 1000,
            ListPanel::COLUMN_RESIZEWITHWINDOW);

        m_pServers->AddColumnHeader(
            2, "players", "Players",
            70, 50, 100, 0);

        m_pServers->AddColumnHeader(
            3, "ping", "Ping",
            60, 40, 100, 0);

        m_pServers->AddColumnHeader(
            4, "secure", "VAC",
            45, 35, 80, 0);

        m_pServers->AddColumnHeader(
            5, "password", "Password",
            75, 50, 100, 0);
    }

    //=====================================================
    // Custom background
    //=====================================================

    virtual void PaintBackground()
    {
        int w, h;
        GetSize(w, h);

        surface()->DrawSetColor(
            23, 27, 22, 245);

        surface()->DrawFilledRect(
            0, 0, w, h);

        surface()->DrawSetColor(
            180, 190, 160, 180);

        surface()->DrawFilledRect(
            0, 28, w, 30);
    }

    //=====================================================
    // Stop existing request
    //=====================================================

    void StopRefresh()
    {
        if (m_pMatchmaking && m_hRequest)
        {
            m_pMatchmaking->CancelQuery(
                m_hRequest);

            m_pMatchmaking->ReleaseRequest(
                m_hRequest);

            m_hRequest = NULL;
        }

        m_bRefreshing = false;

        if (m_pRefresh)
        {
            m_pRefresh->SetText("Refresh");
        }
    }

    //=====================================================
    // Request actual servers
    //=====================================================

    void RefreshServers()
    {
        StopRefresh();

        m_pServers->RemoveAll();
        m_ServerData.RemoveAll();

        m_pMatchmaking =
            SteamMatchmakingServers();

        if (!m_pMatchmaking)
        {
            m_pStatus->SetText(
                "Steam matchmaking unavailable.");

            return;
        }

        AppId_t appID =
            (AppId_t)gabe_browser_appid.GetInt();

        // Apply an optional game-directory filter.

        MatchMakingKeyValuePair_t gameFilter;

        MatchMakingKeyValuePair_t* pFilters[1];

        uint32 filterCount = 0;

        const char* gameDir =
            gabe_browser_gamedir.GetString();

        if (gameDir && gameDir[0])
        {
            Q_strncpy(
                gameFilter.m_szKey,
                "gamedir",
                sizeof(gameFilter.m_szKey));

            Q_strncpy(
                gameFilter.m_szValue,
                gameDir,
                sizeof(gameFilter.m_szValue));

            pFilters[filterCount++] =
                &gameFilter;
        }

        MatchMakingKeyValuePair_t** filters =
            filterCount ? pFilters : NULL;

        m_bRefreshing = true;

        m_pRefresh->SetText("Stop");

        m_pStatus->SetText(
            "Refreshing servers...");

        switch (m_ListType)
        {
        case GABE_SERVERS_INTERNET:
        {
            m_hRequest =
                m_pMatchmaking->
                RequestInternetServerList(
                    appID,
                    filters,
                    filterCount,
                    this);

            break;
        }

        case GABE_SERVERS_FAVORITES:
        {
            m_hRequest =
                m_pMatchmaking->
                RequestFavoritesServerList(
                    appID,
                    filters,
                    filterCount,
                    this);

            break;
        }

        case GABE_SERVERS_HISTORY:
        {
            m_hRequest =
                m_pMatchmaking->
                RequestHistoryServerList(
                    appID,
                    filters,
                    filterCount,
                    this);

            break;
        }

        case GABE_SERVERS_LAN:
        {
            m_hRequest =
                m_pMatchmaking->
                RequestLANServerList(
                    appID,
                    this);

            break;
        }
        }

        if (!m_hRequest)
        {
            m_bRefreshing = false;

            m_pRefresh->SetText("Refresh");

            m_pStatus->SetText(
                "Could not request server list.");
        }
    }

    //=====================================================
    // Steam callback: server responded
    //=====================================================

    virtual void ServerResponded(
        HServerListRequest hRequest,
        int iServer)
    {
        if (!m_pMatchmaking ||
            hRequest != m_hRequest)
        {
            return;
        }

        gameserveritem_t* pServer =
            m_pMatchmaking->GetServerDetails(
                hRequest, iServer);

        if (!pServer)
            return;

        GabeServerInfo_t info;

        Q_memset(&info, 0, sizeof(info));

        Q_strncpy(
            info.name,
            pServer->GetName(),
            sizeof(info.name));

        Q_strncpy(
            info.map,
            pServer->m_szMap,
            sizeof(info.map));

        Q_strncpy(
            info.gameDir,
            pServer->m_szGameDir,
            sizeof(info.gameDir));

        Q_strncpy(
            info.address,
            pServer->m_NetAdr.GetConnectionAddressString(),
            sizeof(info.address));

        info.ip =
            pServer->m_NetAdr.GetIP();

        info.connectionPort =
            pServer->m_NetAdr.GetConnectionPort();

        info.queryPort =
            pServer->m_NetAdr.GetQueryPort();

        info.players =
            pServer->m_nPlayers;

        info.maxPlayers =
            pServer->m_nMaxPlayers;

        info.ping =
            pServer->m_nPing;

        info.passworded =
            pServer->m_bPassword;

        info.secure =
            pServer->m_bSecure;

        // Prevent duplicate entries.

        int index = -1;

        for (int i = 0;
            i < m_ServerData.Count();
            i++)
        {
            if (m_ServerData[i].ip == info.ip &&
                m_ServerData[i].connectionPort ==
                info.connectionPort)
            {
                index = i;
                break;
            }
        }

        if (index == -1)
        {
            index =
                m_ServerData.AddToTail(info);
        }
        else
        {
            m_ServerData[index] = info;
        }

        m_bListDirty = true;
    }

    //=====================================================
    // Steam callback: server timed out
    //=====================================================

    virtual void ServerFailedToRespond(
        HServerListRequest hRequest,
        int iServer)
    {
        // Ignore servers that failed to respond.
    }

    //=====================================================
    // Steam callback: refresh finished
    //=====================================================

    virtual void RefreshComplete(
        HServerListRequest hRequest,
        EMatchMakingServerResponse response)
    {
        if (hRequest != m_hRequest)
            return;

        m_bRefreshing = false;

        m_pRefresh->SetText("Refresh");

        m_bListDirty = true;
    }

    //=====================================================
    // Filtering
    //=====================================================

    bool ShouldShowServer(
        const GabeServerInfo_t& info,
        const char* filter)
    {
        if (gabe_browser_hideempty.GetBool() &&
            info.players == 0)
        {
            return false;
        }

        if (gabe_browser_hidefull.GetBool() &&
            info.players >= info.maxPlayers)
        {
            return false;
        }

        if (gabe_browser_hidepassworded.GetBool() &&
            info.passworded)
        {
            return false;
        }

        const char* gameDir =
            gabe_browser_gamedir.GetString();

        if (gameDir && gameDir[0] &&
            Q_stricmp(info.gameDir, gameDir))
        {
            return false;
        }

        if (filter && filter[0])
        {
            if (!Q_stristr(info.name, filter) &&
                !Q_stristr(info.map, filter) &&
                !Q_stristr(info.address, filter))
            {
                return false;
            }
        }

        return true;
    }

    //=====================================================
    // Build visible server list
    //=====================================================

    void RebuildServerList()
    {
        char filter[128];

        m_pFilter->GetText(
            filter, sizeof(filter));

        // Preserve selection when rebuilding.

        int selectedIndex =
            GetSelectedServerIndex();

        m_pServers->RemoveAll();

        int visibleCount = 0;

        for (int i = 0;
            i < m_ServerData.Count();
            i++)
        {
            GabeServerInfo_t& info =
                m_ServerData[i];

            if (!ShouldShowServer(info, filter))
                continue;

            KeyValues* kv =
                new KeyValues("Server");

            kv->SetString(
                "name", info.name);

            kv->SetString(
                "map", info.map);

            char players[32];

            Q_snprintf(
                players, sizeof(players),
                "%d/%d",
                info.players,
                info.maxPlayers);

            kv->SetString(
                "players", players);

            kv->SetInt(
                "ping", info.ping);

            kv->SetString(
                "secure",
                info.secure ? "Yes" : "No");

            kv->SetString(
                "password",
                info.passworded ? "Yes" : "No");

            kv->SetInt(
                "server_index", i);

            int itemID =
                m_pServers->AddItem(
                    kv, i, false, false);

            kv->deleteThis();

            if (i == selectedIndex)
            {
                m_pServers->SetSingleSelectedItem(
                    itemID);
            }

            visibleCount++;
        }

        m_pServers->SortList();

        char status[128];

        Q_snprintf(
            status, sizeof(status),
            "%d/%d servers%s",
            visibleCount,
            m_ServerData.Count(),
            m_bRefreshing
            ? " - Refreshing..."
            : "");

        m_pStatus->SetText(status);
    }

    //=====================================================
    // Selected server
    //=====================================================

    int GetSelectedServerIndex()
    {
        if (m_pServers->GetSelectedItemsCount() <= 0)
            return -1;

        int itemID =
            m_pServers->GetSelectedItem(0);

        KeyValues* kv =
            m_pServers->GetItem(itemID);

        if (!kv)
            return -1;

        return kv->GetInt(
            "server_index", -1);
    }

    GabeServerInfo_t* GetSelectedServer()
    {
        int index =
            GetSelectedServerIndex();

        if (!m_ServerData.IsValidIndex(index))
            return NULL;

        return &m_ServerData[index];
    }

    //=====================================================
    // Connect
    //=====================================================

    void ConnectToServer()
    {
        GabeServerInfo_t* pServer =
            GetSelectedServer();

        if (!pServer)
        {
            m_pStatus->SetText(
                "Select a server first.");

            return;
        }

        char command[128];

        // Generate the address from numeric fields,
        // rather than trusting arbitrary server text.

        uint32 ip = pServer->ip;

        Q_snprintf(
            command,
            sizeof(command),
            "connect %u.%u.%u.%u:%u\n",
            (ip >> 24) & 255,
            (ip >> 16) & 255,
            (ip >> 8) & 255,
            ip & 255,
            (unsigned int)pServer->connectionPort);

        engine->ClientCmd_Unrestricted(
            command);

        OnClose();
    }

    //=====================================================
    // Favorites
    //=====================================================

    void AddFavorite()
    {
        GabeServerInfo_t* pServer =
            GetSelectedServer();

        if (!pServer || !SteamMatchmaking())
            return;

        SteamMatchmaking()->AddFavoriteGame(
            (AppId_t)gabe_browser_appid.GetInt(),
            pServer->ip,
            pServer->connectionPort,
            pServer->queryPort,
            k_unFavoriteFlagFavorite,
            (uint32)time(NULL));

        m_pStatus->SetText(
            "Server added to favorites.");
    }

    void RemoveFavorite()
    {
        GabeServerInfo_t* pServer =
            GetSelectedServer();

        if (!pServer || !SteamMatchmaking())
            return;

        SteamMatchmaking()->RemoveFavoriteGame(
            (AppId_t)gabe_browser_appid.GetInt(),
            pServer->ip,
            pServer->connectionPort,
            pServer->queryPort,
            k_unFavoriteFlagFavorite);

        if (m_ListType == GABE_SERVERS_FAVORITES)
        {
            RefreshServers();
        }
    }

    //=====================================================
    // Commands
    //=====================================================

    virtual void OnCommand(
        const char* command)
    {
        if (!Q_stricmp(command, "Internet"))
        {
            m_ListType = GABE_SERVERS_INTERNET;
            RefreshServers();
            return;
        }

        if (!Q_stricmp(command, "Favorites"))
        {
            m_ListType = GABE_SERVERS_FAVORITES;
            RefreshServers();
            return;
        }

        if (!Q_stricmp(command, "History"))
        {
            m_ListType = GABE_SERVERS_HISTORY;
            RefreshServers();
            return;
        }

        if (!Q_stricmp(command, "LAN"))
        {
            m_ListType = GABE_SERVERS_LAN;
            RefreshServers();
            return;
        }

        if (!Q_stricmp(command, "Refresh"))
        {
            if (m_bRefreshing)
                StopRefresh();
            else
                RefreshServers();

            return;
        }

        if (!Q_stricmp(command, "Connect"))
        {
            ConnectToServer();
            return;
        }

        if (!Q_stricmp(command, "AddFavorite"))
        {
            AddFavorite();
            return;
        }

        if (!Q_stricmp(command, "RemoveFavorite"))
        {
            RemoveFavorite();
            return;
        }

        BaseClass::OnCommand(command);
    }

    //=====================================================
    // Open / close
    //=====================================================

    void OpenBrowser()
    {
        SetVisible(true);

        MakePopup();

        SetMouseInputEnabled(true);
        SetKeyBoardInputEnabled(true);

        MoveToFront();
        RequestFocus();

        RefreshServers();
    }

    virtual void OnClose()
    {
        StopRefresh();

        SetVisible(false);

        SetMouseInputEnabled(false);
        SetKeyBoardInputEnabled(false);
    }

    //=====================================================
    // Updates and resolution changes
    //=====================================================

    virtual void OnTick()
    {
        BaseClass::OnTick();

        int sw, sh;

        surface()->GetScreenSize(sw, sh);

        if (sw > 0 && sh > 0 &&
            (sw != m_iScreenW ||
                sh != m_iScreenH))
        {
            m_iScreenW = sw;
            m_iScreenH = sh;

            int w = (int)(sw * 0.82f);
            int h = (int)(sh * 0.70f);

            SetBounds(
                (sw - w) / 2,
                (sh - h) / 2,
                w, h);

            InvalidateLayout(true, true);
            Repaint();
        }

        char filter[128];

        m_pFilter->GetText(
            filter, sizeof(filter));

        if (Q_strcmp(filter, m_szLastFilter))
        {
            Q_strncpy(
                m_szLastFilter,
                filter,
                sizeof(m_szLastFilter));

            m_bListDirty = true;
        }

        if (m_bListDirty)
        {
            m_bListDirty = false;

            RebuildServerList();
        }

        bool selected =
            GetSelectedServer() != NULL;

        m_pConnect->SetEnabled(selected);

        m_pAddFavorite->SetEnabled(selected);

        m_pRemoveFavorite->SetEnabled(
            selected &&
            m_ListType ==
            GABE_SERVERS_FAVORITES);
    }

private:

    Button* m_pInternet;
    Button* m_pFavorites;
    Button* m_pHistory;
    Button* m_pLAN;

    Button* m_pRefresh;
    Button* m_pConnect;
    Button* m_pAddFavorite;
    Button* m_pRemoveFavorite;

    TextEntry* m_pFilter;

    ListPanel* m_pServers;

    Label* m_pStatus;

    CUtlVector<GabeServerInfo_t> m_ServerData;

    ISteamMatchmakingServers* m_pMatchmaking;

    HServerListRequest m_hRequest;

    GabeServerListType m_ListType;

    bool m_bRefreshing;

    bool m_bListDirty = false;

    int m_iScreenW;
    int m_iScreenH;

    char m_szLastFilter[128];
};

//=========================================================
// Global instance
//=========================================================

static CGabeServerBrowser* g_pGabeBrowser = NULL;

//=========================================================
// Initialization
//=========================================================

void GabeServerBrowser_Init()
{
    if (g_pGabeBrowser)
        return;

    if (!enginevgui)
        return;

    VPANEL parent =
        enginevgui->GetPanel(
            PANEL_GAMEUIDLL);

    if (!parent)
        return;

    g_pGabeBrowser =
        new CGabeServerBrowser(parent);

    HScheme hScheme =
        scheme()->LoadSchemeFromFile(
            "resource/ClientScheme.res",
            "ClientScheme");

    g_pGabeBrowser->SetScheme(hScheme);

    g_pGabeBrowser->InvalidateLayout(
        true, true);
}

//=========================================================
// Shutdown
//=========================================================

void GabeServerBrowser_Shutdown()
{
    delete g_pGabeBrowser;

    g_pGabeBrowser = NULL;
}

//=========================================================
// Open browser
//=========================================================

void GabeServerBrowser_Open()
{
    GabeServerBrowser_Init();

    if (!g_pGabeBrowser)
        return;

    engine->ClientCmd_Unrestricted(
        "gameui_activate\n");

    g_pGabeBrowser->OpenBrowser();
}

//=========================================================
// Console commands
//=========================================================

CON_COMMAND(
    gabe_serverbrowser,
    "Opens the custom server browser.")
{
    GabeServerBrowser_Open();
}

CON_COMMAND(
    gabe_serverbrowser_close,
    "Closes the custom server browser.")
{
    if (g_pGabeBrowser)
    {
        g_pGabeBrowser->OnClose();
    }
}