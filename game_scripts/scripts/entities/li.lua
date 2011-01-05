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
-- Merman / Thin
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

honeyPower = 0

swimTime = 0
swimTimer = swimTime - swimTime/4
dirTimer = 0
dir = 0

gvel = false

forcedHug = false

incut = false

seen = false
inVeil01 = false

bone_helmet = 0
bone_head = 0
bone_fish1 = 0
bone_fish2 = 0
bone_hand = 0
bone_arm = 0
bone_weaponGlow = 0
bone_leftHand = 0

bone_llarm = 0
bone_ularm = 0

switchGiggle = false

pathDelay = 0

naijaOut = -25
hugOut = 0
curNote = -1

followDelay = 0

chaseTime = 0
expressionTimer = 0

STATE_HANG 			= 1000
STATE_SWIM 			= 1001
STATE_BURST 		= 1002
STATE_CHASED 		= 1003
STATE_RUNTOCAVE 	= 1004
STATE_BEFOREMEET 	= 1005
STATE_FADEOUT		= 1006
STATE_FOLLOWING 	= 1007
STATE_CORNERED		= 1008
STATE_CHASEFOOD		= 1009
STATE_EAT			= 1010
STATE_PATH			= 1011

naijaLastHealth = 0
nearEnemyTimer = 0
nearNaijaTimer = 0
headDelay = 1

flipDelay = 0

n=0

normal = 0
angry = 1
happy = 2
hurt = 3
laugh = 4
surprise = 5

zapDelay = 0.1

breathTimer = 0

ing = 0

function distFlipTo(me, ent)
	if math.abs(entity_x(me)-entity_x(ent)) > 32 then
		entity_flipToEntity(me, ent)
	end
end

function flipHug(me)
	debugLog("flipHug")
	if hugOut < 0 then
		hugOut = -naijaOut
	else
		hugOut = naijaOut
	end
	setNaijaHugPosition(me)
	entity_flipToEntity(me, n)
	entity_flipToEntity(n, me)
end

function activate(me)
	--debugLog("Li: activate")
	if entity_isState(me, STATE_HUG) then
		endHug(me)
	else
	
		if isFlag(FLAG_LI, 101) or isFlag(FLAG_LI, 102) then
			--debugLog("setting li to follow")
			fade(1, 1)
			entity_idle(n)
			watch(1)
			if not switchGiggle then
				emote(EMOTE_NAIJAGIGGLE)
				switchGiggle = true
			end
			watch(0.3)
			playSfx("changeclothes2")
			setFlag(FLAG_LI, 100)
			entity_setActivationType(me, AT_NONE)
			entity_setState(me, STATE_IDLE)
			bone_alpha(bone_helmet, 0, 0.5)
			watch(0.5)
			fade(0,1)
			watch(1)
			setLi(me)
		end
	end
end

function expression(me, ep, t)
	expressionTimer = t
	bone_showFrame(bone_head, ep)
	--[[
	if ep == "" then
		bone_setTexture(bone_head, "Li/Head")
	else
		ep = string.format("%s%s", "Li/Head-", ep)
		bone_setTexture(bone_head, ep)
	end
	]]--
end

function init(me)

--[[
	if isFlag(FLAG_LI, 0) or isFlag(FLAG_LI, 1) then
		debugLog("SETTING BEFOREMEET")
		entity_setState(me, STATE_BEFOREMEET)
	elseif isFlag(FLAG_LI, 101) or isFlag(FLAG_LI, 102) then
		entity_setState(me, STATE_BEFOREMEET)
	else
		--debugLog(string.format("Got head: %d", bone_head))
		bone_alpha(bone_helmet, 0, 0.1)
		entity_setState(me, STATE_IDLE)
	end	
]]--
	
	setupBasicEntity(me, 
	"",								-- texture
	32,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	28,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	64,								-- sprite width	
	64,								-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	1,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000							-- updateCull -1: disabled, default: 4000
	)
	
	entity_initSkeletal(me, "Li")
	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	
	entity_scale(me, 0.5, 0.5)

	bone_helmet = entity_getBoneByName(me, "Helmet")
	bone_head = entity_getBoneByName(me, "Head")
	bone_fish1 = entity_getBoneByName(me, "Fish1")
	bone_fish2 = entity_getBoneByName(me, "Fish2")
	bone_hand = entity_getBoneByName(me, "RightArm")
	bone_arm = entity_getBoneByName(me, "RightArm2")
	bone_weaponGlow = entity_getBoneByName(me, "WeaponGlow")
	bone_setBlendType(bone_weaponGlow, BLEND_ADD)
	bone_alpha(bone_fish1)
	bone_alpha(bone_fish2)
	
	bone_llarm = entity_getBoneByName(me, "LLArm")
	bone_ularm = entity_getBoneByName(me, "ULArm")
	bone_leftHand = entity_getBoneByName(me, "LeftArm")

	
	entity_setEntityType(me, ET_NEUTRAL)

	--entity_setSpiritFreeze(me, false)
	
	entity_setBeautyFlip(me, false)
	entity_setDamageTarget(me, DT_AVATAR_LANCEATTACH, false)
	entity_setDamageTarget(me, DT_AVATAR_LANCE, false)
	
	esetv(me, EV_ENTITYDIED, 1)
	
	
	inVeil01 = isMapName("veil01")

	
	--entity_setRenderPass(me, 1)

end

function entityDied(me, ent)
	if ing ~= 0 and ent == ing then
		entity_setState(me, STATE_IDLE)
		ing = 0
	end
end

function pathCheck(me, dt)
	-- messes up on small passages etc
	--[[
	if pathDelay > 0 then
		pathDelay = pathDelay - dt
	end
	if pathDelay <= 0 then
		pathDelay = 3
		entity_setState(me, STATE_PATH)
		return true
	end
	]]--
	return false
end


function postInit(me)
	n = getNaija()
	naijaLastHealth = entity_getHealth(n)
	bwgsz = bone_getScale(bone_weaponGlow)
	refreshWeaponGlow(me)
	
	if isFlag(FLAG_LI, 0) or isFlag(FLAG_LI, 1) then
		--debugLog("SETTING BEFOREMEET")
		entity_setState(me, STATE_BEFOREMEET, -1, true)
	elseif isFlag(FLAG_LI, 101) or isFlag(FLAG_LI, 102) then
		entity_setState(me, STATE_BEFOREMEET, -1, true)
	elseif isFlag(FLAG_LI, 200) then
		bone_alpha(bone_helmet, 0, 0.1)
		-- overridable, do nothing
	else
		bone_alpha(bone_helmet, 0, 0.1)
		entity_setState(me, STATE_IDLE, -1, true)
	end		
	
	
	if isMapName("licave") then
		entity_moveToBack(me)
	end
end

function zap(me)
	--debugLog("Zap!")
	--attackRange = 256
	attackRange = 800
	chaseRange = 300
	attacked = false

	fx, fy = bone_getWorldPosition(bone_hand)
	ent = getFirstEntity()
	while ent ~= 0 do
		if entity_isValidTarget(ent) then
			if entity_isDamageTarget(ent, DT_AVATAR_LIZAP, true) then
				--entity_setTarget(me, ent)
				if entity_isEntityInRange(me, ent, chaseRange) and not entity_isEntityInRange(me, ent, attackRange) then
					--entity_moveTowardsTarget(me, 1, 300)
					--entity_setMaxSpeedLerp(me, 1.1, 0.5)
					--entity_flipToEntity(me, ent)
				elseif entity_isEntityInRange(me, ent, attackRange) and not entity_isDead(ent) then
					-- zap this one					
					--entity_damage(ent, me, 1.0, DT_AVATAR_LIZAP)
					entity_animate(me, "fire", 0, LAYER_UPPERBODY)
					s = createShot("Li", me, ent, fx, fy)
					endHug(me)
					
					ax, ay = bone_getWorldPosition(bone_arm)
					dx = entity_x(ent) - ax
					dy = entity_y(ent) - ay
					shot_setAimVector(s, dx, dy)
					
					--softFlipTo(me, ent)
					
					--entity_setMaxSpeedLerp(me, 1, 0.5)
					attacked = true					
					break
				end
			end			
		end
		ent = getNextEntity(iter)
	end	
	entity_setTarget(me, getNaija())
	--[[
	if attacked then
		spawnParticleEffect("LiZap", fx, fy)
	end
	]]--
end

function shiftWorlds(me, old, new)
	if hasLi() then
	--[[
		if new == WT_SPIRIT then
			entity_alpha(me, 0.1)
		else
			entity_alpha(me, 1)
		end
		x,y = entity_getPosition(n)
		entity_setPosition(me, x+1, y+1)
		]]--
	end
end

function song(me, song)
	--debugLog("Li: Sung song!")
	if entity_isState(me, STATE_HUG) then
		if song == SONG_SHIELD then
			flipHug(me)
		end
		if song == SONG_LI then
			entity_setState(me, STATE_IDLE)
		end
	end
	
	if entity_isState(me, STATE_CORNERED) then
		--debugLog("i'm cornered and you sun a song")
		if song == SONG_BIND then
			--debugLog("it was bind")
			--debugLog(string.format("FLAG_LI: %d", getFlag(FLAG_LI)))
			if isFlag(FLAG_LI, 1) then			
				--debugLog("calling cutscene")
				cutscene(me)
			end
		end
	else
		if song == SONG_ENERGYFORM then
			nearNaijaTimer = 0
			expression(me, surprise, 1.5)
			entity_flipToEntity(me, n)
			--entity_moveTowardsTarget(me, 1, -1000)
		elseif song == SONG_BEASTFORM then
			nearNaijaTimer = 0
			expression(me, angry, 4)
			entity_flipToEntity(me, n)
		elseif song == SONG_NATUREFORM then
			nearNaijaTimer = 2
			expression(me, happy, 3)
			entity_flipToEntity(me, n)		
		end
	end
end

function softFlipTo(me, ent)
	if flipDelay < 0 then
		entity_flipToEntity(me, ent)
		flipDelay = 1
	end
end

function endHug(me)
	if entity_getRiding(n) == me then
		entity_setRiding(n, 0)
		entity_idle(n)
	end
	if entity_isState(me, STATE_HUG) then
		if not isForm(FORM_DUAL) then
			entity_setState(me, STATE_IDLE)
		end
	end
end

function update(me, dt)
	if isForm(FORM_DUAL) then return end
	if incut then return end
	if entity_isState(me, STATE_WAIT) then return end
	if entity_isState(me, STATE_TRAPPEDINCREATOR) then return end
	if entity_isState(me, STATE_OPEN) then return end
	if entity_isState(me, STATE_CLOSE) then return end
	
	if entity_isState(me, STATE_PUPPET) then return end
	
	
	if not hasLi() and not seen then
		if inVeil01 then
			if entity_isEntityInRange(me, n, 600) then
				seen = true
				musicVolume(0.1, 0.5)
				entity_idle(n)
				entity_flipToEntity(n, me)
				--playSfx("naijachildgiggle")
				cam_toEntity(me)
				setGameSpeed(0.5, 1)
				--playSfx("heartbeat")
				wait(1.5)
				playSfx("heartbeat")
				wait(0.75)
				playSfx("heartbeat")
				wait(0.5)
				--playSfx("heartbeat")
				setGameSpeed(1, 1)
				cam_toEntity(n)
				musicVolume(1, 1)
				
			end
		end
	end
	
	--debugLog(string.format("liupdate state: %d", entity_getState(me)))
	
	liPower = getLiPower()
	if liPower > 0 then
		debugLog("liPower!")
		entity_setColor(me, 0.6, 0.7, 1.0, 0.1)
	else
		entity_setColor(me, 1,1,1,0.1)
	end
	
	if entity_isState(me, STATE_CARRIED) then
		bone_alpha(bone_helmet, 0)
		--entity_setPosition(me, entity_x(n)+24, entity_y(n))
		return
	end
	--entity_touchAvatarDamage(me, 32, 1, 1200)
	--entity_handleShotCollisions(me)
	
	if bone_head ~= 0 then
		entity_setLookAtPoint(me, bone_getWorldPosition(bone_head))
	end
	entity_updateCurrents(me, dt)
	
	spdf = 1
	if liPower > 0 then
		spdf = 8
	end
	
	flipDelay = flipDelay - dt*spdf
	if flipDelay < 0 then
		flipDelay = 0
	end
	--if isFlag(FLAG_LI, 100) then
	if hasLi() and not entity_isState(me, STATE_CHASEFOOD) then
		if headDelay > 0 then
			headDelay = headDelay - dt
		else
			ent = entity_getNearestEntity(me)
			if eisv(ent, EV_TYPEID, EVT_PET) then
				ent = n
			end
			if ent ~=0 and entity_isEntityInRange(me, ent, 256) then
				if not entity_isState(me, STATE_HUG) then
					if entity_getEntityType(ent) == ET_INGREDIENT then
						if ing_hasIET(ent, IET_LI) then
							
							-- move towarsd
							expression(me, happy, 2)
							--entity_moveTowards(me, entity_x(ent), entity_y(ent), dt, 500)
							entity_setTarget(me, ent)
							--entity_updateMovement(me, dt)
							if not entity_isState(me, STATE_CHASEFOOD) and ent ~= 0 then
								ing = ent
								entity_setState(me, STATE_CHASEFOOD)
							end
						end
					elseif entity_getEntityType(ent) == ET_ENEMY and entity_isEntityInRange(me, ent, 128) then
						if eisv(ent, EV_TYPEID, EVT_PET) then
							ent = 0
						else
							nearEnemyTimer = nearEnemyTimer + dt*2
							nearNaijaTimer = nearNaijaTimer - dt
							if nearEnemyTimer > 10 then
								expression(me, angry, 2)
								nearEnemyTimer = 10
							else
								expression(me, surprise, 1)
							end
							entity_setNaijaReaction(me, "")
						end
					elseif entity_getEntityType(ent) == ET_AVATAR and entity_isEntityInRange(me, ent, 128) then
						--softFlipTo(me, ent)
						distFlipTo(me, ent)
						if entity_getHealth(ent) > 2 and isForm(FORM_NORMAL) and not avatar_isSinging() then
							nearNaijaTimer = nearNaijaTimer + dt*2
							if nearNaijaTimer > 4 then
								expression(me, happy, 1)
							end
							if nearNaijaTimer > 5 then
								entity_setNaijaReaction(me, "smile")
							end
							if nearNaijaTimer > 14 then
								nearNaijaTimer = 0+math.random(2)
								entity_setNaijaReaction(me, "")
							end
							
							if avatar_getStillTimer() > 4 and not avatar_isOnWall() and nearNaijaTimer > 8 then
								if not isInputEnabled() or avatar_isSinging() then 
									nearNaijaTimer = 0
								else
									if entity_getRiding(getNaija()) == 0 then
										entity_setState(me, STATE_HUG)
									end
								end
							end
						end
					end
				end

				
				
				
				--entity_stopAllAnimations(me)
			else
				ent = 0
			end
			if ent ~= 0 then
				bone_setAnimated(bone_head, ANIM_POS)
				bone_lookAtEntity(bone_head, ent, 0.3, -10, 30, -90)
			else
				bone_setAnimated(bone_head, ANIM_ALL)
				entity_setNaijaReaction(me, "")
			end
		end
		nearEnemyTimer = nearEnemyTimer - dt
		if nearEnemyTimer < 0 then nearEnemyTimer = 0 end
		nearNaijaTimer = nearNaijaTimer - dt
		if nearNaijaTimer < 0 then nearNaijaTimer = 0 end
		
		if entity_getHealth(n) > naijaLastHealth then
			expression(me, happy, 2)
		end
		naijaLastHealth = entity_getHealth(n)
		if entity_getHealth(n) < 1 then
			expression(me, hurt, 2)
		end
		if isFlag(FLAG_LICOMBAT, 1) and not entity_isState(me, STATE_LIPUPPET) then
			if zapDelay > 0 then
				zapDelay = zapDelay - dt
				if zapDelay < 0 then
					zap(me)
					zapDelay = 1.2
					--zapDelay = 0.001
				end
			end
		end
	end
	
	
	if expressionTimer > 0 then
		expressionTimer	= expressionTimer - dt
		if expressionTimer < 0 then
			expressionTimer = 0
			expression(me, normal, 0)
		end
	end	
	if entity_isState(me, STATE_IDLE) then
		entity_setTarget(me, n)
		followDelay = followDelay - dt
		if followDelay < 0 then
			followDelay = 0
		end
		if entity_isEntityInRange(me, n, 1024) and not entity_isEntityInRange(me, n, 256) and not avatar_isOnWall() and entity_isUnderWater(n) then
			if followDelay <= 0 then
				entity_setState(me, STATE_FOLLOWING)
			end
		end 
		entity_doSpellAvoidance(me, dt, 128, 0.1)
		--entity_doEntityAvoidance(me, dt, 64, 0.5)
		if entity_isEntityInRange(me, n, 20) then
			entity_moveTowardsTarget(me, dt, -150)
		end
	elseif entity_isState(me, STATE_PATH) then
		--debugLog("updating state path")
		if entity_isFollowingPath(me) then
			if entity_isEntityInRange(me, n, 300) then
				entity_stopFollowingPath(me)
				entity_moveTowardsTarget(me, 1, 500)
				entity_setState(me, STATE_FOLLOWING)
			end
			
			--entity_setState(me, STATE_FOLLOWING)
		else
			entity_moveTowardsTarget(me, 1, 500)
			entity_setState(me, STATE_FOLLOWING)
		end
	elseif entity_isState(me, STATE_FOLLOWING) then		
		--debugLog("updating following")
		amt = 800
		--not avatar_isOnWall() and 
		
		entity_doCollisionAvoidance(me, dt, 4, 1, 100, 1, true)
	
		entity_setTarget(me, n)
		if entity_isUnderWater(n) then
			if entity_isEntityInRange(me, n, 180) then
				entity_setMaxSpeedLerp(me, 0.2, 1)
			else
				entity_setMaxSpeedLerp(me, 1, 0.2)
			end
			
			if entity_isEntityInRange(me, n, 180) then
				entity_doFriction(me, dt, 200)
				if ((math.abs(entity_velx(n)) < 10 and math.abs(entity_vely(n)) < 10) or avatar_isOnWall()) then
					entity_setState(me, STATE_IDLE)
				end
			elseif entity_isEntityInRange(me, n, 250) then
				--entity_moveAroundTarget(me, dt, amt*0.8)
				entity_moveTowardsTarget(me, dt, amt)
			elseif entity_isEntityInRange(me, n, 512) then
				entity_moveTowardsTarget(me, dt, amt*2)
			elseif not entity_isEntityInRange(me, n, 1024) then
				if entity_isUnderWater(n) and not avatar_isOnWall() then
					if not pathCheck(me, dt) then
						entity_moveTowardsTarget(me, dt, amt)
					end
				else
					entity_moveTowardsTarget(me, dt, amt)
				end
			else
				entity_moveTowardsTarget(me, dt, amt)
			end
		else
			entity_setState(me, STATE_IDLE)
		end
		-- hmm?
		--entity_doSpellAvoidance(me, dt, 128, 0.2)
		
		
		--entity_doCollisionAvoidance(me, dt, 8, 0.05)

		if entity_doCollisionAvoidance(me, dt, 5, 0.1) then
			--entity_moveTowardsTarget(me, dt, 250)
		end
		--[[
		if entity_doCollisionAvoidance(me, dt, 1, 1) then
			entity_moveTowardsTarget(me, dt, 200)
		end
		]]--
		
		if math.abs(entity_velx(me)) < 1 and math.abs(entity_vely(me)) < 1 then
			--debugLog("get unstuck")
			entity_setMaxSpeedLerp(me, 1)
			entity_moveTowardsTarget(me, 1, 500)
		end
		--debugLog(string.format("li v(%d, %d)", entity_velx(me), entity_vely(me)))
	elseif entity_isState(me, STATE_CHASEFOOD) then
		if ing == 0 then
			entity_setState(me, STATE_IDLE)
		else
			amt = 500

			entity_moveTowards(me, entity_x(ing), entity_y(ing), dt, amt)

			--entity_doSpellAvoidance(me, dt, 128, 0.2))
			entity_doCollisionAvoidance(me, dt, 3, 0.1)
			if ing ~= 0 and entity_isEntityInRange(me, ing, 32) then
				-- do yum type things
				entity_delete(ent)
				ent = 0
				ing = 0
				entity_setState(me, STATE_EAT)
				expression(me, happy, 2)
				
				--debugLog("setting li power!")
				setLiPower(1, 30)
			end
		end

	elseif entity_isState(me, STATE_BEFOREMEET) then
		--debugLog("updating before meet")
		dirTimer = dirTimer + dt
		if dirTimer > 3 then
			dirTimer = 0
			if dir > 0 then
				dir = 0
			else
				dir = 1
			end
		end
		spd = 300
		if dir > 0 then
			spd = -spd
		end
		entity_addVel(me, spd, 0)
		entity_doEntityAvoidance(me, dt, 256, 0.1)
		entity_doCollisionAvoidance(me, dt, 6, 0.5)
		
		if getFlag(FLAG_LI) < 100 then
			if entity_isEntityInRange(me, getNaija(), 150) then
				entity_setState(me, STATE_CHASED)
			end
		end

		--debugLog(string.format("vel: %d", entity_velx(me)))
	elseif entity_isState(me, STATE_CHASED) then
		chaseTime = chaseTime + dt
		-- 10
		if chaseTime > 1 then
			entity_setState(me, STATE_RUNTOCAVE)
		end
		entity_moveTowardsTarget(me, dt, -500)
		entity_doCollisionAvoidance(me, dt, 6, 0.5)
	elseif entity_isState(me, STATE_RUNTOCAVE) then
		liin = getNode("LI_IN")
		if not entity_isEntityInRange(me, getNaija(), 1000) and not node_isEntityIn(liin, me) then
			entity_stopFollowingPath(me)
			entity_setState(me, STATE_BEFOREMEET)
		else
			if not entity_isFollowingPath(me) then
				if isFlag(FLAG_LI, 0) then
					entity_setState(me, STATE_FADEOUT)
				elseif isFlag(FLAG_LI, 1) then
					entity_setState(me, STATE_CORNERED)
				end
			end
		end
	elseif entity_isState(me, STATE_HUG) then
		--debugLog("state hug")
		entity_setMaxSpeedLerp(me, 2)
		expression(me, happy, 0.5)
		if entity_getRiding(n) == me then
			entity_animate(n, "hugLi", 0, 3)
			if curNote ~= -1 then
				vx, vy = getNoteVector(curNote, 400*dt)
				entity_addVel(me, vx, vy)
			end
			entity_doCollisionAvoidance(me, dt, 5, 0.1)
			entity_doCollisionAvoidance(me, dt, 1, 1)
			entity_doFriction(me, dt, 100)
			entity_updateMovement(me, dt)
			
			setNaijaHugPosition(me)
			
			entity_updateLocalWarpAreas(me, true)
			
			bone_setRenderPass(bone_llarm, 3)
			bone_setRenderPass(bone_ularm, 3)
			bone_setRenderPass(bone_leftHand, 3)
			
			if not forcedHug then
				if not isForm(FORM_NORMAL) or not isInputEnabled() or entity_isFollowingPath(n) or avatar_getStillTimer() < 1 or honeyPower ~= entity_getHealthPerc(n) then
					endHug(me)
				end
			end
			
			
			
			--[[
			ent = entity_getNearestEntity(me, "", 400, ET_ENEMY)
			if ent ~= 0 then
				expression(me, angry, 1)
				entity_setState(me, STATE_IDLE)
				entity_flipToEntity(me, ent)
				entity_flipToEntity(n, ent)
			end
			]]--
		else
			--debugLog("naija is not riding")
			entity_setRiding(n, me)
		end
		--entity_setPosition(n, )
		

	end
	
	if not entity_isState(me, STATE_FADEOUT) and not entity_isState(me, STATE_HUG) and not entity_isState(me, STATE_PATH) then
		if (math.abs(entity_velx(me))) > 10 then
			entity_flipToVel(me)
		end
		if not entity_isState(me, STATE_IDLE) then
			entity_rotateToVel(me, 0.1)
		end
		if math.abs(entity_velx(me)) > 20 or math.abs(entity_vely(me)) > 20 then
			entity_doFriction(me, dt, 150)
			gvel = true
		else
			if gvel then
				entity_clearVel(me)
				gvel = false
			else
				entity_doFriction(me, dt, 40)
			end
		end
		entity_updateMovement(me, dt)
	end
	
	if not entity_isUnderWater(me) then
		w = getWaterLevel()
		if math.abs(w - entity_y(me)) <= 40 then
			entity_setPosition(me, entity_x(me), w+40)
			entity_clearVel(me)
		else
			if entity_isUnderWater(n) then
				entity_setPosition(me, entity_x(n), entity_y(n))
			end
		end
	end
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function setNaijaHugPosition(me)
	entity_setPosition(n, entity_x(me)+hugOut, entity_y(me))
	fh = entity_isfh(me)
	if fh then
		fh = false
	else
		fh = true
	end
	entity_setRidingData(me, entity_x(me)+hugOut, entity_y(me), 0, fh)
end

function cutscene(me)
	setCutscene(1,1)
	fadeOutMusic(4)
	--watch(2)
	--changeForm(FORM_NORMAL)
	
	setBeacon(BEACON_LI, false)
	
	inp(0)
	overrideZoom(0.8, 5)
	entity_animate(me, "helmetFlyOff", 0, 3)
	entity_idle(n)
	
	voiceInterupt("NAIJA_LIBINDSONG1")
	
	
	watch(3)
	
	bone_alpha(bone_helmet, 0, 0.5)
	
	node = entity_getNearestNode(me, "NAIJALI")
	entity_swimToNode(n, node)	
	
	entity_animate(me, "choke", LOOP_INF)
	
	expression(me, hurt, 99)
	
	entity_watchForPath(n)
	
	entity_flipToEntity(me, n)
	entity_flipToEntity(n, me)
	
	watchForVoice()


	
	
	watch(2)
	
	voice("NAIJA_LIBINDSONG2")
	-- naija floats forwards, kisses
	entity_setPosition(n, entity_x(me)-30, entity_y(me), 1, 0, 0, 1)	
	entity_animate(n, "kissLi")
	cam_toNode(getNode("KISSCAM"))
	watch(1)
	expression(me, normal, 99)
	entity_animate(me, "getKissed")
	entity_setPosition(n, entity_x(me)-23, entity_y(me), 1, 0, 0, 1)
	watch(1)
	--[[
	entity_animate(n, "getKissed", LOOP_INF)
	while entity_isAnimating(n) do
		watch(FRAME_TIME)
	end
	]]--
	--[[
	entity_offset(n, 0, 8, 2, -1, 1)
	entity_offset(me, 0, 8, 2, -1, 1)
	]]--
	avatar_setHeadTexture("blink")
	expression(me, surprise, 2)
	entity_animate(n, "kissLiLoop", LOOP_INF)
	entity_animate(me, "kissLiLoop", LOOP_INF)
	--entity_animate(n, "getKissedLoop", LOOP_INF)
	
	watchForVoice()
	
	-- music
	playMusic("Moment")

	-- particle effects start
	kissNode = entity_getNearestNode(me, "KISSPRT")
	spawnParticleEffect("Kiss", node_x(kissNode), node_y(kissNode))
	
	watch(3)
	
	voice("NAIJA_LIBINDSONG3")
	
	watch(3)
	
	watchForVoice()
	watch(3)
	
	voice("NAIJA_LIBINDSONG4")
	watchForVoice()
	
	watch(2)
	-- drift apart

	voice("NAIJA_LIBINDSONG5")
	watch(3)
	--entity_addVel(me, 200, 0)
	entity_setPosition(me, entity_x(me)+200, entity_y(me), 10, 0, 0, 1)
	entity_setPosition(n, entity_x(n)-250, entity_y(n), 10, 0, 0, 1)
	
	entity_animate(n, "kissFloat")
	entity_animate(me, "kissFloat")
	
	--entity_addVel(n, -200, 0)
	--watchForVoice()
	
	--watch()
	watch(1)
	
	
	
	voice("NAIJA_LIBINDSONG6")
	
	watch(1)
	
	--watchForVoice()
	
	fade(1, 5)
	watch(3)
	
	fadeOutMusic(8)
	watch(8)
	
	watch(1)
	
	entity_offset(n)
	entity_offset(me)
	
	cam_toEntity(n)
	-- cutscene 2 goes here
	
	-- warp li outta here
	entity_setPosition(me, 0, 0)
	
	-- warp naija to sleep position
	avatar_setHeadTexture("")
	sleepNode = getNode("NAIJAWAKE")
	entity_setPosition(n, node_x(sleepNode), node_y(sleepNode))
	entity_animate(n, "sleep", -1)
	node = getNode("PUPPETLI")
	entity_setPosition(me, node_x(node), node_y(node))
	entity_flipToEntity(n, me)	
	entity_flipToEntity(me, n)
	
	entity_animate(me, "idle", -1)
	
	-- skip that interp
	watch(0.5)
	
	fade(0, 5)
	watch(6)
	voice("Naija_NaijaAwakes1")
	entity_animate(n, "slowWakeUp")
	while entity_isAnimating(n) do
		watch(FRAME_TIME)
	end
	entity_animate(n, "idle", -1)
	
	watchForVoice()
	watch(1)
	
	node = getNode("NAIJAGETUP")
	entity_setPosition(n, node_x(node), node_y(node), -500, 0, 0, 1)
	
	entity_stopAllAnimations(me)
	entity_swimToNode(me, getNode("LISAYHI"))
	entity_animate(me, "swim", -1)
	voice("Naija_NaijaAwakes2")
	watch(2)
	entity_flipToEntity(me, n)
	cam_toEntity(me)
	entity_flipToEntity(me, n)
	watchForVoice()
	entity_flipToEntity(me, n)
	watch(1)
	voice("Naija_NaijaAwakes3")
	watch(1)
	cam_toEntity(n)
	watch(3)
	entity_animate(n, "ashamed", -1, LAYER_UPPERBODY)
	watchForVoice()
	entity_swimToNode(me, getNode("LIGETFISH"))
	
	entity_watchForPath(me)
	
	bone_alpha(bone_fish1, 1)
	bone_alpha(bone_fish2, 1)

	entity_swimToNode(me, getNode("LISAYHI"))
	voice("Naija_NaijaAwakes4")
	watchForVoice()	
	entity_animate(me, "holdFish", -1, LAYER_UPPERBODY)
	expression(me, happy, 5)
	entity_stopAllAnimations(n)
	entity_idle(n)
	cam_toEntity(me)
	watch(2)
	
	fade(1, 1.5)
	watch(1.5)
	
	--bone_alpha(bone_fish1)
	bone_alpha(bone_fish2)
	n_fish2 = entity_getBoneByName(n, "Fish2")
	bone_alpha(n_fish2, 1)

	
	cam_toEntity(n)
	entity_stopAllAnimations(n)
	entity_idle(n)
	entity_stopAllAnimations(me)
	
	esetv(n, EV_LOOKAT, 0)
	
	naijaSit = getNode("NAIJASIT")
	liSit = getNode("LISIT")
	entity_setPosition(n, node_x(naijaSit), node_y(naijaSit))
	entity_setPosition(me, node_x(liSit), node_y(liSit))
	entity_animate(me, "sitAndEat", -1)
	entity_animate(n, "sitAndEat", -1)
	
	cam_toNode(getNode("EATCAM"))
	watch(1)
	
	fade(0, 1.5)
	watch(1.5)	
	
	voice("Naija_NaijaAwakes5")
	watchForVoice()
	
	fade(1, 3)
	watch(3)
	
	bone_alpha(bone_fish1)
	bone_alpha(n_fish2)
	
	cam_toEntity(n)
	
	
	entity_idle(n)
	-- and then:
	
	playMusic("LiCave")
	watch(1)
	
	voice("Naija_NaijaAwakes6")
	setFlag(FLAG_LI, 100)
	entity_setState(me, STATE_IDLE)
	-- get to end nodes
	
	esetv(n, EV_LOOKAT, 1)
	
	-- end test
	fade(0, 1)
	watch(1)
	inp(1)
	
	overrideZoom(0)
	
	setCutscene(0)
	
	learnSong(SONG_LI)
	
	setControlHint(getStringBank(42), 0, 0, 0, 10, "", SONG_LI)
	
	setLi(me)
end

function enterState(me, state)
	--debugLog(string.format(%s%d, "li state: ", entity_getState(me)))
	timer = 0
	if entity_isState(me, STATE_IDLE) then
		debugLog("idle")
		entity_rotate(me,0,0.5)
		entity_setMaxSpeed(me, 200)
		entity_animate(me, "idle", LOOP_INF)
		if n ~= 0 then
			entity_flipToEntity(me, n)
		end
		if not(isFlag(FLAG_LI, 101) or isFlag(FLAG_LI, 102)) and getFlag(FLAG_LI) >= 100 then
			if bone_helmet ~= 0 then
				--debugLog("setting helmet alpha to 0")
				bone_alpha(bone_helmet, 0)
			end
		end
	elseif entity_isState(me, STATE_CARRIED) then
		entity_rotate(me, 0)
		--entity_rotate(me, 360, 5, -1)
		--entity_stopAllAnimations(me)
		if entity_isfh(me) then entity_fh(me) end
		bone_setAnimated(bone_head, ANIM_ALL)
		--entity_animate(me, "trappedInCreator", -1)
	elseif entity_getState(me)==STATE_BEFOREMEET then
		--debugLog("beforemeet")
		chaseTime = chaseTime - 3
		entity_rotate(me,0,0.5)
		entity_setMaxSpeed(me, 200)
		entity_animate(me, "idle", LOOP_INF)
		
		if isFlag(FLAG_LI, 101) or isFlag(FLAG_LI, 102) then
			bone_alpha(bone_helmet, 1)
			entity_setActivationType(me, AT_CLICK)
		end
	elseif entity_isState(me, STATE_CHASED) then
		--debugLog("chased")
		entity_setMaxSpeed(me, 500)
		entity_setTarget(me, getNaija())
	elseif entity_isState(me, STATE_FOLLOWING) then
		--debugLog("following")
		followDelay = 0.2
		entity_animate(me, "swim", LOOP_INF)
		entity_setMaxSpeed(me, 600)
		
		entity_setMaxSpeedLerp(me, 1, 0.1)
	elseif entity_isState(me, STATE_CHASEFOOD) then
		--debugLog("chase food")
		entity_animate(me, "swim", LOOP_INF)
		entity_setMaxSpeed(me, 650)
	elseif entity_isState(me, STATE_EAT) then
		--debugLog("eat")
		entity_animate(me, "eat", LOOP_INF)
		entity_rotate(me,0,0.5)
		entity_setMaxSpeed(me, 200)
		entity_setStateTime(me, 3)
	elseif entity_isState(me, STATE_RUNTOCAVE) then
		--debugLog("runtocave")
		entity_setMaxSpeed(me, 700)
		node = getNode("LICAVE")
		if node ~= 0 then
			entity_swimToNode(me, node, SPEED_LITOCAVE)
			if not entity_isFollowingPath(me) then
				entity_setState(me, STATE_CHASED)
			end
		end
	elseif entity_getState(me)==STATE_SWIM then
		--debugLog("swim")
		entity_animate(me, "swim", LOOP_INF)
	elseif entity_isState(me, STATE_FADEOUT) then
		--debugLog("fadeout")
		--debugLog("setting flag to 1")
		setFlag(FLAG_LI, 1)
		entity_alpha(me, 0, 1)
		-- Make sure we don't see the head through the fading helmet.
		bone_showFrame(bone_head, -1)
	elseif entity_getState(me)==STATE_BURST then
		debugLog("burst")
		burstDelay = 6
		entity_animate(me, "burst")
		--entity_doSpellAvoidance(me, 1, 256, 1.0)
		entity_doEntityAvoidance(me, 1, 256, 1.0)
		entity_doCollisionAvoidance(me, 1, 256, 1.0)
	elseif entity_isState(me, STATE_CORNERED) then
		debugLog("cornered")
		voice("NAIJA_TRAPPEDLI")
		entity_flipToEntity(me, getNaija())
		--entity_setActivation(me, AT_CLICK, 64, 512)
	elseif entity_isState(me, STATE_WAIT) then
		debugLog("wait")
	elseif entity_isState(me, STATE_HUG) then
		incut = true
		debugLog("HUG!")
		
		entity_flipToEntity(me, n)
		entity_flipToEntity(n, me)
		
		nearNaijaTimer = 0
		hugOut = naijaOut
		if entity_isfh(me) then
			hugOut = -hugOut
		end
		
		entity_setNaijaReaction(me, "")
		expression(me, shock, 1)
		
		entity_clearVel(me)
		entity_clearVel(n)
		
		entity_idle(n)
		entity_setPosition(n, entity_x(me)+hugOut, entity_y(me), 1, 0, 0, 1)
		watch(1)
		
		honeyPower = entity_getHealthPerc(n)
	
		entity_setRiding(n, me)
		
		entity_flipToEntity(me, n)
		entity_flipToEntity(n, me)
		
		entity_setNaijaReaction(me, "smile")
		
		entity_animate(me, "hugNaija")
		
		entity_offset(me, 0, 0, 0)
		entity_offset(n, 0, 0, 0)
		
		entity_offset(me, 0, 10, 1, -1, 1, 1)
		entity_offset(n, 0, 10, 1, -1, 1, 1)
		
		entity_setActivationType(me, AT_CLICK)
		
		if not forcedHug then
			if chance(75) then
				if chance(50) then
					emote(EMOTE_NAIJAGIGGLE)
				else
					emote(EMOTE_NAIJASIGH)
				end
			end
		end
		incut = false
	elseif entity_isState(me, STATE_PATH) then
		debugLog("enter state path")
		entity_swimToPosition(me, entity_x(n), entity_y(n), SPEED_NORMAL)
	elseif entity_isState(me, STATE_TRAPPEDINCREATOR) then
		entity_rotate(me, 0)
		--entity_rotate(me, 360, 5, -1)
		entity_stopAllAnimations(me)
		if entity_isfh(me) then entity_fh(me) end
		bone_setAnimated(bone_head, ANIM_ALL)
		entity_animate(me, "trappedInCreator", -1)
		--[[
		entity_offset(me, 0)
		entity_offset(me, 0, 30, 1, 0, 0, 1)
		]]--
	elseif entity_isState(me, STATE_OPEN) then
		entity_rotate(me, 0)
		--entity_rotate(me, 360, 5, -1)
		entity_stopAllAnimations(me)
		if entity_isfh(me) then entity_fh(me) end
		bone_setAnimated(bone_head, ANIM_ALL)
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_CLOSE) then
		-- when getting sucked into the creator
		entity_rotate(me, 0)
		entity_stopAllAnimations(me)
		if entity_isfh(me) then entity_fh(me) end
		bone_setAnimated(bone_head, ANIM_ALL)
		entity_animate(me, "suckedin", -1)
	elseif entity_isState(me, STATE_PUPPET) then
		entity_idle(me, "idle", -1)
	end
end

function exitState(me)
	if entity_isState(me, STATE_BURST) then
		entity_setState(me, STATE_SWIM)
	elseif entity_isState(me, STATE_HUG) then
		entity_setMaxSpeedLerp(me, 1, 0.5)
		debugLog("hug off")
		entity_offset(me, 0, 0, 0)
		entity_offset(n, 0, 0, 0)
		
		bone_setRenderPass(bone_llarm, 1)
		bone_setRenderPass(bone_ularm, 1)
		bone_setRenderPass(bone_leftHand, 1)
		
		endHug(me)
		
		entity_setActivationType(me, AT_NONE)
	elseif entity_isState(me, STATE_EAT) then
		entity_setState(me, STATE_IDLE)
	end
end

function hitSurface(me)
end

function refreshWeaponGlow(me)
	t = 0.5
	f = 3
	if isFlag(FLAG_LICOMBAT, 1) then
		bone_alpha(bone_weaponGlow, 1, 0.5)
		bone_color(bone_weaponGlow, 1, 0.5, 0.5, t)
	else
		bone_alpha(bone_weaponGlow, 0.5, 0.5)
		bone_color(bone_weaponGlow, 0.5, 0.5, 1, t)
	end
	--[[
	bone_scale(bone_weaponGlow, bwgsz, bwgsz)
	bone_scale(bone_weaponGlow, bwgsz*f, bwgsz*f, t*0.75, 1, 1)		
	]]--
end
		
function msg(me, msg, v)
	-- switch to and from combat mode
	if msg == "c" then
		refreshWeaponGlow(me)
		entity_animate(me, "switchCombat", 0, LAYER_UPPERBODY)
	elseif msg == "forcehug" then
		forcedHug = true
		entity_setState(me, STATE_HUG, -1, 1)
	elseif msg == "endhug" then
		forcedHug = false
		endHug(me)
	elseif msg == "expression" then
		expression(me, v, 2)
	end
end

function songNote(me, note)
	a = 400
	ha = a/2
	curNote = note
end

function songNoteDone(me, note, len)
	curNote = -1
end
