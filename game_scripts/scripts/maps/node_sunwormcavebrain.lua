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

h1 = 0
h2 = 0
past = 0

timer = 0

levelAt = 1
pastDefault = false

n = 0

function init(me)
	h1 = getNode("WATERLEVEL_HIGH")
	h2 = getNode("WATERLEVEL_LOW")
	past = getNode("RAISE_WATERLEVEL")
	bossRoom = getNode("NAIJA_ENTER")
	n = getNaija()
	
	loadSound("waterlevelchange")
end
	
function activate(me)
end

function update(me, dt)
	if not isFlag(FLAG_BOSS_SUNWORM, 0) then
		setWaterLevel(node_y(getNode("ENDWATERLEVEL")))
		return
	end
	if entity_x(n) > node_x(past) then
		if not pastDefault then
			setWaterLevel(node_y(h1), 2)
			pastDefault = true
		end
	else
		pastDefault = false
		timer = timer + dt
		if timer > 7 then
			playSfx("waterlevelchange")
			if levelAt == 1 then
				levelAt = 2
			else
				levelAt = 1
			end
			if levelAt == 1 then
				setWaterLevel(node_y(h1), 2)
			elseif levelAt == 2 then
				setWaterLevel(node_y(h2), 2)
			end
			timer = 0
		end
	end
end
