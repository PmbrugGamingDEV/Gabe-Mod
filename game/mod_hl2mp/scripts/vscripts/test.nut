//
// test_hud.nut
//

g_MyHud <- null;

function MyHud_Paint()
{
    // Background
    CL_Surface.SetDrawColor(20, 20, 20, 210);
    CL_Surface.DrawFilledRect(20, 20, 300, 100);

    // Border
    CL_Surface.SetDrawColor(255, 150, 50, 255);
    CL_Surface.DrawOutlinedRect(20, 20, 300, 100);

    // Little decorative lines
    CL_Surface.SetDrawColor(255, 255, 255, 255);

    CL_Surface.DrawLine(35, 60, 80, 60);
    CL_Surface.DrawLine(57, 38, 57, 82);
}

function CreateMyHud()
{
    if (g_MyHud != null)
    {
        g_MyHud.HUD_Remove();
        g_MyHud = null;
    }

    g_MyHud = CL_HUD.CreateElement("MyScriptHUD");

    if (g_MyHud == null)
    {
        printl("Failed to create HUD");
        return;
    }

    g_MyHud.SetPaintFunction("MyHud_Paint");

    g_MyHud.HUD_SetBounds(
        0,
        0,
        CL_Surface.GetScreenWide(),
        CL_Surface.GetScreenTall()
    );

    g_MyHud.HUD_SetActive(true);
    g_MyHud.HUD_SetAlpha(255);

    printl("Created scripted HUD!");
}

function RemoveMyHud()
{
    if (g_MyHud != null)
    {
        g_MyHud.HUD_Remove();
        g_MyHud = null;

        printl("Removed scripted HUD");
    }
}

CreateMyHud();