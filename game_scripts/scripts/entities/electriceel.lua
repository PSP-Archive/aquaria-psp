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
-- ELECTRIC EEL
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

songTimer = 0

spawns = 0

shockDelay = 6
STATE_SHOCK			= 1000

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
	-1							-- updateCull -1: disabled, default: 4000
	)
	
	entity_setDropChance(me, 50)
	
	lungeDelay = 1.0				-- prevent the nautilus from attacking right away

	entity_initHair(me, 80, 4, 32, "eel-0002")
	--[[
	entity_initSegments(
	25,								-- num segments
	2,								-- minDist
	12,								-- maxDist
	"wurm-body",						-- body tex
	"wurm-tail",						-- tail tex
	128,								-- width
	128,								-- height
	0.01,							-- taper
	0								-- reverse segment direction
	)
	]]--
	
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
	--entity_setDeathParticleEffect(me, "Explode")
	entity_setDeathScene(me, true)
	
	spawns = 1 + math.random(2)
	
	entity_setCanLeaveWater(me, 1)
end

-- the entity's main update function
function update(me, dt)

	if shockDelay > 0 then
		entity_setState(me, STATE_IDLE)
	end

	if songTimer > 0 then
		songTimer = songTimer - dt*0.4
		--debugLog(string.format("songTimer: %f", songTimer))
	end
	
	if songTimer > 6 then
		songTimer = 3
		if spawns > 0 then
			playSfx("secret")
			spawnIngredient("EelOil", entity_x(me), entity_y(me))
			spawns = spawns - 1
		end
	end

	if colorRevertTimer > 0 then
		colorRevertTimer = colorRevertTimer - dt
		if colorRevertTimer < 0 then
			entity_setColor(me, 1, 1, 1, 3)
		end
	end
	entity_handleShotCollisionsHair(me, collisionSegs)
	--entity_handleShotCollisions(me)
	if entity_collideHairVsCircle(me, naija, collisionSegs) then
		entity_touchAvatarDamage(me, 0, 0, 500)
	end
	-- in idle state only
	if entity_getState(me)==STATE_IDLE or entity_getState(me)==STATE_SHOCK then
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
							entity_moveTowardsTarget(me, dt, -500)		-- if we're too close, move away
						elseif not entity_isTargetInRange(me, 300) then
							entity_moveTowardsTarget(me, dt, 1000)		-- move in if we're too far away
						end
						entity_moveAroundTarget(me, dt, 1000+wiggleTime, dir)
					end
				end
			else
				if entity_isTargetInRange(me, 1600) then
					entity_moveTowardsTarget(me, dt, -100)		-- if we're too close, move away
				end
			end
			
			--[[
			switchDirDelay = switchDirDelay - dt
			if switchDirDelay < 0 then
				switchDirDelay = math.random(800)/100.0
				if dir == 0 then
					dir = 1
				else
					dir = 0
				end
			end
			]]--
			
			--[[
			-- 40% of the time when we're in range and not delaying, launch an attack
			if entity_isTargetInRange(me, 300) then
				if math.random(100) < 40 and lungeDelay == 0 then
					entity_setState(me, STATE_ATTACKPREP, 0.5)
				end
			end
			]]--
			
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
			entity_doCollisionAvoidance(me, dt, 10, 0.5)
			entity_doCollisionAvoidance(me, dt, 4, 0.8)
		end
	end
	entity_updateMovement(me, dt)	
	entity_setHairHeadPosition(me, entity_x(me), entity_y(me))
	entity_updateHair(me, dt)
	
	
	if entity_checkSplash(me) then
		if not entity_isUnderWater(me) then
			entity_setMaxSpeedLerp(me, 2)
			
			if entity_velx(me) < 0 then
				entity_addVel(me, -200, -500)
			else
				entity_addVel(me, 200, -500)
			end
			entity_setWeight(me, 600)
			jumpDelay = 2+math.random(5)
		else
			--entity_setCanLeaveWater(me, false)
			entity_setWeight(me, 0)
		end
	end

	shockDelay = shockDelay - dt
	if shockDelay < 0 then
		shockDelay = 5 + math.random(3)
		entity_setState(me, STATE_SHOCK, 0.5)
	end

end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_setMaxSpeed(me, 400)
	elseif entity_isState(me, STATE_SHOCK) then
		entity_sound(me, "EnergyOrbCharge")
		spawnParticleEffect("JellyShock", entity_x(me), entity_y(me))
	elseif entity_isState(me, STATE_DEATHSCENE) then
		entity_clearVel(me)
		entity_setStateTime(me, 1.2)
		sz = 20.0
		for i=80,0,-sz do
			x, y = entity_getHairPosition(me, i)
			spawnParticleEffect("TinyGreenExplode", x, y, 0.15*((80-i)/sz))
		end		
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

function songNoteDone(me, note, t)
	songTimer = songTimer + t
end


function dieNormal(me)
	if chance(25) then
		spawnIngredient("EelOil", entity_x(me), entity_y(me))
	end
end

