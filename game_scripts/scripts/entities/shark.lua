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

attackDelay = 0
dir = 1
n = getNaija()
jaw = 0

go = false

function init(me)
	setupBasicEntity(
	me,
	"",							-- texture
	30,							-- health
	2,							-- manaballamount
	2,							-- exp
	10,							-- money
	64,							-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,							-- particle "explosion" type, 0 = none
	0,							-- 0/1 hit other entities off/on (uses collideRadius)
	4000							-- updateCull -1: disabled, default: 4000
	)
	
	entity_initSkeletal(me, "Shark")	
	entity_generateCollisionMask(me)
	

	entity_setDeathParticleEffect(me, "Explode")
	
	entity_setState(me, STATE_IDLE)
	entity_setCullRadius(me, 1024)
	
	n = getNaija()
	jaw = entity_getBoneByName(me, "Jaw")
end

function update(me, dt)
	if entity_isState(me, STATE_ATTACK) and not entity_isAnimating(me) then
		entity_setState(me, STATE_IDLE)
	end
	
	if entity_isState(me, STATE_IDLE) then
		inRange = false
		x,y = bone_getWorldPosition(jaw)
		if entity_isPositionInRange(n, x, y, 550) then
			inRange = true
		end
		if dir < 0 then
			if entity_x(n) > entity_x(me) then
				inRange = false
			end
		else
			if entity_x(n) < entity_x(me) then
				inRange = false
			end
		end
		if inRange then
			attackDelay = attackDelay + dt
			if attackDelay > 1 then
				entity_setState(me, STATE_ATTACK)
				attackDelay = 0
			end
		end
	end
	
	entity_addVel(me, 500*dir, 0)
	
	if entity_isEntityInRange(me, n, 900) then
		entity_moveTowards(me, entity_x(n), entity_y(n), 1, 250)
	end
	
	if go then
		entity_moveTowardsTarget(me, 1, 1000)
	end
	
	entity_updateCurrents(me, dt)
	entity_updateMovement(me, dt)
	
	if not entity_isVelIn(me, 60) then
		entity_flipToVel(me)
	end
	
	if isObstructed(entity_x(me) + 300*dir, entity_y(me)) then
		dir = -dir
	end
	
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~= 0 then
		if avatar_isTouchHit() then
			if entity_isState(me, STATE_ATTACK) then
				entity_damage(n, me, 2)
			else
				entity_damage(n, me, 1)
			end
		end
	end
end

function hitSurface(me)
	dir = -dir
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", LOOP_INF)
	elseif entity_isState(me, STATE_ATTACK) then
		entity_animate(me, "attack")
	end
end

function animationKey(me, key)
	if entity_isState(me, STATE_ATTACK) then
		if key == 2 then
			entity_setMaxSpeedLerp(me, 2)
			entity_moveTowardsTarget(me, 1, 1000)
			go = true
		elseif key == 3 then
			playSfx("bite", 0.2)
		elseif key == 5 then
			entity_setMaxSpeedLerp(me, 1, 0.5)
			go = false
		end
		--entity_moveTowardsTarget(me, 1, 50000)
	end
end

function exitState(me)
end

function dieNormal(me)
	if chance(100) then
		spawnIngredient("SharkFin", entity_x(me), entity_y(me))
	end
end

function damage(me)
	if entity_x(n) > entity_x(me) then
		dir = 1
	else
		dir = -1
	end
	entity_moveTowardsTarget(me, 1, 1000)
	return true
end
