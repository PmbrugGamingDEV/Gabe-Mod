//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client VScript
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vscript_client.h"
#include "c_gabe_vscript.h"
#include "eiface.h"
#include "icommandline.h"
#include "filesystem.h"
#include "tier1/fmtstr.h"
#include "tier1/utlbuffer.h"
#include "../../public/vscript/vscript_templates.h"
#include <vgui/VGUI.h>
#include <vgui/ISurface.h>
#include <vgui_controls/panel.h>
#include <vgui_controls/editablepanel.h>
#include <vgui_controls/frame.h>
#include <vgui_controls/label.h>
#include <vgui_controls/button.h>
#include <vgui_controls/textentry.h>
#include <vgui_controls/checkbutton.h>
#include <vgui_controls/combobox.h>
#include <vgui_controls/slider.h>
#include <vgui_controls/listpanel.h>
#include <vgui_controls/sectionedlistpanel.h>
#include <vgui_controls/propertysheet.h>
#include <vgui_controls/propertypage.h>
#include <vgui_controls/imagepanel.h>
#include <vgui_controls/richtext.h>
#include <vgui_controls/menu.h>
#include "con_nprint.h"
#include <vgui_controls/menubutton.h>
#include <vgui_controls/menubar.h>
#include <vgui_controls/scrollbar.h>
#include <vgui_controls/scrollbarslider.h>
#include <vgui_controls/treeview.h>
#include <vgui_controls/progressbar.h>
#include <vgui_controls/togglebutton.h>
#include <vgui_controls/querybox.h>
#include <vgui_controls/messagebox.h>
#include <vgui_controls/wizardpanel.h>
#include <vgui_controls/Tooltip.h>
#include "hudelement.h"
#include "hud_element_helper.h"
#include "hud.h"
#include "hud_macros.h"
#include "vgui/cursor.h"
#include "vgui/dar.h"
#include "vgui/iborder.h"
#include "vgui/iclientpanel.h"
#include "vgui/ihtml.h"
#include "vgui/iimage.h"
#include "vgui/iinput.h"
#include "inputsystem/iinputsystem.h"
#include "vgui/iinputinternal.h"
#include "vgui/ilocalize.h"
#include "vgui/ipanel.h"
#include "vgui/ischeme.h"
#include "vgui/isystem.h"
#include "vgui/ivgui.h"
#include "vgui/ivguimatinfo.h"
#include "vgui/ivguimatinfovar.h"
#include "vgui/keycode.h"
#include "vgui/mousecode.h"
#include "vgui/point.h"
#include "ienginevgui.h"
#include "vgui_controls/RadioButton.h"
#include "vgui_controls/URLLabel.h"
#include "vgui_controls/Divider.h"
#include "vgui_controls/Controls.h"
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

	const char* GetHUDName()
	{
		return CHudElement::GetName();
	}

	void HUD_Init()
	{
		CHudElement::Init();
	}

	void HUD_VidInit()
	{
		VidInit();
	}

	void HUD_LevelInit()
	{
		LevelInit();
	}

	void HUD_LevelShutdown()
	{
		LevelShutdown();
	}

	void HUD_Reset()
	{
		Reset();
	}

	void HUD_ProcessInput()
	{
		ProcessInput();
	}

	bool HUD_ShouldDraw()
	{
		return ShouldDraw();
	}

	bool HUD_IsActive()
	{
		return IsActive();
	}

	void HUD_SetActive(bool bActive)
	{
		SetActive(bActive);
		SetVisible(bActive);
	}

	void HUD_SetHiddenBits(int bits)
	{
		SetHiddenBits(bits);
	}

	void HUD_SetNeedsRemove(bool bRemove)
	{
		SetNeedsRemove(bRemove);
	}

	bool HUD_IsParentedToClientDLLRootPanel()
	{
		return IsParentedToClientDLLRootPanel();
	}

	void HUD_SetParentedToClientDLLRootPanel(bool bParented)
	{
		SetParentedToClientDLLRootPanel(bParented);
	}

	void HUD_RegisterForRenderGroup(const char* group)
	{
		if (group && group[0])
			RegisterForRenderGroup(group);
	}

	void HUD_UnregisterForRenderGroup(const char* group)
	{
		if (group && group[0])
			UnregisterForRenderGroup(group);
	}

	void HUD_HideLowerPriorityHudElementsInGroup(const char* group)
	{
		if (group && group[0])
			HideLowerPriorityHudElementsInGroup(group);
	}

	void HUD_UnhideLowerPriorityHudElementsInGroup(const char* group)
	{
		if (group && group[0])
			UnhideLowerPriorityHudElementsInGroup(group);
	}

	int HUD_GetRenderGroupPriority()
	{
		return GetRenderGroupPriority();
	}

	void HUD_SetPos(int x, int y)
	{
		SetPos(x, y);
	}

	void HUD_SetSize(int w, int h)
	{
		SetSize(w, h);
	}

	void HUD_SetBounds(int x, int y, int w, int h)
	{
		SetBounds(x, y, w, h);
	}

	int HUD_GetX()
	{
		int x, y;
		GetPos(x, y);
		return x;
	}

	int HUD_GetY()
	{
		int x, y;
		GetPos(x, y);
		return y;
	}

	int HUD_GetWide()
	{
		return GetWide();
	}

	int HUD_GetTall()
	{
		return GetTall();
	}

	void HUD_SetAlpha(int a)
	{
		SetAlpha(clamp(a, 0, 255));
	}

	int HUD_GetAlpha()
	{
		return GetAlpha();
	}

	void HUD_SetZPos(int z)
	{
		SetZPos(z);
	}

	int HUD_GetZPos()
	{
		return GetZPos();
	}

	void HUD_SetBgColor(int r, int g, int b, int a)
	{
		SetBgColor(Color(r, g, b, a));
	}

	void HUD_SetFgColor(int r, int g, int b, int a)
	{
		SetFgColor(Color(r, g, b, a));
	}

	void HUD_SetPaintBackgroundEnabled(bool b)
	{
		SetPaintBackgroundEnabled(b);
	}

	void HUD_SetPaintBorderEnabled(bool b)
	{
		SetPaintBorderEnabled(b);
	}

	void HUD_SetMouseInputEnabled(bool b)
	{
		SetMouseInputEnabled(b);
	}

	void HUD_SetKeyboardInputEnabled(bool b)
	{
		SetKeyBoardInputEnabled(b);
	}

	void HUD_MoveToFront()
	{
		MoveToFront();
	}

	void HUD_RequestFocus()
	{
		RequestFocus();
	}

	bool HUD_IsVisible()
	{
		return IsVisible();
	}

	bool HUD_IsEnabled()
	{
		return IsEnabled();
	}

	void HUD_SetEnabled(bool b)
	{
		SetEnabled(b);
	}

	bool HUD_IsHiddenByBits(int bits)
	{
		return (m_iHiddenBits & bits) != 0;
	}

	int HUD_GetHiddenBits()
	{
		return m_iHiddenBits;
	}

	void HUD_AddHiddenBits(int bits)
	{
		SetHiddenBits(m_iHiddenBits | bits);
	}

	void HUD_RemoveHiddenBits(int bits)
	{
		SetHiddenBits(m_iHiddenBits & ~bits);
	}

	void HUD_ClearHiddenBits()
	{
		SetHiddenBits(0);
	}

	void HUD_ToggleActive()
	{
		HUD_SetActive(!IsActive());
	}

	void HUD_Remove()
	{
		SetVisible(false);
		SetActive(false);
		SetNeedsRemove(true);
		MarkForDeletion();
	}

	void HUD_ForceShow()
	{
		SetHiddenBits(0);
		SetActive(true);
		SetVisible(true);
	}

	void HUD_ForceHide()
	{
		SetActive(false);
		SetVisible(false);
	}

	bool HUD_IsDrawableNow()
	{
		return IsActive() && IsVisible() && ShouldDraw();
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
DEFINE_SCRIPTFUNC(GetHUDName, "Gets HUD element name")
DEFINE_SCRIPTFUNC(HUD_Init, "Calls HUD Init")
DEFINE_SCRIPTFUNC(HUD_VidInit, "Calls HUD VidInit")
DEFINE_SCRIPTFUNC(HUD_LevelInit, "Calls HUD LevelInit")
DEFINE_SCRIPTFUNC(HUD_LevelShutdown, "Calls HUD LevelShutdown")
DEFINE_SCRIPTFUNC(HUD_Reset, "Calls HUD Reset")
DEFINE_SCRIPTFUNC(HUD_ProcessInput, "Calls HUD ProcessInput")
DEFINE_SCRIPTFUNC(HUD_ShouldDraw, "Returns CHudElement ShouldDraw")
DEFINE_SCRIPTFUNC(HUD_IsActive, "Returns CHudElement active state")
DEFINE_SCRIPTFUNC(HUD_SetActive, "Sets CHudElement active state")
DEFINE_SCRIPTFUNC(HUD_SetHiddenBits, "Sets HIDEHUD hidden bits")
DEFINE_SCRIPTFUNC(HUD_SetNeedsRemove, "Marks HUD element for removal")
DEFINE_SCRIPTFUNC(HUD_IsParentedToClientDLLRootPanel, "Returns parented-to-clientdll-root state")
DEFINE_SCRIPTFUNC(HUD_SetParentedToClientDLLRootPanel, "Sets parented-to-clientdll-root state")
DEFINE_SCRIPTFUNC(HUD_RegisterForRenderGroup, "Registers HUD element for render group")
DEFINE_SCRIPTFUNC(HUD_UnregisterForRenderGroup, "Unregisters HUD element from render group")
DEFINE_SCRIPTFUNC(HUD_HideLowerPriorityHudElementsInGroup, "Hides lower priority HUD elements")
DEFINE_SCRIPTFUNC(HUD_UnhideLowerPriorityHudElementsInGroup, "Unhides lower priority HUD elements")
DEFINE_SCRIPTFUNC(HUD_GetRenderGroupPriority, "Gets render group priority")
DEFINE_SCRIPTFUNC(HUD_SetPos, "")
DEFINE_SCRIPTFUNC(HUD_SetSize, "")
DEFINE_SCRIPTFUNC(HUD_SetBounds, "")
DEFINE_SCRIPTFUNC(HUD_GetX, "")
DEFINE_SCRIPTFUNC(HUD_GetY, "")
DEFINE_SCRIPTFUNC(HUD_GetWide, "")
DEFINE_SCRIPTFUNC(HUD_GetTall, "")
DEFINE_SCRIPTFUNC(HUD_SetAlpha, "")
DEFINE_SCRIPTFUNC(HUD_GetAlpha, "")
DEFINE_SCRIPTFUNC(HUD_SetZPos, "")
DEFINE_SCRIPTFUNC(HUD_GetZPos, "")
DEFINE_SCRIPTFUNC(HUD_SetBgColor, "")
DEFINE_SCRIPTFUNC(HUD_SetFgColor, "")
DEFINE_SCRIPTFUNC(HUD_SetPaintBackgroundEnabled, "")
DEFINE_SCRIPTFUNC(HUD_SetPaintBorderEnabled, "")
DEFINE_SCRIPTFUNC(HUD_SetMouseInputEnabled, "")
DEFINE_SCRIPTFUNC(HUD_SetKeyboardInputEnabled, "")
DEFINE_SCRIPTFUNC(HUD_MoveToFront, "")
DEFINE_SCRIPTFUNC(HUD_RequestFocus, "")
DEFINE_SCRIPTFUNC(HUD_IsEnabled, "")
DEFINE_SCRIPTFUNC(HUD_SetEnabled, "")
DEFINE_SCRIPTFUNC(HUD_IsHiddenByBits, "")
DEFINE_SCRIPTFUNC(HUD_GetHiddenBits, "")
DEFINE_SCRIPTFUNC(HUD_AddHiddenBits, "")
DEFINE_SCRIPTFUNC(HUD_RemoveHiddenBits, "")
DEFINE_SCRIPTFUNC(HUD_ClearHiddenBits, "")
DEFINE_SCRIPTFUNC(HUD_ToggleActive, "")
DEFINE_SCRIPTFUNC(HUD_Remove, "")
DEFINE_SCRIPTFUNC(HUD_ForceShow, "")
DEFINE_SCRIPTFUNC(HUD_ForceHide, "")
DEFINE_SCRIPTFUNC(HUD_IsDrawableNow, "")
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
static float Script_Time()
{
	return gpGlobals ? gpGlobals->curtime : 0.0f;
}

static void Script_Msg(const char* s) { Msg("%s", s ? s : ""); }
static void Script_Warning(const char* s) { Warning("%s", s ? s : ""); }
static void Script_Log(const char* s) { Log("%s", s ? s : ""); }
static void Script_EngineError(const char* s) { Error("%s", s ? s : ""); }

static void Script_DMsg(const char* group, int level, const char* s)
{
	DMsg(group ? group : "script", level, "%s", s ? s : "");
}

static void Script_DWarning(const char* group, int level, const char* s)
{
	DWarning(group ? group : "script", level, "%s", s ? s : "");
}

static void Script_DLog(const char* group, int level, const char* s)
{
	DLog(group ? group : "script", level, "%s", s ? s : "");
}

static void Script_DevMsg(int level, const char* s)
{
	DevMsg(level, "%s", s ? s : "");
}

static void Script_DevWarning(int level, const char* s)
{
	DevWarning(level, "%s", s ? s : "");
}

static void Script_DevLog(int level, const char* s)
{
	DevLog(level, "%s", s ? s : "");
}

static void Script_ConMsg(int level, const char* s)
{
	ConMsg(level, "%s", s ? s : "");
}

static void Script_ConWarning(int level, const char* s)
{
	ConWarning(level, "%s", s ? s : "");
}

static void Script_ConLog(int level, const char* s)
{
	ConLog(level, "%s", s ? s : "");
}

static void Script_ConColorMsg(int level, int r, int g, int b, int a, const char* s)
{
	ConColorMsg(level, Color(clamp(r, 0, 255), clamp(g, 0, 255), clamp(b, 0, 255), clamp(a, 0, 255)), "%s", s ? s : "");
}

static void Script_ConDMsg(const char* s)
{
	ConDMsg("%s", s ? s : "");
}

static void Script_ConDWarning(const char* s)
{
	ConDWarning("%s", s ? s : "");
}

static void Script_ConDLog(const char* s)
{
	ConDLog("%s", s ? s : "");
}

static void Script_ConDColorMsg(int r, int g, int b, int a, const char* s)
{
	ConDColorMsg(Color(clamp(r, 0, 255), clamp(g, 0, 255), clamp(b, 0, 255), clamp(a, 0, 255)), "%s", s ? s : "");
}

static void Script_NetMsg(int level, const char* s)
{
	NetMsg(level, "%s", s ? s : "");
}

static void Script_NetWarning(int level, const char* s)
{
	NetWarning(level, "%s", s ? s : "");
}

static void Script_NetLog(int level, const char* s)
{
	NetLog(level, "%s", s ? s : "");
}

static void Script_Assert(bool condition)
{
	Assert(condition);
}

static void Script_AssertMsg(bool condition, const char* msg)
{
	AssertMsg(condition, "%s", msg ? msg : "Script assertion failed");
}

static void Script_AssertMsgOnce(bool condition, const char* msg)
{
	AssertMsgOnce(condition, "%s", msg ? msg : "Script assertion failed once");
}

static void Script_AssertFatalMsg(bool condition, const char* msg)
{
	AssertFatalMsg(condition, "%s", msg ? msg : "Fatal script assertion failed");
}

static void Script_Verify(bool condition)
{
	Verify(condition);
}

static void Script_ErrorIfNot(bool condition, const char* msg)
{
	if (!condition)
		Error("%s", msg ? msg : "Script ErrorIfNot failed");
}

static void Script_DebuggerBreakIfDebugging()
{
	DebuggerBreakIfDebugging();
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

static bool Script_IsTakingScreenshot()
{
	return engine->IsTakingScreenshot();
}

static void Script_TakeScreenshot()
{
	engine->ClientCmd_Unrestricted("jpeg");
}

// IVEngineClient Exposures

static float Script_EngineTime() { return engine ? engine->Time() : 0.0f; }
static float Script_GetLastTimeStamp() { return engine ? engine->GetLastTimeStamp() : 0.0f; }

static float Script_GetViewPitch()
{
	QAngle ang;
	engine->GetViewAngles(ang);
	return ang.x;
}

static float Script_GetViewYaw()
{
	QAngle ang;
	engine->GetViewAngles(ang);
	return ang.y;
}

static float Script_GetViewRoll()
{
	QAngle ang;
	engine->GetViewAngles(ang);
	return ang.z;
}

static void Script_SetViewAngles(float pitch, float yaw, float roll)
{
	QAngle ang(pitch, yaw, roll);
	engine->SetViewAngles(ang);
}

static const char* Script_GetLevelName()
{
	return engine ? engine->GetLevelName() : "";
}

static int Script_GetLevelVersion()
{
	return engine ? engine->GetLevelVersion() : 0;
}

static int Script_LevelLeafCount()
{
	return engine ? engine->LevelLeafCount() : 0;
}

static bool Script_IsPaused()
{
	return engine ? engine->IsPaused() : false;
}

static bool Script_IsHLTV()
{
	return engine ? engine->IsHLTV() : false;
}

static bool Script_IsPlayingDemo()
{
	return engine ? engine->IsPlayingDemo() : false;
}

static bool Script_IsRecordingDemo()
{
	return engine ? engine->IsRecordingDemo() : false;
}

static bool Script_IsPlayingTimeDemo()
{
	return engine ? engine->IsPlayingTimeDemo() : false;
}

static int Script_GetDemoRecordingTick()
{
	return engine ? engine->GetDemoRecordingTick() : 0;
}

static int Script_GetDemoPlaybackTick()
{
	return engine ? engine->GetDemoPlaybackTick() : 0;
}

static int Script_GetDemoPlaybackStartTick()
{
	return engine ? engine->GetDemoPlaybackStartTick() : 0;
}

static float Script_GetDemoPlaybackTimeScale()
{
	return engine ? engine->GetDemoPlaybackTimeScale() : 0.0f;
}

static int Script_GetDemoPlaybackTotalTicks()
{
	return engine ? engine->GetDemoPlaybackTotalTicks() : 0;
}

static bool Script_IsLevelMainMenuBackground()
{
	return engine ? engine->IsLevelMainMenuBackground() : false;
}

static const char* Script_GetMainMenuBackgroundName()
{
	static char buf[256];
	buf[0] = 0;

	if (engine)
		engine->GetMainMenuBackgroundName(buf, sizeof(buf));

	return buf;
}

static float Script_GetScreenAspectRatio()
{
	return engine ? engine->GetScreenAspectRatio() : 0.0f;
}

static unsigned int Script_GetEngineBuildNumber()
{
	return engine ? engine->GetEngineBuildNumber() : 0;
}

static const char* Script_GetProductVersionString()
{
	return engine ? engine->GetProductVersionString() : "";
}

static int Script_GetDXSupportLevel()
{
	return engine ? engine->GetDXSupportLevel() : 0;
}

static bool Script_SupportsHDR()
{
	return engine ? engine->SupportsHDR() : false;
}

static bool Script_MapHasHDRLighting()
{
	return engine ? engine->MapHasHDRLighting() : false;
}

static bool Script_IsHammerRunning()
{
	return engine ? engine->IsHammerRunning() : false;
}

static bool Script_IsWindowedMode()
{
	return engine ? engine->IsWindowedMode() : false;
}

static bool Script_IsActiveApp()
{
	return engine ? engine->IsActiveApp() : false;
}

static int Script_GetClientVersion()
{
	return engine ? engine->GetClientVersion() : 0;
}

static int Script_GetInstancesRunningCount()
{
	return engine ? engine->GetInstancesRunningCount() : 0;
}

static void Script_ExecuteClientCmd(const char* cmd)
{
	if (engine && cmd && cmd[0])
		engine->ExecuteClientCmd(cmd);
}

static void Script_ServerCmd(const char* cmd)
{
	if (engine && cmd && cmd[0])
		engine->ServerCmd(cmd, true);
}

static void Script_ClientCmd(const char* cmd)
{
	if (engine && cmd && cmd[0])
		engine->ClientCmd(cmd);
}

static void Script_ClientCmdUnrestricted(const char* cmd)
{
	if (engine && cmd && cmd[0])
		engine->ClientCmd_Unrestricted(cmd);
}

static const char* Script_KeyLookupBinding(const char* binding)
{
	if (!engine || !binding)
		return "";

	const char* out = engine->Key_LookupBinding(binding);
	return out ? out : "";
}

static const char* Script_KeyLookupBindingExact(const char* binding)
{
	if (!engine || !binding)
		return "";

	const char* out = engine->Key_LookupBindingExact(binding);
	return out ? out : "";
}

static int Script_GetPlayerForUserID(int userid)
{
	return engine ? engine->GetPlayerForUserID(userid) : 0;
}

static const char* Script_GetPlayerName(int entIndex)
{
	static char buf[MAX_PLAYER_NAME_LENGTH];
	buf[0] = 0;

	if (!engine)
		return "";

	player_info_t info;
	if (!engine->GetPlayerInfo(entIndex, &info))
		return "";

	Q_strncpy(buf, info.name, sizeof(buf));
	return buf;
}

static int Script_GetPlayerUserID(int entIndex)
{
	if (!engine)
		return 0;

	player_info_t info;
	if (!engine->GetPlayerInfo(entIndex, &info))
		return 0;

	return info.userID;
}

static bool Script_IsPlayerFake(int entIndex)
{
	if (!engine)
		return false;

	player_info_t info;
	if (!engine->GetPlayerInfo(entIndex, &info))
		return false;

	return info.fakeplayer;
}

static bool Script_IsPlayerHLTV(int entIndex)
{
	if (!engine)
		return false;

	player_info_t info;
	if (!engine->GetPlayerInfo(entIndex, &info))
		return false;

	return info.ishltv;
}

static void Script_CheckPoint(const char* name)
{
	if (engine && name && name[0])
		engine->CheckPoint(name);
}

static void Script_DrawPortals()
{
	if (engine)
		engine->DrawPortals();
}

static void Script_FlashWindow()
{
	if (engine)
		engine->FlashWindow();
}

static void Script_TakeNamedScreenshot(const char* filename)
{
	if (engine && filename && filename[0])
		engine->TakeScreenshot(filename);
}

static bool Script_StartDemoRecording(const char* filename)
{
	if (!engine || !filename || !filename[0])
		return false;

	return engine->StartDemoRecording(filename);
}

static void Script_StopDemoRecording()
{
	if (engine)
		engine->StopDemoRecording();
}

static void Script_Con_NPrintf(int pos, const char* text)
{
	if (!engine || !text)
		return;

	engine->Con_NPrintf(pos, "%s", text);
}

static void Script_Con_NXPrintf(
	int pos,
	float duration,
	float r,
	float g,
	float b,
	const char* text)
{
	if (!engine || !text)
		return;

	con_nprint_s info;
	info.index = pos;
	info.time_to_live = duration;
	info.fixed_width_font = false;
	info.color[0] = clamp(r, 0.0f, 1.0f);
	info.color[1] = clamp(g, 0.0f, 1.0f);
	info.color[2] = clamp(b, 0.0f, 1.0f);

	engine->Con_NXPrintf(&info, "%s", text);
}

static int Script_RandomInt(int min, int max)
{
	return RandomInt(min, max);
}

static float Script_RandomFloat(float min, float max)
{
	return RandomFloat(min, max);
}

static float Script_RandomFloatExp(float min, float max)
{
	return RandomFloatExp(min, max);
}

static float Script_RandomGaussianFloat(float mean, float stddev)
{
	return RandomGaussianFloat(mean, stddev);
}

static void Script_RandomSeed(int seed)
{
	RandomSeed(seed);
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
		Script_Time,
		"Time",
		"Returns client curtime"
	);

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Msg, "Msg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Warning, "Warning", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Log, "Log", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_EngineError, "EngineError", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DMsg, "DMsg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DWarning, "DWarning", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DLog, "DLog", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DevMsg, "DevMsg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DevWarning, "DevWarning", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DevLog, "DevLog", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConMsg, "ConMsg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConWarning, "ConWarning", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConLog, "ConLog", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConColorMsg, "ConColorMsg", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConDMsg, "ConDMsg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConDWarning, "ConDWarning", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConDLog, "ConDLog", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ConDColorMsg, "ConDColorMsg", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NetMsg, "NetMsg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NetWarning, "NetWarning", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_NetLog, "NetLog", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Assert, "Assert", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_AssertMsg, "AssertMsg", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_AssertMsgOnce, "AssertMsgOnce", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_AssertFatalMsg, "AssertFatalMsg", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Verify, "Verify", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ErrorIfNot, "ErrorIfNot", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DebuggerBreakIfDebugging, "DebuggerBreakIfDebugging", "");

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

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RandomInt, "RandomInt", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RandomFloat, "RandomFloat", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RandomFloatExp, "RandomFloatExp", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RandomGaussianFloat, "RandomGaussianFloat", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_RandomSeed, "RandomSeed", "");

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

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsTakingScreenshot, "IsTakingScreenshot", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_TakeScreenshot, "TakeScreenshot", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_EngineTime, "EngineTime", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetLastTimeStamp, "GetLastTimeStamp", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetViewPitch, "GetViewPitch", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetViewYaw, "GetViewYaw", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetViewRoll, "GetViewRoll", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_SetViewAngles, "SetViewAngles", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetLevelName, "GetLevelName", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetLevelVersion, "GetLevelVersion", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_LevelLeafCount, "LevelLeafCount", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsConsoleVisible, "IsConsoleVisible", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsPaused, "IsPaused", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsHLTV, "IsHLTV", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsPlayingDemo, "IsPlayingDemo", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsRecordingDemo, "IsRecordingDemo", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsPlayingTimeDemo, "IsPlayingTimeDemo", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetDemoRecordingTick, "GetDemoRecordingTick", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetDemoPlaybackTick, "GetDemoPlaybackTick", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetDemoPlaybackStartTick, "GetDemoPlaybackStartTick", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetDemoPlaybackTimeScale, "GetDemoPlaybackTimeScale", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetDemoPlaybackTotalTicks, "GetDemoPlaybackTotalTicks", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsLevelMainMenuBackground, "IsLevelMainMenuBackground", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetMainMenuBackgroundName, "GetMainMenuBackgroundName", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetScreenAspectRatio, "GetScreenAspectRatio", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetEngineBuildNumber, "GetEngineBuildNumber", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetProductVersionString, "GetProductVersionString", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetDXSupportLevel, "GetDXSupportLevel", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_SupportsHDR, "SupportsHDR", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_MapHasHDRLighting, "MapHasHDRLighting", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsHammerRunning, "IsHammerRunning", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsWindowedMode, "IsWindowedMode", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsActiveApp, "IsActiveApp", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetClientVersion, "GetClientVersion", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetInstancesRunningCount, "GetInstancesRunningCount", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ExecuteClientCmd, "ExecuteClientCmd", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ServerCmd, "ServerCmd", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ClientCmd, "ClientCmd", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_ClientCmdUnrestricted, "ClientCmdUnrestricted", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_KeyLookupBinding, "KeyLookupBinding", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_KeyLookupBindingExact, "KeyLookupBindingExact", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetPlayerForUserID, "GetPlayerForUserID", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetPlayerName, "GetPlayerName", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_GetPlayerUserID, "GetPlayerUserID", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsPlayerFake, "IsPlayerFake", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_IsPlayerHLTV, "IsPlayerHLTV", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_CheckPoint, "CheckPoint", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_DrawPortals, "DrawPortals", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_FlashWindow, "FlashWindow", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_TakeNamedScreenshot, "TakeNamedScreenshot", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_StartDemoRecording, "StartDemoRecording", "");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_StopDemoRecording, "StopDemoRecording", "");

	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Con_NPrintf, "Con_NPrintf", "Prints debug text at indexed screen position");
	ScriptRegisterFunctionNamed(g_pScriptVM, Script_Con_NXPrintf, "Con_NXPrintf", "Prints colored debug text at indexed screen position");

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

CON_COMMAND_F(cl_script_execute, "Run a client-side VScript file", FCVAR_NONE)
{
	if (args.ArgC() < 2)
	{
		Msg("Usage: client_script_execute <scriptname>\n");
		Msg("Example: client_script_execute test\n");
		return;
	}

	VScriptClientRunScript(args[1]);
}

CON_COMMAND(cl_script, "Run the text as a script")
{
	if (!*args[1])
	{
		Log_Warning(LOG_VScript, "No function name specified\n");
		return;
	}

	if (!g_pScriptVM)
	{
		Log_Warning(LOG_VScript, "Scripting disabled or no server running\n");
		return;
	}

	const char* pszScript = args.GetCommandString();

	pszScript += 13;

	while (*pszScript == ' ')
	{
		pszScript++;
	}

	if (!*pszScript)
	{
		return;
	}

	if (*pszScript != '\"')
	{
		g_pScriptVM->Run(pszScript);
	}
	else
	{
		pszScript++;
		const char* pszEndQuote = pszScript;
		while (*pszEndQuote != '\"')
		{
			pszEndQuote++;
		}
		if (!*pszEndQuote)
		{
			return;
		}
		*((char*)pszEndQuote) = 0;
		g_pScriptVM->Run(pszScript);
		*((char*)pszEndQuote) = '\"';
	}
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

		CFmtStr clCmd("cl_script_execute %s", script);
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

bool g_bVscriptGameDebugEnabled;

void CL_VScriptServerToggleGameDebug()
{
	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	g_bVscriptGameDebugEnabled = !g_bVscriptGameDebugEnabled;
	Msg("cl_script_debug %s\n", (g_bVscriptGameDebugEnabled) ? "ON" : "OFF");
	const char* pszFunction = (g_bVscriptGameDebugEnabled) ? "BeginScriptDebug" : "EndScriptDebug";
	HSCRIPT hFunction = g_pScriptVM->LookupFunction(pszFunction);
	if (!hFunction)
		return;
	g_pScriptVM->ExecuteFunction(hFunction, NULL, 0, NULL, NULL, true);
	g_pScriptVM->ReleaseFunction(hFunction);
}

CON_COMMAND_F(cl_script_attach_debugger, "Connect the vscript VM to the script debugger", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}
	g_pScriptVM->ConnectDebugger();
}

CON_COMMAND_F(cl_script_debug, "Toggle the in-game script debug features", FCVAR_CHEAT)
{

	if (tolower(*args[1]) == 'w'/*atch*/)
	{
		g_pScriptVM->Run("ScriptDebugDrawWatchesEnabled = !ScriptDebugDrawWatchesEnabled");
		return;
	}
	else if (tolower(*args[1]) == 'm'/*essages*/)
	{
		g_pScriptVM->Run("ScriptDebugDrawTextEnabled = !ScriptDebugDrawTextEnabled");
		return;
	}

	CL_VScriptServerToggleGameDebug();
}

CON_COMMAND_F(cl_script_add_watch, "Add a watch to the game debug overlay", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugAddWatch( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_remove_watch, "Remove a watch from the game debug overlay", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugRemoveWatch( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_add_watch_pattern, "Add a watch to the game debug overlay", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugAddWatchPattern( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_remove_watch_pattern, "Remove a watch from the game debug overlay", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugRemoveWatchPattern( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_add_debug_filter, "Add a filter to the game debug overlay", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugAddTextFilter( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_remove_debug_filter, "Remove a filter from the game debug overlay", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugRemoveTextFilter( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_trace_enable, "Turn on a particular trace output by file or function name", FCVAR_CHEAT)
{
	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	if (!*args[1])
	{
		Msg("No key specified\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugAddTrace( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_trace_disable, "Turn off a particular trace output by file or function name", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	if (!*args[1])
	{
		Msg("No key specified\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugRemoveTrace( \"%s\" )", pszScript));
}

CON_COMMAND_F(cl_script_trace_enable_key, "Turn on a particular trace output by table/instance", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	if (!*args[1])
	{
		Msg("No key specified\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugAddTrace( %s )", pszScript));
}

CON_COMMAND_F(cl_script_trace_disable_key, "Turn off a particular trace output by table/instance", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	if (!*args[1])
	{
		Msg("No key specified\n");
		return;
	}

	const char* pszScript = args[1];
	g_pScriptVM->Run(CFmtStr("ScriptDebugRemoveTrace( %s )", pszScript));
}

CON_COMMAND_F(cl_script_trace_enable_all, "Turn on all trace output", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	g_pScriptVM->Run("ScriptDebugTraceAll( true )");
}

CON_COMMAND_F(cl_script_trace_disable_all, "Turn off all trace output", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	g_pScriptVM->Run("ScriptDebugTraceAll( false )");
}

CON_COMMAND_F(cl_script_clear_watches, "Clear all watches from the game debug overlay", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	g_pScriptVM->Run("ScriptDebugClearWatches()");
}

CON_COMMAND_F(cl_script_find, "Find a key in the VM ", FCVAR_CHEAT)
{

	if (!g_pScriptVM)
	{
		Msg("Scripting disabled or no server running\n");
		return;
	}

	if (!*args[1])
	{
		Msg("No key specified\n");
		return;
	}

	const char* pszFunction = "ScriptDebugDumpKeys";
	HSCRIPT hFunction = g_pScriptVM->LookupFunction(pszFunction);
	if (!hFunction)
		return;
	g_pScriptVM->Call(hFunction, NULL, true, NULL, args[1]);
	g_pScriptVM->ReleaseFunction(hFunction);
}