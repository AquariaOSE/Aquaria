-- Copyright (C) 2007, 2010 - Bit-Blot
--
-- This file is part of Aquaria.
--
-- Aquaria is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
--
-- See the GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

if not v then v = {} end
if not AQUARIA_VERSION then dofile("scripts/entities/entityinclude.lua") end

dofile(appendUserDataPath("_mods/jukebox/scripts/jukeboxinclude.lua"))

--[[
Button layout on screen:

         RAND
  PREV          NEXT 
  
                    *EXIT*
--]]

local navmap = {
    [ACTION_MENUUP] = "jukebox-random",
    [ACTION_MENULEFT] = "jukebox-previous",
    [ACTION_MENURIGHT] = "jukebox-next"
}

function init(me)
	node_setCursorActivation(me, true)
	node_setCatchActions(me, true)
end

function action(me, action, state)
	if isNestedMain() then return end
	return jukebox_doButtonAction(me, action, state, navmap)
end

function activate(me)
	if isNestedMain() then return end

	playSfx("TitleAction")
	spawnParticleEffect("TitleEffect1", node_x(me), node_y(me))
	watch(0.5)
	
	local doQuit = false
	
	if confirm("", "exit") then
		doQuit = true
	end

	setNodeToActivate(0)
	
	if doQuit then
		fadeOutMusic(2)
		toggleCursor(false)
		fade(1, 2, 0, 0, 0)
		watch(2)
		watch(0.5)
		goToTitle()
	end
end

function update(me, dt)
end
