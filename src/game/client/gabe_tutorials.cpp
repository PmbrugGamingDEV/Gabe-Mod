#include "cbase.h"

#include <vgui_controls/Frame.h>
#include <vgui_controls/PropertySheet.h>
#include <vgui_controls/HTML.h>

#include "ienginevgui.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "vgui_int.h"
#include "filesystem.h"

using namespace vgui;

//------------------------------------------------------------
// Globals
//------------------------------------------------------------
class CGabeTutorialPanel;
static CGabeTutorialPanel* g_pTutorialPanel = NULL;

//------------------------------------------------------------
// Tutorial Panel
//------------------------------------------------------------
class CGabeTutorialPanel : public Frame
{
	DECLARE_CLASS_SIMPLE(CGabeTutorialPanel, Frame);

public:
	CGabeTutorialPanel(VPANEL parent) : BaseClass(NULL, "GabeTutorialPanel")
	{
		SetParent(parent);
		SetTitle("GABE MOD - Tutorials", true);

		int sw, sh;
		surface()->GetScreenSize(sw, sh);

		int w = (int)(sw * 0.75f);
		int h = (int)(sh * 0.8f);

		SetSize(w, h);
		SetPos((sw - w) / 2, (sh - h) / 2);

		SetMoveable(true);
		SetSizeable(false);
		SetCloseButtonVisible(true);
		SetMinimizeButtonVisible(false);
		SetMaximizeButtonVisible(false);
		SetDeleteSelfOnClose(true);

		m_pTabs = new PropertySheet(this, "TutorialTabs");
		m_pTabs->SetBounds(10, 35, w - 20, h - 45);

		AddTabs();

		MakePopup();
		MoveToFront();
		Activate();
		RequestFocus();
	}

	virtual void OnClose()
	{
		BaseClass::OnClose();
		g_pTutorialPanel = NULL;
	}

	virtual void OnKeyCodePressed(KeyCode code)
	{
		if (code == KEY_ESCAPE)
		{
			Close();
			return;
		}

		BaseClass::OnKeyCodePressed(code);
	}

private:
	//--------------------------------------------------------
	// Adds one HTML tab
	//--------------------------------------------------------
	void AddHTMLTab(const char* tabName, const char* htmlFile)
	{
		HTML* html = new HTML(m_pTabs, tabName);

		// Correct layout handling
		html->SetBounds(0, 0,
			m_pTabs->GetWide(),
			m_pTabs->GetTall() - 25);


		char path[MAX_PATH];

		// Load requested file if it exists
		if (filesystem->FileExists(htmlFile, "GAME"))
		{
			Q_snprintf(path, sizeof(path),
				"file://%s/%s",
				engine->GetGameDirectory(),
				htmlFile);
		}
		else
		{
			// Guaranteed fallback
			Q_snprintf(path, sizeof(path),
				"file://%s/resource/ui/html/tutorial_missing.html",
				engine->GetGameDirectory());
		}

		html->OpenURL(path, NULL);
		m_pTabs->AddPage(html, tabName);
	}


	//--------------------------------------------------------
	// Add all tutorial tabs here
	//--------------------------------------------------------
	void AddTabs()
	{
		AddHTMLTab("Basics", "resource/ui/html/tutorial_basics.html");
		AddHTMLTab("Weapons", "resource/ui/html/tutorial_weapons.html");
		AddHTMLTab("Physics", "resource/ui/html/tutorial_physics.html");
		AddHTMLTab("Building", "resource/ui/html/tutorial_building.html");
		AddHTMLTab("Tips", "resource/ui/html/tutorial_tips.html");
	}

private:
	PropertySheet* m_pTabs;
};

//------------------------------------------------------------
// Show panel
//------------------------------------------------------------
void ShowGabeTutorialPanel()
{
	if (g_pTutorialPanel)
		return;

	g_pTutorialPanel = new CGabeTutorialPanel(
		enginevgui->GetPanel(PANEL_GAMEUIDLL));
}

//------------------------------------------------------------
// Console command
//------------------------------------------------------------
CON_COMMAND_F(gabe_tutorial, "Shows the Gabe Mod tutorial panel.", FCVAR_CLIENTDLL)
{
	ShowGabeTutorialPanel();
}
