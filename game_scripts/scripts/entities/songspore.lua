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
-- SONG SPORE
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")


-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

activeTimer = 0
xdir = -1
dirTimer = 0
glow = 0
shell = 0
spd = 0

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function init(me)
	setupBasicEntity(
	me,
	"",					-- texture
	3,								-- health
	2,								-- manaballamount
	2,								-- exp
	10,								-- money
	16,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	32,							-- sprite width	
	32,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000,							-- updateCull -1: disabled, default: 4000
	1
	)
	
	entity_initSkeletal(me, "SongSpore")
	entity_setEntityType(me, ET_NEUTRAL)
	entity_setDeathParticleEffect(me, "SongSporeExplode")
	--entity_setDeathSound(me, "")
	entity_setState(me, STATE_IDLE)	
	entity_scale(me, 0, 0)
	entity_scale(me, 1, 1, 1)
	glow = entity_getBoneByName(me, "Glow")
	shell = entity_getBoneByName(me, "Shell")
	
	entity_setDeathSound(me, "")
	
	bone_setBlendType(glow, BLEND_ADD)
	
	if chance(50) then
		xdir = 1
	else
		xdir = -1
	end
end

function postInit(me)
end

function songNote(me, note)
	if entity_isState(me, STATE_IDLE) then
		entity_setState(me, STATE_ACTIVE)
	end
	activeTimer = 3.5
	transTime = 0.5
	r,g,b = getNoteColor(note)
	bone_setColor(glow, r,g,b, transTime)
	--[[
	r = (r + 1.0)/2.0
	g = (g + 1.0)/2.0
	b = (b + 1.0)/2.0
	]]--
	bone_setColor(shell, r,g,b, transTime)
end

function update(me, dt)
	if entity_isState(me, STATE_ACTIVE) then
		if activeTimer > 0 then
			activeTimer = activeTimer - dt
			if activeTimer <= 0 then
				entity_setState(me, STATE_IDLE)
			end
		end
	end
	if spd < 100 then
		spd = spd + 40.0*dt
	end
	entity_addVel(me, spd*xdir, -100)
	dirTimer = dirTimer + dt
	if dirTimer > 1.2 then
		dirTimer = 0
		if xdir == 1 then
			xdir = -1
		else
			xdir = 1
		end
		spd = 0
	end
	entity_updateMovement(me, dt)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_offset(me, 0, 0, 2)
		if glow ~= 0 and shell ~= 0 then
			bone_setColor(glow, 1, 1, 1, 2)
			bone_setColor(shell, 1, 1, 1, 2)
			bone_scale(glow, 1, 1, 0.5)
		end
		entity_setMaxSpeed(me, 50)
	elseif entity_isState(me, STATE_ACTIVE) then
		bone_scale(glow, 1.25, 1.25, 0.5, -1, 1)
		entity_offset(me, -5, 0, 0.1, LOOP_INF, 1)
		entity_setMaxSpeed(me, 100)
	end
end

function hitSurface(me)
	entity_damage(me, me, 1000)
end

function damage(me, attacker, bone, damageType, dmg)
	if attacker == me then
		return true
	else
		return false
	end
end

function exitState(me)
end
