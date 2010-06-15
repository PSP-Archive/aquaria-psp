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
-- JELLY SMALL
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")


-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

revertTimer = 0
baseSpeed = 150
excitedSpeed = 200
runSpeed = 600
useMaxSpeed = 0
pushed = false
soundDelay = 0
sx = 0
sy = 0
sz = 0.8
transition = false
burstTimer = 0

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function doIdleScale(me)
	entity_scale(me, 0.75*sz, 1*sz)
	entity_scale(me, 1*sz, 0.75*sz, 1.5, -1, 1, 1)
end

function init(me)
	setupBasicEntity(
	me,
	"EvilJelly",						-- texture
	7,							-- health
	2,							-- manaballamount
	2,							-- exp
	10,							-- money
	16,							-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,							-- particle "explosion" type, 0 = none
	0,							-- 0/1 hit other entities off/on (uses collideRadius)
	4000,							-- updateCull -1: disabled, default: 4000
	1
	)
	
	--entity_setRenderPass(me, 3)
		
	entity_setDeathParticleEffect(me, "TinyRedExplode")
	useMaxSpeed = baseSpeed
	entity_setEntityType(me, ET_ENEMY)
	
	entity_setPauseInConversation(me, false)

	entity_setState(me, STATE_IDLE)
	entity_setDropChance(me, 10, 1)
	
	entity_initStrands(me, 5, 16, 8, 5, 1, 0.8, 0.8)
	
	doIdleScale(me)
	soundDelay = math.random(3)
	sx, sy = entity_getScale(me)
	
	--entity_setColor(me, 1, 0.5, 0.75)
	
	entity_setMaxSpeed(me, excitedSpeed)
	entity_setEatType(me, EAT_FILE, "MiniFood")
	
	entity_addIgnoreShotDamageType(me, DT_AVATAR_BITE)
	entity_addIgnoreShotDamageType(me, DT_AVATAR_VINE)
end

function songNote(me, note)
end

function update(me, dt)
	entity_touchAvatarDamage(me, 16, 1, 1000)
	burstTimer = burstTimer + dt
	if burstTimer > 1 then
		entity_setMaxSpeedLerp(me, 1)
		entity_setMaxSpeedLerp(me, 6, 0.5	, 1, 1)
		burstTimer = 0
	end
	avoid = false
	if not avatar_isOnWall() and entity_isNearObstruction(getNaija(), 5) then
		avoid = true
	end
	if entity_isState(me, STATE_IDLE) and not transition and not entity_isScaling(me) then
		entity_scale(me, 0.75*sz, 1*sz, 0.2)
		transition = true
	end
	if transition then
		if not entity_isScaling(me) then
			doIdleScale(me)
			transition = false
		end
	end
	entity_handleShotCollisions(me)
	entity_findTarget(me, 1024)
	if not entity_hasTarget(me) then
		--entity_doCollisionAvoidance(me, dt, 4, 0.1)		
	end
	
	if revertTimer > 0 then
		revertTimer = revertTimer - dt
		if revertTimer < 0 then
			useMaxSpeed = baseSpeed
			entity_setMaxSpeed(me, baseSpeed)
		end
	end
	-- cheap hack
	if not avatar_isBursting() then
		entity_doEntityAvoidance(me, dt, 64, 0.8)
	end
	if entity_hasTarget(me) and not avoid then			
		if entity_isTargetInRange(me, 1000) then
			if not entity_isTargetInRange(me, 64) then				
				entity_moveTowardsTarget(me, dt, 1000)
			end
		end
	end
	
	entity_doCollisionAvoidance(me, dt, 3, 1.0)
	
	entity_doSpellAvoidance(me, dt, 200, 0.8)
	
	entity_updateCurrents(me, dt*5)
	
	entity_rotateToVel(me, 0.1)
	entity_updateMovement(me, dt)	
end

function hitSurface(me)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		useMaxSpeed = baseSpeed
		entity_setMaxSpeed(me, baseSpeed)
		entity_animate(me, "idle", LOOP_INF)
		
		x = math.random(2000)-1000
		y = math.random(2000)-1000
		entity_addVel(me,x,y)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if damageType == DT_AVATAR_BITE then
		entity_setHealth(me, 0)
	end
	return true
end

function exitState(me)
end
