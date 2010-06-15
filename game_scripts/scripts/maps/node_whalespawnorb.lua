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

orb = 0

function init(me)
	if isFlag(FLAG_DEEPWHALE, 2) then
		orb = createEntity("EnergyOrb", "", node_x(me), node_y(me))
	end
	
	if isFlag(FLAG_DEEPWHALE, 3) then
		nd = getNode("OPENENERGYDOOR")
		orb = createEntity("EnergyOrb", "", node_x(nd), node_y(nd))
		entity_setState(orb, STATE_CHARGED, -1, 1)
	end
end

function update(me, dt)
	if orb ~= 0 then
		if isFlag(FLAG_DEEPWHALE, 2) then
			if entity_isState(orb, STATE_CHARGED) and node_isEntityIn(getNode("OPENENERGYDOOR"), orb) then
				setFlag(FLAG_DEEPWHALE, 3)
			end
		end
	end
end
