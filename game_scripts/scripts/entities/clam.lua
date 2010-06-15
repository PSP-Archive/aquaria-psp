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

sleepLoc = 0
held = 0
held2 = 0
seen = false

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "Clam")	
	entity_setAllDamageTargets(me, false)
	
	--entity_generateCollisionMask(me)	
	
	entity_setState(me, STATE_IDLE)
	
	entity_setActivation(me, AT_CLICK, 128, 512)
	
	sleepLoc = entity_getBoneByName(me, "SleepLoc")
	bone_alpha(sleepLoc)
	
	loadSound("clam-open")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	if not seen and entity_isEntityInRange(me, n, 700) then
		if chance(50) then
			emote(EMOTE_NAIJAGIGGLE)
		else
			emote(EMOTE_NAIJAWOW)
		end
		seen = true
	end
	if isForm(FORM_BEAST) then
		entity_setActivationType(me, AT_CLICK)
	else
		entity_setActivationType(me, AT_NONE)
	end
	
	--[[
	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	entity_updateMovement(me, dt)
	]]--
	
	if held ~= 0 then
		entity_setPosition(held, bone_getWorldPosition(sleepLoc))
		entity_rotate(held, bone_getWorldRotation(sleepLoc))
		entity_clearVel(held)
	end
	
	if held2 ~= 0 then
		bx, by = bone_getWorldPosition(sleepLoc)
		entity_setPosition(held2, bx-20, by-20)
		entity_rotate(held2, bone_getWorldRotation(sleepLoc))
		entity_clearVel(held2)
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_CLOSE) then
		entity_animate(me, "close")
	elseif entity_isState(me, STATE_OPEN) then
		playSfx("clam-open")
		entity_setStateTime(me, entity_animate(me, "open"))
	end
end

function exitState(me)
	if entity_isState(me, STATE_OPEN) then
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
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
	-- sleep
	
	n = getNaija()
	l = 0
	if hasLi() then
		l = getLi()
	end
	
	esetv(n, EV_LOOKAT, 0)
	
	saveCombat = getFlag(FLAG_LICOMBAT)
	if l ~= 0 then
		fade2(1, 0.5, 1, 1, 1)
		watch(0.5)
		
		setFlag(FLAG_LICOMBAT, 0)
		
		entity_setState(l, STATE_PUPPET)
		
		entity_setPosition(l, entity_x(n), entity_y(n))
		
		fade2(0, 0.5, 1, 1, 1)
	end
		
		
	
	entity_idle(n)
	if entity_isfh(me) and entity_isfh(n) then
		entity_fh(n)
	elseif not entity_isfh(me) and not entity_isfh(n) then
		entity_fh(n)
	end
	entity_clearVel(n)
	watch(0.1)
	x, y = bone_getWorldPosition(sleepLoc)
	entity_setPosition(n, x, y, 1)
	watch(1)
	
	if l ~= 0 then
		entity_clearVel(l)
		entity_setPosition(l, x, y, 1)
	end
	
	held = n
	entity_clearVel(n)
	--entity_rotate(held, bone_getWorldRotation(sleepLoc), 1)
	emote(EMOTE_NAIJASIGH)
	entity_animate(n, "sleep", -1, LAYER_OVERRIDE)
	watch(1)
	
	if l ~= 0 then
		if entity_fh(n) and entity_fh(l) then
			entity_fh(l)
		elseif entity_fh(n) and not entity_fh(l) then
			entity_fh(l)
		end
		
		entity_clearVel(l)
		held2 = l
		--entity_rotate(l, bone_getWorldRotation(sleepLoc), 1)
		entity_animate(l, "sleep", -1)
		watch(1)
	end
	
	entity_setState(me, STATE_CLOSE)
	watch(1.5)
	musicVolume(0.25, 1)
	fade(1, 1)
	watch(1)
	nd = entity_getNearestNode(me, "CLAMCAM")
	if nd ~= 0 then
		cam_toNode(nd)
	end
	watch(1)
	fade(0.5, 1)
	watch(1)
	--entity_heal(n, 20)
	-- do sleep wait for input thing
	while (not isLeftMouse()) and (not isRightMouse()) do		
		watch(FRAME_TIME)
		entity_heal(n, FRAME_TIME)
	end
	
	musicVolume(1, 1)
	fade(0, 2)
	entity_setState(me, STATE_OPEN)
	watch(0.5)
	cam_toEntity(n)
	entity_idle(n)
	entity_animate(n, "slowWakeUp")
	while entity_isAnimating(n) do
		watch(FRAME_TIME)
	end	
	nx, ny = entity_getNormal(me)
	nx,ny = vector_setLength(nx, ny, 200)
	entity_idle(n)
	entity_addVel(n, nx, ny)
	entity_rotateToVel(n)
	entity_animate(n, "burst")
	
	if l ~= 0 then
		entity_setState(l, STATE_IDLE)
		setFlag(FLAG_LICOMBAT, saveCombat)
	end
	
	esetv(n, EV_LOOKAT, 1)
	entity_setInternalOffset(l, 0, 0, 0.5)
	
	
	--entity_animate(n, "wakeUp")
	--watch(1)
	held = 0
	held2 = 0
	
end
