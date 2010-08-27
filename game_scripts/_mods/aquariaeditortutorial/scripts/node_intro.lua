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

started 		= false
n 			= 0
timer 			= 999
thingsToSay		= 20
thingSaying		= -1
timeToSay		= 5

function init(me)
	n = getNaija()
	started = true
	thingSaying = -1
	timer = 999
	setStringFlag("editorhint", node_getName(me))
end

function sayNext()
	if thingSaying == 0 then
		setControlHint("Welcome to the Aquaria Editor Tutorial Mod! This will teach you the basics of using Aquaria's level editor.", 0, 0, 0, 16)
	elseif thingSaying == 1 then
		setControlHint("Throughout the tutorial, you will see bubbles with question marks in them.", 0, 0, 0, 16)
	elseif thingSaying == 2 then
		setControlHint("These are hint bubbles! Right-click on them to learn about the editor.", 0, 0, 0, 16)
	elseif thingSaying == 3 then
		setControlHint("To begin the tutorial, right-click on these bubbles, from left to right!", 0, 0, 0, 16)
	end
end

function update(me, dt)
	if getStringFlag("editorhint") ~= node_getName(me) then
		started = false
		return
	end
	if started and node_isEntityIn(me, n) then
		timer = timer + dt
		if timer > timeToSay then
			timer = 0
			thingSaying = thingSaying + 1
			sayNext()
		end
	end
	if not started and node_isEntityIn(me, n) then
		started = true
	end
end
