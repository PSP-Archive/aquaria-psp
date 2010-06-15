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
-- M U T I L E Y E   (alpha)
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")


-- ================================================================================================
-- L O C A L   V A R I A B L E S 
-- ================================================================================================

orient = ORIENT_LEFT
orientTimer = 0

swimTimer = 0.28 + (math.random(34) * 0.01)

node_mist = 0
eMate = 0
matingTimer = 0
mateCheckDelay = 4
 
-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function init(me)
	setupBasicEntity(me, 
	"Mutileye/Head",				-- texture
	4 + math.random(4),				-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	28,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	1,								-- 0/1 hit other entities off/on (uses collideRadius)	
	4000,							-- updateCull -1: disabled, default: 4000
	1
	)
	
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	
	entity_initSkeletal(me, "Mutileye")
	head = entity_getBoneByName(me, "Head")
	tail01 = entity_getBoneByName(me, "Tail01")
	tail02 = entity_getBoneByName(me, "Tail02")
	tail03 = entity_getBoneByName(me, "Tail03")
	tail04 = entity_getBoneByName(me, "Tail04")
	
	-- SEGMENT TAIL
	bone_setSegmentChainHead(head, true)
	bone_setSegmentProps(head, 30, 30, true)
	bone_addSegment(head, tail04)
	bone_rotateOffset(tail04, -180)
	bone_addSegment(head, tail03)
	bone_rotateOffset(tail03, -180)
	bone_addSegment(head, tail02)
	bone_rotateOffset(tail02, -180)
	bone_addSegment(head, tail01)
	bone_rotateOffset(tail01, -180)
	
	--entity_scale(me, 0.42 + (math.random(21) * 0.01), 0.42 + (math.random(21) * 0.01))  -- Segment tail doesn't scale yet
	entity_scale(me, 0.8, 0.8)
	
	entity_setMaxSpeed(me, 567)
	
	entity_setEatType(me, EAT_FILE, "SmallFood")
end

function postInit(me)
	entity_setState(me, STATE_IDLE)
end

function update(me, dt)
	amt = 321 + math.random(234)
	
	if not entity_hasTarget(me) then
		entity_findTarget(me, 690)
			
		swimTimer = swimTimer - dt
		if swimTimer <= 0 then	
			swimTimer = 0.64
				
			if orient == ORIENT_LEFT then
				entity_addVel(me, -amt, 0)
				orient = ORIENT_UP
			elseif orient == ORIENT_UP then
				entity_addVel(me, 0, -amt)
				orient = ORIENT_RIGHT
			elseif orient == ORIENT_RIGHT then
				entity_addVel(me, amt, 0)
				orient = ORIENT_DOWN
			elseif orient == ORIENT_DOWN then
				entity_addVel(me, 0, amt)
				orient = ORIENT_LEFT
			end			
			entity_rotateToVel(me, 0.2)
			orientTimer = orientTimer + dt
			entity_doEntityAvoidance(me, 1, 256, 0.2)
		end
		entity_doCollisionAvoidance(me, dt, 6, 0.5)
	else
			
		swimTimer = swimTimer - dt
		if swimTimer <= 0 then			
			swimTimer = 0.28 + (math.random(54) * 0.01)
			
			entity_moveTowardsTarget(me, 1, amt)
			if not entity_isNearObstruction(getNaija(), 8) then
				entity_doCollisionAvoidance(me, 1, 6, 0.5)
			end
			entity_doEntityAvoidance(me, 1, 256, 0.2)
			entity_rotateToVel(me, 0.2)
				
		else
			entity_moveTowardsTarget(me, dt, 123)
			entity_doEntityAvoidance(me, dt, 64, 0.12)
			entity_doCollisionAvoidance(me, dt, 8, 0.48)
		end
		
		entity_findTarget(me, 1111)
	end

	entity_doFriction(me, dt, 600)
	entity_updateCurrents(me, dt)
	entity_updateMovement(me, dt)
	entity_handleShotCollisions(me)
	entity_touchAvatarDamage(me, 23, 1, 1234)
end

function enterState(me)
	if entity_getState(me) == STATE_IDLE then
		entity_animate(me, "idle", LOOP_INF)
		mateCheckDelay = 3
		matingTimer = 0
		entity_setMaxSpeed(me, 400)
	end
end

function exitState(me)
end

function hitSurface(me)
	orient = orient + 1
end

function dieNormal(me)
end


