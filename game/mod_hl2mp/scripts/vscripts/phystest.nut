local wep = null;

while ((wep = Entities.FindByClassname(wep, "weapon_physgun")) != null)
{
    printl("Found weapon_physgun");

    printl("IsHoldingObject: " + wep.IsHoldingObject());
    printl("GetHoldDistance: " + wep.GetHoldDistance());
    printl("IsRotatingObject: " + wep.IsRotatingObject());
    printl("GetFrozenObjectCount: " + wep.GetFrozenObjectCount());

    local held = wep.GetHeldEntity();

    if (held)
        printl("Held entity: " + held.GetClassname());
    else
        printl("Held entity: null");

    return;
}

printl("No weapon_physgun found");