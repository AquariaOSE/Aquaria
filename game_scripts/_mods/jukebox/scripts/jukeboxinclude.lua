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

SONG_LIST = {
"abyss",
"ancienttest",
"arboreal",
"archaic",
"bblogo",
"bigboss",
"brightwaters",
"cathedral",
"cave",
"druniaddance",
"endingpart1",
"fallenbreed",
"fallofmithalas",
"flyaway",
"forestgod",
"gullet",
"hopeofwinter",
"icywaters",
"inevitable",
"licave",
"light",
"losttothewaves",
"lucien",
"marchofthekrotites",
"miniboss",
"mithala",
"mithalaanger",
"mithalaend",
"mithalapeace",
"moment",
"mystery",
"openwaters",
"openwaters2",
"openwaters3",
"prelude",
"prometheus",
"remains",
"seahorse",
"sunken",
"sunkencity",
"suntemple",
"sunworm",
"sunwormcave",
"superflyremix",
"test",
"thebody",
"theend",
"title",
"veil",
"worship1",
"worship2",
"worship3",
"worship4",
"worship5",
"youth",
}

--[[
DEFAULT_WATERLEVEL = 0

OVERRIDE_WATERLEVEL = {
	["ancienttest"] = -2700,
}
--]]

function jukebox_initButton(me)
	node_setCursorActivation(me, true)
	node_setCatchActions(me, true)
end

function jukebox_doButtonAction(me, action, state, transitions, isdefault)
	if isNestedMain() then return end
	if getNodeToActivate() == me and state == 1 then
		local name = transitions[action]
		if name then
			debugLog("jukebox_doButtonAction : "..node_getName(me).." -- "..action.." -> "..name)
			local node = getNode(name)
			setNodeToActivate(node)
			setMousePos(toWindowFromWorld(node_x(node), node_y(node)-20))
		end
		return false
	end
	if getNodeToActivate() == 0 and state == 1 and isdefault then
		if action == ACTION_MENURIGHT or action == ACTION_MENULEFT or action == ACTION_MENUUP or action == ACTION_MENUDOWN then
			setNodeToActivate(me)
			setMousePos(toWindowFromWorld(node_x(me), node_y(me)-20))
		end
		return false
	end
	return true
end

function jukebox_getSong()
    return getStory()
end

function jukebox_playSong(index)
	setStory(index)
	local songName = SONG_LIST[index]
	setControlHint("Now playing: "..songName)
	playMusic(songName)

	--[[
	local waterLevel = getWaterLevel()
	local newWaterLevel = OVERRIDE_WATERLEVEL[songName] or DEFAULT_WATERLEVEL
	if newWaterLevel ~= waterLevel then setWaterLevel(newWaterLevel) end
	--]]
end
