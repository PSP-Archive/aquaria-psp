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

flag = 0
node = 0
gfx = ""
function commonInit(me, g, f)
	gfx = g
	flag = f
	setupEntity(me, gfx)
	entity_setEntityType(me, ET_NEUTRAL)
	--[[
	if not isFlag(flag, 0) then
		entity_alpha(me, 0)
	end
	]]--
	entity_setWeight(me, 200)
	if isFlag(flag, 0) then
		debugLog("SETTING MOVABLE to TRUE")
		entity_setProperty(me, EP_MOVABLE, true)
	else
		entity_setProperty(me, EP_MOVABLE, false)
	end
	--entity_setActivation(me, AT_CLICK, 512, 32)
end

function hitSurface(me)
end

function update(me, dt)
	if node == 0 then
		node = entity_getNearestNode(me, gfx)
	end
	if isFlag(flag, 0) then
		entity_updateMovement(me, dt)
		x = node_x(node)
		y = node_y(node)
		--debugLog(string.format("node(%d, %d)", x, y))
		if entity_isPositionInRange(me, x, y, 64) then
			--debugLog("IN RANGE!")
			entity_stopPull(me)
			entity_setProperty(me, EP_MOVABLE, false)
			setFlag(flag, 1)			
		end
	else
		entity_setPosition(me, node_x(node), node_y(node))
	end
end

function activate(me)
--[[
	if isFlag(flag, 0) then
		entity_alpha(me, 0, 1)
		spawnParticleEffect("Collect", entity_x(me), entity_y(me))
		setFlag(flag, 1)
		entity_setActivation(me, AT_NONE)
	end
	]]--
end

function enterState(me, state)
end

function exitState(me, state)
end


