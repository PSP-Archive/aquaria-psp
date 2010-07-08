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

v.myNote = 0
v.noteQuad = 0

function commonInit(me, note)
	v.myNote = note
end

function update(me, dt)
end

function songNote(me, note)
	if note == v.myNote then
		if v.noteQuad ~= 0 then
			quad_delete(v.noteQuad)
			v.noteQuad = 0
		end
		
		v.noteQuad = createQuad(string.format("Song/NoteSymbol%d", v.myNote), 6)
		quad_alpha(v.noteQuad, 1)
		quad_alpha(v.noteQuad, 0, 0.5)
		--quad_scale(v.noteQuad, 3, 3, 0.5, 0, 0, 1)
		--quad_setBlendType(v.noteQuad, BLEND_ADD)
		
		local r,g,b = getNoteColor(v.myNote)
		quad_color(v.noteQuad, r, g, b)
		
		local x,y = node_getPosition(me)
		quad_setPosition(v.noteQuad, x, y)
	end
end

function songNoteDone(me, note, t)
end




