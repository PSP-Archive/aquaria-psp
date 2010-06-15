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
curNode = 1
lastNode = 0
bone_head = 0

leaveNode = 0

range = 0

function init(me)
	setupEntity(me)
	entity_setEntityLayer(me, 1)
	
	loadSound("13Touch")
	loadSound("mia-appear")
	loadSound("mia-scream")
	loadSound("mia-sillygirl")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)	
	
	entity_setPosition(me, node_getPosition(lastNode))
	
	load = false
	
	entity_initSkeletal(me, "13")
	entity_scale(me, 0.6, 0.6)
	entity_animate(me, "idle", -1)
	bone_head = entity_getBoneByName(me, "Head")
	

	
	range = entity_getNearestNode(me, "13range")
end

done1st = false
over = false
is = false	

function update(me, dt)
	entity_setLookAtPoint(me, bone_getWorldPosition(bone_head))
	
	if doFirstVision and not over then
		if node_isEntityIn(range, n) and not is then	
			is = true
			
			setCutscene(1,0)
			-- do scene
			setFlag(FLAG_VISION_ENERGYTEMPLE, 1)
			stopVoice()
			
			musicVolume(0, 2)
			
			--entity_clearVel(n)
			entity_idle(n)
			entity_setInvincible(n, true)
			-- watch for seahorse!
			watch(1)
			
			overrideZoom(0.9, 4)
			
			entity_flipToEntity(me, n)
			entity_flipToEntity(n, me)		
			
			entity_swimToNode(n, getNode("NAIJA_13"), SPEED_NORMAL)
			entity_watchForPath(n)
			
			entity_flipToEntity(me, n)
			entity_flipToEntity(n, me)				
			watch(2)
			entity_animate(me, "touchNaija")
			watch(1)
			

			ompo = getEntity("Ompo")
			entity_msg(ompo, "fade")
			
			playSfx("13Touch")
			while entity_isAnimating(me) do
				watch(FRAME_TIME)
			end
			avatar_setHeadTexture("Blink")
			watch(1)
			
			vision("EnergyTemple", 4)
			
			stopMusic()
			fade2(1, 0.1, 1, 1, 1)
			ox, oy = entity_getPosition(n)
			
			learnSong(SONG_ENERGYFORM)
			changeForm(FORM_ENERGY)
			
			setCanChangeForm(false)
			
			sn = getNode("FIRSTVISIONSTART")
			entity_setPosition(n, node_x(sn), node_y(sn))
			entity_fh(n)
			fade2(0, 2, 1, 1, 1)
			
			overrideZoom(0)
			
			emote(EMOTE_NAIJAUGH)
			
			setInvincibleOnNested(false)
			avatar_setCanDie(false)
			
			cn = getNode("FIRSTVISIONEXIT")
			
			c = 0
			while not node_isEntityIn(cn, n) do 
				wait(FRAME_TIME)
				if entity_getHealth(n) <= 1 then
					entity_heal(n, 1)
					c = c + 1
				end
				if c > 3 then
					break
				end
			end
			entity_heal(n, 100)
			fade2(1, 0.5, 1, 1, 1)
			watch(0.5)
			entity_heal(n, 100)
			
			setCanChangeForm(true)
			changeForm(FORM_NORMAL)
			unlearnSong(SONG_ENERGYFORM)
			
			setInvincibleOnNested(true)
			
			n13 = getNode("NAIJA_13")
			
			entity_setPosition(n, node_x(n13), node_y(n13))
			entity_rotate(n, 0)
			entity_flipToEntity(n, me)
			entity_idle(n)
			entity_offset(n, 0, 0)
			watch(1)
			
			entity_heal(n, 100)
			avatar_setCanDie(true)
			
			fade2(0, 0.5, 1, 1, 1)
			
			vision("EnergyTemple", 4)
			
			fade2(1, 0.5, 1, 1, 1)
			watch(0.5)
			fade2(0, 5, 1, 1, 1)
			watch(5)
			
			entity_alpha(me, 0.01, 3)
			over = true
			entity_setPosition(me, entity_x(me)+50, entity_y(me)-50, 5, 0, 0, 1)
			setMusicToPlay("OpenWaters")
			playMusic("OpenWaters")					
			watch(2)

			entity_setInvincible(n, false)
			entity_rotate(me, -90, 2, 0, 0, 1)
			playSfx("mia-sillygirl")
			watch(1)
			avatar_setHeadTexture("")
			
			
			entity_animate(me, "trail")
			nx, ny = node_getPosition(getNode("13LEAVE"))
			entity_setPosition(me, nx, ny, 8, 0, 0, 1)
			entity_setCullRadius(me, 4000)
			watch(0.5)
			
			watch(2)
			emote(EMOTE_NAIJASADSIGH)
			watch(2)
			
			voice("naija_vision_mainarea")
			
			setBeacon(BEACON_SONGCAVE, true, 128.578, 159.092, 0.5, 1, 1)
			beaconEffect(BEACON_SONGCAVE)
			
			setCutscene(0,0)
			is = false
			--entity_setState(me, STATE_TRANS, 2)
		end
	end
end


function enterState(me)
end

function exitState(me)
end

function msg(me, msg)
	if msg == "firstvision" then
		doFirstVision = true
	end
end
