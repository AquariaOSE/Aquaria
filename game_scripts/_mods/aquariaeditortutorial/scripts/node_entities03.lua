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

v.started 		= false
v.n 			= 0
v.timer 			= 999
v.thingsToSay		= 20
v.thingSaying		= -1
v.timeToSay		= 5
function init(me)
	v.n = getNaija()
	node_setCursorActivation(me, true)
end

local function sayNext()
	if v.thingSaying == 0 then
		setControlHint("To select a new entity to drop, hold Control and press 'E'.", 0, 0, 0, 16)
	elseif v.thingSaying == 1 then
		setControlHint("You can also select this from the drop-down menu.", 0, 0, 0, 16)
	elseif v.thingSaying == 2 then
		setControlHint("Use 'E' and 'R' to change entity groups and use the Left Mouse Button to select an entity.", 0, 0, 0, 16)
	elseif v.thingSaying == 3 then
		setControlHint("Once you've selected an entity to drop, press Escape to exit the Entity Selection Menu.", 0, 0, 0, 16)
	elseif v.thingSaying == 4 then
		setControlHint("Press Spacebar to drop the entity! That's all there is to it!", 0, 0, 0, 16)
	end
end

function update(me, dt)
	if getStringFlag("editorhint") ~= node_getName(me) then
		v.started = false
		return
	end
	if v.started then
		v.timer = v.timer + dt
		if v.timer > v.timeToSay then
			v.timer = 0
			v.thingSaying = v.thingSaying + 1
			sayNext()
		end
	end
end

function activate(me)
	clearControlHint()
	v.started = true
	v.thingSaying = -1
	v.timer = 999
	setStringFlag("editorhint", node_getName(me))
end

