-- Generate list of script function names available in the OSE version
-- call with path/to/ScriptInterface.cpp

local RO = { flipHorizontal = true, flipVertical = true }
local Q = {}
local F = {}

local function emit(prefix, a)
    for k in pairs(a) do
        F[prefix .. "_" .. k] = true
    end
end

for line in io.lines((...)) do
    local fn = line:match"^%s*luaRegister%(([%w_]+)%)"
    if fn then
        F[fn] = true
    end
    
    local rf = line:match"^%s*RO_FUNC%(.-([%w_]+)%s*%)"
    if rf then
        RO[rf] = true
    end
    
    local qf = line:match"^%s*Q_FUNC%(.-([%w_]+)%s*%)"
    if qf then
        Q[qf] = true
        RO[qf] = true
    end
    
    local lut = { QUAD = Q, ROBJ = RO }
    local c, p = line:match"^%s*MAKE_([A-Z]+)_FUNCS%(.-([%w_]+)%s*%)"
    if c and p ~= "prefix" then
        --print(c, p, line)
        emit(p, lut[c])
    end
end

local LIST = {}
for k in pairs(F) do
    table.insert(LIST, k)
end
table.sort(LIST)
print(table.concat(LIST, "\n"))

