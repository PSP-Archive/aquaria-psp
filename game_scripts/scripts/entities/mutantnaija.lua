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

STATE_SWIM			= 1000
STATE_BURST			= 1001

burstDelay 			= 0

singDelay 			= 4

fireDelay 			= 3
fired				= 0
fireShotDelay		= 0

function idle(me)
	entity_setState(me, STATE_IDLE, math.random(1)+0.5)
end

function doBurstDelay()
	burstDelay = math.random(4) + 2
end

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "Naija", "Mutant")	
	--entity_setAllDamageTargets(me, false)
	
	entity_scale(me, 0.7, 0.7)
	
	entity_setCollideRadius(me, 20)
	entity_setHealth(me, 20)
	
	
	bone_alpha(entity_getBoneByName(me, "Fish2"),0)
	bone_alpha(entity_getBoneByName(me, "DualFormGlow"),0)
	
	head = entity_getBoneByName(me, "Head")
	
	entity_setDeathScene(me, true)
	
	idle(me)
	
	entity_setUpdateCull(me, 1300)
	
	loadSound("mutantnaija-note")
	loadSound("mutantnaija-note-hit")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	entity_updateMovement(me, dt)
	
	entity_handleShotCollisions(me)
	
	entity_setLookAtPoint(me, bone_getWorldPosition(head))
	
	if entity_isState(me, STATE_IDLE) then
		entity_rotate(me, 0, 0.1)
	elseif entity_isState(me, STATE_SWIM) then
		entity_moveTowardsTarget(me, dt, 500)
		burstDelay = burstDelay - dt
		if burstDelay < 0 then
			doBurstDelay()
			entity_setMaxSpeedLerp(me, 2)
			entity_setMaxSpeedLerp(me, 1, 4)
			entity_moveTowardsTarget(me, 1, 1000)
			entity_animate(me, "burst", 0, 1)
		end
		entity_rotateToVel(me, 0.1)
	end
	entity_doEntityAvoidance(me, dt, 32, 0.5)
	entity_doCollisionAvoidance(me, dt, 4, 0.5)
	
	--[[
	singDelay = singDelay - dt
	if singDelay < 0 then
		singDelay = math.random(3) + 3
		entity_sound(me, getNoteName(math.random(8)), 1, 3)
		x = entity_x(n) - entity_x(me)
		y = entity_y(n) - entity_y(me)
		x, y = vector_setLength(x, y, 1000)
		entity_addVel(n, x, y)
	end
	]]--
	
	if entity_isState(me, STATE_SWIM) then
		if fired == -1 then
			fireDelay = fireDelay - dt
			if fireDelay < 0 then
				fired = 3
				fireDelay = math.random(2) + 3
				fireShotDelay = 0
			end
		end
	end
	
	if fired > -1 then
		fireShotDelay = fireShotDelay - dt
		if fireShotDelay < 0 then
			s = createShot("MutantNaija", me, n, bone_getWorldPosition(head))
			fired = fired - 1
			if fired == 0 then
				fired = -1 
			end
			fireShotDelay = 0.2
		end
	end
	
	thresh = 10
	if entity_velx(me) > thresh and not entity_isfh(me) then
		entity_fh(me)
	end
	if entity_velx(me) < -thresh and entity_isfh(me) then
		entity_fh(me)
	end
	
	entity_touchAvatarDamage(me, 8, 1, 500)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_SWIM) then
		entity_animate(me, "swim", -1)
	elseif entity_isState(me, STATE_DEATHSCENE) then
		entity_setColor(me, 0.3, 0.3, 0.3, 1)
		entity_setPosition(me, entity_x(me), entity_y(me)+400, -300)
		entity_setStateTime(me, entity_animate(me, "diePainfully", 2))
		entity_rotate(me, 0, 0.1)
	end
end

function exitState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_rotate(me, 0, 0.1)
		entity_setState(me, STATE_SWIM, math.random(6)+4)
	elseif entity_isState(me, STATE_SWIM) then
		idle(me)
		doBurstDelay()
	elseif entity_isState(me, STATE_DEATHSCENE) then
		spawnParticleEffect("TinyBlueExplode", entity_getPosition(me))
	end
end

function damage(me, attacker, bone, damageType, dmg)
	return true
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

