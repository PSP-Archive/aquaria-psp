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

grab = 0
head = 0

attached = 0

STATE_LUNGE			= 1000
STATE_BACK			= 1001
STATE_WAIT2			= 1002

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "seawolf")
	
	entity_setEntityLayer(me, 1)
	
	entity_setAllDamageTargets(me, false)
	
	grab = entity_getBoneByName(me, "grab")
	head = entity_getBoneByName(me, "head")
	
	--entity_generateCollisionMask(me)	
	
	entity_setState(me, STATE_IDLE)
	
	entity_setCullRadius(me, 2000)
	entity_setUpdateCull(me, 2300)
	
	bone_alpha(grab, 0)
	
	entity_offset(me, 0, -5)
	entity_offset(me, 0, 5, 1, -1, 1, 1)
	
	loadSound("seawolf")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	
	n = entity_getNearestNode(me, "flip")
	if n ~= 0 then
		if node_isEntityIn(n, me) then
			entity_fh(me)
		end
	end
end

function update(me, dt)
	n = getNaija()
	--entity_updateMovement(me, dt)
	bx,by = bone_getWorldPosition(grab)
	
	if entity_isState(me, STATE_IDLE) then
		--debugLog(string.format("%f, %f", bx, by))
		if entity_isPositionInRange(n, bx, by, 160) then
			debugLog("SETTING STATE LUNGE")
			entity_setState(me, STATE_LUNGE)
		end
	end
	
	bx2,by2 = bone_getWorldPosition(head)
	if attached ~= 0 then
		entity_setPosition(attached, bx2, by2)
	end
	
	--[[
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	]]--
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_LUNGE) then
		playSfx("seawolf")
		entity_setStateTime(me, entity_animate(me, "lunge"))
	elseif entity_isState(me, STATE_WAIT) then
		if attached ~= 0 then
			entity_setStateTime(me, 3)
		else
			entity_setStateTime(me, 0.1)
		end
	elseif entity_isState(me, STATE_BACK) then
		debugLog("back")
		entity_setStateTime(me, entity_animate(me, "back"))
	end
end

function exitState(me)
	if entity_isState(me, STATE_LUNGE) then
		bx,by = bone_getWorldPosition(grab)
		if entity_isPositionInRange(n, bx, by, 160) then
			attached = n
		end
		if attached ~= 0 then
			entity_damage(attached, me, 1)
		end
		entity_setState(me, STATE_WAIT)
	elseif entity_isState(me, STATE_WAIT) then
		entity_setState(me, STATE_BACK)
	elseif entity_isState(me, STATE_WAIT2) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_BACK) then
		if attached~=0 then
			if attached == n then
				--centerText("Lost Ingredients!")
				vx, vy = entity_getNormal(me)
				p = vy
				vy = -vx
				vx = p
				if entity_isfh(me) then
					vx = -vx
				end
				vx, vy = vector_setLength(vx, vy, 2000)
				entity_idle(n)
				entity_push(n, vx, vy, 1, 3000, 0.5)
			end
				
		end
		attached = 0
		entity_setState(me, STATE_WAIT2, 3)
	end
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

