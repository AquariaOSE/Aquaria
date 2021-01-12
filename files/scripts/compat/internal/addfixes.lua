-- Extra fixes to warn about compatibility issues, eg. crashes in earlier versions

local BUILTIN = {}

local function wouldcrash(fn)
    errorLog("Old Aquaria would have crashed now. Calling function: " .. fn)
end

local F =
{
    -- Old version would crash if e is not a string
    createEntity = function(e, name, x, y)
        if type(e) == "string" and #e > 0 then
            return BUILTIN.createEntity(e, name, x, y)
        end
        wouldcrash"createEntity"
        return 0
    end,
}

for name, f in pairs(F) do
    if not BUILTIN[name] then
        BUILTIN[name] = assert(_G[name], name)
        _G[name] = f
    end
end
