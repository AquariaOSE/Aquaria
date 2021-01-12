-- Registers old functions that have been deprecated in 1.1.1 already,
-- but some mods may still use them.
-- Most that had no function are dummied out, some existed but threw errors,
-- and others were registered under different names.

local NULLREF = 0

-- these did something but are now gone
local WARN_FUNCTIONS =
{
    healEntity = true,
    killEntity = true,
    entity_warpToPathStart = true,
    sendEntityMessage = true,
    entity_fireShot = true,
    entity_resetTimer = true,
    moveEntity = true,
    playVfx = true,
    registerSporeChildData = true,
    setMiniMapHint = true,
    setupConversationEntity = true,
}

-- These returned something important, so here we just return a failure/dummy value
local WARN_FUNCTIONS_VAL =
{
    getAngleBetween = 0.0,
    getAngleBetweenEntities = 0.0,
    getEntityInGroup = NULLREF,
    getNearestNode = NULLREF,
    getNodeFromEntity = NULLREF,
    isInDialog = false,
}

-- These had no function and we can just ignore the call
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
    setEntityScript = true,
    streamSfx = true,
}

-- Deprecated stuff from v1.1's scripts/entities/entityinclude.lua

local function entity_watchSwimToEntitySide(ent1, ent2)
    local xoff=entity_getCollideRadius(ent2)+64
    if entity_x(ent1) < entity_x(ent2) then
        xoff = -xoff
    end
    entity_swimToPosition(ent1, entity_x(ent2)+xoff, entity_y(ent2))
    entity_watchForPath(ent1)
    entity_idle(ent1)
    entity_clearVel(ent1)
    entity_flipToEntity(ent1, ent2)
    entity_flipToEntity(ent2, ent1)
end


-- Duplicated and renamed functions
local REPLACED_FUNCTIONS =
{
    -- alternate names, old name on the left, new name on the right
    -- (Might want tp use the name on the right, they existed in 1.1.1 already)
    entity_getPositionX = entity_x,
    entity_getPositionY = entity_y,
    entity_applyRandomForce = entity_addRandomVel,
    getNodeByName = getNode,
    getEntityByName = getEntity,
    entity_flipTo = entity_fhTo,
    bone_getidx = bone_getIndex,
    getAvatar = getNaija,
    getRandVector = randVector,
    inp = toggleInput,
    isPlayingVoice = isStreamingVoice,
    playVoice = voice,
    
    -- These are unfortunately broken and can't be fixed.
    -- They are interface function names and the first loaded script would grab them,
    -- thinking an interface function with that name was provided by whichever script was just loaded.
    -- So we ignore these and hope nobody misses them.
    --msg = screenMessage,
    --castSong = singSong,
    
    -- entityinclude functions
    entity_watchSwimToEntitySide = entity_watchSwimToEntitySide,
}

----------------------------------------------------
---- Functors to generate replacement function -----
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
local function mkdummy(name, param)
    return dummy
end


-- register existing function under a different name
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
makestubs(WARN_FUNCTIONS_VAL, mkwarnret)
makestubs(DUMMY_FUNCTIONS, mkdummy)
makestubs(REPLACED_FUNCTIONS, mkalias)
