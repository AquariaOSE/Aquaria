-- Script loader hijack
-- Intercepts loads of script files and patches interface functions on the fly
-- Mods may provide their own mod-compat.lua file (optionally containing aditional hooks).
-- Requires instance variables in "v".

assert(AQUARIA_VERSION)

local rawset = rawset
local rawget = rawget
local debug = rawget(_G, "debug") 
local errorLog_o = assert(errorLog, "errorLog() not present")

-- IMPORTANT that this gets loaded only once. Exit & re-enter the mod if the file was changed.
if rawget(_G, ".__COMPAT_LOADED") then
    errorLog_o("COMPAT: Compat loader already loaded")
    return
end

local function formatStack(lvl)
    if debug then
        return debug.traceback("", lvl or 1) or "[No traceback available]"
    end
    return "[No debug library available]"
end

local function errorLogWrap(s, level)
    return errorLog_o(s .. "\n" .. formatStack(level or 2))
end
rawset(_G, "errorLog", errorLogWrap)

---------------------------------------------------------------------
---- Create hook scripts for entity interface function detouring ----
---------------------------------------------------------------------

local WARNINGS --= isDeveloperKeys()
local HOOKS
if fileExists("mod-compat.lua") then
    HOOKS = dofile("mod-compat.lua")
end
HOOKS = HOOKS or {}
assert(type(HOOKS) == "table", "mod-compat.lua must return nothing or table, not " .. type(HOOKS))

local v_meta
if WARNINGS then
    local function _warnUndefInstance(tab, key)
        if WARNINGS then
            errorLog("COMPAT/WARNING: script tried to get/call undefined instance variable " .. tostring(key), 3)
            rawset(tab, key, false) -- warn only once, not spam
        end
    end
    v_meta = { __index = _warnUndefInstance }
end

-- Callback function, to be called whenever a new script is loaded and stored by the scripting interface.
-- Note: This function must never raise an error, otherwise the program will crash!
local function onCreateScript(scriptname, functable)
    debugLog("=== Creating script instance: " .. tostring(scriptname) .. " ===")
    assert(type(functable) == "table")
    
    -- detour init() function to patch v to produce reasonable stackdumps,
    -- and optionally call custom hook
    local oldinit = functable.init
    local function newinit(me)
        assert(v)
        setmetatable(v, v_meta) -- global lookup: uses entity context's v
        -- ^ and yes, this overwrites v's previous metatable that is added by C++ in dev mode.
        -- We don't want warnings for mods that don't initialize their variables
        if oldinit then
            oldinit(me)
        end
        if me and me ~= 0 then
            local hook = HOOKS.init
            if hook then
                return hook(scriptname, me)
            end
        end
    end
    functable.init = newinit
    
    local postInitHook = HOOKS.postInit
    if postInitHook then
        local oldpostinit = functable.postInit
        function functable.postInit(me)
            oldpostinit(me)
            postInitHook(me)
        end
    end
    
    local f = HOOKS.onCreateScript
    if f then
        f(scriptname, functable)
    end
end

-- Hidden, secret global table that stores interface functions for script templates
local _scriptfuncs = assert(_scriptfuncs, "_scriptfuncs missing")

-- Intercept writes to _scriptfuncs. Game uses lua_setfield() to populate the table, which honors metatables.
setmetatable(_scriptfuncs, {
    __newindex = function(tab, scriptname, functable)
        onCreateScript(scriptname, functable)
        -- do the set, or it will crash
        rawset(tab, scriptname, functable)
    end
})

rawset(_G, ".__COMPAT_LOADED", true)
debugLog("COMPAT/loader: Installed")
