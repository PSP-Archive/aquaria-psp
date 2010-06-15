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
glow = 0
boneGlow = 0
lungeDelay = 0

STATE_AROUSED		= 1001
STATE_LUNGE			= 1002
hitDelay = 0
lunging = false

function init(me)
	glow = createQuad("Naija/LightFormGlow", 13)
	quad_scale(glow, 3, 3, 1)
	quad_alpha(glow, 1)	
	
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "AnglerFish")	
	--entity_setAllDamageTargets(me, false)
	
	entity_generateCollisionMask(me)	
		
	boneGlow = entity_getBoneByName(me, "Glow")
	
	entity_setCullRadius(me, 30000)
	
	entity_setState(me, STATE_IDLE)
	
	--[[
	entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
	entity_setDamageTarget(me, DT_AVATAR_SHOCK, true)
	
	entity_setDamageTarget(me, DT_AVATAR_VINE, true)
	]]--
	
	
	
	entity_setMaxSpeed(me, 600)
	entity_setEntityLayer(me, 1)
	entity_setCollideRadius(me, 128)
	entity_setBounceType(me, BOUNCE_REAL)
	entity_setBounce(me, 0.5)
	
	entity_setDeathScene(me, true)
	
	entity_setHealth(me, 32)
	
	entity_setDropChance(me, 20, 1)
	
	entity_setUpdateCull(me, 3000)
	
	entity_setNaijaReaction(me, "shock")
	
	--entity_setDeathParticleEffect(me, "BigFishDie")
	
	loadSound("AnglerAwake")
	loadSound("AnglerDie")
	loadSound("AnglerHit")
end

function dieNormal(me)
	if chance(5) then
		spawnIngredient("GlowingEgg", entity_x(me), entity_y(me))
	end
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	if hitDelay > 0 then
		hitDelay = hitDelay - dt
		if hitDelay < 0 then
			hitDelay = 0
		end
	end

	if entity_isState(me, STATE_AROUSED) then
		
		if entity_isTargetInRange(me, 256) then
			entity_moveTowardsTarget(me, dt, -1000)
		else
			entity_moveTowardsTarget(me, dt, 600)
		end
		entity_doCollisionAvoidance(me, dt, 12, 0.5)
		if not entity_isEntityInRange(me, n, 1200) then
			entity_clearVel(me)
			entity_setState(me, STATE_IDLE)
		else
			lungeDelay = lungeDelay + dt 
			if lungeDelay > 3 then
				lungeDelay = 0
				entity_setState(me, STATE_LUNGE)
			end
		end		
	end
	if entity_isState(me, STATE_AROUSED) or entity_isState(me, STATE_LUNGE) then
		entity_doCollisionAvoidance(me, dt, 4, 1.0)
		entity_updateMovement(me, dt)
		
	end
	if entity_isState(me, STATE_LUNGE) then
		if not lunging then
			entity_doFriction(me, dt, 1000)
		else
			entity_moveTowardsTarget(me, dt, 500)
		end

		--[[
		e = getFirstEntity()
		while e ~= 0 do
			if e~=me and not entity_isDead(e) and entity_getEntityType(e) == ET_ENEMY and entity_isDamageTarget(e, DT_AVATAR_BITE) then
				if entity_isEntityInRange(me, e, 96) then
					entity_damage(e, me, 2)
				end
			end
			e = getNextEntity()
		end
		]]--
	end
	if entity_isState(me, STATE_IDLE) then
		if entity_isPositionInRange(n, gx, gy, 200) then
			entity_sound(me, "AnglerAwake", 950 + math.random(100))
			entity_setState(me, STATE_LUNGE)
			
		end
	end
	if math.abs(entity_x(me)-entity_x(n)) > 300 then
		entity_flipToEntity(me, n)
	end
	gx,gy = bone_getWorldPosition(boneGlow)
	quad_setPosition(glow, gx,gy, 0.1)	
	
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~=0 then
		if entity_isState(me, STATE_IDLE) then
			entity_setState(me, STATE_AROUSED)
		end
		if avatar_isTouchHit() then
			entity_damage(n, me, 1)
		end
		entity_pushTarget(me, 500)
	end	
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
		if glow ~= 0 then
			quad_scale(glow, 3, 3, 1)
			quad_alpha(glow, 1)
		end
	elseif entity_isState(me, STATE_AROUSED) then
		entity_setDamageTarget(me, DT_AVATAR_LIZAP, true)
		entity_animate(me, "aroused", -1)
		quad_scale(glow, 9, 9, 0.8)
	elseif entity_isState(me, STATE_LUNGE) then
		entity_setDamageTarget(me, DT_AVATAR_LIZAP, true)
		lunging = false
		quad_scale(glow, 6, 6, 0.8)
		entity_sound(me, "AnglerAwake", 1000 + math.random(100))
		entity_setStateTime(me, entity_animate(me, "lunge"))
		--entity_rotateToEntity(me, n)
	elseif entity_isState(me, STATE_DEATHSCENE) then
		shakeCamera(2, 2)
		playSfx("AnglerDie")
		spawnParticleEffect("BigFishDie", entity_getPosition(me))
		entity_setStateTime(me, entity_animate(me, "die")-0.1)
	elseif entity_isState(me, STATE_DEAD) then
		spawnParticleEffect("BigFishDie", entity_getPosition(me))
		spawnParticleEffect("TinyRedExplode", entity_getPosition(me))
		quad_delete(glow, 3)
	end
end

function exitState(me)
	if entity_isState(me, STATE_LUNGE) then
		entity_setMaxSpeedLerp(me, 0.5)
		entity_setMaxSpeedLerp(me, 1, 1)
		entity_setState(me, STATE_AROUSED)		
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_IDLE) then
		entity_setState(me, STATE_AROUSED, -1, 1)
	end
	if entity_isState(me, STATE_AROUSED) then
		lungeDelay = lungeDelay - 0.1
	end
	--entity_sound(me, "AnglerHit", 1000 + math.random(100))
	return true
end

function animationKey(me, key)
	if entity_isState(me, STATE_LUNGE) then
		if key == 2 then
			lunging = true
			entity_setMaxSpeedLerp(me, 2.0)
			entity_setMaxSpeedLerp(me, 1, 2)
			entity_moveTowardsTarget(me, 1, 8000)
			entity_flipToEntity(me, n)
			--[[
			if entity_isfh(me) then
				entity_rotateToVel(me, 0.1, -90)
			else
				entity_rotateToVel(me, 0.1, 90)
			end
			]]--
		elseif key == 3 then
			
			entity_sound(me, "Bite", 400 + math.random(100))
			entity_moveTowardsTarget(me, 1, 8000)
		end
	end
end

function hitSurface(me)
	if entity_isState(me, STATE_LUNGE) then
		if hitDelay == 0 then
			entity_sound(me, "BigRockHit", 900+math.random(200))
			cx, cy = getLastCollidePosition()
			spawnParticleEffect("Dirt", cx, cy)
			shakeCamera(5, 0.5)
			hitDelay = 0.8
		end
	end
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

