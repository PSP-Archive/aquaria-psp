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

-- generic collectible costume

dofile("scripts/include/collectibletemplate.lua")

on = false
cname = ""

function commonInit2(me, gfx, flag, costumeName)
	commonInit(me, gfx, flag, true)
	cname = costumeName
	entity_setEntityLayer(me, -1)
	
	-- cached now
	loadSound("ChangeClothes1")
	loadSound("ChangeClothes2")
end

function update(me, dt)
	commonUpdate(me, dt)

	if entity_isState(me, STATE_COLLECTEDINHOUSE) then
		if getCostume() == cname then
			if on then
				entity_alpha(me, 0.5)
				on = false
			end
		else
			if not on then
				entity_alpha(me, 1)
				on = true
			end
		end
	end
end

function enterState(me, state)
	commonEnterState(me, state)
	if entity_isState(me, STATE_COLLECTEDINHOUSE) then
		entity_setActivation(me, AT_CLICK, 32, 700)
	end
end

function activate(me)
	if entity_isState(me, STATE_COLLECTEDINHOUSE) then
		-- go to changing area
		
		if not isForm(FORM_NORMAL) then
			changeForm(FORM_NORMAL)
		end
		node = getNodeByName("CHANGE")
		avatar_fallOffWall()
		watch(0.5)
		entity_swimToNode(getNaija(), node)
		entity_watchForPath(getNaija())
		entity_idle(getNaija())
		entity_setColor(getNaija(), 0.01, 0.01, 0.01, 1)
		watch(0.5)
		
		entity_animate(getNaija(), "changeCostume")
		watch(1)
		playSfx("ChangeClothes1")
		watch(1)
		playSfx("ChangeClothes2")
		watch(1.2)
		
		watch(0.6)
		playSfx("ChangeClothes1")
		if getCostume() == cname then
			setCostume("")
		else
			setCostume(cname)
		end	
		while entity_isAnimating(getNaija()) do
			watch(FRAME_TIME)
		end
		
		watch(0.5)
		-- change	

		--watch(0.5)
		entity_setColor(getNaija(), 1, 1, 1, 0.5)
		entity_swimToNode(getNaija(), getNodeByName("CHANGEEXIT"))
		entity_watchForPath(getNaija())	
		if chance(50) then
			emote(EMOTE_NAIJAGIGGLE)
		end
	end
end

function exitState(me, state)
	commonExitState(me, state)
end
