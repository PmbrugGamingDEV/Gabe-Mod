/*
 * Copyright Weapons Expanded / Gabe Mod
 * Purpose: CITY17 Web Browser - classic VGUI interface
 */
#include "cbase.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/HTML.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/TextEntry.h"
#include "vgui_controls/Label.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "ienginevgui.h"
#include "filesystem.h"
#include "tier0/memdbgon.h"

using namespace vgui;
static const char* kBrowserHome = "https://www.moddb.com/mods/weapons-expanded/downloads/weapons-expanded-beta-081";

class CBrowserButton : public Button
{
    DECLARE_CLASS_SIMPLE(CBrowserButton, Button);
public:
    CBrowserButton(Panel* parent, const char* name, const char* text, const char* command)
        : BaseClass(parent, name, text, parent, command) {}
    void ApplySchemeSettings(IScheme* scheme) OVERRIDE
    {
        BaseClass::ApplySchemeSettings(scheme);
        SetPaintBorderEnabled(false);
        const Color white(240, 240, 240, 255);
        SetDefaultColor(white, white);
        SetArmedColor(white, white);
        SetDepressedColor(white, white);
    }
    void PaintBackground() OVERRIDE
    {
        int shade = IsDepressed() ? 65 : IsArmed() ? 125 : 96;
        surface()->DrawSetColor(shade, shade, shade, 255);
        surface()->DrawFilledRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(155, 155, 155, 255);
        surface()->DrawOutlinedRect(0, 0, GetWide(), GetTall());
        surface()->DrawSetColor(62, 62, 62, 255);
        surface()->DrawFilledRect(1, GetTall() - 2, GetWide() - 1, GetTall() - 1);
    }
};

class CBrowserAddress : public TextEntry
{
    DECLARE_CLASS_SIMPLE(CBrowserAddress, TextEntry);
public:
    CBrowserAddress(Panel* parent) : BaseClass(parent, "URLBar"), m_pBrowser(parent) {}
    void OnKeyCodeTyped(KeyCode code) OVERRIDE
    {
        if (code == KEY_ENTER || code == KEY_PAD_ENTER)
        {
            m_pBrowser->OnCommand("go");
            return;
        }
        BaseClass::OnKeyCodeTyped(code);
    }
private:
    Panel* m_pBrowser;
};

class CWebBrowserPanel : public Frame
{
    DECLARE_CLASS_SIMPLE(CWebBrowserPanel, Frame);
public:
    CWebBrowserPanel(VPANEL parent);
    void ShowPanel(bool show);
    void PerformLayout() OVERRIDE;
    void OnClose() OVERRIDE { ShowPanel(false); }
    void ApplySchemeSettings(IScheme* scheme) OVERRIDE;
    void PaintBackground() OVERRIDE;
    void OnCommand(const char* command) OVERRIDE;
private:
    void Navigate(const char* url);
    void BookmarkCurrentPage();
    void AppendURL(const char* file, const char* url);
    HTML* m_pHTML;
    TextEntry* m_pURLBar;
    Label* m_pAddressLabel, * m_pStatus, * m_pBrand;
    CBrowserButton* m_pBack, * m_pForward, * m_pRefresh, * m_pHome, * m_pGo, * m_pBookmark, * m_pClose;
    bool m_bStarted;
};

CWebBrowserPanel::CWebBrowserPanel(VPANEL parent) : BaseClass(NULL, "WebBrowserPanel")
{
    SetParent(parent);
    SetTitle("CITY17 - Web Browser", true);
    SetSizeable(true);
    SetMoveable(true);
    SetMinimumSize(620, 360);
    SetDeleteSelfOnClose(false);
    SetProportional(false);
    SetPaintBackgroundEnabled(true);
    SetVisible(false);
    m_bStarted = false;
    m_pHTML = new HTML(this, "WebBrowserHTML");
    m_pHTML->SetMouseInputEnabled(true);
    m_pHTML->SetKeyBoardInputEnabled(true);
    m_pURLBar = new CBrowserAddress(this);
    m_pURLBar->SetText(kBrowserHome);
    m_pAddressLabel = new Label(this, "AddressLabel", "Address:");
    m_pBrand = new Label(this, "BrowserBrand", "CITY17 / INTERNET");
    m_pStatus = new Label(this, "BrowserStatus", "Ready");
    m_pBack = new CBrowserButton(this, "Back", "< Back", "back");
    m_pForward = new CBrowserButton(this, "Forward", "Forward >", "forward");
    m_pRefresh = new CBrowserButton(this, "Refresh", "Refresh", "refresh");
    m_pHome = new CBrowserButton(this, "Home", "Home", "home");
    m_pGo = new CBrowserButton(this, "Go", "Go", "go");
    m_pBookmark = new CBrowserButton(this, "Bookmark", "Bookmark", "bookmark");
    m_pClose = new CBrowserButton(this, "Close", "Close", "close");
    int sw, sh; surface()->GetScreenSize(sw, sh);
    int w = MIN(1280, MAX(620, sw - 40)), h = MIN(720, MAX(360, sh - 40));
    SetSize(w, h); SetPos(MAX(0, (sw - w) / 2), MAX(0, (sh - h) / 2));
}

void CWebBrowserPanel::ApplySchemeSettings(IScheme* scheme)
{
    BaseClass::ApplySchemeSettings(scheme);
    const Color fg(235, 235, 235, 255);
    m_pAddressLabel->SetFgColor(fg);
    m_pBrand->SetFgColor(Color(190, 195, 200, 255));
    m_pStatus->SetFgColor(fg);
    m_pURLBar->SetFgColor(Color(25, 25, 25, 255));
    m_pURLBar->SetBgColor(Color(225, 225, 225, 255));
}

void CWebBrowserPanel::PaintBackground()
{
    int w = GetWide(), h = GetTall();
    surface()->DrawSetColor(58, 58, 58, 255);
    surface()->DrawFilledRect(0, 0, w, h);
    surface()->DrawSetColor(79, 79, 79, 255);
    surface()->DrawFilledRect(5, 28, w - 5, 91);
    surface()->DrawSetColor(145, 145, 145, 255);
    surface()->DrawOutlinedRect(5, 28, w - 5, 91);
    surface()->DrawSetColor(24, 24, 24, 255);
    surface()->DrawFilledRect(5, 94, w - 5, h - 35);
    surface()->DrawSetColor(85, 85, 85, 255);
    surface()->DrawFilledRect(5, h - 31, w - 5, h - 5);
    surface()->DrawSetColor(135, 135, 135, 255);
    surface()->DrawOutlinedRect(5, h - 31, w - 5, h - 5);
}

void CWebBrowserPanel::PerformLayout()
{
    BaseClass::PerformLayout();
    int w = GetWide(), h = GetTall();
    m_pBack->SetBounds(10, 33, 65, 25);
    m_pForward->SetBounds(79, 33, 82, 25);
    m_pRefresh->SetBounds(165, 33, 70, 25);
    m_pHome->SetBounds(239, 33, 60, 25);
    m_pBookmark->SetBounds(303, 33, 88, 25);
    m_pBrand->SetBounds(400, 33, MAX(1, w - 410), 25);
    m_pAddressLabel->SetBounds(10, 63, 58, 24);
    m_pURLBar->SetBounds(70, 63, MAX(1, w - 132), 24);
    m_pGo->SetBounds(w - 58, 63, 48, 24);
    m_pHTML->SetBounds(8, 97, MAX(1, w - 16), MAX(1, h - 135));
    m_pStatus->SetBounds(12, h - 29, MAX(1, w - 100), 22);
    m_pClose->SetBounds(w - 78, h - 29, 68, 22);
}

void CWebBrowserPanel::ShowPanel(bool show)
{
    SetVisible(show);
    SetMouseInputEnabled(show);
    SetKeyBoardInputEnabled(show);
    if (!show) return;
    Activate(); MoveToFront();
    if (!m_bStarted) { m_bStarted = true; Navigate(kBrowserHome); }
}

void CWebBrowserPanel::AppendURL(const char* name, const char* url)
{
    FileHandle_t file = filesystem->Open(name, "a", "MOD");
    if (file == FILESYSTEM_INVALID_HANDLE) { m_pStatus->SetText("Unable to save URL."); return; }
    filesystem->FPrintf(file, "%s\n", url);
    filesystem->Close(file);
}

void CWebBrowserPanel::Navigate(const char* input)
{
    if (!input || !*input) return;
    char url[2048];
    if (strchr(input, '\n') || strchr(input, '\r')) return;
    if (!Q_strnicmp(input, "https://", 8) || !Q_strnicmp(input, "http://", 7))
        Q_strncpy(url, input, sizeof(url));
    else if (!Q_strstr(input, "://") && !strchr(input, ':') && !strchr(input, ' '))
        Q_snprintf(url, sizeof(url), "https://%s", input);
    else { m_pStatus->SetText("Enter an HTTP or HTTPS address."); return; }
    m_pURLBar->SetText(url);
    m_pHTML->OpenURL(url, NULL);
    m_pStatus->SetText("Navigation requested");
    AppendURL("history.txt", url);
}

void CWebBrowserPanel::BookmarkCurrentPage()
{
    char url[2048]; m_pURLBar->GetText(url, sizeof(url));
    if (Q_strnicmp(url, "https://", 8) && Q_strnicmp(url, "http://", 7)) return;
    if (strchr(url, '\r') || strchr(url, '\n')) return;
    m_pStatus->SetText("Address bookmarked");
    AppendURL("bookmarks.txt", url);
}

void CWebBrowserPanel::OnCommand(const char* command)
{
    if (!Q_stricmp(command, "go")) { char url[2048]; m_pURLBar->GetText(url, sizeof(url)); Navigate(url); }
    else if (!Q_stricmp(command, "back")) { m_pHTML->GoBack(); m_pStatus->SetText("Back requested"); }
    else if (!Q_stricmp(command, "forward")) { m_pHTML->GoForward(); m_pStatus->SetText("Forward requested"); }
    else if (!Q_stricmp(command, "refresh")) { m_pHTML->Refresh(); m_pStatus->SetText("Refresh requested"); }
    else if (!Q_stricmp(command, "home")) Navigate(kBrowserHome);
    else if (!Q_stricmp(command, "bookmark")) BookmarkCurrentPage();
    else if (!Q_stricmp(command, "close")) ShowPanel(false);
    else BaseClass::OnCommand(command);
}

static CWebBrowserPanel* g_pWebBrowserPanel = NULL;
void ShowWebBrowser()
{
    if (!g_pWebBrowserPanel)
    {
        VPANEL root = enginevgui->GetPanel(PANEL_GAMEUIDLL);
        if (!root) return;
        g_pWebBrowserPanel = new CWebBrowserPanel(root);
    }
    g_pWebBrowserPanel->ShowPanel(true);
}
void HideWebBrowser() { if (g_pWebBrowserPanel) g_pWebBrowserPanel->ShowPanel(false); }
static ConCommand web_browser_show("gabe_showbrowser", ShowWebBrowser, "Shows City Browser", FCVAR_CLIENTDLL);
static ConCommand web_browser_hide("gabe_hidebrowser", HideWebBrowser, "Hides City Browser", FCVAR_CLIENTDLL);
