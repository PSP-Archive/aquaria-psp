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
-- D E E P   C R A W L E Y
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")

-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

size = 0
t = 0.5
size0 = 1.5

n = 0
mld = 0.2
ld = mld
note = -1
excited = 0
excited = 0
glow = 0

maxSpeed = 321 + math.random(32)
width = 128
dir = -1

STATE_ROTATE = 1000
STATE_WALK = 1001
STATE_MOVEAWAY = 1002

-- ================================================================================================
-- F U N C T I O N S
-- ================================================================================================

function init(me)
	setupEntity(me)
	entity_setEntityLayer(me, -2)
	entity_setEntityType(me, ET_ENEMY)
	entity_setTexture(me, "deepcrawley")
	--entity_setAllDamageTargets(me, false)
	
	--entity_generateCollisionMask(me)
	
	entity_setState(me, STATE_IDLE)
	entity_addRandomVel(me, 500)

	esetv(me, EV_TYPEID, EVT_GLOBEJELLY)
	
	entity_setHealth(me, 3)
	entity_setDropChance(me, 20, 1)
	
	entity_setDeathParticleEffect(me, "TinyRedExplode")
	entity_setUpdateCull(me, 4000)

	-- SLIGHT SCALE AND COLOUR VARIATION
	sz = 0.8 + (math.random(400) * 0.001)
	entity_scale(me, sz, sz)
	cl = 1.0 - (math.random(2345) * 0.0001)
	entity_color(me, cl, cl, cl)
	width = width * sz
	entity_setCollideRadius(me, width)

	entity_scale(me, size0, size0)

	glow = createQuad("Naija/LightFormGlow", 13)
	quad_scale(glow, 4, 4)
end

function postInit(me)
	n = getNaija()

	entity_setMaxSpeed(me, maxSpeed)
	entity_rotate(me, randAngle360())
	entity_addRandomVel(me, 123)
	
	if chance(50) then dir = 1 end
end

function update(me, dt)
	ld = ld - dt
	if ld < 0 then
		ld = mld
		l = createQuad("Naija/LightFormGlow", 13)
		r = 1
		g = 1
		b = 1
		if note ~= -1 then
			r, g, b = getNoteColor(note)
			r = r*0.5 + 0.5
			g = g*0.5 + 0.5
			b = b*0.5 + 0.5
		end
		quad_setPosition(l, entity_getPosition(me))
		quad_scale(l, 4.0, 4.0)
		quad_alpha(l, 0.1)
		quad_alpha(l, 0.7, 0.5)
		quad_color(l, r, g, b)		
		quad_delete(l, 2)
		quad_color(glow, r, g, b, 0.5)
	end

	entity_clearTargetPoints(me)
	
	-- ROTATE GENTLY
	rotSpeed = (entity_getVelLen(me)/300) + 1
	if entity_velx(me) < 0 then dir = -1
	else dir = 1 end
	entity_rotateTo(me, entity_getRotation(me) + (rotSpeed * dir))
	
	-- AVOIDANCE
	if entity_getBoneLockEntity(n) ~= me then entity_doEntityAvoidance(me, dt, entity_getCollideRadius(me)*2.1, 1.23) end
	entity_doCollisionAvoidance(me, dt, ((entity_getCollideRadius(me)*0.01)*7)+1, 0.421)
	-- MOVEMENT
	if entity_getVelLen(me) > 64 then entity_doFriction(me, dt, 42) end
	entity_updateMovement(me, dt)

	entity_handleShotCollisions(me)
	if entity_touchAvatarDamage(me, entity_getCollideRadius(me), 1, 400) then
		entity_moveTowardsTarget(me, 1, -500)
	end

	quad_setPosition(glow, entity_getPosition(me))
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_ROTATE) then
		entity_animate(me, "walk", -1)
	elseif entity_isState(me, STATE_WALK) then
		entity_animate(me, "walk", -1)		
	end
		
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	if not entity_isInvincible(me) and (damageType == DT_AVATAR_ENERGYBLAST or damageType == DT_AVATAR_SHOCK or damageType == DT_AVATAR_LIZAP) then
		entity_heal(me, 999)
		
		size = size + dmg
		maxSpeed = maxSpeed + dmg * 10
		if size >= 16 then
			entity_setState(me, STATE_EXPLODE)
		end	
		--entity_setCollideRadius(me, getRadius(me))
		entity_setCollideRadius(me, entity_getCollideRadius(me)-(size*0.5))
		
		sz = size0 - (size * 0.1)
		entity_scale(me, sz, sz, 0.5)
	end
	return true
end

function animationKey(me, key)
end

function hitSurface(me)
	--debugLog("HIT")
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

