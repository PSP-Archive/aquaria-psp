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

hatchMax = 5
hatchTimer = hatchMax

rollTimer = 0
rollMax = 2

hint = false

function init(me)
	setupEntity(me, "Collectibles/egg-dumbo")
	loadSound("Pet-Hatch")
end

function postInit(me)
	n = getNaija()
end

function update(me, dt)
	if isFlag(FLAG_PET_DUMBO, 1) then
		entity_alpha(me, 0)
	end
	
	if isFlag(FLAG_PET_DUMBO, 0) and not hint and entity_isEntityInRange(me, n, 256) then
		playSfx("secret")
		setControlHint(getStringBank(32), 0, 0, 0, 6, "collectibles/egg-dumbo")
		hint = true
	end
	
	--[[
	if entity_isState(me, STATE_IDLE) and entity_getAlpha(me) == 1 and isFlag(FLAG_PET_DUMBO, 0) then
		hatchTimer = hatchTimer + dt*0.5
		if hatchTimer > hatchMax then
			hatchTimer = hatchMax
		end
	end
	]]--
	if entity_getAlpha(me) == 1 and isFlag(FLAG_PET_DUMBO, 0) then
		if entity_isEntityInRange(me, n, 300) then
			entity_offset(me, math.random(2)-1, 0)
			hatchTimer = hatchTimer - dt
			if hatchTimer < 0 then
				
				hatchTimer = 0
				entity_setState(me, STATE_HATCH)
			end
		else
			entity_offset(me, 0, 0)
			hatchTimer = hatchTimer + dt*0.5
			if hatchTimer > hatchMax then
				hatchTimer = hatchMax
			end
		end
	end
	
	rollTimer = rollTimer + dt
	if rollTimer >= rollMax then
		rollTimer = 0
		entity_rotate(me, 0)
		if chance(50) then
			entity_rotate(me, -30, 0.2, 3, 1, 1)
		else
			entity_rotate(me, 30, 0.2, 3, 1, 1)
		end
	end
end

function lightFlare(me)
	--[[
	if entity_getAlpha(me) == 1 and isFlag(FLAG_PET_DUMBO, 0) then
		hatchTimer = hatchTimer - 3.1
		if hatchTimer <= 0 then
			hatchTimer = 0
			
			entity_setState(me, STATE_HATCH)
		end
		spawnParticleEffect("DumboEggCharge", entity_x(me), entity_y(me))
	end
	]]--
end

function enterState(me, state)
	if entity_isState(me, STATE_HATCH) then
		playSfx("Pet-Hatch")
		entity_setStateTime(me, 2)
		
		entity_alpha(me, 0.7, 2)
		entity_scale(me, 1.2, 1.2, 2)
	end
end

function exitState(me, state)
	if entity_isState(me, STATE_HATCH) then
		
		setFlag(FLAG_PET_DUMBO, 1)
		e = setActivePet(FLAG_PET_DUMBO)
		
		if e ~= 0 then
			entity_setPosition(e, entity_x(me), entity_y(me))
		end
		
		playSfx("Secret")
		playSfx("Collectible")
	end
end
