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

dofile("scripts/entities/entityinclude.lua")

n = 0

function init(me)
end

function update(me, dt)
	n = getNaija()
	if not isForm(FORM_FISH) and node_isEntityIn(me, n) then
		x = entity_x(n) - node_x(me)
		y = entity_y(n) - node_y(me)
		avatar_fallOffWall()
		vector_setLength(x, y, 20000*dt)
		entity_clearVel(n)
		entity_addVel(n, x, y)
		entity_addVel2(n, x, y)
		
		entity_warpLastPosition(n)
	end
end

function songNote(me, note)
end

function songNoteDone(me, note, done)
end
