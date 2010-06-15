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

mergog = 0

function spawnEgg(me)
	node = getNodeByName("COLLECTIBLECRABCOSTUMELOCATION")
	createEntity("CollectibleCrabCostume", "", node_x(node), node_y(node))
end

function init(me)
	if isFlag(FLAG_MINIBOSS_CRAB, 0) then
		mergog = createEntity("CrabBoss", "", node_x(me), node_y(me))
	else
		spawnEgg(me)
	end
end

function update(me, dt)
	if mergog ~= 0 then
		if entity_isState(mergog, STATE_DEAD) then
			setFlag(FLAG_MINIBOSS_CRAB, 1)
			spawnEgg(me)
			mergog = 0
		end
	end
end
