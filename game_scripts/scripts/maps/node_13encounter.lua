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
mia = 0
done = false
head = 0

function init(me)
	n = getNaija()
	
	-- get start conditions
	
	mia = createEntity("13_Progression", "", node_x(me), node_y(me))
	entity_alpha(mia)
	
	loadSound("mia-appear")
end

function update(me, dt)
	if not done then 
		if node_isEntityIn(me, n) then
			--debugLog(string.format("mithala flag: %d", getFlag(FLAG_BOSS_MITHALA)))
			if isFlag(FLAG_BOSS_MITHALA, 1) then
				setFlag(FLAG_BOSS_MITHALA, 2)
			elseif isFlag(FLAG_BOSS_FOREST, 1) then
				setFlag(FLAG_BOSS_FOREST, 2)
			-- no suitable place for this to happen right now
			--[[
			elseif isFlag(FLAG_BOSS_SUNWORM, 1) then
				setFlag(FLAG_BOSS_SUNWORM, 2)
			]]--
			else
				return
			end
			
			offx = 80
			offy = -20
			if entity_x(n) < node_x(me) then
				offx = -offx
			end
			
			entity_setPosition(n, node_x(me)+offx, node_y(me)+offy, 1, 0, 0, 1)
		
			debugLog("running script")
			done = true
			
			entity_idle(n)
			entity_setPosition(mia, node_x(me), node_y(me))
			entity_flipToEntity(mia, n)
			entity_flipToEntity(n, mia)
			
			cam_toEntity(mia)
			
			playSfx("mia-appear")
			
			spawnParticleEffect("MiaWarp", node_x(me), node_y(me))
			
			fadeOutMusic(2)
			setSceneColor(0.5, 0.5, 1, 2)
			watch(2)
			
			playMusic("Mystery")
			
			entity_alpha(mia, 1, 2)
			
			watch(2)
			watch(4)
			
			if isFlag(FLAG_13PROGRESSION, 0) then
			end
			incrFlag(FLAG_13PROGRESSION, 1)
			
			playSfx("mia-appear")
			spawnParticleEffect("MiaWarp", node_x(me), node_y(me))
			watch(1)
			fadeOutMusic(3)
			setSceneColor(1, 1, 1, 3)
			entity_alpha(mia, 0, 1)
			watch(2)
			entity_setPosition(mia, 0, 0)
			
			cam_toEntity(n)
			
			updateMusic()
		
		end
	end
	
end
