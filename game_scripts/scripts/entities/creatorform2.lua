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

STATE_START			= 1000
STATE_GRABATTACK	= 1001
STATE_HOLDING		= 1002
STATE_INHAND		= 1003
STATE_BEAM			= 1004

bone_head			= 0
bone_body			= 0
grabPoint 			= 0
inHand				= false
breakFreeTimer		= 0
grabDelay			= 3
hits 				= 200 * 1.3

grabRange			= 0

shotDelay			= 0
shotDelayTime		= 0

prepDelay			= 8

beam = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "CreatorForm2")	
	--entity_setAllDamageTargets(me, false)
	
	entity_generateCollisionMask(me)
	
	entity_setState(me, STATE_IDLE)
	
	grabPoint = entity_getBoneByName(me, "Hand")
	bone_head = entity_getBoneByName(me, "Head")
	bone_body = entity_getBoneByName(me, "Body")
	grabRange = entity_getBoneByName(me, "GrabRange")
	
	entity_setMaxSpeed(me, 800)
	
	entity_setDamageTarget(me, DT_ENEMY_BEAM, false)
	
	entity_setCull(me, false)
	
	playMusic("Worship2")
	
	loadSound("creatorform2-shot")
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	--debugLog(string.format("hits %d", hits))
	if entity_isState(me, STATE_TRANSITION) or entity_isState(me, STATE_WAIT) then
		return
	end
	entity_doFriction(me, dt, 800)
	entity_doCollisionAvoidance(me, dt, 15, 0.5)
	entity_updateMovement(me, dt)


	if grabDelay > 0 then
		grabDelay = grabDelay - dt
		if grabDelay < 0 then
			grabDelay = 0
		end
	end
	
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	
	if bone ~= 0 then
		if not inHand and grabDelay == 0 and bone == grabPoint then
			inHand = true
			avatar_fallOffWall()
		end
		if not inHand and avatar_isBursting() and bone == bone_body and entity_setBoneLock(n, me, bone) then
		else
			bx, by = bone_getWorldPosition(bone)
			x, y = entity_getPosition(n)
			bx = x - bx
			by = y - by
			bx, by = vector_setLength(bx, by, 800)
			entity_addVel(n, bx, by)
		end
		if bone == grabPoint then
			entity_damage(n, me, 0.5)
		end
	end
	
	if inHand then
		entity_setPosition(n, bone_getWorldPosition(grabPoint))
		entity_rotate(n, bone_getWorldRotation(grabPoint)-90)
		
		if avatar_isRolling() then
			breakFreeTimer = breakFreeTimer + dt
			if breakFreeTimer > 2 then
				inHand = false
				breakFreeTimer = 0
				grabDelay = 4
			end
		end
	end
	
	if not inHand and math.abs(entity_x(me) - entity_x(n)) > 256 then
		entity_flipToEntity(me, n)
	end
	
	entity_clearTargetPoints(me)
	entity_addTargetPoint(me, bone_getWorldPosition(bone_head))
	
	if entity_isState(me, STATE_IDLE) then
		prepDelay = prepDelay - dt
		if prepDelay < 0 then
			bx, by = bone_getWorldPosition(grabRange)
			if (entity_getBoneLockEntity(n) == me) or (entity_isPositionInRange(n, bx, by, 512) and chance(30)) then
				entity_setState(me, STATE_GRABATTACK)
			else
				if chance(50) then
					entity_setState(me, STATE_PREP)
				elseif chance(50) then
					entity_setState(me, STATE_BEAM)
				end
			end
		end
	end
	
	if entity_isState(me, STATE_ATTACK) then
		shotDelay = shotDelay - dt
		if shotDelay <= 0 then
			s = createShot("CreatorForm2", me, n, bone_getWorldPosition(bone_head))
			shotDelayTime = shotDelayTime - 0.01
			shotDelay = shotDelayTime
		end
	end
	
	if entity_isState(me, STATE_BEAM) then
		if beam ~= 0 then
			if entity_isfh(me) then
				beam_setAngle(beam, bone_getWorldRotation(bone_head)+90)
			else
				beam_setAngle(beam, bone_getWorldRotation(bone_head)-90)
			end
			beam_setPosition(beam, bone_getWorldPosition(bone_head))
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "start", -1)
	elseif entity_isState(me, STATE_START) then
		entity_setStateTime(me, entity_animate(me, "start", 2))
	elseif entity_isState(me, STATE_TRANSITION) then
		inHand = false
		clearShots()
		entity_setAllDamageTargets(me, false)
		
		entity_idle(n)
		disableInput()
		entity_setInvincible(n, true)
		cam_toEntity(me)
		
		node = entity_getNearestNode(me, "CENTER")
		entity_setPosition(me, node_x(node), node_y(node), 3, 0, 0, 1)	
		
		t = entity_animate(me, "die")
		entity_setStateTime(me, t + 2)
		
		node = getNode("HASTYEXIT")
		door = node_getNearestEntity(node, "FinalDoor")
		entity_setState(door, STATE_OPEN)
	elseif entity_isState(me, STATE_PREP) then
		entity_setStateTime(me, entity_animate(me, "prep"))
	elseif entity_isState(me, STATE_ATTACK) then
		entity_animate(me, "attack", -1)
		entity_setStateTime(me, 6)
		shotDelayTime = 1
	elseif entity_isState(me, STATE_GRABATTACK) then
		entity_setStateTime(me, entity_animate(me, "grabAttack"))
		shakeCamera(10, 1)
		avatar_fallOffWall()
	elseif entity_isState(me, STATE_BEAM) then
		entity_setStateTime(me, entity_animate(me, "beam"))
		shakeCamera(10, 3)
		avatar_fallOffWall()
		beam = 0
		voice("Laugh3")
	end
end

function exitState(me)
	if entity_isState(me, STATE_START) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_TRANSITION) then
		bx, by = bone_getWorldPosition(bone_head)
		createEntity("CreatorForm4", "", bx, by)
		entity_setState(me, STATE_WAIT, 2)
	elseif entity_isState(me, STATE_WAIT) then
		entity_delete(me)
		enableInput()
		entity_setInvincible(n, false)
		cam_toEntity(n)
	elseif entity_isState(me, STATE_PREP) then
		entity_setState(me, STATE_ATTACK)
	elseif entity_isState(me, STATE_ATTACK) or entity_isState(me, STATE_GRABATTACK) or entity_isState(me, STATE_BEAM) then
		if beam ~= 0 then
			beam_delete(beam)
			beam = 0
		end
		prepDelay = math.random(3)+4
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if bone == bone_head then
		bone_damageFlash(bone)
		hits = hits - dmg
		if hits <= 0 then
			entity_setState(me, STATE_TRANSITION)
		end
		return false
	end
	return false
end

function animationKey(me, key)
	if entity_isState(me, STATE_IDLE) and (key == 2 or key == 3) then
		entity_moveTowards(me, entity_x(n), entity_y(n), 1, 1000)
	elseif entity_isState(me, STATE_BEAM) and key == 1 then
		beam = createBeam()
		beam_setTexture(beam, "particles/Beam")
		beam_setDamage(beam, 3)
		playSfx("PowerUp")
		playSfx("FizzleBarrier")
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

