-- This takes care of providing some deprecated functions that were added and later removed again
-- in the OSE version of Aquaria.

local util = dofile("scripts/compat/internal/util.lua")

local WARN_FUNCTIONS = {
    -- git commit f466e1e7c8d0a0a3ea40048a0a944419f9373dfb
    obj_setOverrideRenderPass = true,
    quad_setOverrideRenderPass = true,
    bone_setOverrideRenderPass = true,
    shot_setOverrideRenderPass = true,
    beam_setOverrideRenderPass = true,
    web_setOverrideRenderPass = true,
    text_setOverrideRenderPass = true,
    pe_setOverrideRenderPass = true,
}

local REPLACED_FUNCTIONS = {
    entity_setOverrideRenderPass = entity_setRenderPass,
}

util.makestubs(WARN_FUNCTIONS, util.mkwarn)
util.makestubs(REPLACED_FUNCTIONS, util.mkalias)
