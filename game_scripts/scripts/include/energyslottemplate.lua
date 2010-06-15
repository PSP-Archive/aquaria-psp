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

chargeIDOffset = 5000
function init(me)
	node_setCursorActivation(me, false)
	if getFlag(flag) > 0 then
		charged = true
		id = getFlag(flag)
		if id > chargeIDOffset then
			charged = false
			id = id - chargeIDOffset
		end
		--[[
		if getFlag(chargeFlag) == 0 then
			charged = false
		end
		]]--
		
		orbHolder = getEntityByID(holderID)		
		energyOrb = getEntityByID(id)
		if energyOrb ~=0 and orbHolder ~=0 then
			--debugLog(string.format("%s : setting orb to %d, %d", node_getName(me), entity_x(orbHolder), entity_y(orbHolder)))
			entity_setPosition(energyOrb, entity_x(orbHolder), entity_y(orbHolder))
			if charged then
				entity_setState(energyOrb, STATE_CHARGED)
			end
		end
		if charged then
			door = getEntityByID(doorID)
			if door ~= 0 then
				entity_setState(door, STATE_OPENED)
			end
		end
	end
end

function activate(me)
	if getFlag(flag)==0 or getFlag(flag) >= chargeIDOffset then
		energyOrb = node_getNearestEntity(me, "EnergyOrb")
		if energyOrb ~= 0 then
			if entity_isState(energyOrb, STATE_CHARGED) then
				debugLog("Saving orb in slot, charged")
				setFlag(flag, entity_getID(energyOrb))
				door = getEntityByID(doorID)
				if door ~= 0 then
					entity_setState(door, STATE_OPEN)
				else
					debugLog("COULD NOT FIND DOOR")
				end
			else
				debugLog("Saving orb in slot, not charged")
				setFlag(flag, entity_getID(energyOrb)+chargeIDOffset)				
			end
		else
			debugLog("Could not find orb")
		end
	end
end

function update(me, dt)
end