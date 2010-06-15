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
attackDelay = 1
boneHead = 0
attackNum = 0

a1 = 0
a2 = 0
a3 = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "Moray")	
	--entity_setAllDamageTargets(me, false)
	entity_setHealth(me, 10)
	
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	
	entity_generateCollisionMask(me)	
	
	entity_setState(me, STATE_IDLE)
	
	entity_setCullRadius(me, 512)
	entity_setDeathScene(me, true)
	
	boneHead = entity_getBoneByName(me, "Head")
	
	
	a1 = entity_getBoneByName(me, "a1")
	a2 = entity_getBoneByName(me, "a2")
	a3 = entity_getBoneByName(me, "a3")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	
	node = entity_getNearestNode(me, "FLIP")
	if node_isEntityIn(node, me) then
		entity_fh(me)
	end
end

function update(me, dt)
	--entity_updateMovement(me, dt)
	entity_clearTargetPoints(me)
	
	if entity_isState(me, STATE_IDLE) then
		if entity_isEntityInRange(me, n, 512) then
			attackDelay = attackDelay - dt
			if attackDelay < 0 then
				entity_setState(me, STATE_ATTACK)
			end
		end
	end
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~= 0 then
		if entity_isState(me, STATE_ATTACK) then
			--entity_damage(n, me, 1)
			entity_touchAvatarDamage(me, 0, 1, 800)
		else
			entity_touchAvatarDamage(me, 0, 0.5, 800)
		end
	end
	
	entity_addTargetPoint(me, bone_getWorldPosition(boneHead))
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_DEATHSCENE) then
		entity_setStateTime(me, entity_animate(me, "death")+1)
		entity_setColor(me, 0.2, 0.2, 0.2, entity_getStateTime(me))
	elseif entity_isState(me, STATE_ATTACK) then
		--[[
		atkname = "attack1"
		if entity_y(n) < entity_y(me)-16 then
			atkname = "attack2"
		end
		]]--
		x=entity_x(n)
		y=entity_y(n)
		
		bx,by = bone_getWorldPosition(a1)
		d1 = vector_getLength(x-bx,y-by)
		
		bx,by = bone_getWorldPosition(a2)
		d2 = vector_getLength(x-bx,y-by)
		
		bx,by = bone_getWorldPosition(a3)
		d3 = vector_getLength(x-bx,y-by)
		
		--debugLog(string.format("d1: %d, d2: %d, d3: %d", d1, d2, d3))
		
		if d1 < d2 and d1 < d3 then
			attackNum = 1
		elseif d2 < d1 and d2 < d3 then
			attackNum = 2
		elseif d3 < d1 and d3 < d1 then
			attackNum = 3
		else
			attackNum = 1
		end
		
			
		entity_setStateTime(me, entity_animate(me, string.format("attack%d", attackNum)))
	end
end

function exitState(me)
	if entity_isState(me, STATE_ATTACK) then
		attackDelay = 0.5
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	attackDelay = attackDelay - dmg*2
	entity_setColor(me, 1, 0.5, 0.5)
	entity_setColor(me, 1, 1, 1, 1)
	return true
end

function animationKey(me, key)
	if entity_isState(me, STATE_ATTACK) then
		if key == 2 then
			entity_sound(me, "bite")
		end
	end
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

function dieNormal(me)
	if chance(75) then
		bx, by = bone_getWorldPosition(boneHead)
		if chance(50) then
			spawnIngredient("SmallEgg", bx , by)
		else
			spawnIngredient("EelOil", bx, by)
		end
	end
end

