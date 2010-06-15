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

n = 0
done = false

function init(me)
	n = getNaija()
end

function update(me)
	if node_isEntityInside(me, n) and not done then
		voice("Laugh1")
		done = true
		--[[
		v = getFlag(FLAG_CREATORVOICE)
		if isFlag(FLAG_CREATORVOICE, 0) then
			voice("Laugh1")
		elseif isFlag(FLAG_CREATORVOICE, 1) then
			voice("Laugh2")
		elseif isFlag(FLAG_CREATORVOICE, 2) then
			voice("Laugh2")
		end
		setFlag(FLAG_CREATORVOICE, getFlag(FLAG_CREATORVOICE)+1)
		]]--
	end
end
