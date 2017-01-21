-- Registers old functions that have been deprecated in 1.1.1 already,
-- but some mods may still use them.
-- Most that had no function are dummied out, some existed but threw errors,
-- and others were registered under different names.

local WARN_FUNCTIONS =
{
    getAngleBetween = true,
    getAngleBetweenEntities = true,
    getNearestNode = true,
    getNodeFromEntity = true,
    healEntity = true,
    killEntity = true,
    entity_warpToPathStart = true,
    sendEntityMessage = true,
    entity_fireShot = true,
    entity_resetTimer = true,
}

local DUMMY_FUNCTIONS =
{
    entity_setCollideWithAvatar = true,
    entity_setTouchDamage = true,
    bone_setTouchDamage = true,
    entity_setClampOnSwitchDir = true,
    entity_addGroupVel = true,
    entity_avgVel = true,
    entity_fireAtTarget = true,
    entity_flipHToAvatar = true,
    entity_getBehaviorType = true,
    entity_isSaying = true,
    entity_moveTowardsGroupCenter = true,
    entity_moveTowardsGroupHeading = true,
    entity_say = true,
    entity_setAffectedBySpells = true,
    entity_setBehaviorType = true,
    entity_setNodeGroupActive = true,
    entity_setSayPosition = true,
    entity_setTouchPush = true,
    entity_setPauseInConversation = true,
    learnSpell = true,
    reloadTextures = true,
    setGameOver = true,
    setGLNearest = true,
    setNaijaModel = true,
    shot_setBounceType = true,
    showControls = true,
    stopCursorGlow = true,
    toggleTransitFishRide = true,
    entity_stopTimer = true,
}

local REPLACED_FUNCTIONS =
{
    -- alternate names
    entity_getPositionX = entity_x,
    entity_getPositionY = entity_y,
    entity_applyRandomForce = entity_addRandomVel,
    getNodeByName = getNode,
    getEntityByName = getEntity,
    entity_flipTo = entity_fhTo,
    bone_getidx = bone_getIndex,
}

----------------------------------------------------
---- Functors to generate replacement function -----
----------------------------------------------------

local function mkwarn(name, param)
    local err = "Dummy function: " .. name .. "() - no longer present in the current API, fix the script!"
    return function() errorLog(err) end
end

local function dummy(name, param)
end
local function mkdummy(name, param)
    return dummy
end
local function mkalias(name, param)
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

----------------
---- Do it! ----
----------------
makestubs(WARN_FUNCTIONS, mkwarn)
makestubs(DUMMY_FUNCTIONS, mkdummy)
makestubs(REPLACED_FUNCTIONS, mkalias)
