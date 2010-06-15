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

ompo = 0
n = 0
d = false

function init(me)
	n = getNaija()
end

function update(me)
	if not d and node_isEntityIn(me, n) then
		d = true
		h = getEntity("healthupgrade3")
		
		if h ~= 0 then
			
			entity_idle(n)
			watch(1)
			setCameraLerpDelay(0.5)
			cam_toEntity(h)
			watch(1)
			
			watch(1)
			emote(EMOTE_NAIJAWOW)
			watch(1)
			cam_toEntity(n)
			watch(2)
			
			setCameraLerpDelay(0)
		end
	end
end

