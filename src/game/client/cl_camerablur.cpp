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
#include "input.h"
#include "c_baseplayer.h"
#include "in_buttons.h"
#include "usercmd.h"
#include "viewpostprocess.h"
#include "con_nprint.h"
#include "view_scene.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterialsystem.h"

extern IViewRender* view;

//========= Client Stub For weapon_camera (HL2MP 2007 Safe) ============//

#include "basehlcombatweapon_shared.h"

STUB_WEAPON_CLASS_IMPLEMENT(weapon_camera, C_WeaponCamera);

IMPLEMENT_CLIENTCLASS_DT(C_WeaponCamera, DT_WeaponCamera, CWeaponCamera)
RecvPropBool(RECVINFO(m_bViewLocked)),
RecvPropBool(RECVINFO(m_bAttack2Held)),
RecvPropFloat(RECVINFO(m_flRollAngle)),
END_RECV_TABLE()

bool C_WeaponCamera::OverrideViewAngles(void)
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

    if (!pPlayer || pPlayer->GetActiveWeapon() != this)
    {
        m_bLocalViewLocked = false;
        return false;
    }

    if (!m_bAttack2Held)
    {
        m_bLocalViewLocked = false;
        return false;
    }

    if (!m_bLocalViewLocked)
    {
        engine->GetViewAngles(m_angLockedView);

        pPlayer->m_vecUseAngles = m_angLockedView;
        m_bLocalViewLocked = true;
    }

    return true;
}

float C_WeaponCamera::GetCameraRoll() const
{
    return m_flRollAngle;
}

// --------------------------------------------------
// State
// --------------------------------------------------

static int   g_iMode = 0;
static float g_flTargetStrength = 0.0f;
static float g_flCurrentStrength = 0.0f;
static float g_flFadeSpeed = 3.0f;

static IMaterial* g_pScreenMat = NULL;

static void InitCamBlurMaterial()
{
    if (g_pScreenMat)
        return;

    IMaterial* pMaterial = materials->FindMaterial(
        "effects/camerablur",
        TEXTURE_GROUP_CLIENT_EFFECTS
    );

    if (!pMaterial || pMaterial->IsErrorMaterial())
        return;

    pMaterial->IncrementReferenceCount();

    g_pScreenMat = pMaterial;
}

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
    InitCamBlurMaterial();

    if (!g_pScreenMat || g_pScreenMat->IsErrorMaterial())
        return;

    const int w = ScreenWidth();
    const int h = ScreenHeight();

    if (w <= 0 || h <= 0)
        return;

    // Capture the screen into the framebuffer texture.
    Rect_t actualRect;

    UpdateScreenEffectTexture(
        0, 0, 0, w, h, false, &actualRect
    );

    ITexture* pTexture = GetFullFrameFrameBufferTexture(0);

    if (!pTexture || pTexture->IsError())
        return;

    CMatRenderContextPtr pRenderContext(materials);

    const float t = gpGlobals->curtime;
    const int samples = 14;

    // Alpha parameter for progressive blending.
    bool bFoundAlpha = false;

    IMaterialVar* pAlpha = g_pScreenMat->FindVar(
        "$alpha", &bFoundAlpha, false
    );

    // Only perform screen effects when active.
    if (g_iMode != 0 && strength > 0.01f)
    {
        for (int i = 0; i < samples; ++i)
        {
            const float fraction =
                (float)i / (float)(samples - 1);

            const float offset = fraction * strength;

            float xOff = 0.0f;
            float yOff = 0.0f;

            switch (g_iMode)
            {
            case 1: // Soft Smear
                xOff = offset;
                break;

            case 2: // Double Vision
                xOff = (i % 2 == 0 ? -1.0f : 1.0f)
                    * strength * 2.0f;
                break;

            case 3: // Screen Tear
                xOff = sin(t * 8.0f + i * 2.0f)
                    * offset * 2.0f;
                break;

            case 4: // Pulse Warp
                xOff = sin(t * 5.0f) * offset;
                yOff = cos(t * 5.0f) * offset;
                break;

            case 5: // Heavy Blur
                xOff = offset;
                yOff = offset;
                break;

            case 6: // Spiral Drift
                xOff = sin(i + t) * offset;
                yOff = cos(i + t) * offset;
                break;

            case 7: // Horizontal Collapse
                xOff = offset * 4.0f;
                break;

            case 8: // Vertical Collapse
                yOff = offset * 4.0f;
                break;

            case 9: // Shake Distortion
                xOff = sin(t * 30.0f + i) * strength;
                yOff = cos(t * 25.0f + i) * strength;
                break;

            case 10: // TV Static
                xOff = sin(i * 12.9898f + t * 30.0f)
                    * offset;
                yOff = cos(i * 78.233f + t * 30.0f)
                    * offset;
                break;

            case 11: // Too Much Beer
                xOff = sin(t * 2.0f) * offset * 3.0f;
                yOff = cos(t * 1.5f) * offset * 3.0f;
                break;

            case 12: // Reality Melting
                xOff = sin(t * i) * offset * 5.0f;
                yOff = cos(t * i) * offset * 5.0f;
                break;
            }

            // Running average of framebuffer samples.
            // Requires an alpha-blended VMT.
            if (bFoundAlpha && pAlpha)
            {
                pAlpha->SetFloatValue(
                    1.0f / (float)(i + 1)
                );
            }

            pRenderContext->DrawScreenSpaceRectangle(
                g_pScreenMat,
                0, 0,
                w, h,
                actualRect.x + xOff,
                actualRect.y + yOff,
                actualRect.x + actualRect.width - 1 + xOff,
                actualRect.y + actualRect.height - 1 + yOff,
                pTexture->GetActualWidth(),
                pTexture->GetActualHeight()
            );
        }
    }

    // Restore the material alpha.
    if (bFoundAlpha && pAlpha)
    {
        pAlpha->SetFloatValue(1.0f);
    }

    // --------------------------------------
    // Draw mode name (colored)
    // --------------------------------------

    con_nprint_s info;
    info.index = 0;
    info.time_to_live = 0.1f;
    info.fixed_width_font = false;

    info.color[0] = 255;
    info.color[1] = 255;
    info.color[2] = 255;

    switch (g_iMode)
    {
    case 0:
        info.color[0] = 150;
        info.color[1] = 255;
        info.color[2] = 150;
        break;

    case 1:
        info.color[0] = 200;
        info.color[1] = 200;
        info.color[2] = 255;
        break;

    case 2:
        info.color[0] = 255;
        info.color[1] = 255;
        info.color[2] = 200;
        break;

    case 3:
        info.color[0] = 255;
        info.color[1] = 100;
        info.color[2] = 100;
        break;

    case 4:
        info.color[0] = 255;
        info.color[1] = 0;
        info.color[2] = 255;
        break;

    case 5:
        info.color[0] = 255;
        info.color[1] = 180;
        info.color[2] = 0;
        break;

    case 6:
        info.color[0] = 0;
        info.color[1] = 255;
        info.color[2] = 255;
        break;

    case 7:
        info.color[0] = 255;
        info.color[1] = 255;
        info.color[2] = 0;
        break;

    case 8:
        info.color[0] = 180;
        info.color[1] = 180;
        info.color[2] = 255;
        break;

    case 9:
        info.color[0] = 255;
        info.color[1] = 120;
        info.color[2] = 0;
        break;

    case 10:
        info.color[0] = 200;
        info.color[1] = 200;
        info.color[2] = 200;
        break;

    case 11:
        info.color[0] = 255;
        info.color[1] = 0;
        info.color[2] = 0;
        break;

    case 12:
        info.color[0] = 255;
        info.color[1] = 0;
        info.color[2] = 150;
        break;
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