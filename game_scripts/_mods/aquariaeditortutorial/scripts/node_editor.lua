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

v.started 			= false
v.n 				= 0
v.timer 			= 999
v.thingsToSay		= 20
v.thingSaying		= -1
v.timeToSay		= 7

function init(me)
	v.n = getNaija()
end

local function sayNext()
	if v.thingSaying == 0 then
		setControlHint("Hi, I'm Derek, one of the two developers of Aquaria!", 0, 0, 0, 16)
	elseif v.thingSaying == 1 then
		setControlHint("To get you started right away, I'm going to tell you how to get into the editor and start changing stuff on this level!", 0, 0, 0, 16)
	elseif v.thingSaying == 2 then
		setControlHint("You can hit TAB to toggle editing mode off or on. Once in editing mode, a menu and other options become available. Hit TAB to come back when you're ready to hear more!", 0, 0, 0, 16)
	elseif v.thingSaying == 3 then
		setControlHint("In the editor, you can hold the middle mouse button or use the A,S,D,W keys to move the view around. Use the scroll wheel or PageUp and PageDown buttons to zoom in and out!", 0, 0, 0, 16)
	elseif v.thingSaying == 4 then
		setControlHint("There are three main editing modes: Tile Edit, Entity Edit and Node Edit. In the editor, you can switch between these modes quickly using F5, F6 and F7. You can also use the menu at the top of the screen.", 0, 0, 0, 16)
	elseif v.thingSaying == 5 then
		setControlHint("In Tile Edit Mode, use the Left Mouse Button to drag tiles around. You can also hold right mouse button and drag to rotate them. Try moving these plants next to me!", 0, 0, 0, 16)
	elseif v.thingSaying == 6 then
		setControlHint("You can hold Shift and press Left Mouse Button to Clone a tile. Try it out!", 0, 0, 0, 16)
	elseif v.thingSaying == 7 then
		setControlHint("If you hover the cursor over a tile and press the 'E' or 'R' key, you can cycle through the tiles. You can also use E and R to select the 'placer tile'. Drop a new tile by pressing spacebar.", 0, 0, 0, 16)
	elseif v.thingSaying == 8 then
		setControlHint("Not too hard, right? If you need to delete a tile, press Delete or Backspace.", 0, 0, 0, 16)
	end
end

function update(me, dt)
	if v.started and node_isEntityIn(me, v.n) then
		v.timer = v.timer + dt
		if v.timer > v.timeToSay then
			v.timer = 0
			v.thingSaying = v.thingSaying + 1
			sayNext()
		end
	end
	if not v.started and node_isEntityIn(me, v.n) then
		v.started = true

	end
end
