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
-- S T A R M I E   C O M M O N   S C R I P T
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- L O C A L   V A R I A B L E S 
-- ================================================================================================

rotDir = math.random(2)-1	-- Random direction for spinning 'round Naija

shotDelay = 0
sD = 6	-- Time between shots

animSpeed = 1
openDelay = 0

maxSpeed = 700
shotForce = 432	-- For pushing Starmie around

pYo = 2			-- Pupil y offset
pupilFreeze = 0
blinkTime = 0
 
-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function commonInit(me, skin)
	setupBasicEntity(
	me,
	"Starmie/Body",					-- texture
	15,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	64,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	2200							-- updateCull -1: disabled, default: 4000
	)
	
	loadSound("StarmieAwake")
	
	entity_setEatType(me, EAT_FILE, "Starmie")
	
	entity_setDeathParticleEffect(me, "StarmieDeath")
	entity_setDropChance(me, 6)
	
	if skin == 1 then 
		entity_initSkeletal(me, "Starmie")
	elseif skin == 2 then
		entity_initSkeletal(me, "Starmie", "Starmie2")
	end
	
	pupil = entity_getBoneByName(me, "Pupil")
	lid = entity_getBoneByName(me, "Lid")
	eye = entity_getBoneByName(me, "Eye")
	
	entity_setState(me, STATE_IDLE)
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
end

function dieNormal(me)
	if chance(5) then
		spawnIngredient("SmallEye", entity_x(me), entity_y(me))
	end
end

function update(me, dt)

	-- WAKE UP STARMIE WHEN THE TIME IS RIGHT
	if entity_getState(me)==STATE_IDLE
	and not isForm(FORM_FISH) then	
		entity_findTarget(me, 321)
		
		if not entity_hasTarget(me) then
			bone_setPosition(pupil, 0, pYo)
		else
			entity_setState(me, STATE_OPEN)
		end
	elseif entity_getState(me)==STATE_OPEN then
		if openDelay > 0 then openDelay = openDelay - dt
		elseif openDelay <= 0 then
			entity_setState(me, STATE_HOSTILE)
		end
	end
	
	-- STARMIE ON THE ATTACK
	if entity_getState(me)==STATE_HOSTILE then
		entity_findTarget(me, 2000)
		
		-- TIME BETWEEN SHOTS
		if shotDelay > 0 then shotDelay = shotDelay - dt
		else shotDelay = 0 end
		-- PUPIL FREEZE COUNTDOWN
		if pupilFreeze > 0 then pupilFreeze = pupilFreeze - dt
		else pupilFreeze = 0 end
		
		-- DO BLINKING
		if blinkTime > 0 and pupilFreeze == 0 then blinkTime = blinkTime - dt
		elseif blinkTime <= 0 and blinkTime > -0.18 then
			bone_alpha(lid, 1)
			blinkTime = blinkTime - dt
		else
			bone_alpha(lid, 0, 0.024)
			blinkTime = 5 + (math.random(600) * 0.01)
		end

		if not entity_hasTarget(me) then
			-- RETURN TO "HIDING"
			entity_clearVel(me)
			entity_setState(me, STATE_IDLE)
			bone_setPosition(pupil, 0, pYo)
			entity_rotate(me, randAngle360())
		else
			if pupilFreeze == 0 and blinkTime > 0 then
				-- EYE TRACKING
				nX, nY = entity_getPosition(getNaija())	-- Naija's position
				sX, sY = entity_getPosition(me)			-- Starmie's position
				x = (nX - sX)
				y = (nY - (sY+pYo))
				x, y = vector_cap(x, y, 7.5)
				bone_setPosition(pupil, x, y, 0.24)
				
				-- ATTACK
				if shotDelay == 0 and entity_hasTarget(me) then entity_setState(me, STATE_ATTACK) end
			else
				if shotDelay == 0 then shotDelay = shotDelay + 0.34 end	-- Helps keep Starmie stunned when being hit
			end

			-- MOVEMENT
			entity_moveAround(me, nX, nY, dt, 255, rotDir)
			entity_moveTowardsTarget(me, dt, 186)
			if not entity_isTargetInRange(me, 1248) then entity_moveTowardsTarget(me, dt, shotForce) end -- Move in if far away
		end
	end
	
	if entity_getState(me)==STATE_ATTACK then
		-- BOUNCE STARMIE AFTER SHOOTING
		entity_moveTowardsTarget(me, 1, -(shotForce * 0.9))
		entity_setState(me, STATE_HOSTILE)
	end

	-- SPEED UP/SLOW DOWN ROTATION BASED ON ACTUAL SPEED
	animSpeed = ((entity_getVelLen(me) / maxSpeed) * 2) + 0.2
	entity_setAnimLayerTimeMult(me, 0, animSpeed)
	
	entity_doEntityAvoidance(me, dt, 123, 0.32)
	entity_doCollisionAvoidance(me, dt, 8, 0.6)
	
	-- UPDATE ERRVRYTHING
	
	if not entity_isState(me, STATE_IDLE) then
		entity_updateCurrents(me, dt)
		entity_doFriction(me, dt, 200)
		entity_updateMovement(me, dt)
		
		entity_touchAvatarDamage(me, 32, 0.25, 640)
	end
	
	entity_handleShotCollisions(me)
end

function enterState(me)
	appearSpeed = 0.30
	lookSpeed = 0.2

	-- HIDE STARMIE IN THE BACKGROUND...
	if entity_getState(me)==STATE_IDLE then
		bone_setPosition(pupil, 0, pYo)
		bone_alpha(lid, 1)
		entity_scale(me, 0.7, 0.7)
		entity_color(me, 0.6, 0.6, 0.6)
	
		entity_animate(me, "idle", LOOP_INF)
		animSpeed = 0
		
		shotDelay = 1 + (math.random(50) * 0.1)
		entity_setMaxSpeed(me, maxSpeed/8)
		
		blinkTime = 5 + (math.random(600) * 0.01)
	
	-- BRING STARMIE TO LIFE!
	elseif entity_getState(me)==STATE_OPEN then
		entity_sound(me, "StarmieAwake")
	
		-- SPIN IN THE PROPER DIRECTION, BASED ON HOW STARMIE IS ROTATING AROUND NAIJA
		if rotDir == 0 then entity_animate(me, "spinLeft", LOOP_INF)
		elseif rotDir == 1 then entity_animate(me, "spinRight", LOOP_INF) end
		
		animSpeed = 1
		
		bone_setPosition(pupil, 0, pYo)
		bone_alpha(lid, 0, appearSpeed) --fade away the eyelid
		entity_scale(me, 1.2, 1.2, appearSpeed) --scale to normal size
		entity_color(me, 1, 1, 1, appearSpeed)	--set to normal colour
		bone_scale(pupil, 1.27, 1.27)
		bone_scale(pupil, 1, 1, appearSpeed) -- Pupil adjusting to light -> may have to tweak timing to get it lookin' nice
		
		entity_rotate(me, 0, lookSpeed)
		openDelay = appearSpeed + lookSpeed
		
		entity_setMaxSpeed(me, maxSpeed/4)
		entity_moveTowards(me, entity_x(getNaija()), entity_y(getNaija()), 1, -1234)
		
		
		entity_setDamageTarget(me, DT_AVATAR_PET, true)
		entity_setDamageTarget(me, DT_AVATAR_LIZAP, true)
		
	elseif entity_getState(me)==STATE_HOSTILE then
		openDelay = 0
		pupilFreeze = 0
		entity_setMaxSpeed(me, maxSpeed)
	
	-- SHOT WEB, LOL
	elseif entity_getState(me)==STATE_ATTACK then
		pupx, pupy = bone_getWorldPosition(pupil)
		spawnParticleEffect("StarShot", pupx, pupy)
		s = createShot("StarFire", me, entity_getTarget(me))
		shot_setOut(s, 12)	
		
		bone_setColor(pupil, 2, 2, 0)
		bone_setColor(pupil, 1, 1, 1, 0.15)
		bone_alpha(pupil, 0.45)
		bone_alpha(pupil, 1, 0.5)
		bone_scale(pupil, 1.27, 1.27)
		bone_scale(pupil, 1, 1, 0.32)
		
		bone_setColor(eye, 1, 1, 0)
		bone_setColor(eye, 1, 1, 1, 0.04)
		
		shotDelay = sD
	end
end

-- TAKE DAMAGE -> STUN STARMIE WHEN HIT
function damage(me, attacker, bone, damageType, dmg, x, y)
	bone_setPosition(pupil, 0, pYo, 0.021)
	bone_scale(pupil, 0.76, 0.76)
	bone_scale(pupil, 1, 1, 0.1)
	pupilFreeze = 0.32
	
	entity_moveTowards(me, x, y, 1, -shotForce)

	if entity_getState(me)==STATE_IDLE then	
		entity_setState(me, STATE_OPEN)
		pupilFreeze = 0
	end
	
	return true
end

function exitState(me)
	if entity_isState(me, STATE_OPEN) then
	end
end

function songNoteDone(me, note)
end

function hitSurface(me)
end
