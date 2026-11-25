//=========================================================
// Gabe Mod - Notification HUD
// Source SDK Base 2007 / 2013
//
// GMod-style hints, undo messages and notifications.
//=========================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_baseplayer.h"
#include "filesystem.h"

#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui_controls/Panel.h>

#include "tier1/utlvector.h"
#include <stdlib.h>

#include "tier0/memdbgon.h"

using namespace vgui;

//=========================================================
// ConVars
//=========================================================

static ConVar gabe_hints(
    "gabe_hints", "1", FCVAR_ARCHIVE,
    "Enable startup hints.");

static ConVar gabe_notification_sounds(
    "gabe_notification_sounds", "1", FCVAR_ARCHIVE,
    "Enable notification sounds.");

static ConVar gabe_notification_duration(
    "gabe_notification_duration", "7", FCVAR_ARCHIVE,
    "Startup notification duration.");

static ConVar gabe_notification_y(
    "gabe_notification_y", "0.55", FCVAR_ARCHIVE,
    "Vertical notification position.");

static ConVar gabe_notification_alpha(
    "gabe_notification_alpha", "175", FCVAR_ARCHIVE,
    "Notification background opacity.");

static ConVar gabe_notification_max(
    "gabe_notification_max", "6", FCVAR_ARCHIVE,
    "Maximum visible notifications.");

static ConVar gabe_notification_startdelay(
    "gabe_notification_startdelay", "2", FCVAR_ARCHIVE,
    "Delay before startup hints appear.");

static ConVar gabe_notification_interval(
    "gabe_notification_interval", "3", FCVAR_ARCHIVE,
    "Interval between startup hints.");

//=========================================================
// Notification types
//=========================================================

enum GabeNotificationType
{
    GABE_NOTIFY_HINT = 0,
    GABE_NOTIFY_GENERIC,
    GABE_NOTIFY_ERROR,
    GABE_NOTIFY_UNDO
};

//=========================================================
// Startup hints
//=========================================================

static const char* g_StartupHints[] =
{
    "Press Q to open the spawn menu.",
    "Use the physics gun to move objects.",
    "Use the multitool to modify objects.",
    "Press Z to undo your last spawned object.",
    "You can disable hints with gabe_hints 0."
};

//=========================================================
// Notification data
//=========================================================

struct GabeNotification_t
{
    wchar_t text[256];

    int type;

    float startTime;
    float duration;
    float currentY;

    bool initialized;
};

//=========================================================
// HUD declaration
//=========================================================

class CHudGabeNotifications :
    public CHudElement,
    public Panel
{
    DECLARE_CLASS_SIMPLE(
        CHudGabeNotifications, Panel);

public:

    CHudGabeNotifications(
        const char* pElementName);

    virtual void Init();
    virtual void VidInit();

    virtual void LevelInit();
    virtual void LevelShutdown();

    virtual void OnThink();
    virtual void Paint();

    virtual bool ShouldDraw();

    virtual void ApplySchemeSettings(
        IScheme* pScheme);

    void AddNotification(
        const char* text,
        int type,
        float duration);

    void AddUndoNotification(
        const char* name);

    void ClearNotifications();

private:

    void PlayNotificationSound(int type);

    void UpdateScreenBounds();

    void DrawShadowedText(
        HFont font,
        const wchar_t* text,
        int x,
        int y,
        Color color);

    void DrawUndoIcon(
        int x,
        int y,
        int size,
        int alpha);

    CUtlVector<GabeNotification_t> m_Notifications;

    HFont m_hTextFont;
    HFont m_hIconFont;

    bool m_bStartupFinished;

    int m_iStartupIndex;

    float m_flNextHintTime;
};

DECLARE_HUDELEMENT(CHudGabeNotifications);

//=========================================================
// Constructor
//=========================================================

CHudGabeNotifications::CHudGabeNotifications(
    const char* pElementName)
    : CHudElement(pElementName),
    BaseClass(NULL, "HudGabeNotifications")
{
    SetParent(g_pClientMode->GetViewport());

    SetPaintBackgroundEnabled(false);
    SetPaintBorderEnabled(false);

    SetMouseInputEnabled(false);
    SetKeyBoardInputEnabled(false);

    SetProportional(false);

    SetHiddenBits(HIDEHUD_MISCSTATUS);

    m_hTextFont = INVALID_FONT;
    m_hIconFont = INVALID_FONT;

    m_bStartupFinished = false;
    m_iStartupIndex = 0;
    m_flNextHintTime = 0.0f;

    SetVisible(true);
}

//=========================================================
// Initialization
//=========================================================

void CHudGabeNotifications::Init()
{
    ClearNotifications();
}

void CHudGabeNotifications::VidInit()
{
    UpdateScreenBounds();

    InvalidateLayout(true, true);
}

void CHudGabeNotifications::LevelInit()
{
    ClearNotifications();

    m_bStartupFinished = false;
    m_iStartupIndex = 0;
    m_flNextHintTime = 0.0f;

    UpdateScreenBounds();

    SetVisible(true);
}

void CHudGabeNotifications::LevelShutdown()
{
    ClearNotifications();

    m_bStartupFinished = true;
    m_flNextHintTime = 0.0f;
}

//=========================================================
// Scheme
//=========================================================

void CHudGabeNotifications::ApplySchemeSettings(
    IScheme* pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    m_hTextFont = pScheme->GetFont(
        "GabeNotificationText", false);

    m_hIconFont = pScheme->GetFont(
        "GabeNotificationIcon", false);

    if (m_hTextFont == INVALID_FONT)
    {
        m_hTextFont = pScheme->GetFont(
            "DefaultBold", false);
    }

    if (m_hTextFont == INVALID_FONT)
    {
        m_hTextFont = pScheme->GetFont(
            "Default", false);
    }

    if (m_hIconFont == INVALID_FONT)
    {
        m_hIconFont = pScheme->GetFont(
            "DefaultLarge", false);
    }

    if (m_hIconFont == INVALID_FONT)
    {
        m_hIconFont = m_hTextFont;
    }

    SetPaintBackgroundEnabled(false);
    SetPaintBorderEnabled(false);

    UpdateScreenBounds();
}

//=========================================================
// Screen resolution
//=========================================================

void CHudGabeNotifications::UpdateScreenBounds()
{
    int w, h;

    surface()->GetScreenSize(w, h);

    if (w <= 0 || h <= 0)
        return;

    if (GetWide() != w || GetTall() != h)
    {
        SetBounds(0, 0, w, h);
    }
}

//=========================================================
// Sound
//=========================================================

void CHudGabeNotifications::PlayNotificationSound(
    int type)
{
    if (!gabe_notification_sounds.GetBool())
        return;

    char sound[128];

    Q_snprintf(
        sound,
        sizeof(sound),
        "ambient/water/drip%d.wav",
        RandomInt(1, 4));

    char path[256];

    Q_snprintf(
        path,
        sizeof(path),
        "sound/%s",
        sound);

    // Prevent missing sound warnings.

    if (filesystem &&
        filesystem->FileExists(path, "GAME"))
    {
        surface()->PlaySound(sound);
    }
}

//=========================================================
// Add generic notification
//=========================================================

void CHudGabeNotifications::AddNotification(
    const char* text,
    int type,
    float duration)
{
    if (!text || !text[0])
        return;

    GabeNotification_t notification;

    V_UTF8ToUnicode(
        text,
        notification.text,
        sizeof(notification.text));

    notification.type = clamp(
        type,
        (int)GABE_NOTIFY_HINT,
        (int)GABE_NOTIFY_UNDO);

    notification.startTime =
        gpGlobals->realtime;

    notification.duration =
        clamp(duration, 1.0f, 60.0f);

    notification.currentY = 0.0f;
    notification.initialized = false;

    const int maxNotifications = clamp(
        gabe_notification_max.GetInt(),
        1, 16);

    while (m_Notifications.Count() >=
        maxNotifications)
    {
        m_Notifications.Remove(0);
    }

    m_Notifications.AddToTail(notification);

    PlayNotificationSound(notification.type);

    Repaint();
}

//=========================================================
// Undo notification
//=========================================================

void CHudGabeNotifications::AddUndoNotification(
    const char* name)
{
    if (!name || !name[0])
        name = "Entity";

    char text[256];

    Q_snprintf(
        text,
        sizeof(text),
        "Undone %s",
        name);

    AddNotification(
        text,
        GABE_NOTIFY_UNDO,
        4.0f);
}

//=========================================================
// Clear notifications
//=========================================================

void CHudGabeNotifications::ClearNotifications()
{
    m_Notifications.RemoveAll();

    Repaint();
}

//=========================================================
// Visibility
//=========================================================

bool CHudGabeNotifications::ShouldDraw()
{
    // Do not check the notification count here.
    // The HUD must remain active so startup hints
    // can be generated even when the list is empty.

    if (!engine->IsInGame())
        return false;

    if (engine->IsLevelMainMenuBackground())
        return false;

    return CHudElement::ShouldDraw();
}

//=========================================================
// Startup hints and notification cleanup
//=========================================================

void CHudGabeNotifications::OnThink()
{
    BaseClass::OnThink();

    UpdateScreenBounds();

    const float now = gpGlobals->realtime;

    // Remove expired notifications.

    for (int i = m_Notifications.Count() - 1;
        i >= 0; --i)
    {
        const GabeNotification_t& n =
            m_Notifications[i];

        if (now >= n.startTime + n.duration)
        {
            m_Notifications.Remove(i);
        }
    }

    if (!engine->IsInGame())
        return;

    if (engine->IsLevelMainMenuBackground())
        return;

    C_BasePlayer* pPlayer =
        C_BasePlayer::GetLocalPlayer();

    if (!pPlayer || !pPlayer->IsAlive())
        return;

    if (!gabe_hints.GetBool() ||
        m_bStartupFinished)
    {
        return;
    }

    // Begin the sequence after entering the map.

    if (m_flNextHintTime == 0.0f)
    {
        m_flNextHintTime =
            now + MAX(
                0.0f,
                gabe_notification_startdelay.GetFloat());
    }

    if (now < m_flNextHintTime)
        return;

    if (m_iStartupIndex >=
        ARRAYSIZE(g_StartupHints))
    {
        m_bStartupFinished = true;
        return;
    }

    AddNotification(
        g_StartupHints[m_iStartupIndex],
        GABE_NOTIFY_HINT,
        gabe_notification_duration.GetFloat());

    ++m_iStartupIndex;

    m_flNextHintTime =
        now + MAX(
            0.1f,
            gabe_notification_interval.GetFloat());

    if (m_iStartupIndex >=
        ARRAYSIZE(g_StartupHints))
    {
        m_bStartupFinished = true;
    }
}

//=========================================================
// Shadowed text
//=========================================================

void CHudGabeNotifications::DrawShadowedText(
    HFont font,
    const wchar_t* text,
    int x,
    int y,
    Color color)
{
    if (!text || font == INVALID_FONT)
        return;

    const int length = wcslen(text);

    surface()->DrawSetTextFont(font);

    // Black shadow.

    surface()->DrawSetTextColor(
        0, 0, 0, color.a());

    surface()->DrawSetTextPos(
        x + 1, y + 1);

    surface()->DrawPrintText(
        text, length);

    // Foreground text.

    surface()->DrawSetTextColor(
        color.r(),
        color.g(),
        color.b(),
        color.a());

    surface()->DrawSetTextPos(x, y);

    surface()->DrawPrintText(
        text, length);
}

//=========================================================
// Draw undo arrow
// No external texture required.
//=========================================================

void CHudGabeNotifications::DrawUndoIcon(
    int x,
    int y,
    int size,
    int alpha)
{
    const int thickness = MAX(2, size / 8);

    const int left = x + size / 5;
    const int right = x + size * 4 / 5;

    const int top = y + size / 4;
    const int bottom = y + size * 3 / 4;

    const int middle = (top + bottom) / 2;

    surface()->DrawSetColor(
        255, 215, 75, alpha);

    // Horizontal section.

    surface()->DrawFilledRect(
        left,
        top,
        right,
        top + thickness);

    // Right side of the returning arrow.

    surface()->DrawFilledRect(
        right - thickness,
        top,
        right,
        bottom);

    surface()->DrawFilledRect(
        left + size / 4,
        bottom - thickness,
        right,
        bottom);

    // Arrowhead.

    for (int i = 0; i < thickness; ++i)
    {
        surface()->DrawLine(
            left,
            top + i,
            left + size / 4,
            middle + i);

        surface()->DrawLine(
            left,
            top + i,
            left + size / 4,
            y + i);
    }
}

//=========================================================
// Draw notifications
//=========================================================

void CHudGabeNotifications::Paint()
{
    if (m_hTextFont == INVALID_FONT)
        return;

    if (m_Notifications.Count() == 0)
        return;

    int sw, sh;

    GetSize(sw, sh);

    const float now = gpGlobals->realtime;

    const float scale = clamp(
        (float)sh / 768.0f,
        0.75f, 2.0f);

    const int textTall =
        surface()->GetFontTall(m_hTextFont);

    const int iconTall =
        (m_hIconFont != INVALID_FONT)
        ? surface()->GetFontTall(m_hIconFont)
        : textTall;

    const int rowH = MAX(
        textTall + (int)(10 * scale),
        iconTall + (int)(4 * scale));

    const int spacing = MAX(
        2, (int)(4 * scale));

    const int rightMargin =
        (int)(12 * scale);

    const int iconW = MAX(
        iconTall + 6,
        (int)(28 * scale));

    const int totalHeight =
        m_Notifications.Count() *
        (rowH + spacing);

    int targetY = (int)(
        sh * clamp(
            gabe_notification_y.GetFloat(),
            0.0f, 0.95f));

    // Prevent the notification stack from extending
    // below the bottom of the screen.

    targetY = MAX(
        0,
        MIN(targetY,
            sh - totalHeight - 8));

    for (int i = 0;
        i < m_Notifications.Count();
        ++i)
    {
        GabeNotification_t& n =
            m_Notifications[i];

        const float age =
            now - n.startTime;

        const float remaining =
            n.duration - age;

        if (remaining <= 0.0f)
            continue;

        // Fade-in and fade-out.

        const float fadeIn =
            clamp(age / 0.25f,
                0.0f, 1.0f);

        const float fadeOut =
            clamp(remaining / 0.65f,
                0.0f, 1.0f);

        const float opacity =
            fadeIn * fadeOut;

        const int alpha =
            (int)(opacity * 255.0f);

        int textW, textH;

        surface()->GetTextSize(
            m_hTextFont,
            n.text,
            textW,
            textH);

        const int boxW =
            textW + iconW +
            (int)(16 * scale);

        // Slide in from the right.

        float slide = 1.0f - fadeIn;

        slide *= slide;

        int x =
            sw - boxW - rightMargin +
            (int)(slide * (boxW + 20));

        // Slide out when expiring.

        if (remaining < 0.65f)
        {
            float exitSlide =
                1.0f - fadeOut;

            x += (int)(
                exitSlide * exitSlide *
                (boxW + rightMargin));
        }

        // Smooth vertical stacking.

        if (!n.initialized)
        {
            n.currentY = (float)targetY;
            n.initialized = true;
        }

        n.currentY = Approach(
            (float)targetY,
            n.currentY,
            MAX(0.0f, gpGlobals->frametime) *
            450.0f);

        const int y =
            (int)n.currentY;

        // Drop shadow.

        surface()->DrawSetColor(
            0, 0, 0,
            (int)(opacity * 75));

        surface()->DrawFilledRect(
            x + 2,
            y + 2,
            x + boxW + 2,
            y + rowH + 2);

        // Main translucent background.

        surface()->DrawSetColor(
            20, 20, 20,
            (int)(opacity * clamp(
                gabe_notification_alpha.GetInt(),
                0, 255)));

        surface()->DrawFilledRect(
            x,
            y,
            x + boxW,
            y + rowH);

        // Slight top highlight.

        surface()->DrawSetColor(
            255, 255, 255,
            (int)(opacity * 25));

        surface()->DrawFilledRect(
            x,
            y,
            x + boxW,
            y + 1);

        // Notification icon.

        const wchar_t* icon = L"?";

        Color iconColor(
            100, 255, 25, alpha);

        switch (n.type)
        {
        case GABE_NOTIFY_GENERIC:
        {
            icon = L"i";

            iconColor =
                Color(80, 190, 255, alpha);

            break;
        }

        case GABE_NOTIFY_ERROR:
        {
            icon = L"!";

            iconColor =
                Color(255, 70, 55, alpha);

            break;
        }

        case GABE_NOTIFY_UNDO:
        {
            iconColor =
                Color(255, 215, 75, alpha);

            break;
        }

        default:
            break;
        }

        // Undo notifications have a custom arrow.

        if (n.type == GABE_NOTIFY_UNDO)
        {
            const int arrowSize =
                MIN(rowH - 6, iconW - 6);

            DrawUndoIcon(
                x + (iconW - arrowSize) / 2,
                y + (rowH - arrowSize) / 2,
                arrowSize,
                alpha);
        }
        else if (m_hIconFont != INVALID_FONT)
        {
            int iconTextW, iconTextH;

            surface()->GetTextSize(
                m_hIconFont,
                icon,
                iconTextW,
                iconTextH);

            DrawShadowedText(
                m_hIconFont,
                icon,
                x + (iconW - iconTextW) / 2,
                y + (rowH - iconTall) / 2,
                iconColor);
        }

        // Notification message.

        DrawShadowedText(
            m_hTextFont,
            n.text,
            x + iconW,
            y + (rowH - textTall) / 2,
            Color(255, 255, 255, alpha));

        targetY += rowH + spacing;
    }
}

//=========================================================
// Public helper functions
//=========================================================

void GabeNotification(
    const char* text,
    int type,
    float duration)
{
    CHudGabeNotifications* pHUD =
        GET_HUDELEMENT(CHudGabeNotifications);

    if (pHUD)
    {
        pHUD->AddNotification(
            text,
            type,
            duration);
    }
}

void GabeUndoNotification(const char* name)
{
    CHudGabeNotifications* pHUD =
        GET_HUDELEMENT(CHudGabeNotifications);

    if (pHUD)
    {
        pHUD->AddUndoNotification(name);
    }
}

//=========================================================
// General notification command
//=========================================================

CON_COMMAND_F(
    gabe_notify,
    "gabe_notify <message> [type] [duration]",
    FCVAR_CLIENTDLL)
{
    if (args.ArgC() < 2)
    {
        Msg(
            "Usage: gabe_notify "
            "<message> [type] [duration]\n");

        return;
    }

    const int type =
        args.ArgC() >= 3
        ? atoi(args[2])
        : GABE_NOTIFY_GENERIC;

    const float duration =
        args.ArgC() >= 4
        ? atof(args[3])
        : 5.0f;

    GabeNotification(
        args[1],
        type,
        duration);
}

//=========================================================
// Undo notification command
//=========================================================

CON_COMMAND_F(
    gabe_notify_undo,
    "Display an undo notification.",
    FCVAR_CLIENTDLL)
{
    if (args.ArgC() < 2)
    {
        GabeUndoNotification("Entity");
        return;
    }

    GabeUndoNotification(args[1]);
}

//=========================================================
// Clear all notifications
//=========================================================

CON_COMMAND_F(
    gabe_notify_clear,
    "Clear all notifications.",
    FCVAR_CLIENTDLL)
{
    CHudGabeNotifications* pHUD =
        GET_HUDELEMENT(CHudGabeNotifications);

    if (pHUD)
    {
        pHUD->ClearNotifications();
    }
}