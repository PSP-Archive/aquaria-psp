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

n=0
sz = 64
delay = 0
		
function init(me)
	n = getNaija()
end

function spawnSpore(x, y)
	-- SLOW have to load a script each time
	createSpore(x, y)
	--[[
	ent = createEntity("Spore", "", x, y)
	e = entity_getNearestEntity(ent, "Spore")
	if e ~= 0 then
		if entity_isEntityInRange(ent, e, sz/2) then
			entity_delete(ent)
		end
	end
	]]--
end

function update(me, dt)
	delay = delay - dt
	if delay < 0 then
		if node_isEntityIn(me, n) then
			-- spawn around on a grid
			x,y = entity_getPosition(n)

			x = math.floor(x / sz)
			y = math.floor(y / sz)
			x = x * sz + sz/2
			y = y * sz + sz/2
			
			out = 4
			spawnSpore(x, y-sz*out)
			spawnSpore(x+sz*out, y-sz*out)
			spawnSpore(x-sz*out, y-sz*out)
			
			spawnSpore(x, y+sz*out)
			spawnSpore(x+sz*out, y+sz*out)
			spawnSpore(x-sz*out, y+sz*out)
			
			spawnSpore(x+sz*out, y)
			spawnSpore(x-sz*out, y)
		end
		delay = 0.5
	end
end
