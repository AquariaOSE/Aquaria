----------------------------------------------------
---- Functors to generate replacement functions -----
----------------------------------------------------

local warnLog = (isDeveloperKeys() and errorLog) or debugLog

-- generate function that warns when called and returns nil
local function warndummy(name)
    warnLog("Dummy function: " .. name .. "() - no longer present in the current API, fix the script!")
end
local function mkwarn(name)
    return function() warndummy(name) end
end

-- generate function that warns when called and returns a non-nil fixed value
local function mkwarnret(name, param)
    return function() warndummy(name) return param end
end

-- generate silent dummy that does nothing when called and returns nil
local function dummy() end
local function mkdummy(name)
    return dummy
end


-- register existing function under a different name
local function mkalias(name, param)
    if type(param) == "string" then
        param = _G[param]
    end
    return assert(param, name)
end

local function makestubs(tab, gen)
    for name, param in pairs(tab) do
        if rawget(_G, name) then
            errorLog("WARNING: oldfunctions.lua: function " .. name .. " already exists")
        else
            local f = gen(name, param)
            rawset(_G, name, f)
        end
    end
end

return {
    warnLog = warnLog,
    warndummy = warndummy,
    mkwarn = mkwarn,
    mkwarnret = mkwarnret,
    mkdummy = mkdummy,
    mkalias = mkalias,
    makestubs = makestubs,
}
