-- Removes more functions than to no-deprecated
-- Some original game scripts will not work with this!

dofile("scripts/compat/no-deprecated.lua")

incrFlag = nil
decrFlag = nil
entity_sound = nil
entity_toggleBone = nil
isPlat = nil -- should not matter on the Lua side what the platform is
toggleVersionLabel = nil
setVersionLabelText = nil
quit = nil -- mods should not do this ever
doModSelect = nil
doLoadMenu = nil
appendUserDataPath = nil
entity_incrTargetLeaches = nil -- use avatar_incrLeaches() instead
entity_decrTargetLeaches = nil -- use avatar_decrLeaches() instead
entity_soundFreq = nil
