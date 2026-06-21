local Author = "PmbrugGaming";
local Version = "1.0.0";
local DateMade = "2026-06-20";
local Description = "Don't Touch!!!!!!!!!";

local frame = VGUI_CreateFrame(
    "Welcome to Gabe Mod",
    100,
    50,
    900,
    700
);

frame.SetVisible(true);
frame.SetSizeable(false);
frame.MoveToFront();
frame.RequestFocus();

local html = frame.CreateHTML(
    5,
    30,
    890,
    665,
    true
);

html.HTMLSetContextMenuEnabled(false);
html.HTMLSetScrollbarsEnabled(true);
html.HTMLSetViewSourceEnabled(false);

// Load local MOTD page
html.HTMLOpenURL("file:///C:/GabeMod/game/mod_hl2mp/resource/motd.html", "", true);