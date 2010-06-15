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
-- AGGRO HOPPER
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")
-- specific
STATE_JUMP				= 1000
STATE_TRANSITION		= 1001
STATE_JUMPPREP			= 1002

-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

jumpDelay = 0+math.random(3)
moveTimer = 0
rotateOffset = 0
mouthOpen = 0
fireDelay = 2

STATE_FIREPREP = 1003
STATE_FIRE	 = 1004

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================


--[[
function land(me)
	entity_clampToSurface(me)	
	entity_rotateToSurfaceNormal(me, 0.1)
end
]]--

function init(me)

	setupBasicEntity(
	me,
	"",								-- texture
	8,								-- health
	2,								-- manaballamount
	2,								-- exp
	10,								-- money
	50,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	3000,							-- updateCull -1: disabled, default: 4000
	1
	)
	entity_initSkeletal(me, "Toad")
	
	entity_setDeathParticleEffect(me, "TinyBlueExplode")


	--land(me)
	entity_clampToSurface(me)
	entity_setState(me, STATE_IDLE)
	
	mouthOpen = entity_getBoneByName(me, "Head-MouthOpen")
	bone_alpha(mouthOpen, 0)
	
	entity_setInternalOffset(me, 0, -20)
	
	entity_setEatType(me, EAT_FILE, "BouncyBall")
	
	entity_setDropChance(me, 0, 1)
	esetv(me, EV_WALLOUT, 16)
	--entity_setBounce(0)
end

function update(me, dt)
	if entity_getState(me)==STATE_IDLE then	
		--entity_moveAlongSurface(me, dt, 0.5, 6, 16)
		--entity_switchSurfaceDirection(me)
		entity_rotateToSurfaceNormal(me, 0.1)
		if not(entity_hasTarget(me)) then
			entity_findTarget(me, 1200)
		else
			fireDelay = fireDelay - dt
			if fireDelay < 0 then
				fireDelay = math.random(2)+0.75
				entity_setState(me, STATE_FIREPREP)
			end
			if entity_isTargetInRange(me, 600) then
				jumpDelay = jumpDelay - dt
				if entity_getHealth(me) < 3 then
					jumpDelay = jumpDelay - dt*2
				end
				if jumpDelay < 0 then
					angry = true
					jumpDelay = 3+math.random(2)
					entity_setState(me, STATE_JUMPPREP)
				end
			end
		end		
	elseif entity_getState(me)==STATE_JUMPPREP then
		if not entity_isAnimating(me) then
			entity_setState(me, STATE_JUMP)
		end
	elseif entity_getState(me)==STATE_JUMP then
		entity_updateCurrents(me, dt)
		entity_updateMovement(me, dt*1.5)		
	elseif not(entity_getState(me)==STATE_TRANSITION) then
		--entity_updateMovement(me, dt)
	end
	entity_handleShotCollisions(me)
	if entity_hasTarget(me) then
		entity_touchAvatarDamage(me, entity_getCollideRadius(me), 1, 500)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_IDLE) and chance(20) then
		entity_setState(me, STATE_JUMPPREP)
	end
	if damageType == DT_AVATAR_BITE then
		entity_changeHealth(me, -999) -- -4
	end
	return true
end

--[[
function hitSurface(me)
	if entity_getState(me)==STATE_JUMP then
		entity_clampToSurface(me, 0.1) -- (0.1)
		entity_moveAlongSurface(me, 0, 1, 6, 24)
		entity_setState(me, STATE_IDLE)
	end
end
]]--

bounces = 0
function hitSurface(me)
	cx, cy = getLastCollidePosition()
	spawnParticleEffect("HitSurface", cx, cy)
	if entity_getState(me)==STATE_JUMP then		
		--nx, ny = getWallNormal(cx, cy)
		--if ny < 0 then
		if entity_checkSurface(me, 6, STATE_IDLE, -1) then
		end
		--[[
		if entity_isNearObstruction(me, 4, OBSCHECK_DOWN) then
			land(me)
			entity_setState(me, STATE_IDLE)
		else
			bounces = bounces + 1
			if bounces > 8 then
				land(me)
				entity_setState(me, STATE_IDLE)
			end
		end
		]]--
	end
end

weight = 600
function enterState(me)
	if entity_getState(me)==STATE_IDLE then
		entity_setWeight(me, 0)
		entity_animate(me, "idle", LOOP_INF)
		entity_setMaxSpeed(me, weight)
		entity_clearVel(me)
		--entity_adjustPositionBySurfaceNormal(me, 16)
	elseif entity_isState(me, STATE_TRANSITION) then
		entity_rotateToSurfaceNormal(me, 0.1)
		entity_setWeight(me, 0)
		entity_clearVel(me)
	elseif entity_getState(me)==STATE_JUMPPREP then
		entity_animate(me, "jumpPrep")
	elseif entity_getState(me)==STATE_JUMP then
		bounces = 0
		entity_setWeight(me, weight)
		entity_animate(me, "jump")
		rotateOffset = 0
		--entity_applySurfaceNormalForce(me, 800)
		force = 1200
		if entity_x(getNaija()) < entity_x(me) then
			entity_addVel(me, -force, -force*0.75)
		else
			entity_addVel(me, force, -force*0.75)
		end
		entity_adjustPositionBySurfaceNormal(me, 64)
		entity_rotate(me, 0, 1)
	elseif entity_isState(me, STATE_FIREPREP) then
		entity_animate(me, "idle", LOOP_INF)
		bone_alpha(mouthOpen, 1, 0.1)
		entity_setStateTime(me, 0.5)
	elseif entity_isState(me, STATE_FIRE) then
		spd = 700
		--homing = 100
		--s = entity_fireAtTarget(me, "", 1, spd, 0, 0, 32)
		s = createShot("BouncyBall", me, entity_getTarget(me))
		vx=0
		vy=0
		if chance(50) then
			vx, vy = entity_getAimVector(me, -20, 1)
		else
			vx, vy = entity_getAimVector(me, 20, 1)
		end
		shot_setAimVector(s, vx, vy)
		shot_setOut(s, 96)
		--[[
		shot_setNice(s, "Shots/BouncyBall", "BouncyBallTrail", "BouncyBallHit")
		shot_setBounceType(s, BOUNCE_REAL)
		shot_setVel(s, vx, vy)
		]]--

		entity_setStateTime(me, 0.5)
		bone_alpha(mouthOpen, 0, 0.1)
	end
end

function exitState(me)
	if entity_getState(me)==STATE_TRANSITION then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_FIREPREP) then
		entity_setState(me, STATE_FIRE)		
	elseif entity_isState(me, STATE_FIRE) then
		bone_alpha(mouthOpen, 0, 0.2)
		entity_setState(me, STATE_IDLE)
	end
end
