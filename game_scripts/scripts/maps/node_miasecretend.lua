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

done = false

bone_body = 0
match = false
mia = 0
n = 0

function init(me)
end

function update(me, dt)
	if match then
		-- float with
		if bone_body == 0 then
			bone_body = entity_getBoneByName(mia, "Body")
		end
		
		bx, by = bone_getWorldPosition(bone_body)
		entity_setPosition(n, entity_x(n), by)
	end
	if not done and node_isEntityIn(me, getNaija()) then
		done = true
		
		n = getNaija()
		
		cam = getNode("cam")
		cam2 = getNode("cam2")
		naija2 = getNode("naija2")
		
		entity_idle(n)
		
		changeForm(FORM_NORMAL)
		watch(0.5)
		
		entity_idle(n)
		
		fade2(1, 0.5)
		watch(0.5)
		
		overrideZoom(0.9)
		-- do the secret ending!
		thir = createEntity("13_progression")
		mia = createEntity("mia")
		entity_offset(thir, 0, 40)
		entity_warpToNode(n, getNode("naija"))
		entity_warpToNode(thir, getNode("mia"))
		entity_warpToNode(mia, getNode("mia"))
		entity_flipToEntity(mia, n)
		entity_flipToEntity(thir, n)
		entity_flipToEntity(n, mia)
		entity_alpha(mia, 0)
		fadeIn(1)
		fade2(0, 1, 0, 0, 0)
		watch(1)
		
		watch(1)
		
		watch(0.5)
		
		--cam_toEntity(mia)
		cam_toNode(cam)
		
		watch(0.5)
		
		playSfx("spirit-beacon")
		
		spawnParticleEffect("SpiritBeacon", entity_x(mia), entity_y(mia))
		fade2(1,0.2,1,1,1) watch(0.2)
		fade2(0,0.2,1,1,1) watch(0.2)
		
		playSfx("mia-appear")
		
		entity_alpha(thir, 0.1, 3)
		entity_alpha(mia, 1, 3)
		watch(2.8)
		fade2(1,0.2,1,1,1) watch(0.2)
		fade2(0,0.8,1,1,1)
		entity_setPosition(thir, 0, 0)
		
		playVoice("mia-and-naija")
		
		watch(22)
		
		--22 
		fade2(1, 0.5)
		
		--cam_toNode(cam)
		
		
		watch(0.5)
		
		entity_setPosition(n, node_x(naija2), node_y(naija2))
		
		overrideZoom(1.2)
		
		entity_animate(mia, "hug", -1, 1)
		entity_animate(n, "hugLi", -1, 3)
		match = true
		
		cam_toNode(cam2)
		watch(0.5)
		
		--23
		
		fade2(0, 0.5)
		watch(1)
		
		--24
		watch(7)
		
		-- 31 "I..."
		
		watch(1)
		
		-- 32 "I killed him.."
		
		watch(2)
		-- 34 "but he was just a little boy"
		
		-- 33
		
		watch(27)
		
		-- 01:00
		
		watch(60+12)
		-- (02:14)
		fade(1, 4)
		
		watchForVoice()
		
		fade2(1, 1, 1, 1, 1)
		watch(1)
		
		--goToTitle()
		loadMap("lucien")
	end
end