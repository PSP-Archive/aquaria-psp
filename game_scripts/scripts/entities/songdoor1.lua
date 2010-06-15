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

-- energy door
dofile("scripts/entities/songdoorcommon.lua")

songNote1 = 0
songNote2 = 0
songNote3 = 0
songNote4 = 0

function init(me)
	commonInit(me)
	if isFlag(FLAG_SONGDOOR1, 1) then
		entity_setState(me, STATE_OPENED)
	else
		entity_setState(me, STATE_CLOSED)
	end
	setWarpSceneNode("SongCave02", "SONGDOORENTER", "R")
end

function songNote(me, note)
	if entity_isState(me, STATE_CLOSED) then
		songNote1 = songNote2
		songNote2 = songNote3
		songNote3 = songNote4
		songNote4 = note
		--[[
		debugLog(string.format("%d, %d, %d, %d", songNote1, songNote2, songNote3, songNote4))
		]]--
		if songNote1 == 1 and songNote2 == 3 and songNote3 == 7 and songNote4 == 5 then
			voiceInterupt("SongDoor1Open")
			setFlag(FLAG_SONGDOOR1, 1)
			entity_setState(me, STATE_OPEN)
		end
	end
end