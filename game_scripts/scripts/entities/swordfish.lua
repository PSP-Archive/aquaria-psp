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
attackDelay = 0
attacked = 0
aggro = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "Swordfish")
	
	entity_generateCollisionMask(me)	
	
	entity_setState(me, STATE_IDLE)
	
	entity_setCanLeaveWater(me, true)
	
	entity_setCollideRadius(me, 128)
	entity_setUpdateCull(me, 4000)
	entity_setDeathParticleEffect(me, "TinyBlueExplode")
	
	entity_setCullRadius(me, 256)
	
	entity_setHealth(me, 12)
	
	loadSound("swordfish-attack")
	loadSound("swordfish-die")
	
	entity_setDeathSound(me, "swordfish-die")
end

function postInit(me)
	n = getNaija()
	---entity_setTarget(me, n)
end

t = 0
function rotflip(me)
	
	entity_flipToVel(me)
	if entity_isfh(me) then
		entity_rotateToVel(me, t, -90)
	else
		entity_rotateToVel(me, t, 90)
	end
	
end

function update(me, dt)

	if not isForm(FORM_FISH) then
		aggro = 1
	end

	if attacked > 0 then
		attacked = attacked - dt
		if attacked < 0 then
			attacked = 0
		end
	end
	
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~= 0 then
		entity_damage(n, me, 1)
		x, y = entity_getVectorToEntity(me, n)
		entity_addVel(n, x*500, y*500)
	end
	if entity_isState(me, STATE_ATTACK) then
		if entity_isEntityInRange(me, n, 800) then
			entity_moveTowardsTarget(me, dt, 800)
		end
	end
	if entity_isState(me, STATE_CHARGE1) then
		
	end
	if entity_isState(me, STATE_IDLE) then
		if entity_hasTarget(me) then
			entity_moveTowardsTarget(me, dt, 100)
			if entity_isUnderWater(me) then
				if attacked > 0 or entity_isEntityInRange(me, n, 512) then
					attackDelay = attackDelay - dt
					if attackDelay <= 0
					and aggro == 1 then
						--[[
						entity_moveTowardsTarget(me, 1, 1000)
						entity_flipToEntity(me, n)
						rotflip(me)
						entity_clearVel(me)
						]]--
						entity_moveTowardsTarget(me, 1, -100)
						
						entity_setState(me, STATE_CHARGE1)
						--[[
						x, y = entity_getNormal(me)
						sw = x
						x = -y
						y = sw
						len = -1000
						x = x * len
						y = y * len
						if entity_collideCircleVsLine(n, entity_x(me), entity_y(me), entity_x(me)+x, entity_y(me)+y, 128) then
							entity_setState(me, STATE_CHARGE1)
						end
						]]--
					end
				end
			end
			entity_doCollisionAvoidance(me, dt, 8, 0.1)
			entity_doCollisionAvoidance(me, dt, 4, 0.5)
			entity_doEntityAvoidance(me, dt, 32, 0.2)
			--entity_doSpellAvoidance(me, dt, 256, 0.2)
			--entity_doSpellAvoidance(me, dt, 128, 0.8)
			entity_findTarget(me, 2500)
		else
			entity_findTarget(me, 1000)
		end
	end
	if not entity_isState(me, STATE_CHARGE1) then
		rotflip(me)
	end
	if entity_isState(me, STATE_CHARGE1) then
		vx = entity_velx(me)
		vy = entity_vely(me)
		entity_clearVel(me)
		entity_moveTowardsTarget(me, 1, 1000)
		rotflip(me)
		entity_clearVel(me)
		entity_addVel(me, vx, vy)
	end
	--entity_flipToVel(me)
	entity_updateMovement(me, dt)
	if entity_checkSplash(me) then
		if not entity_isUnderWater(me) then
			entity_setMaxSpeedLerp(me, 4)
			
			if entity_velx(me) < 0 then
				entity_addVel(me, -300, -500)
			else
				entity_addVel(me, 300, -500)
			end
			entity_setWeight(me, 650)
			jumpDelay = 2+math.random(5)
		else
			--entity_setCanLeaveWater(me, false)
			entity_setWeight(me, 0)
		end
	end	
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_CHARGE1) then
		entity_sound(me, "swordfish-attack")
		entity_moveTowardsTarget(me, 1, -1000)
		--entity_flipToEntity(me, n)
		entity_setMaxSpeedLerp(me, 0.75)
		entity_setMaxSpeedLerp(me, 0.5, 0.2)
		entity_setStateTime(me, entity_animate(me, "attackPrep"))
		entity_doGlint(me, "Glint", BLEND_ADD)
		--entity_fv(me)
	elseif entity_isState(me, STATE_ATTACK) then
		--entity_fv(me)
		entity_setMaxSpeedLerp(me, 4)
		entity_setStateTime(me, entity_animate(me, "attack"))
		x, y = entity_getNormal(me)
		sw = x
		x = -y
		y = sw
		spd = 1000
		entity_moveTowardsTarget(me, 1, 3000)
		rotflip(me)
	end
end

function exitState(me)
	if entity_isState(me, STATE_CHARGE1) then
		entity_setState(me, STATE_ATTACK)
	elseif entity_isState(me, STATE_ATTACK) then
		attackDelay = math.random(3)+2
		entity_setState(me, STATE_IDLE)
		entity_setMaxSpeedLerp(me, 1, 1)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	attackDelay = attackDelay - dmg * 2
	
	attacked = 1
	aggro = 1
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

function dieNormal(me)
	if chance(50) then
		spawnIngredient("SwordfishSteak", entity_x(me), entity_y(me))
	else
		spawnIngredient("FishMeat", entity_x(me), entity_y(me))
	end
end

