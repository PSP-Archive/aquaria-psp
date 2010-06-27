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

spawned = false
dad = 0
mom = 0
door = 0
didInit = false

function init(me)
	door = node_getNearestEntity(me, "EnergyDoor")
	dad = getEntity("SunkenDad")
	mom = getEntity("SunkenMom")	
	if isFlag(FLAG_SUNKENCITY_BOSS, 1) then
		entity_delete(dad)
		entity_delete(mom)
		return
	end
end

function doInit(me)
	--debugLog("Setting door to opened")
	entity_setState(door, STATE_OPENED)
end

function update(me, dt)
	if not didInit then
		doInit(me)
		didInit = true
	end
	if isFlag(FLAG_SUNKENCITY_BOSS, 1) then
		return
	end
	if entity_isState(dad, STATE_DEATHSCENE) then
		setFlag(FLAG_SUNKENCITY_BOSS, 1)
		entity_setState(door, STATE_OPEN)
	end	
	if not spawned then
		if node_isEntityIn(me, getNaija()) then
			entity_setState(dad, STATE_START)
			entity_setState(mom, STATE_START)
			entity_setState(door, STATE_CLOSE)
			spawned = true			
		end
	end
end
