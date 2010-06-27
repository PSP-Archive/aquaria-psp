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

dir = -1

ing = 0

mouthState = 0

MOUT_IDLE		= 0
MOUTH_OPEN	 	= 1
MOUTH_CLOSED	= 2

STATE_SPIT		= 1000

holding = 0

bite = 0

eyeglow = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "grouper")	
	--entity_setAllDamageTargets(me, false)
	
	entity_setCollideRadius(me, 128)
	
	entity_setHealth(me, 24)
	
	--entity_setEntityLayer(me, 1)
	
	entity_offset(me, 0, -40)
	entity_offset(me, 0, 40, 3, -1, 1, 1)
	
	entity_setState(me, STATE_IDLE)
	
	eyeglow = entity_getBoneByName(me, "glow")
	bone_setBlendType(eyeglow, BLEND_ADD)
	bone_alpha(eyeglow, 0)
	
	bite = entity_getBoneByName(me, "bite")
	bone_alpha(bite)
	
	esetv(me, EV_ENTITYDIED, 1)
	
	entity_setCullRadius(me, 800)
	
	loadSound("grouper")
	loadSound("grouper-hurt")
	loadSound("grouper-die")
	
	entity_setDeathSound(me, "grouper-die")
	
	entity_setDeathScene(me, true)
	
	entity_setDeathParticleEffect(me, "tinygreenexplode")
	
	--entity_addVel(me, randVector(500))
end

function doSetRenderPass(me, pass)
	for i=0,5 do if i ~= 1 then bone_setRenderPass(entity_getBoneByIdx(me, i), pass) end end
end

function closeMouth(me)
	entity_stopAllAnimations(me)
	entity_animate(me, "idle", -1)
	entity_animate(me, "close", 0, 1)
	mouthState = MOUTH_IDLE
	
	debugLog("set render pass 0")
	doSetRenderPass(me, 0)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function entityDied(me, theIng)
	if theIng == ing then
		entity_stopAllAnimations(me)
		entity_animate(me, "idle", -1)
		entity_animate(me, "close", 0, 1)
		ing = 0
	end
end

function checkMouth(me)
	bx,by = bone_getWorldPosition(bite)
	if mouthState == MOUTH_OPEN then
		if entity_isPositionInRange(n, bx, by, 96) then
			holding = n
			
			closeMouth(me)
			
						
			entity_setState(me, STATE_WAIT, 2, 1)
			ing = 0
			
			debugLog("set render pass 3")
			doSetRenderPass(me, 3)
		end
	end
end

function update(me, dt)
	entity_updateMovement(me, dt)
	
	--[[
	--entity_addVel(me, dir*100*dt, 0)
	entity_doCollisionAvoidance(me, dt, 32, 0.1)
	entity_doCollisionAvoidance(me, dt, 16, 0.1)
	
	if math.abs(entity_velx(me)) < 100 or math.abs(entity_vely(me)) < 100 then
		entity_addVel(me, randVector(500))
	end
	
	entity_flipToVel(me)
	]]--
	
	bx,by = bone_getWorldPosition(bite)
	
	if entity_isState(me, STATE_IDLE) then
		if ing == 0 then
			ing = entity_getNearestEntity(me, "", 1024, ET_INGREDIENT, 0)
			entity_setMaxSpeedLerp(me, 0.01, 0.2)
			mouthState = MOUTH_IDLE
		else
			entity_doCollisionAvoidance(me, dt, 16, 0.5)
			--debugLog(string.format("ing: %s", entity_getName(ing)))
			if mouthState ~= MOUTH_OPEN then
				spawnParticleEffect("bubble-release", bx, by)
				entity_animate(me, "open", 0, 1)
				mouthState = MOUTH_OPEN
				
				debugLog("set render pass 3")
				doSetRenderPass(me, 3)
				
				entity_sound(me, "grouper")
			end
			
			entity_moveTowards(me, entity_x(ing)-64, entity_y(ing), dt, 2000)
			if entity_isPositionInRange(me, entity_x(ing), entity_y(ing), 128) then
				entity_sound(me, "gulp")
				entity_delete(ing)
				ing = 0
				entity_stopAllAnimations(me)
				entity_animate(me, "idle", -1)
				entity_animate(me, "close", 0, 1)
			end
			entity_setMaxSpeedLerp(me, 2, 0.2)
			checkMouth(me)
		end
	elseif entity_isState(me, STATE_OPEN) then
		checkMouth(me)
	end
	
	if ing == 0 then
		if mouthState == MOUTH_OPEN then
			closeMouth(me)
			entity_setState(me, STATE_IDLE)
		end
	end
	

	
	if holding~=0 then
		entity_setPosition(holding, bx, by)
	end
	
	if holding~=0 and not (entity_isState(me, STATE_WAIT) or entity_isState(me, STATE_SPIT)) then
		entity_setState(me, STATE_SPIT, 1)
	end
	
	if holding == 0 and ing == 0 and mouthState ~= MOUTH_OPEN then
		doSetRenderPass(me, 0)
	end
	
	entity_doCollisionAvoidance(me, dt, 10, 0.5)
	entity_flipToVel(me)
	
	if entity_isState(me, STATE_IDLE) then
		if entity_touchAvatarDamage(me, entity_getCollideRadius(me), 0) then
			if mouthState ~= MOUTH_OPEN and avatar_isBursting() and entity_setBoneLock(n, me) then
				-- yay!
			else
				x, y = entity_getVectorToEntity(me, n, 1000)
				entity_addVel(n, x, y)
			end
		end
	end
	
	entity_handleShotCollisions(me)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_SPIT) then
		bx,by = bone_getWorldPosition(bite)
		spawnParticleEffect("bubble-release", bx, by)
		entity_animate(me, "open2", 0, 1)
		if holding ~= 0 then
			entity_clearVel(holding)
			if entity_isfh(me) then
				entity_push(holding, 5000, 0, 2, 5000, 0.5)
				entity_addVel(holding, 5000, 0)
			else
				entity_push(holding, -5000, 0, 2, 5000, 0.5)
				entity_addVel(holding, -5000, 0)
			end
		end
		holding = 0
	elseif entity_isState(me, STATE_WAIT) then
		entity_setMaxSpeedLerp(me, 0, 2)
	elseif entity_isState(me, STATE_OPEN) then
		if entity_getBoneLockEntity(getNaija()) == me then
			avatar_fallOffWall()
		end
		if mouthState ~= MOUTH_OPEN then
			bx,by = bone_getWorldPosition(bite)
			spawnParticleEffect("bubble-release", bx, by)
			entity_animate(me, "open", 0, 1)
			mouthState = MOUTH_OPEN
			
			debugLog("set render pass 3")
			doSetRenderPass(me, 3)
		end
	elseif entity_isState(me, STATE_DEATHSCENE) then
		entity_scale(me, 0, 0, 2)
		entity_setStateTime(me, 2)
	end
end

function exitState(me)
	if entity_isState(me, STATE_WAIT) then
		entity_setState(me, STATE_SPIT, 1)
	elseif entity_isState(me, STATE_SPIT) then
		closeMouth(me)
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_OPEN) then
		closeMouth(me)
		mouthState = MOUTH_IDLE
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	entity_sound(me, "grouper-hurt")
	n = getNaija()
	entity_setMaxSpeedLerp(me, 2, 0)
	entity_setMaxSpeedLerp(me, 0, 2)
	entity_moveTowards(me, entity_x(n), entity_y(n), 1, -3000)
	return true
end

function animationKey(me, key)
end

function hitSurface(me)
end

function songNote(me, note)
	bone_alpha(eyeglow, 0.5, 1)
	bone_setColor(eyeglow, getNoteColor(note))
end

function songNoteDone(me, note, timer)
	if timer > 1 then
		entity_setState(me, STATE_OPEN, 2)
	end
	bone_alpha(eyeglow, 0, 1)
end

function song(me, song)
end

function activate(me)
end

