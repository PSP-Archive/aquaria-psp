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

myNote = 0
timingNote = false
noteTimer = 0
NOTE_TIME = 2.5
core = 0
shell1 = 0
shell2 = 0
glow = 0

myID = 0

STATE_SHAKE		= 1000

function commonInit(me, id)
	-- set color based on note
	setupEntity(me, "", "")
	entity_setEntityType(me, ET_NEUTRAL)
	entity_initSkeletal(me, "HealthUpgrade")
	
	
	core = entity_getBoneByName(me, "Core")
	shell1 = entity_getBoneByName(me, "Shell1")
	shell2 = entity_getBoneByName(me, "Shell2")
	glow = entity_getBoneByName(me, "Glow")
	
	if id == 0 then
		myNote = 0
	elseif id == 1 then
		myNote = 2
	elseif id == 2 then
		myNote = 4
	elseif id == 3 then
		myNote = 6
	elseif id == 4 then
		myNote = 1
	end		
	
	myID = id
	
	bone_setColor(core, getNoteColor(myNote))
	bone_setColor(shell1, getNoteColor(myNote))
	bone_setColor(shell2, getNoteColor(myNote))
	
	bone_setBlendType(glow, BLEND_ADD)
	bone_setColor(glow, getNoteColor(myNote))

	glowNormal(me)
	
	
	entity_scale(me, 0.6, 0.6)
	
	entity_setState(me, STATE_IDLE)
	
	entity_setEntityLayer(me, -1)
end

function postInit(me)
	--if entity_isFlag(me, 1) then
	if isFlag(FLAG_HEALTHUPGRADES + myID, 1) then
		entity_delete(me)
	end
	--end
end

function glowNormal(me)
	bone_alpha(glow, 0.3)
	bone_alpha(glow, 0.4, 1, -1, 1, 1)
	bone_scale(glow, 6, 6)
	bone_scale(glow, 8, 8, 1, -1, 1, 1)
end

function glowSinging(me)
	bone_alpha(glow, 0.5)
	bone_alpha(glow, 0.7, 0.2, -1, 1, 1)

	bone_scale(glow, 6, 6)
	bone_scale(glow, 24, 24, 0.2, -1, 1, 1)
end

incut = false

function update(me, dt)
	if incut then return end
	
	if entity_isState(me, STATE_OPENED) then
		if entity_isEntityInRange(me, getNaija(), 64) then
			incut = true
			playSfx("HealthUpgrade-Collect")
			spawnParticleEffect("HealthUpgradeReceived", entity_getPosition(me))
			setFlag(FLAG_HEALTHUPGRADES + myID, 1)
			upgradeHealth()
			setSceneColor(1, 1, 1, 4)
			entity_idle(getNaija())
			watch(3)
			
			if isFlag(FLAG_FIRSTHEALTHUPGRADE, 0) then
				voice("Naija_HealthUpgrade")
				setFlag(FLAG_FIRSTHEALTHUPGRADE, 1)
			else
				voice("naija_healthupgrade2")
			end
			entity_delete(me)
		end
	elseif entity_isState(me, STATE_OPEN) and not entity_isAnimating(me) then
		entity_setState(me, STATE_OPENED)
	else
		if timingNote then
			noteTimer = noteTimer + dt
			if noteTimer > NOTE_TIME then
				noteTimer = 0
				timingNote = false
				entity_setState(me, STATE_OPEN)
			end
		end
	end
end

function songNote(me, note)
	if entity_getAlpha(me) < 1 then return end
	if entity_isState(me, STATE_OPEN) or entity_isState(me, STATE_OPENED) then return end
	if note == myNote then
		entity_setState(me, STATE_SHAKE)
		timingNote = true
		noteTimer = 0
	else
		timingNote = false
	end
	timer = 0
end

function songNoteDone(me, note, len)
	if entity_isState(me, STATE_OPEN) or entity_isState(me, STATE_OPENED) then return end
	if timingNote and note == myNote then		
		if not entity_isState(me, STATE_OPEN) then
			entity_setState(me, STATE_IDLE)
		end
	end
end

function enterState(me, state)
	--debugLog("HU enterState!")
	if entity_isState(me, STATE_IDLE) then
		if shell1~=0 and shell2 ~= 0 then
			bone_alpha(shell1, 0)
			bone_alpha(shell2, 0)
		end
		timingNote = false
		noteTimer = 0
		entity_animate(me, "idle", LOOP_INF)
		
		glowNormal(me)
	elseif entity_isState(me, STATE_SHAKE) then
		glowSinging(me)
		entity_animate(me, "shake", LOOP_INF)
	elseif entity_isState(me, STATE_OPEN) then
		bone_alpha(core, 0.01, 0.5)
		bone_alpha(shell1, 1, 0.1)
		bone_alpha(shell2, 1, 0.1)
		entity_animate(me, "open")
		
		r, g, b = getNoteColor(myNote)
		setSceneColor(r, g, b, 2)
		
		playSfx(getNoteName(myNote, "low-"))
	end
end

function exitState(me, state)
end

