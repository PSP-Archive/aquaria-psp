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

myNote = 0
noteQuad = 0

function commonInit(me, note)
	myNote = note
end

function update(me, dt)
end

function songNote(me, note)
	if note == myNote then
		if noteQuad ~= 0 then
			quad_delete(noteQuad)
			noteQuad = 0
		end
		
		noteQuad = createQuad(string.format("Song/NoteSymbol%d", myNote), 6)
		quad_alpha(noteQuad, 1)
		quad_alpha(noteQuad, 0, 0.5)
		--quad_scale(noteQuad, 3, 3, 0.5, 0, 0, 1)
		--quad_setBlendType(noteQuad, BLEND_ADD)
		
		r,g,b = getNoteColor(myNote)
		quad_color(noteQuad, r, g, b)
		
		x,y = node_getPosition(me)
		quad_setPosition(noteQuad, x, y)
	end
end

function songNoteDone(me, note, t)
end




