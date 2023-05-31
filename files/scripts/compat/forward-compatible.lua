-- Removes more functions than to no-deprecated
-- Some original game scripts will not work with this!
-- This is fine to use when developing a mod, but do NOT distribute a mod with this set!
-- (Consider using "no-deprecated", "default", "none" or simply leave the compat tag away)

dofile("scripts/compat/no-deprecated.lua")

incrFlag = nil
decrFlag = nil
entity_sound = nil
quit = nil -- mods should not do this ever
doModSelect = nil
doLoadMenu = nil
entity_incrTargetLeaches = nil -- use avatar_incrLeaches() instead
entity_decrTargetLeaches = nil -- use avatar_decrLeaches() instead
entity_soundFreq = nil

isPlat = nil -- should not matter on the Lua side what the platform is
PLAT_WIN = nil
PLAT_MAC = nil
PLAT_LNX = nil
