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
-- S L I P P E R  L O B S T E R
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

delay = 1.0

-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_setAllDamageTargets(me, false)
	
	entity_initSkeletal(me, "GlobeCrab")

	--entity_generateCollisionMask(me)
	--entity_setCollideRadius(me, 32)
	
	entity_setState(me, STATE_IDLE)
	
	entity_setHealth(me, 3)
	entity_setDropChance(me, 20, 1)
	
	entity_setDeathParticleEffect(me, "TinyRedExplode")
	entity_setUpdateCull(me, 4000)
end

function postInit(me)
	n = getNaija()
	--entity_setTarget(me, n)
end

function update(me, dt)

	if delay > 0 then
		delay = delay - dt
	else
		ent = createEntity("SlipperLobster", "", entity_x(me), entity_y(me))
		entity_rotate(ent, entity_getRotation(me))
		delay = 10.0
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_ROTATE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_WALK) then
		entity_animate(me, "idle", -1)		
	end
		
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	return true
end

function animationKey(me, key)
end

function hitSurface(me)
	--debugLog("HIT")
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end
