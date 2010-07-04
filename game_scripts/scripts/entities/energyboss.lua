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

-- ================================================================================================
-- Energy Boss
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

STATE_MOVING				= 1001
STATE_MOVEBACK				= 1002
STATE_SHOCKED				= 1003
STATE_FIRE					= 1004
STATE_HITBARRIER			= 1005
STATE_MOVEBACKFROMBARRIER 	= 1006
STATE_COLLAPSE				= 1007
STATE_COLLAPSED				= 1100

ATTACK_UP					= 1
ATTACK_DOWN					= 2
ATTACK_BITE					= 3

attacks = 0

attack = 0
awoken = false
attackDelay = 0
naija = 0

maxMove = 0
maxMove2 = 0
minMove = 0
pushBackHits = 0
maxPushBackHits = 6
moveDelay = 0
fireDelay = 0
fireBit = 0
shotsFired = 0
barrier = 0
maxHits = 3
hits = maxHits
endTextDelay = 0
playedMusic = false
dead = false

orb = 0

bone_jaw = 0
bone_claw = 0
bone_head = 0
bone_body = 0

function damage(me, attacker, bone, damageType, dmg)
	bone_damageFlash(bone, 1)
	if entity_x(me) > node_x(minMove)+50 and not entity_isState(me, STATE_MOVEBACK) and not entity_isState(me, STATE_HITBARRIER) and not entity_isState(me, STATE_MOVEBACKFROMBARRIER) and not entity_isState(me, STATE_COLLAPSE) then
		pushBackHits = pushBackHits + dmg
		if pushBackHits >= maxPushBackHits then
			pushBackHits = 0
			entity_setState(me, STATE_MOVEBACK)
		end
	end
	return false
end

function init(me)	
	setupBasicEntity(
	me,
	"",								-- texture
	30,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	0,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	90,								-- sprite width
	90,								-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	6000,							-- updateCull -1: disabled, default: 4000
	0
	)
	entity_initSkeletal(me, "EnergyBoss")
	entity_setState(me, STATE_IDLE)
	entity_setCull(me, false)
	entity_setCollideWithAvatar(me, true)
	
	entity_setName(me, "EnergyBoss")
	
	--entity_flipHorizontal(me)
		
	entity_scale(me, 1.5, 1.5)
	entity_setTouchDamage(me, 1)
	entity_setTouchPush(me, 1)
	
	entity_setWeight(me, 800)
	
	entity_setMaxSpeed(me, 2000)
	
	entity_generateCollisionMask(me)
	
	bone_jaw = entity_getBoneByName(me, "Jaw")
	bone_claw = entity_getBoneByName(me, "Claw")
	bone_head = entity_getBoneByName(me, "Head")
	bone_body = entity_getBoneByName(me, "Body")
	
	maxMove = getNode("ENERGYBOSSMAXMOVE")
	maxMove2 = getNode("ENERGYBOSSMAXMOVE2")
	minMove = getNode("ENERGYBOSSMINMOVE")
	
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
	
	
	naija = getNaija()
	
	loadSound("EnergyBoss-Attack")
	loadSound("EnergyBoss-Die")
	loadSound("EnergyBoss-Hurt")
	loadSound("BossDieSmall")
end

function postInit(me)
	if not entity_isState(me, STATE_COLLAPSE) then
		if getFlag(FLAG_ENERGYBOSSDEAD)>0 and not entity_isState(me, STATE_COLLAPSED) then
			entity_setPosition(me, node_x(maxMove2), entity_y(me))
			entity_setState(me, STATE_COLLAPSED)
			dead = true
			return
		end
	end

	
	orb = getEntityByID(4)
	holder = getEntityByID(3)
	barrier = getEntityByID(5)
end

function update(me, dt)
	--[[
	if entity_isState(me, STATE_COLLAPSED) then
		if getFlag(FLAG_ENERGYBOSSDEAD)>0 then
			fadeOutMusic(0.5)
		end
	end
	]]--
	if entity_isState(me, STATE_COLLAPSE) then
		if endTextDelay > 0 then
			endTextDelay = endTextDelay - dt
			if endTextDelay < 0 then
				endTextDelay = 0
				
				cam_toEntity(naija)
				changeForm(FORM_NORMAL)
				
				setInvincible(true)
				
				entity_idle(naija)
				entity_animate(naija, "agony", -1)
				entity_flipToEntity(naija, me)
				--voiceOnce("Naija_EnergyBossOver")
				voice("Naija_Vision_EnergyBoss1")
				--entity_idle(naija)
				entity_idle(naija)
				entity_animate(naija, "agony", -1)
				fade2(0.5, 8, 1, 1, 1)
				watchForVoice()
				fade2(1, 1, 1, 1, 1)
				watch(1)
				
				collectibleNode = getNode("COLLECTIBLE")
				ent = createEntity("CollectibleEnergyBoss", "", node_x(collectibleNode), node_y(collectibleNode))
				entity_alpha(ent, 0)
				entity_alpha(ent, 1, 2)
				watch(0.5)
				
				setInvincible(false)
				
				if entity_getHealth(naija) < 1 then
					entity_heal(naija, 1)
				end
				
				loadMap("EnergyTempleVision")
				
				--entity_idle(naija)
				--watch(6.5)
				--[[
				showImage("Visions/EnergyBoss/00")
				voice("Naija_Vision_EnergyBoss2")
				watchForVoice()
				entity_idle(naija)
				hideImage()
				watch(1)
				voice("Naija_Vision_EnergyBoss3")
				]]--

			end
		end
		return
	end
	if entity_isState(me, STATE_COLLAPSED) then
		return
	end
	if not awoken and not playedMusic and getFlag(FLAG_ENERGYBOSSDEAD)==0 then
		if not isFlag(FLAG_OMPO, 4) then 
			if entity_isEntityInRange(me, naija, 1220) then
				emote(EMOTE_NAIJAUGH)
				playedMusic = true
				playMusic("BigBoss")
				--setNaijaHeadTexture("shock", 4)
				entity_setState(me, STATE_INTRO)
			end
		end
	end
	
	if not awoken then return end
	

	if entity_isState(me, STATE_COLLAPSED) then
		return
	end

	
	--[[
	if barrier == 0 then
		barrier = getEntityByID(5)
	end
	]]--
	
	bone = entity_collideSkeletalVsCircle(me, getNaija())
	if bone ~= 0 or entity_x(naija) < entity_x(me) then
		entity_damage(getNaija(), me, 1)
		if entity_y(naija) > entity_y(me)+50 then
			entity_push(getNaija(), 1200, -600, 0.5)
		elseif entity_y(naija) < entity_y(me) then
			entity_push(getNaija(), 1200, 600, 0.5)
		else
			entity_push(getNaija(), 1200, 0, 0.5)
		end
	end
		
	if entity_isState(me, STATE_ATTACK) or entity_isState(me, STATE_INTRO) then
		if not entity_isAnimating(me) then
			if entity_isState(me, STATE_ATTACK) then
				attacks = attacks + 1 
				if attacks >= 1 then
					attacks = 0
					entity_setState(me, STATE_MOVING)
				else
					entity_setState(me, STATE_IDLE)
				end
			else
				entity_setState(me, STATE_IDLE)
			end
		end
	end
	
	--[[
	if entity_isState(me, STATE_HITBARRIER) and not entity_isAnimating(me) then
		entity_setState(me, STATE_MOVEBACKFROMBARRIER)
	end
	]]--


	
	if not entity_isState(me, STATE_MOVEBACK) and not entity_isState(me, STATE_MOVEBACKFROMBARRIER) then
		if entity_x(orb) < entity_x(barrier) then
			if moveDelay > 0 then
				moveDelay = moveDelay - dt
				if moveDelay < 0 then
					moveDelay = 0
				end
			end
		end
	end

	if barrier ~= 0 then
		--debugLog("ENERGYBOSS: has Barrier")
		if entity_isState(barrier, STATE_PULSE) then
			--debugLog("ENERGYBOSS: barrier pulse")
			if not entity_isState(me, STATE_HITBARRIER) and not entity_isState(me, STATE_MOVEBACKFROMBARRIER) then			
				lineBone = entity_collideSkeletalVsLine(me, entity_x(barrier), entity_y(barrier)+500, entity_x(barrier), entity_y(barrier)-500, 8)
				if lineBone ~= 0 then
					--debugLog("ENERGYBOSS: hit barrier!")
					bx,by = bone_getPosition(lineBone)
					spawnParticleEffect("HitEnergyBarrierBig", entity_x(barrier), by)
					bone_damageFlash(bone_head)
					bone_damageFlash(bone_body)
					hits = hits - 1
					if hits <= 0 then
						entity_setState(me, STATE_COLLAPSE)						
					else
						entity_setState(me, STATE_HITBARRIER, 1)
					end
				end
			end
		else
			--debugLog("ENERGYBOSS: barrier off")
		end
	else
		--debugLog("ENERGYBOSS: Did not find barrier")
	end
	
	if entity_isState(me, STATE_FIRE) then
		fireBit = fireBit - dt
		if fireBit < 0 then
			entity_setTarget(me, naija)
			offx, offy = 0
			while shotsFired < 3 do
				if shotsFired == 3 then
					velx = 100
					vely = 50
				elseif shotsFired == 0 then
					velx = 100
					vely = 25
				elseif shotsFired == 1 then
					velx = 100
					vely = 0
				elseif shotsFired == 2 then
					velx = 100
					vely = -25
				end	
				--entity_fireAtTarget(me, "Purple", 1, 500, 100, 0, 0, offx, offy, velx, vely)
				s = createShot("EnergyBoss", me, entity_getTarget(me), entity_getPosition(me))
				shot_setAimVector(s, velx, vely)
				
				shotsFired = shotsFired + 1
			end
			entity_setState(me, STATE_IDLE)
		end
	end
	
	if entity_isState(me, STATE_IDLE) then
		if moveDelay <= 0 and entity_x(me) < node_x(maxMove)
		and entity_x(naija) < entity_x(me)+1200 then
			entity_setState(me, STATE_MOVING)
		else
			if awoken then
				fireDelay = fireDelay - dt
				if fireDelay <= 0 then
					fireDelay = 0
					entity_setState(me, STATE_FIRE)
				end
			end
			
			if entity_isState(me, STATE_IDLE) then
				attackDelay = attackDelay - dt
				if attackDelay <= 0 and entity_x(naija) < entity_x(me)+800 then
					attackDelay = 1
					entity_setState(me, STATE_ATTACK)
				end
			end
		end
	end
	if entity_isState(me, STATE_MOVING) and entity_x(me) >= node_x(maxMove) then
		if entity_isInterpolating() then
			entity_animate(me, "idle")
		end
		entity_stopInterpolating(me)
		entity_setPosition(me, node_x(maxMove), entity_y(me))
	end
	if (entity_isState(me, STATE_MOVEBACK) or entity_isState(me, STATE_MOVEBACKFROMBARRIER)) and entity_x(me) <= node_x(minMove) then
		entity_stopInterpolating(me)
		entity_setPosition(me, node_x(minMove), entity_y(me))
	end
	if entity_isState(me, STATE_MOVING) or entity_isState(me, STATE_MOVEBACK) or entity_isState(me, STATE_MOVEBACKFROMBARRIER) then
		if not entity_isInterpolating(me) then
			entity_setState(me, STATE_IDLE)
		end
	end
	entity_handleShotCollisionsSkeletal(me)	
	entity_clearTargetPoints(me)
	entity_addTargetPoint(me, bone_getWorldPosition(bone_head))
	if entity_isState(me, STATE_ATTACK) then
		entity_setLookAtPoint(me, bone_getWorldPosition(bone_claw))
		entity_setNaijaReaction(me, "shock")
	else
		entity_setLookAtPoint(me, bone_getWorldPosition(bone_jaw))
		entity_setNaijaReaction(me, "")
	end
	--entity_setEnergyShotTargetPosition(me, bone_getWorldPosition(bone_head))	
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_stopInterpolating(me)
		entity_animate(me, "idle", LOOP_INF)
	elseif entity_isState(me, STATE_ATTACK) then
		playSfx("EnergyBoss-Attack", 900+math.random(200))
		x, y = bone_getPosition(bone_jaw)
		if entity_isPositionInRange(naija, x, y, 600)
		and entity_y(naija) < y+64
		and entity_y(naija) > y-64
		then
			entity_animate(me, "bite")
			attack = ATTACK_BITE
		else
			if entity_y(naija) < entity_y(me)-200 then
				entity_animate(me, "attackUp")
				attack = ATTACK_UP
			else
				entity_animate(me, "lungeBite")
				attack = ATTACK_DOWN
			end
		end
	elseif entity_isState(me, STATE_FIRE) then
		shotsFired = 0
		entity_animate(me, "firing")
		fireDelay = 4
	elseif entity_isState(me, STATE_MOVING) then
		if hits <= 1 then
			maxMove = maxMove2
		end	
		entity_animate(me, "moveForward", LOOP_INF)
		if entity_x(orb) > entity_x(barrier) then
			entity_setPosition(me, entity_x(me)+(800/2), entity_y(me), 3.5/2)
		else
			entity_setPosition(me, entity_x(me)+800, entity_y(me), 3.5)
		end
	elseif entity_isState(me, STATE_MOVEBACK) then
		entity_animate(me, "moveBackward", LOOP_INF)
		
		if entity_x(orb) > entity_x(barrier) then
			moveDelay = 999
		else
			moveDelay = moveDelay + 10
		end
		attackDelay = 0
		fireDelay = 0
		playSfx("EnergyBoss-Hurt", 900+math.random(200))
		entity_animate(me, "hurt")
		entity_setPosition(me, entity_x(me)-500, entity_y(me), 1.6)
	elseif entity_isState(me, STATE_HITBARRIER) then
		entity_stopInterpolating(me)
		playSfx("EnergyBoss-Die", 1100+math.random(200))
		entity_animate(me, "hitBarrier")	
		
		entity_spawnParticlesFromCollisionMask(me, "energyboss-hit", 4)
	elseif entity_isState(me, STATE_MOVEBACKFROMBARRIER) then
		--entity_animate(me, "idle", LOOP_INF)
		--entity_setPosition(me, entity_x(me)-(1500*0.80), entity_y(me), 2)
		nodeName = string.format("bossbackloc%d", hits)
		debugLog(string.format("nodeName: %s", nodeName))
		backNode = getNode(nodeName)
		entity_setPosition(me, node_x(backNode), entity_y(me), -800)
	elseif entity_isState(me, STATE_COLLAPSE) then
		clearShots()
		playSfx("EnergyBoss-Die", 1000)
		setFlag(FLAG_ENERGYBOSSDEAD, 1)	
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, false)
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, false)	
		endTextDelay = 6.5
		
		entity_spawnParticlesFromCollisionMask(me, "energyboss-hit", 4)
		
		
		fadeOutMusic(2)
		
		entity_animate(me, "die")
		cam_toEntity(me)
		disableInput()
		--[[
		cam_toEntity(me)		
		watch(4)
		voiceOnce("Naija_EnergyBossOver")
		cam_toEntity(getNaija())
		]]--
	elseif entity_isState(me, STATE_COLLAPSED) then
		orb = getEntityByID(4)
		holder = getEntityByID(3)
		if orb and holder then
			entity_setPosition(orb, entity_x(holder), entity_y(holder))
		end
		collectibleNode = getNode("COLLECTIBLE")
		createEntity("CollectibleEnergyBoss", "", node_x(collectibleNode), node_y(collectibleNode))
		--debugLog("animating dead")
		entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, false)
		entity_setDamageTarget(me, DT_AVATAR_SHOCK, false)
		entity_animate(me, "dead")
		
		--if entity_isFlag(me, 0) then
		
		-- you can't beat the game without dual form
		-- and we don't want to stop the music when we're panning near the boss at the end of the game
		-- therefore: DO THIS:
		if not hasSong(SONG_DUALFORM) then
			fadeOutMusic(0.1)
		end
			--entity_setFlag(me, 1)
		--end
	elseif entity_isState(me, STATE_INTRO) then
		awoken = true
		playSfx("EnergyBoss-Die", 800)
		shakeCamera(10, 3)
		entity_stopInterpolating(me)
		entity_animate(me, "roar")
		overrideZoom(0.5, 1)
		
	elseif entity_isState(me, STATE_APPEAR) then
		entity_animate(me, "eatOmpo")
	end
end

function animationKey(me, key)
	if entity_isState(me, STATE_APPEAR) then
		if key == 5 then
			x, y = bone_getWorldPosition(bone_claw)
			spawnParticleEffect("Dirt", x,y)
			playSfx("RockHit-Big")
		elseif key == 7 then
			playSfx("Bite")
		elseif key == 9 then
			playSfx("Bite")
		elseif key == 15 then
			x, y = bone_getWorldPosition(bone_claw)
			spawnParticleEffect("Dirt", x,y)
			--playSfx("RockHit-Big")
		elseif key == 16 then
			playSfx("Gulp")
		elseif key == 17 then
			x, y = bone_getWorldPosition(bone_claw)
			spawnParticleEffect("Dirt", x,y)
			playSfx("RockHit-Big")
		end
		
		return
	end
	if entity_isState(me, STATE_COLLAPSE) then
		if key == 9 then
			playSfx("BossDieSmall")
			fade2(0.5, 0, 1, 1, 1)
			fade2(0, 1, 1, 1, 1)
		end
		if key == 11 then
			playSfx("BossDieSmall")
			fade2(0.2, 0, 1, 1, 1)
			fade2(0, 1, 1, 1, 1)
			entity_spawnParticlesFromCollisionMask(me, "energyboss-hit", 4)
		end
	else
		if attack == ATTACK_DOWN then
			if entity_isState(me, STATE_ATTACK) and key == 4 then
				x, y = bone_getWorldPosition(bone_claw)
				spawnParticleEffect("Dirt", x,y)
				playSfx("RockHit-Big")
				shakeCamera(15, 0.5)
			end
		elseif attack == ATTACK_UP then
			if entity_isState(me, STATE_ATTACK) and key == 4 then
				x, y = bone_getWorldPosition(bone_claw)
				spawnParticleEffect("Dirt", x,y)
				playSfx("RockHit-Big")
				shakeCamera(15, 0.5)
			end	
		end
	end
end

function exitState(me)
	if entity_isState(me, STATE_HITBARRIER) then
		entity_setState(me, STATE_MOVEBACKFROMBARRIER)
	elseif entity_isState(me, STATE_MOVING) then
		moveDelay = 4
	end
end

function activate(me)
end

function hitSurface(me)
end

