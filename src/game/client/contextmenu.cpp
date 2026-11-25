// GabeMod context menu - client DLL, Source SDK 2013 MP
#include "cbase.h"
#include "c_baseplayer.h"
#include "c_baseentity.h"
#include "view.h"
#include <math.h>
#include "util_shared.h"
#include "ienginevgui.h"
#include "vgui/ISurface.h"
#include "vgui/IInput.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Menu.h"
#include "vgui_controls/TextEntry.h"
#include "tier0/memdbgon.h"
using namespace vgui;

class CGabeContextMenu : public Panel
{
    DECLARE_CLASS_SIMPLE(CGabeContextMenu, Panel);
public:
    CGabeContextMenu(VPANEL parent) : BaseClass(NULL, "GabeContextMenu")
    {
        SetParent(parent);
        SetPaintBackgroundEnabled(false);
        SetPaintBorderEnabled(false);
        SetVisible(false);
        SetMouseInputEnabled(false);
        SetKeyBoardInputEnabled(false);
        m_pInfo = new Label(this, "Info", "Right-click an entity to edit it.");
        m_pInfo->SetMouseInputEnabled(false);
        m_pInfo->SetFgColor(Color(255, 255, 255, 255));
        m_pMenu = new Panel(this, "EntityProperties");
        m_pMenu->SetPaintBackgroundEnabled(true);
        m_pMenu->SetBgColor(Color(48, 48, 48, 245));
        m_pMenu->SetVisible(false);
        m_pMenu->SetKeyBoardInputEnabled(false);
        m_pTitle = new Label(m_pMenu, "Title", "Entity properties");
        m_pTitle->SetFgColor(Color(255, 255, 255, 255));
        Add("Physics", "physics"); Add("Appearance", "appearance");
        Add("Actions", "actions"); Add("Inspect", "inspect");
        m_iCategory = 0;
        BuildCategory();
        m_hTarget = NULL;
        m_bOpen = false;
    }
    void Add(const char* title, const char* cmd)
    {
        Button* button = new Button(m_pMenu, cmd, title, this, cmd);
        button->SetKeyBoardInputEnabled(false);
        button->SetVisible(true);
        m_Buttons.AddToTail(button);
    }
    void BuildCategory()
    {
        for (int i = m_Buttons.Count() - 1; i >= 4; --i)
        {
            m_Buttons[i]->SetVisible(false);
            m_Buttons[i]->MarkForDeletion();
            m_Buttons.Remove(i);
        }
        if (m_iCategory == 0)
        {
            Add("Freeze", "freeze"); Add("Unfreeze", "unfreeze");
            Add("Gravity off", "gravityoff"); Add("Gravity on", "gravityon");
            Add("Stop movement", "stop"); Add("Wake physics", "wake");
            Add("Mass: 1 kg", "mass1"); Add("Mass: 10 kg", "mass10");
            Add("Mass: 50 kg", "mass50"); Add("Mass: 200 kg", "mass200");
            Add("Mass: 1000 kg", "mass1000"); Add("Reset damping", "damping0");
            Add("High damping", "damping5"); Add("Debris collision", "collisionoff");
            Add("Normal collision", "collisionon");
        }
        else if (m_iCategory == 1)
        {
            Add("White", "white"); Add("Red", "red"); Add("Green", "green"); Add("Blue", "blue");
            Add("Yellow", "yellow"); Add("Purple", "purple"); Add("Black", "black"); Add("Orange", "orange");
            Add("Opaque", "opaque"); Add("50% opacity", "transparent");
            Add("Reset skin", "skinreset");
            Add("Cycle bodygroup 1", "bodygroup"); Add("Reset bodygroups", "bodyreset");
        }
        else
        {
            Add("Duplicate prop", "duplicate"); Add("Extinguish", "extinguish");
            Add("Ignite (5 seconds)", "ignite"); Add("Rotate 45 degrees", "rotate");
            Add("Undo last spawn", "undo"); Add("Remove entity", "remove");
        }
        LayoutEditor();
    }
    void LayoutEditor()
    {
        m_pMenu->SetSize(340, 270);
        m_pTitle->SetBounds(8, 4, 324, 22);
        for (int i = 0; i < m_Buttons.Count(); ++i)
        {
            if (i < 4) m_Buttons[i]->SetBounds(5 + i * 83, 30, 81, 24);
            else { int n = i - 4; m_Buttons[i]->SetBounds(6 + (n % 2) * 165, 60 + (n / 2) * 25, 162, 23); }
        }
    }
    void Open()
    {
        if (m_bOpen || !engine->IsInGame()) return;
        int w, h; surface()->GetScreenSize(w, h);
        SetBounds(0, 0, w, h);
        m_pInfo->SetBounds(18, 12, w - 36, 24);
        m_pInfo->SetText("Context Menu | Right-click an entity | Release C to close");
        m_hTarget = NULL;
        SetVisible(true);
        MakePopup(false);
        SetKeyBoardInputEnabled(false); // key-up must reach the engine binding
        SetMouseInputEnabled(true);
        MoveToFront();
        input()->SetCursorPos(w / 2, h / 2);
        m_bOpen = true;
    }
    void Close()
    {
        if (!m_bOpen) return;
        m_pMenu->SetVisible(false);
        SetMouseInputEnabled(false);
        SetKeyBoardInputEnabled(false);
        SetVisible(false);
        m_hTarget = NULL;
        m_bOpen = false;
    }
    virtual void OnThink()
    {
        BaseClass::OnThink();
        if (m_bOpen && (!engine->IsInGame() || !C_BasePlayer::GetLocalPlayer())) Close();
    }
    virtual void PaintBackground() {} // leave game visible
    virtual void Paint()
    {
        surface()->DrawSetColor(40, 40, 40, 220);
        surface()->DrawFilledRect(10, 8, MIN(GetWide() - 10, 650), 40);
        surface()->DrawSetColor(140, 140, 140, 255);
        surface()->DrawOutlinedRect(10, 8, MIN(GetWide() - 10, 650), 40);
    }
    virtual void OnMousePressed(MouseCode code)
    {
        if (!m_bOpen) return;
        if (code == MOUSE_LEFT) { m_pMenu->SetVisible(false); return; }
        if (code != MOUSE_RIGHT) return;
        C_BasePlayer* player = C_BasePlayer::GetLocalPlayer();
        if (!player) return;
        int x, y, w, h; input()->GetCursorPos(x, y); surface()->GetScreenSize(w, h);
        // Construct a world ray from the cursor and current render camera.
        // FOV is the horizontal FOV at the engine's 4:3 reference aspect.
        Vector forward, right, up;
        AngleVectors(MainViewAngles(), &forward, &right, &up);
        float fov = player->GetFOV();
        float tangent = tanf(DEG2RAD(fov) * 0.5f);
        float vertical = tangent * 0.75f;
        float horizontal = vertical * float(w) / MAX(1, h);
        Vector direction = forward + right * ((2.0f * x / MAX(1, w) - 1.0f) * horizontal)
            + up * ((1.0f - 2.0f * y / MAX(1, h)) * vertical);
        VectorNormalize(direction);
        trace_t tr;
        Vector origin = MainViewOrigin();
        UTIL_TraceLine(origin, origin + direction * 2048.0f, MASK_SHOT, player, COLLISION_GROUP_NONE, &tr);
        C_BaseEntity* e = tr.m_pEnt;
        if (!e || e->entindex() <= 0 || e->IsPlayer())
        {
            m_hTarget = NULL;
            m_pMenu->SetVisible(false);
            m_pInfo->SetText("No editable entity under cursor.");
            return;
        }
        m_hTarget = e;
        char text[256];
        Q_snprintf(text, sizeof(text), "Selected entity #%d | model index %d", e->entindex(), e->GetModelIndex());
        m_pInfo->SetText(text);
        m_pTitle->SetText(text);
        m_pMenu->SetPos(clamp(x, 0, MAX(0, w - 340)), clamp(y, 0, MAX(0, h - 270)));
        m_pMenu->SetVisible(true);
        m_pMenu->MoveToFront();
    }
    virtual void OnCommand(const char* command)
    {
        if (!Q_stricmp(command, "physics") || !Q_stricmp(command, "appearance") || !Q_stricmp(command, "actions"))
        {
            m_iCategory = !Q_stricmp(command, "physics") ? 0 : (!Q_stricmp(command, "appearance") ? 1 : 2);
            BuildCategory(); return;
        }
        C_BaseEntity* e = m_hTarget.Get();
        if (m_bOpen && e)
        {
            // Commands are fixed Menu entries; the server validates independently.
            char cmd[128]; Q_snprintf(cmd, sizeof(cmd), "gabe_context_action %d %s\n", e->entindex(), command);
            engine->ClientCmd_Unrestricted(cmd);
            if (!Q_stricmp(command, "remove")) { m_pMenu->SetVisible(false); m_hTarget = NULL; }
        }
    }
private:
    Label* m_pInfo;
    Panel* m_pMenu;
    Label* m_pTitle;
    CUtlVector<Button*> m_Buttons;
    int m_iCategory;
    CHandle<C_BaseEntity> m_hTarget;
    bool m_bOpen;
};
static CGabeContextMenu* g_pContext = NULL;
static void OpenContext()
{
    if (!engine->IsInGame()) return;
    if (!g_pContext) g_pContext = new CGabeContextMenu(enginevgui->GetPanel(PANEL_INGAMESCREENS));
    g_pContext->Open();
}
static void CloseContext() { if (g_pContext) g_pContext->Close(); }
static ConCommand context_open("+gabe_contextmenu", OpenContext, "Open sandbox context menu", FCVAR_CLIENTDLL);
static ConCommand context_close("-gabe_contextmenu", CloseContext, "Close sandbox context menu", FCVAR_CLIENTDLL);
