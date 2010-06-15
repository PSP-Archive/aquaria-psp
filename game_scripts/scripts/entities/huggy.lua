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
-- H U G G Y
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")
-- entity specific
STATE_ATTACHED			= 1000
STATE_FLYOFF			= 1001

-- ================================================================================================
-- L O C A L   V A R I A B L E S 
-- ================================================================================================

eyes = 0
color = math.random(8)-1
noteDown = -1
angle = randAngle360()

swimTime = 0.60 + math.random(40)/100
swimTimer = swimTime * 0.76

attachBone = 0
rollTime = 0
maxRollTime = math.random(12)/30.0 + 0.45

healDelay = 0

-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function init(me)
	setupBasicEntity(
	me,
	"Huggy/Head",					-- texture
	2,								-- health
	2,								-- manaballamount
	1,								-- exp
	0,								-- money
	16,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	64,								-- sprite width
	64,								-- sprite height
	0,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	1500,							-- updateCull -1: disabled, default: 4000
	1
	)
	
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	entity_setDropChance(me, 21)
	
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)	-- Stop Li from shooting at huggys!
	
	entity_initSkeletal(me, "Huggy")
	eyes = entity_getBoneByName(me, "Eyes")
	
	size = 0.77 + (math.random(15)*0.01)
	entity_scale(me, size, size)
	
	entity_initHair(me, 32*size, 2, 40*size, "Huggy/Tail2")
	entity_exertHairForce(me, 0, 400, 1)
	
	r,g,b = getNoteColor(color)
	bone_setColor(eyes, r, g, b)
	
	entity_setState(me, STATE_IDLE)
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function update(me, dt)
	boost = 315

	if entity_getState(me)==STATE_IDLE then
		if not entity_hasTarget(me) then
		
			if noteDown == color then 
				entity_findTarget(me, 550)
			end
			
			swimTimer = swimTimer + dt
			
			if swimTimer > swimTime then
				swimTimer = swimTimer - swimTime
				
				angle = angle + math.random(180) - 90
				if angle > 355 then
					angle = angle - 355
				elseif angle < 0 then
					angle = angle + 355
				end
				
				entity_moveTowardsAngle(me, angle, 1, boost)
				
				entity_doEntityAvoidance(me, dt, 64, 0.2)
				entity_rotateToVel(me, 0.4)
				
				lastBoostX = boostX
				lastBoostY = boostY
			end
			entity_doCollisionAvoidance(me, dt, 6, 0.5)
		else
			swimTimer = swimTimer + dt
			if swimTimer > swimTime then			
				entity_moveTowardsTarget(me, 1, boost*4)
					
				entity_doEntityAvoidance(me, 1, 64, 0.1)
				swimTimer = swimTimer - swimTime
			else
				entity_moveTowardsTarget(me, dt, boost/3)
				entity_doEntityAvoidance(me, dt, 64, 0.1)
		
				entity_doCollisionAvoidance(me, dt, 4, 0.2)
			end
				
			if entity_isTargetInRange(me, 32) then
				entity_setState(me, STATE_ATTACHED)
			elseif entity_isTargetInRange(me, 64) then
				entity_moveTowardsTarget(me, 1, boost)
			end
			
			entity_rotateToVel(me, 0.2)
			
			entity_findTarget(me, 800)
			
		end
		
	elseif entity_getState(me)==STATE_ATTACHED then
	
		healDelay = healDelay - dt
		if healDelay < 0 then
			healDelay = 0.5
			spawnParticleEffect("HuggyHeal", entity_x(me), entity_y(me))
			entity_heal(getNaija(), 1*dt)
		end
		
		entity_exertHairForce(me, 0, 4, 1)				--tail gravity
	
		t = entity_getTarget(me)
		entity_rotate(me, bone_getWorldRotation(attachBone))
		entity_setPosition(me, bone_getWorldPosition(attachBone))
		
		if avatar_isRolling() then
			rollTime = rollTime + dt
			if rollTime > maxRollTime then			
				entity_setState(me, STATE_FLYOFF, 0.5)
			end
		end
		if isForm(FORM_FISH) then
			entity_setState(me, STATE_FLYOFF, 0.5)
		end
	end		
	
	if not entity_isState(me, STATE_ATTACHED) then
		entity_handleShotCollisions(me)
		entity_doFriction(me, dt, 323)
		entity_updateCurrents(me, dt)
		entity_updateMovement(me, dt)
		entity_rotateToVel(me, 0.2)
		angle = entity_getRotation(me)
	end
	
	entity_setHairHeadPosition(me, entity_x(me), entity_y(me))
	entity_updateHair(me, dt)
end

function songNote(me, note)
	if getForm()~=FORM_NORMAL then
		return
	end
	
	noteDown = note
end

function songNoteDone(me, note)
	noteDown = -1
end

function enterState(me)
	if entity_getState(me)==STATE_IDLE then
		entity_setEntityType(me, ET_ENEMY)
		esetv(me, EV_LOOKAT,1)
		entity_animate(me, "idle", LOOP_INF)
		entity_setMaxSpeed(me, 400)
		
	elseif entity_getState(me)==STATE_ATTACHED then
		entity_setEntityType(me, ET_NEUTRAL)
		esetv(me, EV_LOOKAT,0)
		entity_setMaxSpeed(me, 0)
		entity_animate(me, "attached", LOOP_INF)
		entity_sound(me, "Leach")
		attachBone = entity_getNearestBoneToPosition(entity_getTarget(me), entity_getPosition(me))
		
	elseif entity_isState(me, STATE_FLYOFF) then
		entity_setEntityType(me, ET_ENEMY)
		esetv(me, EV_LOOKAT,1)
		rollTime = 0
		entity_setMaxSpeed(me, 840)
		entity_addRandomVel(me, 1200)
		entity_setTarget(me, 0)
	end
end

function exitState(me)
	if entity_getState(me)==STATE_ATTACHED then
	
	elseif entity_isState(me, STATE_FLYOFF) then
		entity_setState(me, STATE_IDLE)
	end
end

function hitSurface(me)
end

function damage(me, attacker, bone, damageType, dmg)
	if damageType == DT_AVATAR_BITE then
		entity_setHealth(me, 0)
	end
	return true
end