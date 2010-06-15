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

dofile("scripts/entities/doorcommon.lua")

n = 0
glow = 0
body = 0

curNote = 0
sungNote = -1
notesToSing = 8

timer = 3

function init(me)
	commonInit(me, "sun-door-0001", "EnergyDoor", 1, 0)
	entity_setState(me, STATE_CLOSED, -1)
	
	if entity_isFlag(me, 1) then
		entity_setState(me, STATE_OPENED)
	end
	--entity_initSkeletal(me, "SunDoor")
	--[[
	body = entity_getBoneByName(me, "Body")
	bone_alpha(body, 0)
	
	glow = entity_getBoneByName(me, "Glow")
	bone_setBlendType(glow, BLEND_ADD)
	]]--
end

function singNote(me)
	timer = 9 - curNote
	sungNote = getRandNote()
	q = createQuad("particles/ring2")
	quad_setPosition(q, entity_x(me), entity_y(me))
	quad_scale(q, 0, 0)
	quad_scale(q, 4, 4, 0.2)
	quad_setBlendType(q, BLEND_ADD)
	r, g, b = getNoteColor(sungNote)
	quad_color(q, r, g, b, 0.1)
	quad_delete(q, 1)
	
	
	q = createQuad(string.format("song/notesymbol%d", sungNote))
	quad_setPosition(q, entity_x(me), entity_y(me))
	quad_scale(q, 0, 0)
	quad_scale(q, 2, 2, 1)
	--quad_setBlendType(q, BLEND_ADD)
	r, g, b = getNoteColor(sungNote)
	quad_color(q, r, g, b, 3)
	quad_delete(q, 5)
end

function update(me, dt)
	if n == 0 then n = getNaija() end
	commonUpdate(me, dt)
	
	if entity_isState(me, STATE_CLOSED) then
		if entity_isEntityInRange(me, n, 256) then
			--debugLog("In range")
			timer = timer - dt
			if timer < 0 then
				if chance(75) then
					if chance(50) then
						emote(EMOTE_NAIJASIGH)
					else
						emote(EMOTE_NAIJAUGH)
					end
				end
				playSfx("Denied")
				curNote = 0
				-- play a note
				singNote(me)
			end
		end
	end
		
	-- timer do stuff
end

function songNote(me, note)
	if entity_isState(me, STATE_CLOSED) then
		if sungNote > -1 then
			if note == sungNote then
				
				curNote = curNote + 1
				if curNote >= notesToSing then
					entity_setFlag(me, 1)
					entity_setState(me, STATE_OPEN)
					playSfx("sunform")
					playSfx("collectible")
				else
					playSfx("secret")
					singNote(me)
				end
			else
				curNote = 0
				sungNote = -1
				emote(EMOTE_NAIJASIGH)
				playSfx("Denied")
			end
		end
	end
end

