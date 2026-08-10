print("OpenMod Lua autorun loaded!")

local ent = CreateEntityByName("prop_physics")

if ent then
    ent:SetModel("models/props_c17/oildrum001.mdl")

    ent:SetAbsOrigin(Vector.new(0, 0, 128))

    DispatchSpawn(ent)

    print("Created prop:", ent:entindex())
else
    print("Failed to create prop_physics")
end