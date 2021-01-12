-- Some things that behave differently or have received fixes after v1.1
-- Loading this file restores the 1.1 behavior as much as is feasible

local BUILTIN = {}

local F =
{
}

-- TODO --

-- dofile() should not accept mod-relative paths
-- entity_color() should undo interpolation modes
-- *_getNearestNode() and *_getNearestEntity() -- remove ignore param
-- *_getNearestNode -- must only scan by full name
-- entity_setHealth(), entity_changeHealth() -- cast to int


for name, f in pairs(F) do
    if not BUILTIN[name] then
        BUILTIN[name] = assert(_G[name], name)
        _G[name] = f
    end
end
