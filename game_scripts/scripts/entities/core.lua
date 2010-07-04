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

node = 0

dir = 0
dirTimer = 0

bone_body = 0
bone_eye1 = 0
bone_eye2 = 0
bone_eye3 = 0
bone_eye4 = 0
bone_tentacles = 0

function doEye(bone)
	bone_scale(bone, 0.6, 0.6, 0)
	bone_scale(bone, 1.2, 1.2, 4 + math.random(6))
end

function updateEye(bone)
	x,y = bone_getScale(bone)
	if x > 1.1 then
		sx,sy = bone_getWorldPosition(bone)
		createEntity("Moneye", "", sx, sy)
		spawnParticleEffect("TinyGreenExplode", sx, sy)
		doEye(bone)
	end
end

function clearBarriers()
	if node ~= 0 then
		-- do magic
		node_setElementsInLayerActive(node, 2, false)
		reconstructGrid()
	end
end

function init(me)
	if entity_isFlag(me, 1) then
		return
	end

	setupBasicEntity(
	me,
	"",								-- texture
	64,								-- health
	1,								-- manaballamount
	1,								-- exp
	1,								-- money
	135,							-- collideRadius
	STATE_IDLE,						-- initState
	0,								-- sprite width
	0,								-- sprite height
	0,								-- particle "explosion" type, maps to particleEffects.txt -1 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000
	)
	
	entity_initSkeletal(me, "Core")	
	entity_animate(me, "idle")
	
	bone_body = entity_getBoneByName(me, "Body")
	bone_eye1 = entity_getBoneByName(me, "Eye1")
	bone_eye2 = entity_getBoneByName(me, "Eye2")
	bone_eye3 = entity_getBoneByName(me, "Eye3")
	bone_eye4 = entity_getBoneByName(me, "Eye4")
	bone_tentacles = entity_getBoneByName(me, "Tentacles")
	
	doEye(bone_eye1)
	doEye(bone_eye2)
	doEye(bone_eye3)
	doEye(bone_eye4)
	
	bone_setSegs(bone_body, 2, 8, 0.5, 0.1, -0.018, 0, 2, 1)
	bone_setSegs(bone_tentacles, 2, 8, 0.5, 0.1, -0.018, 0, 3, 0.5)
	
	
	entity_setMaxSpeed(me, 100)
	
	--entity_setCullRadius(me, 1024)
end

function postInit(me)
	node = entity_getNearestNode(me, "CORERANGE")
	
	if entity_isFlag(me, 1) then
		clearBarriers()
		entity_delete(me)
	end
end

function update(me, dt)
	entity_handleShotCollisions(me)
	
	entity_touchAvatarDamage(me, entity_getCollideRadius(me), 0, 1000)
	
	if dir == 0 then
		entity_addVel(me, -100*dt, 0)
	else
		entity_addVel(me, 100*dt, 0)
	end
	
	dirTimer = dirTimer - dt
	if dirTimer < 0 then
		dirTimer = math.random(2)+1
		if dir == 0 then
			dir = 1
		else
			dir = 0
		end
	end
	
	entity_updateCurrents(me, dt)
	entity_updateMovement(me, dt)
	
	updateEye(bone_eye1)
	updateEye(bone_eye2)
	updateEye(bone_eye3)
	updateEye(bone_eye4)
end

function enterState(me, state)
	if entity_isState(me, STATE_DEAD) then
		clearBarriers()
		entity_setFlag(me, 1)
	end
end

function damage(me)
	return true
end
