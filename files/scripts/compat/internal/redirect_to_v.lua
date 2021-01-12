-- Compatibility wrapper [interface detouring]
-- allows to run scripts written for the old script interface (1.1.0, 1.1.1, 1.1.2)
-- on 1.1.3+ / OSE and its new scripting interface.
-- Adds automatic detouring of what used to be per-script globals to the new instance-local "v" table.

-- Note that this is a gross hack and a lot of guesswork is involved,
-- but any sanely written mod should run without changes.

-- Notes:
-- * Any assignment to a global named "v" will break things. Like v = 0.
-- * Variables whose names only contain all-uppercase, underscores, and numbers are considered true globals.
--   Assignments to those will not cause warnings and go to _G.
--   Assignments to non-globals will go to _G.v.
--   Undefined reads from either _G or v will cause a warning (in dev mode).

local rawset = rawset
local rawget = rawget
local _G = _G

-- All global constants are UPPERCASE_DEFINE_NAMES (hopefully)
local function looksLikeGlobal(s)
    return not s:match("[^_%u%d]")
end

-- Restore functions that only existed in v1.1 and have been removed since then
-- Most are dummied out, but it allows some mods to run


-- loading entityinclude.lua is no longer necessary and would do more bad than good,
-- so make sure it's not loaded even if scripts explicitly load that file.
local o_dofile = dofile
local function dofileWrap(file)
    if file:lower() ~= "scripts/entities/entityinclude.lua" then -- already in
        --debugLog("dofile(" .. file .. ")")
        return o_dofile(file)
    end
end
rawset(_G, "dofile", dofileWrap)


-- Prepare interface function lookup table
local INTERFACE_LUT = {}
do
    local names = getInterfaceFunctionNames()
    --errorLog(table.concat(names, ", "))
    for i = 1, #names do
        INTERFACE_LUT[names[i]] = true
    end
end

-- Detour global reads and writes to the currently active instance table
-- n = 0   will become   v.n = 0
-- local x = entity_x(n)   will become   local x = entity_x(v.n)
-- important here is that no instance-local variables accidentally become globals.
-- globals that get written into the instance table are no problem. (if it ever happens - doubt it)
local _G_meta =
{
    __index = function(tab, key)
        if not INTERFACE_LUT[key] then
            local v = rawget(_G, "v")
            return v and v[key]
        end
    end,
    
    __newindex = function(tab, key, val)
        if key == "v" then -- v is set by the engine whenever script context is switched, this is no problem
            if val ~= nil and type(val) ~= "table" then
                errorLog("WARNING: COMPAT: Setting v to non-table (" .. type(v) .. ")! This is BAD and WILL break something!\n", 3)
                -- but do it anyway.
            end
            rawset(tab, key, val)
            return
        end
        if INTERFACE_LUT[key] then
            --debugLog("Setting interface: " .. tostring(key) .. " = " .. tostring(val))
            rawset(tab, key, val)
            return
        end
        if type(key) == "string" and looksLikeGlobal(key) then
            --debugLog("Setting global: " .. tostring(key) .. " = " .. tostring(val))
            rawset(tab, key, val)
            return
        end
        
        local v = rawget(_G, "v")
        if v then
            --debugLog("Setting v." .. tostring(key) .. " = " .. tostring(val))
            v[key] = val
            return
        end
        
        errorLog("Variable not set: " .. tostring(key) .. " = " .. tostring(val))
    end
}
setmetatable(_G, _G_meta)

debugLog("COMPAT/1.1: Redirecting global writes to v")
