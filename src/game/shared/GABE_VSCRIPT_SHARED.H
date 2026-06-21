#include "cbase.h"
#include "eiface.h"
#include "icommandline.h"
#include "filesystem.h"
#include "tier1/fmtstr.h"
#include "tier1/utlbuffer.h"
#include "mathlib/mathlib.h"
#include "convar.h"
#ifdef CLIENT_DLL
#include "iclientmode.h"
#include "vgui/ISurface.h"
#endif
#include "KeyValues.h"
#include "../../public/vscript/vscript_templates.h"

static Vector V(float x, float y, float z)
{
	return Vector(x, y, z);
}

static QAngle A(float p, float y, float r)
{
	return QAngle(p, y, r);
}

#ifdef CLIENT_DLL

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

static vgui::HFont g_hScriptSurfaceFont = vgui::INVALID_FONT;


static HSCRIPT Script_VGUI_CreateHTML(
	CScriptVPanel* pParent,
	int x,
	int y,
	int w,
	int h,
	bool allowJS = false);

static void Script_HTML_OpenURL(
	CScriptVPanel* pPanel,
	const char* url,
	const char* postData,
	bool force);

static bool Script_HTML_StopLoading(CScriptVPanel* pPanel);
static bool Script_HTML_Refresh(CScriptVPanel* pPanel);
static void Script_HTML_OnMove(CScriptVPanel* pPanel);
static void Script_HTML_RunJavascript(CScriptVPanel* pPanel, const char* js);
static void Script_HTML_GoBack(CScriptVPanel* pPanel);
static void Script_HTML_GoForward(CScriptVPanel* pPanel);
static bool Script_HTML_BCanGoBack(CScriptVPanel* pPanel);
static bool Script_HTML_BCanGoForward(CScriptVPanel* pPanel);

static bool Script_HTML_OnStartRequest(
	CScriptVPanel* pPanel,
	const char* url,
	const char* target,
	const char* postData,
	bool redirect);

static void Script_HTML_OnSetHTMLTitle(
	CScriptVPanel* pPanel,
	const char* title);

static void Script_HTML_OnLinkAtPosition(
	CScriptVPanel* pPanel,
	const char* url);

static void Script_HTML_OnURLChanged(
	CScriptVPanel* pPanel,
	const char* url,
	const char* postData,
	bool redirect);

static bool Script_HTML_OnOpenNewTab(
	CScriptVPanel* pPanel,
	const char* url,
	bool foreground);

static void Script_HTML_SetScrollbarsEnabled(
	CScriptVPanel* pPanel,
	bool state);

static void Script_HTML_SetContextMenuEnabled(
	CScriptVPanel* pPanel,
	bool state);

static void Script_HTML_SetViewSourceEnabled(
	CScriptVPanel* pPanel,
	bool state);

static void Script_HTML_NewWindowsOnly(
	CScriptVPanel* pPanel,
	bool state);

static bool Script_HTML_IsScrolledToBottom(
	CScriptVPanel* pPanel);

static bool Script_HTML_IsScrollbarVisible(
	CScriptVPanel* pPanel);

static void Script_HTML_AddCustomURLHandler(
	CScriptVPanel* pPanel,
	const char* protocol,
	CScriptVPanel* pTarget);

static void Script_HTML_Paint(CScriptVPanel* pPanel);

static void Script_HTML_OnSizeChanged(
	CScriptVPanel* pPanel,
	int wide,
	int tall);

static void Script_HTML_OnMousePressed(
	CScriptVPanel* pPanel,
	int code);

static void Script_HTML_OnMouseReleased(
	CScriptVPanel* pPanel,
	int code);

static void Script_HTML_OnCursorMoved(
	CScriptVPanel* pPanel,
	int x,
	int y);

static void Script_HTML_OnMouseDoublePressed(
	CScriptVPanel* pPanel,
	int code);

static void Script_HTML_OnKeyTyped(
	CScriptVPanel* pPanel,
	int unichar);

static void Script_HTML_OnKeyCodeTyped(
	CScriptVPanel* pPanel,
	int code);

static void Script_HTML_OnKeyCodeReleased(
	CScriptVPanel* pPanel,
	int code);

static void Script_HTML_PerformLayout(
	CScriptVPanel* pPanel);

static void Script_HTML_OnMouseWheeled(
	CScriptVPanel* pPanel,
	int delta);

static void Script_HTML_PostChildPaint(
	CScriptVPanel* pPanel);

static void Script_HTML_OnCommand(
	CScriptVPanel* pPanel,
	const char* command);

static void Script_HTML_AddHeader(
	CScriptVPanel* pPanel,
	const char* header,
	const char* value);

static void Script_HTML_OnKillFocus(
	CScriptVPanel* pPanel);

static void Script_HTML_OnSetFocus(
	CScriptVPanel* pPanel);

static void Script_HTML_Find(
	CScriptVPanel* pPanel,
	const char* text);

static void Script_HTML_StopFind(
	CScriptVPanel* pPanel);

static void Script_HTML_FindNext(
	CScriptVPanel* pPanel);

static void Script_HTML_FindPrevious(
	CScriptVPanel* pPanel);

static void Script_HTML_ShowFindDialog(
	CScriptVPanel* pPanel);

static void Script_HTML_HideFindDialog(
	CScriptVPanel* pPanel);

static bool Script_HTML_FindDialogVisible(
	CScriptVPanel* pPanel);

static int Script_HTML_HorizontalScrollMax(
	CScriptVPanel* pPanel);

static int Script_HTML_VerticalScrollMax(
	CScriptVPanel* pPanel);

static void Script_HTML_GetLinkAtPosition(
	CScriptVPanel* pPanel,
	int x,
	int y);

static void Script_VGUI_SetSizeable(CScriptVPanel* pPanel, bool state);
#endif // CLIENT_DLL

static FileFindHandle_t g_ScriptFindHandle = FILESYSTEM_INVALID_FIND_HANDLE;

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

#ifdef CLIENT_DLL
static float Script_Time()
{
	return gpGlobals ? gpGlobals->curtime : 0.0f;
}
#endif

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

#ifdef CLIENT_DLL

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
#endif // CLIENT_DLL

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

#ifdef CLIENT_DLL

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
#endif

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

#ifdef CLIENT_DLL
static int Script_GetBuildNumber()
{
	return engine->GetEngineBuildNumber();
}
#endif

#ifndef CLIENT_DLL
static void Script_ServerCommand(const char* cmd)
{
	if (cmd && cmd[0])
		engine->ServerCommand(cmd);
}
#endif
#ifdef CLIENT_DLL

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

#endif

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

#ifdef CLIENT_DLL

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
#endif

static void Script_Con_NPrintf(int pos, const char* text)
{
	if (!engine || !text)
		return;

	engine->Con_NPrintf(pos, "%s", text);
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
