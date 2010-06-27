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
	
	
	toggleSteam(false)
	--[[
	if isFlag(FLAG_SUNKENCITY_BOSS, 1) then
		entity_delete(dad)
		entity_delete(mom)
		return
	end
	]]--
end

function update(me, dt)

	if isFlag(FLAG_SUNKENCITY_BOSS, 1) then
		return
	end
	
	if entity_isState(dad, STATE_DEATHSCENE) then
		setFlag(FLAG_SUNKENCITY_BOSS, 1)
		entity_setState(door, STATE_OPEN)
	end	
	
	if not spawned and getFlag(FLAG_SUNKENCITY_PUZZLE)==SUNKENCITY_CLAYDONE then
		debugLog("starting boss fight")
		
		shakeCamera(5, 3)
		
		playSfx("sunkendad-breakout")
		
		toggleSteam(true)
		
		setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_BOSSFIGHT)
		dadSpawn = getNode("DADSPAWN")
		momSpawn = getNode("MOMSPAWN")
		
		entity_setPosition(dad, node_x(dadSpawn), node_y(dadSpawn))
		entity_setPosition(mom, node_x(momSpawn), node_y(momSpawn))
		
		entity_setState(dad, STATE_START)
		entity_setState(mom, STATE_START)
		
		if not entity_isState(door, STATE_CLOSED) then
			entity_setState(door, STATE_CLOSE)
		end
		playMusic("sunken")
		
	end
	--[[
	if not spawned then
		if node_isEntityIn(me, getNaija()) then
			entity_setState(dad, STATE_START)
			entity_setState(mom, STATE_START)
			entity_setState(door, STATE_CLOSE)
			spawned = true			
		end
	end
	]]--
end
