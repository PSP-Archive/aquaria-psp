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
dofile(appendUserDataPath("_mods/jukebox/scripts/jukeboxinclude.lua"))

local naija = 0
mbDown = false

function init(me)
	resetContinuity()
	setOverrideMusic("")

	naija = getNaija()

	entity_heal(naija, 999)
	cam_toNode(me)
	cam_setPosition(node_x(me), node_y(me))
	entity_setInvincible(naija, true)
	entity_setState(naija, STATE_TITLE)


	setMousePos(400, 550)

	disableInput()
	toggleCursor(true, 0.1)
	overrideZoom(1.0)

	resetTimer()

	local throne = getNode("naija")
	avatar_toggleCape(false)
	entity_animate(naija, "sitthrone", -1, 4)
	entity_setPosition(naija, node_x(throne), node_y(throne))

	setMousePos(400, 550)

	fade(1, 0)
	fade(0, 1)
	fade2(0, 2, 1, 1, 1)

	jukebox_playSong(math.random(#SONG_LIST))
end

function update(me, dt)
	cam_setPosition(node_x(me), node_y(me))
	if isInputEnabled() then
		disableInput()
		toggleCursor(true, 0.1)
	end

	scale = 800.0/1024.0 + 0.01
	
	overrideZoom(scale, 0)
	
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
