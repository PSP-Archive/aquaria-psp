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
-- M A U L
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- entity specific
dir = 0
myNote = 0
noteDown = -1
n = 0
followDelay = 0
 
-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function commonInit(me, note)
	setupBasicEntity(
	me,
	"",					-- texture
	3,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	16,								-- collideRadius 
	STATE_IDLE,						-- initState
	64,							-- sprite width	
	64,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	-1							-- updateCull -1: disabled, default: 4000
	)
		
	entity_setEntityLayer(me, 1)
	entity_setDropChance(me, 10)
	
	entity_setMaxSpeedLerp(me, 0.5)
	entity_setMaxSpeedLerp(me, 1, 1, -1, 1)
	entity_setSegs(me, 8, 2, 0.1, 0.9, 0, -0.03, 8, 0)
	entity_setDeathParticleEffect(me, "TinyRedExplode")
	myNote = note
	
	
	entity_setBeautyFlip(me, false)
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
	entity_setDamageTarget(me, DT_AVATAR_BITE, false)
	
	entity_initSkeletal(me, "CaveFish")
	glow = entity_getBoneByName(me, "glow")
	bone_setBlendType(glow, BLEND_ADD)
	bone_alpha(glow, 0)
	
	bone_scale(glow, 4, 4)
	bone_scale(glow, 8, 8, 1, -1, 1)
	
	entity_setColor(me, getNoteColor(myNote))
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)	
	entity_handleShotCollisions(me)	
	--entity_touchAvatarDamage(me, 32, 1, 1200)
	
	if noteDown == myNote or followDelay > 0 then
		if noteDown ~= myNote and followDelay > 0 then
			followDelay = followDelay - dt
		end
		entity_moveTowardsTarget(me, dt, 500)
		entity_doCollisionAvoidance(me, dt, 4, 0.5)
		entity_setMaxSpeedLerp(me, 1.2, 0.2)
		--entity_doEntityAvoidance(me, dt, 32, 0.5)
	else
		if dir==0 then
			entity_addVel(me, -500*dt, 0)
		else
			entity_addVel(me, 500*dt, 0)
		end
		entity_setMaxSpeedLerp(me, 1, 0.2)
	end
	entity_doEntityAvoidance(me, dt, 64, 0.1)
	
	
	entity_flipToVel(me)
	if entity_isfh(me) then
		entity_rotateToVel(me, 0, -90)
	else
		entity_rotateToVel(me, 0, 90)
	end	
	
	entity_updateMovement(me, dt)
end

function enterState(me)
end

function exitState(me)
end

function hitSurface(me)
	
	if followDelay <= 0 then
		--debugLog("hit surface!")
		--entity_flipHorizontal(me)
		if dir == 0 then
			--entity_applySurfaceNormalForce(me, 1000)
			dir = 1
		elseif dir == 1 then 
			--entity_applySurfaceNormalForce(me, 1000)
			dir = 0			
		end
		
		entity_clearVel(me)
	end
end

function songNote(me, note)
	noteDown = note
	if note == myNote then
		followDelay = 1.5
		bone_alpha(glow, 0.5, 0.2)
	end
	
end

function songNoteDone(me, note)
	noteDown = -1
	bone_alpha(glow, 0, 3)
end

function activate(me)
end