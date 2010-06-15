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

curNote = -1
noteTimer = 0

delayTime = 2
delay = delayTime

attackDelay = 0

noteQuad = 0



maxHits = 6
hits = maxHits

STATE_HURT				= 1000
STATE_SING				= 1001

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_setTexture(me, "particles/lines")
	entity_setAllDamageTargets(me, false)
	
	entity_rotate(me, 360, 1, -1)
	
	entity_scale(me, 1.5, 1.5, 2, -1, 1, 1)

	entity_setCollideRadius(me, 32)	
	
	entity_setState(me, STATE_IDLE)
	
	entity_setBlendType(me, BLEND_ADD)
	
	
	
	for i=0,7 do
		loadSound(getNoteName(i, "low-"))
	end
	
	entity_addVel(me, randVector(500))
	
	entity_setMaxSpeed(me, 300)
	entity_alpha(me, 0)
	
	loadSound("energyboss-die")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	
	playSfx("spirit-enter")
	entity_alpha(me, 1, 1.5)
	
	playMusic("ancienttest")
end

function update(me, dt)
	entity_updateMovement(me, dt)
	
	entity_doCollisionAvoidance(me, dt, 10, 0.2)
	
	if entity_isState(me, STATE_IDLE) then
		delay = delay - dt
		if delay < 0 then
			delay = delayTime
			entity_setState(me, STATE_SING) 
		end
	end
	
	if entity_isState(me, STATE_SING) then
		if holdingNote then
			noteTimer = noteTimer + dt
			if noteTimer > 1 then
				-- hit it!
				holdingNote = false
				entity_setState(me, STATE_HURT)
			end
		end
		if noteQuad~=0 then
			quad_setPosition(noteQuad, entity_x(me), entity_y(me))
		end
	end
	
	if entity_isState(me, STATE_ATTACK) then
		entity_doEntityAvoidance(me, dt, 128, 0.2)
		attackDelay = attackDelay + (dt * (1.0-(hits/maxHits))*3)
		if attackDelay > 1 then
			s = createShot("energygodspirit", me, n, entity_x(me), entity_y(me))
			attackDelay = 0
		end
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_HURT) then
		entity_setMaxSpeedLerp(me, 4)
		entity_setMaxSpeedLerp(me, 1, 5)
		
		entity_addVel(me, randVector(500))
		
		playSfx("secret")
		playSfx("energyorbcharge")
		entity_setStateTime(me, 3)
		if hits == 1 then
			entity_setStateTime(me, 0.5)
		end
		curNote = -1
		spawnParticleEffect("energygodspirithit", entity_x(me), entity_y(me))
		entity_heal(n, 0.5)
		setSceneColor(0.5, 0.5, 1, 0.5)
		
		entity_setMaxSpeed(me, entity_getMaxSpeed(me)+50)
	elseif entity_isState(me, STATE_SING) then
		
		entity_setStateTime(me, 5)
		curNote = getRandNote()
		r, g, b = getNoteColor(curNote)
		entity_color(me, r*0.9 + 0.1, g*0.9 + 0.1, b*0.9 + 0.1, 1)
		
		playSfx(getNoteName(curNote, "low-"))
		
		
		t = 6
		noteQuad = createQuad(string.format("Song/NoteSymbol%d", curNote), 6)
		quad_alphaMod(noteQuad, 0.2)
		quad_scale(noteQuad, 1, 1)
		quad_scale(noteQuad, 3, 3, t, 0, 0, 1)
		quad_setPosition(noteQuad, entity_x(me), entity_y(me))
		quad_setBlendType(noteQuad, BLEND_ADD)
		quad_delete(noteQuad, t)
		
		setSceneColor(r*0.5 + 0.5, g*0.5 + 0.5, b*0.5 + 0.5, 1)
		
		shakeCamera(4, 3)
	elseif entity_isState(me, STATE_ATTACK) then
		entity_setMaxSpeedLerp(me, 4)
		entity_addVel(me, randVector(800))
		--entity_moveTowardsTarget(me, 800, 1)
		entity_setMaxSpeedLerp(me, 1, 5)
		entity_setStateTime(me, 4)
		setSceneColor(1, 0.5, 0.5, 1)
		attackDelay = 0
	elseif entity_isState(me, STATE_DEATHSCENE) then

	end
end

function exitState(me)
	if entity_isState(me, STATE_SING) then
		entity_color(me, 1, 1, 1, 1)
		--voice("laugh1")
		entity_setState(me, STATE_ATTACK)
		if noteQuad ~= 0 then
			quad_delete(noteQuad, 1)
			noteQuad = 0
		end
	elseif entity_isState(me, STATE_ATTACK) then
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_HURT) then
		
		
		hits = hits - 1
		
		if hits <= 0 then
			setFlag(FLAG_ENERGYGODENCOUNTER, 2)
			entity_damage(me, n, 10000)
			
			setSceneColor(1, 1, 1, 5)
			fadeOutMusic(6)
			shakeCamera(10, 4)
			cam_toEntity(me)
			shakeCamera(10, 4)
			watch(4)
			spawnParticleEffect("energygodspirithit", entity_x(me), entity_y(me))
			playSfx("energyboss-die")
			shakeCamera(15, 2)
			watch(2)
			playSfx("spirit-enter")
			shakeCamera(20, 2)
			watch(2)
			cam_toEntity(getNaija())
		
			node = entity_getNearestNode(me, "energygodencounter")
			if node ~= 0 then
				node_activate(node)
			end
		else
			entity_setState(me, STATE_ATTACK)
		end
	end
end

function damage(me, attacker, bone, damageType, dmg)
	return true
end

function animationKey(me, key)
end

function hitSurface(me)
end

function songNote(me, note)
	if curNote == note then
		holdingNote = true
		noteTimer = 0
	end
end

function songNoteDone(me, note)
	holdingNote = false
end

function song(me, song)
end

function activate(me)
end

