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
-- C R E A T O R ,   F O R M   4   (beta)
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- L O C A L   V A R I A B L E S
-- ================================================================================================

n = 0

bone_body = 0
bone_darkLi	= 0

hits1 = 4
hits2 = 2

lureNode = 1
lastNodeNum = 0

hideCount = hits1

maxSpeed = 505

chaseRange = 690
attackRange = 489
waitToAttack = 0
hitNaija = 0

-- ================================================================================================
-- S T A T E S
-- ================================================================================================

STATE_TRAP			= 1000
STATE_PAIN			= 1001
STATE_ATTACK		= 1002
STATE_LUREWAIT		= 1003
STATE_INTRO			= 1004
STATE_CHASE			= 1005
STATE_CREEP			= 1006

-- ================================================================================================
-- P H A S E S
-- ================================================================================================

phase = 0

PHASE_LURE			= 0
PHASE_HIDE			= 1
PHASE_FINAL			= 2

-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	
	entity_initSkeletal(me, "CreatorForm4")	
	bone_body = entity_getBoneByName(me, "Body")
	bone_darkLi = entity_getBoneByName(me, "DarkLi")
	bone_leftHand = entity_getBoneByName(me, "LeftHand")
	bone_rightHand = entity_getBoneByName(me, "RightHand")
	
	bone_leftLeg1 = entity_getBoneByName(me, "LeftLowerLeg1")
	bone_leftLeg2 = entity_getBoneByName(me, "LeftLowerLeg2")
	bone_rightLeg1 = entity_getBoneByName(me, "RightLowerLeg1")
	bone_rightLeg2 = entity_getBoneByName(me, "RightLowerLeg2")
	
	entity_generateCollisionMask(me)
	entity_setAllDamageTargets(me, true)
	
	entity_setBeautyFlip(me, false)
	esetv(me, EV_FLIPTOPATH, 0)
	
	entity_setCullRadius(me, 700)
	
	loadSound("CreatorForm4-Hit1")
	loadSound("CreatorForm4-Hit2")
	loadSound("CreatorForm4-Die")
	loadSound("creatorform4-bite")
	
	esetv(me, EV_MINIMAP, 1)
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function postInit(me)
	entity_setState(me, STATE_INTRO)
	
	n = getNaija()
	entity_setTarget(me, n)
	
	playSfx("CreatorForm4-Die")
	
	fadeOutMusic(6)
end

function update(me, dt)
	overrideZoom(0.60)
	entity_updateMovement(me, dt)
	
	if entity_isState(me, STATE_IDLE) then
		entity_setState(me, STATE_MOVE)
	end
	
	if entity_isState(me, STATE_MOVE) then
		entity_setAnimLayerTimeMult(me, 0, 1.89)
	end
	if entity_isState(me, STATE_MOVE) and not entity_isFollowingPath(me) then
		entity_setStateTime(me, 0.1)
	end
	
	if entity_isState(me, STATE_LUREWAIT) then
		entity_rotateToEntity(me, n, 0.1)
		if entity_isEntityInRange(me, n, 543) then
			entity_setState(me, STATE_MOVE)
		end
	end
	
	-- WAITING FOR NAIJA AT A NODE
	if entity_isState(me, STATE_TRAP) then
		-- FACE NAIJA
		if entity_isEntityInRange(me, n, 1234) then
			entity_rotateToEntity(me, n, 0.23)
		end
		
		-- ATTAAAACK
		if entity_isEntityInRange(me, n, 543) and waitToAttack == 1 then
			entity_setState(me, STATE_ATTACK)
		end
		if entity_isEntityInRange(me, n, attackRange) then
			entity_setState(me, STATE_ATTACK)
		-- CHASE
		elseif entity_isEntityInRange(me, n, chaseRange) and waitToAttack == 0 then
			entity_setState(me, STATE_CHASE)
		end
	end
	
	if entity_isState(me, STATE_ATTACK) then
		entity_rotateToEntity(me, n, 1)
	end
	
	if entity_isState(me, STATE_CHASE) then
		if entity_isEntityInRange(me, n, 210) then
			entity_setState(me, STATE_TRAP)
		end
	
		--overrideZoom(0.67)
		entity_setAnimLayerTimeMult(me, 0, 0.72)
		entity_moveTowards(me, entity_x(n), entity_y(n), dt, 432)
		entity_rotateToEntity(me, n, 0.21)
		
	elseif entity_isState(me, STATE_CREEP) then
		entity_setAnimLayerTimeMult(me, 0, 0.64)
		entity_moveTowards(me, entity_x(n), entity_y(n), dt, 323)
		entity_rotateToEntity(me, n, 0.34)
		
	else
		if entity_getVelLen(me) <= 234 then
			entity_clearVel(me)
			entity_setMaxSpeed(me, 0)
		end
	end
	
	-- AVOID WALLS
	entity_doCollisionAvoidance(me, dt, 8, 0.32)
		
	vecX, vecY = entity_getPosition(me)
	wallX, wallY = getWallNormal(entity_x(me), entity_y(me), 12)
	if wallX ~= 0 or wallY ~= 0 then
		vecX = vecX + wallX*256
		vecY = vecY + wallY*256
		entity_moveTowards(me, vecX, vecY, dt, 248)
	end
	
	entity_doFriction(me, dt, 234)
	
	-- COLLISIONS
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~= 0 then
		-- BITE NAIJA
		if entity_isState(me, STATE_ATTACK) then 
			entity_touchAvatarDamage(me, 0, 1, 800)
			hitNaija = 1
			
		--BUMP NAIJA
		else
			entity_touchAvatarDamage(me, 0, 0.1, 321)
		end
	end
	
	if not entity_isState(me, STATE_INTRO) then
		r = entity_getDistanceToEntity(me, n)
		if r < 800 then
			musicVolume(1, 0.1)
		else
			r = 1 - ((r-800) / 1024)
			if r < 0.3 then
				r = 0.3
			end
			if r > 1 then
				r = 1
			end
			musicVolume(r)
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", LOOP_INF)
		entity_setAnimLayerTimeMult(me, 0, 1)
		
	elseif entity_isState(me, STATE_MOVE) then
		
		shakeCamera(3.4, 2.3)
		
		entity_animate(me, "crawl", LOOP_INF)
		node = 0
		
		-- MOVING BETWEEN NODES w/ PATHFINDING
		if phase == PHASE_LURE then
			nodeName = string.format("L%d", lureNode)
		elseif phase == PHASE_HIDE then
			rnd = math.random(9)
			while rnd == lastNodeNum do	-- While loops are scary
				rnd = math.random(9)
			end
			nodeName = string.format("W%d", rnd)
			lastNodeNum = rnd
		elseif phase == PHASE_FINAL then
			nodeName = "WFINAL"
			
			node = getNode("LIDOOR")
			door = node_getNearestEntity(node, "FinalDoor")
			entity_setState(door, STATE_OPENED, -1, 1)
			
			node = getNode("LIDOOR2")
			door = node_getNearestEntity(node, "FinalDoor")
			entity_setState(door, STATE_OPENED, -1, 1)
			
			playSfx("TentacleDoor")
			playSfx("CreatorForm4-hit1")
		end
		node = entity_getNearestNode(me, nodeName)
		entity_swimToNode(me, node, SPEED_FAST2)
		
	elseif entity_isState(me, STATE_TRAP) then
		entity_setMaxSpeed(me, maxSpeed/8)
		entity_animate(me, "idle", LOOP_INF)
		
	elseif entity_isState(me, STATE_ATTACK) then
		entity_setStateTime(me, entity_animate(me, "attack"))
		waitToAttack = 0
		hitNaija = 0
	
	elseif entity_isState(me, STATE_CHASE) then
		entity_setMaxSpeed(me, maxSpeed)
		stateTime = 1.56
		shakeCamera(2.3, stateTime)
		entity_animate(me, "crawl", LOOP_INF)
		entity_setStateTime(me, stateTime)
	
	elseif entity_isState(me, STATE_CREEP) then
		entity_setMaxSpeed(me, maxSpeed/2)
		stateTime = 1.2
		shakeCamera(2.1, stateTime)
		entity_animate(me, "crawl", LOOP_INF)
		entity_setStateTime(me, stateTime)
		
	elseif entity_isState(me, STATE_PAIN) then
		if chance(50) then
			playSfx("CreatorForm4-Hit1")
		else
			playSfx("CreatorForm4-Hit2")
		end
		entity_setStateTime(me, entity_animate(me, "pain"))
		
	elseif entity_isState(me, STATE_TRANSITION) then
		playSfx("CreatorForm4-Die")
		lastNode = getNode("WFINAL")
		entity_stopInterpolating()
		
		entity_setPosition(me, node_x(lastNode), node_y(lastNode))
		entity_setPosition(me, node_x(lastNode), node_y(lastNode), 1)
		
		entity_animate(me, "idle", LOOP_INF)
		entity_rotate(me, 0, 1,0,0,1)
		entity_setStateTime(me, 1)
		
		entity_idle(n)
		disableInput()
		cam_toEntity(me)
	
	elseif entity_isState(me, STATE_INTRO) then
		entity_scale(me, 0, 0)
		entity_scale(me, 1, 1, 3)
		entity_animate(me, "idle")
		entity_setStateTime(me, 3)
		
		playMusic("worship3")
	end
end

function exitState(me)
	if entity_isState(me, STATE_MOVE) then
		debugLog("move state ended")
		if phase == PHASE_LURE then
			lureNode = lureNode + 1
			if lureNode > 7 then
				phase = PHASE_HIDE
				entity_setState(me, STATE_MOVE)
			else
				entity_setState(me, STATE_LUREWAIT)
			end
		elseif phase == PHASE_HIDE then
			if hideCount <= 0 then
				entity_setState(me, STATE_TRAP)
				waitToAttack = 1
				hideCount = 0
			else
				hideCount = hideCount - 1
				entity_setState(me, STATE_MOVE)
			end
		elseif phase == PHASE_FINAL then
			entity_setState(me, STATE_TRAP)
			waitToAttack = 1
		end
		
	elseif entity_isState(me, STATE_PAIN) then
		if phase == PHASE_HIDE then
			hideCount = 4
			entity_setState(me, STATE_MOVE)
		else
			entity_setState(me, STATE_TRAP)
		end
		
	elseif entity_isState(me, STATE_IDLE) then
		entity_setState(me, STATE_MOVE)
		
	elseif entity_isState(me, STATE_ATTACK) then
		-- POST ATTACK, MOVE OR HANG BACK?
		if hitNaija == 0 and entity_isEntityInRange(me, n, attackRange) then
			entity_setState(me, STATE_CREEP)
			
		elseif hitNaija == 0 then
			entity_setState(me, STATE_CHASE)
			
		else
			entity_setState(me, STATE_TRAP)
		end
		
	elseif entity_isState(me, STATE_CHASE) then
		if entity_isEntityInRange(me, n, chaseRange) then 
			entity_setState(me, STATE_ATTACK)
		else
			entity_setState(me, STATE_TRAP)
		end
	
	elseif entity_isState(me, STATE_CREEP) then
		entity_setState(me, STATE_ATTACK)
		
	elseif entity_isState(me, STATE_TRANSITION) then
		entity_idle(n)
		enableInput()
		cam_toEntity(n)
		
		bx, by = bone_getWorldPosition(bone_darkLi)
		ent = createEntity("CreatorForm5", "", bx, by)
		
		entity_alpha(me, 0.6, 3)
		entity_setState(me, STATE_WAIT, 6)
		
	elseif entity_isState(me, STATE_WAIT) then
		entity_delete(me, 1)
	elseif entity_isState(me, STATE_INTRO) then
		entity_setState(me, STATE_MOVE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if phase == PHASE_LURE then return false end
	if entity_isState(me, STATE_PAIN) then return false end
	
	if bone == bone_body then
		bone_damageFlash(bone)
		if hits1 > 0 then
			hits1 = hits1 - 1	--dmg?
			if hits1 <= 0 then
				phase = PHASE_FINAL
				entity_setState(me, STATE_MOVE)
			else
				entity_setState(me, STATE_PAIN)
			end
		elseif hits2 > 0 then
			hits2 = hits2 - 1 --dmg?
			if hits2 <= 0 then
				entity_setState(me, STATE_TRANSITION)
			else
				entity_setState(me, STATE_PAIN)
			end
		end
	end
	
	return false
end

function animationKey(me, key)
	if entity_isState(me, STATE_MOVE) or entity_isState(me, STATE_CHASE) or entity_isState(me, STATE_CREEP) then
		if key == 1 then
			hX, hY = bone_getWorldPosition(bone_rightHand)
			spawnParticleEffect("CreatorForm4HandDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_leftHand)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			
			hX, hY = bone_getWorldPosition(bone_leftLeg1)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_leftLeg2)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			
			entity_sound(me, "RockHit")
			
		elseif key == 3 then
			hX, hY = bone_getWorldPosition(bone_leftHand)
			spawnParticleEffect("CreatorForm4HandDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_rightHand)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			
			hX, hY = bone_getWorldPosition(bone_rightLeg1)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_rightLeg2)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			
			entity_sound(me, "RockHit")
		end
	
	elseif entity_isState(me, STATE_ATTACK) then
		if key == 3 then
			hX, hY = bone_getWorldPosition(bone_leftHand)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_rightHand)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			
			hX, hY = bone_getWorldPosition(bone_leftLeg1)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_leftLeg2)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_rightLeg1)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			hX, hY = bone_getWorldPosition(bone_rightLeg2)
			spawnParticleEffect("CreatorForm4FootDust", hX, hY)
			
			entity_sound(me, "creatorform4-bite")
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
