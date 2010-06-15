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
-- M A N T I S
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

STATE_SCREECH			= 1000

STATE_FIREBOMBS			= 1001
STATE_FALL				= 1002
STATE_FLOAT				= 1003
STATE_SHOOT				= 1004
STATE_ATTACK			= 1005
STATE_HURT				= 1006
STATE_RECOVER			= 1007
STATE_DIE				= 1008

n = 0
fight = false

spawnPoint = 0


phaseDelayTime			= 1.5
phaseDelay				= phaseDelayTime

phase					= 0

PHASE_NONE				= 0
PHASE_SHOTS				= 2
PHASE_FIREBOMBS			= 1
PHASE_MAX				= 2

startx = 0
starty = 0

hits = 3

body = 0

item = 0

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================

function init(me)
	setupBasicEntity(
	me,
	"",								-- texture
	999,							-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	32,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	512,							-- sprite width	
	512,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	5500							-- updateCull -1: disabled, default: 4000
	)
	
	entity_initSkeletal(me, "mantis")
	entity_animate(me, "idle", -1)
	
	entity_generateCollisionMask(me)
	
	entity_setState(me, STATE_IDLE)

	entity_setDeathParticleEffect(me, "TinyGreenExplode")
	
	entity_setCanLeaveWater(me, true)
	
	
	
	entity_scale(me, 2, 2)
	
	--entity_setAllDamageTargets(me, false)
	entity_setDamageTarget(me, DT_AVATAR_VINE, true)
	
	entity_setDamageTarget(me, DT_ENEMY, true)
	entity_setDamageTarget(me, DT_ENEMY_MANTISBOMB, true)
	
	
	loadSound("mantis-fall")
	loadSound("mantis-bomb")
	loadSound("mantis-die")
	loadSound("mantis-fire")
	loadSound("mantis-roar")
	
	
	body = entity_getBoneByName(me, "body")
	spawnPoint = entity_getBoneByName(me, "spawnpoint")
	
	bone_alpha(spawnPoint, 0)
	

end

function postInit(me)
	n = getNaija()
	
	item = entity_getNearestEntity(me, "healthupgrade4")
	--debugLog(string.format("item: %d", item))
	
	if not entity_isFlag(me, 0) then
		entity_delete(me)
	else
		entity_alpha(item, 0)
	end
	
	startx = entity_x(me)
	starty = entity_y(me)
end

function update(me, dt)
	--entity_updateMovement(me, dt)
	if not fight and entity_isEntityInRange(me, n, 1400) then
		if not cut then
			cut = true
			fight = true
			entity_idle(n)
			playMusic("MiniBoss")
			cam_toEntity(me)
			watch(0.5)
			playSfx("mantis-roar")
			watch(1.5)
			cam_toEntity(n)
			cut = false
		end
	end
	
	if fight and not entity_isEntityInRange(me, getNaija(), 3800) then
		fight = false
		updateMusic()
		phase = 0
	end
	
	
	if fight then
		if entity_isState(me, STATE_IDLE) then
			phaseDelay = phaseDelay - dt
			if phaseDelay < 0 then
				debugLog("phaseDelay 0")
				phase = phase + 1
				if phase == PHASE_FIREBOMBS then
					debugLog("firebombs")
					entity_setState(me, STATE_FIREBOMBS)
				end
				if phase == PHASE_MAX then
					phase = 0
				end
				phaseDelay = phaseDelayTime
			end
		end
	end
	
	if fight then
		entity_handleShotCollisionsSkeletal(me)
		
		bone = entity_collideSkeletalVsCircle(me, n)
		if bone ~= 0 then
			entity_touchAvatarDamage(me, 0, 1, 500)
		end
	end
end

function hitSurface(me)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_offset(me, 0, 0, 0.1)
		entity_animate(me, "idle", -1)
	--[[
	elseif entity_isState(me, STATE_) then
	elseif entity_isState(me, STATE_) then
	]]--
	
	elseif entity_isState(me, STATE_FIREBOMBS) then
		entity_setStateTime(me, entity_animate(me, "firebombs"))
	elseif entity_isState(me, STATE_FALL) then
		playSfx("mantis-fall")
	
		entity_animate(me, "hurt", 4, 1)
		nd = getNode("mantisfall")
		nx = node_x(nd)
		ny = node_y(nd)
		entity_setPosition(me, nx, ny, 1)
		
		entity_setStateTime(me, 6)
		

		
	elseif entity_isState(me, STATE_RECOVER) then
		entity_setPosition(me, startx, starty, 1, 0, 0, 1)
		entity_setStateTime(me, 1)
		for i=1,3 do
			v = entity_getNearestEntity(me, "UberVine")
			if v ~= 0 then
				entity_delete(v)
			end
		end
	elseif entity_isState(me, STATE_HURT) then
		playSfx("mantis-roar")
		
		entity_offset(me, -10, 0)
		entity_offset(me, 10, 0, 0.1, -1, 1)
		bone_damageFlash(body)
		hits = hits - 1
		
		shakeCamera(10, 3)
		
		entity_setStateTime(me, 1)
	elseif entity_isState(me, STATE_DIE) then
		entity_idle(n)
		entity_flipToEntity(n, me)
		
		e = getFirstEntity()
		while e ~= 0 do
			if entity_isName(e, "mantis-bomb") then
				entity_msg(e, "exp")
				watch(0.2)
			end
			e = getNextEntity()
		end
		
		spawnParticleEffect("gateway-die", entity_x(me), entity_y(me))
		fadeOutMusic(1)
		cam_toEntity(me)
		playSfx("mantis-roar")
		entity_setStateTime(me, entity_animate(me, "die"))
		entity_setFlag(me, 1)
		setFlag(FLAG_MINIBOSS_MANTISSHRIMP, 1)
	end
end

function exitState(me)
	if entity_isState(me, STATE_FIREBOMBS) or entity_isState(me, STATE_RECOVER) then
		entity_setState(me, STATE_IDLE, -1)
	elseif entity_isState(me, STATE_HURT) then
		entity_offset(me, 0, 0, 0.1)
		if hits <= 0 then
			entity_setState(me, STATE_DIE)
		else
			entity_setState(me, STATE_RECOVER)
		end
		
	elseif entity_isState(me, STATE_DIE) then
		playSfx("mantis-die")
		spawnParticleEffect("gateway-die", entity_x(me), entity_y(me))
		cam_toEntity(getNaija())
		entity_delete(me)
		
		entity_alpha(item, 1)
		
		pickupGem("boss-mantis")
	elseif entity_isState(me, STATE_FALL) then
		entity_setState(me, STATE_RECOVER)
	end
end

function animationKey(me, key)
	if entity_isState(me, STATE_FIREBOMBS) then
		if key == 1 or key == 3 or (key == 5 and chance(100)) or (key==6 and chance(50)) then
			
			
			sx, sy = bone_getWorldPosition(spawnPoint)
			e = createEntity("mantis-bomb", "", sx, sy)
			if key == 1 then
				entity_moveTowards(e, entity_x(n), entity_y(n), 1000, 1)
			else
				entity_addVel(e, math.random(500)-250, 300)
			end
			
			playSfx("mantis-fire")
		end
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if damageType == DT_ENEMY_MANTISBOMB then
		--debugLog("MANTIS BOMB DAMAGE TYPE")
		if entity_isState(me, STATE_FALL) then
			entity_setState(me, STATE_HURT)
			shakeCamera(10, 2)
			return false
		end
	end
	if damageType == DT_AVATAR_VINE then
		if entity_isState(me, STATE_IDLE) or entity_isState(me, STATE_FIREBOMBS) then
			entity_setState(me, STATE_FALL)
		end
	end
	if damageType == DT_AVATAR_ENERGYBLAST or damageType == DT_AVATAR_SHOCK then
		playNoEffect()
		return false
	end
	return false
end

