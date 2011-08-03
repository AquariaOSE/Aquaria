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
		setControlHint("Currents are defined by nodes.", 0, 0, 0, 16)
	elseif v.thingSaying == 1 then
		setControlHint("To add a link to a node, hover your mouse over the center of the node and press Spacebar.", 0, 0, 0, 16)
	elseif v.thingSaying == 2 then
		setControlHint("To set what the node does, hover your mouse over the center of the node and press 'N'.", 0, 0, 0, 16)
	elseif v.thingSaying == 3 then
		setControlHint("If you try it on this current node, you will see that it is defined as a current with power 500.", 0, 0, 0, 16)
	elseif v.thingSaying == 4 then
		setControlHint("The '0.1' defines the transparency of the current.", 0, 0, 0, 16)
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

