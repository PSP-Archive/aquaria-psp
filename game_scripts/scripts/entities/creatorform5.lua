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

-- intro animation sequence:
STATE_INTRO				= 1000
STATE_INTRO2			= 1001
STATE_INTRO3			= 1002
-- creator is getting ready to sing:
STATE_SING				= 1003
-- creator is singing a song:
STATE_PLAYSEG			= 1004
-- creator is in pain:
STATE_PAIN				= 1005
-- naija sings a wrong note:
STATE_WRONG				= 1006

STATE_SPAWNSPHERES		= 1007

delay = 0

songNotes 				= {}

songSize 				= 0
curNote 				= 1
noteDelay 				= 0
songLevel 				= 3
waitTime				= 5
userNote				= 1
maxHits					= 5
hits 					= maxHits
-- counts # of successful songs between song levels
songsDone				= 0


setSongSize				= 3

bone_head				= 0

died					= 0

function getHitPerc()
	return hits/maxHits
end

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "CreatorForm5")	
	entity_setAllDamageTargets(me, false)
	
	--entity_generateCollisionMask(me)
	
	entity_setState(me, STATE_INTRO)
	entity_alpha(me, 0)
	
	entity_scale(me, 0.5, 0.5)
	entity_setCullRadius(me, 1024)
	
	bone_head = entity_getBoneByName(me, "Head")
	
	esetv(me, EV_ENTITYDIED, 1)
	
	loadSound("quakeloop1")
	loadSound("quakeloop2")
	loadSound("quakeloop3")
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
	
	loadSound("hellbeast-shot")
end

function entityDied(me)
end

function postInit(me)
	n = getNaija()
	node = getNode("NAIJA5")
	entity_setPosition(n, node_x(node), node_y(node), 2, 0, 0, 1)
	entity_flipToEntity(n, me)
	entity_setTarget(me, n)
	
	node = getNode("LIDOOR2")
	door = node_getNearestEntity(node, "FinalDoor")
	entity_setState(door, STATE_CLOSED)
	
	musicVolume(1, 1)
	playMusic("worship4")
end


function generateSong(size)
	debugLog("generating song")
	songSize = size
	for i=1,size do
		songNotes[i] = math.random(8)-1
    end
end

function update(me, dt)
	if entity_isState(me, STATE_IDLE) then
		delay = delay + dt
		if delay > 2 then
			delay = 0
			entity_setState(me, STATE_SING) 
		end
	elseif entity_isState(me, STATE_WAIT) then
		shots = 0
		e = getFirstEntity()
		while e ~= 0 do
			if eisv(e, EV_TYPEID, EVT_DARKLISHOT) then
				shots = shots + 1
			end
			e = getNextEntity()
		end
		if shots == 0 then
			entity_setState(me, STATE_IDLE)
		end
	elseif entity_isState(me, STATE_PLAYSEG) then
		noteDelay = noteDelay + dt
		if noteDelay > 0.5 - (songLevel*0.02) then
			debugLog(string.format("singing note: %d", songNotes[curNote]))
			noteDelay = 0
			
			sfx = playSfx(getNoteName(songNotes[curNote], "low-"))
			fadeSfx(sfx, 2)
			
			playSfx("")
			
			x, y = entity_getPosition(me)
			dx, dy = getNoteVector(songNotes[curNote], 256)
			x = x + dx
			y = y + dy
			
			t = 1
			
			noteQuad = createQuad(string.format("Song/NoteSymbol%d", songNotes[curNote]), 6)
			quad_alpha(noteQuad, 0)
			quad_alpha(noteQuad, 1, 0.1)
			quad_scale(noteQuad, 3, 3)
			quad_scale(noteQuad, 6, 6, t, 0, 0, 1)
			quad_color(noteQuad, getNoteColor(songNotes[curNote]))
			node = getNode(string.format("SN%d", songNotes[curNote]))
			quad_setPosition(noteQuad, node_x(node), node_y(node))
			quad_delete(noteQuad, t)
			
			if curNote >= songSize then
				entity_setState(me, STATE_SPAWNSPHERES, -1)
			else
				curNote = curNote + 1
			end
		end
	end
	overrideZoom(0.5)
end

function msg(me, msg, v)
	if entity_isState(me, STATE_WAIT) then
		if msg == "died" then
			died = died + 1
			if died == 3 then
				entity_setState(me, STATE_PAIN)
			end
		end
	end
end

incut = false

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		died = 0
		entity_animate(me, "idle", -1)
		
		e = getFirstEntity()
		while e ~=0 do
			if eisv(e, EV_TYPEID, EVT_DARKLISHOT) then
				
			end
			e = getNextEntity()
		end
		
	elseif entity_isState(me, STATE_INTRO) then
		entity_idle(n)
		disableInput()
		cam_toEntity(me)
		
		entity_alpha(me, 0)
		entity_alpha(me, 1, 2)
		entity_setStateTime(me, 2)
	elseif entity_isState(me, STATE_INTRO2) then
		entity_setStateTime(me, entity_animate(me, "jumpOut"))
	elseif entity_isState(me, STATE_INTRO3) then
		entity_setStateTime(me, entity_animate(me, "land"))
	elseif entity_isState(me, STATE_SING) then
		-- animate?
		entity_setStateTime(me, 0.1)
	elseif entity_isState(me, STATE_PLAYSEG) then
		curNote = 1
		generateSong(setSongSize)
	elseif entity_isState(me, STATE_WAIT) then
		userNote = 1
	elseif entity_isState(me, STATE_PAIN) then
		--bone_damageFlash(me, entity_getBoneByIdx(me, 0))
		
		for i=0,50 do
			bone_damageFlash(entity_getBoneByIdx(me, i))
		end
		
		entity_setStateTime(me, entity_animate(me, "pain"))
	elseif entity_isState(me, STATE_WRONG) then
		entity_setStateTime(me, entity_animate(me, "wrong"))
	elseif entity_isState(me, STATE_ATTACK) then
		entity_setStateTime(me, entity_animate(me, "attack"))
		for i=1,3 do
			sn = math.random(8)-1
			node = getNode(string.format("SN%d", sn))
			ent = createEntity("Mutilus", "", node_x(node), node_y(node))
		end
	elseif entity_isState(me, STATE_TRANSITION) then
		if not incut then
			entity_setStateTime(me, 99)
			incut = true
			entity_idle(n)
			cam_toEntity(me)
			overrideZoom(0.9)
			watch(entity_animate(me, "die"))
			watch(3)
			overrideZoom(0.8)
			shakeCamera(2, 4)
			loop = playSfx("quakeloop1")
			watch(4)
			overrideZoom(0.7)
			shakeCamera(20, 4)
			fadeSfx(loop, 0.5)
			loop = playSfx("quakeloop2")
			watch(4)
			overrideZoom(0.6)
			fade2(1, 4, 1, 1, 1)
			shakeCamera(100, 4)
			fadeSfx(loop, 0.5)
			loop = playSfx("quakeloop3")
			watch(4)
			entity_setStateTime(me, 0.1)
			incut = false
			fadeSfx(loop, 4)
		end
	elseif entity_isState(me, STATE_DELAY) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_SPAWNSPHERES) then
		entity_setStateTime(me, entity_animate(me, "attack"))
	end
end

ic = false

function exitState(me)
	if entity_isState(me, STATE_INTRO) then
		entity_setState(me, STATE_INTRO2)
	elseif entity_isState(me, STATE_SPAWNSPHERES) then
		if not ic then
			ic = true
			
			for n=1,songSize do
				x, y = entity_getPosition(me)
				dx, dy = getNoteVector(songNotes[n], 440)
				x = x + dx
				y = y + dy
				
				e = createEntity("dark-li-shot", "", x, y)
				entity_msg(e, "note", songNotes[n])
				entity_msg(e, "sd", n*(1.0-((1.0-getHitPerc())*0.1)))
				
				entity_setMaxSpeedLerp(e, 1.0+((1.0-getHitPerc())*0.5))
				
				--[[
				t = 3- (3-songLevel)*0.5
				if t <= 0 then
					t = 0.5
				end
				]]--
				--wait(1)
			end	
			
			entity_setState(me, STATE_WAIT)
			
			ic = false
		end
	elseif entity_isState(me, STATE_INTRO2) then
		entity_setState(me, STATE_INTRO3)
	elseif entity_isState(me, STATE_INTRO3) then
		entity_idle(n)
		enableInput()
		cam_toEntity(n)
		
		entity_setState(me, STATE_IDLE)
		--[[
	elseif entity_isState(me, STATE_WAIT) then
		entity_setState(me, STATE_ATTACK)
		]]--
	elseif entity_isState(me, STATE_TRANSITION) then
		-- transition to finalboss02
		loadMap("FinalBoss02", "ENTER")
	elseif entity_isState(me, STATE_SING) then
		entity_setState(me, STATE_PLAYSEG)
	elseif entity_isState(me, STATE_PAIN) then
		songLevel = songLevel + 1
		hits = hits - 1
		debugLog("test")
		if hits <= 0 then
			entity_setState(me, STATE_TRANSITION)
		else
			entity_setState(me, STATE_IDLE)
		end
	elseif entity_isState(me, STATE_WRONG) then
		entity_setState(me, STATE_ATTACK)
	elseif entity_isState(me, STATE_ATTACK) then
		bx, by = bone_getWorldPosition(bone_head)
		for i=1,3,1 do
			createEntity("Mutilus", "", bx, by)
		end
		entity_setState(me, STATE_DELAY, 8)
	elseif entity_isState(me, STATE_DELAY) then
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function animationKey(me, key)
end

function hitSurface(me)
end

function songNote(me, note)
	-- song spheres handle the work now
--[[
	if entity_isState(me, STATE_WAIT) then
		if note == songNotes[userNote] then
			userNote = userNote + 1
			if userNote > songSize then
				songsDone = songsDone + 1
				if songsDone >= 3 then
					songLevel = songLevel + 1
					songsDone = 0
				end
				entity_setState(me, STATE_PAIN)
			end
		else
			entity_setState(me, STATE_WRONG)
		end
	end
]]--
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

