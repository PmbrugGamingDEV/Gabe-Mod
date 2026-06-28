::settingsFrame <- VGUI_CreateFrame("Gabe Mod Settings", 220, 120, 520, 420);

settingsFrame.SetSizeable(false);
settingsFrame.MoveToFront();
settingsFrame.RequestFocus();

settingsFrame.CreateLabel("Player Name:", 25, 45, 120, 22);
::nameEntry <- settingsFrame.CreateTextEntry(GetConVarString("name"), 150, 42, 220, 26);

settingsFrame.CreateLabel("FOV:", 25, 90, 120, 22);
::fovSlider <- settingsFrame.CreateSlider(150, 92, 220, 20);
fovSlider.SetSliderRange(75, 120);
fovSlider.SetSliderValue(GetConVarInt("fov_desired"));

::crosshairBox <- settingsFrame.CreateCheckButton("Enable Crosshair", 25, 130, 220, 24);
crosshairBox.SetChecked(GetConVarBool("crosshair"));

::devBox <- settingsFrame.CreateCheckButton("Developer Mode", 25, 165, 220, 24);
devBox.SetChecked(GetConVarBool("developer"));

::output <- settingsFrame.CreateRichText(25, 210, 465, 110);
output.RichTextInsert("Settings panel loaded.\n");
output.RichTextInsert("Change values, then press Apply.\n");

::applyBtn <- settingsFrame.CreateButton("Apply Settings", 25, 340, 150, 35);
applyBtn.SetCommand(@"
    SetConVarString(""name"", ::nameEntry.GetText());
    SetConVarInt(""fov_desired"", ::fovSlider.GetSliderValue());

    SetConVarInt(""crosshair"", ::crosshairBox.IsChecked() ? 1 : 0);
    SetConVarInt(""developer"", ::devBox.IsChecked() ? 1 : 0);

    ::output.RichTextClear();
    ::output.RichTextInsert(""Applied settings:\n"");
    ::output.RichTextInsert(""Name: "" + ::nameEntry.GetText() + ""\n"");
    ::output.RichTextInsert(""FOV: "" + ::fovSlider.GetSliderValue() + ""\n"");
    ::output.RichTextInsert(""Crosshair: "" + ::crosshairBox.IsChecked() + ""\n"");
    ::output.RichTextInsert(""Developer: "" + ::devBox.IsChecked() + ""\n"");
");

::closeBtn <- settingsFrame.CreateButton("Close", 340, 340, 150, 35);
closeBtn.SetCommand(@"
    ::settingsFrame.Delete();
");