-- Loaded when mod compat setting is empty or does not exist.
-- Does a few basic adjustments but does not offer compatbility with legacy mods.
-- (In this case and for 1.1.x/Steam compatibility, use "legacy" or "legacy-strict")

dofile("scripts/compat/internal/osestubs.lua")
dofile("scripts/compat/internal/oldfunctions.lua")
