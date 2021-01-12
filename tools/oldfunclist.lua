-- To generate the list of function names available in 1.1, run this script on ScriptInterface.cpp
-- obtained from:
-- https://hg.icculus.org/icculus/aquaria/raw-file/75924df9cbe8/Aquaria/ScriptInterface.cpp


local F = {}

for line in io.lines((...)) do
    local f = line:match'^%s*lua_register%s*%(.-%"([^%"]+)%".-%)'
    if f then
        F[f] = true
    end
    
    f = line:match'^%s*luar%s*%(%s*([%w_]+)%s*%)'
    if f then
        F[f] = true
    end
end

-- These are commented out but the regex picks them up
F.opt = nil
F.options = nil

local LIST = {}
for k in pairs(F) do
    table.insert(LIST, k)
end
table.sort(LIST)
print(table.concat(LIST, "\n"))
