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
-- PHONOGRAPH
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")


-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

activeTimer = 0
noteToPlay = 0
noteDelay = 0

singer = true

singTimer = 0

bubbles = 0
body = 0

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function init(me)
	setupBasicEntity(
	me,
	"",								-- texture
	3,								-- health
	2,								-- manaballamount
	2,								-- exp
	10,								-- money
	16,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	256,							-- sprite width	
	256,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000,							-- updateCull -1: disabled, default: 4000
	0
	)
	
	entity_initSkeletal(me, "Phonograph")
	
	entity_setEntityType(me, ET_NEUTRAL)
	
	entity_animate(me, "idle", -1)
	
	bubbles = entity_getBoneByName(me, "Bubbles")
	body = entity_getBoneByName(me, "Body")
	
	
	
	--[[
	for i=0,7 do
		loadSound(getNoteName(i, "low-"))
	end
	]]--
end

function postInit(me)
	if singer then
		e = getFirstEntity()
		while e ~= 0 do
			if e ~= me and entity_isName(e, entity_getName(me)) and entity_isEntityInRange(me, e, 1024) then	
				entity_msg(e, "no-sing")
			end
			e = getNextEntity()
		end
	end
end

function songNote(me, note)
	if entity_isEntityInRange(me, getNaija(), 600) then
		noteToPlay = note
		noteDelay = 0.2
	end
end

function songNoteDone(me, note)
end

function update(me, dt)
	if noteDelay > 0 then
		noteDelay = noteDelay - dt
		if noteDelay < 0 then
			if singer then
				entity_sound(me, getNoteName(noteToPlay, "low-"), 1, 2)
			end
			if activeTimer == 0 then
				debugLog("bone segs")
				bone_setSegs(body, 2, 8, 0.7, 0.1, -0.018, 0, 10, 1)
			end
			activeTimer = 2
		end
	end
	if activeTimer > 0 then
		singTimer = singTimer + dt
		if singTimer > 0.8 then
			x,y = bone_getWorldPosition(bubbles)
			spawnParticleEffect("bubble-release", x, y)
			singTimer = 0
		end
		activeTimer = activeTimer - dt
		if activeTimer < 0 then
			activeTimer = 0
			bone_setSegs(body, 2, 8, 0.7, 0.1, -0.018, 0, 2, 1)
		end
	end
	if activeTimer == 0 then
		singTimer = singTimer - dt * 2
		if singTimer < 0 then
			singTimer = 0
		end
	end
end

function enterState(me)
end

function hitSurface(me)
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function exitState(me)
end

function msg(me, msg)
	if msg == "no-sing" then
		debugLog("no sing")
		singer = false
	end
end

