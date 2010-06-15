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
-- NUDI
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")
hasShell = true
pullTime = 0.4
escapeTimer = 0
escaping = false
isShell = false
dir = -1
bone_shell = 0
bone_body = 0
moveTimer = 0

 
function commonInit(me, shell)
	hasShell = shell
	
	layer = 1
	if not shell then
		layer = 0
	end
	setupBasicEntity(
	me,
	"",								-- texture
	3,								-- health
	1,								-- manaballamount
	1,								-- exp
	0,								-- money
	20,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	1,								-- 0/1 hit other entities off/on (uses collideRadius)
	2000,							-- updateCull -1: disabled, default: 4000
	layer
	)
	
	entity_initSkeletal(me, "Nudi")
	
	entity_setMaxSpeed(me, 300)
	entity_setEntityType(me, ET_ENEMY)
	
	bone_shell = entity_getBoneByName(me, "Shell")
	bone_body = entity_getBoneByName(me, "Body")
	
	entity_setAllDamageTargets(me, false)
	
	bone_setSegs(bone_body)
	
	if hasShell then
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, false)
		entity_setProperty(me, EP_MOVABLE, true)
	else
		entity_setAllDamageTargets(me, true)
		bone_alpha(bone_shell, 0, 0.1)
		entity_setDeathSound(me, "squishy-die")
	end
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	entity_clampToSurface(me)
	esetv(me, EV_WALLOUT, 16)
	
	entity_setState(me, STATE_IDLE)
end

function update(me, dt)
	entity_handleShotCollisions(me)
	
	if not isShell then
		if hasShell then
			entity_moveAlongSurface(me, dt, 40, 6, 30)
		else
			entity_moveAlongSurface(me, dt, 120, 6, 30)
		end
		entity_rotateToSurfaceNormal(me, 0.2)
		moveTimer = moveTimer + dt
		if moveTimer > 10 then
			entity_switchSurfaceDirection(me)
			moveTimer = 0
		end		
	else
		entity_updateMovement(me, dt)
	end
	
	if hasShell then
		entity_touchAvatarDamage(me, 60, 1, 1200)
	else
		entity_touchAvatarDamage(me, 60, 0, 1200)
	end
	if hasShell then
		if entity_isBeingPulled(me) then
			x, y = entity_getVectorToEntity(getNaija(), me)
			--if not vector_isLength2DIn(x, y, 300) and not escaping then
			if true then
				entity_animate(me, "shellQuiver", LOOP_INF)
				pullTime = pullTime - dt
				if pullTime < 0 then
					entity_animate(me, "idle", LOOP_INF)
					entity_applySurfaceNormalForce(me, 800)
					entity_adjustPositionBySurfaceNormal(me, 64)
					hasShell = false
					isShell = true
					
					avatar_setPullTarget(0)
					--entity_setProperty(me, EP_MOVABLE, false)
					
					bone_alpha(bone_body, 0, 0.1)
					entity_setWeight(me, 300)
					entity_createEntity(me, "NudiNoShell")
					
					playSfx("popshell")
				end
			else
				entity_animate(me, "idle", LOOP_INF)
			end
			--[[
			if hasShell then
				if x ~= 0 or y ~= 0 then
					x, y = vector_setLength(x, y, 2000*dt)
					entity_addVel(getNaija(), x, y)
				end
			end
			]]--
		else
			entity_animate(me, "idle", LOOP_INF)
		end
	end
end

function enterState(me)
	if entity_getState(me) == STATE_IDLE then
		bone_setSegs(bone_body, 4, 4, 0.54, 0.54, -0.022, 0, 4.3, 1)
	end
end

function exitState(me)
end

function hitSurface(me)
end

function damage(me, attacker, bone, damageType, dmg)
	if hasShell or isShell then
		return false
	end
	return true
end

function activate(me)
end

function dieNormal(me)
	if not isShell and not hasShell then
		--50% poultice, 20% healing poultice, 2% sight poultice, 2% leeching poultice
		p = randRange(1, 100)
		if p >= 1 and p <= 2 then
			spawnIngredient("LeechingPoultice", entity_x(me), entity_y(me))
		elseif p >= 3 and p <= 4 then
			spawnIngredient("SightPoultice", entity_x(me), entity_y(me))
		elseif p >= 5 and p <= 25 then
			spawnIngredient("HealingPoultice", entity_x(me), entity_y(me))
		else
			spawnIngredient("LeafPoultice", entity_x(me), entity_y(me))
		end
	end
end

