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

d1 = 0
d2 = 0
d3 = 0

firstSet = true

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_NEUTRAL)
	entity_initSkeletal(me, "timerock")	

	--entity_setTexture(me, "missingImage")
	entity_scale(me, 1.25, 1.25)	
	
	entity_alpha(me, 0.5)
	
	entity_setState(me, STATE_IDLE)
	
	entity_setEntityLayer(me, -1)
	
	entity_setCullRadius(me, 256)
	
	d1 = entity_getBoneByName(me, "d1")
	d2 = entity_getBoneByName(me, "d2")
	d3 = entity_getBoneByName(me, "d3")
	
	firstSet = true
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

function update(me, dt)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	end
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function msg(me, s, v)
	if s == "time" then
		if entity_isEntityInRange(me, getNaija(), 3500) then
			spawnParticleEffect("tinyredexplode", entity_x(me), entity_y(me))
			playSfx("saved")
		end
		
		mins = math.floor(v/60)
		secs = v - (mins*60)
		
		secs1 = math.floor(secs/10)
		secs2 = secs - (secs1*10)
		
		if mins > 9 then mins = 9 end
		
		debugLog(string.format("timerock %d : %d%d", mins, secs1, secs2))
		
		bone_setTexture(d1, string.format("seahorse/num-%d", mins))
		bone_setTexture(d2, string.format("seahorse/num-%d", secs1))
		bone_setTexture(d3, string.format("seahorse/num-%d", secs2))
	end
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

