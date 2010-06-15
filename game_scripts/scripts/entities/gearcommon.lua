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

rotSpd = 0.0
n=0
gear = 0
gearBack = 0
actDelay = 0
t = 15
soundTimer =0

useSetRotSpd = 0

function commonInit(me, usrs)
	useSetRotSpd = usrs
	entity_setEntityType(me, ET_NEUTRAL)
	--entity_setTexture(me, "")
	entity_initSkeletal(me, "Gear")
	--entity_setWidth(me, 64)
	--entity_setHeight(me, 64)
	entity_setUpdateCull(me, -1)
	n = getNaija()
	
	entity_setCollideRadius(me, 160)
	
	gear = entity_getBoneByName(me, "Gear")
	gearBack = entity_getBoneByName(me, "GearBack")
	
	entity_scale(me, 1.5, 1.5)
	
	loadSound("GearTurn")
	loadSound("GearWaterLevel")
	
	esetv(me, EV_BEASTBURST, 0)
	esetv(me, EV_LOOKAT, 0)
end

function postInit(me)
	mult = 1
	node = entity_getNearestNode(me, "FLIP")
	if node ~= 0 and node_isEntityIn(node, me) then
		useSetRotSpd = -useSetRotSpd
	end
end

function enterState(me)
end

function exitState(me)
end

function activate(me)
end

function songNote(me, note)
end

function damage(me)
	return false
end

function doFunction(me)
	if actDelay == 0 then
		actDelay = t
		node = entity_getNearestNode(me)
		if node ~= 0 and node_isEntityIn(node, me) then
			node_activate(node)
		end
		playSfx("GearWaterLevel")
	end
end

function update(me, dt)
	spinning = false
	if actDelay > 0 then
		actDelay = actDelay - dt
		if actDelay < 0 then
			actDelay = 0
		end
	end
	
	
	if useSetRotSpd==0 then
		if entity_isEntityInRange(me, n, 600) then
			if entity_isUnderWater(n) and avatar_isRolling() then
				rotSpd = rotSpd + 90*dt*avatar_getRollDirection()
				if rotSpd > 360 then
					rotSpd = 360
				elseif rotSpd < -360 then
					rotSpd = -360
				end
				spinning = true
				
			end
		end
	else
		spinning = true
		rotSpd = useSetRotSpd
	end
	--debugLog(string.format("rotspd:%d", rotSpd))
	if rotSpd ~= 0 then
		
		entity_rotate(me, entity_getRotation(me)+rotSpd*dt)
		--bone_rotate(gear, bone_getRotation(gear)+rotSpd*dt)
		bone_rotate(gearBack, bone_getRotation(gearBack)-rotSpd*2*dt)
		
		if bone_getRotation(gear) > 360 then
			bone_rotate(gear, bone_getRotation(gear)-360)
			--entity_sound(me, "GearTurn")
		elseif bone_getRotation(gear) < -360 then
			bone_rotate(gear, bone_getRotation(gear)+360)
			--entity_sound(me, "GearTurn")			
		end
		soundTimer = soundTimer + rotSpd*dt
		--debugLog(string.format("soundTimer: %f", soundTimer))
		intv = 90
		if soundTimer > intv then
			soundTimer = 0
			entity_sound(me, "GearTurn")
		end
		if soundTimer < -intv then
			soundTimer = 0
			entity_sound(me, "GearTurn")
		end

		if not spinning then
			dir = 1
			if rotSpd > 0 then
				dir = -1
			end
			rotSpd = rotSpd + (30.0*dt*dir)
			if dir == 1 and rotSpd > 0 then
				rotSpd = 0
			elseif dir == -1 and rotSpd < 0 then
				rotSpd = 0
			end
		end
	end

	minSpd = 300
	if rotSpd > minSpd or rotSpd < -minSpd then
		doFunction(me)
	end
	
	if entity_touchAvatarDamage(me, entity_getCollideRadius(me), 0) then
		if avatar_isLockable() and entity_setBoneLock(n, me) then
		else
			x, y = entity_getVectorToEntity(me, n, 8000)
			entity_addVel(n, x, y)
		end
	end
	entity_handleShotCollisions(me)
end
