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

core = 0
ring = 0
n=0
spawnTimer = 0
closeRange = 860
farRange = 950
started = false

STATE_READY				= 1000
STATE_SPAWNJELLIES		= 1001
STATE_MOVECORE			= 1002

maxHealth = 60

zaps = {}
zapGlows = {}

nZaps = 3

zapTimer = 0
zapStart = 5
zapTime = 5
zapGlowStart = 3

zapsOn = false
zapGlowOn = false
dist = 90
start = 0
spread = 120

hardLevel = 0

function init(me)
	setupEntity(me)
	--entity_setEntityLayer(me, 1)
	entity_initSkeletal(me, "KingJelly")
	entity_setTargetPriority(me, 1)
	entity_setEntityType(me, ET_ENEMY)
	entity_setCollideRadius(me, 120)
	entity_generateCollisionMask(me)
	entity_animate(me, "idle")
	entity_setCull(me, false)
	
	entity_setHealth(me, maxHealth)
	
	core = entity_getBoneByName(me, "Core")
	ring = entity_getBoneByName(me, "Ring")
	
	--bone_scale(core, 1.2, 1.2, 1, -1)
	
	entity_setState(me, STATE_IDLE)	
	n = getNaija()
	
	--entity_setActivation(me, AT_CLICK, 64, 512)
	
	--[[
	entity_setAllDamageTargets(me, false)
	entity_setDamageTarget(me, DT_AVATAR_ENERGYBLAST, true)
	entity_setDamageTarget(me, DT_AVATAR_SHOCK, true)
	]]--
	
	for i=5,5+nZaps do
		zaps[i-4] = entity_getBoneByIdx(me, i)
		bone_alpha(zaps[i-4], 0)
	end
	
	for i=10,10+nZaps do
		zapGlows[i-9] = entity_getBoneByIdx(me, i)
		bone_alpha(zapGlows[i-10], 0)
	end	
	
	--entity_initStrands(me, 32, 16, 256, 20, 0.8, 0.9, 1.0)
	--bone_setPosition(core, 0, -40, 3, -1, 1, 1)
	entity_setDeathScene(me, true)
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
end

function postInit(me)
	if isFlag(FLAG_MINIBOSS_KINGJELLY, 1) then
		entity_delete(me)
	end
end

function activate(me)
	entity_setActivationType(me, AT_NONE)
	entity_setState(me, STATE_READY)
end

function killJellies(me)
	ent = getFirstEntity()
	while ent ~= 0 do
		if ent ~= me and entity_getEntityType(ent)==ET_ENEMY and entity_isEntityInRange(me, ent, farRange*2) and entity_isName(ent, "EvilJelly") then
			entity_damage(ent, me, 1000)
		end
		ent = getNextEntity()
	end
end

function toggleZaps(me, on)
	zapsOn = on
	if on then
		toggleZapGlow(me, false)
		for i=1,nZaps do
			bone_alpha(zaps[i], 1, 0.1)
		end
		for i=1,nZaps do
			bone_setSegs(zaps[i], 2, 32, 0.8, 0.8, -0.1, 0, 50, 1)
			bone_rotate(zaps[i], start+(i-1)*spread)
			bone_rotate(zaps[i], start+dist+(i-1)*spread, zapTime/2, -1, 1, 1)
		end		
	else
		toggleZapGlow(me, false)
		for i=1,nZaps do
			bone_alpha(zaps[i], 0, 0.1)
		end
	end
end

function toggleZapGlow(me, on)
	zapGlowOn = on
	if on then
		start = math.random(360*2)-360
		for i=1,nZaps do
			--debugLog("setting alpha to 1")
			bone_rotate(zapGlows[i], start+(i-1)*spread)
			bone_alpha(zapGlows[i], 1, 0.2)
			bone_scale(zapGlows[i], 1, 1)
			bone_scale(zapGlows[i], 1.5, 1.5, 0.2, -1)
		end
	else
		for i=1,nZaps do
			--debugLog("setting alpha to 0")
			bone_alpha(zapGlows[i], 0, 0.5)
		end	
	end
end

function zapCollision(me, dt)
	if not zapsOn then return end
	if avatar_isShieldActive() then return end
	for i=1,nZaps do
		rot = bone_getRotation(zaps[i])
		if entity_collideCircleVsLineAngle(n, rot, 64, 900, 8, entity_getPosition(me)) then
			entity_damage(n, me, 1)
		end
	end
end

function update(me, dt)
	if entity_isState(me, STATE_DESCEND) and not entity_isInterpolating(me) then
		--entity_setState(me, STATE_IDLE)
		activate(me)
	end
	if entity_getHealthPerc(me) < 0.75 then
		hardLevel = 1
	elseif entity_getHealthPerc(me) < 0.90 then
		hardLevel = 2
	end
	if hardLevel == 2 then
		dt = dt * 1.25
	end
	if not entity_isState(me, STATE_DEATHSCENE) then
		entity_handleShotCollisionsSkeletal(me)
		entity_handleShotCollisions(me)
		
		bone = entity_collideSkeletalVsCircle(me, n)
		if bone ~= 0 then
			nx,ny = entity_getPosition(n)
			cx,cy = entity_getPosition(me)
			x = nx-cx
			y = ny-cy
			x,y = vector_setLength(x,y,2000)
			entity_addVel(n, x, y)
			entity_damage(n, me, 1)
		end
		entity_touchAvatarDamage(me, entity_getCollideRadius(me), 0, 2000)
	else
		killJellies(me)
	end
	
	if not entity_isState(me, STATE_IDLE) and not entity_isState(me, STATE_DEATHSCENE) then
		if hardLevel > 0 then
			zapTimer = zapTimer + dt
			if zapTimer > zapGlowStart and zapTimer < zapStart and not zapGlowOn then
				toggleZapGlow(me, true)
			end
			if zapTimer > zapStart and not zapsOn then
				toggleZaps(me, true)
			elseif zapTimer > zapStart+zapTime and zapsOn then
				toggleZaps(me, false)
				zapTimer = 0
			end
			zapCollision(me, dt)
		end
		ent = getFirstEntity()
		while ent ~= 0 do
			if ent ~= me and (entity_getEntityType(ent)==ET_AVATAR or entity_isEntityInRange(me, ent, farRange)) then
				if not entity_isEntityInRange(me, ent, closeRange) or entity_y(ent) > entity_y(me)+512 then
					if not entity_isState(me, STATE_DESCEND) or entity_y(ent) < entity_y(me) then
						if entity_getEntityType(ent) == ET_AVATAR then
							avatar_fallOffWall()
						end
						nx,ny = entity_getPosition(ent)
						cx,cy = entity_getPosition(me)
						x = nx-cx
						y = ny-cy
						x,y = vector_setLength(x,y,-2000)
						entity_clearVel(ent)
						entity_addVel(ent, x, y)
					end
					
				end
			end
			ent = getNextEntity()
		end
		
		if not entity_isState(me, STATE_DESCEND) then
			spawnTimer = spawnTimer + dt
			if spawnTimer > 10 then
				for i=1,2+hardLevel do
					createEntity("EvilJelly", "", entity_getPosition(me))
				end
				spawnTimer = 0
			end
		end
	end
	if entity_isState(me, STATE_READY) then
		bone_rotate(ring, bone_getRotation(ring)+dt*20)
	end
end


function enterState(me)
	if entity_isState(me, STATE_IDLE) then
	elseif entity_isState(me, STATE_READY) then
		if not started then
			overrideZoom(0.5, 1)
			playMusic("MiniBoss")
			emote(EMOTE_NAIJAUGH)
			started = true
		end
		for i=1,2+hardLevel do
			createEntity("EvilJelly", "", entity_getPosition(me))
		end			
	elseif entity_isState(me, STATE_MOVECORE) then
		bone_rotate(ring, math.random(360*2)+360*6, 4, 0, 0, 1)
		entity_setStateTime(me, 4)
	elseif entity_isState(me, STATE_DEAD) then
		overrideZoom(0)
	elseif entity_isState(me, STATE_DESCEND) then
		node = getNode("KINGJELLYPOS")
		entity_setPosition(me, node_x(node), node_y(node), -200, 0, 0, 1)
		--playMusic("inevitable")
	elseif entity_isState(me, STATE_DEATHSCENE) then
		setFlag(FLAG_MINIBOSS_KINGJELLY, 1)
		fadeOutMusic(10)
		entity_setStateTime(me, -1)
		entity_idle(n)
		entity_clearVel(n)
		entity_flipTo(n, me)
		for i=1,10 do
			killJellies(me)
			watch(0.1)
			killJellies(me)
		end
		playSfx("BeastForm")
		entity_scale(me, 0.05, 0.05, 4)
		overrideZoom(1, 4)
		watch(4)
		overrideZoom(0)
		
		pickupGem("boss-jelly")
		
		--entity_setStateTime(me, 0)
		entity_delete(me)
	end
end

function exitState(me)
	if entity_isState(me, STATE_MOVECORE) then
		rot = bone_getRotation(ring)
		while rot > 360 do
			rot = rot - 360
		end
		bone_rotate(ring, rot)
		entity_setState(me, STATE_READY)
	end
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function hitSurface(me)
	--entity_sound(me, "rock-hit")
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_DESCEND) then
		return false
	end
	if bone == ring then
		return false
	end
	if entity_isState(me, STATE_READY) then
		if bone == 0 then
		--[[
			if entity_getHealth(me)-dmg <= 0 then
				deathScene(me)
			end
			]]--
			entity_setState(me, STATE_MOVECORE)
			return true
		end
	end
	if entity_isState(me, STATE_MOVECORE) then
		if bone == 0 then
			return true
		end
	end
	return false
end
