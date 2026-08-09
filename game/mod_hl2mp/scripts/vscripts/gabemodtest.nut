printl("========== GabeMod VScript Test ==========");

if (GabeMod == null)
{
    printl("[FAIL] GabeMod singleton does not exist!");
    return;
}

printl("[PASS] GabeMod singleton exists");

local player = GabeMod.GetPlayer();

if (player == null)
{
    printl("[FAIL] GabeMod.GetPlayer() returned null");
    return;
}

printl("[PASS] GetPlayer()");
printl("Player = " + player);

// Messaging
GabeMod.PrintToYou(
    player,
    3,
    "GabeMod VScript test!"
);

printl("[PASS] PrintToYou()");

GabeMod.PrintToAllPlayers(
    3,
    "GabeMod.PrintToAllPlayers works!"
);

printl("[PASS] PrintToAllPlayers()");

// Player vectors
local shootPos = GabeMod.ShootPos(player);
local forward = GabeMod.Forward(player);

printl("[PASS] ShootPos() = " + shootPos);
printl("[PASS] Forward() = " + forward);

// Debug overlays
GabeMod.DebugLine(
    shootPos,
    shootPos + forward * 512.0,
    10.0
);

printl("[PASS] DebugLine()");

GabeMod.DebugBox(
    shootPos + forward * 128.0,
    8.0,
    10.0
);

printl("[PASS] DebugBox()");

// Trace
local hit = GabeMod.TraceEntity(
    player,
    4096.0
);

if (hit != null)
{
    printl("[PASS] TraceEntity() hit: " + hit);
}
else
{
    printl("[PASS] TraceEntity() returned null (nothing hit)");
}

// Entity spawning
local spawnPos =
    shootPos +
    forward * 150.0 +
    Vector(0, 0, 32);

local prop = GabeMod.SpawnProp(
    "models/props_c17/oildrum001.mdl",
    spawnPos,
    QAngle(0, 0, 0)
);

if (prop != null)
{
    printl("[PASS] SpawnProp()");

    GabeMod.SetMass(
        prop,
        25.0
    );

    printl("[PASS] SetMass()");

    GabeMod.ApplyForce(
        prop,
        Vector(0, 0, 20000)
    );

    printl("[PASS] ApplyForce()");
}
else
{
    printl("[FAIL] SpawnProp()");
}

// Gameplay
GabeMod.Heal(
    player,
    1
);

printl("[PASS] Heal()");

// Searching
local found = GabeMod.Gabe_FindByClass(
    "prop_physics"
);

if (found != null)
{
    printl("[PASS] Gabe_FindByClass() = " + found);
}
else
{
    printl("[FAIL] Gabe_FindByClass()");
}

// Visibility
if (prop != null)
{
    local los = GabeMod.Gabe_HasLineOfSight(
        player,
        prop
    );

    printl(
        "[PASS] Gabe_HasLineOfSight() = " +
        los
    );

    local fov = GabeMod.Gabe_IsInFOV(
        player,
        prop,
        90.0
    );

    printl(
        "[PASS] Gabe_IsInFOV() = " +
        fov
    );
}

printl("========== GabeMod Test Complete ==========");