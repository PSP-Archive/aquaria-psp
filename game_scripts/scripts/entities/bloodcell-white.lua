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

dofile("scripts/entities/bloodcell-common.lua")

fireDelay = 0
fireDelayTime = 1

function init(me)
	commonInit(me, "bloodcell-white")
	
	esetv(me, EV_TYPEID, EVT_CELLWHITE)
end

function fire(me)
	e = getFirstEntity()
	target = 0
	while e ~= 0 do
		if entity_getEntityType(e)==ET_ENEMY and not eisv(e, EV_TYPEID, EVT_CELLWHITE) and not eisv(e, EV_TYPEID, EVT_PET) then
			if entity_isEntityInRange(me, e, 450) then
				target = e
				break
			end
		end
		e = getNextEntity()
	end
	if target ~= 0 then
		s = createShot("energyblast", me, target, entity_x(me), entity_y(me))
	end
	fireDelay = fireDelayTime + randRange(0,2)
end

function update(me, dt)
	commonUpdate(me, dt)
	
	if sing then
		entity_moveTowardsTarget(me, dt, 300)
	end
	
	fireDelay = fireDelay - dt
	if fireDelay < 0 then
		fire(me)
	end

	rangeNode = entity_getNearestNode(me, "KILLENTITY")
	if node_isPositionIn(rangeNode, entity_x(me), entity_y(me)) then
		entity_setState(me, STATE_DIE)
	end
end

function songNote(me, note)
	r,g,b = getNoteColor(note)
	bone_setColor(glow, r*0.5+0.5, g*0.5+0.5, b*0.5+0.5)
	bone_alpha(glow, 0.4, 1)
	sing = true
end

function songNoteDone(me, note, t)
	bone_alpha(glow, 0, 4)
	sing = false
end
