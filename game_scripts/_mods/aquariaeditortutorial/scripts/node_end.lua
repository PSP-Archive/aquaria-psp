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

v = getVars()

v.started 		= false
v.n 			= 0
v.timer 			= 999
v.thingsToSay		= 20
v.thingSaying		= -1
v.timeToSay		= 5
function init(me)
	v.n = getNaija()
	node_setCursorActivation(me, true)
end

function sayNext()
	if v.thingSaying == 0 then
		setControlHint("Well, that's the end of the tutorial.", 0, 0, 0, 16)
	elseif v.thingSaying == 1 then
		setControlHint("If you continue further, you will be taken to the beginning of the tutorial.", 0, 0, 0, 16)
	elseif v.thingSaying == 2 then
		setControlHint("The exit to your right is defined by a node, as well.", 0, 0, 0, 16)
	elseif v.thingSaying == 3 then
		setControlHint("If you don't want to take the exit, you can try pressing 'P' in the level editor and see what happens!", 0, 0, 0, 16)
	elseif v.thingSaying == 4 then
		setControlHint("For more information on making mods, read the provided documentation.", 0, 0, 0, 16)
	elseif v.thingSaying == 5 then
		setControlHint("Hope to see some cool levels from you in the future!", 0, 0, 0, 16)
	end
end

function update(me, dt)
	if getStringFlag("editorhint") ~= node_getName(me) then
		v.started = false
		return
	end
	if v.started then
		v.timer = v.timer + dt
		if v.timer > v.timeToSay then
			v.timer = 0
			v.thingSaying = v.thingSaying + 1
			sayNext()
		end
	end
end

function activate(me)
	clearControlHint()
	v.started = true
	v.thingSaying = -1
	v.timer = 999
	setStringFlag("editorhint", node_getName(me))
end

