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

STATE_EYESOPEN 		= 1000
STATE_DIE			= 1001
STATE_TENTACLES 	= 1002
STATE_SEED 			= 1003
STATE_DONE			= 1004
STATE_FIRESEEDS		= 1005
STATE_FIRESPIKY		= 1006
STATE_SINGNOTE		= 1007
STATE_VINES			= 1008
STATE_RAGE			= 1009

curCurrent = 0
c1 = 0
c2 = 0

delay = -3
eyes =0
bone_bg = 0
door = 0
noteTimer= 0

rageState = 0

config = 0

lastPlayerNote = 0


-- NOTE: forest goddess has two health bars

-- phase 1 health
maxHits = 50
hits = maxHits

-- phase 2 health
maxRageHits = 70
rageHits = maxRageHits

n = 0
stage = 0
seedDelay = 0
seedNode = 0
started = false
enter = 0

sungNote = -1

noteQuad = 0

vh1 = 0
vh2 = 0
vh3 = 0



shotOff = 0
rageShotMax = 1000
rageShotStart = -rageShotMax
rageShotPos = 0
rageShotAdd = 100
ba = 0

b1 = 0
b2 = 0
b3 = 0
b4 = 0

bd = 1

function clearVines()
	playSfx("vineshrink")
	e = getFirstEntity()
	while e ~= 0 do
		if eisv(e, EV_TYPEID, EVT_FORESTGODVINE) then
			entity_delete(e, 0.3)
		end
		e = getNextEntity()
	end
end

function spawnVines(me, num)
	if config == 0 then
		v1 = getNode("V1")
		v2 = getNode("V2")
		v3 = getNode("V3")
	elseif config == 1 then
		v1 = getNode("V4")
		v2 = getNode("V5")
		v3 = getNode("V1")
	elseif config == 2 then
		v1 = getNode("V2")
		v2 = getNode("V4")
		v3 = getNode("V1")
	end
	config = config + 1
	if config > 2 then
		config = 0
	end
	if num >= 1 then
		vh1 = createEntity("ForestGodVineHead", "", node_x(v1), node_y(v1))
	end
	if num >= 2 then
		vh2 = createEntity("ForestGodVineHead", "", node_x(v2), node_y(v2))
	end
	if num >= 3 then
		vh3 = createEntity("ForestGodVineHead", "", node_x(v3), node_y(v3))
	end
	playSfx("UberVineShrink")
end

function init(me)
	-- NOTE: HEALTH IS SET IN HITS AND MAXHITS
	setupBasicEntity(
	me,
	"",								-- texture
	99,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	128,							-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	90,								-- sprite width
	90,								-- sprite height
	1,						-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	6000,							-- updateCull -1: disabled, default: 4000
	-3
	)
	
	entity_setCull(me, false)
	entity_initSkeletal(me, "ForestGod")
	entity_generateCollisionMask(me)	
	
	entity_animate(me, "idle", LOOP_INF)
	
	entity_setDeathParticleEffect(me, "ForestGodExplode")
	
	
	
	eyes = entity_getBoneByName(me, "eyes")
	bone_bg = entity_getBoneByName(me, "bg")
	
	bone_alpha(eyes, 0)
	
	entity_setTargetPriority(me, 1)	
	entity_setTargetRange(me, 300)
	
	--entity_scale(me, 5.5, 5.5)
	entity_scale(me, 3.2, 3.2)
	
	entity_setState(me, STATE_IDLE)
	entity_setTarget(me, getNaija())
	
	--entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, false)
	entity_setAllDamageTargets(me, false)
	
	node_seed1 = entity_getNearestNode(me, "SEED1")
	node_seed2 = entity_getNearestNode(me, "SEED2")
	node_seed3 = entity_getNearestNode(me, "SEED3")
	node_seed4 = entity_getNearestNode(me, "SEED4")
	
	
	esetv(me, EV_ENTITYDIED, 1)
	
	loadSound("ForestGod-Awaken")
	
	entity_setDamageTarget(me, DT_ENEMY_ENERGYBLAST, false)
	
	cc1 = getNode("cc1")
	cc2 = getNode("cc2")
	
	c1 = getNearestNodeByType(node_x(cc1), node_y(cc1), PATH_CURRENT)
	c2 = getNearestNodeByType(node_x(cc2), node_y(cc2), PATH_CURRENT)
	
	node_setActive(c1, true)
	node_setActive(c2, false)
	
	curCurrent = 1
end

function entityDied(me, ent)
	if ent == vh1 then
		vh1 = 0
	end
	if ent == vh2 then
		vh2 = 0
	end
	if ent == vh3 then
		vh3 = 0
	end
end

function postInit(me)
	door = entity_getNearestEntity(me, "VineDoor")
	entity_setState(door, STATE_OPENED)
	enter = getNode("NAIJAENTER")
	if not isFlag(FLAG_BOSS_FOREST,0) then
		entity_setState(me, STATE_DONE)
		setMusicToPlay("")
	end
	n = getNaija()
	entity_setTarget(me, n)
	
	if entity_isFlag(me, 1) then
		setControlHint(getStringBank(40), 0, 0, 0, 10, "", SONG_NATUREFORM)
		entity_setFlag(me, 2)
	end
end

inCutScene = false
function cutscene(me)
	n = getNaija()
	if not inCutScene then
		inCutScene = true
		pickupGem("Boss-PlantGoddess")
		setFlag(FLAG_BOSS_FOREST,1)
		ent = getFirstEntity()
		while ent ~= 0 do
			--if entity_isName(ent, "UberVine")
			if entity_isName(ent, "SpikyBall")
			or entity_isName(ent, "SporeSeed") then
				entity_setDieTimer(ent, 0.2)
			end
			ent = getNextEntity()
		end
		watch(1)
		changeForm(FORM_NORMAL)
		--fadeOutMusic(4)
		entity_idle(n)
		entity_flipToEntity(n, me)
		pn = getNode("NAIJADONE")
		entity_animate(n, "agony", LOOP_INF)
		watch(2)
		learnSong(SONG_NATUREFORM)
		entity_setPosition(n, node_x(pn), node_y(pn), 2, 0, 0, 1)
		watch(2)
		entity_setFlag(me, 1)
		loadMap("ForestVision")
		--[[
		showImage("Visions/Forest/00")
		watch(0.5)
		voice("Naija_Vision_Forest")
		watchForVoice()
		hideImage()
		
		watch(2)
		entity_idle(n)
		changeForm(FORM_NATURE)
		voice("Naija_Song_NatureForm")
		entity_idle(n)
		entity_setState(door, STATE_OPEN)
		--watchForVoice()
		-- show help text
		]]--
	end
end

function update(me, dt)

	if isNested() then
		return
	end
	
	if entity_isState(me, STATE_DIE) or entity_isState(me, STATE_DONE) then
		return
	end
	
	if noteTimer > 0 then
		--debugLog(string.format("noteTimer: %f", noteTimer))
		noteTimer = noteTimer - dt
		if noteTimer <= 0 then
			--debugLog(string.format("sungNote: %d, lastPlayerNote: %d", sungNote, lastPlayerNote))
			if sungNote == lastPlayerNote then
				entity_setState(me, STATE_EYESOPEN)
			end
		end
	end
	
	if started then
		overrideZoom(0.4, 1)
	end
	if not entity_isState(me, STATE_DIE) and not entity_isState(me, STATE_DONE) then
		entity_handleShotCollisions(me, dt)
		if entity_isEntityInRange(me, n, 128) then
			--entity_hurtTarget(me, 1)
			entity_setTarget(me, n)
			entity_pushTarget(me, 400)
		end
	end
	
	if entity_isState(me, STATE_SEED) then
		seedDelay = seedDelay + dt
		if seedDelay > 2 then
			node = 0
			if seedNode == 0 then
				node = node_seed1
			elseif seedNode == 1 then
				node = node_seed2
			elseif seedNode == 2 then
				node = node_seed3
			elseif seedNode == 3 then
				node = node_seed4
			end
			createEntity("ForestGodSeed", "", node_x(node), node_y(node))
			seedNode = seedNode + 1
			if seedNode > 3 then
				seedNode = 0
			end
			seedDelay = 0
		end			
	end
	
	if isFlag(FLAG_BOSS_FOREST, 0) and node_isEntityIn(enter, n) then
		if not started then
			started = true
			entity_setState(door, STATE_CLOSE)
			playMusic("ForestGod")
		end
	else
		overrideZoom(0)
	end
	
	if entity_isState(me, STATE_TENTACLES) and not entity_isAnimating(me) then
		entity_setState(me, STATE_IDLE)
	end
	
	
	if entity_isState(me, STATE_RAGE) and started then
		delay = delay + dt
		
		if delay >= 18 and rageState ~= 0 then
			rageState = 0
			delay = 0
			
			clearVines()
			
			fade2(1,0,1,1,1)
			fade2(0,0.5,1,1,1)
			bd = -bd
			
		elseif delay >= 9 and rageState < 2 then
			rageState = 2
			
			spawnVines(me, 2)
		elseif delay >= 3 and rageState < 1 then
		

			rageState = 1
			
			fireDelay = 999
			
			rageShotPos = rageShotStart + shotOff
			shotOff = math.random(50)-100
		end
		if rageState == 1 then
			
			fireDelay = fireDelay + dt
			if fireDelay > 0.3 then
				fireDelay = 0
	
				
				s = createShot("ForestGod2", me, 0, entity_x(me)+rageShotPos, entity_y(me)-520)
				shot_setAimVector(s, 0, 400)
				rageShotPos = rageShotPos + rageShotAdd
				if rageShotPos >= (rageShotMax+shotOff) then
					fireDelay = -1000
				end
			end
		end
		
		ba = ba + 3.14*0.4*dt*bd
		off = 3.14/2
		out = 200
		entity_setPosition(b1, entity_x(me) + out*math.sin(ba+off*0), entity_y(me) + out*math.cos(ba+off*0))
		entity_setPosition(b2, entity_x(me) + out*math.sin(ba+off*1), entity_y(me) + out*math.cos(ba+off*1))
		entity_setPosition(b3, entity_x(me) + out*math.sin(ba+off*2), entity_y(me) + out*math.cos(ba+off*2))
		--entity_setPosition(b4, entity_x(me) + out*math.sin(ba+off*3), entity_y(me) + out*math.cos(ba+off*3))
	end
	
	
	if entity_isState(me, STATE_IDLE) and started then
	
	--[[
		if isLeftMouse() then
			entity_setState(me, STATE_RAGE)
		end
	]]--
		
		delay = delay + dt
		if delay > 4 then
			--debugLog("delay > 0!!")
			delay = 0
			
			t = 3
			t2 = 7
			t2 = 10
			if hits/maxHits < 0.5 then
				t = 1.5
				t2 = 5
			end
			--entity_setState(me, STATE_SEED, 9)
			if stage == 0 then
				--entity_setState(me, STATE_FIRESEEDS, t)
				entity_setState(me, STATE_FIRESPIKY, t)
				
			--elseif stage == 1 then
				
			elseif stage == 1 then
				entity_setState(me, STATE_VINES, t2)
			elseif stage == 2 then
				entity_setState(me, STATE_SINGNOTE, t2)
			end
			--[[
			if stage == 0 then
				entity_setState(me, STATE_TENTACLES)
			elseif stage == 1 then
				entity_setState(me, STATE_EYESOPEN, 2)
			elseif stage == 2 then
				entity_setState(me, STATE_TENTACLES)
			elseif stage == 3 then
				entity_setState(me, STATE_SEED, 9)
			end
			]]--
			stage = stage + 1
			if stage > 2 then
				stage = 0
			end
		end
	end
end

function enterState(me, state)
	if entity_isState(me, STATE_EYESOPEN) then
		playSfx("ForestGod-Awaken")
		
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, true)
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
		bone_alpha(eyes, 1, 0.1)
		
		shotSpd = 500
		maxa = 3.14 * 2
		a = 0
		while a < maxa do
			--entity_fireShot(me, 0, 0, math.sin(a)*shotSpd, math.cos(a)*shotSpd, 0, 500, "BlasterFire")
			s = createShot("ForestGod", me, 0)
			shot_setAimVector(s, math.sin(a), math.cos(a))
			a = a + (3.14*2)/16.0
		end
		entity_setStateTime(me, 5.5)
		
		curCurrent = curCurrent + 1
		if curCurrent > 2 then
			curCurrent = 1
		end
		
		if curCurrent == 1 then
			node_setActive(c1, true)
			node_setActive(c2, false)
		elseif curCurrent == 2 then
			node_setActive(c1, false)
			node_setActive(c2, true)
		end
		
	elseif entity_isState(me, STATE_RAGE) then
		setSceneColor(1, 0.7, 0.7, 6)
		shakeCamera(10, 3)
		entity_color(me, 1, 0.5, 0.5, 3)
		delay = 0
		rageState = 0
		bone_alpha(eyes, 1, 0.1)
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, true)
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
		
		b1 = createEntity("spikyblocker", "", entity_x(me), entity_y(me))
		b2 = createEntity("spikyblocker", "", entity_x(me), entity_y(me))
		b3 = createEntity("spikyblocker", "", entity_x(me), entity_y(me))
		--b4 = createEntity("spikyblocker", "", entity_x(me), entity_y(me))
	elseif entity_isState(me, STATE_FIRESEEDS) then
		shotSpd = 1000
		
		a = 0 + math.random(314)/10.0
		maxa = a + 3.14 * 2
		while a < maxa do
			vx = math.sin(a)*shotSpd
			vy = math.cos(a)*shotSpd
			dx,dy = vector_setLength(vx, vy, 64)

			s = createShot("SeedUberVineUnlimited", me, 0, entity_x(me)+dx, entity_y(me)+dy)
			shot_setAimVector(s, vx, vy)
			
			--ent = createEntity("SporeSeed", "", entity_x(me)+dx, entity_y(me)+dy)
			--entity_setDieTimer(ent, 12)

			--entity_setState(ent, STATE_CHARGE2)
			--entity_addVel(ent, vx, vy)
			
			perc = hits/maxHits
			--debugLog(string.format("perc: %f", perc))
			if perc < 0.25 then
				a = a + (3.14*2)/20.0
			elseif perc < 0.5 then
				a = a + (3.14*2)/12.0
			else
				a = a + (3.14*2)/6.0
			end
		end	
	elseif entity_isState(me, STATE_FIRESPIKY) then
		shotSpd = 800
		
		a = 0 + math.random(314)/10.0
		maxa = a + 3.14 * 2
		while a < maxa do
			vx = math.sin(a)*shotSpd
			vy = math.cos(a)*shotSpd
			dx,dy = vector_setLength(vx, vy, 128)
			
			ent = createEntity("SpikyBall", "", entity_x(me)+dx, entity_y(me)+dy)
			
			entity_setBounceType(ent, BOUNCE_REAL)
			entity_setBounce(ent, 1)
			entity_setDieTimer(ent, 6.5)
			--entity_setLife(ent, 7)
			
			entity_setState(ent, STATE_CHARGE1)
			entity_addVel(ent, vx, vy)
			
			perc = hits/maxHits
			if perc < 0.25 then
				a = a + (3.14*2)/8.0
			elseif perc < 0.75 then
				a = a + (3.14*2)/6.0
			else
				a = a + (3.14*2)/4.0
			end
		end			
	elseif entity_isState(me, STATE_IDLE) then
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, false)
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, false)

		bone_alpha(eyes, 0, 0.2)
	elseif entity_isState(me, STATE_TENTACLES) then
		entity_animate(me, "tentacles")
	elseif entity_isState(me, STATE_SEED) then
		delay = 0.5
	elseif entity_isState(me, STATE_DIE) then		
		setSceneColor(0.7, 0.8, 1, 4)
		
		shakeCamera(2, 4)
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, false)
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, false)
		--debugLog("state die!!!")
		entity_setColor(me, 0.5, 0.5, 0.8, 4)
		--bone_setColor(eyes, 0.5, 0.5, 0.8, 4)
		bone_alpha(eyes, 1)
		bone_alpha(eyes, 0, 1)
		entity_setStateTime(me, 4)
		
		clearVines()
		
		if b1 ~= 0 then entity_delete(b1) end
		if b2 ~= 0 then entity_delete(b2) end
		if b3 ~= 0 then entity_delete(b3) end
		if b4 ~= 0 then entity_delete(b4) end
		
		fadeOutMusic(4)
		
	elseif entity_isState(me, STATE_SINGNOTE) then
		sungNote = math.random(7)
		entity_sound(me, string.format("Note%d", sungNote), 500, entity_getStateTime(me))
		noteQuad = createQuad(string.format("Song/NoteSymbol%d", sungNote), 6)
		quad_alpha(noteQuad, 0)
		quad_alpha(noteQuad, 0.8, 2)
		quad_scale(noteQuad, 3, 3, 2, 0, 0, 1)
		quad_setBlendType(noteQuad, BLEND_ADD)
		
		r,g,b = getNoteColor(sungNote)
		quad_color(noteQuad, r*0.8 + 0.2, g*0.8+0.2, b*0.8+0.2)
		
		x,y = entity_getPosition(me)
		quad_setPosition(noteQuad, x, y+130)
	elseif entity_isState(me, STATE_DONE) then
		
		entity_setColor(me, 0.5, 0.5, 0.8)
		--bone_setColor(eyes, 0.5, 0.5, 0.8)
		bone_alpha(eyes, 0)
		hits = 0
		
		node_setActive(c1, false)
		node_setActive(c2, false)
	elseif entity_isState(me, STATE_VINES) then
		num = 1
		perc = hits/maxHits
		if perc < 0.5 then
			num = 3
		elseif perc < 0.75 then
			num = 2
		end
		
		spawnVines(me, num)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if attacker == me then
		return false
	end
	
	if entity_isState(me, STATE_EYESOPEN) then		
		bone_damageFlash(bone_bg)
		bone_damageFlash(eyes)
		hits = hits - dmg
		--debugLog(string.format("hits: %d", hits))
		if hits <= 0 then
			--debugLog("setting state die...")
			--entity_setState(me, STATE_DIE)
			entity_setState(me, STATE_RAGE)
		else
			--entity_setState(me, STATE_IDLE)
		end
		--return true
	end
	if entity_isState(me, STATE_RAGE) then
		bone_damageFlash(bone_bg)
		bone_damageFlash(eyes)
		rageHits = rageHits - dmg
		if rageHits <= 0 then
			entity_setState(me, STATE_DIE)
		end
		
		
	end
	return false
end

function exitState(me, state)
	if entity_isState(me, STATE_EYESOPEN) then
		clearShots()
		entity_setState(me, STATE_IDLE)
		shakeCamera(2, 3)
	elseif entity_isState(me, STATE_SINGNOTE) then
		sungNote = -1
		entity_setState(me, STATE_IDLE)
		if noteQuad ~= 0 then
			quad_delete(noteQuad, 0.5)
			noteQuad = 0
		end
	elseif entity_isState(me, STATE_VINES) then
		entity_setState(vh1, STATE_OFF)
		entity_setState(vh2, STATE_OFF)
		entity_setState(vh3, STATE_OFF)
		vh1 = 0
		vh2 = 0
		vh3 = 0
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_SEED) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_FIRESEEDS) or entity_isState(me, STATE_FIRESPIKY) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_DIE) then
		overrideZoom(0)
		cutscene(me)
		entity_setState(me, STATE_DONE)
	end
end

function songNote(me, note)
	lastPlayerNote = note
	noteTimer = 0.5
end

function songNoteDone(me, note, t)
	noteTimer = 0
end
