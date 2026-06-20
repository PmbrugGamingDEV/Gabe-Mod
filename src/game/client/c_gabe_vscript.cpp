#include "cbase.h"
#include "vscript_client.h"
#include "c_gabe_vscript.h"

#include "icommandline.h"
#include "filesystem.h"
#include "tier1/fmtstr.h"
#include "tier1/utlbuffer.h"
#include "../../public/vscript/vscript_templates.h"

#include "vgui_all.h"
#include "vgui_controls/ToolTip.h"
#include "vgui_controls/Controls.h"
#include "vgui/IPanel.h"

#include "mathlib/mathlib.h"
#include "convar.h"

#include "iclientmode.h"

#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static IScriptManager* s_pScriptManager = NULL;
static CreateInterfaceFn s_AppSystemFactory = NULL;
static FileFindHandle_t g_ScriptFindHandle = FILESYSTEM_INVALID_FIND_HANDLE;

IScriptVM* g_pScriptVM = NULL;

using vgui::VPANEL;

//-----------------------------------------------------------------------------
// Factory setter
//-----------------------------------------------------------------------------

void VScriptClientSetFactory(CreateInterfaceFn factory)
{
	s_AppSystemFactory = factory;
}

static vgui::Panel* GetScriptHUDParent()
{
	VPANEL root = enginevgui->GetPanel(PANEL_CLIENTDLL);
	if (!root)
		return NULL;

	static vgui::Panel* s_pHudRoot = NULL;

	if (!s_pHudRoot)
	{
		s_pHudRoot = new vgui::Panel(NULL, "ScriptHUDRoot");
		s_pHudRoot->SetParent(root);
		s_pHudRoot->SetBounds(0, 0, ScreenWidth(), ScreenHeight());
		s_pHudRoot->SetVisible(true);
		s_pHudRoot->SetMouseInputEnabled(false);
		s_pHudRoot->SetKeyBoardInputEnabled(false);
	}

	return s_pHudRoot;
}

//-----------------------------------------------------------------------------
// Client script functions
//-----------------------------------------------------------------------------


class CScriptVPanel;

static HSCRIPT Script_VGUI_CreatePanel(CScriptVPanel* pParent, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateLabel(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateTextEntry(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateCheckButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateComboBox(CScriptVPanel* pParent, int x, int y, int w, int h, int maxItems);
static HSCRIPT Script_VGUI_CreateSlider(CScriptVPanel* pParent, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateProgressBar(CScriptVPanel* pParent, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateImagePanel(CScriptVPanel* pParent, const char* image, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateRichText(CScriptVPanel* pParent, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateRadioButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateMenu(CScriptVPanel* pParent);
static HSCRIPT Script_VGUI_CreateMenuButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateListPanel(CScriptVPanel* pParent, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateSectionedListPanel(CScriptVPanel* pParent, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateURLLabel(CScriptVPanel* pParent, const char* text, const char* url, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreateDivider(CScriptVPanel* pParent, int x, int y, int w, int h);
static HSCRIPT Script_VGUI_CreatePropertySheet(CScriptVPanel* pParent, int x, int y, int w, int h);

static void Script_VGUI_SetText(CScriptVPanel* pPanel, const char* text);
static const char* Script_VGUI_GetText(CScriptVPanel* pPanel);
static void Script_VGUI_SetChecked(CScriptVPanel* pPanel, bool bChecked);
static bool Script_VGUI_IsChecked(CScriptVPanel* pPanel);
static void Script_VGUI_ComboAddItem(CScriptVPanel* pPanel, const char* text);
static void Script_VGUI_ComboActivateItem(CScriptVPanel* pPanel, int itemID);
static int Script_VGUI_ComboGetActiveItem(CScriptVPanel* pPanel);
static void Script_VGUI_SetSliderRange(CScriptVPanel* pPanel, int min, int max);
static void Script_VGUI_SetSliderValue(CScriptVPanel* pPanel, int value);
static int Script_VGUI_GetSliderValue(CScriptVPanel* pPanel);
static void Script_VGUI_SetProgress(CScriptVPanel* pPanel, float progress);
static void Script_VGUI_RichTextInsert(CScriptVPanel* pPanel, const char* text);
static void Script_VGUI_RichTextClear(CScriptVPanel* pPanel);
static bool Script_VGUI_IsVisible(CScriptVPanel* pPanel);
static bool Script_VGUI_IsEnabled(CScriptVPanel* pPanel);
static void Script_VGUI_SetAlpha(CScriptVPanel* pPanel, int a);
static int Script_VGUI_GetAlpha(CScriptVPanel* pPanel);
static void Script_VGUI_SetZPos(CScriptVPanel* pPanel, int z);
static int Script_VGUI_GetZPos(CScriptVPanel* pPanel);
static void Script_VGUI_SetMouseInputEnabled(CScriptVPanel* pPanel, bool bEnabled);
static void Script_VGUI_SetKeyboardInputEnabled(CScriptVPanel* pPanel, bool bEnabled);
static void Script_VGUI_SetPaintBackgroundEnabled(CScriptVPanel* pPanel, bool bEnabled);
static void Script_VGUI_SetPaintBorderEnabled(CScriptVPanel* pPanel, bool bEnabled);
static void Script_VGUI_CenterOnScreen(CScriptVPanel* pPanel);
static void Script_VGUI_SetImage(CScriptVPanel* pPanel, const char* image);
static void Script_VGUI_SetEditable(CScriptVPanel* pPanel, bool bEditable);
static void Script_VGUI_SelectAllText(CScriptVPanel* pPanel);

static HSCRIPT Script_VGUI_CreateTreeView(CScriptVPanel* pParent, int x, int y, int w, int h);

static int Script_VGUI_TreeAddItem(CScriptVPanel* pPanel, int parentItem, const char* text);
static void Script_VGUI_TreeRemoveItem(CScriptVPanel* pPanel, int item);
static void Script_VGUI_TreeRemoveAll(CScriptVPanel* pPanel);
static int Script_VGUI_TreeGetSelectedItem(CScriptVPanel* pPanel);
static void Script_VGUI_TreeExpandItem(CScriptVPanel* pPanel, int item, bool expand);

static const char* Script_FS_FindFirst(const char* wildcard);
static const char* Script_FS_FindNext();
static void Script_FS_FindClose();
static bool Script_FS_IsDirectory(const char* path);

class CScripted_HudElement : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE(CScripted_HudElement, vgui::Panel);

public:
	CScripted_HudElement(const char* pElementName)
		: vgui::Panel(g_pClientMode->GetViewport(), pElementName),
		CHudElement(pElementName)
	{
		SetBounds(0, 0, ScreenWidth(), ScreenHeight());
		SetVisible(true);
		SetPaintBackgroundEnabled(false);
		SetMouseInputEnabled(false);
		SetKeyBoardInputEnabled(false);

		m_hFont = vgui::INVALID_FONT;
		m_Text[0] = 0;
		m_TextColor = Color(255, 255, 255, 255);
		m_nTextX = 20;
		m_nTextY = 20;
	}

	virtual void ApplySchemeSettings(vgui::IScheme* pScheme)
	{
		BaseClass::ApplySchemeSettings(pScheme);
		m_hFont = pScheme->GetFont("Default", true);
	}

	virtual bool ShouldDraw()
	{
		return CHudElement::ShouldDraw();
	}

	virtual void Paint()
	{
		BaseClass::Paint();

		if (!m_Text[0] || m_hFont == vgui::INVALID_FONT)
			return;

		wchar_t wszText[1024];
		g_pVGuiLocalize->ConvertANSIToUnicode(m_Text, wszText, sizeof(wszText));

		vgui::surface()->DrawSetTextFont(m_hFont);
		vgui::surface()->DrawSetTextColor(m_TextColor);
		vgui::surface()->DrawSetTextPos(m_nTextX, m_nTextY);
		vgui::surface()->DrawPrintText(wszText, wcslen(wszText));
	}

	void SetHUDText(const char* text)
	{
		Q_strncpy(m_Text, text ? text : "", sizeof(m_Text));
	}

	void SetHUDTextPos(int x, int y)
	{
		m_nTextX = x;
		m_nTextY = y;
	}

	void SetHUDTextColor(int r, int g, int b, int a)
	{
		m_TextColor = Color(r, g, b, a);
	}

	void SetHUDVisible(bool bVisible)
	{
		SetVisible(bVisible);
		SetActive(bVisible);
	}

	bool IsHUDVisible()
	{
		return IsVisible();
	}

private:
	char m_Text[1024];
	Color m_TextColor;
	vgui::HFont m_hFont;
	int m_nTextX;
	int m_nTextY;
};

BEGIN_SCRIPTDESC_ROOT(CScripted_HudElement, "Scripted HUD element")
DEFINE_SCRIPTFUNC(SetHUDText, "Sets HUD text")
DEFINE_SCRIPTFUNC(SetHUDTextPos, "Sets HUD text position")
DEFINE_SCRIPTFUNC(SetHUDTextColor, "Sets HUD text color")
DEFINE_SCRIPTFUNC(SetHUDVisible, "Sets HUD visible/active")
DEFINE_SCRIPTFUNC(IsHUDVisible, "Returns true if HUD visible")
END_SCRIPTDESC();

static HSCRIPT Script_HUD_CreateElement(const char* name)
{
	if (!g_pScriptVM)
		return INVALID_HSCRIPT;

	CScripted_HudElement* pHud =
		new CScripted_HudElement(name ? name : "Scripted_HudElement");

	return g_pScriptVM->RegisterInstance(
		GetScriptDescForClass(CScripted_HudElement),
		pHud
	);
}

class CScriptVButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE(CScriptVButton, vgui::Button);

public:
	CScriptVButton(vgui::Panel* parent, const char* panelName, const char* text)
		: BaseClass(parent, panelName, text)
	{
		m_szCommand[0] = 0;
	}

	void SetScriptCommand(const char* pszCommand)
	{
		Q_strncpy(m_szCommand, pszCommand ? pszCommand : "", sizeof(m_szCommand));
	}

	virtual void DoClick()
	{
		BaseClass::DoClick();

		if (m_szCommand[0] && g_pScriptVM)
		{
			g_pScriptVM->Run(m_szCommand, true);
		}
	}

private:
	char m_szCommand[1024];
};

class CScriptVPanel
{
public:
	CScriptVPanel()
	{
		m_pPanel = NULL;
	}

	CScriptVPanel(vgui::Panel* pPanel)
	{
		m_pPanel = pPanel;
	}

	~CScriptVPanel()
	{}

	bool IsValid()
	{
		return m_pPanel != NULL;
	}

	void Delete()
	{
		if (m_pPanel)
		{
			m_pPanel->MarkForDeletion();
			m_pPanel = NULL;
		}
	}

	void SetVisible(bool bVisible)
	{
		if (m_pPanel)
			m_pPanel->SetVisible(bVisible);
	}

	void SetEnabled(bool bEnabled)
	{
		if (m_pPanel)
			m_pPanel->SetEnabled(bEnabled);
	}

	void SetPos(int x, int y)
	{
		if (m_pPanel)
			m_pPanel->SetPos(x, y);
	}

	void SetSize(int w, int h)
	{
		if (m_pPanel)
			m_pPanel->SetSize(w, h);
	}

	void SetBounds(int x, int y, int w, int h)
	{
		if (m_pPanel)
			m_pPanel->SetBounds(x, y, w, h);
	}

	int GetX()
	{
		if (!m_pPanel)
			return 0;

		int x, y;
		m_pPanel->GetPos(x, y);
		return x;
	}

	int GetY()
	{
		if (!m_pPanel)
			return 0;

		int x, y;
		m_pPanel->GetPos(x, y);
		return y;
	}

	int GetWide()
	{
		return m_pPanel ? m_pPanel->GetWide() : 0;
	}

	int GetTall()
	{
		return m_pPanel ? m_pPanel->GetTall() : 0;
	}

	void MoveToFront()
	{
		if (m_pPanel)
			m_pPanel->MoveToFront();
	}

	void RequestFocus()
	{
		if (m_pPanel)
			m_pPanel->RequestFocus();
	}

	void SetParent(CScriptVPanel* pParent)
	{
		if (m_pPanel && pParent && pParent->m_pPanel)
			m_pPanel->SetParent(pParent->m_pPanel);
	}

	void SetName(const char* pszName)
	{
		if (m_pPanel && pszName)
			m_pPanel->SetName(pszName);
	}

	const char* GetName()
	{
		return m_pPanel ? m_pPanel->GetName() : "";
	}

	void SetTooltip(const char* pszText)
	{
		if (!m_pPanel || !pszText)
			return;

		vgui::TextTooltip* pTooltip =
			new vgui::TextTooltip(
				m_pPanel,
				"ScriptTooltip");

		pTooltip->SetText(pszText);

		m_pPanel->SetTooltip(pTooltip, pszText);
	}

	void SetBgColor(int r, int g, int b, int a)
	{
		if (m_pPanel)
			m_pPanel->SetBgColor(Color(r, g, b, a));
	}

	void SetFgColor(int r, int g, int b, int a)
	{
		if (m_pPanel)
			m_pPanel->SetFgColor(Color(r, g, b, a));
	}

	void SetCommand(const char* pszCommand)
	{
		if (!m_pPanel)
			return;

		CScriptVButton* pButton = dynamic_cast<CScriptVButton*>(m_pPanel);
		if (pButton)
		{
			pButton->SetScriptCommand(pszCommand);
		}
	}

	bool IsVisible()
	{
		return Script_VGUI_IsVisible(this);
	}

	bool IsEnabled()
	{
		return Script_VGUI_IsEnabled(this);
	}

	void SetAlpha(int a)
	{
		Script_VGUI_SetAlpha(this, a);
	}

	int GetAlpha()
	{
		return Script_VGUI_GetAlpha(this);
	}

	void SetZPos(int z)
	{
		Script_VGUI_SetZPos(this, z);
	}

	int GetZPos()
	{
		return Script_VGUI_GetZPos(this);
	}

	void SetMouseInputEnabled(bool bEnabled)
	{
		Script_VGUI_SetMouseInputEnabled(this, bEnabled);
	}

	void SetKeyboardInputEnabled(bool bEnabled)
	{
		Script_VGUI_SetKeyboardInputEnabled(this, bEnabled);
	}

	void SetPaintBackgroundEnabled(bool bEnabled)
	{
		Script_VGUI_SetPaintBackgroundEnabled(this, bEnabled);
	}

	void SetPaintBorderEnabled(bool bEnabled)
	{
		Script_VGUI_SetPaintBorderEnabled(this, bEnabled);
	}

	void CenterOnScreen()
	{
		Script_VGUI_CenterOnScreen(this);
	}

	void SetImage(const char* image)
	{
		Script_VGUI_SetImage(this, image);
	}

	void SetEditable(bool bEditable)
	{
		Script_VGUI_SetEditable(this, bEditable);
	}

	void SelectAllText()
	{
		Script_VGUI_SelectAllText(this);
	}

	HSCRIPT CreatePanel(int x, int y, int w, int h)
	{
		return Script_VGUI_CreatePanel(this, x, y, w, h);
	}

	HSCRIPT CreateLabel(const char* text, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateLabel(this, text, x, y, w, h);
	}

	HSCRIPT CreateButton(const char* text, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateButton(this, text, x, y, w, h);
	}

	HSCRIPT CreateTextEntry(const char* text, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateTextEntry(this, text, x, y, w, h);
	}

	HSCRIPT CreateCheckButton(const char* text, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateCheckButton(this, text, x, y, w, h);
	}

	HSCRIPT CreateComboBox(int x, int y, int w, int h, int maxItems)
	{
		return Script_VGUI_CreateComboBox(this, x, y, w, h, maxItems);
	}

	HSCRIPT CreateSlider(int x, int y, int w, int h)
	{
		return Script_VGUI_CreateSlider(this, x, y, w, h);
	}

	HSCRIPT CreateProgressBar(int x, int y, int w, int h)
	{
		return Script_VGUI_CreateProgressBar(this, x, y, w, h);
	}

	HSCRIPT CreateImagePanel(const char* image, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateImagePanel(this, image, x, y, w, h);
	}

	HSCRIPT CreateRichText(int x, int y, int w, int h)
	{
		return Script_VGUI_CreateRichText(this, x, y, w, h);
	}

	HSCRIPT CreateRadioButton(const char* text, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateRadioButton(this, text, x, y, w, h);
	}

	HSCRIPT CreateMenu()
	{
		return Script_VGUI_CreateMenu(this);
	}

	HSCRIPT CreateMenuButton(const char* text, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateMenuButton(this, text, x, y, w, h);
	}

	HSCRIPT CreateListPanel(int x, int y, int w, int h)
	{
		return Script_VGUI_CreateListPanel(this, x, y, w, h);
	}

	HSCRIPT CreateSectionedListPanel(int x, int y, int w, int h)
	{
		return Script_VGUI_CreateSectionedListPanel(this, x, y, w, h);
	}

	HSCRIPT CreateURLLabel(const char* text, const char* url, int x, int y, int w, int h)
	{
		return Script_VGUI_CreateURLLabel(this, text, url, x, y, w, h);
	}

	HSCRIPT CreateDivider(int x, int y, int w, int h)
	{
		return Script_VGUI_CreateDivider(this, x, y, w, h);
	}

	HSCRIPT CreatePropertySheet(int x, int y, int w, int h)
	{
		return Script_VGUI_CreatePropertySheet(this, x, y, w, h);
	}

	void SetText(const char* text)
	{
		Script_VGUI_SetText(this, text);
	}

	const char* GetText()
	{
		return Script_VGUI_GetText(this);
	}

	void SetChecked(bool bChecked)
	{
		Script_VGUI_SetChecked(this, bChecked);
	}

	bool IsChecked()
	{
		return Script_VGUI_IsChecked(this);
	}

	void ComboAddItem(const char* text)
	{
		Script_VGUI_ComboAddItem(this, text);
	}

	void ComboActivateItem(int itemID)
	{
		Script_VGUI_ComboActivateItem(this, itemID);
	}

	int ComboGetActiveItem()
	{
		return Script_VGUI_ComboGetActiveItem(this);
	}

	void SetSliderRange(int min, int max)
	{
		Script_VGUI_SetSliderRange(this, min, max);
	}

	void SetSliderValue(int value)
	{
		Script_VGUI_SetSliderValue(this, value);
	}

	int GetSliderValue()
	{
		return Script_VGUI_GetSliderValue(this);
	}

	void SetProgress(float progress)
	{
		Script_VGUI_SetProgress(this, progress);
	}

	void RichTextInsert(const char* text)
	{
		Script_VGUI_RichTextInsert(this, text);
	}

	void RichTextClear()
	{
		Script_VGUI_RichTextClear(this);
	}

	HSCRIPT CreateTreeView(int x, int y, int w, int h)
	{
		return Script_VGUI_CreateTreeView(this, x, y, w, h);
	}

	int TreeAddItem(int parentItem, const char* text)
	{
		return Script_VGUI_TreeAddItem(this, parentItem, text);
	}

	void TreeRemoveItem(int item)
	{
		Script_VGUI_TreeRemoveItem(this, item);
	}

	void TreeRemoveAll()
	{
		Script_VGUI_TreeRemoveAll(this);
	}

	int TreeGetSelectedItem()
	{
		return Script_VGUI_TreeGetSelectedItem(this);
	}

	void TreeExpandItem(int item, bool expand)
	{
		Script_VGUI_TreeExpandItem(this, item, expand);
	}

	vgui::Panel* m_pPanel;
};

BEGIN_SCRIPTDESC_ROOT(CScriptVPanel, "Script VGUI panel handle")
DEFINE_SCRIPTFUNC(IsValid, "Returns true if panel is valid")
DEFINE_SCRIPTFUNC(Delete, "Deletes the panel")
DEFINE_SCRIPTFUNC(SetVisible, "Sets visible")
DEFINE_SCRIPTFUNC(SetEnabled, "Sets enabled")
DEFINE_SCRIPTFUNC(SetPos, "Sets position")
DEFINE_SCRIPTFUNC(SetSize, "Sets size")
DEFINE_SCRIPTFUNC(SetBounds, "Sets bounds")
DEFINE_SCRIPTFUNC(GetX, "Gets x")
DEFINE_SCRIPTFUNC(GetY, "Gets y")
DEFINE_SCRIPTFUNC(GetWide, "Gets width")
DEFINE_SCRIPTFUNC(GetTall, "Gets height")
DEFINE_SCRIPTFUNC(MoveToFront, "Moves to front")
DEFINE_SCRIPTFUNC(RequestFocus, "Requests focus")
//DEFINE_SCRIPTFUNC(SetParent, "Sets parent panel")
DEFINE_SCRIPTFUNC(SetName, "Sets panel name")
DEFINE_SCRIPTFUNC(GetName, "Gets panel name")
DEFINE_SCRIPTFUNC(SetTooltip, "Sets tooltip")
DEFINE_SCRIPTFUNC(SetBgColor, "Sets background color")
DEFINE_SCRIPTFUNC(SetFgColor, "Sets foreground color")

DEFINE_SCRIPTFUNC(CreatePanel, "Creates child panel")
DEFINE_SCRIPTFUNC(CreateLabel, "Creates child label")
DEFINE_SCRIPTFUNC(CreateButton, "Creates child button")
DEFINE_SCRIPTFUNC(CreateTextEntry, "Creates child text entry")
DEFINE_SCRIPTFUNC(CreateCheckButton, "Creates child check button")
DEFINE_SCRIPTFUNC(CreateComboBox, "Creates child combo box")
DEFINE_SCRIPTFUNC(CreateSlider, "Creates child slider")
DEFINE_SCRIPTFUNC(CreateProgressBar, "Creates child progress bar")
DEFINE_SCRIPTFUNC(CreateImagePanel, "Creates child image panel")
DEFINE_SCRIPTFUNC(CreateRichText, "Creates child rich text")
DEFINE_SCRIPTFUNC(CreateRadioButton, "Creates child radio button")
DEFINE_SCRIPTFUNC(CreateMenu, "Creates child menu")
DEFINE_SCRIPTFUNC(CreateMenuButton, "Creates child menu button")
DEFINE_SCRIPTFUNC(CreateListPanel, "Creates child list panel")
DEFINE_SCRIPTFUNC(CreateSectionedListPanel, "Creates child sectioned list panel")
DEFINE_SCRIPTFUNC(CreateURLLabel, "Creates child URL label")
DEFINE_SCRIPTFUNC(CreateDivider, "Creates child divider")
DEFINE_SCRIPTFUNC(CreatePropertySheet, "Creates child property sheet")

DEFINE_SCRIPTFUNC(SetText, "Sets text")
DEFINE_SCRIPTFUNC(GetText, "Gets text")
DEFINE_SCRIPTFUNC(SetChecked, "Sets check state")
DEFINE_SCRIPTFUNC(IsChecked, "Gets check state")
DEFINE_SCRIPTFUNC(ComboAddItem, "Adds combo item")
DEFINE_SCRIPTFUNC(ComboActivateItem, "Activates combo item")
DEFINE_SCRIPTFUNC(ComboGetActiveItem, "Gets active combo item")
DEFINE_SCRIPTFUNC(SetSliderRange, "Sets slider range")
DEFINE_SCRIPTFUNC(SetSliderValue, "Sets slider value")
DEFINE_SCRIPTFUNC(GetSliderValue, "Gets slider value")
DEFINE_SCRIPTFUNC(SetProgress, "Sets progress")
DEFINE_SCRIPTFUNC(RichTextInsert, "Inserts rich text")
DEFINE_SCRIPTFUNC(RichTextClear, "Clears rich text")

DEFINE_SCRIPTFUNC(SetCommand, "Sets button command script")

DEFINE_SCRIPTFUNC(IsVisible, "Returns true if panel is visible")
DEFINE_SCRIPTFUNC(IsEnabled, "Returns true if panel is enabled")
DEFINE_SCRIPTFUNC(SetAlpha, "Sets panel alpha")
DEFINE_SCRIPTFUNC(GetAlpha, "Gets panel alpha")
DEFINE_SCRIPTFUNC(SetZPos, "Sets panel z position")
DEFINE_SCRIPTFUNC(GetZPos, "Gets panel z position")
DEFINE_SCRIPTFUNC(SetMouseInputEnabled, "Sets mouse input")
DEFINE_SCRIPTFUNC(SetKeyboardInputEnabled, "Sets keyboard input")
DEFINE_SCRIPTFUNC(SetPaintBackgroundEnabled, "Toggles background painting")
DEFINE_SCRIPTFUNC(SetPaintBorderEnabled, "Toggles border painting")
DEFINE_SCRIPTFUNC(CenterOnScreen, "Centers panel on screen")
DEFINE_SCRIPTFUNC(SetImage, "Sets image on ImagePanel")
DEFINE_SCRIPTFUNC(SetEditable, "Sets TextEntry editable")
DEFINE_SCRIPTFUNC(SelectAllText, "Selects all TextEntry text")
DEFINE_SCRIPTFUNC(CreateTreeView, "Creates child tree view")
DEFINE_SCRIPTFUNC(TreeAddItem, "Adds tree item")
DEFINE_SCRIPTFUNC(TreeRemoveItem, "Removes tree item")
DEFINE_SCRIPTFUNC(TreeRemoveAll, "Removes all tree items")
DEFINE_SCRIPTFUNC(TreeGetSelectedItem, "Gets selected tree item")
DEFINE_SCRIPTFUNC(TreeExpandItem, "Expands or collapses tree item")

END_SCRIPTDESC();

extern vgui::IPanel* vgui_ipanel;

static HSCRIPT WrapPanel(vgui::Panel* pPanel)
{
	if (!g_pScriptVM || !pPanel)
		return INVALID_HSCRIPT;

	CScriptVPanel* pScriptPanel = new CScriptVPanel(pPanel);

	return g_pScriptVM->RegisterInstance(
		GetScriptDescForClass(CScriptVPanel),
		pScriptPanel
	);
}

static vgui::Panel* UnwrapParent(CScriptVPanel* pParent)
{
	if (pParent && pParent->m_pPanel)
		return pParent->m_pPanel;

	return NULL;
}

static HSCRIPT Script_VGUI_CreatePanel(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::Panel* p = new vgui::Panel(UnwrapParent(pParent), "ScriptPanel");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateFrame(const char* title, int x, int y, int w, int h)
{
	vgui::Frame* p = new vgui::Frame(NULL, "ScriptFrame");

	p->SetTitle(title ? title : "Script Frame", true);
	p->SetBounds(x, y, w, h);
	p->SetSizeable(true);
	p->SetMoveable(true);
	p->SetVisible(true);

	p->MakePopup();
	p->MoveToFront();
	p->RequestFocus();
	p->Activate();

	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateLabel(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h)
{
	vgui::Label* p = new vgui::Label(UnwrapParent(pParent), "ScriptLabel", text ? text : "");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h)
{
	CScriptVButton* p = new CScriptVButton(UnwrapParent(pParent), "ScriptButton", text ? text : "");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateTextEntry(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h)
{
	vgui::TextEntry* p = new vgui::TextEntry(UnwrapParent(pParent), "ScriptTextEntry");
	p->SetBounds(x, y, w, h);
	p->SetText(text ? text : "");
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateCheckButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h)
{
	vgui::CheckButton* p = new vgui::CheckButton(UnwrapParent(pParent), "ScriptCheckButton", text ? text : "");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateComboBox(CScriptVPanel* pParent, int x, int y, int w, int h, int maxItems)
{
	vgui::ComboBox* p = new vgui::ComboBox(UnwrapParent(pParent), "ScriptComboBox", maxItems, false);
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateSlider(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::Slider* p = new vgui::Slider(UnwrapParent(pParent), "ScriptSlider");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateProgressBar(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::ProgressBar* p = new vgui::ProgressBar(UnwrapParent(pParent), "ScriptProgressBar");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateImagePanel(CScriptVPanel* pParent, const char* image, int x, int y, int w, int h)
{
	vgui::ImagePanel* p = new vgui::ImagePanel(UnwrapParent(pParent), "ScriptImagePanel");
	p->SetBounds(x, y, w, h);

	if (image && image[0])
		p->SetImage(image);

	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateRichText(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::RichText* p = new vgui::RichText(UnwrapParent(pParent), "ScriptRichText");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateRadioButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h)
{
	vgui::RadioButton* p = new vgui::RadioButton(UnwrapParent(pParent), "ScriptRadioButton", text ? text : "");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateMenu(CScriptVPanel* pParent)
{
	vgui::Menu* p = new vgui::Menu(UnwrapParent(pParent), "ScriptMenu");
	p->SetVisible(false);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateMenuButton(CScriptVPanel* pParent, const char* text, int x, int y, int w, int h)
{
	vgui::MenuButton* p = new vgui::MenuButton(UnwrapParent(pParent), "ScriptMenuButton", text ? text : "");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateListPanel(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::ListPanel* p = new vgui::ListPanel(UnwrapParent(pParent), "ScriptListPanel");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateSectionedListPanel(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::SectionedListPanel* p = new vgui::SectionedListPanel(UnwrapParent(pParent), "ScriptSectionedListPanel");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateURLLabel(CScriptVPanel* pParent, const char* text, const char* url, int x, int y, int w, int h)
{
	vgui::URLLabel* p = new vgui::URLLabel(UnwrapParent(pParent), "ScriptURLLabel", text ? text : "", url ? url : "");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreateDivider(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::Divider* p = new vgui::Divider(UnwrapParent(pParent), "ScriptDivider");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static HSCRIPT Script_VGUI_CreatePropertySheet(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::PropertySheet* p = new vgui::PropertySheet(UnwrapParent(pParent), "ScriptPropertySheet");
	p->SetBounds(x, y, w, h);
	p->SetVisible(true);
	return WrapPanel(p);
}

static void Script_VGUI_SetText(CScriptVPanel* pPanel, const char* text)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::Label* pLabel = dynamic_cast<vgui::Label*>(pPanel->m_pPanel);
	if (pLabel)
	{
		pLabel->SetText(text ? text : "");
		return;
	}

	vgui::Button* pButton = dynamic_cast<vgui::Button*>(pPanel->m_pPanel);
	if (pButton)
	{
		pButton->SetText(text ? text : "");
		return;
	}

	vgui::TextEntry* pTextEntry = dynamic_cast<vgui::TextEntry*>(pPanel->m_pPanel);
	if (pTextEntry)
	{
		pTextEntry->SetText(text ? text : "");
		return;
	}

	vgui::Frame* pFrame = dynamic_cast<vgui::Frame*>(pPanel->m_pPanel);
	if (pFrame)
	{
		pFrame->SetTitle(text ? text : "", true);
		return;
	}
}

static const char* Script_VGUI_GetText(CScriptVPanel* pPanel)
{
	static char buf[4096];
	buf[0] = 0;

	if (!pPanel || !pPanel->m_pPanel)
		return "";

	vgui::TextEntry* pTextEntry = dynamic_cast<vgui::TextEntry*>(pPanel->m_pPanel);
	if (pTextEntry)
	{
		pTextEntry->GetText(buf, sizeof(buf));
		return buf;
	}

	vgui::Label* pLabel = dynamic_cast<vgui::Label*>(pPanel->m_pPanel);
	if (pLabel)
	{
		pLabel->GetText(buf, sizeof(buf));
		return buf;
	}

	return "";
}

static void Script_VGUI_SetChecked(CScriptVPanel* pPanel, bool bChecked)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::CheckButton* p = dynamic_cast<vgui::CheckButton*>(pPanel->m_pPanel);
	if (p)
		p->SetSelected(bChecked);
}

static bool Script_VGUI_IsChecked(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return false;

	vgui::CheckButton* p = dynamic_cast<vgui::CheckButton*>(pPanel->m_pPanel);
	return p ? p->IsSelected() : false;
}

static void Script_VGUI_ComboAddItem(CScriptVPanel* pPanel, const char* text)
{
	if (!pPanel || !pPanel->m_pPanel || !text)
		return;

	vgui::ComboBox* p = dynamic_cast<vgui::ComboBox*>(pPanel->m_pPanel);
	if (p)
		p->AddItem(text, NULL);
}

static void Script_VGUI_ComboActivateItem(CScriptVPanel* pPanel, int itemID)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::ComboBox* p = dynamic_cast<vgui::ComboBox*>(pPanel->m_pPanel);
	if (p)
		p->ActivateItem(itemID);
}

static int Script_VGUI_ComboGetActiveItem(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return -1;

	vgui::ComboBox* p = dynamic_cast<vgui::ComboBox*>(pPanel->m_pPanel);
	return p ? p->GetActiveItem() : -1;
}

static void Script_VGUI_SetSliderRange(CScriptVPanel* pPanel, int min, int max)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::Slider* p = dynamic_cast<vgui::Slider*>(pPanel->m_pPanel);
	if (p)
		p->SetRange(min, max);
}

static void Script_VGUI_SetSliderValue(CScriptVPanel* pPanel, int value)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::Slider* p = dynamic_cast<vgui::Slider*>(pPanel->m_pPanel);
	if (p)
		p->SetValue(value);
}

static int Script_VGUI_GetSliderValue(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return 0;

	vgui::Slider* p = dynamic_cast<vgui::Slider*>(pPanel->m_pPanel);
	return p ? p->GetValue() : 0;
}

static void Script_VGUI_SetProgress(CScriptVPanel* pPanel, float progress)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::ProgressBar* p = dynamic_cast<vgui::ProgressBar*>(pPanel->m_pPanel);
	if (p)
		p->SetProgress(clamp(progress, 0.0f, 1.0f));
}

static void Script_VGUI_RichTextInsert(CScriptVPanel* pPanel, const char* text)
{
	if (!pPanel || !pPanel->m_pPanel || !text)
		return;

	vgui::RichText* p = dynamic_cast<vgui::RichText*>(pPanel->m_pPanel);
	if (p)
		p->InsertString(text);
}

static void Script_VGUI_RichTextClear(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::RichText* p = dynamic_cast<vgui::RichText*>(pPanel->m_pPanel);
	if (p)
		p->SetText("");
}

static bool Script_VGUI_IsVisible(CScriptVPanel* pPanel)
{
	return pPanel && pPanel->m_pPanel ? pPanel->m_pPanel->IsVisible() : false;
}

static bool Script_VGUI_IsEnabled(CScriptVPanel* pPanel)
{
	return pPanel && pPanel->m_pPanel ? pPanel->m_pPanel->IsEnabled() : false;
}

static void Script_VGUI_SetAlpha(CScriptVPanel* pPanel, int a)
{
	if (pPanel && pPanel->m_pPanel)
		pPanel->m_pPanel->SetAlpha(clamp(a, 0, 255));
}

static int Script_VGUI_GetAlpha(CScriptVPanel* pPanel)
{
	return pPanel && pPanel->m_pPanel ? pPanel->m_pPanel->GetAlpha() : 0;
}

static void Script_VGUI_SetZPos(CScriptVPanel* pPanel, int z)
{
	if (pPanel && pPanel->m_pPanel)
		pPanel->m_pPanel->SetZPos(z);
}

static int Script_VGUI_GetZPos(CScriptVPanel* pPanel)
{
	return pPanel && pPanel->m_pPanel ? pPanel->m_pPanel->GetZPos() : 0;
}

static void Script_VGUI_SetMouseInputEnabled(CScriptVPanel* pPanel, bool bEnabled)
{
	if (pPanel && pPanel->m_pPanel)
		pPanel->m_pPanel->SetMouseInputEnabled(bEnabled);
}

static void Script_VGUI_SetKeyboardInputEnabled(CScriptVPanel* pPanel, bool bEnabled)
{
	if (pPanel && pPanel->m_pPanel)
		pPanel->m_pPanel->SetKeyBoardInputEnabled(bEnabled);
}

static void Script_VGUI_SetPaintBackgroundEnabled(CScriptVPanel* pPanel, bool bEnabled)
{
	if (pPanel && pPanel->m_pPanel)
		pPanel->m_pPanel->SetPaintBackgroundEnabled(bEnabled);
}

static void Script_VGUI_SetPaintBorderEnabled(CScriptVPanel* pPanel, bool bEnabled)
{
	if (pPanel && pPanel->m_pPanel)
		pPanel->m_pPanel->SetPaintBorderEnabled(bEnabled);
}

static void Script_VGUI_CenterOnScreen(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	int sw, sh;
	engine->GetScreenSize(sw, sh);

	int w = pPanel->m_pPanel->GetWide();
	int h = pPanel->m_pPanel->GetTall();

	pPanel->m_pPanel->SetPos((sw - w) / 2, (sh - h) / 2);
}

static void Script_VGUI_SetImage(CScriptVPanel* pPanel, const char* image)
{
	if (!pPanel || !pPanel->m_pPanel || !image)
		return;

	vgui::ImagePanel* pImage = dynamic_cast<vgui::ImagePanel*>(pPanel->m_pPanel);
	if (pImage)
		pImage->SetImage(image);
}

static void Script_VGUI_SetEditable(CScriptVPanel* pPanel, bool bEditable)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::TextEntry* pText = dynamic_cast<vgui::TextEntry*>(pPanel->m_pPanel);
	if (pText)
		pText->SetEditable(bEditable);
}

static void Script_VGUI_SelectAllText(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::TextEntry* pText = dynamic_cast<vgui::TextEntry*>(pPanel->m_pPanel);
	if (pText)
		pText->SelectAllText(true);
}

static HSCRIPT Script_VGUI_CreateTreeView(CScriptVPanel* pParent, int x, int y, int w, int h)
{
	vgui::TreeView* p = new vgui::TreeView(UnwrapParent(pParent), "ScriptTreeView");

	p->SetBounds(x, y, w, h);

	vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetDefaultScheme());
	if (pScheme)
	{
		vgui::HFont font = pScheme->GetFont("Default", true);

		if (font == vgui::INVALID_FONT)
			font = pScheme->GetFont("DefaultSmall", true);

		if (font != vgui::INVALID_FONT)
			p->SetFont(font);

		p->SetFgColor(Color(255, 255, 255, 255));
	}

	p->SetVisible(true);
	return WrapPanel(p);
}

static int Script_VGUI_TreeAddItem(CScriptVPanel* pPanel, int parentItem, const char* text)
{
	if (!pPanel || !pPanel->m_pPanel || !text)
		return -1;

	vgui::TreeView* pTree = dynamic_cast<vgui::TreeView*>(pPanel->m_pPanel);
	if (!pTree)
		return -1;

	KeyValues* kv = new KeyValues("TreeItem");
	kv->SetString("text", text);

	int item = pTree->AddItem(kv, parentItem);
	kv->deleteThis();

	return item;
}

static void Script_VGUI_TreeRemoveItem(CScriptVPanel* pPanel, int item)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::TreeView* pTree = dynamic_cast<vgui::TreeView*>(pPanel->m_pPanel);
	if (pTree)
		pTree->RemoveItem(item, false, true);
}

static void Script_VGUI_TreeRemoveAll(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::TreeView* pTree = dynamic_cast<vgui::TreeView*>(pPanel->m_pPanel);
	if (pTree)
		pTree->RemoveAll();
}

static int Script_VGUI_TreeGetSelectedItem(CScriptVPanel* pPanel)
{
	if (!pPanel || !pPanel->m_pPanel)
		return -1;

	vgui::TreeView* pTree = dynamic_cast<vgui::TreeView*>(pPanel->m_pPanel);
	return pTree ? pTree->GetFirstSelectedItem() : -1;
}

static void Script_VGUI_TreeExpandItem(CScriptVPanel* pPanel, int item, bool expand)
{
	if (!pPanel || !pPanel->m_pPanel)
		return;

	vgui::TreeView* pTree = dynamic_cast<vgui::TreeView*>(pPanel->m_pPanel);
	if (pTree)
		pTree->ExpandItem(item, expand);
}

static const char* Script_FS_FindFirst(const char* wildcard)
{
	static char buf[MAX_PATH];
	buf[0] = 0;

	if (g_ScriptFindHandle != FILESYSTEM_INVALID_FIND_HANDLE)
	{
		filesystem->FindClose(g_ScriptFindHandle);
		g_ScriptFindHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	}

	if (!wildcard || !wildcard[0])
		return "";

	const char* found = filesystem->FindFirst(wildcard, &g_ScriptFindHandle);

	if (!found)
		return "";

	Q_strncpy(buf, found, sizeof(buf));
	return buf;
}

static const char* Script_FS_FindNext()
{
	static char buf[MAX_PATH];
	buf[0] = 0;

	if (g_ScriptFindHandle == FILESYSTEM_INVALID_FIND_HANDLE)
		return "";

	const char* found = filesystem->FindNext(g_ScriptFindHandle);

	if (!found)
		return "";

	Q_strncpy(buf, found, sizeof(buf));
	return buf;
}

static void Script_FS_FindClose()
{
	if (g_ScriptFindHandle != FILESYSTEM_INVALID_FIND_HANDLE)
	{
		filesystem->FindClose(g_ScriptFindHandle);
		g_ScriptFindHandle = FILESYSTEM_INVALID_FIND_HANDLE;
	}
}

static bool Script_FS_IsDirectory(const char* path)
{
	return path && path[0] && filesystem->IsDirectory(path, "GAME");
}

static void Script_MessageBox(const char* pszTitle, const char* pszText)
{
	vgui::MessageBox* pBox = new vgui::MessageBox(
		pszTitle ? pszTitle : "Message",
		pszText ? pszText : "",
		NULL);

	pBox->MakePopup();
	pBox->MoveToFront();
	pBox->SetVisible(true);
}

static void Script_ClientMsg(const char* pszText)
{
	Msg("%s\n", pszText ? pszText : "");
}

static float Script_Time()
{
	return gpGlobals ? gpGlobals->curtime : 0.0f;
}

static void Script_Warning(const char* pszText)
{
	Warning("%s\n", pszText ? pszText : "");
}

static const char* Script_GetMapName()
{
	return engine->GetLevelName();
}

static int Script_GetLocalPlayerIndex()
{
	return engine->GetLocalPlayer();
}

static bool Script_IsInGame()
{
	return engine->IsInGame();
}

static bool Script_IsConnected()
{
	return engine->IsConnected();
}

static int Script_GetScreenWidth()
{
	int w, h;
	engine->GetScreenSize(w, h);
	return w;
}

static int Script_GetScreenHeight()
{
	int w, h;
	engine->GetScreenSize(w, h);
	return h;
}

static void Script_ClientCommand(const char* pszCommand)
{
	if (pszCommand && pszCommand[0])
	{
		engine->ClientCmd(pszCommand);
	}
}

static void Script_ConCommand(const char* pszCommand)
{
	if (pszCommand && pszCommand[0])
	{
		engine->ClientCmd_Unrestricted(pszCommand);
	}
}

static void Script_PlaySound(const char* pszSound)
{
	if (pszSound && pszSound[0])
	{
		vgui::surface()->PlaySound(pszSound);
	}
}

static float Script_FrameTime()
{
	return gpGlobals ? gpGlobals->frametime : 0.0f;
}

static int Script_MaxClients()
{
	return gpGlobals ? gpGlobals->maxClients : 0;
}

static float Script_RealTime() { return gpGlobals ? gpGlobals->realtime : 0.0f; }
static int Script_FrameCount() { return gpGlobals ? gpGlobals->framecount : 0; }
static int Script_TickCount() { return gpGlobals ? gpGlobals->tickcount : 0; }
static float Script_IntervalPerTick() { return gpGlobals ? gpGlobals->interval_per_tick : 0.0f; }
static float Script_CurTime() { return gpGlobals ? gpGlobals->curtime : 0.0f; }

static float Script_RandomFloat(float a, float b) { return RandomFloat(a, b); }
static int Script_RandomInt(int a, int b) { return RandomInt(a, b); }

static float Script_Sin(float x) { return sinf(x); }
static float Script_Cos(float x) { return cosf(x); }
static float Script_Tan(float x) { return tanf(x); }
static float Script_Sqrt(float x) { return sqrtf(x); }
static float Script_Abs(float x) { return fabsf(x); }
static float Script_Floor(float x) { return floorf(x); }
static float Script_Ceil(float x) { return ceilf(x); }

static float Script_Min(float a, float b) { return MIN(a, b); }
static float Script_Max(float a, float b) { return MAX(a, b); }
static float Script_Clamp(float x, float a, float b) { return clamp(x, a, b); }

static float Script_Deg2Rad(float x) { return DEG2RAD(x); }
static float Script_Rad2Deg(float x) { return RAD2DEG(x); }

static const char* Script_GetGameDirectory()
{
	return engine->GetGameDirectory();
}

static int Script_GetAppID()
{
	return engine->GetAppID();
}

static bool Script_IsDrawingLoadingImage()
{
	return engine->IsDrawingLoadingImage();
}

static float Script_TimeScale()
{
	static ConVarRef host_timescale("host_timescale");
	return host_timescale.IsValid() ? host_timescale.GetFloat() : 1.0f;
}

static const char* Script_GetConVarString(const char* name)
{
	static char buf[256];

	if (!name || !name[0])
		return "";

	ConVarRef ref(name);

	if (!ref.IsValid())
		return "";

	Q_strncpy(buf, ref.GetString(), sizeof(buf));
	return buf;
}

static float Script_GetConVarFloat(const char* name)
{
	ConVarRef ref(name);
	return ref.IsValid() ? ref.GetFloat() : 0.0f;
}

static int Script_GetConVarInt(const char* name)
{
	ConVarRef ref(name);
	return ref.IsValid() ? ref.GetInt() : 0;
}

static bool Script_GetConVarBool(const char* name)
{
	ConVarRef ref(name);
	return ref.IsValid() ? ref.GetBool() : false;
}

static void Script_SetConVarString(const char* name, const char* value)
{
	if (!name || !value)
		return;

	ConVarRef ref(name);

	if (ref.IsValid())
		ref.SetValue(value);
}

static void Script_SetConVarFloat(const char* name, float value)
{
	ConVarRef ref(name);

	if (ref.IsValid())
		ref.SetValue(value);
}

static void Script_SetConVarInt(const char* name, int value)
{
	ConVarRef ref(name);

	if (ref.IsValid())
		ref.SetValue(value);
}

static bool Script_IsKeyDown(int buttonCode)
{
	return inputsystem->IsButtonDown((ButtonCode_t)buttonCode);
}

static int Script_ButtonCodeToStringExists(int buttonCode)
{
	return inputsystem->ButtonCodeToString((ButtonCode_t)buttonCode) != NULL;
}

static const char* Script_ButtonCodeToString(int buttonCode)
{
	const char* psz = inputsystem->ButtonCodeToString((ButtonCode_t)buttonCode);
	return psz ? psz : "";
}

static int Script_StringToButtonCode(const char* name)
{
	if (!name)
		return BUTTON_CODE_INVALID;

	return inputsystem->StringToButtonCode(name);
}

static void Script_SetMousePosition(int x, int y)
{
	inputsystem->SetCursorPosition(x, y);
}

static void Script_ShowCursor(bool show)
{
	vgui::surface()->SetCursorAlwaysVisible(show);
}

static bool Script_IsConsoleVisible()
{
	return enginevgui->IsGameUIVisible();
}

static int Script_GetBuildNumber()
{
	return engine->GetEngineBuildNumber();
}

static void Script_ServerCommand(const char* cmd)
{
	if (cmd && cmd[0])
		engine->ServerCmd(cmd);
}

static void Script_ClearConsole()
{
	engine->ClientCmd_Unrestricted("clear");
}

static void Script_ToggleConsole()
{
	engine->ClientCmd_Unrestricted("toggleconsole");
}

static void Script_Disconnect()
{
	engine->ClientCmd_Unrestricted("disconnect");
}

static void Script_Quit()
{
	engine->ClientCmd_Unrestricted("quit");
}

static const char* Script_GetLanguage()
{
	static ConVarRef ref("cl_language");
	return ref.IsValid() ? ref.GetString() : "";
}

static bool Script_FileExists(const char* path)
{
	return path && filesystem->FileExists(path, "GAME");
}

static const char* Script_ReadTextFile(const char* path)
{
	static CUtlBuffer buf;

	buf.Clear();

	if (!path || !filesystem->ReadFile(path, "GAME", buf))
		return "";

	buf.PutChar(0);
	return (const char*)buf.Base();
}

static void Script_WriteTextFile(const char* path, const char* text)
{
	if (!path || !text)
		return;

	CUtlBuffer buf;
	buf.PutString(text);

	filesystem->WriteFile(path, "GAME", buf);
}

static float Script_VectorLength(float x, float y, float z)
{
	return Vector(x, y, z).Length();
}

static float Script_VectorDistance(
	float ax, float ay, float az,
	float bx, float by, float bz)
{
	Vector a(ax, ay, az);
	Vector b(bx, by, bz);
	return a.DistTo(b);
}

static float Script_DotProduct(
	float ax, float ay, float az,
	float bx, float by, float bz)
{
	return DotProduct(Vector(ax, ay, az), Vector(bx, by, bz));
}

static float Script_AngleNormalize(float angle)
{
	return AngleNormalize(angle);
}

static float Script_Approach(float target, float value, float speed)
{
	return Approach(target, value, speed);
}

static float Script_ApproachAngle(float target, float value, float speed)
{
	return ApproachAngle(target, value, speed);
}

static const char* Script_GetClipboardText()
{
	static char buf[4096];
	buf[0] = 0;

	vgui::system()->GetClipboardText(0, buf, sizeof(buf));
	return buf;
}

static void Script_SetClipboardText(const char* text)
{
	if (text)
		vgui::system()->SetClipboardText(text, Q_strlen(text));
}

static void Script_DebugPrint(const char* text)
{
	DevMsg("%s\n", text ? text : "");
}

static void Script_DevWarning(const char* text)
{
	DevWarning("%s\n", text ? text : "");
}

static bool Script_IsTakingScreenshot()
{
	return engine->IsTakingScreenshot();
}

static void Script_TakeScreenshot()
{
	engine->ClientCmd_Unrestricted("jpeg");
}

static void RegisterClientScriptFunctions()
{
	if (!g_pScriptVM)
		return;

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		Script_MessageBox,
		"MessageBox",
		"Shows a client VGUI message box"
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		Script_ClientMsg,
		"ClientMsg",
		"Prints text to the client console"
	);

	ScriptRegisterFunctionNamed(
		g_pScriptVM,
		Script_Time,
		"Time",
		"Returns client curtime"
	);

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Warning, "Warning", "Prints warning text to the console");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetMapName, "GetMapName", "Returns the current map path");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetLocalPlayerIndex, "GetLocalPlayerIndex", "Returns the local player index");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsInGame, "IsInGame", "Returns true if client is in game");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsConnected, "IsConnected", "Returns true if client is connected");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetScreenWidth, "GetScreenWidth", "Returns screen width");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetScreenHeight, "GetScreenHeight", "Returns screen height");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ClientCommand, "ClientCommand", "Runs a client command");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConCommand, "ConCommand", "Runs an unrestricted client command");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_PlaySound, "PlaySound", "Plays a UI sound");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FrameTime, "FrameTime", "Returns client frametime");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_MaxClients, "MaxClients", "Returns max clients");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RealTime, "RealTime", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FrameCount, "FrameCount", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_TickCount, "TickCount", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IntervalPerTick, "IntervalPerTick", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_CurTime, "CurTime", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RandomFloat, "RandomFloat", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RandomInt, "RandomInt", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Sin, "Sin", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Cos, "Cos", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Tan, "Tan", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Sqrt, "Sqrt", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Abs, "Abs", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Floor, "Floor", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Ceil, "Ceil", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Min, "Min", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Max, "Max", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Clamp, "Clamp", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Deg2Rad, "Deg2Rad", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Rad2Deg, "Rad2Deg", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetGameDirectory, "GetGameDirectory", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetAppID, "GetAppID", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsDrawingLoadingImage, "IsDrawingLoadingImage", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_TimeScale, "TimeScale", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetConVarString, "GetConVarString", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetConVarFloat, "GetConVarFloat", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetConVarInt, "GetConVarInt", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetConVarBool, "GetConVarBool", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_SetConVarString, "SetConVarString", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_SetConVarFloat, "SetConVarFloat", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_SetConVarInt, "SetConVarInt", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsKeyDown, "IsKeyDown", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ButtonCodeToStringExists, "ButtonCodeToStringExists", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ButtonCodeToString, "ButtonCodeToString", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_StringToButtonCode, "StringToButtonCode", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_SetMousePosition, "SetMousePosition", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ShowCursor, "ShowCursor", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsConsoleVisible, "IsConsoleVisible", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetBuildNumber, "GetBuildNumber", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ServerCommand, "ServerCommand", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ClearConsole, "ClearConsole", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ToggleConsole, "ToggleConsole", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Disconnect, "Disconnect", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Quit, "Quit", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetLanguage, "GetLanguage", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FileExists, "FileExists", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ReadTextFile, "ReadTextFile", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_WriteTextFile, "WriteTextFile", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_VectorLength, "VectorLength", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_VectorDistance, "VectorDistance", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DotProduct, "DotProduct3D", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_AngleNormalize, "AngleNormalize", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Approach, "Approach", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ApproachAngle, "ApproachAngle", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetClipboardText, "GetClipboardText", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_SetClipboardText, "SetClipboardText", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DebugPrint, "DevMsg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DevWarning, "DevWarning", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsTakingScreenshot, "IsTakingScreenshot", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_TakeScreenshot, "TakeScreenshot", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FS_FindFirst, "FS_FindFirst", "Finds first matching GAME file");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FS_FindNext, "FS_FindNext", "Finds next matching GAME file");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FS_FindClose, "FS_FindClose", "Closes file search");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FS_IsDirectory, "FS_IsDirectory", "Returns true if GAME path is directory");

	g_pScriptVM->RegisterClass(GetScriptDescForClass(CScriptVPanel));

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_VGUI_CreateFrame, "VGUI_CreateFrame", "");

	g_pScriptVM->RegisterClass(GetScriptDescForClass(CScripted_HudElement));

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_HUD_CreateElement, "HUD_CreateElement", "Creates scripted HUD element");
}

//-----------------------------------------------------------------------------
// Script execution
//-----------------------------------------------------------------------------

bool VScriptClientRunScript(const char* pszScriptName)
{
	if (!g_pScriptVM)
		return false;

	if (!pszScriptName || !pszScriptName[0])
		return false;

	CFmtStr scriptPath("scripts/vscripts/%s", pszScriptName);

	if (!V_strstr(pszScriptName, ".nut"))
	{
		scriptPath.sprintf("scripts/vscripts/%s.nut", pszScriptName);
	}

	CUtlBuffer buffer;

	if (!filesystem->ReadFile(scriptPath.Access(), "GAME", buffer))
	{
		Warning("Client VScript: could not read script '%s'\n", scriptPath.Access());
		return false;
	}

	buffer.PutChar(0);

	bool bResult = g_pScriptVM->Run((const char*)buffer.Base(), true);

	return bResult;
}

//-----------------------------------------------------------------------------
// VM init / shutdown
//-----------------------------------------------------------------------------

bool VScriptClientInit()
{
	if (g_pScriptVM)
		return true;

	if (!s_AppSystemFactory)
	{
		Warning("Client VScript: appSystemFactory is NULL\n");
		return false;
	}

	s_pScriptManager = (IScriptManager*)s_AppSystemFactory(
		VSCRIPT_INTERFACE_VERSION,
		NULL);

	if (!s_pScriptManager)
	{
		Warning("Client VScript: could not get IScriptManager\n");
		return false;
	}

	g_pScriptVM = s_pScriptManager->CreateVM(SL_SQUIRREL);

	if (!g_pScriptVM)
	{
		Warning("Client VScript: failed to create Squirrel VM\n");
		return false;
	}

	RegisterClientScriptFunctions();

	Msg("Client VScript: Squirrel VM started\n");

	VScriptClientRunScript("client_init");

	return true;
}

void VScriptClientTerm()
{
	if (g_pScriptVM && s_pScriptManager)
	{
		s_pScriptManager->DestroyVM(g_pScriptVM);
	}

	g_pScriptVM = NULL;
	s_pScriptManager = NULL;
}

CON_COMMAND_F(gabe_cl_script_execute, "Run a client-side VScript file", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: client_script_execute <scriptname>\n");
		Msg("Example: client_script_execute test\n");
		return;
	}

	VScriptClientRunScript(args[1]);
}

class CGabeVGUIScriptRunner;

static CGabeVGUIScriptRunner* g_pScriptRunner = NULL;

class CGabeVGUIScriptRunner : public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CGabeVGUIScriptRunner, vgui::Frame);

public:
	CGabeVGUIScriptRunner() : BaseClass(NULL, "GabeVGUIScriptRunner")
	{
		SetTitle(".nut Script Runner", true);
		SetSize(900, 520);
		SetPos(160, 90);
		SetSizeable(false);
		SetMoveable(true);
		SetDeleteSelfOnClose(true);

		m_pList = new vgui::ListPanel(this, "ScriptList");
		m_pList->SetBounds(15, 35, 560, 390);
		m_pList->AddColumnHeader(0, "file", "File", 170, 0);
		m_pList->AddColumnHeader(1, "author", "Author", 90, 0);
		m_pList->AddColumnHeader(2, "version", "Version", 80, 0);
		m_pList->AddColumnHeader(3, "date", "Date Made", 100, 0);
		m_pList->AddColumnHeader(4, "desc", "Description", 260, 0);

		m_pEntry = new vgui::TextEntry(this, "ManualEntry");
		m_pEntry->SetBounds(15, 435, 360, 26);

		m_pLog = new vgui::RichText(this, "Log");
		m_pLog->SetBounds(590, 35, 290, 390);

		(new vgui::Button(this, "RunSelected", "Run Selected", this, "RunSelected"))
			->SetBounds(390, 435, 120, 30);

		(new vgui::Button(this, "RunTyped", "Run Typed", this, "RunTyped"))
			->SetBounds(520, 435, 100, 30);

		(new vgui::Button(this, "Refresh", "Refresh", this, "Refresh"))
			->SetBounds(630, 435, 90, 30);

		(new vgui::Button(this, "Close", "Close", this, "Close"))
			->SetBounds(730, 435, 90, 30);

		RefreshScripts();

		SetVisible(true);
		MakePopup();
		MoveToFront();
		Activate();
	}

	void OnCommand(const char* cmd) OVERRIDE
	{
		if (!Q_stricmp(cmd, "RunSelected"))
		{
			RunSelected();
			return;
		}

		if (!Q_stricmp(cmd, "RunTyped"))
		{
			RunTyped();
			return;
		}

		if (!Q_stricmp(cmd, "Refresh"))
		{
			RefreshScripts();
			return;
		}

		if (!Q_stricmp(cmd, "Close"))
		{
			Close();
			return;
		}

		BaseClass::OnCommand(cmd);
	}

	virtual void OnClose()
	{
		if (g_pScriptRunner == this)
			g_pScriptRunner = NULL;

		BaseClass::OnClose();
	}

	~CGabeVGUIScriptRunner()
	{
		if (g_pScriptRunner == this)
			g_pScriptRunner = NULL;
	}

private:
	vgui::ListPanel* m_pList;
	vgui::TextEntry* m_pEntry;
	vgui::RichText* m_pLog;

	void Log(const char* fmt, ...)
	{
		char buf[1024];

		va_list args;
		va_start(args, fmt);
		Q_vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		m_pLog->InsertString(buf);
		m_pLog->InsertString("\n");
	}

	bool ExtractMeta(const char* text, const char* key, char* out, int outSize)
	{
		out[0] = 0;

		CFmtStr findLocal("local %s", key);
		const char* p = V_stristr(text, findLocal.Access());

		if (!p)
		{
			CFmtStr findRoot("::%s", key);
			p = V_stristr(text, findRoot.Access());
		}

		if (!p)
			return false;

		const char* q1 = V_strchr(p, '"');
		if (!q1)
			return false;

		q1++;

		const char* q2 = V_strchr(q1, '"');
		if (!q2)
			return false;

		int len = MIN((int)(q2 - q1), outSize - 1);
		Q_strncpy(out, q1, len + 1);
		out[len] = 0;

		return true;
	}

	void ReadMetadata(const char* path, char* author, int authorSize,
		char* desc, int descSize, char* version, int versionSize,
		char* date, int dateSize)
	{
		author[0] = desc[0] = version[0] = date[0] = 0;

		CUtlBuffer buf;
		if (!filesystem->ReadFile(path, "GAME", buf))
			return;

		buf.PutChar(0);
		const char* text = (const char*)buf.Base();

		ExtractMeta(text, "Author", author, authorSize);
		ExtractMeta(text, "Description", desc, descSize);
		ExtractMeta(text, "Version", version, versionSize);
		ExtractMeta(text, "DateMade", date, dateSize);
	}

	void RefreshScripts()
	{
		m_pList->RemoveAll();
		m_pLog->SetText("");

		Log("Scanning scripts/vscripts/*.nut...");

		FileFindHandle_t hFind;
		const char* file = filesystem->FindFirst("scripts/vscripts/*.nut", &hFind);

		int count = 0;

		while (file)
		{
			char author[128];
			char desc[256];
			char version[64];
			char date[64];

			CFmtStr fullPath("scripts/vscripts/%s", file);
			ReadMetadata(fullPath.Access(), author, sizeof(author),
				desc, sizeof(desc), version, sizeof(version),
				date, sizeof(date));

			KeyValues* kv = new KeyValues("script");
			kv->SetString("file", file);
			kv->SetString("author", author[0] ? author : "Unknown");
			kv->SetString("version", version[0] ? version : "-");
			kv->SetString("date", date[0] ? date : "-");
			kv->SetString("desc", desc[0] ? desc : "No description");

			m_pList->AddItem(kv, 0, false, false);
			kv->deleteThis();

			count++;
			file = filesystem->FindNext(hFind);
		}

		filesystem->FindClose(hFind);

		Log("Found %d scripts.", count);
	}

	void NormalizeScriptName(char* script, int scriptSize)
	{
		V_StripExtension(script, script, scriptSize);

		if (!Q_strnicmp(script, "scripts/vscripts/", 17))
		{
			Q_memmove(script, script + 17, Q_strlen(script + 17) + 1);
		}
	}

	void RunScriptEverywhere(const char* scriptName)
	{
		if (!scriptName || !scriptName[0])
		{
			Log("No script name.");
			return;
		}

		char script[MAX_PATH];
		Q_strncpy(script, scriptName, sizeof(script));
		NormalizeScriptName(script, sizeof(script));

		Log("Running: %s", script);

		VScriptClientRunScript(script);

		CFmtStr clCmd("gabe_cl_script_execute %s", script);
		engine->ClientCmd_Unrestricted(clCmd.Access());

		CFmtStr serverCmd("script_execute %s", script);
		engine->ClientCmd_Unrestricted(serverCmd.Access());
		engine->ServerCmd(serverCmd.Access());
	}

	void RunSelected()
	{
		int item = m_pList->GetSelectedItem(0);
		if (item < 0)
		{
			Log("No selected script.");
			return;
		}

		KeyValues* kv = m_pList->GetItem(item);
		if (!kv)
		{
			Log("Selected row had no data.");
			return;
		}

		RunScriptEverywhere(kv->GetString("file", ""));
	}

	void RunTyped()
	{
		char text[MAX_PATH];
		m_pEntry->GetText(text, sizeof(text));
		RunScriptEverywhere(text);
	}
};

CON_COMMAND_F(gabe_loadscripts, "Open script runner", FCVAR_NONE)
{
	if (g_pScriptRunner && !g_pScriptRunner->IsMarkedForDeletion())
	{
		g_pScriptRunner->SetVisible(false);
		g_pScriptRunner->MarkForDeletion();
		g_pScriptRunner = NULL;
		return;
	}

	g_pScriptRunner = new CGabeVGUIScriptRunner();
}