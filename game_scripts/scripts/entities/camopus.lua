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

head = 0
tback = 0
tfront = 0


STATE_APPEAR			= 1000
STATE_HIDDEN			= 1001
STATE_HIDE				= 1002

inout	= 0
dir 	= 1
spawn = false

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "camopus")
	
	entity_setAllDamageTargets(me, false)
	
	head = entity_getBoneByName(me, "head")
	tfront = entity_getBoneByName(me, "tfront")
	tback = entity_getBoneByName(me, "tback")
	
	bone_setSegs(head, 2, 8, 0.8, 0.8, -0.018, 0, 6, 1)
	bone_setSegs(tfront, 2, 8, 0.8, 0.8, -0.018, 0, 6, 1)
	bone_setSegs(tback, 2, 8, -0.8, -0.8, -0.018, 0, 6, 1)
	
	entity_setCollideRadius(me, 32)
	
	entity_animate(me, "idle", -1)
	
	entity_setState(me, STATE_HIDDEN)
	
	entity_setUpdateCull(me, 3000)
	
	entity_setMaxSpeed(me, 800)
	
	entity_setHealth(me, 20)
	
	entity_setEatType(me, EAT_FILE, "Ink")
	
	entity_setDeathParticleEffect(me, "explode")
	
	entity_setCullRadius(me, 300)
	
	loadSound("camopus-roar")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	if entity_isState(me, STATE_HIDDEN)
	and not isForm(FORM_FISH) then
		if entity_isEntityInRange(me, n, 180) then
			entity_setState(me, STATE_APPEAR)
		end
	end
	
	if entity_isState(me, STATE_IDLE) then
		inout = inout + dt*dir
		if not spawn and inout > 0.2 then
			spawnParticleEffect("bubble-release", entity_x(me), entity_y(me))
			spawn = true
		end
		if inout > 1 then
			dir = -1
			inout = 1
			spawnParticleEffect("bubble-release", entity_x(me), entity_y(me))
		elseif inout < 0 then
			dir = 1
			inout = 0
			spawn = false
			
			if entity_isEntityInRange(me, n, 1000) then
				s = createShot("camopus-ink", me, n, entity_x(me), entity_y(me))
				shot_setAimVector(s, entity_x(n) - entity_x(me), entity_y(n) - entity_y(me))
			end
			
		end
		
		bone_scale(tfront, 0.2*(1-inout)+0.8, 0.9 + inout*0.1)
		bone_scale(tback, 0.2*inout+0.8, 1)
		
		bone_scale(head, 0.9 + 0.1 * (1-inout), 0.2*inout + 0.8)
		
		entity_setMaxSpeedLerp(me, inout*2 + 0.4)
		
		
		
		entity_moveTowardsTarget(me, dt, -400)
		entity_doCollisionAvoidance(me, dt, 10, 0.2)
		entity_doEntityAvoidance(me, dt, 64, 0.1)
		if not entity_isEntityInRange(me, n, 1224) then
			entity_setState(me, STATE_HIDE) 
		end
		
		entity_rotateToVel(me, 0.4)
	end
	
	entity_updateMovement(me, dt)

	entity_handleShotCollisions(me)
	
	entity_touchAvatarDamage(me, 48, 0, 500)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_moveTowardsTarget(me, 1, -800)
		entity_setAllDamageTargets(me, true)
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_APPEAR) then
		spawnParticleEffect("bubble-release", entity_x(me), entity_y(me))
		playSfx("camopus-roar")
		entity_setStateTime(me, 0.3)
		entity_alpha(me, 1, 0.5)
	elseif entity_isState(me, STATE_HIDDEN) then
		entity_setAllDamageTargets(me, false)
		entity_alpha(me, 0.05)
	elseif entity_isState(me, STATE_HIDE) then
		entity_clearVel(me)
		entity_rotate(me, 0, 1, 0, 0, 1)
		entity_alpha(me, 0.05, 3)
		entity_setStateTime(me, 3)
		bone_scale(head, 1, 1, 1)
		bone_scale(tfront, 1, 1, 1)
		bone_scale(tback, 1, 1, 1)
	end
end

function exitState(me)
	if entity_isState(me, STATE_APPEAR) then
		emote(EMOTE_NAIJAUGH)
		entity_setState(me, STATE_IDLE, -1)
	elseif entity_isState(me, STATE_HIDE) then
		entity_setState(me, STATE_HIDDEN)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_HIDDEN) then
		inout = 0.5
		dir = -1
		entity_setState(me, STATE_APPEAR)
	end
	if entity_isState(me, STATE_IDLE) then
		return true
	end
	return false
end

function animationKey(me, key)
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

function dieNormal(me)
	playSfx("camopus-roar")
	cam_toEntity(me)
	
	entity_idle(n)
	watch(0.5)
	
	shakeCamera(2, 3)
	spawnIngredient("RubberyMeat", entity_x(me), entity_y(me))
	spawnIngredient("RubberyMeat", entity_x(me), entity_y(me))
	spawnIngredient("RubberyMeat", entity_x(me), entity_y(me))
	spawnIngredient("RubberyMeat", entity_x(me), entity_y(me))
	if chance(90) then spawnIngredient("SmallTentacle", entity_x(me), entity_y(me)) end
	watch(1)
	cam_toEntity(n)
end

