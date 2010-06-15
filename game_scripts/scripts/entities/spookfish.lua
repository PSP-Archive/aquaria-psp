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
-- S P O O K F I S H   (pre-alpha)
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")


-- ================================================================================================
-- L O C A L   V A R I A B L E S 
-- ================================================================================================

segsOn = true

angle = 0

swimTimer = 0.64 + (math.random(64) * 0.01)

boostTimer = 4.56 + (math.random(640) * 0.01)
boostLen = 0
boostDir = 0

-- ================================================================================================
-- M Y   F U N C T I O N S
-- ================================================================================================

function setSpookSegsOn(me)
	bone_setSegs(body, 8, 2, 0.12, 0.42, 0, -0.03, 8, 0)
	bone_setSegs(glow01, 8, 2, 0.12, 0.42, 0, -0.03, 8, 0)
	bone_setSegs(glow02, 8, 2, 0.12, 0.42, 0, -0.03, 8, 0)
end

function setSpookSegsOff(me)
	bone_setSegs(body, 8, 2, 0.23, 0.69, 0, -0.03, 8, 0)
	bone_setSegs(glow01, 8, 2, 0.23, 0.69, 0, -0.03, 8, 0)
	bone_setSegs(glow02, 8, 2, 0.23, 0.69, 0, -0.03, 8, 0)
	--bone_setSegs(body)
	--bone_setSegs(glow01)
	--bone_setSegs(glow02)
end

-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function init(me)
	setupBasicEntity(me, 
	"Spookfish/Body",				-- texture
	2,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	64,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	1,								-- 0/1 hit other entities off/on (uses collideRadius)	
	4000,							-- updateCull -1: disabled, default: 4000
	1
	)
	
	entity_setAllDamageTargets(me, false)
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	
	entity_initSkeletal(me, "Spookfish")
	body = entity_getBoneByName(me, "Body")
	glow01 = entity_getBoneByName(me, "Glow01")
	glow02 = entity_getBoneByName(me, "Glow02")
end

function postInit(me)
	angle = randAngle360()
	entity_rotateTo(me, angle)
	entity_moveTowardsAngle(me, angle, 1, 101)
	entity_setState(me, STATE_IDLE)
	
	bone_scale(glow02, 1.31, 2.5, 3.4, -1, 1, 1)
	bone_alpha(glow01, 0.12, 1.23, -1, 1, 1)
	bone_alpha(body, 0.23, 4.2, -1, 1, 1)

	setSpookSegsOn(me)
	segsOn = true
end

function update(me, dt)
	
	angle = entity_getRotation(me)

	if entity_getState(me) == STATE_IDLE then
		-- BOOST FORWARD RANDOMLY
		boostTimer = boostTimer - dt
		if boostTimer <= 0 then
			boostTimer = 4.56 + (math.random(640) * 0.01)
			
			angle = angle + math.random(46) - 23
			if angle > 360 then
				angle = angle - 360
			elseif angle < 0 then
				angle = angle + 360
			end
			
			if segsOn == true then 
				setSpookSegsOff(me)
				segsOn = false
			end
			boostLen = 1.4
			if chance(50) then boostDir = -1
			else boostDir = 1 end
			entity_setMaxSpeedLerp(me, 4.2)
			entity_setMaxSpeedLerp(me, 1, boostLen)
			entity_moveTowardsAngle(me, angle, 1, 1234)
		end
		
		-- Curve with boost
		boostLen = boostLen - dt
		if boostLen <= 0 then
			boostLen = 0
			if segsOn == false then 
				setSpookSegsOn(me)
				segsOn = true
			end
		else
			angle = angle + (40 * boostDir)
			if angle > 360 then
				angle = angle - 360
			elseif angle < 0 then
				angle = angle + 360
			end
			entity_rotateTo(me, angle, 0.1)
			entity_moveTowardsAngle(me, angle, 1, 98)
		end
		
		-- CHANGE SWIM DIRECTION SLIGHTLY
		swimTimer = swimTimer - dt
		if swimTimer <= 0 then
			swimTimer = 0.64 + math.random(64)/100
				
			angle = angle + math.random(90) - 45
			if angle > 360 then
				angle = angle - 360
			elseif angle < 0 then
				angle = angle + 360
			end
					
			entity_moveTowardsAngle(me, angle, 1, 87)
			entity_doEntityAvoidance(me, dt, 64, 0.21)
		end
		
		entity_moveTowardsAngle(me, angle, dt, 124)
		entity_doEntityAvoidance(me, dt, 128, 0.08)
		entity_doCollisionAvoidance(me, dt, 12, 0.36)
	end
	
	-- FLIP
	flipThresh = 32
	if entity_isfh(me) and entity_velx(me) < -flipThresh then 
		entity_fh(me)
	elseif not entity_isfh(me) and entity_velx(me) > flipThresh then
		entity_fh(me)
	end
	
	entity_handleShotCollisions(me)
	
	entity_doFriction(me, dt, 123)
	entity_updateCurrents(me, dt)
	entity_rotateToVel(me, 0.8)
	entity_updateMovement(me, dt)
	entity_touchAvatarDamage(me, 64, 0, 321)
end

function enterState(me)
	if entity_getState(me) == STATE_IDLE then
		entity_animate(me, "idle", LOOP_INF)
		entity_setMaxSpeed(me, 123)
	end
end

function exitState(me)
end

function hitSurface(me)
	boostLen = 0
	entity_doCollisionAvoidance(me, 1, 4, 2.34)
end
