printl("=== VEHICLE TEST START ===");

local player = GetListenServerHost();

if (player == null)
{
	printl("ERROR: Could not get player.");
	return;
}

printl("Player found.");


// ------------------------------------------------------------
// Create a normal jeep
// ------------------------------------------------------------

local jeep = CreateJeep(
	player.GetOrigin() + Vector(200, 0, 64),
	QAngle(0, 0, 0)
);

if (jeep == null)
{
	printl("ERROR: Failed to create jeep.");
	return;
}

printl("Jeep created!");


// ------------------------------------------------------------
// Test basic vehicle information
// ------------------------------------------------------------

printl("Vehicle classname: " + jeep.GetVehicleClassname());
printl("Vehicle model: " + jeep.GetVehicleModel());

printl("Health: " + jeep.GetVehicleHealth());
printl("Speed: " + jeep.GetVehicleSpeed());


// ------------------------------------------------------------
// Health test
// ------------------------------------------------------------

jeep.SetVehicleHealth(500);

printl(
	"Health after SetVehicleHealth(500): "
	+ jeep.GetVehicleHealth()
);


// ------------------------------------------------------------
// Lock test
// ------------------------------------------------------------

jeep.Lock();

printl(
	"Locked: "
	+ jeep.IsLocked()
);

jeep.Unlock();

printl(
	"Locked after Unlock(): "
	+ jeep.IsLocked()
);


// ------------------------------------------------------------
// Engine test
// ------------------------------------------------------------

printl("Starting engine...");

jeep.StartEngine();


// ------------------------------------------------------------
// Physics test
// ------------------------------------------------------------

printl("Applying upward force...");

jeep.ApplyForce(
	Vector(0, 0, 50000)
);


// ------------------------------------------------------------
// Driver test
// ------------------------------------------------------------

printl(
	"Has driver before entering: "
	+ jeep.HasDriver()
);

printl("Putting player into vehicle...");

jeep.EnterVehicle(player);

printl(
	"Has driver after entering: "
	+ jeep.HasDriver()
);

printl("Driver:");

printl(
	jeep.GetDriver()
);


// ------------------------------------------------------------

printl("=== VEHICLE TEST COMPLETE ===");