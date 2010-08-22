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

-- regular areas
TURTLE_REGULAR		= 0
-- energy upgrade secret, forest 05, openwater03
TURTLE_SECRET1		= 1

seat = 0
seat2 = 0
tame = 0
n = 0
leave = 0
avatarAttached 	= false
liAttached		= false
myFlag = 0
turtleType = TURTLE_REGULAR

light1 = 0
light2 = 0

seen = false

sbank = 0

function init(me)
	n = getNaija()
	
	setupEntity(me, "")
	
	entity_initSkeletal(me, "TransTurtle")
	
	entity_setEntityType(me, ET_NEUTRAL)
	entity_setActivation(me, AT_CLICK, 128, 512)
	
	seat = entity_getBoneByName(me, "Seat")
	seat2 = entity_getBoneByName(me, "Seat2")
	tame = entity_getBoneByName(me, "Tame")
	entity_setCullRadius(me, 1024)
	bone_alpha(seat, 0)
	bone_alpha(seat2, 0)
	bone_alpha(tame, 0)
	
	if isMapName("VEIL01") then
		debugLog("is veil01")
		myFlag = FLAG_TRANSTURTLE_VEIL01
		sbank = 1014
	elseif isMapName("VEIL02") then
		debugLog("is veil02")
		myFlag = FLAG_TRANSTURTLE_VEIL02
		sbank = 1014
	elseif isMapName("OPENWATER03") then
		myFlag = FLAG_TRANSTURTLE_OPENWATER03
		sbank = 1009
	elseif isMapName("FOREST04") then
		myFlag = FLAG_TRANSTURTLE_FOREST04
		sbank = 1010
-- think openwater06 is unused atm
	elseif isMapName("OPENWATER06")	then
		myFlag = FLAG_TRANSTURTLE_OPENWATER06
		sbank = 1009
-- think openwater06 is unused atm
	elseif isMapName("MAINAREA") then
		myFlag = FLAG_TRANSTURTLE_MAINAREA
		sbank = 1008
	elseif isMapName("ABYSS03") then
		myFlag = FLAG_TRANSTURTLE_ABYSS03
		turtleType = TURTLE_REGULAR
		sbank = 1015
	elseif isMapName("FINALBOSS") then
		myFlag = FLAG_TRANSTURTLE_FINALBOSS
		turtleType = TURTLE_REGULAR
		sbank = 1021
	elseif isMapName("FOREST05") then
		myFlag = FLAG_TRANSTURTLE_FOREST05
		turtleType = TURTLE_SECRET1
	elseif isMapName("SEAHORSE") then
		myFlag = FLAG_TRANSTURTLE_SEAHORSE
		turtleType = TURTLE_SECRET1
	end
	
	if myFlag ~= 0 and (not entity_isFlag(me, 0)) then
		setFlag(myFlag, 1)
	end
	
	light1 = entity_getBoneByName(me, "Light1")
	light2 = entity_getBoneByName(me, "Light2")
	
	bone_setBlendType(light1, BLEND_ADD)
	bone_setBlendType(light2, BLEND_ADD)
	
	loadSound("TransTurtle-Light")
	loadSound("transturtle-takeoff")
	
	
end


function lights(me, on, t)
	a = 1
	if not on then
		a = 0
		debugLog("Lights off!")
	else
		debugLog("Lights on!")
	end
	
	bone_alpha(light1, a, t)
	bone_alpha(light2, a, t)
end

function postInit(me)
	leave = entity_getNearestNode(me, "TRANSTURTLELEAVE")
	
	if turtleType == TURTLE_REGULAR then
		debugLog("Regular turtle")
		if isFlag(myFlag, 0) then
			lights(me, false, 0)
		else
			-- turn on ze lights
			lights(me, true, 0)
		end
		
		if sbank ~= 0 then
			if entity_isEntityInRange(me, getNaija(), 600) then
				centerText(getStringBank(sbank))
			end
		end
	else
		debugLog("Special turtle")
		lights(me, true, 0)
	end
	
	
	-- if naija starts on a turtle, ignore the seen/hint
	if entity_isEntityInRange(me, n, 350) then
		seen = true
	end
end

function update(me, dt)

--[[
	if isForm(FORM_BEAST) then
		entity_setActivationType(me, AT_CLICK)
	else
		entity_setActivationType(me, AT_NONE)
	end
	
	if not hasSong(SONG_BEASTFORM) then
		if entity_isEntityInRange(me, n, 512) then
			voiceOnce("Naija_TransportTurtles")
		end
	end
	]]--
	if entity_isFlag(me, 0) then
		--debugLog("flag is 0")
		entity_setActivationType(me, AT_NONE)
	else
		--debugLog("setting click")
		entity_setActivationType(me, AT_CLICK)
	end
	
	if avatarAttached then
		--entity_flipToSame(n, me)
		x,y = bone_getWorldPosition(seat)
		
		entity_setRidingData(me, x, y, 0, entity_isfh(me))
	end
	
	if liAttached then
		x,y = bone_getWorldPosition(seat2)
		entity_setPosition(li, x, y)
		entity_rotate(li, entity_getRotation(me))
		if entity_isfh(me) and not entity_isfh(li) then
			entity_fh(li)
		elseif not entity_isfh(me) and entity_isfh(li) then
			entity_fh(li)
		end
	end
	
	if entity_isEntityInRange(me, n, 300) then
		if not seen then
			emote(EMOTE_NAIJAWOW)
			if anyOtherFlag() then
				setControlHint(getStringBank(226), 0, 0, 0, 5, "transturtle/headicon")
			else
				setControlHint(getStringBank(225), 0, 0, 0, 5, "transturtle/headicon")
			end
			seen = true
		end
	end
	
	if turtleType == TURTLE_REGULAR then
		if isNested() then return end
		if entity_isEntityInRange(me, n, 300) and (not isFlag(myFlag, 1) or entity_isFlag(me, 0)) and entity_isUnderWater(n) then
			entity_idle(n)
			entity_setInvincible(n, true)
			entity_flipToEntity(n, me)
			cam_toEntity(me)
			watch(1.5)
			playSfx("TransTurtle-Light")
			lights(me, true, 1.5)
			watch(2)
			cam_toEntity(n)
			watch(1)
			setFlag(myFlag, 1)
			pickupGem("Turtle")
			entity_setFlag(me, 1)
		end
	else
		if entity_isEntityInRange(me, n, 256) and entity_isFlag(me,0) then
			entity_setFlag(me, 1)
			pickupGem("Turtle")
			--debugLog(string.format("setting %d to 1", myFlag));
			setFlag(myFlag, 1)
		end
	end
end

function isOtherFlag(flag)
	return (myFlag ~= flag and isFlag(flag, 1))
end

function anyOtherFlag()
	if turtleType == TURTLE_REGULAR then
		debugLog("turtle is regular")
		if isOtherFlag(FLAG_TRANSTURTLE_VEIL01) then
			return true
		elseif isOtherFlag(FLAG_TRANSTURTLE_VEIL02) then
			return true
		elseif isOtherFlag(FLAG_TRANSTURTLE_OPENWATER03) then
			return true
		elseif isOtherFlag(FLAG_TRANSTURTLE_FOREST04) then
			return true
		elseif isOtherFlag(FLAG_TRANSTURTLE_MAINAREA) then
			return true
		elseif isOtherFlag(FLAG_TRANSTURTLE_FINALBOSS) then
			return true
		elseif isOtherFlag(FLAG_TRANSTURTLE_ABYSS03) then
			return true
		end
	end
	if turtleType == TURTLE_SECRET1 then
		debugLog("turtle is secret")
		return true
	end
	debugLog("turtle is nothing")
	return false
end

function activate(me)
	--if isForm(FORM_BEAST) then
	--[[
	if turtleType == TURTLE_REGULAR then
		voiceOnce("Naija_TransportTurtles2")
	end
	]]--
	if entity_isFlag(me, 0) then return end
	
	if entity_getRiding(getNaija())~=0 then
		return
	end
	
	if anyOtherFlag() then
		entity_setActivation(me, AT_NONE)
		
		if isFlag(FLAG_FIRSTTRANSTURTLE, 0) then
			x,y = bone_getWorldPosition(tame)
			entity_swimToPosition(n, x, y)
			entity_watchForPath(n)
			entity_flipToEntity(n, me)
			entity_animate(n, "tameTurtle", 0, LAYER_UPPERBODY)
			entity_animate(me, "tame")
			while entity_isAnimating(me) do
				watch(FRAME_TIME)
			end
			entity_idle(n)
			entity_animate(me, "idle")
			watch(0.5)
			-- don't forget this later: 
			setFlag(FLAG_FIRSTTRANSTURTLE, 1)
		end
		li = 0
		if hasLi() then
			li = getLi()
			if entity_isEntityInRange(n, li, 512) then
			else
				fade2(1, 0.2, 1, 1, 1)
				watch(0.2)
				entity_setPosition(li, entity_x(n), entity_y(n))
				fade2(0, 0.5)
				watch(0.5)
			end
		end
		x,y = bone_getWorldPosition(seat)

		entity_swimToPosition(n, x, y)
		entity_watchForPath(n)
		entity_animate(n, "rideTurtle", -1)
		avatarAttached = true
		if entity_isfh(me) and not entity_isfh(n) then
			entity_fh(n)
		elseif not entity_isfh(me) and entity_isfh(n) then
			entity_fh(n)
		end
		
		if li ~= 0 then
			debugLog("here!")
			entity_setState(li, STATE_PUPPET, -1, 1)
			x2,y2 = bone_getWorldPosition(seat2)
			entity_swimToPosition(li, x2, y2)
			entity_watchForPath(li)
			entity_animate(li, "rideTurtle", -1)
			liAttached = true
			entity_setRiding(li, me)
		end
		
		
		entity_setRiding(n, me)
		overrideZoom(0.75, 1.5)
		if isMapName("VEIL01") then
			entity_rotate(me, -80, 2, 0, 0, 1)
		end
		entity_animate(me, "swimPrep")
		while entity_isAnimating(me) do
			watch(FRAME_TIME)
		end
		

		entity_moveToNode(me, leave, SPEED_FAST)
		entity_animate(me, "swim", -1)
		
		playSfx("transturtle-takeoff")
		watch(1)
		fade(1, 1)
		watch(1)
		
		-- HACK: Keep the mouse cursor from reappearing for an instant
		-- when under keyboard or joystick control.
		disableInput()
		
		
		-- rotation
		
		--[[
		VEIL02
		VEIL01
		OPENWATER03
		MAINAREA
		FOREST04
		
		VEIL02
		]]--
				
		-- regular cycle:
		if turtleType == TURTLE_REGULAR then
			if isMapName("VEIL01") then
				if isFlag(FLAG_TRANSTURTLE_ABYSS03, 1) then
					warpNaijaToSceneNode("ABYSS03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FINALBOSS, 1) then
					warpNaijaToSceneNode("FINALBOSS", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_OPENWATER03, 1) then
					warpNaijaToSceneNode("OPENWATER03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_MAINAREA, 1) then
					warpNaijaToSceneNode("MAINAREA", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FOREST04, 1) then
					warpNaijaToSceneNode("FOREST04", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL02, 1) then
					warpNaijaToSceneNode("VEIL02", "TRANSTURTLE")
				end
			elseif isMapName("VEIL02") then
				if isFlag(FLAG_TRANSTURTLE_VEIL01, 1) then
					warpNaijaToSceneNode("VEIL01", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_ABYSS03, 1) then
					warpNaijaToSceneNode("ABYSS03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FINALBOSS, 1) then
					warpNaijaToSceneNode("FINALBOSS", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_OPENWATER03, 1) then
					warpNaijaToSceneNode("OPENWATER03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_MAINAREA, 1) then
					warpNaijaToSceneNode("MAINAREA", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FOREST04, 1) then
					warpNaijaToSceneNode("FOREST04", "TRANSTURTLE")
				end
			elseif isMapName("OPENWATER03") then
				if isFlag(FLAG_TRANSTURTLE_MAINAREA, 1) then
					warpNaijaToSceneNode("MAINAREA", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FOREST04, 1) then
					warpNaijaToSceneNode("FOREST04", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL02, 1) then
					warpNaijaToSceneNode("VEIL02", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL01, 1) then
					warpNaijaToSceneNode("VEIL01", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_ABYSS03, 1) then
					warpNaijaToSceneNode("ABYSS03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FINALBOSS, 1) then
					warpNaijaToSceneNode("FINALBOSS", "TRANSTURTLE")
				end
			elseif isMapName("FOREST04") then
				if isFlag(FLAG_TRANSTURTLE_VEIL02, 1) then
					warpNaijaToSceneNode("VEIL02", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL01, 1) then
					warpNaijaToSceneNode("VEIL01", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_ABYSS03, 1) then
					warpNaijaToSceneNode("ABYSS03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FINALBOSS, 1) then
					warpNaijaToSceneNode("FINALBOSS", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_OPENWATER03, 1) then
					warpNaijaToSceneNode("OPENWATER03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_MAINAREA, 1) then
					warpNaijaToSceneNode("MAINAREA", "TRANSTURTLE")
				end
			elseif isMapName("MAINAREA") then
				if isFlag(FLAG_TRANSTURTLE_FOREST04, 1) then
					warpNaijaToSceneNode("FOREST04", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL02, 1) then
					warpNaijaToSceneNode("VEIL02", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL01, 1) then
					warpNaijaToSceneNode("VEIL01", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_ABYSS03, 1) then
					warpNaijaToSceneNode("ABYSS03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FINALBOSS, 1) then
					warpNaijaToSceneNode("FINALBOSS", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_OPENWATER03, 1) then
					warpNaijaToSceneNode("OPENWATER03", "TRANSTURTLE")
				end
			elseif isMapName("ABYSS03") then
				if isFlag(FLAG_TRANSTURTLE_FINALBOSS, 1) then
					warpNaijaToSceneNode("FINALBOSS", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_OPENWATER03, 1) then
					warpNaijaToSceneNode("OPENWATER03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_MAINAREA, 1) then
					warpNaijaToSceneNode("MAINAREA", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FOREST04, 1) then
					warpNaijaToSceneNode("FOREST04", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL02, 1) then
					warpNaijaToSceneNode("VEIL02", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL01, 1) then
					warpNaijaToSceneNode("VEIL01", "TRANSTURTLE")
				end
			elseif isMapName("FINALBOSS") then
				if isFlag(FLAG_TRANSTURTLE_OPENWATER03, 1) then
					warpNaijaToSceneNode("OPENWATER03", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_MAINAREA, 1) then
					warpNaijaToSceneNode("MAINAREA", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_FOREST04, 1) then
					warpNaijaToSceneNode("FOREST04", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL02, 1) then
					warpNaijaToSceneNode("VEIL02", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_VEIL01, 1) then
					warpNaijaToSceneNode("VEIL01", "TRANSTURTLE")
				elseif isFlag(FLAG_TRANSTURTLE_ABYSS03, 1) then
					warpNaijaToSceneNode("ABYSS03", "TRANSTURTLE")
				end
			end
		end
		
		-- secret:
		if turtleType == TURTLE_SECRET1 then
			if isMapName("SEAHORSE") then
				warpNaijaToSceneNode("FOREST05", "TRANSTURTLE")
			elseif isMapName("FOREST05") then
				warpNaijaToSceneNode("SEAHORSE", "TRANSTURTLE")
			end
		end
	else
		debugLog("TransTurtle: no other flag set")
		playSfx("denied")
		setControlHint(getStringBank(225), 0, 0, 0, 4, "transturtle/headicon")
	end
	--end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1, 0, -1)
	end
end

function exitState(me)
end
