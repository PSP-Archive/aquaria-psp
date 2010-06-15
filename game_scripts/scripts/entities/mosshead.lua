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

n = 0
tongueTarget = 0
tongue = 0
sx=0
sy=0
attackDelay = 1

function init(me)
	setupEntity(me)
	entity_setEntityLayer(me, -2)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "MossHead")
	--[[
	entity_setAllDamageTargets(me, false)
	entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
	entity_setDamageTarget(me, DT_AVATAR_BITE, true)
	]]--
	
	entity_generateCollisionMask(me)
	
	entity_setState(me, STATE_IDLE)
	
	tongueTarget = entity_getBoneByName(me, "TongueTarget")
	tongue = entity_getBoneByName(me, "Tongue")
	bone_alpha(tongueTarget)
	
	entity_scale(me, 1.5, 1.5)
	sx,sy = entity_getScale(me)	
	entity_setCullRadius(me, 1024)
	entity_setUpdateCull(me, 2000)
	
	entity_setDeathScene(me, true)
	entity_setHealth(me, 6)
	
	entity_setEatType(me, EAT_NONE)
	
	loadSound("MossHead")
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	node = entity_getNearestNode(me, "FLIP")
	if node ~=0 then
		if node_isEntityIn(node, me) then
			entity_fh(me)
		end
	end
end

function update(me, dt)
	entity_clearTargetPoints(me)
	if entity_isState(me, STATE_IDLE) then
		attackDelay = attackDelay - dt
		if attackDelay < 0 then
			if entity_isEntityInRange(me, n, 280*sx)
			and not isForm(FORM_FISH) then
				entity_setState(me, STATE_ATTACK)
			end			
		end
	elseif entity_isState(me, STATE_ATTACK) then
		entity_handleShotCollisionsSkeletal(me)
		bone = entity_collideSkeletalVsCircle(me, n)
		-- do damage to avatar
		if bone~= 0 then
			if avatar_isTouchHit() then
				entity_damage(n, me, 0.5, 500)
			end
		end
			
		x,y = bone_getWorldPosition(tongueTarget)
		entity_addTargetPoint(me, x, y)
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_ATTACK) then
		entity_sound(me, "Mosshead", 900+math.random(100))
		entity_setStateTime(me, entity_animate(me, "attack"))
	elseif entity_isState(me, STATE_DEATHSCENE) then		
		entity_setStateTime(me, entity_animate(me, "die"))
	end
end

function exitState(me)
	if entity_isState(me, STATE_ATTACK) then
		attackDelay = 1+math.random(2)
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_ATTACK) then
		if bone == tongue then
			if damageType == DT_AVATAR_BITE then
				entity_changeHealth(me, -6)
			end
			return true
		end
	end
	return false
end

function animationKey(me, key)
end

function hitSurface(me)
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

function dieNormal(me)
	if chance(10) then
		spawnIngredient("VeggieSoup", entity_x(me), entity_y(me))
	else
		if chance(20) then
			spawnIngredient("VeggieCake", entity_x(me), entity_y(me))
		else
			if chance(50) then
				spawnIngredient("Poultice", entity_x(me), entity_y(me))
			end
		end
	end
end

