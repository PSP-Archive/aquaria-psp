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

n = 0

STATE_STEPFORE			= 1000
STATE_STEPBACK			= 1001
STATE_ATTACK1			= 1002
STATE_ATTACK2			= 1003
STATE_ATTACK3			= 1004
STATE_BACKHANDATTACK	= 1005
STATE_MOUTHATTACK		= 1006
STATE_SPAWNNAIJA		= 1007

STATE_SCENEGHOST		= 1010


maxLeft					= 0
maxRight				= 0



stepDelay = 0
attackDelay = 0

eye = 0
hand = 0
forearm = 0
socket = 0
neck = 0
backHand = 0
tongue = 0

li = 0

shieldHits = 3      --9
hits = 3 -- 3

camNode	= 0
camBone = 0


chestMonster = 0
chestShield = 0
eyeCover = 0
eyeSocket = 0

eyeSpiral = 0

eyeCoverHits = 24

PHASE_HASLI		= 0
PHASE_FINAL		= 1

phase = PHASE_HASLI

attackPhase = 0

function enterFinalPhase(me)
	debugLog("setting phase to final")
	
	playSfx("naijali1")
	
	setFlag(FLAG_LI, 100)
	entity_setState(li, STATE_IDLE, -1, true)
	phase = PHASE_FINAL
	
	chestMonster = createEntity("chestmonster", "", entity_x(me), entity_y(me))
	
	playSfx("licage-shatter")
	
	bone_alpha(chestShield, 0, 2)
	
	attackPhase = 0
end

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "CreatorForm6")	
	--entity_setAllDamageTargets(me, false)
	entity_setCull(me, false)
	
	entity_generateCollisionMask(me)	
	
	entity_setState(me, STATE_IDLE)
	entity_scale(me, 2, 2)
	
	eye = entity_getBoneByName(me, "Eye")
	hand = entity_getBoneByName(me, "Hand")
	forearm = entity_getBoneByName(me, "Forearm")
	socket = entity_getBoneByName(me, "Socket")
	neck = entity_getBoneByName(me, "Neck")
	
	
	chestShield = entity_getBoneByName(me, "chestshield")
	
	eyeSocket = entity_getBoneByName(me, "eyesocket")
	eyeCover = entity_getBoneByName(me, "eyecover")
	
	
	entity_setTargetRange(me, 2000)
	
	
	bone_setVisible(eyeSocket, 0)
	
	backHand = entity_getBoneByName(me, "BackHand")
	tongue = entity_getBoneByName(me, "tongue")
	
	bone_setAnimated(eye, ANIM_POS)
	
	camBone = eye
	
	li = getLi()
	
	esetv(me, EV_SOULSCREAMRADIUS, -1)
	
	setFlag(FLAG_LI, 200)
	
	loadSound("licage-crack1")
	loadSound("licage-crack2")
	loadSound("licage-shatter")
	
	loadSound("creatorform6-die3")
	
	loadSound("hellbeast-shot-skull")
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	
	node = getNode("MAXLEFT")
	maxLeft = node_x(node)
	node = getNode("MAXRIGHT")
	maxRight = node_x(node)
	
	camNode = getNode("CAM")
	
	
	
	if li == 0 then
		-- create li
		li = createEntity("Li")
		setLi(li)
	end
	
	entity_setState(li, STATE_TRAPPEDINCREATOR, -1, true)
end

pd = 0
function update(me, dt)
	if entity_isState(me, STATE_WAIT) then
		return
	end
	if entity_isState(me, STATE_TRANSITION) or entity_isState(me, STATE_SCENEGHOST) then
		bx, by = bone_getWorldPosition(camBone)
		node_setPosition(camNode, bx, by)
		cam_toNode(camNode)
		pd = pd + dt
		if pd > 0.2 then
			spawnParticleEffect("TinyRedExplode", bx-500+math.random(1000), by-500 + math.random(1000))
			pd = 0
		end
		return 
	end
	
	entity_updateMovement(me, dt)
	
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	
	if bone ~= 0 then
		--[[
		if avatar_isBursting() and bone ~= hand and bone ~= forearm and entity_setBoneLock(n, me, bone) then
		else
		]]--
			-- puuush
			--entity_addVel(n, -800, 0)
			bx, by = bone_getWorldPosition(bone)
			x, y = entity_getPosition(n)
			x = x - bx
			y = y - by
			x,y = vector_setLength(x, y, 2000)
			entity_clearVel(n)
			entity_addVel(n, x, y)
			
			x,y = vector_setLength(x, y, 8)
			entity_setPosition(n, entity_x(n) + x -20, entity_y(n) + y)
			
			--if bone == hand or bone == forearm then
			entity_damage(n, me, 1)
			avatar_fallOffWall()
			--end
			--entity_addVel(n, -400, 0)
		--end
	end
	
	overrideZoom(0.45)
	
	if entity_isState(me, STATE_IDLE) then
		stepDelay = stepDelay + dt
		if stepDelay > 2 then
			stepDelay = 0
			if entity_x(me) > maxLeft and chance(50) then
				entity_setState(me, STATE_STEPFORE)
			elseif entity_x(me) < maxRight then
				entity_setState(me, STATE_STEPBACK)
			end
		end
		
		if phase == PHASE_HASLI then
			attackDelay = attackDelay + dt
			if attackDelay > 4 then
				attackDelay = 0
				if attackPhase == 0 then
					entity_setState(me, STATE_BACKHANDATTACK)
				elseif attackPhase == 1 then
					entity_setState(me, STATE_SPAWNNAIJA)
				elseif attackPhase == 2 then
					entity_setState(me, STATE_MOUTHATTACK)
				end
				attackPhase = attackPhase + 1
				if attackPhase > 2 then
					attackPhase = 0
				end
			end
		end
		if phase == PHASE_FINAL then
			attackDelay = attackDelay + dt
			if attackDelay > 4 then
				attackDelay = 0
				if attackPhase == 0 then
					entity_setState(chestMonster, STATE_OPEN)
					attackDelay = -3
				elseif attackPhase == 1 then
					entity_setState(me, STATE_BACKHANDATTACK)
				elseif attackPhase == 2 then
					-- spawn aleph ???
				elseif attackPhase == 3 then
				end
				attackPhase = attackPhase + 1
				if attackPhase > 1 then
					attackPhase = 0
				end
			end
		end
	end
	
	dist = 270
	bx, by = bone_getWorldPosition(eye)
	if entity_y(n) > by + dist then
		bone_rotate(eye, -25, 0.5)
	elseif entity_y(n) < by - dist then
		bone_rotate(eye, 35, 0.5)
	else
		bone_rotate(eye, 0, 0.5)
	end
	
	if li ~= 0 and phase == PHASE_HASLI then
		entity_setPosition(li, bone_getWorldPosition(socket))
	end
	
	if phase == PHASE_FINAL then
		if chestMonster ~= 0 then
			entity_setPosition(chestMonster, bone_getWorldPosition(socket))
		end
	end
	
	if eyeSpiral ~= 0 then
		bx,by = bone_getWorldPosition(eyeSocket)
		entity_setPosition(eyeSpiral, bx-64, by)
	end
	
	entity_clearTargetPoints(me)
	
	if phase == PHASE_HASLI then
		if bone_isVisible(eyeCover) then
			entity_addTargetPoint(me, bone_getWorldPosition(eyeCover))
		end
	end
end

stepTime = 2

function flash()
end

incut = false

function enterState(me)
	if incut then return end
	
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_STEPFORE) then
		entity_setPosition(me, entity_x(me)-600, entity_y(me), stepTime, 0, 0, 0)
		entity_setStateTime(me, stepTime)
	elseif entity_isState(me, STATE_STEPBACK) then
		entity_setPosition(me, entity_x(me)+600, entity_y(me), stepTime, 0, 0, 0)
		entity_setStateTime(me, stepTime)
	elseif entity_isState(me, STATE_ATTACK1) then
		entity_setStateTime(me, entity_animate(me, "attack1"))
	elseif entity_isState(me, STATE_TRANSITION) then
		if chestMonster ~= 0 then
			entity_delete(chestMonster)
			chestMonter = 0
		end
		
		bone_setAnimated(eye, ANIM_ALL)
		incut = true
		

		--entity_setStateTime(me, entity_animate(me, "die"))
		--entity_setStateTime(me, 22)
		
		-- 22
		-- gets picked up by node FINALBOSSDEATH
		--[[
		flash() entity_animate(me, "die1", -1)
		watch(7)
		flash() entity_animate(me, "die2", -1)
		watch(5)
		flash() entity_animate(me, "die3", -1)
		watch(9)
		entity_setStateTime(me, 0.01)
		]]--
		incut = false
	elseif entity_isState(me, STATE_SCENEGHOST) then
		debugLog("ghost")
		incut = true

		incut = false
	elseif entity_isState(me, STATE_WAIT) then
		debugLog("wait")
	elseif entity_isState(me, STATE_BACKHANDATTACK) then
		entity_setStateTime(me, entity_animate(me, "backHandAttack"))
	elseif entity_isState(me, STATE_MOUTHATTACK) then
		entity_setStateTime(me, entity_animate(me, "mouthattack"))
	elseif entity_isState(me, STATE_SPAWNNAIJA) then
		entity_setStateTime(me, entity_animate(me,  "spawnthing"))
	end
end

function exitState(me)
	if entity_isState(me, STATE_STEPFORE) or entity_isState(me, STATE_STEPBACK) or entity_isState(me, STATE_ATTACK1) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_TRANSITION) then
		--entity_setState(me, STATE_SCENEGHOST)
	elseif entity_isState(me, STATE_SCENEGHOST) then
		disableInput()
		fade(1, 2, 0, 0, 0)
	elseif entity_isState(me, STATE_BACKHANDATTACK) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_MOUTHATTACK) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_SPAWNNAIJA) then
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	--[[
	if phase == PHASE_HASLI then
		if damageType == DT_ENEMY_CREATOR then
			debugLog("damage type is dt_enemy_creator")
			if bone == socket then
				shieldHits = shieldHits - dmg
				bone_damageFlash(bone)
				if shieldHits <= 0 then
					enterFinalPhase(me)
				end
			end
		end
	end
	]]--
	if damageType == DT_ENEMY_BEAM then
		return false
	end
	if bone == eyeCover then
		bone_damageFlash(eyeCover)
		eyeCoverHits = eyeCoverHits - dmg
		playSfx("licage-crack1")
		if eyeCoverHits <= 0 then
			bone_setVisible(eyeCover, 0)
			bone_setVisible(eyeSocket, 1)
			playSfx("licage-shatter")
			
			eyeSpiral = createEntity("eyespiral", "", bone_getWorldPosition(eyeSocket))
		end
	end
	
	if phase == PHASE_FINAL then
		if damageType == DT_AVATAR_DUALFORMNAIJA then
			hits = hits - 1
			for i = 0,10 do
				bone_damageFlash(entity_getBoneByIdx(me, i))
			end
			playSfx("creatorform6-die3")
			if hits <= 0 then
				entity_setState(me, STATE_TRANSITION)
			end
		end
	end

	return false
end

function animationKey(me, key)
	if entity_isState(me, STATE_BACKHANDATTACK) then
		if key == 4 or key == 5 or key == 6 or key == 7 or key == 8 then
			-- create entity
			debugLog("Creating entity")
			bx, by = bone_getWorldPosition(backHand)
			vx,vy = entity_getPosition(getNaija())
			vx = vx - bx
			vy = vy - by
			--e = createEntity("cf6-shot", "", bx, by)
			s = createShot("creatorform6-hand", me, getNaija(), bx, by)
			shot_setAimVector(s, vx, vy)
		end
	elseif entity_isState(me, STATE_MOUTHATTACK) then
		if key == 4 or key == 5 or key == 6 or key == 7 or key == 8 or key == 9 or key == 10 then
			bx, by = bone_getWorldPosition(tongue)
			vx,vy = entity_getPosition(getNaija())
			vx = vx - bx
			vy = vy - by
			
			s = createShot("creatorform6-hand", me, getNaija(), bx, by)
			shot_setAimVector(s, vx, vy)
		end
	elseif entity_isState(me, STATE_SPAWNNAIJA) then
		if key == 5 then -- key == 3 or 
			bx, by = bone_getWorldPosition(hand)
			spawnParticleEffect("tinyredexplode", bx, by)
			createEntity("mutantnaija", "", bx, by)
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

function msg(me, msg)
	if msg == "eye" then
		camBone = eye
	elseif msg == "neck" then
		camBone = neck
	end
	if msg == "eyedied" then
		enterFinalPhase(me)
	end
	if msg == "eyepopped" then
		fade2(1, 0, 1, 1, 1)
		fade2(0, 1, 1, 1, 1)
		playSfx("creatorform6-die3")
		eyeSpiral = 0
		setSceneColor(1, 0, 0, 0)
		setSceneColor(1, 1, 1, 4)
		entity_animate(me, "eyehurt", 0, 1)
	end
end

