//========= Custom Camera FX System (Source 2007 Safe) ============//

#include "cbase.h"
#include "view_shared.h"
#include "viewrender.h"
#include "usermessages.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "tier0/vprof.h"
#include "c_weapon__stubs.h"
#include "cl_camerablur.h"
#include "cdll_util.h"
#include "con_nprint.h"

extern IViewRender* view;

//========= Client Stub For weapon_camera (HL2MP 2007 Safe) ============//

#include "basehlcombatweapon_shared.h"

class C_WeaponCamera : public C_BaseHLCombatWeapon
{
    DECLARE_CLASS(C_WeaponCamera, C_BaseHLCombatWeapon);
public:
    DECLARE_PREDICTABLE();
    DECLARE_CLIENTCLASS();
    C_WeaponCamera() {};

    // Hide viewmodel completely
    virtual bool ShouldDrawViewModel() { return false; }

    // Do not draw world model either
    virtual bool ShouldDraw() { return false; }

    // Prevent animation spam
    virtual bool IsPredicted() const { return true; }

private:

    C_WeaponCamera(const C_WeaponCamera&);
};

STUB_WEAPON_CLASS_IMPLEMENT(weapon_camera, C_WeaponCamera);
IMPLEMENT_CLIENTCLASS_DT(C_WeaponCamera, DT_WeaponCamera, CWeaponCamera)
END_RECV_TABLE()

// --------------------------------------------------
// State
// --------------------------------------------------

static int   g_iMode = 0;
static float g_flTargetStrength = 0.0f;
static float g_flCurrentStrength = 0.0f;
static float g_flFadeSpeed = 3.0f;

static IMaterial* g_pScreenMat = NULL;

// --------------------------------------------------
// Mode Strength
// --------------------------------------------------

static float GetStrengthForMode(int mode)
{
    switch (mode)
    {
    case 0:  return 0.0f;
    case 1:  return 0.6f;
    case 2:  return 1.2f;
    case 3:  return 1.5f;
    case 4:  return 2.0f;
    case 5:  return 3.0f;
    case 6:  return 2.5f;
    case 7:  return 4.0f;
    case 8:  return 4.0f;
    case 9:  return 2.0f;
    case 10: return 3.5f;
    case 11: return 5.0f;
    case 12: return 6.0f;
    }
    return 0.0f;
}

// --------------------------------------------------
// UserMessage
// --------------------------------------------------

static void __MsgFunc_CamBlur(bf_read& msg)
{
    g_iMode = msg.ReadByte();

    if (g_iMode < 0 || g_iMode > 12)
        g_iMode = 0;

    g_flTargetStrength = GetStrengthForMode(g_iMode);
}

// --------------------------------------------------
// Drunk Wobble
// --------------------------------------------------

static void ApplyWobble(CViewSetup* pSetup)
{
    if (g_iMode != 11)
        return;

    float t = gpGlobals->curtime;

    pSetup->angles[ROLL] = sin(t * 2.0f) * 6.0f;
    pSetup->angles[YAW] += sin(t * 1.5f) * 0.5f;
}

// --------------------------------------------------
// Draw FX (Correct Framebuffer Copy)
// --------------------------------------------------

static const char* g_pszCamFXNames[] =
{
    "Calm",
    "Soft Smear",
    "Double Vision",
    "Screen Tear",
    "Pulse Warp",
    "Heavy Blur",
    "Spiral Drift",
    "Horizontal Collapse",
    "Vertical Collapse",
    "Shake Distortion",
    "TV Static",
    "Too Much Beer",
    "Reality Melting"
};

static void DrawFX(float strength)
{
    if (!g_pScreenMat)
    {
        g_pScreenMat = materials->FindMaterial(
            "effects/camerablur",
            TEXTURE_GROUP_CLIENT_EFFECTS
        );
    }

    if (!g_pScreenMat || g_pScreenMat->IsErrorMaterial())
        return;

    CMatRenderContextPtr pRenderContext(materials);

    int w = ScreenWidth();
    int h = ScreenHeight();

    pRenderContext->CopyRenderTargetToTexture(
        pRenderContext->GetRenderTarget()
    );

    float t = gpGlobals->curtime;
    const int samples = 14;

    for (int i = 0; i < samples; ++i)
    {
        float offset = ((float)i / samples) * strength;

        float xOff = 0;
        float yOff = 0;

        switch (g_iMode)
        {
        case 1: xOff = offset; break;
        case 2: xOff = sin(t * 3.0f) * strength; break;
        case 3: xOff = (rand() % 20 - 10) * 0.2f; break;
        case 4: xOff = sin(t * 5.0f) * offset;
            yOff = cos(t * 5.0f) * offset; break;
        case 5: xOff = offset; yOff = offset; break;
        case 6: xOff = sin(i + t) * offset;
            yOff = cos(i + t) * offset; break;
        case 7: xOff = offset * 4.0f; break;
        case 8: yOff = offset * 4.0f; break;
        case 9: xOff = sin(t * 30.0f) * strength;
            yOff = cos(t * 25.0f) * strength; break;
        case 10: xOff = (rand() % 50 - 25) * 0.1f;
            yOff = (rand() % 50 - 25) * 0.1f; break;
        case 11: xOff = sin(t * 2.0f) * offset * 3.0f;
            yOff = cos(t * 1.5f) * offset * 3.0f; break;
        case 12: xOff = sin(t * i) * offset * 5.0f;
            yOff = cos(t * i) * offset * 5.0f; break;
        }

        pRenderContext->DrawScreenSpaceRectangle(
            g_pScreenMat,
            0, 0,
            w, h,
            xOff, yOff,
            w - 1 + xOff, h - 1 + yOff,
            w,
            h
        );
    }

    // -------------------------
    // Draw Mode Name (Colored)
    // -------------------------

    con_nprint_s info;
    info.index = 0;
    info.time_to_live = 0.1f;
    info.fixed_width_font = false;

    // Default white
    info.color[0] = 255;
    info.color[1] = 255;
    info.color[2] = 255;

    switch (g_iMode)
    {
    case 0:  info.color[0] = 150; info.color[1] = 255; info.color[2] = 150; break;
    case 1:  info.color[0] = 200; info.color[1] = 200; info.color[2] = 255; break;
    case 2:  info.color[0] = 255; info.color[1] = 255; info.color[2] = 200; break;
    case 3:  info.color[0] = 255; info.color[1] = 100; info.color[2] = 100; break;
    case 4:  info.color[0] = 255; info.color[1] = 0;   info.color[2] = 255; break;
    case 5:  info.color[0] = 255; info.color[1] = 180; info.color[2] = 0;   break;
    case 6:  info.color[0] = 0;   info.color[1] = 255; info.color[2] = 255; break;
    case 7:  info.color[0] = 255; info.color[1] = 255; info.color[2] = 0;   break;
    case 8:  info.color[0] = 180; info.color[1] = 180; info.color[2] = 255; break;
    case 9:  info.color[0] = 255; info.color[1] = 120; info.color[2] = 0;   break;
    case 10: info.color[0] = 200; info.color[1] = 200; info.color[2] = 200; break;
    case 11: info.color[0] = 255; info.color[1] = 0;   info.color[2] = 0;   break;
    case 12: info.color[0] = 255; info.color[1] = 0;   info.color[2] = 150; break;
    }

    engine->Con_NXPrintf(
        &info,
        "View: %s",
        g_pszCamFXNames[g_iMode]
    );
}

// --------------------------------------------------
// Public Hooks
// --------------------------------------------------

void CamBlur_Init()
{
    usermessages->HookMessage("CamBlur", __MsgFunc_CamBlur);
}

void CamBlur_OnOverrideView(CViewSetup* pSetup)
{
    ApplyWobble(pSetup);
}

void CamBlur_PostRender()
{
    VPROF("CamBlur_PostRender");

    g_flCurrentStrength = Approach(
        g_flTargetStrength,
        g_flCurrentStrength,
        gpGlobals->frametime * g_flFadeSpeed
    );

    if (g_flCurrentStrength <= 0.01f)
        return;

    DrawFX(g_flCurrentStrength);
}