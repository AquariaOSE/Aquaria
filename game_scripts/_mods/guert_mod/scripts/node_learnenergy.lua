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


v.n = 0
v.isenergylearned = false

function init(me)
	v.n = getNaija()     
end


function update(me, dt)

if v.isenergylearned == false then
 if node_isEntityIn(me, v.n) then
   setControlHint("You've found a new song! You can now use ENERGY form!", 0, 0, 0, 16)
   learnSong(1)
   v.isenergylearned = true
 end
end 

end
