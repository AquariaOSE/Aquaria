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

-- last known coordinates of Nautilus Prime
v.lx = 0
v.ly = 0

-- start out not having spawned the orb
v.spawned = false

function init(me)
end

function update(me, dt)
	if not v.spawned then
		local naut = getEntity("nautilusprime")
		
		-- if nautilus prime exists...
		if naut ~= 0 then
			-- store her location for later
			v.lx, v.ly = entity_getPosition(naut)
		else
			-- no nautilus prime! she must have died
			
			-- second parameter is a name override
			local orb = createEntity("EnergyOrb", "", v.lx, v.ly)
			
			-- do a fade-in effect
			
			-- set the orb to 0 transparency, invisible
			entity_alpha(orb, 0)
			
			-- set it to 1.0 full transparency over 1.0 seconds
			entity_alpha(orb, 1, 1)
			
			-- make sure we only spawn the orb once!
			v.spawned = true
		end
	end	
end
