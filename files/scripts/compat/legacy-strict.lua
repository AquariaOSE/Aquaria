-- Same compatibility wrapper as "legacy",
-- but in addition removes all functions added after the last commercial release (1.1.1)
-- If your scripts run in this mode it's very likely that they will work
-- in the Steam/GOG/Humble/Ambrosia/... version.

dofile("scripts/compat/legacy.lua")
dofile("scripts/compat/internal/addquirks.lua")
dofile("scripts/compat/internal/removenewfunctions.lua")

-- v1.1 does not expose the debug library
debug = nil

-- This MUST NOT be removed. Core scripts depend on this, and mods are likely to use some of those.
--AQUARIA_VERSION = nil
