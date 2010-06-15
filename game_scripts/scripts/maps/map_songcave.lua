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

leave = true

function init()
	if isFlag(FLAG_ENDING, ENDING_NAIJACAVE) then
		loadSound("spirit-awaken")
		
		e1 = getNode("end1")
		e2 = getNode("end2")
		e3 = getNode("end3")
		e4 = getNode("end4")
		e5 = getNode("end5")
		e6 = getNode("end6")
		
		g1 = getNode("ghost1")
		g2 = getNode("ghost2")
		
		overrideZoom(0.8)
		
		fade2(1, 0)
		
		spirit = createEntity("erulianghost", "", node_x(e6), node_y(e6))
		turtle = entity_getNearestEntity(spirit, "turtle")
		if turtle ~= 0 then
			entity_delete(turtle)
			turtle = 0
		end
		entity_alpha(spirit, 1)
		entity_offset(spirit, 0, -10)
		entity_offset(spirit, 0, 10, 1, -1, 1, 1)
		
		cam = createEntity("empty", "", node_x(e1), node_y(e1))
		cam_toEntity(cam)
		
		watch(0.5)
		
		entity_setPosition(cam, node_x(e2), node_y(e2), 8, 0, 0, 1)
		
		fade2(0, 3)
		fadeIn(0)
		watch(5)
		
		fade2(1, 3)
		watch(3)
		
		
		entity_warpToNode(cam, e3)
		watch(0.5)
		
		entity_setPosition(cam, node_x(e4), node_y(e4), 8, 0, 0, 1)
		
		
		fade2(0, 3)
		watch(5)
		
		fade2(1, 3)
		watch(3)
		
		
		entity_warpToNode(cam, e5)
		watch(0.5)
		
		entity_setPosition(cam, node_x(e6), node_y(e6), 4, 0, 0, 1)
		
		
		fade2(0, 3)
		watch(5)
		
		--entity_animate(spirit, "sometihng")
		cam_toNode(g1)
		
		playSfx("spirit-awaken")
		spawnParticleEffect("erulian-appear", node_x(g1), node_y(g1))
		e = createEntity("erulianghost", "", node_x(g1), node_y(g1))
		entity_fh(e)
		entity_setColor(e, 0.5, 0.7, 1)
		entity_offset(e, 0, -10)
		entity_offset(e, 0, 10, 1, -1, 1, 1)
		entity_alpha(e, 0)
		entity_alpha(e, 1, 2)
		
		watch(3)
		
		entity_fh(spirit)
		
		cam_toNode(g2)
		
		playSfx("spirit-awaken")
		spawnParticleEffect("erulian-appear", node_x(g2), node_y(g2))
		e = createEntity("erulianghost", "", node_x(g2), node_y(g2))
		entity_setColor(e, 0.7, 1, 0.7)
		entity_offset(e, 0, -10)
		entity_offset(e, 0, 10, 1, -1, 1, 1)
		entity_alpha(e, 0)
		entity_alpha(e, 1, 2)
		
		watch(2)
		
		setCameraLerpDelay(1)
		
		cam_toEntity(spirit)
		overrideZoom(0.6, 4)
		
		watch(1)
	
		
		fade2(1, 3)
		watch(3)
		
		
		overrideZoom(0)
		setCameraLerpDelay(0)
		
		if leave then
			loadMap("energytemple05")
		else
			watch(1)
			fade2(0, 0)
			cam_toEntity(getNaija())
		end
		
		
		--fade2(0,4)
		
		--loadMap("energytemple")
	end
end
