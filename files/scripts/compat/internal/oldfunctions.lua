-- Registers old functions that have been deprecated in 1.1.1 already,
-- but some mods may still use them.
-- Most that had no function are dummied out, some existed but threw errors,
-- and others were registered under different names.

local util = dofile("scripts/compat/internal/util.lua")

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
    entity_grabTarget = true,
    entity_releaseTarget = true,
    entity_watchEntity = true,
    entityFollowEntity = true,
    entity_followEntity = true,
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

-- These had no effective function and we can just ignore the call
local DUMMY_FUNCTIONS =
{
    entity_setCollideWithAvatar = true,
    entity_setTouchDamage = true,
    bone_setTouchDamage = true,
    entity_setClampOnSwitchDir = true,
    entity_addGroupVel = true,
    entity_avgVel = true,
    entity_fireAtTarget = true,
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
    toggleVersionLabel = true,
    setVersionLabelText = true,
    entity_isFollowingEntity = true, -- never following anything now since those functions are gone
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

-- Removed unused functions but re-implemented in Lua

local function entity_flipHToAvatar(me)
    return entity_flipToEntity(me, getNaija())
end


-- Duplicated and renamed functions
local REPLACED_FUNCTIONS =
{
    -- alternate names, old name on the left, new name on the right
    -- (Might want to use the name on the right, they existed in 1.1.1 already)
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
    -- (This happens to be Li if he's available, or any other entity if he's not spawned)
    -- So we ignore these and hope nobody misses them.
    --msg = screenMessage,
    --castSong = singSong,

    -- entityinclude functions
    entity_watchSwimToEntitySide = entity_watchSwimToEntitySide,

    -- removed from C++ but implemented in Lua
    entity_flipHToAvatar = entity_flipHToAvatar,
}


util.makestubs(WARN_FUNCTIONS, util.mkwarn)
util.makestubs(WARN_FUNCTIONS_VAL, util.mkwarnret)
util.makestubs(DUMMY_FUNCTIONS, util.mkdummy)
util.makestubs(REPLACED_FUNCTIONS, util.mkalias)
