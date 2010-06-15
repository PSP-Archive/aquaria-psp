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
soundDelay = 0.1
inCurrent = false
updateDelay = 0
updateDelayTime = 0.5
groupNode = 0
bounceGroup = {}
bounceGroupSize = 1

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "UpsideDownJelly")	
	entity_setAllDamageTargets(me, true)
	entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, false)
	entity_setDamageTarget(me, DT_AVATAR_SHOCK, false)
	entity_setCollideRadius(me, 40)
	entity_setHealth(me, 9)	
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	entity_setUpdateCull(me, 1024)
	entity_setState(me, STATE_IDLE)
	bone_setSegs(entity_getBoneByName(me, "Tentacles"), 8, 2, 0.1, 0.9, 0, -0.03, 8, 0)	
	esetv(me, EV_ENTITYDIED, 1)
	loadSound("SpikyBounce")
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
end

function grabBounceGroup(me)
	bounceGroup = {}
	if groupNode ~= 0 then

		e = getFirstEntity()
		c = 1
		while e ~= 0 do
			if e ~= me and not entity_isDead(e) and entity_getEntityType(e) == ET_ENEMY and node_isEntityIn(groupNode, e) then
				--debugLog(string.format("c: %d e: %d", c, e))
				bounceGroup[c] = e
				c = c + 1
			end
			e = getNextEntity()
		end
		bounceGroupSize = c
		--debugLog(string.format("bounceGroupSize: %d", bounceGroupSize))
	end
end

function postInit(me)
	e = 0
	n = getNaija()
	entity_setTarget(me, n)
	entity_update(me, math.random(100)/200.0)
	inCurrent = entity_updateCurrents(me, dt)
	
	groupNode = entity_getNearestNode(me, "JELLYBOUNCE")
	grabBounceGroup(me)
end

function bounceReact(me)
	entity_scale(me, 1, 1)
	entity_scale(me, 0.8, 1, 0.1, 5, 1)		
	if soundDelay < 0 then
		entity_sound(me, "SpikyBounce", 400+math.random(200))
		soundDelay = 0.8
	end
end

-- warning: only called if EV_ENTITYDIED set to 1!
function entityDied(me, died)
	--debugLog("entityDied!!")
	--[[
	if bounceGroupSize ~= 1 then
		for i=1,bounceGroupSize-1 do
			if bounceGroup[i] == died then
				--debugLog("   erasing ent!")
				bounceGroups[i] = 0
			end
		end
	end
	]]--
	grabBounceGroup(me)
end

function update(me, dt)
	if soundDelay > 0 then
		soundDelay = soundDelay - dt
	end
	--entity_updateCurrents(me, dt)
	--entity_updateMovement(me, dt)	
	
	
	entity_clearVel2(me)
	if entity_touchAvatarDamage(me, entity_getCollideRadius(me), 0, 1200) then
		if not isForm(FORM_NATURE) then
			bounceReact(me)
--[[
			vx, vy = entity_getVectorToEntity(me, n)
			dx, dy = vector_setLength(vx, vy, entity_getCollideRadius(me)+entity_getCollideRadius(n)+1)
			vx, vy = vector_setLength(vx, vy, 2000)
			entity_push(n, vx, vy, 0.5, 2000, 0)
			entity_addVel(n, vx, vy)
			vx, vy = vector_setLength(vx, vy, 2000)
			entity_clearVel2(n)			
			entity_setPosition(n, entity_x(me)+dx, entity_y(me)+dy)	
]]--			
			if inCurrent then
				vx, vy = entity_getVectorToEntity(me, n)
				dx, dy = vector_setLength(vx, vy, entity_getCollideRadius(me)+entity_getCollideRadius(n)+1)
				vx, vy = vector_setLength(vx, vy, 2500)
				entity_push(n, vx, vy, 0.5, 2500, 0)
				entity_addVel(n, vx, vy)
				vx, vy = vector_setLength(vx, vy, 2500)
				entity_clearVel2(n)
				entity_addVel2(n, vx, vy)
				entity_setPosition(n, entity_x(me)+dx, entity_y(me)+dy)
			else
				vx, vy = entity_getVectorToEntity(me, n)
				dx, dy = vector_setLength(vx, vy, entity_getCollideRadius(me)+entity_getCollideRadius(n)+1)
				vx, vy = vector_setLength(vx, vy, 2000)
				--entity_push(n, vx, vy, 0.5, 1000, 0)				
				entity_addVel(n, vx, vy)
				entity_clearVel2(n)
				vx, vy = vector_setLength(vx, vy, 800)
				entity_addVel2(n, vx, vy)
				entity_setPosition(n, entity_x(me)+dx, entity_y(me)+dy)				
			end
		end
	end
	if bounceGroupSize ~= 1 then
		for i=1,bounceGroupSize-1 do
			--debugLog(string.format("i: %d, bgrp: %d", i, bounceGroupSize))
			e = bounceGroup[i]
			--debugLog(string.format("i: %d e: %d", i, ent))
			if e ~= 0 then
				if entity_isEntityInRange(me, e, entity_getCollideRadius(me)+entity_getCollideRadius(e)) then
					vx, vy = entity_getVectorToEntity(me, e)
					vx, vy = vector_setLength(vx, vy, 1200)
					entity_addVel(e, vx, vy)
					vx, vy = vector_setLength(vx, vy, 800)
					entity_addVel2(e, vx, vy)			
					dx, dy = vector_setLength(vx, vy, entity_getCollideRadius(me)+entity_getCollideRadius(n)+1)
					entity_setPosition(e, entity_x(me)+dx, entity_y(me)+dy)
					bounceReact(me)
				end
			end
		end
	end
	--[[
	iter = 0
	e = getFirstEntity()
	while e ~= 0 do
		if e ~= me and entity_getEntityType(e) == ET_ENEMY and entity_isEntityInRange(me, e, entity_getCollideRadius(me)+entity_getCollideRadius(e)) then
			vx, vy = entity_getVectorToEntity(me, e)
			vx, vy = vector_setLength(vx, vy, 1200)
			entity_addVel(e, vx, vy)
			vx, vy = vector_setLength(vx, vy, 800)
			entity_addVel2(e, vx, vy)			
			dx, dy = vector_setLength(vx, vy, entity_getCollideRadius(me)+entity_getCollideRadius(n)+1)
			entity_setPosition(e, entity_x(me)+dx, entity_y(me)+dy)
			bounceReact(me)
			break
		end
		e = getNextEntity()
	end
	]]--
	
	entity_handleShotCollisions(me)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	end
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	return true
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

