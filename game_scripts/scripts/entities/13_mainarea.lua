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

--- THINK THIS FILE ISN'T USED ANYMORE!!!!
--- NOT UUUUUSED!

n = 0
curNode = 1
lastNode = 0
bone_head = 0

STATE_HEADTOCAVE 	= 1000
STATE_TRANS			= 1001

function init(me)
	setupEntity(me)
	entity_setEntityLayer(me, 1)
	--entity_setBeautyFlip(me, false)
	loadSound("13Touch")
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)	
	
	lastNode = getNode(string.format("13_%d", curNode))
	entity_setPosition(me, node_getPosition(lastNode))
	-- check flags
	if isFlag(FLAG_VISION_ENERGYTEMPLE, 0) then
		entity_initSkeletal(me, "13")
		entity_scale(me, 0.6, 0.6)
		entity_animate(me, "idle", -1)
		bone_head = entity_getBoneByName(me, "Head")
	else
		entity_alpha(me)
		entity_setPosition(me)
	end
end

done1st = false
over = false
is = false	
function doScene(me)
	done1st = true
	if is then return end
	setCameraLerpDelay(1)
	is = true
	entity_idle(n)
	entity_setInvincible(n, true)
	entity_flipToEntity(n, me)
	
	ompo = getEntity("Ompo")
	entity_msg(ompo, "fmia")
	
	watch(1)	
	cam_toEntity(me)
	voice("Naija_See13")
	watch(2)
	--entity_say(me, "naija...")
	watch(4)
	--fade(1, 1)
	
	--watch(1)
	
	--watch(0.1)
	screenFadeCapture()
	
	
	setCameraLerpDelay(0)
	cam_toEntity(n)
	cam_setPosition(entity_x(n), entity_y(n))	
	
	watch(0.1)
	--watch(0.5)
	--fade(0,1)
	screenFadeTransition(2)
	watch(0.5)
	--watch(1)
	entity_setInvincible(n, false)
	is = false
end

function update(me, dt)	
	if entity_isState(me, STATE_HEADTOCAVE) then
		--debugLog("Head to Cave")
		return
	end
	if bone_head~= 0 then
		entity_setLookAtPoint(me, bone_getWorldPosition(bone_head))
	end
	if isFlag(FLAG_VISION_ENERGYTEMPLE, 0) and n~=0 then
		if not done1st and curNode == 1 and node_isEntityIn(lastNode, n) then
			doScene(me)
		end
		if not entity_isInterpolating(me) and entity_isEntityInRange(me, n, 256) and not is then
			curNode = curNode + 1
			--debugLog(string.format("CURNODE: %f", curNode))
			if curNode > 4 then
				-- do scene
				setFlag(FLAG_VISION_ENERGYTEMPLE, 1)
				stopVoice()
				
				musicVolume(0, 2)
				
				--entity_clearVel(n)
				entity_idle(n)
				entity_setInvincible(n, true)
				-- watch for seahorse!
				watch(1)
				
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
				setNaijaHeadTexture("Blink")
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
				
				emote(EMOTE_NAIJAUGH)
				
				setInvincibleOnNested(false)
				setCanDie(false)
				entity_heal(n, 100)
				
				cn = getNode("FIRSTVISIONEXIT")
				
				
				c = 0
				while not node_isEntityIn(cn, n) do 
					wait(FRAME_TIME)
					if entity_getHealth(n) <= 1 then
						--entity_heal(n, 1)
						c = c + 1
					end
					if c > 2 then
						break
					end
				end
				
				entity_heal(n, 100)
				
				setCanChangeForm(true)
				changeForm(FORM_NORMAL)
				unlearnSong(SONG_ENERGYFORM)
				
				setInvincibleOnNested(true)
				
				fade2(1, 0.5, 1, 1, 1)
				entity_setPosition(n, ox, oy)
				entity_rotate(n, 0)
				entity_flipToEntity(n, me)
				entity_idle(n)
				fade2(0, 0.5, 1, 1, 1)
				
				entity_heal(n, 100)
				
				setCanDie(true)
				
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
				voiceInterupt("Naija_Vision_MainArea")
				--entity_setPosition(me)
				entity_setInvincible(n, false)
				watch(1)
				setNaijaHeadTexture("")
				entity_setState(me, STATE_TRANS, 2)
			else
				lastNode = getNode(string.format("13_%d", curNode))
				entity_setPosition(me, node_x(lastNode), node_y(lastNode), -400, 0, 0, 0)
				--entity_say(me, "follow...")
			end
		end
		if not over then
			if math.abs(entity_x(me)-entity_x(n)) > 100 then
				entity_flipToEntity(me, n)
			end
		end
	end
	entity_setSayPosition(me, entity_x(me), entity_y(me)-160)
end


function enterState(me)
	if entity_isState(me, STATE_HEADTOCAVE) then
		entity_animate(me, "trail")
		--esetv(me, EV_LOOKAT, 0)
		--debugLog("Setting HeadToCave")
		entity_setPosition(me, entity_getPosition(me))
		entity_clearVel(me)
		entity_swimToNode(me, entity_getNearestNode(me, "13_SONGCAVE"), SPEED_SLOW)
		entity_setCullRadius(me, 1024)
	end
end

function exitState(me)
	if entity_isState(me, STATE_TRANS) then
		entity_setState(me, STATE_HEADTOCAVE)
	end
end
