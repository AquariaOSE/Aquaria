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
  PREV         *NEXT*
  
                     EXIT
--]]

local navmap = {
    [ACTION_MENUUP] = "jukebox-random",
    [ACTION_MENULEFT] = "jukebox-previous",
    [ACTION_MENUDOWN] = "jukebox-quit"
}

function init(me)
    jukebox_initButton(me)
end

function action(me, action, state)
	if isNestedMain() then return end
    return jukebox_doButtonAction(me, action, state, navmap)
end

function activate(me)
	if isNestedMain() then return end
	
	--playSfx("TitleAction")
	spawnParticleEffect("TitleEffect1", node_x(me), node_y(me))
	watch(0.5)
	
    local n = jukebox_getSong()
    if n < #SONG_LIST then n = n + 1 else n = 1 end
    jukebox_playSong(n)
end

function update(me, dt)
end
