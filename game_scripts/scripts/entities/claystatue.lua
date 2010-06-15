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

head = 0
leftarm = 0
rightarm = 0
legs = 0
feet = 0
chest = 0

b = {}
cb = 1
maxb = 6

dad = 0

soundDelay = 0.4

function init(me)
	setupEntity(me)
	entity_initSkeletal(me, "ClayStatue")	
	entity_setAllDamageTargets(me, false)
		
	b[6] = entity_getBoneByName(me, "Head")
	b[4] = entity_getBoneByName(me, "LeftArm")
	b[5] = entity_getBoneByName(me, "RightArm")
	b[2] = entity_getBoneByName(me, "Legs")
	b[1] = entity_getBoneByName(me, "Feet")
	b[3] = entity_getBoneByName(me, "Chest")
	
	for i=1,maxb,1 do
		bone_alpha(b[i], 0.04)
	end
	
	entity_setState(me, STATE_IDLE)
	
	loadSound("claystatue-crumble")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	if entity_isState(me, STATE_IDLE) then
		if dad == 0 then
			dad = entity_getNearestEntity(me, "SunkenDad")
		else
			--debugLog("checking for dad")
			if entity_isEntityInRange(me, dad, 500) then
				debugLog("dad in range")
				entity_setState(me, STATE_BREAK)
			end
		end
	elseif entity_isState(me, STATE_BREAK) then
		if soundDelay > 0 then
			soundDelay = soundDelay - dt
			if soundDelay <= 0 then
				playSfx("claystatue-crumble")
			end
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_BREAK) then
		
		entity_animate(me, "break")
	end
end

function exitState(me)
end

function msg(me, msg)
	if msg == "p" then
		bone_alpha(b[cb], 1, 1)
		cb = cb + 1
		bone_alpha(b[cb], 1, 1)
		cb = cb + 1
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
end

