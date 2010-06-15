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
timer = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	--entity_initSkeletal(me, "")	
	entity_setAllDamageTargets(me, false)
	
	entity_setTexture(me, "missingimage")
	
	entity_setState(me, STATE_IDLE)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	entity_scale(me, 1, 1, 0.5)
end

function update(me, dt)
	
	
	if entity_isState(me, STATE_ATTACK) then
		--entity_delete(me)
		return
	end
	
	if entity_isState(me, STATE_IDLE) then
		timer = timer + dt
		if timer > 3 then
	
			s = createShot("cf6-shot-shot", me, n, entity_x(me), entity_y(me))
			shot_setAimVector(s, entity_velx(me), entity_vely(me))
			
			entity_setState(me, STATE_ATTACK)
			entity_delete(me, 0.1)
			timer = 0
			return
		end
		entity_moveTowardsTarget(me, dt, 200)
	else
		timer = timer + dt
		if timer > 3 then
			entity_delete(me)
			return
		end
		entity_moveTowardsTarget(me, dt, 1000)
		--entity_addVel(me, entity_velx(n)*0.1, entity_vely(n)*0.1)
	end
	entity_updateMovement(me, dt)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
		entity_setMaxSpeed(me, 100)
	elseif entity_isState(me, STATE_ATTACK) then
		--s = createShot("cf6-shot-shot", entity_x(me), entity_y(me))
		
		--[[
		entity_moveTowardsTarget(me, 1, 800)
		timer = 0
		entity_setMaxSpeed(me, 2000)
		]]--
		
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

