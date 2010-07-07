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

v = getVars()

dofile("scripts/entities/entityinclude.lua")

function init(me)
	node_setCursorActivation(me, true)
end
	
function activate(me)
	v.n = getNaija()
	avatar_fallOffWall()
	entity_idle(v.n)
	entity_setInvincible(v.n, true)
	entity_swimToNode(v.n, me)
	entity_watchForPath(v.n)
	--avatar_toggleCape(false)
	entity_animate(v.n, "sitBack", LOOP_INF)
	--entity_animate(v.n, "trailerIntroAnim2", LOOP_INF)
	overrideZoom(0.5, 2)
	watch(2)
	
	emote(EMOTE_NAIJASIGH)

	
	while (not isLeftMouse()) and (not isRightMouse()) do
		watch(FRAME_TIME)		
	end
	entity_idle(v.n)
	entity_addVel(v.n, 0, -200)
	overrideZoom(1, 1)
	esetv(v.n, EV_NOINPUTNOVEL, 0)
	watch(1)
	esetv(v.n, EV_NOINPUTNOVEL, 1)
	overrideZoom(0)
	entity_setInvincible(v.n, false)
	
	--avatar_toggleCape(true)
end

function update(me, dt)
end
