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
jaw = 0
head = 0
trappedEnt = 0
getOutHits = 0
hx=0
hy=0
hurtTimer = 0

STATE_TRAP		= 1001
STATE_TRAPPED	= 1002
STATE_RELEASE	= 1003

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "PredatoryTunicate")
	entity_setEntityLayer(me)
	entity_setHealth(me, 24)
	
	entity_setAllDamageTargets(me, false)
	
	--entity_generateCollisionMask(me)
	
	entity_setCollideRadius(me, 100)
	entity_setCullRadius(me, 1024)
	--entity_generateCollisionMask(me)
	
	jaw = entity_getBoneByName(me, "Jaw")
	head = entity_getBoneByName(me, "Head")
	
	entity_setState(me, STATE_IDLE)	
	
	entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
	entity_setDamageTarget(me, DT_ENEMY_ENERGYBLAST, true)
	entity_setDamageTarget(me, DT_AVATAR_SHOCK, true)		
	--entity_setDamageTarget(me, DT_AVATAR_VINE, true)
	entity_setUpdateCull(me, 2000)
	
	esetv(me, EV_ENTITYDIED, 1)
end

function entityDied(me, ent)
	if trappedEnt == ent then
		trappedEnd = 0
		entity_setState(me, STATE_IDLE, -1, 1)
	end
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	
	node = entity_getNearestNode(me, "FLIP")
	if node ~= 0 and node_isEntityIn(node, me) then
		entity_fh(me)
	end
end

function update(me, dt)
	--entity_handleShotCollisionsSkeletal(me)
	

	hx, hy = bone_getWorldPosition(head)
	
	entity_clearTargetPoints(me)
	entity_addTargetPoint(me, hx, hy)
	
	entity_handleShotCollisions(me)
	
	if entity_isState(me, STATE_IDLE) then
		ent = getFirstEntity()
		while ent ~= 0 do
			if ent ~= me and entity_isDamageTarget(ent, DT_ENEMY_TRAP) and entity_isEntityInRange(me, ent, 256) then
				if not entity_isDead(ent) then
					if not eisv(ent, EV_TYPEID, EVT_PET) and
					not entity_isName(ent, "ubervine") and
					not (entity_getEntityType(ent)==ET_INGREDIENT) then
						trappedEnt = ent
						entity_setState(me, STATE_TRAP)
						break
					end
				end
			end
			ent = getNextEntity()
		end
	end
	if entity_isState(me, STATE_TRAPPED) then
		if trappedEnt ~= 0 and (entity_getEntityType(trappedEnt)==ET_AVATAR or trappedEnt == getLi()) then
			entity_setDamageTarget(me, DT_AVATAR_LIZAP, true)
		end
	else
		entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	end
	if not entity_isAnimating(me) and entity_isState(me, STATE_TRAP) then
		--[[

		ent = getFirstEntity()
		trappedEnt = 0
		while ent ~= 0 do
			if ent ~= me and entity_isEntityInRange(me, ent, 256) then
				trappedEnt = ent
				break
			end
			ent = getNextEntity()
		end
		]]--
		if trappedEnt ~= 0 then	
			if entity_getEntityType(trappedEnt)==ET_AVATAR or trappedEnt==getLi() then
				toggleLiCombat(true)
			end
			debugLog(string.format("trapped ent:%s", entity_getName(trappedEnt)))
			entity_setState(me, STATE_TRAPPED)
		else
			entity_setState(me, STATE_IDLE)
		end
	end
	--[[
	if not entity_isAnimating(me) and entity_isState(me, STATE_RELEASE) then
		entity_setState(me, STATE_IDLE)
	end
	]]--
	
	if entity_isState(me, STATE_TRAP) and trappedEnt~=0 then
		entity_setPosition(trappedEnt, hx, hy, 0.1)
		if entity_isDead(trappedEnt) then
			trappedEnt = 0
			entity_setState(me, STATE_IDLE)
		end		
	elseif entity_isState(me, STATE_TRAPPED) and trappedEnt~=0 then
		entity_setPosition(trappedEnt, hx, hy)
		hurtTimer = hurtTimer + dt
		if hurtTimer > 1 then
			if entity_getEntityType(trappedEnt) == ET_ENEMY then
				entity_damage(trappedEnt, me, 2, DT_ENEMY_TRAP)
			else
				entity_damage(trappedEnt, me, 1, DT_ENEMY_TRAP)
			end
			if entity_isDead(trappedEnt) then
				trappedEnt = 0
				entity_setState(me, STATE_IDLE)
			end
			hurtTimer = 0
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_TRAP) then
		entity_animate(me, "trap")
	elseif entity_isState(me, STATE_TRAPPED) then
		hurtTimer = 0
		entity_animate(me, "trapped", -1)
	elseif entity_isState(me, STATE_RELEASE) then
		entity_animate(me, "release", -1)
		entity_setStateTime(me, 5)
	end
end

function exitState(me)
	if entity_isState(me, STATE_TRAP) then
	elseif entity_isState(me, STATE_RELEASE) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_TRAPPED) then
		--entity_addVel(trappedEnt, -800, 0)
		if trappedEnt ~= 0 then
			entity_push(trappedEnt, -800, 0, 1)
		end
		trappedEnt = 0
		if trapDelay == 0 then
			trapDelay = 1.5
		end
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_TRAPPED) then
		
		getOutHits = getOutHits  + dmg
		if getOutHits > 5 then
			getOutHits = 0
			entity_setState(me, STATE_RELEASE)
		end
		debugLog(string.format("getOutHits: %d", getOutHits))		
		return true
	end
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

