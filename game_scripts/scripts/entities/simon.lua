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

-- Simon Says: "Eight Eyed Monster!"
-- BOROMAL
dofile("scripts/entities/entityinclude.lua")

-- idle : waiting for user to click
-- playSeg : playing segment of song
-- wait : wait for user input
-- game over : user was too slow
-- victory

STATE_PLAYSEG		= 1000
STATE_WAIT			= 1001
STATE_GAMEOVER		= 1002
STATE_VICTORY		= 1003

songLen = 8
waitTime = 4.5
curNote = 1
onNote = 1
noteDelay = 0
userNote = 1

song = {}
eye = {}
center = 0
centerEye = 0
body = 0

idolWeight = 200

-- note: check this against creatorform5 when its done
function generateSong()
	for i=1, 10 do
		song[i] = math.random(7)
    end
end

function init(me)
	setupEntity(me)
	entity_initSkeletal(me, "Simon")
	entity_setEntityType(me, ET_NEUTRAL)	
	
	entity_setActivation(me, AT_CLICK, 64, 512)
	for	i=1, 8 do
		eye[i] = entity_getBoneByIdx(me, i)
		--entity_getBoneByName(string.format("Eye%d",i))
		bone_setColor(eye[i], getNoteColor(i-1))
		bone_alpha(eye[i], 0)
	end
	center = entity_getBoneByName(me, "Center")	
	centerEye = entity_getBoneByName(me, "CenterEye")
	body = entity_getBoneByName(me, "Body")
	skirtLeft = entity_getBoneByName(me, "SkirtLeft")
	skirtRight = entity_getBoneByName(me, "SkirtRight")
	
	entity_animate(me, "idle")
	
	entity_setState(me, STATE_IDLE)
	
	entity_offset(me, 0, 20, 2, -1, 1, 1)
	
	entity_setCullRadius(me, 1024)
	
	bone_setSegs(body, 2, 32, 0.3, 0.3, -0.018, 0, 6, 1)
	bone_setSegs(skirtLeft, 2, 32, -0.3, 0.3, -0.018, 0, 6, 1)
	bone_setSegs(skirtRight, 2, 32, -0.3, 0.3, -0.018, 0, 6, 1)
	entity_setUpdateCull(me, 2500)
end

function postInit(me)
	if entity_isFlag(me, 1) then
		-- FormUpgradeEnergy2
		ent = createEntity("upgrade-wok", "", entity_getPosition(me))
		entity_setWeight(ent, idolWeight)
	end
end

function update(me, dt)
	if entity_isState(me, STATE_PLAYSEG) then
		noteDelay = noteDelay + dt
		if noteDelay > 0.5 - (onNote*0.02) then
			noteDelay = 0
			playSfx(string.format("MenuNote%d", song[curNote]))
			theEye = eye[song[curNote]+1]
			--bone_alpha(theEye, 0)
			bone_scale(theEye, 1, 0)
			bone_scale(theEye, 1, 1, 0.25, 1, -1)
			bone_alpha(theEye, 1)
			--bone_alpha(, 1, 0.25, 1, -1)
			if curNote >= onNote then
				entity_setState(me, STATE_WAIT, waitTime)
			end
			curNote = curNote + 1
		end
	end
	nx,ny = entity_getPosition(getNaija())
	sx,sy = entity_getPosition(me)
	x = (nx-sx)*0.75
	y = (ny-sy)*0.75
	x,y = vector_cap(x, y, 20)
	bone_setPosition(centerEye, x, y, 1.0)
end

function activate(me)
	--debugLog("ACTIVATE!")
	entity_setActivationType(me, AT_NONE)
	curNote = 1
	userNote = 1
	entity_setState(me, STATE_PLAYSEG)
	musicVolume(0.5, 0.5)
end

function enterState(me)
	colorT = 0.2
	if entity_isState(me, STATE_IDLE) then
		--debugLog("IDLE!")
		entity_setActivationType(me, AT_CLICK)
		generateSong()
		onNote = 1
		curNote = 1
		userNote =1 
		bone_setColor(center, 0.5, 0.5, 1, colorT)
	elseif entity_isState(me, STATE_VICTORY) then
		--debugLog("VICTORY!")
		musicVolume(1, 1)
		if entity_isFlag(me, 0) then
			playSfx("secret")
			ent = createEntity("upgrade-wok", "", entity_getPosition(me))
			entity_alpha(ent, 0)
			entity_alpha(ent, 1, 0.2)
			entity_setWeight(ent, idolWeight)
			entity_setFlag(me, 1)
		else
			playSfx("secret")
			r = randRange(1, 70)
			if r < 10 then
				spawnIngredient("PlantLeaf", entity_getPosition(me))
			elseif r < 20 then
				spawnIngredient("FishOil", entity_getPosition(me))
			elseif r < 30 then
				spawnIngredient("SmallEgg", entity_getPosition(me))
			elseif r < 40 then
				spawnIngredient("SmallEye", entity_getPosition(me))
			elseif r < 50 then
				spawnIngredient("SeaCake", entity_getPosition(me))
			elseif r < 60 then
				spawnIngredient("HandRoll", entity_getPosition(me))
			else
				spawnIngredient("SmallBone", entity_getPosition(me))
			end
		end
		entity_setStateTime(me, 3)
	elseif entity_isState(me, STATE_GAMEOVER) then
		musicVolume(1, 1)
		shakeCamera(10, 1)
		for	i=1, 8 do
			bone_alpha(eye[i], 0)
		end	
		--debugLog("GAMEOVER!")
		playSfx("BeastForm")
		entity_setStateTime(me, 2)
		bone_setColor(center, 1, 0.5, 0.5, colorT)
	elseif entity_isState(me, STATE_WAIT) then
		--[[
		for	i=1, 8 do
			bone_alpha(eye[i], 0)
		end
		]]--	
		--debugLog("WAIT!")
		userNote = 1
		bone_setColor(center, 0.5, 1, 0.5, colorT)
	elseif entity_isState(me, STATE_PLAYSEG) then
		curNote = 1
		--debugLog("PLAYSEG!")
		bone_setColor(center, 0.5, 0.5, 1, colorT)
	end
end

function exitState(me)
	if entity_isState(me, STATE_WAIT) then
		entity_setState(me, STATE_GAMEOVER)
	elseif entity_isState(me, STATE_GAMEOVER) or entity_isState(me, STATE_VICTORY) then
		entity_setState(me, STATE_IDLE)
	end
end

function songNote(me, note)
	--debugLog("songNote!")
	if entity_isState(me, STATE_WAIT) then
		if note == song[userNote] then
			userNote = userNote + 1
			if userNote > onNote then
				if userNote > songLen then
					entity_setState(me, STATE_VICTORY)
				else
					onNote = onNote + 1
					entity_setState(me, STATE_PLAYSEG)
				end
			end
		else
			entity_setState(me, STATE_GAMEOVER)
		end
	end
end

function hitSurface(me)
	--entity_sound(me, "rock-hit")
end

function damage(me, attacker, bone, damageType, dmg)	
	return false
end
