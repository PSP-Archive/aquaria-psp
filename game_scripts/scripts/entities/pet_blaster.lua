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

-- P E T  N A U T I L U S

dofile("scripts/entities/entityinclude.lua")

STATE_ATTACKPREP		= 1000
STATE_ATTACK			= 1001

lungeDelay = 0

spinDir = -1

rot = 0
rot2 = 0
shotDrop = 0

fireDelay = 0
shotsFired = 0
fig = 1

function init(me)
	setupBasicEntity(
	me,
	"",								-- texture
	4,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	0,								-- collideRadius (only used if hit entities is on)
	STATE_IDLE,						-- initState
	90,								-- sprite width
	90,								-- sprite height
	1,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	-1,								-- updateCull -1: disabled, default: 4000
	1
	)
	
	entity_initSkeletal(me, "Blaster")
	
	entity_scale(me, 0.5, 0.5)
	
	entity_setDeathParticleEffect(me, "TinyBlueExplode")

	lungeDelay = 1.0
	
	rot = 0
	
	esetv(me, EV_LOOKAT, 0)
	esetv(me, EV_ENTITYDIED, 1)
	esetv(me, EV_TYPEID, EVT_PET)
	
	for i=DT_AVATAR,DT_AVATAR_END do
		entity_setDamageTarget(me, i, false)
	end
end

function postInit(me)
	n = getNaija()
end

function update(me, dt)
	if getPetPower()==1 then
		entity_setColor(me, 1, 0.5, 0.5, 0.1)
	else
		entity_setColor(me, 1, 1, 1, 1)
	end
	
	if not isInputEnabled() or not entity_isUnderWater(n) then
		entity_setPosition(me, entity_x(n), entity_y(n), 0.3)
		entity_alpha(me, 0, 0.1)
		return
	else
		entity_alpha(me, 1, 0.1)
	end
	
	naijaUnder = entity_y(n) > getWaterLevel()
	if naijaUnder then
		if entity_y(me)-32 < getWaterLevel() then
			entity_setPosition(me, entity_x(me), getWaterLevel()+32)
		end
	else
		if entity_isState(me, STATE_FOLLOW) then
			entity_setPosition(me, entity_x(n), entity_y(n), 0.1)
		end
	end
	
	lastx, lasty = entity_getPosition(me)
	
	if entity_isState(me, STATE_FOLLOW) then
		
		rot = rot + dt*0.75
		rot2 = rot2 + dt *0.25
		if rot > 1 then
			rot = rot - 1
			
			if fig == 1 then
				fig = -1
			else
				fig = 1
			end
		end
		if rot2 > 1 then
			rot2 = rot2 - 1
		end
		dist = 200
		t = 0
		t2 = 0
		x = 0
		y = 0
		if avatar_isRolling() then
			dist = 90
			spinDir = -avatar_getRollDirection()
			t = rot * 6.28
			t2 = rot2 * 6.28
		else
			t = rot * 6.28
			t2 = rot2 * 6.28
		end
		
		if not entity_isEntityInRange(me, n, 1024) then
			entity_setPosition(me, entity_getPosition(n))
		end
		
		x = x + math.cos(t)*dist
		y = y + math.sin(t2)*dist
		
		if naijaUnder then
			entity_setPosition(me, entity_x(n)+x, entity_y(n)+y, 0.6)
		end
		
		--entity_handleShotCollisions(me)
		
		ent = entity_getNearestEntity(me, "", 600, ET_ENEMY, DT_AVATAR_ENERGYBLAST)
		if ent~= 0 and not entity_isDamageTarget(ent, DT_AVATAR_PET) then
			ent = 0
		end
		
		off = 0
		t = 0.15
		if ent == 0 then
			ent = n
			entity_rotateOffset(me, 180)
			t = 0
		else
			
			entity_rotateOffset(me, 0)
		end
		
		if ent ~= 0 then
			entity_rotateToEntity(me, ent, t, off)
		end
		
		lungeDelay = lungeDelay - dt * (getPetPower()+1)
		if lungeDelay < 0 then
			fireDelay = fireDelay - dt * (getPetPower()+1)
			if fireDelay < 0 then			
				if ent ~= 0 and ent ~= n then
					s = createShot("PetBlasterFire", me, ent, entity_x(me), entity_y(me))
					tx, ty = entity_getTargetPoint(ent, 0)
					vx = tx - entity_x(me)
					vy = ty - entity_y(me)
					shot_setAimVector(s, vx, vy)
				end
				shotsFired = shotsFired + 1
				fireDelay = 0.2
			end
			if shotsFired >= 3 then
				lungeDelay = 3
				fireDelay = 0
				shotsFired = 0
			end
		end
	end
end

function entityDied(me, ent)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_FOLLOW) then
		entity_animate(me, "idle", -1)
	end
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function hitSurface(me)
end

function shiftWorlds(me, old, new)
end
