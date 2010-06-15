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

-- DOOR COMMON
dofile("scripts/entities/entityinclude.lua")

init_x = 0
init_y = 0

dist = 700
delay = 0

effectRange = 1800

sound = ""

function commonInit(me, tex, snd, sz, flipDir)
	setupEntity(me, tex, -2)
	entity_scale(me, sz, sz)
	entity_setActivationType(me, AT_NONE)
	entity_setFillGrid(me, true)
	sound = snd
	loadSound(sound)
	if flipDir ~= 0 then
		dist = -dist
	end
end

function postInit(me)
	init_x = entity_x(me)
	init_y = entity_y(me)
end

ret = 0
function commonUpdate(me, dt)
	if entity_getState(me)==STATE_OPEN then
		--ret = ret + dt
		delay = delay + dt
		if delay > 0.2 then
			reconstructEntityGrid()
			delay = 0
		end
		if not entity_isInterpolating(me) then
			if entity_isEntityInRange(me, getNaija(), effectRange) then
				shakeCamera(4, 1)
			end
			entity_setState(me, STATE_OPENED)
		else
		--[[
			if ret > 1 then
				reconstructEntityGrid()
				ret = 0
			end
			]]--
		end
	end
	if entity_getState(me)==STATE_CLOSE then
		if not entity_isInterpolating(me) then
			entity_setState(me, STATE_CLOSED)
			shakeCamera(4, 1)
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_OPEN) then
		if entity_isEntityInRange(me, getNaija(), effectRange) then
			playSfx(sound)
		end
		if entity_getRotation(me) > -90 and entity_getRotation(me) < 90 then
			entity_interpolateTo(me, init_x, init_y-dist, 2)
		elseif entity_getRotation(me) == 90 then
			entity_interpolateTo(me, init_x+dist, init_y, 2)
		elseif entity_getRotation(me) == 270 then
			entity_interpolateTo(me, init_x-dist, init_y, 2)			
		end
	elseif entity_isState(me, STATE_CLOSE) then
		playSfx(sound)
		oldx = entity_x(me)
		oldy = entity_y(me)
		entity_setPosition(me, init_x, init_y)
		reconstructEntityGrid()
		entity_setPosition(me, oldx, oldy)
		entity_interpolateTo(me, init_x, init_y, 2)
	elseif entity_isState(me, STATE_CLOSED) then
		entity_setPosition(me, init_x, init_y)
		reconstructEntityGrid()
	elseif entity_isState(me, STATE_OPENED) then
		--entity_setColor(me, 1, 0, 0)
		if entity_getRotation(me) > -90 and entity_getRotation(me) < 90 then
			entity_setPosition(me, init_x, init_y-dist)
		elseif entity_getRotation(me) == 90 then
			entity_setPosition(me, init_x+dist, init_y)
		elseif entity_getRotation(me) == 270 then
			entity_setPosition(me, init_x-dist, init_y)			
		end
		reconstructEntityGrid()		
	end
end

function exitState(me)
end

function hitSurface(me)
end
