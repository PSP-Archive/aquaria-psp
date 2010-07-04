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

delay = 0

STATE_ATTACK1 		= 1000
STATE_ATTACK2		= 1003
STATE_ATTACK2LOOP	= 1004


bone_head = 0
bone_spawn = 0
bone_hand = 0
bone_mask = 0

hits = 64 * 1.5

fireDelay = 0

takeDamage = true
suck = false

pat = 0

node_creatorcutscene = 0

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "CreatorForm1")
	--entity_setAllDamageTargets(me, false)
	
	entity_generateCollisionMask(me)
	
	entity_setState(me, STATE_WAITFORCUTSCENE)
	
	bone_head = entity_getBoneByName(me, "Head")
	bone_spawn = entity_getBoneByName(me, "Spawn")
	bone_hand = entity_getBoneByName(me, "Hand")
	bone_mask = entity_getBoneByName(me, "Mask")
	bone_alpha(bone_spawn, 0.01)
	
	entity_setCullRadius(me, 600)
	
	
	entity_setDamageTarget(me, DT_AVATAR_PET, false)
	
	loadSound("hellbeast-suck")
	loadSound("mia-appear")
	
	--entity_setHealth(me, 60)
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	
	node_creatorcutscene = getNode("CREATORCUTSCENE")
end

function getDelayTime()
	if hits > 48 then
		return 6
	elseif hits > 32 then
		return 3
	else
		return 0
	end
end

function update(me, dt)
	if not entity_isState(me, STATE_CUTSCENE) and not entity_isState(me, STATE_WAITFORCUTSCENE) then
		overrideZoom(0.5, 1)
	end
	
	if entity_isState(me, STATE_TRANSITION) and suck then
		bx, by = bone_getWorldPosition(bone_head)
		entity_pullEntities(me, bx, by, 1000, 1700, dt)
		entity_clearVel(getLi())
	end
	
	if entity_isState(me, STATE_WAITFORCUTSCENE) then
		if node_isEntityIn(node_creatorcutscene, n) then
			entity_setState(me, STATE_CUTSCENE)
		end
	end
	
	if not entity_isState(me, STATE_WAITFORCUTSCENE) then
		entity_handleShotCollisionsSkeletal(me)
		bone = entity_collideSkeletalVsCircle(me, n)
		--[[
		if avatar_isBursting() and bone ~= 0 and entity_setBoneLock(n, me, bone) then
		else
			if bone ~= 0 and entity_getBoneLockEntity(n) ~= me then
				entity_push(n, -500, 0, 1)
				entity_damage(n, me, 1)
			end
		end
		]]--
		if bone ~= 0 then
			entity_push(n, -500, 0, 1)
			entity_damage(n, me, 0.5)
		end
	end
	
	hx, hy = bone_getWorldPosition(bone_head)
	
	entity_setLookAtPoint(me, hx, hy)
	
	if entity_isState(me, STATE_IDLE) then
		delay = delay + dt
		if delay > getDelayTime() then
			if pat < 3 then
				entity_setState(me, STATE_ATTACK1)
			else
				entity_setState(me, STATE_ATTACK2)
				pat = -1
			end
		end
	end
	
	if entity_isState(me, STATE_ATTACK2LOOP) then
		fireDelay = fireDelay + dt
		if fireDelay > 0.05 then
			bx, by = bone_getWorldPosition(bone_spawn)
			s = createShot("CreatorForm1", me, n, bx, by)
			x = -1000
			y = math.random(2000)-1000
			if chance(10) then
				x = entity_x(n) - bx
				y = entity_y(n) - by
			end
			shot_setAimVector(s, x, y)
			fireDelay = 0
		end
	end
	
	entity_clearTargetPoints(me)
	
	if not entity_isState(me, STATE_WAITFORCUTSCENE) then
		entity_addTargetPoint(me, hx, hy)
	end
	
	--entity_addTargetPoint(me, bone_getWorldPosition(bone_hand))
end

function doIntroEnd(me)
	shakeCamera(2, 1)
	
	voiceInterupt("CreatorLast12")
	--Creator: Then you must die.
	watch(1)
	fade2(1,0,1,1,1)
	fade2(0,1,1,1,1)
	entity_animate(me, "all", 0, 1)
	watchForVoice()
	
	playMusic("Worship1")
	
	watch(1)
	
	cam_toEntity(n)
	
	setOverrideVoiceFader(-1)
	
	entity_setState(me, STATE_IDLE, -1, 1)
	entity_setDamageTarget(me, DT_AVATAR_LIZAP, true)
	
	setCutscene(0)
end

function qws(me, t)
	c = 0
	
	if t == -1 then
		--debugLog("WATCH FOR VOICE!!!")
		while isStreamingVoice() do
			watch(FRAME_TIME)
			-- old skip method
			--[[
			watch(FRAME_TIME, WATCH_QUIT)
			if isQuitFlag() then
				doIntroEnd(me)
				return true
			end
			]]--
		end
	else
		watch(t)
		--[[
		watch(t, WATCH_QUIT)
		if isQuitFlag() then
			doIntroEnd(me)
			return true
		end
		]]--
	end
	
	return false
end

intrans2 = false

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		delay = 0
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_ATTACK1) then
		entity_setStateTime(me, entity_animate(me, "spawn"))
	elseif entity_isState(me, STATE_ATTACK2) then
		voice("Laugh3")
		entity_setStateTime(me, entity_animate(me, "attack2"))
	elseif entity_isState(me, STATE_TRANSITION) then
	
		nd = getNode("fallback")
		
		if not entity_isfh(n) then
			entity_fh(n)
		end
		
		li = getLi()
		
		entity_setPosition(n, node_x(nd), node_y(nd), 1)
		entity_setPosition(li, node_x(nd), node_y(nd), 1)
		
		fadeOutMusic(6)
		--entity_setStateTime(me, entity_animate(me, "die"))
		node = getNode("CREATORFORM1")
		
		entity_animate(me, "die")
		entity_setAllDamageTargets(me, false)
		
		if not(getForm() == FORM_NORMAL) then
			changeForm(FORM_NORMAL)
		end
		
		entity_idle(n)
		disableInput()
		entity_setInvincible(n, true)
		cam_toEntity(me)
		
		entity_setStateTime(me, 5)
	elseif entity_isState(me, STATE_TRANSITION2) then
		if not intrans2 then
			intrans2 = true
			
			n = getNaija()
			
			nd = getNode("fallback")
			
			if not entity_isfh(n) then
				entity_fh(n)
			end
			
			playSfx("naijali1")
			
			li = getLi()
			
			entity_setPosition(n, node_x(nd), node_y(nd))
			entity_setPosition(li, node_x(nd), node_y(nd))
			
			cam_toEntity(li)
			
			bx, by = bone_getWorldPosition(bone_head)
			
			entity_setState(li, STATE_CLOSE, -1, 1)
			
			ent = getFirstEntity()
			while ent ~= 0 do
				if (entity_getEntityType(ent) == ET_ENEMY) and ent ~= me then
					entity_damage(ent, me, 9999, DT_AVATAR_ENERGYBLAST)
				end
				ent = getNextEntity()
			end
			
			entity_setPosition(li, bx, by, 1)
			entity_scale(li, 0.5, 0.5, 4)
			entity_rotate(li, -8)
			entity_rotate(li, 8, 3, 0, 0, 1)
			watch(1)
			
			playSfx("mia-appear")
			
			spawnParticleEffect("tinyblueexplode", entity_x(li), entity_y(li))
			
			entity_delete(li)
			setLi(0)
			
			cam_toEntity(0)
			watch(1)
			
			cam_toEntity(getNaija())
			watch(1)
			playSfx("naijalow1")
			
			setFlag(FLAG_LI, 200)
			intrans2 = false
		end
	elseif entity_isState(me, STATE_ATTACK2LOOP) then
		entity_animate(me, "attack2loop", -1)
		entity_setStateTime(me, 8)
		shakeCamera(4, 8)
	elseif entity_isState(me, STATE_WAITFORCUTSCENE) then
		entity_animate(me, "idle", -1)
		entity_setDamageTarget(me, DT_AVATAR_LIZAP, false)
	elseif entity_isState(me, STATE_CUTSCENE) then
		if isNested() then return end
		
		setCutscene(1,1)
		
		if not (getForm()==FORM_NORMAL) then
			changeForm(FORM_NORMAL)
		end
		
		entity_swimToNode(n, node_creatorcutscene)
		
		while entity_isFollowingPath(n) do
			watch(FRAME_TIME)
		end
		
		setOverrideVoiceFader(1)
		
		fadeOutMusic(2)
		
		--overrideZoom(0.5, 0)
		
		overrideZoom(0.7, 10)
		
		li = getLi()
		
		lucienVol = 0.7
		
		jennaVol = 0
		
		entity_flipToEntity(li, me)
		
		watch(1)
		
		voice("CreatorLast1")
		--He spoke, and his voice was forever burned into my memory
		--in perfect detail
		if qws(me, -1) then return end
		
		entity_flipToEntity(li, me)
		
		cam_toNode(getNode("CONFRONT"))
		
		voice ("CreatorLast2", lucienVol)
		--Creator: Your darkness is mine, little one. Your life is mine. I created all that you have witnessed, and though you have struggled and fought, it is all for nothing. 
		--This world is my canvas. I will paint upon it whatever I see fit, until I have completed my masterpiece.
		--You, little Naija, are flawed. But you come from one who is perfection. You will be my companion, for the rest of eternity. You will have the honor of witnessing my final masterpiece.
		
		
		entity_animate(me, "point", 0, 1)
		if qws(me, 7) then return end 
		entity_animate(me, "all", 0, 1)
		if qws(me, 5) then return end 
		entity_animate(me, "slam", 0, 1)
		shakeCamera(2, 2)
		if qws(me, 5) then return end 
		entity_animate(me, "all", 0, 1)
		
		if qws(me, 11) then return end
		--27 "you little naija are flawed"
		entity_animate(me, "point", 0, 1)
		
		if qws(me, 7) then return end
		--35 "perfection"
		entity_animate(me, "all", 0, 1)
		if qws(me, 3) then return end
		entity_animate(me, "point", 0, 1)
		
		--44 final masterpiece
		if qws(me, 7) then return end
		entity_animate(me, "all", 0, 1)
		
		if qws(me, -1) then return end
		
		
		voice("CreatorLast3")
		--Naija: But I am a conscious being, with my own hopes and desires. I do not belong to you!
		if qws(me, -1) then return end

		voice("CreatorLast4", lucienVol)
		
		entity_animate(me, "slam", 0, 1)
		shakeCamera(2, 2)
		if qws(me, 3) then return end
		
		entity_animate(me, "slam", 0, 1)
		shakeCamera(2, 2)
		
		if qws(me, 4) then return end
		entity_animate(me, "all", 0, 1)
		
		--Creator: Yet it all sprang from MY mind. MY efforts, ungrateful one! Bow before me and worship your Creator.
		if qws(me, -1) then return end
		
		voice("CreatorLast5")
		--Naija: No! I will not. I have lived free. I have explored beyond
		if qws(me, -1) then return end
		
		voice("CreatorLast6", jennaVol)
		--Creator: You have been allowed to play in my world. Now your turn is over. You will love me, and nothing else.
		entity_animate(me, "all", 0, 1)
		if qws(me, -1) then return end
		
		voice("CreatorLast7")
		--Naija: But have you ever truly loved? Has anyone ever willfully loved you?
		if qws(me, -1) then return end
		
		voice("CreatorLast8", lucienVol)
		--Creator: You love an aberration  a foul symbol of chaos!
		entity_animate(me, "slam", 0, 1)
		shakeCamera(2, 2)
		watch(2)
		
		entity_animate(me, "point", 0, 1)
		
		if qws(me, -1) then return end
		
		voice("CreatorLast9")
		--Naija: Why does such a small creature frighten you so? Perhaps you are not as powerful as you claim perhaps you can only control what lies in the waters.
		if qws(me, -1) then return end
		
		voice("CreatorLast10")
		--Creator: Fool! I am a GOD.
		entity_animate(me, "anger", -1, 1)
		shakeCamera(2, 3.5)
		watch(3.27)
		fade2(1,0,1,1,1)
		fade2(0,1,1,1,1)
		--entity_animate(me, "slam", 0, 1)
		shakeCamera(10, 2)
		if qws(me, -1) then return end
		
		entity_animate(me, "down", -1, 1)
	
		
		voice("CreatorLast11")
		--Naija: Perhaps you dream to be but I will never worship you.
		if qws(me, -1) then return end
		
		watch(1)
		
		doIntroEnd(me)
	end
end


function exitState(me)
	if entity_isState(me, STATE_ATTACK1) then
		pat = pat + 1
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_ATTACK2) then
		entity_setState(me, STATE_ATTACK2LOOP)
	elseif entity_isState(me, STATE_ATTACK2LOOP) then
		pat = pat + 1
		entity_setState(me, STATE_IDLE)
	elseif entity_isState(me, STATE_TRANSITION) then
		entity_setState(me, STATE_TRANSITION2, 3)
	elseif entity_isState(me, STATE_TRANSITION2) then
		ent = createEntity("CreatorForm2", "", entity_getPosition(me))
		entity_alpha(me, 0, 0.5)
		entity_alpha(ent, 0)
		entity_alpha(ent, 1, 0.5)
		entity_setState(me, STATE_WAIT, 2)
	elseif entity_isState(me, STATE_WAIT) then
		enableInput()
		entity_setInvincible(n, false)
		cam_toEntity(n)
		entity_delete(me)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if entity_isState(me, STATE_TRANSITION) or takeDamage == false or entity_isState(me, STATE_TRANSITION2) or entity_isState(me, STATE_WAIT) then
		return false
	end
	if entity_isState(me, STATE_CUTSCENE) or entity_isState(me, STATE_WAITFORCUTSCENE) then
		return false
	end
	if damageType == DT_AVATAR_DUALFORMNAIJA then
		hits = hits - dmg
		bone_damageFlash(bone_head)
		bone_damageFlash(bone_mask)
		return false
	end
	if bone == bone_mask or bone == bone_head then
		hits = hits - dmg
		bone_damageFlash(bone)
		--return true
		if hits <= 0 then
			entity_setState(me, STATE_TRANSITION)
		end
	end
	return false
end

function animationKey(me, key)
	if entity_isState(me, STATE_ATTACK1) and key == 3 then
		sx, sy = bone_getWorldPosition(bone_spawn)
		--[[
		r = math.random(6)
		if r == 1 then
			ent = createEntity("Mutilus", "", sx, sy)
		elseif r == 2 then
			for i=1,3,1 do
				ent = createEntity("EvilJelly", "", sx, sy)
			end
		elseif r == 3 then
			ent = createEntity("AbyssOctopus", "", sx, sy)
		elseif r == 4 then
			ent = createEntity("AnglerFish", "", sx, sy)
		elseif r == 5 then
			ent = createEntity("SpiderCrab", "", sx, sy)
		else
			for i=1,3,1 do
				ent = createEntity("Moneye", "", sx, sy)
			end
		end
		]]--
		
		r = math.random(2)
		if r == 1 then
			ent = createEntity("Mutilus", "", sx, sy)
		elseif r == 2 then
			for i=1,3,1 do
				ent = createEntity("EvilJelly", "", sx, sy)
			end
		end
		
		sx, sy = entity_getScale(ent)
		entity_scale(ent, 0, 0)
		entity_scale(ent, sx, sy, 0.5)
	elseif entity_isState(me, STATE_TRANSITION) and key == 7 then
		suck = true
		playSfx("hellbeast-suck")
		nd = getNode("fallback")
		entity_setPosition(n, node_x(nd), node_y(nd), 1)
	end
end

function hitSurface(me)
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

