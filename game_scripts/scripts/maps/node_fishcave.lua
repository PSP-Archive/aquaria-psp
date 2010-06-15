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

door = 0
n1=0
n2=0
n3=0
n4=0
won = false
n=0
tous=0

r1=0
r2=0
r3=0
r4=0

hole = 0

spawnDelay = 0

numFishNeeded = 3

glow1 = 0
glow2 = 0
glow3 = 0
glow4 = 0

function init(me)
	-- for debug: 
	--node_setCursorActivation(me, true)
	n1 = getNode("FISHLOC1")
	n2 = getNode("FISHLOC2")
	n3 = getNode("FISHLOC3")
	n4 = getNode("FISHLOC4")
	
	glow1 = node_getNearestEntity(n1, "fishcaveglow")
	glow2 = node_getNearestEntity(n2, "fishcaveglow")
	glow3 = node_getNearestEntity(n3, "fishcaveglow")
	glow4 = node_getNearestEntity(n4, "fishcaveglow")
	
	entity_msg(glow1, "color", 1)
	entity_msg(glow2, "color", 1)
	entity_msg(glow3, "color", 1)
	entity_msg(glow4, "color", 1)
	
	r1 = getNode("rock1")
	r2 = getNode("rock2")
	r3 = getNode("rock3")
	r4 = getNode("rock4")
	
	n = getNaija()
	
	tous = getNode("TOUS")
	hole = getNode("HOLE")
end

function checkCreateFish(i)
	if spawnDelay == 0 then
		name = string.format("CaveFish%d", i)
		num = node_getNumEntitiesIn(tous, name)
		if num < 6 then
			spawnDelay = 3
			e = createEntity(name, "", node_x(hole), node_y(hole))
			entity_scale(e, 0.1, 0.1)
			entity_scale(e, 1, 1, 2)
			entity_alpha(e, 0)
			entity_alpha(e, 1, 0.5)
		end
	end
end

function update(me, dt)
	if isFlag(FLAG_FISHCAVE, 0) then
		num1 = node_getNumEntitiesIn(n1, "CaveFish1")
		num2 = node_getNumEntitiesIn(n2, "CaveFish2")
		num3 = node_getNumEntitiesIn(n3, "CaveFish3")
		num4 = node_getNumEntitiesIn(n4, "CaveFish4")
		--debugLog(string.format("%d, %d, %d, %d", num1, num2, num3, num4))
		if 	num1 >= numFishNeeded and
			num2 >= numFishNeeded and
			num3 >= numFishNeeded and
			num4 >= numFishNeeded then
			activate(me)
		end
		
		entity_msg(glow1, "g", num1/numFishNeeded)
		entity_msg(glow2, "g", num2/numFishNeeded)
		entity_msg(glow3, "g", num3/numFishNeeded)
		entity_msg(glow4, "g", num4/numFishNeeded)
	end
	
	if spawnDelay > 0 then
		spawnDelay = spawnDelay - dt
		if spawnDelay < 0 then
			spawnDelay = 0
		end
	end
	
	for i=1,4 do
		checkCreateFish(i)
	end
end

function doNode(nd, fx, nt)
	screenFadeCapture()
	cam_toNode(nd)
	screenFadeGo(0.5)
	watch(0.5)
	spawnParticleEffect(fx, node_x(nd), node_y(nd))
	playSfx("speedup")
	playSfx("spirit-return")
	watch(0.4)
	
	if nt == 1 then
		playSfx("low-note0")
	elseif nt == 2 then
		playSfx("low-note4")
	elseif nt == 3 then
		playSfx("low-note5")
	elseif nt == 4 then
		playSfx("low-note3")
	end
	
	watch(2)
end

function activate(me)
	if isFlag(FLAG_FISHCAVE, 0) then
	--if true then
		-- you win
		setFlag(FLAG_FISHCAVE, 1)
		
		entity_idle(n)
		
		fade2(1,0.5,1,1,1)
		watch(0.5)
		entity_setPosition(n, node_x(me), node_y(me))
		fade2(0,1,1,1,1)
		watch(1)
		
		overrideZoom(0.5, 2)
		
		changeForm(FORM_NORMAL)
		entity_idle(n)
		watch(1)
		playSfx("naijagasp")
		entity_animate(n, "agony", -1)
		watch(1)
		
		setCameraLerpDelay(0.001)
		
		doNode(r1, "fishcave1", 1)
		doNode(r2, "fishcave2", 2)
		doNode(r3, "fishcave3", 3)
		doNode(r4, "fishcave4", 4)
		
		screenFadeCapture()
		cam_toEntity(n)
		screenFadeGo(0.5)
		watch(1)
		spawnParticleEffect("fishtrans", entity_x(n), entity_y(n))
		playSfx("invincible")
		playSfx("speedup")
		fade2(1, 2, 1, 1, 1)
		watch(2)
		
		learnSong(SONG_FISHFORM)
		fade2(0, 0.5, 1, 1, 1)
		changeForm(FORM_FISH)
		voice("Naija_Song_FishForm")
		
		setControlHint(getStringBank(39), 0, 0, 0, 10, "", SONG_FISHFORM)
		
		setCameraLerpDelay(0)
		
		overrideZoom(0)
	end
end
