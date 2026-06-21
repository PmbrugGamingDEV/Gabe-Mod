// scripts/vscripts/weapons/weapon_test.nut

function Spawn(wep)
{
	wep.SetPrimaryFunc("Fire")
	wep.SetSecondaryFunc("AltFire")
	wep.SetReloadFunc("DoReload")
	wep.SetThinkFunc("Think")
	wep.SetThinkDelayScript(0.05)

	wep.SetWorldModelScript("models/weapons/w_smg1.mdl")
	wep.SetViewModelScript("models/weapons/v_smg1.mdl")

	wep.PrecacheSoundScript("Weapon_SMG1.Single")
	wep.PrecacheSoundScript("Weapon_Pistol.Empty")

	wep.SetClip1Script(45)

	printl("[weapon_test] spawned")
}

function Deploy(wep)
{
	printl("[weapon_test] deploy")
}

function Holster(wep)
{
	printl("[weapon_test] holster")
}

function Fire(wep)
{
	local clip = wep.GetClip1Script()

	if (clip <= 0)
	{
		wep.EmitWeaponSoundScript("Weapon_Pistol.Empty")
		wep.SetNextPrimaryFireScript(0.25)
		return
	}

	wep.SetClip1Script(clip - 1)

	wep.EmitWeaponSoundScript("Weapon_SMG1.Single")
	wep.FireBulletScript(8, 1, 0.025, 1.0, wep.GetPrimaryAmmoTypeScript())
	wep.OwnerViewPunchScript(-1.5, RandomFloat(-0.5, 0.5), 0)

	wep.SetNextPrimaryFireScript(0.08)
}

function AltFire(wep)
{
	wep.EmitWeaponSoundScript("Weapon_AR2.Single")

	for (local i = 0; i < 5; i++)
	{
		wep.FireBulletScript(5, 1, 0.08, 1.0, wep.GetPrimaryAmmoTypeScript())
	}

	wep.OwnerViewPunchScript(-4, 0, 0)
	wep.SetNextSecondaryFireScript(0.7)
}

function DoReload(wep)
{
	wep.SetClip1Script(45)
	wep.EmitWeaponSoundScript("Weapon_SMG1.Reload")
	wep.SetNextPrimaryFireScript(1.2)
}

function Think(wep)
{
}