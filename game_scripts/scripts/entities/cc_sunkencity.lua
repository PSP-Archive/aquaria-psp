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

node_start			= 0
node_climbdown		= 0
node_runaway		= 0
node_inhole			= 0
node_gf				= 0
node_bullies		= 0
node_mom			= 0
node_gotogf			= 0
node_insectcheck	= 0
node_anima			= 0
node_bosswait		= 0
node_girldoor		= 0

girldoor			= 0

bone_flowers = 0

mom = 0
gf = 0
clay = 0

leadDelay = 0

following			= 0
spawnedInsects		= false

STATE_CREATE		= 1000

inAbyss = false

glow = 0

waitForSceneDelay = 0

abyssEndNode = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_NEUTRAL)
	entity_initSkeletal(me, "CC")
	entity_setAllDamageTargets(me, false)
	
	entity_scale(me, 0.6, 0.6)
	
	
	--[[
	entity_setBlendType(me, BLEND_ADD)
	entity_alpha(me, 0.5)
	entity_alpha(me, 1, 1, -1, 1, 1)
	]]--
	
	
	entity_setState(me, STATE_IDLE)
		
	node_start = getNode("START")
	node_climbdown = getNode("CLIMBDOWN")
	node_runaway = getNode("RUNAWAY")
	node_inhole = getNode("INHOLE")
	node_gf = getNode("GF")
	node_bullies = getNode("BULLIES")
	node_anima = getNode("ANIMA")
	node_gotogf = getNode("GOTOGF")
	node_insectcheck = getNode("INSECTCHECK")
	node_mom = getNode("MOM")
	
	node_bosswait = getNode("BOSSWAIT")
	
	--[[
	node_girldoor = getNode("GIRLDOOR")
	girldoor = node_getNearestEntity(node_girldoor)
	entity_setState(girldoor, STATE_OPEN, -1, 1)
	]]--
	
	
	entity_setBeautyFlip(me, false)
	entity_fh(me)
	
	entity_setCullRadius(me, 256)
	
	abyssEndNode = getNode("CCTONGUE")
end

function updateLocation(me)
	debugLog("updateLocation")
	f = getFlag(FLAG_SUNKENCITY_PUZZLE)
	if isMapName("BoilerRoom") then
		--debugLog("IN BOILER ROOM")
		if f < SUNKENCITY_BOSSWAIT then
			--debugLog("setting position 0, 0")
			entity_setPosition(me, 0, 0)
		elseif f == SUNKENCITY_BOSSWAIT then
			--debugLog("setting position to bosswait")
			entity_setPosition(me, node_getPosition(node_bosswait))
		elseif f >= SUNKENCITY_BOSSDONE then
			entity_setPosition(me, 0, 0)
		end
		entity_animate(me, "cry", -1)
	elseif isMapName("Abyss01") then
		if f==SUNKENCITY_BOSSDONE then
			
			if glow == 0 then
				glow = createQuad("Naija/LightFormGlow", 13)
				quad_scale(glow, 12, 12)
			end
			
			--e = entity_getNearestEntity(me, "FinalTongue")
			--entity_setState(e, STATE_OPEN)
			
			nd = getNode("CCLEADSTART")
			
			entity_setPosition(me, node_x(nd), node_y(nd))
			
			entity_animate(me, "float", -1)
			
			entity_flipToEntity(me, n)
			entity_flipToEntity(n, me)
			
			leadDelay = 4
			
			abyssEndNode = getNode("CCTONGUE")
				
			inAbyss = true
			
			waitForSceneDelay = 3
		else
			entity_alpha(me, 0)
			entity_setPosition(me, 0, 0)
		end
	elseif isMapName("SunkenCity01") then
		if f==SUNKENCITY_START then
			entity_animate(me, "draw", -1)
			entity_setPosition(me, node_getPosition(node_start))
		elseif f==SUNKENCITY_CLIMBDOWN then
			entity_setPosition(me, node_getPosition(node_climbdown))
			entity_fhTo(me, true)
			entity_animate(me, "readyToClimb", LOOP_INF)
		elseif f==SUNKENCITY_RUNAWAY then
			entity_setPosition(me, node_getPosition(node_runaway))
			entity_animate(me, "cry", -1)
		elseif f==SUNKENCITY_INHOLE then
			entity_setPosition(me, node_getPosition(node_inhole))
			entity_animate(me, "cry", -1)		
		elseif f==SUNKENCITY_GF then
			--debugLog("setting gf to state done")
			--entity_setState(gf, STATE_DONE)
			entity_setPosition(me, node_getPosition(node_gf))
			entity_animate(me, "cry", -1)
			entity_fh(me)
		elseif f==SUNKENCITY_BULLIES then
			entity_setPosition(me, node_getPosition(node_bullies))
			entity_animate(me, "cry", -1)
		elseif f==SUNKENCITY_ANIMA then
			entity_setPosition(me, node_getPosition(node_anima))
			entity_animate(me, "cry", -1)
		elseif f == SUNKENCITY_BOSSWAIT then
			entity_setPosition(me, 0, 0)
		end
		
		if f >= SUNKENCITY_INHOLE then
			if mom == 0 then
				mom = createEntity("CC_Mother", "", node_x(node_mom), node_y(node_mom))
				entity_fh(mom)
			end
		end
		if f >= SUNKENCITY_GF then
			if mom ~= 0 then
				entity_setState(mom, STATE_SING)
			end
		end
		if f>=SUNKENCITY_BOSSWAIT then
			door = node_getNearestEntity(node_anima, "EnergyDoor")
			if not entity_isState(door, STATE_OPENED) then
				entity_setState(door, STATE_OPENED)
			end
			if f == SUNKENCITY_BOSSDONE then
				debugLog("BOSS DONE!")
				entity_setState(me, STATE_FOLLOW, -1, 1)
			else
				entity_alpha(me, 0.1)
				entity_setPosition(me, 0, 0)
			end
		end
	else
		if f == SUNKENCITY_BOSSDONE then
			debugLog("BOSS DONE!")
			entity_setState(me, STATE_FOLLOW, -1, 1)
		else
			entity_setPosition(me, 0, 0)
		end
	end
end

function postInit(me)
	n = getNaija()
	gf = getEntity("CC_GF")
	
	updateLocation(me)	
end

incutscene = false

function cutsceneintro(me, node)
	incutscene = true
	entity_idle(n)
	entity_flipToEntity(n, me)
	watch(1)
	if node ~= 0 then
		cam_toNode(node)
		watch(1)
	end
end

function cutsceneextro(me)
	cam_toEntity(n)
	incutscene = false
end

function cutscene1(me)
	cutsceneintro(me, node_start)

	cam_toEntity(me)
	overrideZoom(1, 3)
	watch(3)
	
	entity_fh(me)
	--watch(0.5)
	entity_followPath(me, node_start)
	entity_animate(me, "runLow", -1)
	
	--overrideZoom(0.6, 2)
	
	watch(1)
	cam_toEntity(getNaija())
	
	
	while entity_isFollowingPath(me) do watch(FRAME_TIME) end
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_CLIMBDOWN)
	updateLocation(me)
	
	overrideZoom(0)
	
	cutsceneextro(me)
end

function cutscene2(me)
	cutsceneintro(me, node_climbdown)
	
	entity_followPath(me, node_climbdown, SPEED_SLOW)
	entity_animate(me, "climbDown", -1)
	
	while entity_isFollowingPath(me) do watch(FRAME_TIME) end
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_RUNAWAY)
	updateLocation(me)
	
	cutsceneextro(me)
end

function cutscene3(me)
	cutsceneintro(me, node_runaway)
	
	entity_followPath(me, node_runaway)
	entity_animate(me, "runLow", -1)
	
	while entity_isFollowingPath(me) do watch(FRAME_TIME) end
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_INHOLE)
	voiceOnce("Naija_CreatorChildDarkness")
	updateLocation(me)
	
	cutsceneextro(me)
end

-- reunited with mom
function cutscene4(me)
	cutsceneintro(me, node_mom)
	
	entity_setState(mom, STATE_SING)
	
	debugLog("learn anima")
	learnSong(SONG_ANIMA)
	
	watch(2)
	
	entity_setPosition(me, node_x(node_gotogf), node_y(node_gotogf), 1, 0, 0, 1)
	
	while entity_isInterpolating(me) do watch(FRAME_TIME) end
	
	entity_followPath(me, node_gotogf)
	entity_animate(me, "runLow", -1)
	
	while entity_isFollowingPath(me) do watch(FRAME_TIME) end	
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_GF)
	updateLocation(me)
	
	cutsceneextro(me)
end

function cutscene5(me)
	entity_setState(gf, STATE_DONE)
	
	cutsceneintro(me, node_gf)
	
	--msg("THEY BREAK UP!!!")
	watch(1)
	
	land = entity_getNearestNode(me, "CCGFLAND")
	
	entity_setPosition(gf, node_x(land), node_y(land), 1)
	watch(entity_animate(gf, "land"))
	bone_flowers = entity_getBoneByName(gf, "Flowers")
	bone_alpha(bone_flowers, 0, 1)
	watch(1)
	entity_animate(gf, "idle", -1)
	watch(0.1)
	watch(entity_animate(gf, "reach"))
	
	entity_followPath(me, node_gf)
	entity_animate(me, "runLow", -1)
	
	while entity_isFollowingPath(me) do watch(FRAME_TIME) end
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_BULLIES)
	updateLocation(me)
	
	cutsceneextro(me)
end

function cutscene6(me)
	cutsceneintro(me, getNode("KIDSCAM"))
	
	watch(1)
	iter = 0
	e = getFirstEntity()
	while e ~= 0 do
		if entity_isName(e, "CC_Kid") then
			entity_setState(e, STATE_TRANSFORM)
		end
		e = getNextEntity()
	end
	
	watch(3)
	while node_getNumEntitiesIn(node_insectcheck, "Scavenger") < 1 do
		watch(1)
	end
	watch(2.5)
	--while entity_isFollowingPath(me) do watch(FRAME_TIME) end

	updateLocation(me)
	
	cutsceneextro(me)
end

function cutscene7(me)
	entity_setState(gf, STATE_DONE)
	
	cutsceneintro(me, node_bullies)
	
	watch(1)
	
	entity_followPath(me, node_bullies)
	entity_animate(me, "runLow", -1)
	
	while entity_isFollowingPath(me) do watch(FRAME_TIME) end	
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_ANIMA)
	updateLocation(me)
	
	cutsceneextro(me)
end

function cutscene8(me)
	cutsceneintro(me, node_anima)
	watch(1)
	
	door = entity_getNearestEntity(me, "EnergyDoor")
	if door ~= 0 then
		entity_setState(door, STATE_OPEN)
	end
	
	watch(3)
	
	entity_followPath(me, node_anima)
	entity_animate(me, "runLow", -1)
	
	while entity_isFollowingPath(me) do watch(FRAME_TIME) end
	
	setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_BOSSWAIT)
	updateLocation(me)
	
	cutsceneextro(me)
end

function update(me, dt)
	if incutscene then return end
	
	if entity_isState(me, STATE_FOLLOW) then
		xoff = 64
		yoff = 64
		if entity_isfh(n) then
			xoff = -xoff
		end
		entity_flipToEntity(me, n)
		entity_setPosition(me, entity_x(n)+xoff, entity_y(n)+yoff, 0.8)
		return
	end
	
	if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_START) then
		--start = getNode("start")
		if node_isEntityIn(node_start, n) then
			cutscene1(me)
		end
	end
	
	if entity_isEntityInRange(me, n, 256) then
		--if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_START) then
			--cutscene1(me)
		--else
		if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_CLIMBDOWN) then
			cutscene2(me)
		elseif isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_RUNAWAY) then
			cutscene3(me)
		end
		if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_BULLIES) and not spawnedInsects then
			spawnedInsects = true
			cutscene6(me)
		end		
	end
	
	if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_BULLIES) and spawnedInsects then
		num = node_getNumEntitiesIn(node_insectcheck, "Scavenger")
		if num <= 0 then
			cutscene7(me)
		end
	end
	

	if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_GF) then
		if entity_isEntityInRange(me, gf, 200) then
			cutscene5(me)
		end
	end
	
	if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_INHOLE) then
		if following == 0 then
			if entity_isEntityInRange(me, n, 100) and getForm() == FORM_SUN then
				following = 1
				entity_animate(me, "float", -1)			
			end
		else
			if not entity_isEntityInRange(me, n, 128) then
				vx, vy = entity_getVectorToEntity(me, n)
				vector_setLength(vx, vy, 200*dt)
				entity_addVel(me, vx, vy)
				entity_doCollisionAvoidance(me, dt, 2, 1)
				
				entity_flipToEntity(me, n)
			else
				entity_doFriction(me, dt, 400)
			end
			entity_updateMovement(me, dt)
			if node_isEntityIn(node_mom, me) then
				cutscene4(me)
			else
				if not entity_isEntityInRange(me, n, 450) then
					entity_clearVel(me)
					updateLocation(me)
					following = 0
				end
			end
		end
	end
	
	if entity_isState(me, STATE_CREATE) then
		entity_setPosition(clay, entity_x(me)+32, entity_y(me))
		entity_clearVel(clay)
	else
		f = getFlag(FLAG_SUNKENCITY_PUZZLE)
		if f >= SUNKENCITY_BOSSWAIT and f < SUNKENCITY_CLAYDONE then
			clay = entity_getNearestEntity(me, "Clay")
			if clay ~= 0 and entity_isEntityInRange(me, clay, 128) then
				entity_setProperty(clay, EP_MOVABLE, false)
				entity_clearVel(clay)
				entity_setPosition(clay, entity_x(me)+32, entity_y(me), 1)
				entity_setState(me, STATE_CREATE)
				setFlag(FLAG_SUNKENCITY_PUZZLE, f+1)
				debugLog(string.format("flag is now %d", getFlag(FLAG_SUNKENCITY_PUZZLE)))
			end
			
		end
	end
	
	if glow ~= 0 then
		quad_setPosition(glow, entity_getPosition(me))
	end
	
	--[[
	if leadDelay > 0 then
		if entity_isEntityInRange(me, n, 512) then
			leadDelay = leadDelay - dt
			if leadDelay < 0 then
				leadDelay = 0
				
				entity_swimToNode(me, abyssEndNode)
			end
		end
	end
	]]--
	
	if inAbyss then
	--[[
		if entity_isEntityInRange(me, n, 512) then
			e = entity_getNearestEntity(me, "FinalTongue")
			entity_setPosition(me, node_x(nd), node_y(nd))
			inAbyss = false
		end
	]]--
	--[[
		if entity_isFollowingPath(me) and not entity_isEntityInRange(me, n, 1024) then
			entity_stopFollowingPath(me)
			entity_rotate(me, 0, 1, 0, 0, 1)
			--leadDelay = 1
		end
		]]--
		
		if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_BOSSDONE) then
			--debugLog("bossdone")
			n = getNaija()
			entity_flipToEntity(me, n)
			waitForSceneDelay = waitForSceneDelay - dt
			if waitForSceneDelay < 0 and not inCutScene then
				inCutScene = true
				
				e = entity_getNearestEntity(me, "finaltongue")
				
				cam_toEntity(me)
				entity_idle(n)
				entity_flipToEntity(n, me)
				watch(2)
				entity_idle(n)
				emote(EMOTE_NAIJAUGH)
				
				entity_alpha(me, 0, 2)
				fade2(1, 1, 1, 1, 1)
				watch(1)
				
				entity_setPosition(me, node_x(abyssEndNode), node_y(abyssEndNode))
				entity_flipToEntity(me, e)
				watch(1)
				fade2(0, 1, 1, 1, 1)
				watch(1)
				entity_alpha(me, 1, 2)
				watch(2)
				
				entity_setState(e, STATE_OPEN)
				cam_toNode(getNode("TongueCam"))
				watch(4)
				cam_toEntity(me)
				watch(1)
				
				fade2(1, 1, 1, 1, 1)
				watch(1)
				
				cam_toEntity(n)
				
				watch(1)
				
				fade2(0, 1, 1, 1, 1)
				watch(1)
				
				setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_FINALTONGUE)
				
				inCutScene = false
			end
		end
		
		
		
		--[[
		if not entity_isFollowingPath(me) then
			entity_flipToEntity(me, n)
			if node_isEntityIn(abyssEndNode, me) then
				
				if entity_isEntityInRange(me, n, 512) then
					e = entity_getNearestEntity(me, "finaltongue")
					if e ~= 0 then
						entity_setState(e, STATE_OPEN)
					end
				end
			end
		end
		]]--
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		--entity_animate(me, "float", -1)
	elseif entity_isState(me, STATE_CREATE) then
		entity_animate(me, "create", -1)
		entity_setStateTime(me, 4)
	elseif entity_isState(me, STATE_FOLLOW) then
		entity_setPosition(me, entity_x(n), entity_y(n))
		entity_animate(me, "float", -1)
	end
end

function exitState(me)
	if entity_isState(me, STATE_CREATE) then
		entity_animate(me, "idle", -1)
		entity_delete(clay, 0.5)
		if isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_CLAY3) then
			setFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_CLAYDONE)
			entity_alpha(me, 0, 2)
			entity_setPosition(me, 0, 0, 10)
		end
		statue = entity_getNearestEntity(me, "ClayStatue")
		if statue ~= 0 then
			entity_msg(statue, "p")
		end
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
end

function songNoteDone(me, note)
end

function song(me, song)
	if song == SONG_ANIMA and isFlag(FLAG_SUNKENCITY_PUZZLE, SUNKENCITY_ANIMA) then
		cutscene8(me)
	end
end

function activate(me)
end

