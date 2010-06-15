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
ix,iy=0,0
escapeNode=0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_NEUTRAL)
	entity_initSkeletal(me, "CC")
	entity_setAllDamageTargets(me, false)
	
	entity_scale(me, 0.6, 0.6)
	entity_setBlendType(me, BLEND_ADD)
	entity_alpha(me, 0.5)
	entity_alpha(me, 1, 1, -1, 1, 1)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	ix,iy = entity_getPosition(me)
	escapeNode = getNode("CAVEEXIT")

	if isFlag(FLAG_SUNKENCITY_BOSS, 1) then
		entity_delete(me)
	elseif getFlag(FLAG_SUNKENCITY_PUZZLE) > 0 then		
		node = getNode("MOTHER")
		entity_setPosition(me, node_x(node), node_y(node))
		node2 = getNode("MOTHERSPAWN")
		mother = createEntity("CC_Mother", "", node_x(node2), node_y(node2))	
		entity_setState(me, STATE_DONE)
		entity_animate(me, "float", -1)
	else
		entity_setState(me, STATE_IDLE)
	end	
	entity_rotate(me, 0)
end

done = false
inScene = false
function cutScene(me)
	if inScene then return end
	done = true
	inScene = true
	-- mother arrives, sings song in loop
	entity_idle(n)
	cam_toEntity(me)	
	--node_getPosition(getNode("MOTHER"))
	node = getNode("MOTHER")
	entity_setPosition(me, node_x(node), node_y(node), 3, 0, 0, 1)
	watch(3)
	watch(1)
	-- mother fades in
	node2 = getNode("MOTHERSPAWN")
	mother = createEntity("CC_Mother", "", node_x(node2), node_y(node2))	
	entity_alpha(mother, 0)
	entity_alpha(mother, 1, 2)

	watch(1)
	entity_setState(me, STATE_DONE)
	watch(1)
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, 1)
	cam_toEntity(n)
	
	inScene = false
end

function update(me, dt)
	if entity_isState(me, STATE_IDLE) then
		if entity_isEntityInRange(me, n, 256) and getForm() == FORM_LIGHT then
			entity_setState(me, STATE_FOLLOW)
		end
	elseif entity_isState(me, STATE_FOLLOW) then
		if not done then
			if node_isEntityIn(escapeNode, me) then
				cutScene(me)
			else
				if getForm() ~= FORM_LIGHT or not entity_isEntityInRange(me, n, 512) then
					entity_setPosition(me, ix, iy, 3, 0, 0, 1)
					entity_setState(me, STATE_IDLE)
				else	
					if entity_isEntityInRange(me, n, 128) then
						entity_moveTowardsTarget(me, dt, -500)
					else
						entity_moveTowardsTarget(me, dt, 500)
					end
					entity_doCollisionAvoidance(me, dt, 2, 1)
					entity_updateMovement(me, dt)
					entity_flipToEntity(me, n)
				end
			end
		end
	elseif entity_isState(me, STATE_DONE) then
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "cry", -1)
		entity_setPosition(me, entity_getPosition(me))
	elseif entity_isState(me, STATE_FOLLOW) then
		entity_animate(me, "float", -1)	
	elseif entity_isState(me, STATE_DONE) then
		
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

