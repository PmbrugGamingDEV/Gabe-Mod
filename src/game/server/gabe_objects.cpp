#include "cbase.h"
#include "props.h"
#include "ndebugoverlay.h"
#include "vphysics_interface.h"
#include "player.h"
#include "ai_basenpc.h"
#include "ai_baseactor.h"
#include "soundent.h"
#include "util.h"

// memdbgon last
#include "tier0/memdbgon.h"

class CDevWatermelon : public CBaseAnimating
{
public:
    DECLARE_CLASS(CDevWatermelon, CBaseAnimating);
    DECLARE_DATADESC();

    void Spawn();
    void Think();
};

LINK_ENTITY_TO_CLASS(env_devwatermelon, CDevWatermelon);

BEGIN_DATADESC(CDevWatermelon)
DEFINE_THINKFUNC(Think),
END_DATADESC()

//---------------------------------------------------------
// Spawn
//---------------------------------------------------------
void CDevWatermelon::Spawn()
{
    SetModel("models/props_junk/watermelon01.mdl");
    BaseClass::Spawn();

    SetMoveType(MOVETYPE_VPHYSICS);
    SetSolid(SOLID_VPHYSICS);
    VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);

    SetNextThink(gpGlobals->curtime + 0.1f);
}

//---------------------------------------------------------
// Think � THE DEV TOOL
//---------------------------------------------------------
void CDevWatermelon::Think()
{
    // Debug overlays are hard-gated
    if (developer.GetInt() == 0)
    {
        Msg(
            "env_devwatermelon WARNING: developer 0 � overlays disabled (use developer 1)\n"
        );
        SetNextThink(gpGlobals->curtime + 1.0f);
        return;
    }

    const float dur = 0.25f;

    Vector pos = WorldSpaceCenter();
    QAngle ang = GetAbsAngles();
    Vector vel = GetAbsVelocity();

    IPhysicsObject* pPhys = VPhysicsGetObject();
    bool bPhys = (pPhys != NULL);
    bool bAwake = (bPhys && pPhys->IsMotionEnabled() && !pPhys->IsAsleep());

    // ----------------------------------------------------
    // COLLISION & BOUNDS
    // ----------------------------------------------------

    // Engine collision bounds (truth)
    NDebugOverlay::EntityBounds(this, 255, 0, 0, 64, dur);

    // Oriented bounding box
    NDebugOverlay::BoxAngles(
        pos,
        CollisionProp()->OBBMins(),
        CollisionProp()->OBBMaxs(),
        ang,
        0, 255, 0, 64,
        dur
    );

    // ----------------------------------------------------
    // ORIENTATION
    // ----------------------------------------------------

    // Orientation axis
    NDebugOverlay::Axis(pos, ang, 48, false, dur);

    // Center cross
    NDebugOverlay::Cross3D(pos, 12, 255, 255, 255, false, dur);

    // Forward / Right / Up vectors
    Vector fwd, right, up;
    AngleVectors(ang, &fwd, &right, &up);

    NDebugOverlay::Line(pos, pos + fwd * 32, 255, 255, 0, false, dur); // forward
    NDebugOverlay::Line(pos, pos + right * 32, 0, 255, 255, false, dur); // right
    NDebugOverlay::Line(pos, pos + up * 32, 255, 0, 255, false, dur); // up

    // ----------------------------------------------------
    // VELOCITY & MOTION
    // ----------------------------------------------------

    if (vel.LengthSqr() > 1.0f)
    {
        // Velocity vector
        NDebugOverlay::Line(
            pos,
            pos + vel * 0.1f,
            255, 0, 0,
            false,
            dur
        );

        // Swept movement box
        NDebugOverlay::SweptBox(
            pos,
            pos + vel * 0.1f,
            CollisionProp()->OBBMins(),
            CollisionProp()->OBBMaxs(),
            ang,
            255, 150, 0, 64,
            dur
        );
    }

    // ----------------------------------------------------
    // PHYSICS STATE
    // ----------------------------------------------------

    if (bPhys)
    {
        // Sleep state indicator
        NDebugOverlay::Cross3D(
            pos,
            8,
            bAwake ? 0 : 255,
            bAwake ? 255 : 0,
            0,
            false,
            dur
        );

        // Angular velocity ring
        Vector angVel;
        pPhys->GetVelocity(NULL, &angVel);

        if (angVel.LengthSqr() > 1.0f)
        {
            NDebugOverlay::Circle(
                pos,
                ang,
                clamp(angVel.Length(), 8.0f, 32.0f),
                255, 100, 255, 64,
                false,
                dur
            );
        }
    }

    // ----------------------------------------------------
    // GROUND & ENVIRONMENT
    // ----------------------------------------------------

    trace_t tr;
    UTIL_TraceLine(
        pos,
        pos - Vector(0, 0, 128),
        MASK_SOLID,
        this,
        COLLISION_GROUP_NONE,
        &tr
    );

    if (tr.fraction < 1.0f)
    {
        // Ground normal arrow
        NDebugOverlay::VertArrow(
            tr.endpos,
            tr.endpos + tr.plane.normal * 32,
            6,
            0, 200, 255, 64,
            false,
            dur
        );

        // Distance to ground
        NDebugOverlay::Line(
            pos,
            tr.endpos,
            200, 200, 200,
            false,
            dur
        );
    }

    // Grid reference
    NDebugOverlay::Grid(pos);

    // ----------------------------------------------------
    // INFORMATION BLOCK
    // ----------------------------------------------------

    char buf[512];

    Q_snprintf(
        buf, sizeof(buf),
        "DEV WATERMELON\n"
        "Class: %s\n"
        "Name: %s\n"
        "Pos: %.1f %.1f %.1f\n"
        "Speed: %.1f\n"
        "Phys: %s\n"
        "Gravity: %s",
        GetClassname(),
        STRING(GetEntityName()),
        pos.x, pos.y, pos.z,
        vel.Length(),
        bPhys ? (bAwake ? "Awake" : "Sleeping") : "None",
        (bPhys && pPhys->IsGravityEnabled()) ? "On" : "Off"
    );

    NDebugOverlay::Text(
        pos + Vector(0, 0, 64),
        buf,
        false,
        dur
    );

    SetNextThink(gpGlobals->curtime + 0.1f);
}

class CDebugPartyBall : public CBaseAnimating
{
public:
    DECLARE_CLASS(CDebugPartyBall, CBaseAnimating);
    DECLARE_DATADESC();

    void Precache();
    void Spawn();
    void Think();
};

LINK_ENTITY_TO_CLASS(env_debugpartyball, CDebugPartyBall);

BEGIN_DATADESC(CDebugPartyBall)
DEFINE_THINKFUNC(Think),
END_DATADESC()

//---------------------------------------------------------
void CDebugPartyBall::Precache()
{
    PrecacheModel("models/props_junk/watermelon01.mdl");

    BaseClass::Precache();
}

void CDebugPartyBall::Spawn()
{
    Precache();

    SetModel("models/props_junk/watermelon01.mdl");
    BaseClass::Spawn();

    SetMoveType(MOVETYPE_VPHYSICS);
    SetSolid(SOLID_VPHYSICS);
    VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);

    SetNextThink(gpGlobals->curtime + 0.05f);
}

//---------------------------------------------------------
void CDebugPartyBall::Think()
{
    const float dur = 0.1f;

    Vector pos = WorldSpaceCenter();
    QAngle ang = GetAbsAngles();
    Vector vel = GetAbsVelocity();

    float speed = vel.Length();
    float t = gpGlobals->curtime;

    // Small temporal jitter (engine panic)
    Vector jitter(
        sin(t * 13.1f) * 1.5f,
        cos(t * 17.3f) * 1.5f,
        sin(t * 11.7f) * 1.0f
    );

    Vector p = pos + jitter;

    // --------------------------------------------------
    // PSYCHEDELIC COLOR FIELD
    // --------------------------------------------------
    int r = int(sin(t * 4.0f + speed * 0.02f) * 127 + 128);
    int g = int(sin(t * 4.0f + 2.0f) * 127 + 128);
    int b = int(sin(t * 4.0f + 4.0f) * 127 + 128);

    // --------------------------------------------------
    // MULTI-PHASE HEARTBEAT BOXES
    // --------------------------------------------------
    for (int i = 0; i < 3; i++)
    {
        float phase = t * (2.0f + i) + i * 2.0f;
        float size = 18 + sin(phase) * (6 + speed * 0.05f);

        NDebugOverlay::Box(
            p,
            Vector(-size, -size, -size),
            Vector(size, size, size),
            r, g, b,
            32,
            dur
        );
    }

    // --------------------------------------------------
    // DESYNCED AXES (ENGINE HAVING A STROKE)
    // --------------------------------------------------
    QAngle axisA(t * 120, 0, 0);
    QAngle axisB(0, t * -180, 0);
    QAngle axisC(0, 0, t * 240);

    NDebugOverlay::Axis(p, axisA, 48, false, dur);
    NDebugOverlay::Axis(p, axisB, 36, false, dur);
    NDebugOverlay::Axis(p, axisC, 24, false, dur);

    // --------------------------------------------------
    // REALITY TEAR RINGS
    // --------------------------------------------------
    for (int i = 0; i < 4; i++)
    {
        float ring = fmod(t * 40 + i * 20, 120);
        QAngle ringAng(i * 45, t * 90, 0);

        NDebugOverlay::Circle(
            p,
            ringAng,
            ring,
            255 - i * 40,
            50 + i * 40,
            255,
            24,
            false,
            dur
        );
    }

    // --------------------------------------------------
    // VELOCITY SPIRAL CORKSCREW
    // --------------------------------------------------
    if (speed > 5.0f)
    {
        Vector dir = vel;
        VectorNormalize(dir);

        for (int i = 0; i < 6; i++)
        {
            float a = t * 6 + i;
            Vector swirl(
                cos(a) * 12,
                sin(a) * 12,
                i * 4
            );

            NDebugOverlay::Line(
                p + swirl,
                p + swirl - dir * 24,
                255, 200, 0,
                false,
                dur
            );
        }
    }

    // --------------------------------------------------
    // DEBUG LIGHTNING WEB
    // --------------------------------------------------
    for (int i = 0; i < 5; i++)
    {
        Vector randDir(
            RandomFloat(-1, 1),
            RandomFloat(-1, 1),
            RandomFloat(-1, 1)
        );
        VectorNormalize(randDir);

        NDebugOverlay::Line(
            p,
            p + randDir * RandomFloat(24, 64),
            150, 200, 255,
            false,
            dur
        );
    }

    // --------------------------------------------------
    // PANIC MODE TEXT EXPLOSION
    // --------------------------------------------------
    static const char* panicText[] =
    {
        "!!!",
        "ENGINE PANIC",
        "???",
        "HELP",
        "SOURCE.EXE"
    };

    int txt = int(t * 8) % ARRAYSIZE(panicText);

    for (int i = 0; i < 3; i++)
    {
        Vector textOffset(
            cos(t * 3 + i) * 16,
            sin(t * 3 + i) * 16,
            32 + i * 8
        );

        NDebugOverlay::Text(
            p + textOffset,
            panicText[txt],
            false,
            dur
        );
    }

    // --------------------------------------------------
// SCREEN-SPACE PARTY TEXT (HUD CHAOS)
// --------------------------------------------------
    static const char* screenTexts[] =
    {
        "DEBUG MODE",
        "SOURCE PARTY",
        "ENGINE VIBING",
        "???",
        "PHYSICS JAM",
        "water pee pa",
        "!!!",
        "dev=1"
    };

    int screenCount = clamp(int(speed * 0.05f) + 3, 3, 12);

    for (int i = 0; i < screenCount; i++)
    {
        float x = RandomFloat(0.05f, 0.95f);
        float y = RandomFloat(0.05f, 0.95f);

        int tr = RandomInt(100, 255);
        int tg = RandomInt(100, 255);
        int tb = RandomInt(100, 255);

        const char* msg = screenTexts[
            RandomInt(0, ARRAYSIZE(screenTexts) - 1)
        ];

        NDebugOverlay::ScreenText(
            x,
            y,
            msg,
            tr, tg, tb,
            200,
            dur
        );
    }

    // --------------------------------------------------
// SCREEN FLASH (DEBUG OVERLAY HACK)
// --------------------------------------------------
    float flash = fabs(sin(t * (4.0f + speed * 0.02f)));

    // Only flash when moving / chaotic
    if (flash > 0.7f)
    {
        int fr = int(sin(t * 6.0f) * 127 + 128);
        int fg = int(sin(t * 6.0f + 2.0f) * 127 + 128);
        int fb = int(sin(t * 6.0f + 4.0f) * 127 + 128);

        // Tile the screen with colored blocks
        for (float y = 0.0f; y <= 1.0f; y += 0.1f)
        {
            for (float x = 0.0f; x <= 1.0f; x += 0.1f)
            {
                NDebugOverlay::ScreenText(
                    x,
                    y,
                    "III",
                    fr, fg, fb,
                    180,
                    dur
                );
            }
        }
    }



    SetNextThink(gpGlobals->curtime + 0.05f);
}

class CDebugCrystal : public CBaseAnimating
{
public:
    DECLARE_CLASS(CDebugCrystal, CBaseAnimating);
    DECLARE_DATADESC();

    void Spawn();
    void Think();
};

LINK_ENTITY_TO_CLASS(env_debugcrystal, CDebugCrystal);

BEGIN_DATADESC(CDebugCrystal)
DEFINE_THINKFUNC(Think),
END_DATADESC()

//---------------------------------------------------------
void CDebugCrystal::Spawn()
{
    SetModel("models/props_junk/watermelon01.mdl"); // placeholder crystal
    BaseClass::Spawn();

    SetMoveType(MOVETYPE_VPHYSICS);
    SetSolid(SOLID_VPHYSICS);
    VPhysicsInitNormal(SOLID_VPHYSICS, 0, false);

    SetNextThink(gpGlobals->curtime + 0.05f);
}

//---------------------------------------------------------
void CDebugCrystal::Think()
{
    const float dur = 0.12f;

    Vector pos = WorldSpaceCenter();
    Vector vel = GetAbsVelocity();
    QAngle ang = GetAbsAngles();

    float speed = vel.Length();
    float t = gpGlobals->curtime;

    //--------------------------------------------------
    // COLOR ENERGY (HSV-style cycling)
    //--------------------------------------------------
    int r = int(sin(t * 2.0f) * 127 + 128);
    int g = int(sin(t * 2.0f + 2.1f) * 127 + 128);
    int b = int(sin(t * 2.0f + 4.2f) * 127 + 128);

    //--------------------------------------------------
    // FLOATING CORE BOX
    //--------------------------------------------------
    float pulse = 20.0f + sin(t * 3.0f) * 6.0f;

    NDebugOverlay::Box(
        pos,
        Vector(-pulse, -pulse, -pulse),
        Vector(pulse, pulse, pulse),
        r, g, b,
        64,
        dur
    );

    //--------------------------------------------------
    // ROTATING ENERGY RINGS
    //--------------------------------------------------
    QAngle ringA(0, t * 90.0f, 0);
    QAngle ringB(90, t * -120.0f, 0);

    NDebugOverlay::Circle(pos, ringA, 32, r, g, b, 32, false, dur);
    NDebugOverlay::Circle(pos, ringB, 24, b, r, g, 32, false, dur);

    //--------------------------------------------------
    // AXIS OF POWER
    //--------------------------------------------------
    NDebugOverlay::Axis(pos, QAngle(t * 60, t * 120, 0), 40, false, dur);

    //--------------------------------------------------
    // VELOCITY STREAMS
    //--------------------------------------------------
    if (speed > 5.0f)
    {
        Vector dir = vel;
        VectorNormalize(dir);

        for (int i = 0; i < 4; i++)
        {
            float phase = t * 5.0f + i;
            Vector offset(
                cos(phase) * 12,
                sin(phase) * 12,
                i * 6
            );

            NDebugOverlay::Line(
                pos + offset,
                pos + offset - dir * 24,
                255, 200, 100,
                false,
                dur
            );
        }
    }

    //--------------------------------------------------
    // ENERGY SHOCKWAVE (when moving fast)
    //--------------------------------------------------
    if (speed > 50.0f)
    {
        NDebugOverlay::Circle(
            pos,
            ang,
            clamp(speed * 0.4f, 32.0f, 96.0f),
            255, 100, 0,
            24,
            false,
            dur
        );
    }

    //--------------------------------------------------
    // SCREEN-SPACE ENERGY TEXT
    //--------------------------------------------------
    if (speed > 10.0f)
    {
        NDebugOverlay::ScreenText(
            RandomFloat(0.3f, 0.7f),
            RandomFloat(0.3f, 0.7f),
            "ENERGY",
            r, g, b,
            200,
            dur
        );
    }

    SetNextThink(gpGlobals->curtime + 0.05f);
}

class CGabeDebugHoloKill : public CBaseEntity
{
public:
    DECLARE_CLASS(CGabeDebugHoloKill, CBaseEntity);
    DECLARE_DATADESC();

    void Spawn();
    void Think();

private:
    float m_flCurrentSize;   // half-size
    float m_flTargetSize;
    float m_flExpandSpeed;
    float m_flDamage;
    float m_flNextDamageTime;
};

LINK_ENTITY_TO_CLASS(gabedebug_holokill, CGabeDebugHoloKill);

BEGIN_DATADESC(CGabeDebugHoloKill)
DEFINE_FIELD(m_flCurrentSize, FIELD_FLOAT),
DEFINE_FIELD(m_flTargetSize, FIELD_FLOAT),
DEFINE_FIELD(m_flExpandSpeed, FIELD_FLOAT),
DEFINE_FIELD(m_flDamage, FIELD_FLOAT),
DEFINE_FIELD(m_flNextDamageTime, FIELD_FLOAT),
DEFINE_THINKFUNC(Think),
END_DATADESC()

//---------------------------------------------------------
void CGabeDebugHoloKill::Spawn()
{
    BaseClass::Spawn();

    SetSolid(SOLID_NONE);
    SetMoveType(MOVETYPE_NONE);

    m_flCurrentSize = 8.0f;
    m_flTargetSize = 96.0f;   // final cube half-size
    m_flExpandSpeed = 80.0f;   // units/sec
    m_flDamage = 15.0f;   // shock damage
    m_flNextDamageTime = 0.0f;

    SetNextThink(gpGlobals->curtime + 0.05f);
}

//---------------------------------------------------------
void CGabeDebugHoloKill::Think()
{
    const float dur = 0.1f;
    float dt = gpGlobals->frametime;
    Vector origin = GetAbsOrigin();

    //-----------------------------------------------------
    // EXPAND HOLOGRAM
    //-----------------------------------------------------
    if (m_flCurrentSize < m_flTargetSize)
    {
        m_flCurrentSize += m_flExpandSpeed * dt;
        if (m_flCurrentSize > m_flTargetSize)
            m_flCurrentSize = m_flTargetSize;

    }

    //-----------------------------------------------------
    // COLOR PULSE (HOLOGRAM FEEL)
    //-----------------------------------------------------
    float t = gpGlobals->curtime;
    int r = 100;
    int g = int(sin(t * 4.0f) * 80 + 175);
    int b = 255;
    int a = int(sin(t * 6.0f) * 40 + 120);

    Vector mins(-m_flCurrentSize);
    Vector maxs(m_flCurrentSize);

    //-----------------------------------------------------
    // DRAW HOLOGRAM CUBE
    //-----------------------------------------------------
    NDebugOverlay::Box(
        origin,
        mins,
        maxs,
        r, g, b, a,
        dur
    );

    // Edge emphasis
    NDebugOverlay::Box(
        origin,
        mins * 0.98f,
        maxs * 0.98f,
        0, 200, 255, 255,
        dur
    );

    //-----------------------------------------------------
    // ENTITY CHECK (PLAYERS + NPCS)
    //-----------------------------------------------------
    if (gpGlobals->curtime >= m_flNextDamageTime)
    {
        CBaseEntity* pEnt = NULL;

        CBaseEntity* pList[128];
        int count = UTIL_EntitiesInBox(
            pList,
            ARRAYSIZE(pList),
            origin + mins,
            origin + maxs,
            FL_CLIENT | FL_NPC
        );

        for (int i = 0; i < count; i++)
        {
            CBaseEntity* pEnt = pList[i];
            if (!pEnt || pEnt == this)
                continue;

            if (pEnt->IsPlayer() || pEnt->IsNPC())
            {
                CTakeDamageInfo info(
                    this,
                    this,
                    m_flDamage,
                    DMG_SHOCK
                );

                pEnt->TakeDamage(info);

                // Visual zap
                NDebugOverlay::Line(
                    origin,
                    pEnt->WorldSpaceCenter(),
                    255, 255, 100,
                    false,
                    dur
                );

                // Debug text
                NDebugOverlay::EntityText(
                    pEnt->entindex(),
                    0,
                    "HOLOKILL",
                    dur
                );
            }
        }

        m_flNextDamageTime = gpGlobals->curtime + 0.5f;
    }

    //-----------------------------------------------------
    // CENTER LABEL
    //-----------------------------------------------------
    NDebugOverlay::Text(
        origin + Vector(0, 0, m_flCurrentSize + 16),
        "GABEDEBUG_HOLOKILL",
        false,
        dur
    );

    SetNextThink(gpGlobals->curtime + 0.05f);
}

#ifdef _WIN32
#include <windows.h>
#endif

#include "tier0/memdbgon.h"

//=====================================================================
class CGabeplusServerChaos : public CBaseEntity
{
public:
    DECLARE_CLASS(CGabeplusServerChaos, CBaseEntity);
    DECLARE_DATADESC();

    void Precache();
    void Spawn();
    void Think();

    bool  m_bChaosEnabled;
    float m_flSpin;
    float m_flHue;
};

LINK_ENTITY_TO_CLASS(gabeplus_chaos, CGabeplusServerChaos);

BEGIN_DATADESC(CGabeplusServerChaos)
DEFINE_THINKFUNC(Think),
DEFINE_FIELD(m_bChaosEnabled, FIELD_BOOLEAN),
DEFINE_FIELD(m_flSpin, FIELD_FLOAT),
DEFINE_FIELD(m_flHue, FIELD_FLOAT),
END_DATADESC()

//=====================================================================
static const char* RandomBeep()
{
    static const char* beeps[] =
    {
        "buttons/blip1.wav",
        "buttons/button10.wav",
        "buttons/button14.wav",
        "buttons/combine_button7.wav",
        "common/warning.wav"
    };
    return beeps[random->RandomInt(0, ARRAYSIZE(beeps) - 1)];
}

static void HueToRGB(float h, int& r, int& g, int& b)
{
    h = fmod(h, 360.0f);
    float x = fabs(fmod(h / 60.0f, 2) - 1);
    float c = 1.0f;

    float rf = 0, gf = 0, bf = 0;

    if (h < 60) { rf = c; gf = x; bf = 0; }
    else if (h < 120) { rf = x; gf = c; bf = 0; }
    else if (h < 180) { rf = 0; gf = c; bf = x; }
    else if (h < 240) { rf = 0; gf = x; bf = c; }
    else if (h < 300) { rf = x; gf = 0; bf = c; }
    else { rf = c; gf = 0; bf = x; }

    r = (int)(rf * 255);
    g = (int)(gf * 255);
    b = (int)(bf * 255);
}

//=====================================================================
void CGabeplusServerChaos::Precache()
{
    PrecacheModel("models/error.mdl");

    PrecacheScriptSound("buttons/blip1.wav");
    PrecacheScriptSound("buttons/button10.wav");
    PrecacheScriptSound("buttons/button14.wav");
    PrecacheScriptSound("buttons/combine_button7.wav");
    PrecacheScriptSound("common/warning.wav");

    BaseClass::Precache();
}

//=====================================================================
void CGabeplusServerChaos::Spawn()
{
    Precache();
    BaseClass::Spawn();

    SetSolid(SOLID_NONE);
    SetMoveType(MOVETYPE_NONE);
    AddEffects(EF_NODRAW);

    m_bChaosEnabled = false;
    m_flSpin = 0.0f;
    m_flHue = 0.0f;

#ifdef _WIN32
    if (!engine->IsDedicatedServer())
    {
        int result = MessageBoxA(
            NULL,
            "Continue if you want to see chaos. Click close if you don't want to, and the game will stop. WARNING: This will inevitably crash the game.",
            "GABE MOD +",
            MB_YESNO | MB_ICONWARNING | MB_SYSTEMMODAL | MB_TOPMOST
        );

        if (result == IDYES)
            m_bChaosEnabled = true;
        else
        {
            engine->ServerCommand("quit\n");
            engine->ServerExecute();
            return;
        }
    }
#endif

    SetNextThink(gpGlobals->curtime + 0.02f);
}

//=====================================================================
void CGabeplusServerChaos::Think()
{
    if (!m_bChaosEnabled)
        return;

    const float dur = 0.08f;

    m_flSpin += gpGlobals->frametime * 1080.0f;
    m_flHue += gpGlobals->frametime * 240.0f;

    int cr, cg, cb;
    HueToRGB(m_flHue, cr, cg, cb);

    // --------------------------------------------------
    // GLOBAL OVERLAY STORM
    // --------------------------------------------------
    for (int i = 0; i < 10; i++)
    {
        Vector pos(
            random->RandomFloat(-4096, 4096),
            random->RandomFloat(-4096, 4096),
            random->RandomFloat(0, 2048)
        );

        NDebugOverlay::Axis(
            pos,
            QAngle(m_flSpin, m_flSpin * 2, m_flSpin * 0.25f),
            random->RandomFloat(64, 256),
            true,
            dur
        );

        NDebugOverlay::Circle(
            pos,
            random->RandomFloat(32, 256),
            cr, cg, cb, 200,
            true,
            dur
        );
    }

    // --------------------------------------------------
    // ENTITY CORRUPTION + ORBIT
    // --------------------------------------------------
    for (int i = 1; i < gpGlobals->maxEntities; i++)
    {
        CBaseEntity* ent = UTIL_EntityByIndex(i);
        if (!ent)
            continue;

        Vector o = ent->GetAbsOrigin();
        o.x += cos(m_flSpin * 0.01f) * 2.0f;
        o.y += sin(m_flSpin * 0.01f) * 2.0f;
        ent->SetAbsOrigin(o);

        ent->SetRenderColor(cr, cg, cb);

        NDebugOverlay::EntityBounds(
            ent,
            cr, cg, cb,
            255,
            dur
        );

        // Sky laser
        NDebugOverlay::Line(
            Vector(o.x, o.y, o.z + 4096),
            o,
            cr, cg, cb,
            true,
            dur
        );

        // NPC corruption
        CAI_BaseNPC* npc = dynamic_cast<CAI_BaseNPC*>(ent);
        if (npc && npc->IsAlive())
        {
            if (Q_stricmp(STRING(npc->GetModelName()), "models/error.mdl") != 0)
                npc->SetModel("models/error.mdl");

            NDebugOverlay::EntityText(
                npc->entindex(),
                0,
                "!!! AI PANIC !!!",
                dur,
                255, 0, 0, 255
            );
        }

        // Props -> error.mdl
        const char* cls = ent->GetClassname();
        if (cls &&
            (!Q_stricmp(cls, "prop_physics") ||
                !Q_stricmp(cls, "prop_physics_multiplayer") ||
                !Q_stricmp(cls, "prop_dynamic") ||
                !Q_stricmp(cls, "prop_static")))
        {
            if (ent->GetModelName() == NULL_STRING ||
                Q_stricmp(STRING(ent->GetModelName()), "models/error.mdl") != 0)
            {
                ent->SetModel("models/error.mdl");
            }
        }

        // Random parenting chaos
        if (random->RandomInt(0, 120) == 0)
        {
            CBaseEntity* other = UTIL_EntityByIndex(
                random->RandomInt(1, gpGlobals->maxEntities - 1));
            if (other && other != ent)
                ent->SetParent(other);
        }
    }

    // --------------------------------------------------
    // PLAYER CAMERA TORTURE (2007-CORRECT)
    // --------------------------------------------------
    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        CBasePlayer* pl = UTIL_PlayerByIndex(i);
        if (!pl || !pl->IsAlive())
            continue;

        QAngle ang = pl->EyeAngles();

        ang[PITCH] += random->RandomFloat(-2.0f, 2.0f);
        ang[YAW] += random->RandomFloat(-4.0f, 4.0f);
        ang[ROLL] = sin(m_flSpin * 0.05f) * 10.0f;

        pl->SnapEyeAngles(ang);
        pl->ViewPunch(QAngle(
            random->RandomFloat(-1.5f, 1.5f),
            random->RandomFloat(-1.5f, 1.5f),
            0.0f
        ));
    }

    // --------------------------------------------------
    // AUDIO HELL
    // --------------------------------------------------
    for (int i = 0; i < 3; i++)
    {
        EmitSound_t s;
        s.m_pSoundName = RandomBeep();
        s.m_flVolume = 1.0f;
        s.m_SoundLevel = SNDLVL_130dB;
        s.m_nChannel = CHAN_STATIC;
        s.m_nPitch = 60 + (int)(sin(m_flSpin * 0.05f) * 80);

        CBroadcastRecipientFilter filter;
        EmitSound(filter, entindex(), s);
    }

    SetNextThink(gpGlobals->curtime + 0.02f);
}

void AdjustMaxAmmoForAllWeapons(int newMaxAmmo)
{
    // Get the local player
    CBasePlayer* pPlayer = UTIL_GetCommandClient();
    if (!pPlayer)
    {
        Msg("Player not found.\n");
        return;
    }

    // Loop through all weapons the player has
    for (int i = 0; i < pPlayer->WeaponCount(); ++i)
    {
        CBaseCombatWeapon* pWeapon = pPlayer->GetWeapon(i);
        if (!pWeapon)
            continue;

        // Adjust the maximum ammo for primary and secondary ammo types
        int primaryAmmoType = pWeapon->GetPrimaryAmmoType();
        int secondaryAmmoType = pWeapon->GetSecondaryAmmoType();

        if (primaryAmmoType != -1)
        {
            pPlayer->SetAmmoCount(newMaxAmmo, primaryAmmoType);
        }

        if (secondaryAmmoType != -1)
        {
            pPlayer->SetAmmoCount(newMaxAmmo, secondaryAmmoType);
        }

        Msg("Max ammo set for %s\n", pWeapon->GetClassname());
    }
}

// Define the console command using ConCommand
CON_COMMAND(gabe_maxammo, "Adjusts the max ammo.")
{
    if (args.ArgC() < 2)
    {
        Msg("Usage: gabe_maxammo <amount>\n");
        return;
    }

    int newMaxAmmo = atoi(args[1]);  // Convert the argument to an integer
    if (newMaxAmmo < 0)
    {
        Msg("Invalid max ammo amount. Please enter a positive number.\n");
        return;
    }

    AdjustMaxAmmoForAllWeapons(newMaxAmmo);
}
