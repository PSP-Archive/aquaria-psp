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

-- entity specific
STATE_SOMETHING			= 1000
STATE_SUCK				= 1001
STATE_BLOW				= 1002
 
started = false
chaseDelay = 0
suckDelay = 0
full = false
wasUnderWater = false
outOfWaterSpeed = 1000
n=0
door = 0
enter = 0
maxy = 0

biteDelay = 0
gruntDelay = 0

suckTime = 6 --3
blowTime = 4 --2

fireDelayTime = 0.9
fireDelay = 4

mainHealth = 280
rageHealth = 200
-- NOTE: rage health is not a separate bar, its just a marker of where rage starts


-- stuff.
--[[
mainHealth = 380
rageHealth = 200
]]--


waterLevelMin = 0
waterLevelMax = 0

a = 0

rage = false

function init(me)
--140
	setupBasicEntity(me, 
	"",								-- texture
	mainHealth,						-- health
	1,								-- manaballamount
	2,								-- exp
	1,								-- money
	32,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	256,							-- sprite width
	256,							-- sprite height
	0,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	5000							-- updateCull -1: disabled, default: 4000
	)
	
	entity_flipVertical(me)			-- fix the head orientation
	
	if entity_isFlag(me, 0) then
		entity_initSegments(me, 
		5,								-- num segments
		32,								-- minDist
		64,								-- maxDist
		"SunWorm/Body1",				-- body tex
		"SunWorm/Body5",				-- tail tex
		256,							-- width
		256,							-- height
		0,								-- taper
		0								-- reverse segment direction
		)
		
		entity_setSegmentTexture(me, 1, "SunWorm/Body2")
		entity_setSegmentTexture(me, 2, "SunWorm/Body3")
		entity_setSegmentTexture(me, 3, "SunWorm/Body4")
		entity_initSkeletal(me, "SunWorm")
		entity_animate(me, "idle", LOOP_INF)
	end
	
	entity_setCanLeaveWater(me, true)
	
	wasUnderWater = entity_isUnderWater(me)
	
	entity_setTargetRange(me, 1024)
	entity_setDeathScene(me, true)
	
	loadSound("waterlevelchange")
	loadSound("sunworm-bite")
	loadSound("sunworm-grunt")
	loadSound("sunworm-roar")
	loadSound("BossDieSmall")
	loadSound("BossDieBig")
	loadSound("hellbeast-shot-skull")
	--entity_flipVertical(me)	
	
	esetv(me, EV_WEBSLOW, 80)
end

function postInit(me)
	n = getNaija()
	
	enter = getNode("NAIJAENTER")
	door = entity_getNearestEntity(me, "EnergyDoor")
	entity_setState(door, STATE_OPENED)
	if not entity_isFlag(me, 0) then
		entity_alpha(me, 0)
		entity_delete(me)
	else
		--setCanWarp(false)
	end
	
	node = getNode("SUNWORMMAX")
	maxy = node_y(node)
	
	waterLevelMin = getNode("sunwormwaterlevelmin")
	waterLevelMax = getNode("sunwormwaterlevelmax")
	
	if entity_isFlag(me, 1) then
		setControlHint(getStringBank(41), 0, 0, 0, 10, "", SONG_SUNFORM)
		voice("Naija_Song_SunForm")
		entity_setFlag(me, 2)
	end
end

function update(me, dt)
	odt = dt
	if rage then
		dt = dt * 1.3
	end
	if started then
		if entity_isUnderWater(me) ~= wasUnderWater and not entity_isState(me, STATE_SUCK) then
			wasUnderWater = entity_isUnderWater(me)
			spawnParticleEffect("Splash", entity_x(me), entity_y(me))
			if not entity_isUnderWater(me) then
				--//entity_setMaxSpeed(me, outOfWaterSpeed)
				entity_setMaxSpeedLerp(me, 2, 0.1)
				entity_addVel(me, 0, -800)
			else
				entity_setMaxSpeedLerp(me, 1, 0.8)
			end
		end
		entity_handleShotCollisions(me)
		if entity_hasTarget(me) then
			if entity_isTargetInRange(me, 138) then
				if avatar_isOnWall() then
					shakeCamera(2, 2)
					avatar_fallOffWall()
				end
			end
			if entity_isTargetInRange(me, 96) then
				entity_hurtTarget(me, 1)
				entity_pushTarget(me, 400)
				avatar_fallOffWall()
			end
		end
		if chaseDelay > 0 then
			chaseDelay = chaseDelay - dt
			if chaseDelay < 0 then
				chaseDelay = 0
			end
		end
	end
	if entity_isState(me, STATE_IDLE) then
		biteDelay = biteDelay + dt
		if biteDelay > 0.6 then
			dist = entity_getDistanceToEntity(me, n)
			dist = 1 - (dist / 1024)
			if dist < 0.01 then dist = 0.01 end
			if dist > 1 then dist = 1 end
			playSfx("sunworm-bite", 0, dist)
			biteDelay = 0
		end
		
		gruntDelay = gruntDelay + dt
		if gruntDelay > 2 then
			dist = entity_getDistanceToEntity(me, n)
			dist = 1 - (dist / 1024)
			if dist < 0.01 then dist = 0.01 end
			if dist > 1 then dist = 1 end
			playSfx("sunworm-grunt", 0, dist)
			spawnParticleEffect("bubble-release", entity_x(me), entity_y(me))
			gruntDelay = 0
		end
	end
	if entity_getState(me)==STATE_IDLE or entity_isState(me, STATE_BLOW) then
		if not started then
			if not entity_hasTarget(me) then
				--entity_findTarget(me, 2000)
				--if not started then
				if node_isEntityIn(enter, n) then
					started = true
					playSfx("sunworm-roar")
					shakeCamera(10, 2)
					entity_setTarget(me, n)
					entity_setState(door, STATE_CLOSE)
					playMusic("bigboss")	
					emote(EMOTE_NAIJAUGH)
				end
			end
		else

			overrideZoom(0.3, 1)
			--if chaseDelay==0 then
			if not entity_isUnderWater(me) then
				entity_setMaxSpeed(me, outOfWaterSpeed)
				entity_addVel(me, -2000*dt)
			end			
			if entity_isUnderWater(me) then
				if entity_isState(me, STATE_IDLE) then
					entity_setMaxSpeed(me, 400)
					entity_moveTowardsTarget(me, dt, 1000)
				--[[
					
					if not entity_isTargetInRange(me, 512) then
						entity_moveTowardsTarget(me, dt, 1000)
					elseif not entity_isTargetInRange(me, 128) then
						if entity_x(entity_getTarget(me)) > entity_x(me) then
							entity_addVel(me, 1000*dt, 0)
						else
							entity_addVel(me, -1000*dt, 0)
						end
					end
					]]--
				--[[
					if entity_isTargetInRange(me, 1000) then
						entity_setMaxSpeed(me, 380)
						entity_moveTowardsTarget(me, dt, 1000)
					else
						entity_setMaxSpeed(me, 200)
					end
					]]--
				end
				entity_doEntityAvoidance(me, dt, 200, 0.1)
				if entity_getHealth(me) < 4 then
					entity_doSpellAvoidance(me, dt, 64, 0.5);
				end
				
			end
			--entity_moveTowardsTarget(me, dt, 100)

			--end


		end
	end
	
	if entity_isUnderWater(me) then
		entity_doCollisionAvoidance(me, dt, 5, 1)
	else
		entity_doCollisionAvoidance(me, dt, 20, 0.2)
	end
	entity_updateMovement(me, dt)
	entity_rotateToVel(me, 0.1)

	if started then
		if rage then
			mult = 1
			if entity_getHealth(me) < 75 then
				mult = 1.5
			elseif entity_getHealth(me) < 50 then
				mult = 2.0
			elseif entity_getHealth(me) < 35 then
				mult = 10
			elseif entity_getHealth(me) < 20 then
				mult = 20
			end
			fireDelay = fireDelay - dt * mult
			
			if fireDelay < 0 then
				fireDelay = 0
				fireDelay = fireDelayTime
				
				s = createShot("sunworm", me, n)
				shot_setAimVector(s, math.sin(a), math.cos(a))
				a = a + 3.14*0.25
			end
		end
		
		if entity_isState(me, STATE_SUCK) then
			if not entity_isUnderWater(me) then
				--entity_setState(me, STATE_IDLE)
				entity_setPosition(me, entity_x(me), getWaterLevel() + entity_getCollideRadius(me) + 1)
				entity_addVel(me, 0, 10)
			else
				setWaterLevel(getWaterLevel() + 100*dt)
				if (entity_y(me) - entity_getCollideRadius(me) < getWaterLevel()) then
					entity_setPosition(me, entity_x(me), getWaterLevel() + entity_getCollideRadius(me) + 1)
					entity_addVel(me, 0, 10)
				end
				if getWaterLevel() > node_y(waterLevelMax) then
					setWaterLevel(node_y(waterLevelMax))
				end
				entity_pullEntities(me, entity_x(me), entity_y(me), 2000, 1600, odt) -- 1700
			end
		end
		if entity_isState(me, STATE_BLOW) then
			if not entity_isUnderWater(me) then
				entity_setPosition(me, entity_x(me), getWaterLevel()+2)
				entity_addVel(me, 0, 10)
			end
			setWaterLevel(getWaterLevel() - 120*dt)
			if getWaterLevel() < node_y(waterLevelMin) then
				setWaterLevel(node_y(waterLevelMin))
			end
			
			entity_pullEntities(me, entity_x(me), entity_y(me), 2000, -1600, odt) -- 1700
		end
		
		if not entity_isUnderWater(me) then
			
		else
			if entity_isState(me, STATE_IDLE) and not full and entity_isUnderWater(me) then
				suckDelay = suckDelay + dt
				if suckDelay > 8 then
					suckDelay = 0
					entity_setState(me, STATE_SUCK, suckTime)
				end
			end
	
		end
		if full then
			suckDelay = suckDelay + dt
			if suckDelay > 10 then
				if entity_isUnderWater(me) then
					suckDelay = 0
					entity_setState(me, STATE_BLOW, blowTime)
				end
			end
		end		
	end
	if entity_y(me) < maxy then
		entity_setPosition(me, entity_x(me), maxy)
		vx = entity_velx(me)
		vy = entity_vely(me)
		if vy < 0 then
			vy = -vy
		end
		entity_clearVel(me)
		entity_addVel(me, vx, vy)
	end
end

inCutScene = false
function enterState(me)
	if entity_getState(me)==STATE_IDLE then
		entity_setCanLeaveWater(me, true)
		entity_animate(me, "idle", LOOP_INF)
	elseif entity_isState(me, STATE_BLOW) then
		playSfx("waterlevelchange")
		entity_animate(me, "blow", LOOP_INF)
		avatar_fallOffWall()
	elseif entity_isState(me, STATE_SUCK) then
		entity_setCanLeaveWater(me, false)
		playSfx("waterlevelchange")
		entity_animate(me, "suck", LOOP_INF)
		avatar_fallOffWall()
	elseif entity_isState(me, STATE_DEATHSCENE) then
		if not inCutScene then
			inCutScene = true
			
			entity_setStateTime(me, 99)
			
			entity_offset(me, -10, 0)
			entity_offset(me, 10, 0, 0.01, -1, 1)
			
			shakeCamera(20, 5)
			
			cam_toEntity(me)
			
			for i=1,5 do
				playSfx("BossDieSmall")
				fade(1, 0.2, 1, 1, 1)
				watch(0.2)
				fade(0, 0.2, 1, 1, 1)
				watch(0.2)
			end
			playSfx("sunworm-roar")
			watch(0.7)
			
			playSfx("BossDieSmall")
			fade(1, 0.2, 1, 1, 1)
			watch(0.2)
			fade(0, 0.5, 1, 1, 1)
			watch(0.5)
			playSfx("BossDieBig")
			playSfx("sunworm-roar")
			entity_offset(me, -20, 0)
			entity_offset(me, 20, 0, 0.01, -1, 1)
			fade(1, 1, 1, 1, 1)
			watch(1.2)
			entity_alpha(me, 0, 1)
			entity_setPosition(me, 0, 0)
			fade(0, 0.5, 1, 1, 1)
			
			setSceneColor(1, 1, 1, 3)
			
			cam_toEntity(getNaija())
			
			fadeOutMusic(8)
			if not isForm(FORM_NORMAL) then
				changeForm(FORM_NORMAL)
			end
			entity_setState(door, STATE_OPEN)
			-- this'll get set by SUNWORCAVEBRAIN:
			--setWaterLevel(node_y(getNode("ENDWATERLEVEL")), 3)
			setFlag(FLAG_BOSS_SUNWORM, 1)
			entity_setFlag(me, 1)
			
			
			entity_idle(n)
			watch(2)
			pickupGem("Boss-SunWorm")
			entity_idle(n)
			overrideZoom(1, 3)
			watch(3)
			emote(EMOTE_NAIJAUGH)
			entity_animate(n, "agony", -1)
			watch(3)
			fade2(1, 1, 1, 1, 1)
			watch(1)
			entity_setFlag(me, 1)
			loadMap("SunVision")
			--[[
			showImage("Visions/Veil/00")
			voice("Naija_Vision_SunTemple")
			watchForVoice()
			hideImage()
			learnSong(SONG_SUNFORM)
			watch(1)
			entity_idle(n)
			changeForm(FORM_SUN)
			voice("Naija_Song_SunForm")
			inCutScene = true
			]]--
			inCutScene = false
		end
	end
end

function exitState(me)
	if entity_isState(me, STATE_SUCK) then
		full = true
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_BLOW) then
		full = false
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if started and entity_getHealth(me) < rageHealth and not rage then
		rage = true
		--entity_setColor(me, 1, 0.5, 0.6, 1)
		playSfx("sunworm-roar")
		shakeCamera(10, 3)
		fade2(1, 0, 1, 1, 1)
		fade2(0, 1, 1, 1, 1)
		setSceneColor(1, 0.6, 0.7, 4)
		playMusic("sunworm")
		
		node = getNode("boss2ndwaterlevel")
		setWaterLevel(node_y(node), 0.1)
	end
	if damageType == DT_AVATAR_VINE then
		entity_changeHealth(me, -0.5)
	end
	if rage == true and damageType == DT_AVATAR_LIZAP then
		return false
	end
	return started
end

function hitSurface(me)
end
