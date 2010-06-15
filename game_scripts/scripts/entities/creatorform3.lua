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
-- C R E A T O R ,   F O R M   3   (alpha)
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- D E B U G   J U N K
-- ================================================================================================

current_target = 0
last_node = 0
node_before_that = 0

-- ================================================================================================
-- S T A T E S
-- ================================================================================================

STATE_INTRO = 1001
STATE_PATTERN_01 = 1002
STATE_SWIMMING = 1003
STATE_AVOIDING_WALLS = 1004

-- ================================================================================================
-- L O C A L   V A R I A B L E S
-- ================================================================================================

angle = 0
rotateSpeed = 0.76

moveSpeed = 418 * 2

turnTimer = 0
turnT = 1
n = 0

killedSegs = 0

-- ================================================================================================
-- S E G M E N T   D A T A
-- ================================================================================================

bone_seg = {0, 0, 0, 0, 0, 0, 0, 0}		-- Bone
mouth_seg = {0, 0, 0, 0, 0, 0, 0, 0}

shotT = 2
shotTimer_seg = {math.random(8)*0.5, math.random(8)*0.5, math.random(8)*0.5, math.random(8)*0.5, math.random(8)*0.5, math.random(8)*0.5, math.random(8)*0.5, math.random(8)*0.5}
--shotTimer_seg = {3, 3, 3, 3, 3, 3, 3, 3}

health_seg = {3, 3, 3, 3, 3, 3, 3, 3}

faceFrame_seg = {0, 0, 0, 0, 0, 0, 0, 0}

flipT = 0.42
flipTimer_seg = {0, 0, 0, 0, 0, 0, 0, 0}

-- ================================================================================================
-- N O D E S
-- ================================================================================================

targetNode = 0

A1 = 0
A2 = 0
B1 = 0
B2 = 0
B3 = 0
B4 = 0
C1 = 0
C2 = 0
C3 = 0
C4 = 0
C5 = 0
D1 = 0
D2 = 0
D3 = 0
D4 = 0

-- ================================================================================================
-- M Y   F U N C T I O N S
-- ================================================================================================

function changeAngle(me)
	angle = entity_getRotation(me)
	angle = angle + math.random(110) - 55
	
	entity_rotateTo(me, angle, turnT)
end

function rotateToNode(me, node)
	ndX, ndY = node_getPosition(node)
	meX, meY = entity_getPosition(me)
	vecX = (ndX - meX)
	vecY = (ndY - meY)
	vecX, vecY = vector_cap(vecX, vecY, 512)
	entity_rotateToVec(me, vecX, vecY, rotateSpeed)
end

-- NODE LIST
function loadNodes(me)
	targetNode = entity_getNearestNode(me, "C2")
	
	A1 = entity_getNearestNode(me, "A1")
	A2 = entity_getNearestNode(me, "A2")
	B1 = entity_getNearestNode(me, "B1")
	B2 = entity_getNearestNode(me, "B2")
	B3 = entity_getNearestNode(me, "B3")
	B4 = entity_getNearestNode(me, "B4")
	C1 = entity_getNearestNode(me, "C1")
	C2 = entity_getNearestNode(me, "C2")
	C3 = entity_getNearestNode(me, "C3")
	C4 = entity_getNearestNode(me, "C4")
	C5 = entity_getNearestNode(me, "C5")
	D1 = entity_getNearestNode(me, "D1")
	D2 = entity_getNearestNode(me, "D2")
	D3 = entity_getNearestNode(me, "D3")
	D4 = entity_getNearestNode(me, "D4")
end

-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function init(me)
	setupBasicEntity(
	me,
	"Creator/Form3/Head",			-- texture
	64,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	32,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	256,							-- sprite width
	256,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	6000							-- updateCull -1: disabled, default: 4000
	)
	
	entity_setDeathParticleEffect(me, "Explode")
	entity_setDropChance(me, 21)
	
	entity_initSkeletal(me, "CreatorForm3")
	entity_generateCollisionMask(me)
	head = entity_getBoneByName(me, "Head")
	jaw = entity_getBoneByName(me, "Jaw")
	tail = entity_getBoneByName(me, "Tail")
	
	-- BODY SEGS
	bone_seg[1] = entity_getBoneByName(me, "BodySeg1")
	mouth_seg[1] = entity_getBoneByName(me, "MouthSeg1")
	bone_alpha(mouth_seg[1], 0)
	bone_seg[2] = entity_getBoneByName(me, "BodySeg2")
	mouth_seg[2] = entity_getBoneByName(me, "MouthSeg2")
	bone_alpha(mouth_seg[2], 0)
	bone_seg[3] = entity_getBoneByName(me, "BodySeg3")
	mouth_seg[3] = entity_getBoneByName(me, "MouthSeg3")
	bone_alpha(mouth_seg[3], 0)
	bone_seg[4] = entity_getBoneByName(me, "BodySeg4")
	mouth_seg[4] = entity_getBoneByName(me, "MouthSeg4")
	bone_alpha(mouth_seg[4], 0)
	bone_seg[5] = entity_getBoneByName(me, "BodySeg5")
	mouth_seg[5] = entity_getBoneByName(me, "MouthSeg5")
	bone_alpha(mouth_seg[5], 0)
	bone_seg[6] = entity_getBoneByName(me, "BodySeg6")
	mouth_seg[6] = entity_getBoneByName(me, "MouthSeg6")
	bone_alpha(mouth_seg[6], 0)
	bone_seg[7] = entity_getBoneByName(me, "BodySeg7")
	mouth_seg[7] = entity_getBoneByName(me, "MouthSeg7")
	bone_alpha(mouth_seg[7], 0)
	bone_seg[8] = entity_getBoneByName(me, "BodySeg8")
	mouth_seg[8] = entity_getBoneByName(me, "MouthSeg8")
	bone_alpha(mouth_seg[8], 0)
	
	entity_setCull(me, false)
	
	entity_setMaxSpeed(me, moveSpeed)
	
	-- Don't collide with the level.  Hm.
	--esetv(me, EV_COLLIDELEVEL, 0)
	
	-- SEGMENT TAIL
	bone_setSegmentChainHead(head, true)
	bone_setSegmentProps(head, 154, 154, true)
	for i=10,2,-1 do
		b = entity_getBoneByIdx(me, i)
		bone_addSegment(head, b)
		bone_rotateOffset(b, -90)
	end
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, getNaija())
	
	entity_setState(me, STATE_INTRO)
	
	entity_scale(me, 0, 0)
	
	-- NODE LIST
	loadNodes(me)
	
	current_target = targetNode
	last_node = targetNode
	node_before_that = targetNode
	

end

function update(me, dt)

	dt = dt * 2

	-- DEBUG:  WHERE IS WORMY GOING??
	if current_target ~= targetNode then
		node_before_that = last_node
		last_node = current_target
		current_target = targetNode
		
		debugLog(string.format("- - - "))
		debugLog(string.format("Current Target: %s", node_getName(current_target)))
		debugLog(string.format("Last Node: %s", node_getName(last_node)))
		debugLog(string.format("Node Before That: %s", node_getName(node_before_that)))
	end
	
	-- S E G M E N T   L O O P
	for i=1,8 do
		segX, segY = bone_getWorldPosition(bone_seg[i])
		mouthX, mouthY = bone_getWorldPosition(mouth_seg[i])
		
		if health_seg[i] > 0 then
			-- SHOOTING
			shotTimer_seg[i] = shotTimer_seg[i] - dt
			if shotTimer_seg[i] <= 0 then
				shotTimer_seg[i] = shotT + (math.random(200) * 0.01)
				
				nX, nY = bone_getNormal(bone_seg[i])
				nX, nY = vector_setLength(nX, nY, 123)

				spawnParticleEffect("CreatorForm3Shot1", mouthX, mouthY)
				s = createShot("CreatorForm3Shot1", me, 0, mouthX, mouthY)
				shot_setAimVector(s, -nX, -nY)
			end
			
		else -- If no health
			numFlipFrames = 3
			-- FLIP FACES
			if faceFrame_seg[i] < numFlipFrames then
			
				flipTimer_seg[i] = flipTimer_seg[i] - dt
				if flipTimer_seg[i] <= 0 then
					flipTimer_seg[i] = flipT
					
					faceFrame_seg[i] = faceFrame_seg[i] + 1
					
					bone_setTexture(bone_seg[i], string.format("Creator/Form3/BodySegFlip%d", faceFrame_seg[i]))
				end
			else
				flipTimer_seg[i] = 0
				faceFrame_seg[i] = numFlipFrames
			end
		end
		-- RANDOM BUBBLES
		if chance(2) then
			spawnParticleEffect("CreatorForm3LiteBubbles", segX, segY)
		end
	end
	
	

	if entity_getState(me) == STATE_INTRO then
		entity_addVel(me, 0, -32)
		
	elseif entity_getState(me) == STATE_PATTERN_01 then
		overrideZoom(0.42)
		
	-------------------------------------------------------------------------------------------------
	-- N O D E   C O D E
	-------------------------------------------------------------------------------------------------
	--[[
		-- A1
		if A1 == targetNode and A1 ~=0 and node_isEntityIn(A1, me) then
			if entity_velx(me) < 0 then
				if chance(25) then targetNode = entity_getNearestNode(me, "C1")
				elseif chance(25) then targetNode = entity_getNearestNode(me, "D1")
				elseif chance(25) then targetNode = entity_getNearestNode(me, "C2")
				else targetNode = entity_getNearestNode(me, "B1") end
			else
				if chance(25) then targetNode = entity_getNearestNode(me, "C4")
				elseif chance(25) then targetNode = entity_getNearestNode(me, "B4")
				elseif chance(25) then targetNode = entity_getNearestNode(me, "D4")
				else targetNode = entity_getNearestNode(me, "D3") end	-- C5
			end
		-- A2
		elseif  A2 == targetNode and  A2 ~=0 and node_isEntityIn(A2, me) then 
			if entity_velx(me) < 0 then
				if chance(33.3) then targetNode = entity_getNearestNode(me, "B1")
				elseif chance(33.3) then targetNode = entity_getNearestNode(me, "C1")
				else targetNode = entity_getNearestNode(me, "C2") end
			else
				if chance(33.3) then targetNode = entity_getNearestNode(me, "D4")
				elseif chance(33.3) then targetNode = entity_getNearestNode(me, "B4")
				else targetNode = entity_getNearestNode(me, "C5") end
			end
		-- B1
		elseif  B1 == targetNode and  B1 ~=0 and node_isEntityIn(B1, me) then 
			if entity_vely(me) > 0 then
				if chance(33.3) then targetNode = entity_getNearestNode(me, "D3")
				elseif chance(33.3) then targetNode = entity_getNearestNode(me, "D1")	-- BUMP?
				else targetNode = entity_getNearestNode(me, "C1") end
			else
				if chance(33.3) then targetNode = entity_getNearestNode(me, "B3")
				elseif chance(33.3) then targetNode = entity_getNearestNode(me, "A2")
				else targetNode = entity_getNearestNode(me, "C3") end
			end
		-- B2
		elseif  B2 == targetNode and  B2 ~=0 and node_isEntityIn(B2, me) then 
			if entity_velx(me) < 0 then
				if chance(50) then targetNode = entity_getNearestNode(me, "C1")
				else targetNode = entity_getNearestNode(me, "D2") end
			else
				if chance(33.3) then targetNode = entity_getNearestNode(me, "D4")
				elseif chance(33.3) then targetNode = entity_getNearestNode(me, "C5")
				else targetNode = entity_getNearestNode(me, "B4") end
			end
		-- B3
		elseif  B3 == targetNode and  B3 ~=0 and node_isEntityIn(B3, me) then 
			if entity_velx(me) > 0 then
				if chance(50) then targetNode = entity_getNearestNode(me, "C5")
				else targetNode = entity_getNearestNode(me, "D4") end
			else
				if chance(50) then targetNode = entity_getNearestNode(me, "C2")
				else targetNode = entity_getNearestNode(me, "B1") end
			end
		-- B4
		elseif  B4 == targetNode and  B4 ~=0 and node_isEntityIn(B4, me) then 
			if entity_vely(me) > 0 then
				if chance(50) then targetNode = entity_getNearestNode(me, "D4")
				else targetNode = entity_getNearestNode(me, "D3") end
			else
				if entity_velx(me) < 0 then
					targetNode = entity_getNearestNode(me, "B1")
				else
					targetNode = entity_getNearestNode(me, "B2")
				end
			end
		-- C1
		elseif  C1 == targetNode and  C1 ~=0 and node_isEntityIn(C1, me) then 
			if entity_vely(me) >= 0 then
				if chance(50) then targetNode = entity_getNearestNode(me, "D3")
				else targetNode = entity_getNearestNode(me, "C5") end	--D4?
			else
				if chance(20) then targetNode = entity_getNearestNode(me, "B3")
				elseif chance(20) then targetNode = entity_getNearestNode(me, "C4")
				elseif chance(20) then targetNode = entity_getNearestNode(me, "B1")
				elseif chance(20) then targetNode = entity_getNearestNode(me, "A2")
				else targetNode = entity_getNearestNode(me, "A1") end
			end
		-- C2
		elseif  C2 == targetNode and  C2 ~=0 and node_isEntityIn(C2, me) then 
			if entity_vely(me) > 0 then
				if entity_velx(me) > 0 then
					targetNode = entity_getNearestNode(me, "B4")
				else
					if chance(50) then targetNode = entity_getNearestNode(me, "C1")
					else targetNode = entity_getNearestNode(me, "B1") end	-- trouble?
				end
			else
				if chance(33.3) then targetNode = entity_getNearestNode(me, "B3")
				elseif chance(33.3) then  targetNode = entity_getNearestNode(me, "A2")
				else targetNode = entity_getNearestNode(me, "B4") end
			end
		-- C3
		elseif  C3 == targetNode and  C3 ~=0 and node_isEntityIn(C3, me) then 
			if entity_velx(me) > 0 then
				if entity_vely(me) < 0 then
					if chance(33.3) then targetNode = entity_getNearestNode(me, "B4")
					elseif chance(33.3) then  targetNode = entity_getNearestNode(me, "C5")
					else targetNode = entity_getNearestNode(me, "A2") end	--trouble?
				else
					if chance(50) then targetNode = entity_getNearestNode(me, "C1")
					else targetNode = entity_getNearestNode(me, "C5") end
				end
			else
				if entity_vely(me) < 0 then
					if chance(50) then targetNode = entity_getNearestNode(me, "B1")
					else targetNode = entity_getNearestNode(me, "B4") end
				else
					if chance(50) then targetNode = entity_getNearestNode(me, "D2")
					else targetNode = entity_getNearestNode(me, "C1") end	--trouble?
				end
			end
		-- C4
		elseif C4 == targetNode and C4 ~=0 and node_isEntityIn(C4, me) then 
			if entity_vely(me) > 0 then
				targetNode = entity_getNearestNode(me, "D4")	--D2?
			else
				targetNode = entity_getNearestNode(me, "A1")
			end
		-- C5
		elseif C5 == targetNode and C5 ~=0 and node_isEntityIn(C5, me) then 
			if entity_vely(me) < 0 then
				if chance(33.3) then targetNode = entity_getNearestNode(me, "A1") 
				elseif chance(33.3) then targetNode = entity_getNearestNode(me, "A2") 
				else targetNode = entity_getNearestNode(me, "B4") end
			else
				if entity_velx(me) > 0 then		-- I F F Y
					targetNode = entity_getNearestNode(me, "D4")
					--if chance(50) then targetNode = entity_getNearestNode(me, "D4")
					--else targetNode = entity_getNearestNode(me, "D2") end	-- Aaarrrggghhh
				else
					if chance(50) then targetNode = entity_getNearestNode(me, "D1")
					else targetNode = entity_getNearestNode(me, "D2") end
				end
			end
		-- D1
		elseif D1 == targetNode and D1 ~=0 and node_isEntityIn(D1, me) then 
			if entity_vely(me) < 0 then
					targetNode = entity_getNearestNode(me, "B2")
			else
				if entity_velx(me) > 0 then
					targetNode = entity_getNearestNode(me, "C3")		-- BIG trouble!
				else
					targetNode = entity_getNearestNode(me, "B1")
				end
			end
		-- D2
		elseif D2 == targetNode and D2 ~=0 and node_isEntityIn(D2, me) then 
			if entity_velx(me) < 0 then
				targetNode = entity_getNearestNode(me, "C1")
			else
				if chance(33.3) then targetNode = entity_getNearestNode(me, "C5")
				elseif chance(33.3) then targetNode = entity_getNearestNode(me, "C4")
				else targetNode = entity_getNearestNode(me, "B4") end
			end
		-- D3
		elseif D3 == targetNode and D3 ~=0 and node_isEntityIn(D3, me) then 
			if entity_velx(me) > 0 then
				targetNode = entity_getNearestNode(me, "C5")
			else
				targetNode = entity_getNearestNode(me, "C1")
			end
		-- D4
		elseif D4 == targetNode and D4 ~=0 and node_isEntityIn(D4, me) then 
			if entity_vely(me) > 0 then
				if entity_velx(me) < 0 then
					if chance(33.3) then targetNode = entity_getNearestNode(me, "D2")
					elseif chance(33.3) then targetNode = entity_getNearestNode(me, "B1")
					else targetNode = entity_getNearestNode(me, "D1") end
				else
					targetNode = entity_getNearestNode(me, "B3")
				end
			else
				if chance(50) then targetNode = entity_getNearestNode(me, "B4")
				else targetNode = entity_getNearestNode(me, "A2") end
			end
		end
		]]--
		
		entNode = entity_getNearestNode(n, "WORM")
		targetNode = entNode
		
		-- TURN TO NEXT NODE
		rotateToNode(me, targetNode)
	-------------------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------------
	-------------------------------------------------------------------------------------------------
	
		entity_moveTowardsAngle(me, entity_getRotation(me), dt, moveSpeed)
		
		entity_doCollisionAvoidance(me, dt, 10, 0.1)
		
	elseif entity_getState(me) == STATE_SWIMMING then
		
		-- TURN TIMER
		if turnTimer > 0 then turnTimer = turnTimer - dt
		else
			turnTimer = turnT
				
			changeAngle(me)
		end
		
		entity_moveTowardsAngle(me, entity_getRotation(me), dt, moveSpeed)
		
		-- AVOID WALLS
		cX, cY = entity_getPosition(me)
		wallX, wallY = getWallNormal(entity_x(me), entity_y(me), 24)
		--debugLog(string.format("wall(%d, %d", wallX, wallY))
		if wallX ~= 0 or wallY ~= 0 then 
			entity_setState(me, STATE_AVOIDING_WALLS)
			
			turnTimer = turnT
			entity_setMaxSpeed(me, moveSpeed/2)
			
			cX = cX + wallX*256
			cY = cY + wallY*256
			--createShot("DropShot", me, 0, cX, cY)
			--entity_clearVel(me)
			--entity_moveTowards(me, cX, cY, 1, 1234)
			
			meX, meY = entity_getPosition(me)			
			vX = (cX - meX)
			vY = (cY - meY)
			vX, vY = vector_cap(vX, vY, 32)
			entity_rotateToVec(me, vX, vY, rotateSpeed)
		end
		
		-- AVOID TAIL
		tX, tY = bone_getWorldPosition(tail)
		hX, hY = bone_getWorldPosition(head)
		vX = hX - tX
		vY = hY - tY
		if vector_isLength2DIn(vX, vY, 123) then
			entity_moveTowards(me, tX, tY, dt, -1234)
		end
	
	elseif entity_getState(me) == STATE_AVOIDING_WALLS then
		entity_moveTowardsAngle(me, entity_getRotation(me), dt, moveSpeed)
	end

	-- UPDATE EVERYTHING
	entity_doEntityAvoidance(me, dt, 32, 2)
	entity_doFriction(me, dt, 64)
	entity_updateMovement(me, dt)
	
	-- COLLISIONS
	hitBone = entity_collideSkeletalVsCircle(me, getNaija())
	
	-- Do shot collisions
	entity_handleShotCollisionsSkeletal(me)
	
	-- Hurt Naija if Head, Jaw, or Tail is hit
	if hitBone == jaw or hitBone == tail then
		entity_damage(getNaija(), me, 0.1, DT_ENEMY_BITE)
	end
	
	-- Attach Naija to Back
	if hitBone ~= 0 and hitBone ~= jaw and hitBone ~= tail and avatar_isBursting() and entity_setBoneLock(getNaija(), me, hitBone) then
	
	-- Bump Naija away
	elseif hitBone ~= 0 then
		nX, nY = entity_getPosition(getNaija())
		bX, bY = bone_getWorldPosition(hitBone)
		nX = nX - bX
		nY = nY - bY
		nX, nY = vector_setLength(nX, nY, 600)
		entity_addVel(getNaija(), nX, nY)
	end
	
	entity_clearTargetPoints(me)
	
	for i=1,8 do
		x,y = bone_getWorldPosition(bone_seg[i])
		entity_addTargetPoint(me, x, y)
	end
end

function enterState(me)
	if entity_getState(me) == STATE_INTRO then
		stateTime = 1
		
		entity_setMaxSpeed(me, moveSpeed/4)
		
		entity_animate(me, "idle", LOOP_INF)
		
		entity_setStateTime(me, stateTime)
		entity_scale(me, 1, 1, stateTime)
		
	elseif entity_getState(me) == STATE_PATTERN_01 then
		entity_setMaxSpeed(me, moveSpeed/2)
		
	elseif entity_getState(me) == STATE_SWIMMING then
		entity_setMaxSpeed(me, moveSpeed/2)
		turnTimer = turnT
			
	elseif entity_getState(me) == STATE_AVOIDING_WALLS then
		entity_setStateTime(me, 3)
		
	elseif entity_isState(me, STATE_TRANSITION) then
		entity_setAllDamageTargets(me, false)
		
		entity_idle(n)
		disableInput()
		entity_setInvincible(n, true)
		cam_toEntity(me)
		
		node = entity_getNearestNode(me, "CENTER")
		entity_setPosition(me, node_x(node), node_y(node), 3, 0, 0, 1)	
		entity_setStateTime(me, 2)
	end
end

function exitState(me)
	if entity_getState(me) == STATE_INTRO then
		entity_setState(me, STATE_PATTERN_01)
		
	elseif entity_getState(me) == STATE_SWIMMING then
	
	elseif entity_getState(me) == STATE_AVOIDING_WALLS then
		entity_setState(me, STATE_SWIMMING)
		angle = entity_getRotation(me)
		entity_rotateToAngle(me, angle, 0.1)
		
	elseif entity_isState(me, STATE_TRANSITION) then
		bx, by = bone_getWorldPosition(tail)
		ent = createEntity("CreatorForm4", "", bx, by)
		entity_setState(me, STATE_WAIT, 2)
		cam_toEntity(ent)
	elseif entity_isState(me, STATE_WAIT) then
		entity_delete(me)
		enableInput()
		entity_setInvincible(n, false)
		cam_toEntity(n)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	for i=1,8 do
		if bone == bone_seg[i] and faceFrame_seg[i] == 0 and health_seg[i] > 0 then
			bnx, bny = bone_getNormal(bone_seg[i])
			bx, by = bone_getWorldPosition(bone_seg[i])
			sx, sy = getLastCollidePosition()
			nx = bx - sx
			ny = by - sy
			nx, ny = vector_setLength(nx, ny, 1)
			--dot = vector_dot(bnx, bny, nx, ny)
			dot = 0
			debugLog(string.format("boneNormal(%d, %d) vs shotNormal(%d, %d) dot: %d", bnx, bny, nx, ny, dot))
			if dot > 0 then
				health_seg[i] = health_seg[i] - 1
				bone_damageFlash(bone)
			end
		end
		
		if health_seg[i] <= 0 and faceFrame_seg[i] == 0 then
			faceFrame_seg[i] = 1
			flipTimer_seg[i] = flipT
			health_seg[i] = 0
			killedSegs = killedSegs + 1
			if killedSegs >= 8 then
				entity_setState(me, STATE_TRANSITION)
			end
		end
	end

	return false
end

function hitSurface(me)
	
end
