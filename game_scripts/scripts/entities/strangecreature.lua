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

waitTimeMax = 5
waitTimer = waitTimeMax

notes = { 3, 2, 7, 1 }
numNotes = 4

noteDelay = 0
noteDelayMax = 1

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_NEUTRAL)
	entity_initSkeletal(me, "StrangeCreature")	
	entity_setAllDamageTargets(me, false)

	
	entity_setState(me, STATE_IDLE)
	
	bone_setSegs(entity_getBoneByName(me, "Head"), 2, 16, 0.2, 0.2, -0.03, 0, 6, 1)
	bone_setSegs(entity_getBoneByName(me, "Tentacles"), 2, 16, 0.4, 0.4, -0.01, 0, 6, 0)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	if entity_isState(me, STATE_IDLE) then
		if entity_isEntityInRange(me, n, 256) then
			waitTimer = waitTimer - dt
			if waitTimer <= 0 then
				entity_setState(me, STATE_ON)
			end
		else
			waitTimer = waitTimer + dt
			if waitTimer > waitTimeMax then
				waitTimer = waitTimeMax
			end
		end
	elseif entity_isState(me, STATE_ON) then
		noteDelay = noteDelay - dt
		if noteDelay < 0 then
			playNote = 0
			if curNote == 0 then
				playNote = 3
			elseif curNote == 1 then
				playNote = 2
			elseif curNote == 2 then
				playNote = 7
			elseif curNote == 3 then
				playNote = 1
				entity_setState(me, STATE_IDLE)
			end
			
			h = playSfx(getNoteName(playNote, "low-"))
			fadeSfx(h, 3)
			
			q = createQuad("Particles/BigGlow")
			quad_setBlendType(q, BLEND_ADD)
			quad_scale(q, 0.2, 0.2)
			quad_scale(q, 1, 1, 1)
			quad_color(q, getNoteColor(playNote))
			quad_setPosition(q, entity_x(me)+20, entity_y(me))
			quad_setPosition(q, entity_x(me), entity_y(me)-512, 4)
			quad_delete(q, 4)
			
			curNote = curNote + 1
			if curNote == 3 then
				noteDelay = noteDelayMax*1.2
			else
				noteDelay = noteDelayMax
			end
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		waitTimer = waitTimeMax
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_ON) then
		curNote = 0
	end
end

function exitState(me)
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

