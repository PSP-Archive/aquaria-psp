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
boss = 0
ompo = 0
start = 0
goto = 0
eatPos = 0

function init(me)
	start = getNode("OMPOSTART")
	goto = getNode("OMPOGOTO")
	boss = getEntity("EnergyBoss")
	eatPos = getNode("EATPOS")
	n = getNaija()
	if isFlag(FLAG_OMPO, 4) then
		entity_alpha(boss, 0)
	end
	boss = getEntity("EnergyBoss")
end

c = false
function update(me)
	if c then return end
	
	if node_isEntityIn(me, n) and isFlag(FLAG_OMPO, 4) and isFlag(FLAG_ENERGYBOSSDEAD, 0) then
		c = true
		
		
		entity_idle(n)
		if entity_isfh(n) then entity_fh(n) end
		musicVolume(0.5, 1)
		watch(1)
		
		
		
		ompo = createEntity("Ompo", "", node_x(start), node_y(start))
		entity_alpha(ompo, 0)
		entity_alpha(ompo, 1, 1)
		entity_setState(ompo, STATE_INTRO)
		entity_flipToEntity(ompo, n)
		
		playSfx("Ompo")
		watch(0.5)
		
		--cam_toEntity(ompo)
		
		entity_setPosition(ompo, node_x(goto), node_y(goto), 3, 0, 0, 1)
		--entity_setEntityLayer(ompo, 0)
		watch(3)
		
		musicVolume(0, 5)
		watch(1)
		
		-- do animation stuff
		
		
		
		entity_alpha(boss, 1, 0.1)
		entity_setPosition(boss, node_x(eatPos), node_y(eatPos))
		
		--
		entity_setState(boss, STATE_APPEAR)
		
		watch(1.3)
		
		emote(EMOTE_NAIJAUGH)
		
		setGameSpeed(0.5)
		
		watch(0.5)
		
		setGameSpeed(1)
		
		entity_alpha(ompo, 0)
		entity_setPosition(ompo, 0, 0)
		
		-- now,  set the bone on
		boneOmpo = entity_getBoneByName(boss, "Ompo")
		
		bone_setVisible(boneOmpo, true)
		bone_scale(boneOmpo, 0.4, 0.4)
		
		watch(0.1)
		
		playSfx("Bite")
		
		cam_toEntity(boss)
	
		--[[
		watch(0.5)
		ct = 0
		while entity_isAnimating(boss) do
			watch(0.2)
			--playSfx("Bite")
			
			ct = ct + 1
			if ct >= 2 then
				
				break
			end
		end
		
		watch(2)
		playSfx("Gulp")
		]]--
		
		while entity_isAnimating(boss) do
			watch(FRAME_TIME)
		end
		
		bone_scale(boneOmpo, 0, 0, 0.5)
		--watch(0.5)
		
		
		cam_toEntity(boss)
		
		entity_setState(boss, STATE_INTRO)
		
		watch(1.5)
		
		playMusic("BigBoss")
		
		cam_toEntity(n)
		
		setFlag(FLAG_OMPO, 5)
		
		c = false
	end
end
