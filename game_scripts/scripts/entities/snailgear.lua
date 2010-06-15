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

-- SNAIL GEAR

dofile("scripts/entities/entityinclude.lua")
-- specific
STATE_JUMP				= 1000
STATE_TRANSITION		= 1001

jumpDelay = 0
moveTimer = 0
rotateOffset = 0

function init(me)
	setupBasicEntity(
	me,
	"",								-- texture
	8,								-- health
	2,								-- manaballamount
	2,								-- exp
	10,								-- money
	40,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	2000							-- updateCull -1: disabled, default: 4000
	)
	
	entity_initSkeletal(me, "SnailGear")
	
	
	entity_clampToSurface(me)
	entity_setWeight(me, 300)
	
	entity_setDeathParticleEffect(me, "TinyRedExplode")
	entity_setState(me, STATE_IDLE)
	--entity_scale(me, -1, 1)
	entity_setBeautyFlip(me, false)
	entity_setCanLeaveWater(me, true)
	bone_setSegs(entity_getBoneByName(me, "Snail"), 2, 8, 0.6, 0.6, -0.028, 0, 6, 1)
	loadSound("snailgear-jump")
end

function update(me, dt)
	entity_checkSplash(me)	
	if entity_getState(me)==STATE_IDLE then
		entity_moveAlongSurface(me, dt, 300, 6, 24)
		entity_rotateToSurfaceNormal(me, 0.1)
		moveTimer = moveTimer + dt
		if moveTimer > 8 then
			entity_switchSurfaceDirection(me)
			moveTimer = 0
		end
		if not(entity_hasTarget(me)) then
			entity_findTarget(me, 1200)
		else
			if entity_isTargetInRange(me, 600) then
				jumpDelay = jumpDelay - dt
				if jumpDelay < 0 then
					jumpDelay = 3
					entity_setState(me, STATE_JUMP)
				end
			end
		end
	elseif entity_getState(me)==STATE_JUMP then
	--[[
		rotateOffset = rotateOffset + dt * 400
		if rotateOffset > 180 then
			rotateOffset = 180
		end
		]]--
		
		entity_rotateToVel(me, 0.1, rotateOffset)
		entity_updateMovement(me, dt)
		
		
		
	elseif not(entity_getState(me)==STATE_TRANSITION) then
		entity_updateMovement(me, dt)
	end
	entity_handleShotCollisions(me)
	entity_touchAvatarDamage(me, entity_getCollideRadius(me), 1, 500)
	
	if isObstructed(entity_x(me), entity_y(me)) then
		entity_adjustPositionBySurfaceNormal(me, 1)
	end
end

function hitSurface(me)
	if entity_getState(me)==STATE_JUMP then
		if entity_isNearObstruction(me, 4, OBSCHECK_4DIR) then
			entity_clampToSurface(me, 0.1)
			--entity_clampToHit(me)
			entity_setState(me, STATE_TRANSITION, 0.1)
		end
	end
end

function enterState(me)
	if entity_getState(me)==STATE_IDLE then
		entity_setMaxSpeed(me, 800)
		entity_animate(me, "idle", -1)
	elseif entity_getState(me)==STATE_JUMP then
		spawnParticleEffect("WallBoost", entity_x(me), entity_y(me), 0, entity_getRotation(me)+90)
		rotateOffset = 0
		entity_applySurfaceNormalForce(me, 800)
		entity_adjustPositionBySurfaceNormal(me, 10)
		entity_animate(me, "jump")
		entity_sound(me, "snailgear-jump")
	elseif entity_isState(me, STATE_DEAD) then
		entity_sound(me, "MetalExplode", math.random(100)+750)
		spawnParticleEffect("SnailGearDie", entity_getPosition(me))		
	end
end

function exitState(me)
	if entity_getState(me)==STATE_TRANSITION then
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	--[[
	if entity_isState(me, STATE_IDLE) then
		entity_setState(me, STATE_JUMP)
	end
	]]--
	return true
end