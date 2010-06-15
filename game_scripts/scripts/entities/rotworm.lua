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

-- entity specific
STATE_SOMETHING			= 1000
STATE_GOTOHOLE			= 1001
STATE_HIDE				= 1002
STATE_SHOW				= 1003
 
chaseDelay = 0
lastNode = 0
node = 0

holeDelay = 0
holeSpd = 0

nextHoleDelay = 8

segDist = 32
dist = 0

function init(me)
	setupBasicEntity(me, 
	"rotworm/head",					-- texture
	9,								-- health
	1,								-- manaballamount
	2,								-- exp
	1,								-- money
	32,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	128,							-- sprite width
	128,							-- sprite height
	0,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	5000							-- updateCull -1: disabled, default: 4000
	)
	
	--entity_flipVertical(me)			-- fix the head orientation
	
	
	entity_initSegments(me, 
	9,								-- num segments
	0,								-- minDist
	segDist,							-- maxDist
	"rotworm/segment",				-- body tex
	"rotworm/tail",					-- tail tex
	128,							-- width
	128,							-- height
	0.05,							-- taper
	0								-- reverse segment direction
	)
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	entity_setEatType(me, EAT_NONE)
end

function update(me, dt)	
	if entity_isState(me, STATE_HIDE) then
		dist = dist - dt*48
		if dist < 1 then
			dist = 1
			entity_alpha(me, 0, 0.1)
		end
		entity_setSegsMaxDist(me, dist)	
		entity_clearVel(me)
		entity_updateMovement(me, dt)
	end
	
	if not (entity_isState(me, STATE_HIDE) or entity_isState(me, STATE_SHOW)) then
		entity_handleShotCollisions(me)
		if entity_touchAvatarDamage(me, 64, 1, 400) then
			setPoison(1, 16)
		end
		--[[
		if entity_hasTarget(me) then
			if entity_isTargetInRange(me, 64) then
				entity_hurtTarget(me, 1)
				entity_pushTarget(me, 400)
			end
		end
		]]--		
	end
	if chaseDelay > 0 then
		chaseDelay = chaseDelay - dt
		if chaseDelay < 0 then
			chaseDelay = 0
		end
	end
	if entity_isState(me, STATE_IDLE) then
		holeDelay = holeDelay + dt
		if holeDelay > nextHoleDelay then
			holeDelay = 0
			entity_setState(me, STATE_GOTOHOLE)		
		end
	end

	if entity_isState(me, STATE_GOTOHOLE) then
	--[[
		if not entity_isFollowingPath(me) then
			entity_setState(me, STATE_HIDE, 4)
		end
		x=node_x(node)
		y=node_y(node)
		]]--
		if node ~= 0 then
			if entity_isPositionInRange(me, node_x(node), node_y(node), 64) then							
				entity_setState(me, STATE_HIDE, 4+math.random(3))
			else
				holeSpd = holeSpd + 200*dt
				--[[
				if holeSpd > 1500 then
					holeSpd = 1500
				end
				]]--
				x=node_x(node)-entity_x(me)
				y=node_y(node)-entity_y(me)
				x, y = vector_setLength(x, y, holeSpd*dt)

				entity_addVel(me, x, y)
			end
			entity_doCollisionAvoidance(me, dt, 5, 1)
			entity_updateMovement(me, dt)
			--entity_rotateToVel(me, 0.1)
			entity_rotateToVel(me, 0)
		else
			entity_setState(me, STATE_IDLE)
		end
		--entity_rotateToVec(me, x-entity_x(me), y-entity_y(me), 0.1, -180)
	end
	if entity_getState(me)==STATE_IDLE then
		if not entity_hasTarget(me) then
			entity_findTarget(me, 700)
		else
			--if chaseDelay==0 then
			if entity_isTargetInRange(me, 1000) then
				if entity_getHealth(me) < 6 then
					entity_setMaxSpeed(me, 450)
					entity_moveTowardsTarget(me, dt, 1500)
				else
					entity_setMaxSpeed(me, 380)
					entity_moveTowardsTarget(me, dt, 1000)
				end
			else
				entity_setMaxSpeed(me, 100)
			end
			--end
			entity_doEntityAvoidance(me, dt, 200, 0.1)
			if entity_getHealth(me) < 4 then
				entity_doSpellAvoidance(me, dt, 64, 0.5);
			end
			entity_doCollisionAvoidance(me, dt, 5, 1)
			entity_updateMovement(me, dt)
			--entity_rotateToVel(me, 0.1)
			entity_rotateToVel(me, 0)
			--entity_rotate(me, 0)
		end
	end

end

function enterState(me)
	if entity_getState(me)==STATE_IDLE then
		nextHoleDelay = 6 + math.random(6)
		entity_setSegsMaxDist(me, segDist)
		--entity_flipVertical(me)
	elseif entity_isState(me, STATE_GOTOHOLE) then
		holeSpd = 300
		--entity_flipVertical(me)
		if chance(50) then
			node = entity_getNearestNode(getNaija(), "ROTWORM-HOLE")
		else
			node = entity_getNearestNode(me, "ROTWORM-HOLE")
		end
		if node ~= 0 then
			lastHole = node
		end	
		entity_setMaxSpeedLerp(me, 2)
		entity_setStateTime(me, 6+math.random(4))
		--[[
		node = entity_getNearestNode(me, "ROTWORM-HOLE")
		if node ~= 0 then
			lastHole = node
			entity_swimToNode(me, node, SPEED_NORMAL)
		end
		]]--
	elseif entity_isState(me, STATE_HIDE) then
		entity_clearVel(me)
		--entity_alpha(me, 0.0, 1.0)
		dist = segDist
		entity_setPosition(me, node_x(node), node_y(node), 0.2)
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, false)
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, false)
		entity_setDamageTarget(me, DT_AVATAR_PET, false)
		entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	elseif entity_isState(me, STATE_SHOW) then
		node = entity_getNearestNode(me, "ROTWORM-HOLE", lastHole)
		if node ~= 0 then
			entity_setPosition(me, node_x(node), node_y(node))
			entity_warpSegments(me)
		end	
		entity_clearVel(me)
		entity_alpha(me, 1, 0.5)
	end
end

function exitState(me)
	if entity_isState(me, STATE_SHOW) then
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, true)
		entity_setDamageTarget(me, DT_AVATAR_PET, true)
		entity_setDamageTarget(me, DT_AVATAR_LIZAP, true)
		
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_HIDE) then
		entity_setState(me, STATE_SHOW, 0.5)
	elseif entity_isState(me, STATE_GOTOHOLE) then
		entity_setMaxSpeedLerp(me, 1)
		entity_setState(me, STATE_IDLE)
	end
end
