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

-- energy door
dofile("scripts/entities/entityinclude.lua")

glow = 0
spiral = 0
sceneName = ""
nodeName = ""
flip = ""

function commonInit(me)
	setupEntity(me, "", -2)
	entity_initSkeletal(me, "SongDoor")
	glow = entity_getBoneByName(me, "Glow")
	spiral = entity_getBoneByName(me, "Spiral")
	entity_scale(me, 1.25,1.25)
	entity_setActivationType(me, AT_NONE)
	entity_setFillGrid(me, true)
	entity_setWidth(me, 512)
	entity_setHeight(me, 512)
	--entity_scale(me, 1.5, 1.5)
	bone_alpha(glow, 0)
end

function setWarpSceneNode(scene, node, f)
	sceneName = scene
	nodeName = node
	flip = f
end

function update(me, dt)
	--debugLog("updating!")
	if entity_isState(me, STATE_OPENED) then
		--debugLog("is opened")
		vx,vy = entity_getPosition(getNaija())
		vx2,vy2 = bone_getWorldPosition(spiral)
		if isWithin(vx, vy, vx2, vy2, 140) then
			--debugLog("gotcha!")
			--warpAvatar("NaijaCave", 400, 300)
			if sceneName~="" then
				warpNaijaToSceneNode(sceneName, nodeName, flip)
			end
		end
	end
	if entity_isState(me, STATE_OPEN) then
		if not entity_isAnimating(me) then			
			entity_setState(me, STATE_OPENED)
		end
	end
end

function animationKey(me, key)
	if key == 1 then
		debugLog("Fading out door")
		x, y = bone_getWorldPosition(spiral)
		spawnParticleEffect("SongDoorOpen", x, y)
		bone_alpha(spiral, 0, 2.9)
	end
end

function enterState(me)
	if entity_isState(me, STATE_OPEN) then
		bone_alpha(glow, 1, 2)
		entity_animate(me, "open")
		
	elseif entity_isState(me, STATE_OPENED) then
		entity_animate(me, "opened", LOOP_INF)
		bone_alpha(glow, 1)
		bone_alpha(spiral, 0)
	elseif entity_isState(me, STATE_CLOSED) then
		entity_animate(me, "idle", LOOP_INF)
	end
end

function exitState(me)
end

function hitSurface(me)
end
