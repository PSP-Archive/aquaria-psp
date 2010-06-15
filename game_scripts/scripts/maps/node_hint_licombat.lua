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

function init(me)
end

function update(me, dt)
	if hasLi() and isFlag(FLAG_HINT_LICOMBAT, 0) then
		if node_isEntityIn(me, getNaija()) then
			setControlHint(getStringBank(7), 0, 0, 0, 8, "", SONG_LI)
			setFlag(FLAG_HINT_LICOMBAT, 1)
		end
	end
end
