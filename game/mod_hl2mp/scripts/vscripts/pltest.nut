function TestPlayerFuncs()
{
    local p = null;

    while ((p = Entities.FindByClassname(p, "player")) != null)
    {
        printl("Testing player: " + p.GetPlayerName());

        printl("Name: " + p.GetPlayerName());
        printl("Alive: " + p.IsAlive());
        printl("Frags: " + p.GetFragCount());
        printl("Deaths: " + p.GetDeathCount());
        printl("Armor: " + p.GetArmorValue());
        printl("Death time: " + p.GetDeathTime());
        printl("Buttons: " + p.GetButtons());
        printl("Eye forward: " + p.GetEyeForward());

        p.EquipSuit();

        p.GiveItem("weapon_crowbar");
        p.GiveItem("weapon_physcannon");
        p.GiveItem("weapon_pistol");

        p.GiveAmmo(60, "Pistol");

        p.SwitchToWeapon("weapon_pistol");

        p.SetArmorValue(100);
        p.SetMaxHealth(150);
        p.SetMaxSpeed(400);

        p.AddFrags(1);
        p.AddDeaths(1);
        p.AddTeamScore(1);

        p.ShowViewModel(true);

        local cl_name = p.GetClientConVar("name");
        printl("Client name convar: " + cl_name);

        // Optional dangerous tests:
        // p.RemoveAllItems();
        // p.CommitSuicide();
        // p.ForceRespawn();
        // p.SetPlayerModel("models/player/kleiner.mdl");
        // p.ShowMOTD();
    }
}