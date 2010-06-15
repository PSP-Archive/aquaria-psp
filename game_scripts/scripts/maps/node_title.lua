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

--SCRIPT_OFF

n = 0
mbDown = false
labelTimer = 0

function init(me)
	--stopAllVoice()
	resetContinuity()
	setOverrideMusic("")
	
	n = getNaija()
	
	entity_alpha(n, 0.2)
	
	entity_heal(n, 999)
	cam_toNode(me)
	cam_setPosition(node_x(me), node_y(me))
	entity_setInvincible(n, true)
	entity_setState(n, STATE_TITLE)
	
	--entity_stopAllAnimations(n)
	entity_animate(n, "frozen", -1, 4)
	--c = createEntity("Crotoid", "", 400, 300)

	setMousePos(400, 550)
	
	disableInput()
	toggleCursor(true, 0.1)
	overrideZoom(1.0)
	
	--toggleBlackBars(true)
	
	resetTimer()
	
	playMusicStraight("Title")
	
	
	avatar_toggleCape(false)
	
	setMousePos(400, 550)
	
	fade(1, 0)
	fade(0, 1)
	fade2(0, 2, 1, 1, 1)
	
	setVersionLabelText()
	toggleVersionLabel(1)
	
	labelTimer = 0
end

function update(me, dt)
	labelTimer = labelTimer + dt
	if labelTimer > 0.5 then
		setVersionLabelText()
	end
	cam_setPosition(node_x(me), node_y(me))
	if isInputEnabled() then
		debugLog("calling disable input")
		disableInput()
		toggleCursor(true, 0.1)
	end

	scale = 800.0/1024.0 + 0.01
	
	overrideZoom(scale, 2)
	
	entity_setPosition(n, node_getPosition(getNode("NAIJA")))
	
	if (isLeftMouse() or isRightMouse()) and not mbDown then
		mbDown = true
	elseif (not isLeftMouse() and not isRightMouse()) and mbDown then
		mbDown = false
		node = getNodeToActivate()
		setNodeToActivate(0)
		stopCursorGlow()
		if node ~= 0 then
			node_activate(node, 0)
		end
	end
end
