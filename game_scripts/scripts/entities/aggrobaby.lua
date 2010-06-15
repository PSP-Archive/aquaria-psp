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

-- ================================================================================================
-- AGGRO BABY
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")
-- specific
STATE_JUMP				= 1000
STATE_TRANSITION		= 1001
STATE_JUMPPREP			= 1002

-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

birthed = 1

jumpDelay = 0
moveTimer = 0
rotateOffset = 0
angry = false
enraged = false

out = 32

n = 0

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function land(me)
	entity_clampToSurface(me)
	entity_moveAlongSurface(me, dt, 1, 6, out)
	entity_rotateToSurfaceNormal(me, 0.1)
end

function init(me)

	setupBasicEntity(
	me,
	"aggrobaby",								-- texture
	1,								-- health
	2,								-- manaballamount
	2,								-- exp
	10,								-- money
	16,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000							-- updateCull -1: disabled, default: 4000
	)
	--entity_initSkeletal(me, "AggroHopper")
	
	esetv(me, EV_WALLOUT, out)
	
	entity_setDeathParticleEffect(me, "tinyredexplode")
	
	entity_scale(me, 0.5, 0.5)
	land(me)
	entity_setWeight(me, 1000)
	entity_setState(me, STATE_IDLE)
	--entity_setBounce(0)
	
	loadSound("aggrobaby-jump")
	
end

function postInit(me)
	n = getNaija()
end

function update(me, dt)
	dt = dt * 0.6
	if enraged then
		dt = dt * 1.25
	end
	entity_handleShotCollisions(me)
	if entity_hasTarget(me) then
		if entity_isTargetInRange(me, 64) then
			entity_hurtTarget(me, 0.5)
			entity_pushTarget(me, 500)
		end
	end
	if entity_getState(me)==STATE_IDLE then
		entity_rotateToSurfaceNormal(me, 0.1)
		if not(entity_hasTarget(me)) then
			entity_findTarget(me, 1200)
		else
			if not angry then
				if entity_isTargetInRange(me, 400) then
					jumpDelay = jumpDelay - dt
					if jumpDelay < 0 then
						angry = true
						jumpDelay = 1.5
						entity_setState(me, STATE_JUMPPREP)
					end
				end
			else
				if birthed == 1 or entity_isTargetInRange(me, 1800) then
					jumpDelay = jumpDelay - dt
					if jumpDelay < 0 then
						birthed = 0
						angry = true
						jumpDelay = 1.5
						entity_setState(me, STATE_JUMPPREP)
					end
				end
			end
		end
	elseif entity_getState(me)==STATE_JUMPPREP then
		if not entity_isAnimating(me) then
			entity_setState(me, STATE_JUMP)
		end
	elseif entity_getState(me)==STATE_JUMP then
	--[[
		rotateOffset = rotateOffset + dt * 400
		if rotateOffset > 180 then
			rotateOffset = 180
		end
		entity_rotateToVel(me, 0.1, rotateOffset)
		
		]]--
		entity_updateMovement(me, dt*1.5)
--		entity_applySurfaceNormalForce(1000)
		
	elseif not(entity_getState(me)==STATE_TRANSITION) then
		entity_updateMovement(me, dt)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_getHealth(me) < 6 and not enraged then
		debugLog("ENRAGED!!!!!!!!!")
		enraged = true
		entity_setColor(me, 1, 0.5, 0.5, 1)
	end
	if entity_isState(me, STATE_IDLE) then
		entity_setState(me, STATE_JUMPPREP)
	end
	return true
end

bounces = 0
function hitSurface(me)
	cx, cy = getLastCollidePosition()
	spawnParticleEffect("HitSurface", cx, cy)
	if entity_getState(me)==STATE_JUMP then
		--nx, ny = getWallNormal(cx, cy)
		--if ny < -0.8 and entity_isNearObstruction(me, 3, OBSCHECK_4DIR) then
		if entity_checkSurface(me, 6, STATE_IDLE, -1) then
		end
		--[[
		if entity_isNearObstruction(me, 4, OBSCHECK_DOWN) then
			land(me)
			entity_setState(me, STATE_TRANSITION)
		else
			bounces = bounces + 1
			if bounces > 100 then
				land(me)
				entity_setState(me, STATE_TRANSITION)
			end
		end
		]]--
	end
end

function enterState(me)
	if entity_getState(me)==STATE_IDLE then
		entity_animate(me, "idle", LOOP_INF)
		entity_setMaxSpeed(me, 1000)
		if enraged then
			entity_setMaxSpeed(me, 1200)
		end
	elseif entity_getState(me)==STATE_JUMPPREP then
		entity_animate(me, "jump")
	elseif entity_isState(me, STATE_TRANSITION) then
		entity_setStateTime(me, 0.1)
	elseif entity_getState(me)==STATE_JUMP then
		entity_sound(me, "aggrobaby-jump")
		
		entity_rotate(me, 0, 0.5)
		entity_animate(me, "jumping")
		rotateOffset = 0
		--entity_applySurfaceNormalForce(me, 800)
		force = 2000
		--[[
		if entity_x(getNaija()) < entity_x(me) then
			entity_addVel(me, -force, -force*0.75)
		else
			entity_addVel(me, force, -force*0.75)
		end
		]]--
		x,y = entity_getNormal(me)
		x,y = vector_setLength(x, y, force)
		dx = entity_x(n) - entity_x(me)
		dy = entity_y(n) - entity_y(me)
		dx,dy = vector_setLength(dx, dy, force)
		x = dx*0.5 + x*0.5
		y = dy*0.5 + y*0.5
		entity_addVel(me, x, y)
		entity_adjustPositionBySurfaceNormal(me, 64)
	end
end

function exitState(me)
	if entity_getState(me)==STATE_TRANSITION then
		entity_setState(me, STATE_IDLE)
	end
end
