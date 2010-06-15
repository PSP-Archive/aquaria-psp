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

minNode = 1
maxNodes = 7

nodes = {}
raceStarted = false
avatarNode = 1
n = 0
timer = 0
lastNodeTimer = 0
maxLastNodeTimer = 20
raceTime = 0

kappa1 = 0
kappa2 = 0

songNote1 = 0
songNote2 = 0
songNote3 = 0
songNote4 = 0

timeRock = 0

maxLaps = 4
lap = 1

incut = false

doStartRace = false

function init(me)
	-- this could be odd.. if you add nodes after... postinit for nodes??
	for i=minNode, maxNodes do
		nodeName = string.format("R0%d", i)
		nodes[i] = getNode(nodeName)
		if nodes[i]==0 then
			--debugLog("Could not find node named: " + nodeName)
		end
	end
	n = getNaija()
	
	--debugLog("setting")
	--node_setCursorActivation(me, true)	
	
	kappa1 = node_getNearestEntity(me, "Kappa")
	kappa2 = entity_getNearestEntity(kappa1, "Kappa")
	
	timeRock = node_getNearestEntity(me, "TimeRock")
	
	loadSound("mia-appear")
	
	setFlag(FLAG_SEAHORSETIMETOBEAT, 90) -- 2:00 -- used to be 90 / 1:30
	
	if isFlag(FLAG_SEAHORSEBESTTIME, 0) then
		setFlag(FLAG_SEAHORSEBESTTIME, getFlag(FLAG_SEAHORSETIMETOBEAT))
	end
	
	entity_msg(timeRock, "time", getFlag(FLAG_SEAHORSEBESTTIME))
end

function postInit(me)
end

function showLap()
	if lap >= maxLaps then
		centerText(getStringBank(853))
	else
		centerText(string.format("%s %d", getStringBank(852), lap))
	end
end

function songNoteDone(me, note, t)
	if not raceStarted then
		songNote1 = songNote2
		songNote2 = songNote3
		songNote3 = songNote4
		songNote4 = note

		if songNote1 == 0 and songNote2 == 3 and songNote3 == 4 and songNote4 == 5 then
			debugLog("setting startRace")
			doStartRace = true
		end
	end
end

function update(me, dt)
	if incut then return end
	n = getNaija()
	if doStartRace then
		debugLog("start race!")
		incut = true
		
		entity_idle(n)
		
		avatarNode = 1
		raceTime = 0
		lastNodeTimer = 0
		
		fade2(1, 0.5, 1, 1, 1)
		watch(0.5)
		
		entity_setPosition(n, node_x(me), node_y(me))
		watch(0.4)
		fade2(0, 1, 1, 1, 1)
		overrideZoom(0.4, 3)
		fadeOutMusic(2)
		watch(2.5)
		entity_setState(kappa1, STATE_CHARGE1)
		entity_setState(kappa2, STATE_CHARGE1)
		watch(1)
		entity_setState(kappa1, STATE_CHARGE2)
		entity_setState(kappa2, STATE_CHARGE2)
		watch(1)
		entity_setState(kappa1, STATE_CHARGE3)
		entity_setState(kappa2, STATE_CHARGE3)
		raceStarted = true
		entity_addVel(n, 0, -800)
		
		setCameraLerpDelay(0.04)
		playMusic("sunworm")
		
		lap = 1
		
		showLap()
		
		doStartRace = false
		incut = false
		
		setTimerTextAlpha(1, 1)
	end
	if raceStarted then
		overrideZoom(0.4, 0.5)
		
		node = entity_getNearestNode(n, "wrongway")
		if node ~= 0 and node_isEntityIn(node, n) then
			centerText(getStringBank(851))
			lost(me)
		else
			raceTime = raceTime + dt
			
			setTimerText(raceTime)
			
			lastNodeTimer = lastNodeTimer + dt
			if lastNodeTimer > maxLastNodeTimer then
				lost(me)
			end
			if node_isEntityIn(nodes[avatarNode], n) then
				k1 = node_getNearestEntity(nodes[avatarNode], "kappa")
				k2 = entity_getNearestEntity(k1, "kappa")
				--debugLog(string.format("crossed node %d", avatarNode))
				playSfx("secret", 0, 0.5)
				lastNodeTimer = 0
				avatarNode = avatarNode + 1
				if avatarNode > maxNodes then
					lap = lap + 1
					showLap()
					if lap < maxLaps then
						avatarNode = 1
						entity_setState(k1, STATE_CHARGE2)
						entity_setState(k2, STATE_CHARGE2)
					else
						entity_setState(k1, STATE_CHARGE3)
						entity_setState(k2, STATE_CHARGE3)
						won(me)
					end
				else
					entity_setState(k1, STATE_CHARGE1)
					entity_setState(k2, STATE_CHARGE1)
				end
			end
		end
		
		debugLog(string.format("raceTime: %d", raceTime))
	end
end

function lost(me)
	raceStarted = false
	
	debugLog("you failed the race")

	playSfx("denied")
	entity_idle(n)
	fadeOutMusic(1.4)
	watch(0.5)
	fade2(1, 1, 1, 1, 1)
	watch(1)
	entity_setPosition(n, node_x(me), node_y(me))
	watch(0.5)
	
	updateMusic()
	
	fade2(0, 1, 1, 1, 1)
	watch(1)
	
	stopRace(me)
end

function won(me)
	raceStarted = false
	
	updateMusic()
	
	
	
	entity_idle(n)
	watch(1)
	entity_flipToEntity(n, timeRock)
	
	if raceTime < getFlag(FLAG_SEAHORSEBESTTIME) then
		setFlag(FLAG_SEAHORSEBESTTIME, raceTime)
		cam_toEntity(timeRock)
		watch(0.5)
		entity_msg(timeRock, "time", raceTime)
		watch(3)
	end
	
	
	
	if raceTime < getFlag(FLAG_SEAHORSETIMETOBEAT) then
		if isFlag(FLAG_COLLECTIBLE_SEAHORSECOSTUME, 0) and getEntity("collectibleseahorsecostume") == 0 then
			nd = getNode("armorloc")
			e = createEntity("collectibleseahorsecostume", "", node_x(nd), node_y(nd))
			entity_alpha(e, 0)
			cam_toEntity(e)
			watch(0.5)
			playSfx("secret")
			playSfx("mia-appear")
			spawnParticleEffect("tinyredexplode", node_x(nd), node_y(nd))
			entity_alpha(e, 1, 1)
			watch(2)
		end
	end
	
	cam_toEntity(n)
	
	stopRace(me)
end

function stopRace(me)
	setCameraLerpDelay(0)
	raceStarted = false
	overrideZoom(0, 5)
	
	setTimerTextAlpha(0, 1)
end


function activate(me)
end


