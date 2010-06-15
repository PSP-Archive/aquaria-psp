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
died = false
myNote = -1
singing = false

bone_note = 0

darkli = 0

singTimer = 0

startDelay = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "darklishot")
	entity_setAllDamageTargets(me, false)
	
	entity_setHealth(me, 1)
	
	entity_setState(me, STATE_IDLE)
	
	entity_setDeathScene(me, true)
	
	entity_setMaxSpeed(me, 400)
	
	bone_note = entity_getBoneByIdx(me, 1)
	
	esetv(me, EV_TYPEID, EVT_DARKLISHOT)
	
	darkli = entity_getNearestEntity(me, "creatorform5")
	
	entity_setCollideRadius(me, 96)
	
	entity_scale(me, 0.8, 0.8)
	
	entity_alpha(me, 0)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
	if startDelay > 0 then
		startDelay = startDelay - dt
		
		if startDelay < 0 then
			entity_alpha(me, 1, 0.5)
			spawnParticleEffect("darklishot-spawn", entity_x(me), entity_y(me))
			entity_sound(me, getNoteName(myNote, "low-"), 1, 2)
			playSfx("hellbeast-shot")
		end
		return
	end
	
	entity_moveTowardsTarget(me, dt, 1000)

	entity_updateMovement(me, dt)
	
	entity_handleShotCollisions(me)
	
	if entity_touchAvatarDamage(me, entity_getCollideRadius(me), 1, 100) then
		entity_damage(me, me, 1)
	else
		if singing then
			singTimer = singTimer + dt
			if singTimer > 0.1 then	
				-- die
				entity_msg(darkli, "died")
				entity_damage(me, me, 1)
			end
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_DEATHSCENE) then
		t = 2
		entity_setStateTime(me, t)
		spawnParticleEffect("", entity_x(me), entity_y(me))
		entity_scale(me, 0, 0, t)
	end
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	if attacker == me then
		return true
	end
	return false
end

function animationKey(me, key)
end

function hitSurface(me)
end

function songNote(me, note)
	if entity_getAlpha(me) > 0 then
		if note == myNote then
			singing = true
			entity_offset(me, -5, 0)
			entity_offset(me, 5, 0, 0.01, -1, 1)
		end
	end
end

function songNoteDone(me, note)
	entity_offset(me, 0, 0, 0.1)
	if note == myNote then
		
		singing = false
		singTimer = 0
	end
end

function song(me, song)
end

function activate(me)
end

function msg(me, msg, v)
	if msg == "note" then
		myNote = v
		bone_setTexture(bone_note, string.format("song/notesymbol%d", myNote))
		bone_scale(bone_note, 1, 1)
		bone_scale(bone_note, 4, 4, 0.5, -1, 1)
		bone_alpha(bone_note, 0.5)
		r,g,b = getNoteColor(myNote)
		r = r*0.5 + 0.5
		g = g*0.5 + 0.5
		b = b*0.5 + 0.5
		bone_setColor(bone_note, r, g, b)
	end
	if msg == "sd" then
		startDelay = v
		
		if startDelay <= 0 then
			startDelay = 0.1
		end
	end
end


