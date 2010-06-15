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
	n = getNaija()
end

function update(me, dt)
	if not isForm(FORM_NORMAL) and node_isEntityIn(me, n) then
	
		playSfx("shield-hit")
		spawnParticleEffect("barrier-hit", entity_x(n), entity_y(n))
		
		w, h = node_getSize(me)
		entity_clearVel(n)
		if w > h then
			y = entity_y(n) - node_y(me)
			if entity_y(n) < node_y(me) then
				entity_setPosition(n, entity_x(n), node_y(me) - (h/2) - 10)
			else
				entity_setPosition(n, entity_x(n), node_y(me) + (h+10)/2 + 10)
			end
		else
			x = entity_x(n) - node_x(me)
		end
		
		
		x, y = vector_setLength(x, y, 10000)
		entity_setMaxSpeedLerp(n, 4)
		entity_setMaxSpeedLerp(n, 1, 4)
		entity_addVel(n, x, y)

		if chance(50) then
			emote(EMOTE_NAIJAUGH)
		end
	end
end
