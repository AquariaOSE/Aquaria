-- 1.1.3+ compatibility wrapper
-- Intended for development while keeping support for old versions.
-- Intentionally removes any new functions from the global namespace.
-- Scripts need to follow the rules in ScriptInterface.cpp regarding the 'v' table.

dofile("scripts/compat/internal/oldfunctions.lua")
dofile("scripts/compat/internal/removenewfunctions.lua")

