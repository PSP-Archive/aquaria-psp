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

add = math.random(50)

minCap = 400
maxCap = 700
cap = minCap

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_setTexture(me, "title/minnow")
	entity_setAllDamageTargets(me, false)
	
	entity_setEntityLayer(me, -1)
	
	entity_alpha(me, 0.5)
	
	entity_setState(me, STATE_IDLE)
	esetv(me, EV_LOOKAT, 0)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)	
	cap = cap - dt*400
	if cap < minCap then
		cap = minCap
	end
	if isLeftMouse() then
		cap = maxCap
		add = 600
	end
	--entity_doCollisionAvoidance(me, dt, 4, 0.5)
	entity_doEntityAvoidance(me, dt, 16, 0.5)
	x, y = getMouseWorldPos()
	entity_moveTowards(me, x, y, dt, 800+add)
	
	vx = entity_velx(me)
	vy = entity_vely(me)
	
	vx, vy = vector_cap(vx, vy, cap)
	entity_clearVel(me)
	entity_addVel(me, vx, vy)
	
	entity_setPosition(me, entity_x(me) + entity_velx(me)*dt, entity_y(me)+entity_vely(me)*dt)

	--entity_updateMovement(me, dt)
	entity_rotateToVel(me)
	
	len = vector_getLength(vx, vy)
	addInfluence(entity_x(me), entity_y(me), 16, len)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	end
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function animationKey(me, key)
end

function hitSurface(me)
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

