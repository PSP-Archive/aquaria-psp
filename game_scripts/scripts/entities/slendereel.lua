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
-- EEL
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

dir = 0
switchDirDelay = 0
wiggleTime = 0
wiggleDir = 1
interestTimer = 0
colorRevertTimer = 0

collisionSegs = 50
avoidLerp = 0
avoidDir = 1
interest = false
-- initializes the entity
function init(me)
-- oldhealth : 40
	setupBasicEntity(
	me,
	"",						-- texture
	6,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	32,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	90,								-- sprite width
	90,								-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	-1,							-- updateCull -1: disabled, default: 4000
	1
	)
	
	entity_setDropChance(me, 50)
	
	lungeDelay = 1.0				-- prevent the nautilus from attacking right away

	entity_initHair(me, 64, 4, 16, "SlenderEel")
	
	if chance(50) then
		dir = 0
	else
		dir = 1
	end
	
	if chance(50) then
		interest = true
	end
	switchDirDelay = math.random(800)/100.0
	naija = getNaija()
	
	entity_addVel(me, math.random(1000)-500, math.random(1000)-500)
	entity_setDeathParticleEffect(me, "Explode")
	entity_setUpdateCull(me, 2000)
	
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

-- the entity's main update function
function update(me, dt)


	if colorRevertTimer > 0 then
		colorRevertTimer = colorRevertTimer - dt
		if colorRevertTimer < 0 then
			entity_setColor(me, 1, 1, 1, 3)
		end
	end
	entity_handleShotCollisionsHair(me, collisionSegs)
	--entity_handleShotCollisions(me)
	--[[
	if entity_collideHairVsCircle(me, naija, collisionSegs) then
		entity_touchAvatarDamage(me, 0, 0, 500)
	end
	]]--
	-- in idle state only
	if entity_getState(me)==STATE_IDLE then
		-- count down the lungeDelay timer to 0
		if lungeDelay > 0 then lungeDelay = lungeDelay - dt if lungeDelay < 0 then lungeDelay = 0 end end
		
		-- if we don't have a target, find one
		if not entity_hasTarget(me) then
			entity_findTarget(me, 1000)
		else
			wiggleTime = wiggleTime + (dt*200)*wiggleDir
			if wiggleTime > 1000 then
				wiggleDir = -1	
			elseif wiggleTime < 0 then
				wiggleDir = 1
			end
			
			interestTimer = interestTimer - dt
			if interestTimer < 0 then
				if interest then
					interest = false
				else
					interest = true
					entity_addVel(me, math.random(1000)-500, math.random(1000)-500)
				end
				interestTimer = math.random(400.0)/100.0 + 2.0
			end
			if interest then
				if entity_isNearObstruction(getNaija(), 8) then
					interest = false
				else
					if entity_isTargetInRange(me, 1600) then
						if entity_isTargetInRange(me, 100) then
							entity_moveTowardsTarget(me, dt, -500)
						elseif not entity_isTargetInRange(me, 300) then
							entity_moveTowardsTarget(me, dt, 1000)
						end
						entity_moveAroundTarget(me, dt, 1000+wiggleTime, dir)
					end
				end
			else
				if entity_isTargetInRange(me, 1600) then
					entity_moveTowardsTarget(me, dt, -100)
				end
			end
			
			avoidLerp = avoidLerp + dt*avoidDir
			if avoidLerp >= 1 or avoidLerp <= 0 then
				avoidLerp = 0
				if avoidDir == -1 then
					avoidDir = 1
				else
					avoidDir = -1
				end
			end
			-- avoid other things nearby
			entity_doEntityAvoidance(me, dt, 32, 0.1)
--			entity_doSpellAvoidance(dt, 200, 1.5);
			entity_doCollisionAvoidance(me, dt, 10, 0.1)
			entity_doCollisionAvoidance(me, dt, 4, 0.8)
		end
	end
	entity_updateMovement(me, dt)	
	entity_setHairHeadPosition(me, entity_x(me), entity_y(me))
	entity_updateHair(me, dt)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_setMaxSpeed(me, 400)
	end
end

function exitState(me)
end

function hitSurface(me)
end

function damage(me, attacker, bone, damageType, dmg)
	entity_setMaxSpeed(me, 600)
	return true
end

function songNote(me, note)
	if getForm()~=FORM_NORMAL then
		return
	end
	
	interest = true
	r,g,b = getNoteColor(note)
	entity_setColor(me, r,g,b,1)
	colorRevertTimer = 2 + math.random(300)/100.0
	--entity_setColor(me, 1,1,1,10)
end
