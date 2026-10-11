/*
* Copyright Weapons Expanded / Gabe Mod
* Purpose: Web browser in the Source Engine (Funny, isn't it?)
*/
#include "cbase.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/HTML.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/TextEntry.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "ienginevgui.h"
#include "filesystem.h"
#include "keyvalues.h"

class CWebBrowserPanel : public vgui::Frame
{
    DECLARE_CLASS_SIMPLE(CWebBrowserPanel, vgui::Frame);

public:
    CWebBrowserPanel(vgui::VPANEL parent);
    ~CWebBrowserPanel() {}

    void ShowPanel(bool bShow);
    void PerformLayout() OVERRIDE;  // Override PerformLayout to handle resizing

protected:
    vgui::HTML* m_pHTML;
    vgui::TextEntry* m_pURLBar;

    vgui::Button* m_pGoButton;
    vgui::Button* m_pBackButton;
    vgui::Button* m_pForwardButton;
    vgui::Button* m_pCloseButton;
    vgui::Button* m_pBookmarkButton;
    vgui::Button* m_pRefreshButton;
    vgui::Button* m_pHomeButton;

    CUtlVector<KeyValues*> m_History;  // Vector to store history of visited URLs

    void OnCommand(const char* command) OVERRIDE;
    void BookmarkCurrentPage(); // Function to bookmark the current page
    void SaveHistory(); // Function to save the history of visited URLs
};

CWebBrowserPanel::CWebBrowserPanel(vgui::VPANEL parent) : BaseClass(nullptr, "WebBrowserPanel")
{
    SetParent(parent);
    SetTitle("CITY17 Web Browser", true);
    SetSize(1280, 720);  // Set panel size to 1280x720
    SetPos((ScreenWidth() - 1280) / 2, (ScreenHeight() - 720) / 2);  // Center the panel on the screen
    SetMoveable(true);
    SetSizeable(true);  // Allow resizing
    SetVisible(false);  // Start hidden

    // Create the HTML component and make it interactive
    m_pHTML = new vgui::HTML(this, "WebBrowserHTML");
    m_pHTML->SetMouseInputEnabled(true);  // Enable mouse interaction
    m_pHTML->SetKeyBoardInputEnabled(true);  // Enable keyboard interaction

    // Create the URL bar
    m_pURLBar = new vgui::TextEntry(this, "URLBar");
    m_pURLBar->SetText("https://www.moddb.com/mods/weapons-expanded/downloads/weapons-expanded-beta-081");  // Default URL
    m_pURLBar->SendNewLine(true);  // Allow Enter key to trigger navigation

    // Create navigation buttons
    m_pGoButton = new vgui::Button(this, "GoButton", "Go", this, "go");
    m_pBackButton = new vgui::Button(this, "BackButton", "Back", this, "back");
    m_pForwardButton = new vgui::Button(this, "ForwardButton", "Forward", this, "forward");
    m_pCloseButton = new vgui::Button(this, "CloseButton", "Close", this, "close");
    m_pBookmarkButton = new vgui::Button(this, "BookmarkButton", "Bookmark", this, "bookmark");
    m_pRefreshButton = new vgui::Button(this, "RefreshButton", "Refresh", this, "refresh");
    m_pHomeButton = new vgui::Button(this, "HomeButton", "Home", this, "home");
}

void CWebBrowserPanel::PerformLayout()
{
    BaseClass::PerformLayout();

    // Resize and reposition the controls based on the new size of the panel
    int panelWide, panelTall;
    GetSize(panelWide, panelTall);

    m_pURLBar->SetBounds(10, 30, panelWide - 370, 25);  // Adjust the URL bar size based on panel width
    m_pHTML->SetBounds(10, 60, panelWide - 20, panelTall - 100);  // Adjust HTML area

    m_pHTML->InvalidateLayout();  // Ensure HTML panel resizes correctly

    int buttonWidth = 60;
    int buttonHeight = 25;
    int buttonX = panelWide - 350;
    int buttonY = 30;

    // Update the positions of the buttons
    m_pGoButton->SetBounds(buttonX, buttonY, buttonWidth, buttonHeight);
    m_pBackButton->SetBounds(buttonX + buttonWidth + 10, buttonY, buttonWidth, buttonHeight);
    m_pForwardButton->SetBounds(buttonX + 2 * (buttonWidth + 10), buttonY, buttonWidth, buttonHeight);
    m_pBookmarkButton->SetBounds(buttonX + 3 * (buttonWidth + 10), buttonY, buttonWidth, buttonHeight);
    m_pRefreshButton->SetBounds(buttonX + 4 * (buttonWidth + 10), buttonY, buttonWidth, buttonHeight);
    m_pHomeButton->SetBounds(buttonX + 5 * (buttonWidth + 10), buttonY, buttonWidth, buttonHeight);
    m_pCloseButton->SetBounds(panelWide - 90, panelTall - 35, 80, 25);  // Close button stays at the bottom-right corner
}

void CWebBrowserPanel::ShowPanel(bool bShow)
{
    if (bShow)
    {
        Activate();
        SetVisible(true);
        SetMouseInputEnabled(true);
        SetKeyBoardInputEnabled(true);
        MoveToFront();
    }
    else
    {
        SetVisible(false);
        SetMouseInputEnabled(false);
        SetKeyBoardInputEnabled(false);
    }
}

void CWebBrowserPanel::OnCommand(const char* command)
{
    if (!Q_stricmp(command, "go"))
    {
        char url[512];
        m_pURLBar->GetText(url, sizeof(url));
        m_pHTML->OpenURL(url, nullptr);
        SaveHistory();  // Save the visited URL to history
    }
    else if (!Q_stricmp(command, "back"))
    {
        m_pHTML->GoBack();
    }
    else if (!Q_stricmp(command, "forward"))
    {
        m_pHTML->GoForward();
    }
    else if (!Q_stricmp(command, "bookmark"))
    {
        BookmarkCurrentPage();
    }
    else if (!Q_stricmp(command, "refresh"))
    {
        m_pHTML->Refresh();
    }
    else if (!Q_stricmp(command, "home"))
    {
        m_pHTML->OpenURL("https://www.moddb.com/mods/weapons-expanded/downloads/weapons-expanded-beta-081", nullptr);
    }
    else if (!Q_stricmp(command, "close"))
    {
        ShowPanel(false);
    }
    else
    {
        BaseClass::OnCommand(command);
    }
}

void CWebBrowserPanel::BookmarkCurrentPage()
{
    char url[512];
    m_pURLBar->GetText(url, sizeof(url));

    // Save the bookmark to a file
    FileHandle_t file = filesystem->Open("bookmarks.txt", "a+", "MOD");
    if (file != FILESYSTEM_INVALID_HANDLE)
    {
        filesystem->FPrintf(file, "%s\n", url);
        filesystem->Close(file);
    }
}

void CWebBrowserPanel::SaveHistory()
{
    char url[512];
    m_pURLBar->GetText(url, sizeof(url));

    KeyValues* kv = new KeyValues("History");
    kv->SetString("URL", url);
    m_History.AddToTail(kv);

    // Optionally, you could also save this to a file
    FileHandle_t file = filesystem->Open("history.txt", "a+", "MOD");
    if (file != FILESYSTEM_INVALID_HANDLE)
    {
        filesystem->FPrintf(file, "%s\n", url);
        filesystem->Close(file);
    }
}

// Global pointer to the Web Browser panel
static CWebBrowserPanel* g_pWebBrowserPanel = nullptr;

// Function to show the Web Browser panel
void ShowWebBrowser()
{
    if (!g_pWebBrowserPanel)
    {
        g_pWebBrowserPanel = new CWebBrowserPanel(enginevgui->GetPanel(PANEL_GAMEUIDLL));
    }

    g_pWebBrowserPanel->ShowPanel(true);
}

// Function to hide the Web Browser panel
void HideWebBrowser()
{
    if (g_pWebBrowserPanel)
    {
        g_pWebBrowserPanel->ShowPanel(false);
    }
}

// Console command to show the Web Browser panel
static ConCommand web_browser_show("gabe_showbrowser", ShowWebBrowser, "Shows City Browser", FCVAR_CLIENTDLL);

// Console command to hide the Web Browser panel
static ConCommand web_browser_hide("gabe_hidebrowser", HideWebBrowser, "Hides City Browser", FCVAR_CLIENTDLL);
