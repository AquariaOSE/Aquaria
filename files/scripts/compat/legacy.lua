-- 1.1.3+ compatibility wrapper
-- Allows to run scripts written for the old script interface,
-- but also exposes new functions.

dofile("scripts/compat/internal/oldfunctions.lua")
dofile("scripts/compat/internal/loader.lua")
dofile("scripts/compat/internal/redirect_to_v.lua")
dofile("scripts/compat/internal/addfixes.lua")
