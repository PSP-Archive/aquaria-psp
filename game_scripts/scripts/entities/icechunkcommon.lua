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
-- I C E   C H U N K   C O M M O N   S C R I P T
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- L O C A L   V A R I A B L E S
-- ================================================================================================

n = 0

maxSpeed = 321 + math.random(32)
chunkSize = 0
width = 0
dir = -1
 
-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function commonInit(me, size)
	chunkSize = size

	setupBasicEntity(
	me,
	"IceChunk/Large",				-- texture
	8,								-- health
	1,								-- manaballamount
	1,								-- exp
	0,								-- money
	64,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	512,							-- sprite width	
	512,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	1,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000,							-- updateCull -1: disabled, default: 4000
	layer
	)
	
	loadSound("IceChunkBreak")
	
	entity_setEntityType(me, ET_NEUTRAL)
	--entity_setAllDamageTargets(me, false)
	--entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	entity_setDamageTarget(me, DT_AVATAR_BITE, false)
	
	entity_setDeathScene(me, true)
	
	-- SLIGHT SCALE AND COLOUR VARIATION
	sz = 0.8 + (math.random(400) * 0.001)
	entity_scale(me, sz, sz)
	cl = 1.0 - (math.random(2345) * 0.0001)
	entity_color(me, cl, cl, cl)
	
	-- IF LARGE
	if chunkSize <= 0 then
		chunkSize = 0
		entity_setTexture(me, "IceChunk/Large")
		width = 154 
		entity_setHealth(me, 8)
	
	-- IF MEDIUM
	elseif chunkSize == 1 then
		chunkSize = 1
		entity_setTexture(me, "IceChunk/Medium")
		width = 76
		entity_setHealth(me, 4)
		maxSpeed = maxSpeed * 1.23
	
	-- IF SMALL
	else
		chunkSize = 2
		entity_setTexture(me, "IceChunk/Small")
		width = 42
		entity_setHealth(me, 2)
		maxSpeed = maxSpeed * 1.54
	end
	
	width = width * sz
	entity_setCollideRadius(me, width)
	entity_setDeathSound(me, "")
	entity_alpha(me, 0.9)
end

function postInit(me)
	n = getNaija()

	entity_setMaxSpeed(me, maxSpeed)
	entity_rotate(me, randAngle360())
	entity_addRandomVel(me, 123)
	
	if chance(50) then dir = 1 end
end

function update(me, dt)
	entity_clearTargetPoints(me)
	
	-- ROTATE GENTLY
	rotSpeed = (entity_getVelLen(me)/300) + 1
	if entity_velx(me) < 0 then dir = -1
	else dir = 1 end
	entity_rotateTo(me, entity_getRotation(me) + (rotSpeed * dir))
	
	-- IF LARGE
	if chunkSize == 0 then
		-- LOCK ON TO BIG CHUNK
		if entity_touchAvatarDamage(me, width*1.1, 0) then
			if avatar_isBursting() and entity_setBoneLock(n, me) then
			else
				vecX, vecY = entity_getVectorToEntity(me, n, 1000)
				entity_addVel(n, vecX, vecY)
			end
		end
		
		if entity_getBoneLockEntity(n) ~= me and entity_touchAvatarDamage(me, width, 0, 321) then
		end
	
	-- IF MEDIUM
	elseif chunkSize == 1 then
		-- NAIJA COLLISION
		if entity_getBoneLockEntity(n) ~= me and entity_touchAvatarDamage(me, width, 0, 210) then
			if avatar_isBursting() then
				entity_moveTowards(me, entity_x(getNaija()), entity_y(getNaija()), 1, -456)
			else
				entity_moveTowards(me, entity_x(getNaija()), entity_y(getNaija()), 1, -87)
			end
		end
	
	-- IF SMALL
	elseif chunkSize == 2 then
		-- NAIJA COLLISION
		if entity_getBoneLockEntity(n) ~= me and entity_touchAvatarDamage(me, width, 0, 128) then
			if avatar_isBursting() then
				entity_moveTowards(me, entity_x(getNaija()), entity_y(getNaija()), 1, -1234)
			else
				entity_moveTowards(me, entity_x(getNaija()), entity_y(getNaija()), 1, -128)
			end
		end
	end

	-- AVOIDANCE
	if entity_getBoneLockEntity(n) ~= me then entity_doEntityAvoidance(me, dt, width*1.1, 1.23) end
	entity_doCollisionAvoidance(me, dt, ((width*0.01)*7)+1, 0.421)
	-- MOVEMENT
	if entity_getVelLen(me) > 64 then entity_doFriction(me, dt, 42) end
	entity_updateMovement(me, dt)
	-- SHOT COLLISIONS
	entity_handleShotCollisions(me)
end

function enterState(me)
	if entity_getState(me) == STATE_IDLE then
	
	elseif entity_getState(me) == STATE_DEATHSCENE then
		deathTime = 0.23
		entity_setStateTime(me, deathTime)
		entity_alpha(me, 0, deathTime)
		entity_scale(me, 0, 0, deathTime)
		entity_delete(me, deathTime)
		
		entity_sound(me, "IceChunkBreak")
		
		if chunkSize ~= 2 then
			for i=1,3 do
				if chunkSize == 0 then
					newChunk = createEntity("IceChunkMedium", "", entity_x(me) + (math.random(8) - 4), entity_y(me) + (math.random(8) - 4))
				else
					newChunk = createEntity("IceChunkSmall", "", entity_x(me) + (math.random(4) - 2), entity_y(me) + (math.random(4) - 2))
				end
				entity_moveTowards(newChunk, entity_x(me), entity_y(me), 1, -321)
			end
		end
	end
end

function exitState(me)
end

function hitSurface(me)
	
	
end

function damage(me, attacker, bone, damageType, dmg)
	return true
end

function activate(me)
end
