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

curNote = 0
n = 0
door = 0

numNotes = 4
curNote = 1

function init(me)
	n = getNaija()
	
	l1 = getEntityByName("SongLamp3")
	l2 = getEntityByName("SongLamp2")
	l3 = getEntityByName("SongLamp7")
	l4 = getEntityByName("SongLamp1")
	
	door = node_getNearestEntity(me, "EnergyDoor")
	
	if isFlag(FLAG_WHALELAMPPUZZLE, 1) then
		entity_setState(door, STATE_OPENED)
	end
end
	
	--[[
			if entity_isState(l1, STATE_ON) and entity_isState(l2, STATE_ON) and entity_isState(l3, STATE_ON)
			and entity_isState(l4, STATE_ON) then
			end
	]]--
function activate(me, ent)
	if curNote == 1 and ent == l1 then
		curNote = curNote + 1
	elseif curNote == 2 and ent == l2 then
		curNote = curNote + 1
	elseif curNote == 3 and ent == l3 then
		curNote = curNote + 1
	elseif curNote == 4 and ent == l4 then
		curNote = curNote + 1
	else
		curNote = 1
	end
	
	if curNote > 4 then
		playSfx("Collectible")
		debugLog("DONE")
		setFlag(FLAG_WHALELAMPPUZZLE, 1)
		entity_setState(door, STATE_OPEN)
		
		curNote = 1
	end
end

function update(me, dt)

end

function entityNumber(me, ent, num)
	if isFlag(FLAG_WHALELAMPPUZZLE, 1) then return end
end
