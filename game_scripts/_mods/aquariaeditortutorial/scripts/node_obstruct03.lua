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

started 		= false
n 			= 0
timer 			= 999
thingsToSay		= 20
thingSaying		= -1
timeToSay		= 5
function init(me)
	n = getNaija()
	node_setCursorActivation(me, true)
end

function sayNext()
	if thingSaying == 0 then
		setControlHint("This is a slightly thicker obstruction.  Press 'O' again to make this.", 0, 0, 0, 16)
	end
end

function update(me, dt)
	if getStringFlag("editorhint") ~= node_getName(me) then
		started = false
		return
	end
	if started then
		timer = timer + dt
		if timer > timeToSay then
			timer = 0
			thingSaying = thingSaying + 1
			sayNext()
		end
	end
end

function activate(me)
	clearControlHint()
	started = true
	thingSaying = -1
	timer = 999
	setStringFlag("editorhint", node_getName(me))
end

