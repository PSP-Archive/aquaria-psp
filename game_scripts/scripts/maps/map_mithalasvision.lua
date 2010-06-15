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

dofile("scripts/maps/finalcommon.lua")

function hasQuit()
	if not (getEnqueuedState() == "") then
		fade2(1, 0, 1, 1, 1)
		return true
	end
	return false
end

function quickFlash(t)
	fade2(1, t, 1, 1, 1)
	playSfx("memory-flash", 0, 0.5)
	watch(t)
	fade2(0, t, 1, 1, 1)
	watch(t)
end

function init()
	test = false
	
	if test then return end
	
	setCutscene(1,1)
	
	toggleDamageSprite(false)
	fade2(0, 1, 1, 1, 1)
	
	--return
	
	setOverrideVoiceFader(0.6)
	
	entity_setPosition(getNaija(), 0, 0)

	
	camDummy = createEntity("Empty")
	
	city1 = getNode("City1")
	city2 = getNode("City2")
	city3 = getNode("City3")
	
	family1 = getNode("Family1")
	
	statue1 = getNode("Statue1")
	statue2 = getNode("Statue2")
	
	priests1 = getNode("Priests1")
	priests2 = getNode("Priests2")
	
	creator1 = getNode("Creator1")
	
	execution1 = getNode("Execution1")
	execution2 = getNode("Execution2")
	
	mithala1 = getNode("Mithala1")
	
	black1 = getNode("Black1")
	
	rot1 = getNode("Rot1")
	rot2 = getNode("Rot2")
	
	drask1 = getNode("Drask1")
	drask2 = getNode("Drask2")
	
	priestCenter = getNode("PriestCenter")
	p1 = getNode("P1")
	p2 = getNode("P2")
	
	priest1 = 0
	priest2 = 0
	
	e = getFirstEntity()
	while e ~= 0 do
		if entity_isName(e, "priestnormal") then
			if priest1 == 0 then
				priest1 = e
			else
				priest2 = e
				break
			end
		end
		e = getNextEntity()
	end
	
	
	syncTime = 0.9
	
		
	-- 1
	----------------------------------------
	overrideZoom(0.6)
	entity_warpToNode(camDummy, city1)
	cam_toEntity(camDummy)
	watch(syncTime)

	n = getNaija()
	entity_setPosition(n, 0, 0)
	
	playMusicOnce("fallofmithalas")
	
	entity_setPosition(camDummy, node_x(city2), node_y(city2), 10, 0, 0, 1)

	overrideZoom(0.5, 10.1)
	
	
	fade2(0, 0, 1, 1, 1)
	fadeIn(1)
	
	voice("naija_vision_mithalas1")
	
	while entity_isInterpolating(camDummy) do
		watch(FRAME_TIME)
	end
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	----------------------------------------
	
	-- 2, family
	----------------------------------------
	overrideZoom(0.6)
	entity_warpToNode(camDummy, family1)
	cam_toEntity(camDummy)
	watch(syncTime)
	
	overrideZoom(0.3)
	
	overrideZoom(0.6, 20)
	
	fade2(0, 1, 1, 1, 1)
	watch(1)
	
	voice("naija_vision_mithalas2")
	
	watch(4)
	
	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	----------------------------------------

	
	-- 3,4,5, statue
	----------------------------------------
	overrideZoom(0.9, 0.1)
	entity_warpToNode(camDummy, statue1)
	cam_toEntity(camDummy)
	watch(syncTime)
		
	entity_setPosition(camDummy, node_x(statue2), node_y(statue2), 10, 0, 0, 1)
	overrideZoom(0.3, 50)
	
	fade2(0, 1, 1, 1, 1)
	
	voice("naija_vision_mithalas3")
	watch(1)
	
	while entity_isInterpolating(camDummy) do
		watch(FRAME_TIME)
	end
	
	watchForVoice()
	
	fade2(1, 6)
	watch(2)
	
	voice("naija_vision_mithalas4")
	watchForVoice()
	
	watch(1)
	voice("naija_vision_mithalas5")
	watchForVoice()
	
	fadeOut(0.1)
	watch(0.5)
	fade2(0)
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	
	fadeIn(0.1)
	----------------------------------------
	
	
	-- 6, priests
	----------------------------------------
	overrideZoom(0.9, 0.1)
	entity_warpToNode(camDummy, priests1)
	cam_toEntity(camDummy)
	watch(syncTime)
	
	entity_setPosition(camDummy, node_x(priests2), node_y(priests2), 10, 0, 0, 1)
	overrideZoom(0.5, 30)
	
	fade2(0, 1, 1, 1, 1)
	
	voice("naija_vision_mithalas6")

	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	----------------------------------------
	
	-- 7, creator face statue
	----------------------------------------
	overrideZoom(0.5)
	entity_warpToNode(camDummy, creator1)
	cam_toEntity(camDummy)
	watch(syncTime)

	overrideZoom(0.99, 30)
	
	fade2(0, 1, 1, 1, 1)
	
	voice("naija_vision_mithalas7")
	watchForVoice()
	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	----------------------------------------
	
	
	-- 8, priests w/ particles
	----------------------------------------
	overrideZoom(0.8)
	entity_warpToNode(camDummy, priestCenter)
	cam_toEntity(camDummy)
	watch(syncTime)

	overrideZoom(0.5, 30)
	
	spawnParticleEffect("PriestExperiment", node_x(priestCenter), node_y(priestCenter))
	
	setSceneColor(0.5, 0.5, 1)
	entity_setColor(priest1, 0, 0, 0)
	entity_setColor(priest2, 0, 0, 0)
	
	entity_animate(priest1, "magic", -1)
	entity_animate(priest2, "magic", -1)
	
	entity_setPosition(priest1, node_x(p1), node_y(p1))
	entity_setPosition(priest2, node_x(p2), node_y(p2))
	
	entity_flipToEntity(priest1, priest2)
	entity_flipToEntity(priest2, priest1)
	
	fade2(0, 1, 1, 1, 1)
	
	voice("naija_vision_mithalas8")
	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	
	setSceneColor(1, 1, 1)
	entity_setColor(priest1, 1, 1, 1)
	entity_setColor(priest2, 1, 1, 1)
	----------------------------------------
	
	-- 9, 10
	----------------------------------------
	-- to black
	overrideZoom(0.9)
	entity_warpToNode(camDummy, black1)
	cam_toEntity(camDummy)
	watch(syncTime)
	
	fade2(0, 0.5, 1, 1, 1)
	
	toggleBlackBars(1)
	
	showImage("visions/mithalas/00")
	
	voice("naija_vision_mithalas9")
	watchForVoice()
	
	watch(1)
	
	voice("naija_vision_mithalas10")
	watchForVoice()
	
	hideImage()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	
	toggleBlackBars(0)
	----------------------------------------
	
	--[[
	-- 9, execution
	----------------------------------------
	overrideZoom(0.99, 0.1)
	entity_warpToNode(camDummy, execution2)
	cam_toEntity(camDummy)
	watch(syncTime)

	overrideZoom(0.4, 30)
	
	setSceneColor(1, 0.8, 0.8)
	
	fade2(0, 1, 1, 1, 1)
	
	voice("naija_vision_mithalas9")
	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	
	setSceneColor(1, 1, 1)
	----------------------------------------
	
	
	-- 10, mithala "transformation"
	----------------------------------------
	overrideZoom(0.99, 0.1)
	entity_warpToNode(camDummy, mithala1)
	cam_toEntity(camDummy)
	watch(syncTime)
	
	setSceneColor(1, 0.5, 0.5)

	overrideZoom(0.4, 30)
	
	fade2(0, 1, 1, 1, 1)
	
	voice("naija_vision_mithalas10")
	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	
	setSceneColor(1, 1, 1, 0.1)
	----------------------------------------
	]]--
	
	
	-- 11, rot spread
	----------------------------------------
	overrideZoom(0.9, 0.1)
	entity_warpToNode(camDummy, rot1)
	cam_toEntity(camDummy)
	watch(syncTime)

	overrideZoom(0.4, 30)
	
	setSceneColor(1, 1, 1)
	setSceneColor(1, 0.1, 0.1, 15)
	
	fade2(0, 1, 1, 1, 1)
	entity_setPosition(camDummy, node_x(rot2), node_y(rot2), 15, 0, 0, 1)
	
	voice("naija_vision_mithalas11")
	
	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	
	setSceneColor(1, 1, 1, 0.1)
	----------------------------------------
	
	
	-- 12, drask
	----------------------------------------
	overrideZoom(0.6)
	entity_warpToNode(camDummy, drask1)
	cam_toEntity(camDummy)
	watch(syncTime)

	overrideZoom(0.9, 30)
	
	fade2(0, 1, 1, 1, 1)
	entity_setPosition(camDummy, node_x(drask2), node_y(drask2), 9, 0, 0, 1)
	
	voice("naija_vision_mithalas12")
	watchForVoice()
	
	voice("naija_vision_mithalas13")
	watch(2)
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	----------------------------------------
	
	
	-- 13, confronting priests
	----------------------------------------
	overrideZoom(0.5)
	entity_warpToNode(camDummy, priestCenter)
	cam_toEntity(camDummy)
	watch(syncTime)
	
	
	spawnParticleEffect("priesttransport", node_x(priestCenter), node_y(priestCenter))
	
	entity_alpha(priest1, 0)
	entity_alpha(priest2, 0)

	overrideZoom(0.8, 30)
	
	fade2(0, 1, 1, 1, 1)
	
	
	watchForVoice()
	
	fade2(1, 0.5, 1, 1, 1)
	watch(0.5)
	----------------------------------------
	
	-- to black
	overrideZoom(0.5)
	entity_warpToNode(camDummy, black1)
	cam_toEntity(camDummy)
	watch(syncTime)
	
	fade2(0, 0.5, 1, 1, 1)
	watch(0.5)
	
	-- 14, breaking up
	----------------------------------------
	watch(1)
	quickFlash(0.2)
	watch(0.2)
	quickFlash(0.2)
	watch(0.2)
	
	watch(0.5)
	
	quickFlash(0.1) watch(0.1)
	quickFlash(0.1) watch(0.1)
	quickFlash(0.1) watch(0.1)
	
	watch(0.5)
	
	voice ("naija_vision_mithalas14")
	
	while isStreamingVoice() do
		watch(0.5 + math.random(2))
		quickFlash(0.2)
		watch(0.2)
		quickFlash(0.2)
		watch(0.2)
	end
	
	watchForVoice()
	----------------------------------------
	
	-- 15, picture, talk to the guy
	----------------------------------------
	fade2(1, 1, 1, 1, 1)
	fade2(0, 0.5)
	

	
	voice("mithalapassing")
	
	
	watchForVoice()
	-- "who am i?"
	----------------------------------------	
	
	
	-- ending
	----------------------------------------
	hideImage()
	overrideZoom(1, 1)
	watch(1)

	setOverrideVoiceFader(-1)
	----------------------------------------
	
	
	-- take us out
	----------------------------------------
	--loadMap("EnergyTemple05", "RETURN")
	----------------------------------------
	
	
	
	-- test end
	----------------------------------------
	
	if test then
		fade2(0, 0.5, 1, 1, 1)
		
		overrideZoom(-1)
		
		cam_toEntity(n)
	else
		learnSong(SONG_BEASTFORM)
		watch(0.5)
		entity_idle(n)
		changeForm(FORM_BEAST)
		--voice("naija_song_beastform")
		
		setCutscene(0)
		loadMap("cathedral04", "naijadone")
	end
	
	setCutscene(0)
	

	----------------------------------------
end


