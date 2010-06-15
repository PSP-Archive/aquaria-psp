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

energyGod = 0
naija = 0
noteSung1 = 0
noteSung2 = 0
noteSung3 = 0
foundSong = false
songDelay = 0
singDelay = 8
maxSingDelay = 10
running = false

-- SEE ENERGYTEMPLE_FIRSTSLOT

function singSong(me)
	node = getNodeByName("SONGMOUTH")				
	spawnParticleEffect("EnergyGodSong", node_x(node), node_y(node))
	playSfx("EnergyGodSong")
end

function init(me)
	loadSound("EnergyGodSong")
	
	naija = getNaija()
	
	if isFlag(FLAG_ENERGYGODENCOUNTER, 3) then
		templeStatue = getEntityByName("TempleStatue")
		entity_setState(templeStatue, STATE_BROKEN)
		
		door = node_getNearestEntity(getNode("STATUEEXITDOOR"), "energydoor")
		entity_setState(door, STATE_OPENED)
	end
	
	loadSound("CrumbleFall")
	
	if isFlag(FLAG_ENERGYGODENCOUNTER, 1) then		
		pn = getNodeByName("SONGMOUTH")	
		ent = createEntity("EnergyGodSpirit", "", node_x(pn), node_y(pn))
		fadeOutMusic(1)
	end
	--debugLog(string.format("%d flag is %d", FLAG_ENERGYGODENCOUNTER, getFlag(FLAG_ENERGYGODENCOUNTER)))
	--[[
	if getFlag(FLAG_ENERGYGODENCOUNTER) > 0 then
		debugLog("setting door... closed")
		energyDoor = node_getNearestEntity(me, "EnergyDoor")
		if energyDoor ~= 0 then
			entity_setState(energyDoor, STATE_CLOSED)
		else
			debugLog("Could not find door")
		end
	else
		debugLog("FLAG NOT SET")
	end
	]]--
	--[[
	i = 0
	while (i < 100) do
		debugLog("BLAH")
		i = i + 1
	end
	]]--
	--[[
	energyGod = getEntity("EnergyGod")
	FLAG_ENERGYGODENCOUNTER
	if getStory() >= 5.1 then
		entity_delete(energyGod)
		energyGod = 0
	end	
	]]--
end

function transformScene(me)
	setCutscene(1,1)
	setFlag(FLAG_ENERGYGODENCOUNTER, 3)
	camNode = getNode("ENERGYGODCAM")
	particleNode = getNode("ENERGYGODPARTICLES")
	particleNode2 = getNode("ENERGYGODPARTICLES2")
	entity_swimToNode(naija, me)
	entity_watchForPath(naija)
	entity_flipToNode(naija, camNode)
	entity_idle(naija)
	entity_clearVel(naija)
	watch(0.5)
	cam_toNode(camNode)
	
	templeStatue = getEntityByName("TempleStatue")
	entity_setState(templeStatue, STATE_BREAK)
	playSfx("CrumbleFall")
	crumbleNode = getNodeByName("CRUMBLEPARTICLES")	
	spawnParticleEffect("EnergyGodStatueCrumble", node_x(crumbleNode), node_y(crumbleNode))
	
	watch(2.3)
	
	esetv(naija, EV_LOOKAT, 0)
	
	playSfx("RockHit-Big")
	
	shakeCamera(8, 2)
	spawnParticleEffect("EnergyGodStatueDust", node_x(particleNode), node_y(particleNode))
	
	entity_animate(naija, "look-45", LOOP_INF, LAYER_HEAD)
	
	cam_toNode(getNode("ENERGYGODPARTICLESCAM"))

	while entity_isAnimating(templeStatue) do
		watch(FRAME_TIME)
	end	
	
	watch(2)
	
	setSceneColor(1, 0.6, 0.5, 4)
	
	spawnParticleEffect("EnergyGodEnergy", node_x(particleNode), node_y(particleNode))
	
	watch(2)
	
	spawnParticleEffect("EnergyGodSend", node_x(particleNode2), node_y(particleNode2))
	watch(0.5)
	voice("EnergyGodTransfer")
	entity_animate(naija, "checkoutEnergy")
	watch(1.5)
	
	
	
	watch(0.5)
	cam_toEntity(naija)
	
	setNaijaHeadTexture("Pain")
	entity_idle(naija)
	playSfx("NaijaZapped")
	setSceneColor(1, 0.5, 0.5, 1)
	entity_animate(naija, "energyStruggle", LOOP_INF)
	
	spawnParticleEffect("EnergyGodTransfer", entity_x(naija), entity_y(naija))
	watch(3.5)
	entity_animate(naija, "energyStruggle2", LOOP_INF)
	watch(1.0)
	
	learnSong(SONG_ENERGYFORM)
	changeForm(FORM_ENERGY)
	setSceneColor(1, 1, 1, 10)
	entity_idle(naija)
	playMusic("archaic")
	voice("NAIJA_ENERGYFORM")
	watch(2)
	entity_animate(naija, "checkoutEnergy")
	while entity_isAnimating(naija) do
		watch(FRAME_TIME)
	end
	watch(0.5)
	
	setCutscene(0)
	
	esetv(naija, EV_LOOKAT, 1)
	setControlHint(getStringBank(37), 0, 0, 0, 10, "", SONG_ENERGYFORM)
	
	door = node_getNearestEntity(getNode("STATUEEXITDOOR"), "EnergyDoor")
	entity_setState(door, STATE_OPEN)
end

function songNote(me, songNote)
	if isFlag(FLAG_ENERGYGODENCOUNTER, 2) then
		noteSung1 = noteSung2
		noteSung2 = noteSung3
		noteSung3 = songNote
		songDelay = 0
		
		if noteSung1 == 7 and noteSung2 == 6 and noteSung3 == 5 then
			foundSong = true
			songDelay = 1.2
		end
	end
end

function update(me, dt)
	if running then return end
	running = true
	if isFlag(FLAG_ENERGYGODENCOUNTER, 0) then
		if node_isEntityIn(me, naija) then
			dc = getNode("ENERGYDOORCAM")
			energyDoor = node_getNearestEntity(dc, "EnergyDoor")
			if energyDoor ~= 0 then
				entity_setState(energyDoor, STATE_CLOSE)
			end
			
			
			entity_idle(naija)
			entity_clearVel(naija)
			cam_toNode(getNode("ENERGYDOORCAM"))
			watch(1)
			emote(EMOTE_NAIJAUGH)
			watch(2.1)			
			
			cam_toNode(getNode("ENERGYGODCAM"))
			entity_flipToNode(naija, getNode("ENERGYGODCAM"))
			
			fadeOutMusic(5)
			
			watch(2)
			
			--[[
			sn = getNode("ENERGYSPIRIT")
			spawnParticleEffect("EnergySpirit", node_x(sn), node_y(sn))
			]]--
			--entity_sound(naija, "EnergyGodSong")
			--singSong(me)
			
			
			
			
			pn = getNodeByName("SONGMOUTH")	
			ent = createEntity("EnergyGodSpirit", "", node_x(pn), node_y(pn))
			
			emote(EMOTE_NAIJAWOW)
			watch(1)
			
			emote(EMOTE_NAIJAWOW)
			watch(1)
		
			
			--[[
			singDelay = maxSingDelay
			]]--
			
			cam_toEntity(naija)
			
			setFlag(FLAG_ENERGYGODENCOUNTER, 1)
		end
	elseif isFlag(FLAG_ENERGYGODENCOUNTER, 2) then
		
		if node_isEntityInRange(me, naija, 1000) then
			if singDelay > 0 then
				singDelay = singDelay - dt
				if singDelay < 0 then					
					singDelay = maxSingDelay
					singSong(me)
				end
			end
		end
		if songDelay > 0 then
			songDelay = songDelay - dt
			if songDelay < 0 then
				songDelay = 0
				transformScene(me)
			end
		end
	end
	running = false
end

function activate(me)
	entity_idle(naija)
	entity_clearVel(naija)
	
	cam_toNode(getNode("ENERGYGODCAM"))
	entity_flipToNode(naija, getNode("ENERGYGODCAM"))
	
	singDelay = maxSingDelay
	watch(2)
	
	singSong(me)
	singDelay = maxSingDelay
	
	watch(3)
	
	cam_toEntity(naija)
end
