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
-- M A U L
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- entity specific
STATE_MAULATTACK 		= 1000
STATE_PULLBACK 			= 1001
STATE_ATTACKPREP		= 1002

n = 0

add = math.random(50)

minCap = 400
maxCap = 700
cap = minCap

deathtimer = 20

cr = 8

lungeDelay = 2

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function init(me)
	setupBasicEntity(
	me,
	"Maul",					-- texture
	6,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	cr,								-- collideRadius 
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000							-- updateCull -1: disabled, default: 4000
	)
	
	deathtimer = deathtimer + math.random(20)
	entity_scale(me, 1.5, 1.5)
	--entity_setDropChance(me, 50)
	--entity_scale(me, 0.9, 0.9)	
end

function update(me, dt)

--[[
	if entity_isState(me, STATE_ATTACKPREP) then
		return
	end
	]]--
	
	if entity_getState(me) == STATE_IDLE then
		cap = cap - dt*400
		if cap < minCap then
			cap = minCap
		end
		if isLeftMouse() then
			cap = maxCap
			add = 600
		end
		--entity_doCollisionAvoidance(me, dt, 4, 0.5)
		entity_doEntityAvoidance(me, dt, 16, 0.5)

		e = entity_getNearestEntity(me, "BigMaul", 1400)
		
		if e ~= 0 then
			x = entity_x(e)
			y = entity_y(e)
			entity_moveTowards(me, x, y, dt, 800+add)
		
			vx = entity_velx(me)
			vy = entity_vely(me)
		
			vx, vy = vector_cap(vx, vy, cap)
			entity_clearVel(me)
			entity_addVel(me, vx, vy)
	
			entity_setPosition(me, entity_x(me) + entity_velx(me)*dt, entity_y(me)+entity_vely(me)*dt)
		else
			deathtimer = deathtimer - dt*2
			entity_setPosition(me, entity_x(me) + entity_velx(me)*dt, entity_y(me)+entity_vely(me)*dt)
		end

		--entity_updateMovement(me, dt)
		entity_rotateToVel(me)

		if lungeDelay > 0 then
			lungeDelay = lungeDelay - dt
		else
			if not entity_hasTarget(me) then
				entity_findTarget(me, 800)
			else
				if entity_isTargetInRange(me, 800) then				
					entity_moveTowardsTarget(me, dt, 400)		-- move in if we're too far away
					if entity_isTargetInRange(me, 350) then
						entity_setState(me, STATE_ATTACKPREP)
					end
				end		
			end
		end
	end

	if entity_getState(me) == STATE_MAULATTACK then
		--debugLog("attacking")
		entity_moveTowardsTarget(me, dt, 500)

		--entity_doEntityAvoidance(me, dt, 256, 0.2)
		entity_doCollisionAvoidance(me, dt, 2, 0.5)
		entity_rotateToVel(me, 0.1)
		entity_updateCurrents(me, dt)
		entity_updateMovement(me, dt)
	end

	if entity_getState(me) == STATE_PULLBACK then
		--debugLog("pulling back")
		if not entity_hasTarget(me) then
			entity_setState(me, STATE_IDLE)
		else
			if entity_isTargetInRange(me, 800) then
				entity_moveTowardsTarget(me, dt, -5000)
			else
				entity_setState(me, STATE_IDLE)
			end
		end

		entity_doEntityAvoidance(me, dt, 256, 0.2)
		
		entity_doCollisionAvoidance(me, dt, 6, 0.5)
		entity_rotateToVel(me, 0.1)
		entity_updateCurrents(me, dt)
		entity_updateMovement(me, dt)
	end
	
	if entity_isState(me, STATE_ATTACKPREP) then
		entity_updateMovement(me, dt)
	end

	
	
	deathtimer = deathtimer - dt
	--debugLog(deathtimer)
	if deathtimer < 1 then
		entity_setState(me, STATE_DEAD)
	end
	
	
	entity_handleShotCollisions(me)
	entity_touchAvatarDamage(me, cr, 0.5, 100)
end

function enterState(me)
	if entity_getState(me)==STATE_IDLE then
		entity_setMaxSpeed(me, 500)
		entity_setMaxSpeedLerp(me, 1, 0.5)
	elseif entity_isState(me, STATE_ATTACKPREP) then
		entity_doGlint(me, "Glint", BLEND_ADD)
		entity_setStateTime(me, 0.5)
		entity_setMaxSpeedLerp(me, 0.2, 0)
	elseif entity_getState(me)==STATE_MAULATTACK then
		entity_setMaxSpeed(me, 800)
		entity_setMaxSpeedLerp(me, 1.1, 0)
		lungeDelay = 1
		entity_moveTowardsTarget(me, 1, 1000)
	elseif entity_getState(me)==STATE_PULLBACK then
		entity_setMaxSpeed(me, 550)
		entity_setMaxSpeedLerp(me, 1, 0.5)
	end
end

function exitState(me)
	if entity_getState(me)==STATE_MAULATTACK then
		entity_setState(me, STATE_PULLBACK, 1)
	elseif entity_getState(me)==STATE_PULLBACK then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_ATTACKPREP) then
		entity_setState(me, STATE_MAULATTACK, 1)
	end
end

function hitSurface(me)
end

function activate(me)
end

function damage(me, attacker, bone, damageType, dmg)
	if damageType == DT_AVATAR_BITE then
		entity_changeHealth(me, -99)
	end
	return true
end