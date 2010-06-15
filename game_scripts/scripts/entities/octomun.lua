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
body = 0
weakPoint = 0
marker1 = 0
marker2 = 0
attackDelay = 0
attack1Marker = 0
grabbingEntity = 0
dark = 0
inkBlastDelay = 0

STATE_ATTACK1 = 1001
STATE_BASH = 1002

fireDelay = 0

shot1 = 0
shot2 = 0
shot3 = 0

fireOff = 1

isWeakPointHittable = false

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "Octomun")	
	body = entity_getBoneByName(me, "Body")
	weakPoint = entity_getBoneByName(me, "WeakPoint")
	marker1 = entity_getBoneByName(me, "Marker1")
	marker2 = entity_getBoneByName(me, "Marker2")
	attack1Marker = entity_getBoneByName(me, "Attack1Marker")
	
	bone_alpha(attack1Marker, 0)
	
	shot1 = entity_getBoneByName(me, "Shot1")
	shot2 = entity_getBoneByName(me, "Shot2")
	shot3 = entity_getBoneByName(me, "Shot3")
	
	--entity_setAllDamageTargets(me, false)
	
	entity_generateCollisionMask(me)
	
	entity_setAllDamageTargets(me, false)
	entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
	
	entity_setCullRadius(me, 1024)
	entity_setState(me, STATE_IDLE)
	entity_setTargetRange(me, 512)
	
	dark = createQuad("Octomun/Dark", 13)
	quad_scale(dark, 64, 64)
	quad_alpha(dark, 0)
	
	entity_setUpdateCull(me, 4064)
	
	loadSound("Octomun-Growl")
	loadSound("Octomun-Hit")
	loadSound("Octomun-Shot")
	loadSound("Octomun-Ink")
	
	loadSound("BossDieSmall")
	loadSound("BossDieBig")
	
	loadSound("rotcore-birth")
	
	entity_setTargetRange(me, 1024)
	
	entity_setHealth(me, 30)
	
	entity_setDeathScene(me, true)
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function tentacle1Collision(me)
	if grabbingEntity ~= 0 then
		return
	end
	x1, y1 = bone_getWorldPosition(marker2)
	x2, y2 = bone_getWorldPosition(marker1)
	
	if entity_isPositionInRange(n, x2, y2, 96) then
		grabbingEntity = n
		avatar_fallOffWall()
		entity_idle(grabbingEntity)
		entity_animate(grabbingEntity, "trapped", LOOP_INF, LAYER_OVERRIDE)		
	elseif entity_collideCircleVsLine(n, x1, y1, x2, y2, 32) then
		entity_damage(n, me, 1)
	end
end

seen = 0

fired = 0

started = false

function update(me, dt)
	quad_setPosition(dark, entity_getPosition(me))
	overrideZoom(0.6, 0.5)
	entity_handleShotCollisionsSkeletal(me)
	entity_clearTargetPoints(me)
	bx,by = bone_getWorldPosition(weakPoint)
	entity_addTargetPoint(me, bx, by)
	
	if seen == 0 and entity_isEntityInRange(me, n, 1000) then
		playSfx("Octomun-Growl")
		seen = 1
	end
	
	if seen < 2 and entity_isEntityInRange(me, n, 800) then
		emote(EMOTE_NAIJAUGH)
		playMusic("MiniBoss")
		started = true
		seen = 2
	end
	
	if not started then return end
	
	if quad_getAlpha(dark)<0.1 then
		inkBlastDelay = inkBlastDelay + dt
		if inkBlastDelay > 20 then
			playSfx("Octomun-Ink")
			spawnParticleEffect("InkBlast", entity_getPosition(me))
			quad_alpha(dark, 1, 4)
			inkBlastDelay = math.random(5)
		end
	end
	if entity_isState(me, STATE_IDLE) then
		attackDelay = attackDelay + dt
		if attackDelay > 1 then
			if grabbingEntity~=0 then
				entity_setState(me, STATE_BASH)
			else
				x,y = bone_getWorldPosition(attack1Marker)
				if entity_x(n) < x and entity_y(n) > y and entity_x(n) > entity_x(me) then
					entity_setState(me, STATE_ATTACK1)
				end
			end
		--[[
			bx, by = bone_getWorldPosition(marker1)
			if entity_isPositionInRange(n, bx, by, 128) then
				entity_setState(me, STATE_ATTACK1)
			end
			]]--
		end
		fireDelay = fireDelay + dt
		if fireDelay > 6 and fired == 0 then
			playSfx("Octomun-Shot")
			s = createShot("Octomun", me, n, bone_getWorldPosition(shot1))
			shot_setAimVector(s, 100, -100)
			fired = 1
		elseif fireDelay > 6.5 and fired == 1 then
			playSfx("Octomun-Shot")
			s = createShot("Octomun", me, n, bone_getWorldPosition(shot2))
			shot_setAimVector(s, 100, -50)
			fired = 2
		elseif fireDelay > 7 and fired == 2 then
			playSfx("Octomun-Shot")
			s = createShot("Octomun", me, n, bone_getWorldPosition(shot3))
			shot_setAimVector(s, 100, 0)
			fired = 3
			--[[
			entity_fireAtTarget(me, "Purple", 1, 500, 200, 0, 0, 0, 0, 100, -100, bx, by)
			entity_fireAtTarget(me, "Purple", 1, 500, 200, 0, 0, 0, 0, 100, -50, bx, by)
			entity_fireAtTarget(me, "Purple", 1, 500, 200, 0, 0, 0, 0, 100, 0, bx, by)
			]]--
			
		elseif fireDelay > 9 and fired == 3 then
			fired = 0
			
			if fireOff == 1 then
				playSfx("rotcore-birth")
				
				bx, by = bone_getWorldPosition(shot1)
				
				e = createEntity("Squiddy", "", bx, by)
				entity_alpha(e, 0.001)
				entity_alpha(e, 1, 0.5)
				
				spawnParticleEffect("tinyredexplode", bx, by)
				fireOff = 0
			else
				fireOff = 1
			end
			
			fireDelay = math.random(2)*0.75 + 0.5
		end
	elseif entity_isState(me, STATE_ATTACK1) then
		tentacle1Collision(me)
	end
	
	if grabbingEntity~=0 then
		mx,my = bone_getWorldPosition(marker1)		
		entity_setPosition(grabbingEntity, mx, my)
		entity_rotate(grabbingEntity, bone_getRotation(marker1))
		if entity_isfh(grabbingEntity) then
			entity_fh(grabbingEntity)
		end
	end
	
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~= 0 then
		entity_damage(n, me, 1)
		entity_pushTarget(me, 500)
	end
	entity_updateMovement(me, dt)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		isWeakPointHittable = false
		entity_animate(me, "idle", -1)
		attackDelay = 0
	elseif entity_isState(me, STATE_ATTACK1) then
		playSfx("Octomun-Growl")
		num = entity_animate(me, "attack1")
		entity_setStateTime(me, num)		
	elseif entity_isState(me, STATE_BASH) then
		entity_setStateTime(me, entity_animate(me, "bash",1))
		if grabbingEntity ~= 0 then
			--entity_push(grabbingEntity, 10, 0, 1)
		end
	elseif entity_isState(me, STATE_DEAD) then
		quad_delete(dark)
	elseif entity_isState(me, STATE_DEATHSCENE) then
		setFlag(FLAG_MINIBOSS_OCTOMUN, 1)
		clearShots()
		entity_stopInterpolating(me)
		entity_setStateTime(me, 99)
		fadeOutMusic(6)
		entity_idle(n)
		entity_setInvincible(n, true)
		cam_toEntity(me)
		entity_setInternalOffset(me, 0, 0)
		entity_setInternalOffset(me, 10, 0, 0.1, -1, 1)
		watch(1)
		playSfx("BossDieSmall")
		fade(1, 0.2, 1, 1, 1)
		watch(0.2)
		fade(0, 0.5, 1, 1, 1)
		watch(0.5)
		watch(1)
		playSfx("BossDieSmall")
		fade(1, 0.2, 1, 1, 1)
		watch(0.2)
		fade(0, 0.5, 1, 1, 1)
		watch(0.5)
		playSfx("BossDieSmall")
		fade(1, 0.2, 1, 1, 1)
		watch(0.2)
		fade(0, 0.5, 1, 1, 1)
		watch(0.5)
		entity_setInternalOffset(me, 0, 0)
		entity_setInternalOffset(me, 20, 0, 0.05, -1, 1)
		playSfx("BossDieBig")
		fade(1, 1, 1, 1, 1)
		watch(1.2)
		fade(0, 0.5, 1, 1, 1)
		
		cam_toEntity(n)
		entity_setInvincible(n, false)
		pickupGem("Boss-Octomun")
		overrideZoom(0, 1)
		entity_setStateTime(me, 0.1)
		entity_setState(me, STATE_DEAD, -1, 1)
	end
end

function exitState(me)
	if entity_isState(me, STATE_ATTACK1) then
		isWeakPointHittable = false
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_BASH) then
		if grabbingEntity ~= 0 then
			entity_idle(grabbingEntity)
			entity_push(grabbingEntity, 1000, 0, 1)
		end
		grabbingEntity = 0
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if bone == weakPoint and isWeakPointHittable then
		bone_damageFlash(body)
		playSfx("Octomun-Hit")
		fireDelay = fireDelay - dmg * 0.2
		return true
	else
		playNoEffect()
	end
	return false
end

function animationKey(me, key)
	if entity_isState(me, STATE_BASH) and key == 4 then
		entity_damage(n, me, 0.75)
	elseif entity_isState(me, STATE_ATTACK1) then
		if key == 1 then
			isWeakPointHittable = true
		end
		if key == 5 then
			isWeakPointHittable = false
		end
		if key == 4 then
			playSfx("rockhit-big")
			shakeCamera(10, 0.8)
		end
	end
end

function lightFlare(me)
	if entity_isEntityInRange(me, n, 1024) then
		quad_alpha(dark, 0, 2)
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

