// Client DLL - Source SDK Base 2013
#include "cbase.h"
#include "filesystem.h"
#include "cdll_client_int.h"
#include "ienginevgui.h"
#include "steam/steam_api.h"
#include "tier1/KeyValues.h"
#include "tier1/utlbuffer.h"
#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/CheckButton.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>
#include "tier0/memdbgon.h"

using namespace vgui;

struct MountGame_t
{
    const char* field;
    const char* title;
    AppId_t app;
    const char* folder;
    AppId_t fallback;
    const char* pathID;
};
static const MountGame_t g_Games[] =
{
    { "HalfLife2", "Half-Life 2", 220, "hl2", 0, "HL2" },
    { "CounterStrike", "Counter-Strike: Source", 240, "cstrike", 0, "CSS" },
    { "HalfLife", "Half-Life: Source", 280, "hl1", 0, "HLS" },
    { "DayOfDefeat", "Day of Defeat: Source", 300, "dod", 0, "DODS" },
    { "LostCoast", "Half-Life 2: Lost Coast", 340, "lostcoast", 220, "LOSTCOAST" },
    { "HalfLifeDeathmatch", "Half-Life Deathmatch: Source", 360, "hl1mp", 0, "HL1MP" },
    { "Episodic", "Half-Life 2: Episode One", 380, "episodic", 220, "EP1" },
    { "Portal", "Portal", 400, "portal", 0, "PORTAL" },
    { "EpisodeTwo", "Half-Life 2: Episode Two", 420, "ep2", 220, "EP2" },
    { "TeamFortress", "Team Fortress 2", 440, "tf", 0, "TF2" }
};
static const char* kMountPrefix = "game+gabe_mount_";

static bool ResolveGame(const MountGame_t& game, char* out, int size)
{
    out[0] = 0;
    ISteamApps* apps = SteamApps();
    if (!apps) return false;
    AppId_t ids[] = { game.app, game.fallback };
    for (int i = 0; i < ARRAYSIZE(ids); ++i)
    {
        if (!ids[i] || !apps->BIsAppInstalled(ids[i])) continue;
        char root[MAX_PATH];
        if (!apps->GetAppInstallDir(ids[i], root, sizeof(root))) continue;
        Q_snprintf(out, size, "%s/%s", root, game.folder);
        Q_FixSlashes(out, '/');
        if (filesystem->IsDirectory(out)) return true;
    }
    out[0] = 0;
    return false;
}

static void GameInfoPath(char* out, int size)
{
    Q_snprintf(out, size, "%s/gameinfo.txt", engine->GetGameDirectory());
    Q_FixSlashes(out, '/');
}

static KeyValues* LoadGameInfo()
{
    char path[MAX_PATH]; GameInfoPath(path, sizeof(path));
    KeyValues* kv = new KeyValues("GameInfo");
    if (!kv->LoadFromFile(filesystem, path, NULL))
    {
        kv->deleteThis(); return NULL;
    }
    return kv;
}

static KeyValues* SearchPaths(KeyValues* kv)
{
    if (!kv) return NULL;
    KeyValues* fs = kv->FindKey("FileSystem");
    return fs ? fs->FindKey("SearchPaths") : NULL;
}

static void MountKey(int i, char* out, int size)
{
    // Keep the ownership marker for safe removal, and expose the game ID.
    Q_snprintf(out, size, "%s%s+%s", kMountPrefix, g_Games[i].field, g_Games[i].pathID);
}

static void AddPath(KeyValues* paths, const char* key, const char* value)
{
    // AddSubKey preserves duplicate search-path keys.
    KeyValues* entry = new KeyValues(key);
    entry->SetString(NULL, value);
    paths->AddSubKey(entry);
}

class CGabeGameMounter : public Frame
{
    DECLARE_CLASS_SIMPLE(CGabeGameMounter, Frame);
public:
    CGabeGameMounter(VPANEL parent) : BaseClass(NULL, "GabeGameMounter")
    {
        SetParent(parent);
        SetTitle("Mount Games", true);
        SetSize(440, 395);
        SetSizeable(false);
        SetDeleteSelfOnClose(false);
        SetProportional(false);
        EditablePanel* body = new EditablePanel(this, "ContentSubGames");
        body->SetBounds(0, 30, 440, 315);
        for (int i = 0; i < ARRAYSIZE(g_Games); ++i)
            m_Check[i] = new CheckButton(body, g_Games[i].field, g_Games[i].title);
        Label* note = new Label(body, "Label1", "Restart the game to apply mounting changes.");
        body->LoadControlSettings("Resource/ContentSubGames.res");
        note->SetText("Restart the game to apply mounting changes.");
        m_Status = new Label(this, "Status", "");
        m_Status->SetBounds(16, 340, 408, 20);
        Button* apply = new Button(this, "Apply", "Apply", this, "ApplyMounts");
        apply->SetBounds(248, 365, 82, 24);
        Button* cancel = new Button(this, "Cancel", "Cancel", this, "CancelMounts");
        cancel->SetBounds(338, 365, 82, 24);
    }
    void Open()
    {
        KeyValues* kv = LoadGameInfo();
        KeyValues* paths = SearchPaths(kv);
        const bool hadPaths = paths != NULL;
        for (int i = 0; i < ARRAYSIZE(g_Games); ++i)
        {
            char key[128], folder[MAX_PATH]; MountKey(i, key, sizeof(key));
            char legacyKey[128];
            Q_snprintf(legacyKey, sizeof(legacyKey), "%s%s", kMountPrefix, g_Games[i].field);
            bool selected = paths && (paths->FindKey(key) || paths->FindKey(legacyKey));
            bool available = ResolveGame(g_Games[i], folder, sizeof(folder));
            m_Check[i]->SetSelected(selected);
            m_Check[i]->SetEnabled(available || selected);
            char label[160];
            Q_snprintf(label, sizeof(label), "%s%s", g_Games[i].title,
                available ? "" : " (not installed)");
            m_Check[i]->SetText(label);
        }
        if (kv) kv->deleteThis();
        m_Status->SetText(hadPaths ? "Choose installed games." : "Cannot read gameinfo.txt.");
        MoveToCenterOfScreen();
        SetVisible(true); MakePopup(); Activate();
        SetMouseInputEnabled(true); SetKeyBoardInputEnabled(true);
    }
    virtual void OnCommand(const char* cmd)
    {
        if (!Q_stricmp(cmd, "ApplyMounts")) { ApplyMounts(); return; }
        if (!Q_stricmp(cmd, "CancelMounts")) { Close(); return; }
        BaseClass::OnCommand(cmd);
    }
private:
    void ApplyMounts()
    {
        KeyValues* kv = LoadGameInfo();
        KeyValues* paths = SearchPaths(kv);
        if (!paths)
        {
            if (kv) kv->deleteThis();
            m_Status->SetText("Cannot read gameinfo search paths."); return;
        }
        char folders[ARRAYSIZE(g_Games)][MAX_PATH];
        for (int i = 0; i < ARRAYSIZE(g_Games); ++i)
        {
            folders[i][0] = 0;
            if (m_Check[i]->IsSelected() && !ResolveGame(g_Games[i], folders[i], sizeof(folders[i])))
            {
                kv->deleteThis();
                m_Status->SetText("Deselect unavailable games before applying."); return;
            }
        }
        for (KeyValues* p = paths->GetFirstSubKey(); p; )
        {
            KeyValues* next = p->GetNextKey();
            if (!Q_strnicmp(p->GetName(), kMountPrefix, Q_strlen(kMountPrefix)))
            {
                paths->RemoveSubKey(p); p->deleteThis();
            }
            p = next;
        }
        for (int i = 0; i < ARRAYSIZE(g_Games); ++i)
        {
            if (!folders[i][0]) continue;
            char key[128]; MountKey(i, key, sizeof(key));
            // Mount archive index files, never numbered VPK chunks.
            char wildcard[MAX_PATH];
            Q_snprintf(wildcard, sizeof(wildcard), "%s/*_dir.vpk", folders[i]);
            FileFindHandle_t handle;
            const char* found = filesystem->FindFirst(wildcard, &handle);
            if (found)
            {
                do
                {
                    if (filesystem->FindIsDirectory(handle)) continue;
                    char full[MAX_PATH];
                    Q_snprintf(full, sizeof(full), "%s/%s", folders[i], found);
                    AddPath(paths, key, full);
                } while ((found = filesystem->FindNext(handle)) != NULL);
                filesystem->FindClose(handle);
            }
            // Loose content, at lower priority than the mod's existing paths.
            AddPath(paths, key, folders[i]);
        }
        char path[MAX_PATH], backup[MAX_PATH], temp[MAX_PATH];
        GameInfoPath(path, sizeof(path));
        Q_snprintf(backup, sizeof(backup), "%s.gabe_backup", path);
        Q_snprintf(temp, sizeof(temp), "%s.gabe_tmp", path);
        CUtlBuffer original;
        if (!filesystem->ReadFile(path, NULL, original) ||
            (!filesystem->FileExists(backup) && !filesystem->WriteFile(backup, NULL, original)))
        {
            kv->deleteThis(); m_Status->SetText("Backup failed; nothing changed."); return;
        }
        bool saved = kv->SaveToFile(filesystem, temp, NULL);
        kv->deleteThis();
        if (!saved) { m_Status->SetText("Could not save mounting settings."); return; }
        CUtlBuffer updated;
        if (!filesystem->ReadFile(temp, NULL, updated) || !filesystem->WriteFile(path, NULL, updated))
        {
            original.SeekGet(CUtlBuffer::SEEK_HEAD, 0);
            bool restored = filesystem->WriteFile(path, NULL, original);
            m_Status->SetText(restored ? "Write failed; original restored." : "Write failed; restore .gabe_backup manually.");
            filesystem->RemoveFile(temp, NULL); return;
        }
        filesystem->RemoveFile(temp, NULL);
        m_Status->SetText("Saved. Restart the game to mount content.");
    }
    CheckButton* m_Check[ARRAYSIZE(g_Games)];
    Label* m_Status;
};
static CGabeGameMounter* g_pMounter = NULL;
CON_COMMAND(gabe_mountgames, "Open the game content mounter")
{
    if (!g_pMounter)
        g_pMounter = new CGabeGameMounter(enginevgui->GetPanel(PANEL_GAMEUIDLL));
    g_pMounter->Open();
}
