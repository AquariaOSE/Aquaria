-- Remove all functions that didn't exist in v1.1

-- The known-good list of old script functions
local oldfuncs = dofile("scripts/compat/internal/funclist_1.1.lua")

-- Ignore any standard Lua function known to exist in _G
local luafuncs51 =
{
    assert = true,
    collectgarbage = true,
    dofile = true,
    error = true,
    getfenv = true,
    getmetatable = true,
    ipairs = true,
    load = true,
    loadfile = true,
    loadstring = true,
    next = true,
    pairs = true,
    pcall = true,
    print = true,
    rawequal = true,
    rawget = true,
    rawset = true,
    select = true,
    setfenv = true,
    setmetatable = true,
    tonumber = true,
    tostring = true,
    type = true,
    unpack = true,
    xpcall = true,
    newproxy = true,
}

for k, f in pairs(_G) do
    if      type(k) == "string"
        and type(f) == "function"
        and not luafuncs51[k]
        and not oldfuncs[k]
    then
        debugLog("Compatibility: Remove new script function " .. k)
        _G[k] = nil
    end
end
