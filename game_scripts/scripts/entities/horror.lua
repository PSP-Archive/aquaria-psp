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

STATE_HIDDEN 		= 1000
STATE_REVEAL		= 1001
STATE_FIRE			= 1002

bone_shot = 0
bone_body = 0
bone_whip = 0
fireDelay = 0
soundDelay = 0
whipDeath = false
whipHits = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "Horror")
	entity_setAllDamageTargets(me, true)
	
	entity_generateCollisionMask(me)	
	
	entity_setState(me, STATE_HIDDEN)
	
	entity_setUpdateCull(me, 2000)
	entity_setCullRadius(me, 1024)
	
	bone_shot = entity_getBoneByName(me, "Shot")
	bone_body = entity_getBoneByName(me, "Body")
	bone_whip = entity_getBoneByName(me, "Whip")
	bone_alpha(bone_shot, 0)
	
	entity_setEntityLayer(me, -2)
	
	entity_setHealth(me, 20)
	entity_setDeathScene(me, true)
	entity_setDeathParticleEffect(me, "BigRedExplode")
	
	entity_setDropChance(me, 50, 1)
	
	entity_setEatType(me, EAT_NONE)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	flipNode = entity_getNearestNode(me, "FLIP")
	if flipNode ~= 0 then
		if node_isEntityIn(flipNode, me) then
			entity_fh(me)
		end
	end
end

function update(me, dt)
	
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~= 0 then
		entity_touchAvatarDamage(me, 0, 1)
	end
	
	
	x,y = bone_getWorldPosition(bone_body)
	if entity_isState(me, STATE_HIDDEN) then
		soundDelay = soundDelay +dt
		if soundDelay > 1 then
			entity_sound(me, "Scuttle")
			soundDelay = -math.random(2)
		end
		if entity_y(n) > y and y < entity_y(me) + 1024 then
			spread = 200
			if entity_x(n) > entity_x(me)-spread and entity_x(n) < entity_x(me)+spread then
				entity_setState(me, STATE_REVEAL)
			end
		end
	end
	
	--entity_updateMovement(me, dt)
	
	if entity_isState(me, STATE_IDLE) then
		fireDelay = fireDelay + dt
		if fireDelay > 1.5 then
			fireDelay = -math.random(2)
			entity_setState(me, STATE_FIRE)
		end
	end

	entity_clearTargetPoints(me)
	if not entity_isState(me, STATE_HIDDEN) then
		entity_addTargetPoint(me, x, y)
		x, y = bone_getWorldPosition(bone_whip)
		entity_addTargetPoint(me, x, y)
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		if entity_getAnimationName(me)=="idle" then
		else
			entity_animate(me, "idle", -1)
		end
		--entity_setNaijaReaction(me, "")
	elseif entity_isState(me, STATE_HIDDEN) then
		entity_animate(me, "hidden", -1)
	elseif entity_isState(me, STATE_REVEAL) then
		entity_setStateTime(me, entity_animate(me, "reveal"))
		entity_setNaijaReaction(me, "shock")
		entity_flipToEntity(me, n)
	elseif entity_isState(me, STATE_FIRE) then
		entity_setStateTime(me, entity_animate(me, "fireShots", 0, 1))
	elseif entity_isState(me, STATE_DEATHSCENE) then
		--if whipDeath then
			bone_alpha(bone_whip, 0)
			t = 0
			if chance(50) then
				t = entity_animate(me, "die")
			else
				t = entity_animate(me, "die2")
			end

			x,y = entity_getPosition(me)
			while (isObstructed(x,y) == false) do
				y = y + 20
			end
			entity_setStateTime(me, entity_setPosition(me, x, y, -1000))
			--[[
		else
			entity_setStateTime(me, 0.1)
		end
		]]--
	end
end

function exitState(me)
	if entity_isState(me, STATE_REVEAL) or entity_isState(me, STATE_FIRE) then
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_HIDDEN) then
		entity_setState(me, STATE_REVEAL)
	end
	if bone == bone_whip then
		debugLog("whip hit!")
		whipHits = whipHits + 1
		if damageType == DT_AVATAR_BITE then
			whipHits = whipHits + 2
		end
		bone_damageFlash(bone)
		--debugLog(string.format("whipHits %d", whipHits))
		if whipHits >= 8 then
			--debugLog("whipDeath")
			whipDeath = true
			--entity_adjustHealth(me, -999)
			entity_setHealth(me, 1)
			return true
		end
		return false
	end
	return true
end

function animationKey(me, key)
	if entity_isState(me, STATE_FIRE) then
		if key == 3 or key == 5 or key == 7 then
			x,y = bone_getWorldPosition(bone_shot)
			s = createShot("Horror", me, entity_getTarget(me), x, y)
		end
	end
	--[[
	if entity_isState(me, STATE_DEATHSCENE) then
		if key == 3 or key == 5 or key == 7 then
			
		end
	end
	]]--
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

