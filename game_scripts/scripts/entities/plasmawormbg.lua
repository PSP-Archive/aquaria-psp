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
-- P L A S M A W O R M  B G
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

collisionSegs = 64
avoidLerp = 0
avoidDir = 1
interest = false
glow = {}

segs = 64
glowSegInt = 4
glowSegs = segs/glowSegInt

-- initializes the entity
function init(me)
-- oldhealth : 40
	setupBasicEntity(
	me,
	"plasmaworm/tentacles-bg",			-- texture
	9,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	0,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	128,							-- sprite width
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	3000,								-- updateCull -1: disabled, default: 4000	
	1
	)
	entity_setEntityLayer(me, -3)
	
	entity_setDropChance(me, 50)
	
	lungeDelay = 1.0				-- prevent the nautilus from attacking right away

	entity_initHair(me, segs, 16, 64, "plasmaworm/body-bg")
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
	entity_setDeathParticleEffect(me, "Explode")
	
	entity_setDamageTarget(me, DT_ENEMY_TRAP, false)
	
	--entity_fv(me)
	
	entity_setSegs(me, 8, 2, 0.1, 0.9, 0, -0.03, 8, 0)
	
	for i=1,glowSegs do
		glow[i] = createQuad("Naija/LightFormGlow", 13)
		quad_scale(glow[i], 3, 3)
		quad_scale(glow[i], 4, 4, 0.5, -1, 1, 1)
		quad_alpha(glow[i], 0.4)
	end	
	setNormalGlow()

	entity_scale(me, 0.6, 0.6)

	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	
	entity_setEntityType(me, ET_NEUTRAL)
end

function setNormalGlow()
	for i=1,glowSegs do
		quad_color(glow[i], 0.6, 0.6, 0.9)
		quad_color(glow[i], 1, 1, 1, 2, -1, 1, 1)
	end
end

spin = 0
-- the entity's main update function
function update(me, dt)
	entity_rotateToVel(me)
	if colorRevertTimer > 0 then
		colorRevertTimer = colorRevertTimer - dt
		if colorRevertTimer < 0 then
			entity_setColor(me, 1, 1, 1, 3)
			setNormalGlow()
		end
	end
	
	spin = spin - dt
	if spin > 5 then
		-- do something cool!
		msg("SPUN!")
		spin = 0
	end

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
							entity_moveTowardsTarget(me, dt, -500)		-- if we're too close, move away
						elseif not entity_isTargetInRange(me, 300) then
							entity_moveTowardsTarget(me, dt, 1000)		-- move in if we're too far away
						end
						entity_moveAroundTarget(me, dt, 1000+wiggleTime, dir)
					end
				end
			else
				--if entity_isTargetInRange(me, 1600) then
					--entity_moveTowardsTarget(me, dt, -100)		-- if we're too close, move away
				--end
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
			entity_doCollisionAvoidance(me, dt, 10, 0.1)
			entity_doCollisionAvoidance(me, dt, 6, 1.0)
		end
	end
	entity_updateMovement(me, dt)	
	entity_setHairHeadPosition(me, entity_x(me), entity_y(me))
	entity_updateHair(me, dt)
	
	for i=0,glowSegs-1 do		
		x, y = entity_getHairPosition(me, i*glowSegInt)
		--debugLog(string.format("hair(%d, %d)", x, y))
		quad_setPosition(glow[i+1], x, y)
	end	
	--entity_handleShotCollisionsHair(me, collisionSegs)
	--entity_handleShotCollisions(me)
	--if entity_collideHairVsCircle(me, naija, collisionSegs, 0.5) then
		--entity_touchAvatarDamage(me, 0, 0, 500)
	--end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_setMaxSpeed(me, 400)
	elseif entity_isState(me, STATE_DEAD) then
		for i=1,glowSegs do
			quad_delete(glow[i], 0.5)
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

lastNote = 0
function songNote(me, note)
	if getForm()~=FORM_NORMAL then
		return
	end
	
	if note ~= lastNote then
		spin = spin + 0.1
	end
	lastNote = note
	interest = true
	r,g,b = getNoteColor(note)
	entity_setColor(me, r,g,b,1)
	colorRevertTimer = 10
	
	r = r * 0.5 + 0.5
	g = g * 0.5 + 0.5
	b = b * 0.5 + 0.5
	for i=1,glowSegs do
		quad_color(glow[i], r, g, b)
		quad_color(glow[i], 1, 1, 1, 2, -1, 1, 1)
	end
	--entity_setColor(me, 1,1,1,10)
end
