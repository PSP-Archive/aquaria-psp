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


--function bASDFASDF () ()A {}}A SDFASJDF end end end

n = 0
mia = 0
baby = 0

done = false

function init(me)
	n = getNaija()
	mia = getEntity("MiaGhost")
	baby = getEntity("NaijaChildGhost")
	
	entity_fh(mia)
	entity_alpha(mia, 0)
	entity_alpha(baby, 0)
end

function update(me, dt)
	if not done and isFlag(FLAG_SECRET02, 0) then 
		if node_isEntityIn(me, n) then
			done = true
			
			changeForm(FORM_NORMAL)
			
			entity_idle(n)
			
			--cam_toEntity(mia)
			cam_toNode(getNode("MEMCAM"))
			overrideZoom(0.9, 18)
			
			playSfx("naijagasp")
			
			
			
			setSceneColor(0.5, 0.5, 1, 2)
			watch(2)
			
			playMusic("Mystery")
		
			entity_alpha(mia, 1, 2)
			
			watch(4)
			
			entity_alpha(baby, 1, 2)
			watch(4)
			
			--entity_animate(mia, "babyLookUp")
			
			watch(2.5)
			--fadeOutMusic(3)
			setSceneColor(1, 1, 1, 3)
			entity_alpha(mia, 0, 1)
			watch(2)
			entity_alpha(baby, 0, 1)
			watch(2)
			
			cam_toEntity(n)
			
			setFlag(FLAG_SECRET02, 1)
			
			foundLostMemory()
			
			overrideZoom()
			
			--updateMusic()
		
		end
	end
	
end
