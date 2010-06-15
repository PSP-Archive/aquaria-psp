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

-- Duoeye


dofile("scripts/entities/entityinclude.lua")

-- L O C A L  V A R I A B L E S 


orient = ORIENT_LEFT
orientTimer = 0

swimTime = 0.75
swimTimer = swimTime - swimTime/4

node_mist = 0
eMate = 0
matingTimer = 0
mateCheckDelay = 4
 
-- F U N C T I O N S

function init(me)
	setupBasicEntity(me, 
	"duoeye-head",					-- texture
	6,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	32,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	1,								-- 0/1 hit other entities off/on (uses collideRadius)	
	4000,							-- updateCull -1: disabled, default: 4000
	1
	)
		
	-- entity_initPart(partName, partTexture, partPosition, partFlipH, partFlipV,
	-- partOffsetInterpolateTo, partOffsetInterpolateTime
	entity_initPart(me, 
	"FlipperLeft", 
	"duoeye-flipper",
	-24,
	16,
	0,
	0, 
	0)
	
	entity_initPart(me, 
	"FlipperRight", 
	"duoeye-flipper",
	24,
	16,
	0,
	1,
	0)
	
	entity_partRotate(me, "FlipperLeft", -20, swimTime/2, -1, 1, 1)
	entity_partRotate(me, "FlipperRight", 20, swimTime/2, -1, 1, 1)

	entity_scale(me)
	entity_scale(me, 0.6, 0.6, 0.5)
	
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	entity_setEatType(me, EAT_FILE, "Moneye")
	
	esetv(me, EV_ENTITYDIED, 1)
		
	
	--entity_setMaxSpeed(me, 1000)
end

function postInit(me)
	node_mist = entity_getNearestNode(me, "MIST")
end

-- warning: only called if EV_ENTITYDIED set to 1!
function entityDied(me, ent)
	if eMate == ent then
		eMate = 0
		entity_setState(me, STATE_IDLE)
	end
end

function msg(me, msg, v)
	if msg == "mate" then
		--debugLog("mate msg")
		eMate = v
		entity_setState(me, STATE_MATING)
	end
end

function update(me, dt)
	amt = 400
	
	if entity_isState(me, STATE_MATING) then
		--debugLog(string.format("matingTimer: %d", matingTimer))
		
		if entity_isEntityInRange(me, eMate, 64) then
			--debugLog("in range")
			matingTimer = matingTimer + dt
			if matingTimer > 2 then
				--debugLog("MATED!")
				entity_delete(me)
				entity_delete(eMate)
				ent = createEntity("MoneyeBreeder", "", entity_getPosition(me))
				entity_setState(ent, STATE_GROW)

				eMate = 0
			end
		else
			--debugLog("not in range")
		end
		
		if eMate ~= 0 then
			entity_moveTowards(me, entity_x(eMate), entity_y(eMate), dt, 800)
		end
	else
		if node_mist ~= 0 then
			if node_isEntityIn(node_mist, me) then
				mateCheckDelay = mateCheckDelay - dt
				if mateCheckDelay < 0 then
					eMate = entity_getNearestEntity(me, "Duoeye", 256)
					if eMate ~= 0 and entity_isState(eMate, STATE_IDLE) then
						entity_msg(eMate, "mate", me)
						entity_setState(me, STATE_MATING, 4)					
					end
					mateCheckDelay = 2
				end
			end
		end
		if not entity_hasTarget(me) then
			entity_findTarget(me, 500)
			swimTimer = swimTimer + dt
			if swimTimer > swimTime then	
				swimTimer = swimTimer - swimTime
				if orient == ORIENT_LEFT then
					entity_addVel(me, -amt, 0)
					orient = ORIENT_UP
				elseif orient == ORIENT_UP then
					entity_addVel(me, 0, -amt)
					orient = ORIENT_RIGHT
				elseif orient == ORIENT_RIGHT then
					entity_addVel(me, amt, 0)
					orient = ORIENT_DOWN
				elseif orient == ORIENT_DOWN then
					entity_addVel(me, 0, amt)
					orient = ORIENT_LEFT
				end			
				entity_rotateToVel(me, 0.2)
				orientTimer = orientTimer + dt
				entity_doEntityAvoidance(me, 1, 256, 0.2)
			end
			entity_doCollisionAvoidance(me, dt, 6, 0.5)
		else
			
			swimTimer = swimTimer + dt
			if swimTimer > swimTime then			
				entity_moveTowardsTarget(me, 1, amt)
				if not entity_isNearObstruction(getNaija(), 8) then
					entity_doCollisionAvoidance(me, 1, 6, 0.5)
				end
				entity_doEntityAvoidance(me, 1, 256, 0.2)
				entity_rotateToVel(me, 0.2)
				swimTimer = swimTimer - swimTime
			else
				entity_moveTowardsTarget(me, dt, 100)
				entity_doEntityAvoidance(me, dt, 64, 0.1)
				--if not entity_isNearObstruction(getNaija(), 8) then
				entity_doCollisionAvoidance(me, dt, 6, 0.5)
				--end
			end
			entity_findTarget(me, 800)
		end
	end
	entity_doFriction(me, dt, 600)
	entity_updateCurrents(me, dt)
	entity_updateMovement(me, dt)
	entity_handleShotCollisions(me)
	entity_touchAvatarDamage(me, 16, 1, 1200)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		mateCheckDelay = 3
		matingTimer = 0
		entity_setMaxSpeed(me, 600)
	elseif entity_isState(me, STATE_MATING) then
		entity_offset(me)
		entity_offset(me, 0, 10, 0.05, -1, 1)
	end
end

function exitState(me)
end

function hitSurface(me)
	orient = orient + 1
end