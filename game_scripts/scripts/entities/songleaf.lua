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
-- S O N G  L E A F
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")


-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

activeTimer = 0
singing = false
singTimer = 0

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
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000,							-- updateCull -1: disabled, default: 4000
	0
	)
	
	entity_setEntityType(me, ET_NEUTRAL)
	
	entity_initSkeletal(me, "SongLeaf")
	leaf1 = entity_getBoneByName(me, "Leaf1")
	leaf2 = entity_getBoneByName(me, "Leaf2")
	leaf3 = entity_getBoneByName(me, "Leaf3")

	entity_setEntityLayer(me, -2)

	scale_random = math.random(40) * 0.01
	entity_scale(me, 0.6 + scale_random, 0.6 + scale_random)

	entity_setState(me, STATE_IDLE)
	
	entity_setCull(me, false)
end

function songNote(me, note)
	singing = true
	if entity_isState(me, STATE_IDLE) then
		entity_setState(me, STATE_ACTIVE)
	end
	activeTimer = 99
	transTime = 0.5
	r,g,b = getNoteColor(note)
	r = r*0.75 + 0.25
	g = g*0.75 + 0.25
	b = b*0.75 + 0.25
	bone_setColor(leaf1, r,g,b, transTime)
	bone_setColor(leaf2, r,g,b, transTime)
	bone_setColor(leaf3, r,g,b, transTime)

	bone_setSegs(leaf1, 2, 8, 0.8, 0.8, -0.04, 0, 24, 1)
	bone_setSegs(leaf2, 2, 8, 0.8, 0.8, -0.04, 0, 24, 1)
	bone_setSegs(leaf3, 2, 8, 0.8, 0.8, -0.04, 0, 24, 1)
end

function songNoteDone(me, note)
	activeTimer = 2.5
	singing = false
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
	if singing then
		singTimer = singTimer + dt
		if singTimer > 3 then
			spawnParticleEffect("bubble-release", entity_x(me), entity_y(me)-16)
			singTimer = 1 - math.random(2)
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", LOOP_INF)
		--bone_setSegs(bulb, 2, 8, 0.8, 0.1, -0.018, 0, 6, 1)
		bone_setColor(leaf1, 1, 1, 1, 3)
		bone_setColor(leaf2, 1, 1, 1, 3)
		bone_setColor(leaf3, 1, 1, 1, 3)
			
		bone_setSegs(leaf1, 2, 8, 0.8, 0.8, -0.018, 0, 6, 1)
		bone_setSegs(leaf2, 2, 8, 0.8, 0.8, -0.018, 0, 6, 1)
		bone_setSegs(leaf3, 2, 8, 0.8, 0.8, -0.018, 0, 6, 1)
	elseif entity_isState(me, STATE_ACTIVE) then
		--bone_setSegs(bulb, 2, 8, 0.8, 0.1, -0.018, 0, 20, 1)
		entity_animate(me, "wave", LOOP_INF)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function exitState(me)
end
