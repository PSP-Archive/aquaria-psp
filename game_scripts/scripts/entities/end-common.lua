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

v.spirit = 0
v.n1 = 0
v.n2 = 0
v.n3 = 0
v.n4 = 0

function initEnd(flag, skel)
	if isFlag(FLAG_ENDING, flag) then
		cam_toNode(
		
		fadeIn(1)
		fade2(1, 0)
		return true
	end
	
	return false
end

function endBit(spiritNodeName, anim)
	local sn = getNode(spiritNodeName)
	
	entity_setPosition(v.spirit, node_x(sn), node_y(sn))
	
	entity_animate(v.spirit, anim, -1)
	
	
end

function doEnd1(spiritNodeName, anim)
	cam_toNode(v.n1)
	watch(0.2)
	
	cam_toNode(v.n1)
end

function doEnd2(spiritNodeName, anim)
	cam_toNode(v.n3)
	watch(0.2)
	
end

function endEnd()
end

