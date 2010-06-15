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

a = 0
off = 0
glow = 0
flag = 0

sz = 0.7
nqtimer = 0
noteQuad = 0
delay = 0

boneGroup = {}

noteBone = 0
spinDir = 1

index = 0

function boneGroupAlpha(a, t)
	for i=1,14 do
		bone_alpha(boneGroup[i], a, t, 0, 0, 1)
	end
end

function commonInit(me, skel, num, f, r, g, b)
	setupEntity(me)
	entity_setEntityType(me, ET_NEUTRAL)
	entity_initSkeletal(me, skel)
	
	entity_setEntityLayer(me, 1)
	
	entity_setState(me, STATE_FIGURE)
	entity_scale(me, sz, sz)
	off = 6.28*(num*0.25)
	
	body = entity_getBoneByName(me, "Body")
		
	glow = entity_getBoneByName(me, "Glow")
	noteBone = entity_getBoneByName(me, "Note")
	
	bone_setVisible(glow, true)
	bone_setVisible(noteBone, true)
	
	bone_alpha(noteBone)
	bone_setBlendType(glow, BLEND_ADD)
	bone_scale(glow, 4, 4)
	bone_scale(glow, 8, 8, 0.5, -1, 1, 1)
	
	bone_color(glow, r*0.5 + 0.5, g*0.5 + 0.5, b*0.5 + 0.5)
	
	bone_setAnimated(noteBone, ANIM_POS)
	bone_setAnimated(glow, ANIM_POS)
	
	loadSound("Spirit-Awaken")
	loadSound("Spirit-Join")
	
	flag = f
	
	index = num
	--[[
	for i=0,13 do
		boneGroup[i+1] = entity_getBoneByIndex(me, i)
	end
	]]--
	
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
end

incut=false
function update(me, dt)
	
	if nqtimer > 0 then
		nqtimer = nqtimer - dt
		if nqtimer <= 0 then
		end
	end
	--entity_updateMovement(me, dt)
	if entity_isState(me, STATE_FIGURE) then
		if incut then return end
		if entity_isEntityInRange(me, n, 256) then
			incut = true
			entity_setInvincible(n, true)
			entity_idle(n)
			entity_flipToEntity(n, me)
			cam_toEntity(me)
			watch(2)
			
			if index == 0 then
				voice("Naija_SpiritKrotite")
			elseif index == 1 then
				voice("Naija_SpiritMithalas")
			elseif index == 2 then
				voice("Naija_SpiritDruniad")
			elseif index == 3 then
				voice("Naija_SpiritErulian")
			end
			watchForVoice()
			
			spawnParticleEffect("SpiritBeacon", entity_x(n), entity_y(n))
			playSfx("Spirit-Beacon")
			watch(0.4)
			
			spawnParticleEffect("SpiritBeacon", entity_x(me), entity_y(me))
			playSfx("Spirit-Beacon")
			watch(1)
			
			entity_alpha(me, 1, 2)
		
			watch(0.5)
			playSfx("Spirit-Awaken")
			watch(1.25)
			entity_rotate(me, 0, 1, 0, 0, 1)
			entity_setState(me, STATE_IDLE)
			watch(2)
			watch(1)
			bone_alpha(glow, 1, 1, 0, 0, 1)
			watch(1)
			playSfx("Spirit-Join")
			entity_setState(me, STATE_FOLLOW)
			setFlag(flag, 1)
			watch(2)
			cam_toEntity(n)
			if isFlag(FLAG_SPIRIT_DRASK, 1)
			and isFlag(FLAG_SPIRIT_ERULIAN, 1)
			and isFlag(FLAG_SPIRIT_KROTITE, 1)
			and isFlag(FLAG_SPIRIT_DRUNIAD, 1) then
				voice("Naija_FourSpirits")
				watch(2)
			end
			entity_setInvincible(n, false)
			incut=false
		end
	end
	if entity_isState(me, STATE_FOLLOW) then
		dist = 400
		t = 0
		x = 0
		y = 0
		if avatar_isRolling() then
			dist = 250
			spinDir = -avatar_getRollDirection()
			t = getTimer(6.28)*spinDir
		else
			t = getHalfTimer(6.28)*spinDir
		end
		
		if isForm(FORM_ENERGY) then
			dist = dist - 100
		end
		
		--[[
		if avatar_isBursting() then
			x = entity_velx(n)
			y = entity_vely(n)
			x, y = vector_setLength(x, y, 512)
		end
		]]--
		
		a = t + off
		x = x + math.sin(a)*dist
		y = y + math.cos(a)*dist
		entity_setPosition(me, entity_x(n)+x, entity_y(n)+y, 0.2)
		
		--[[
		delay = delay - dt
		if delay < 0 then
			s = createShot("FinalSpirit", me, 0, entity_x(me), entity_y(me))
			delay = 0.1
		end
		]]--
	end
	
	if noteQuad ~= 0 then
		quad_setPosition(noteQuad, entity_x(me), entity_y(me))
	end
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_FIGURE) then
		entity_scale(me, sz, sz, 1)
		esetv(me, EV_LOOKAT, 1)
		--boneGroupAlpha(1, 1)
		bone_alpha(glow, 0, 1, 0, 0, 1)
		entity_alpha(me, 0.2, 1, 0, 0, 1)
		entity_animate(me, "figure", -1)
	elseif entity_isState(me, STATE_FOLLOW) then
		entity_scale(me, 0.2, 0.2, 1)
		entity_animate(me, "ball", -1, 0, 3)
		esetv(me, EV_LOOKAT, 0)
		--boneGroupAlpha(0, 1)
		bone_alpha(glow, 1, 1, 0, 0, 1)
		entity_alpha(me, 1, 2, 0, 0, 1)
		bone_rotate(glow, 0, 1, 0, 0, 1)
		bone_rotate(glow, 360, 1, -1)
	end
	bone_rotate(boneNote, -entity_getRotation(me))
end

function exitState(me)
end

function damage(me, attacker, bone, damageType, dmg)
	return false
end

function animationKey(me, key)
end

function hitSurface(me)
end

function songNote(me, note)
	t = 1
	--noteQuad = createQuad(string.format("Song/NoteSymbol%d", note), 6)
	bone_setTexture(noteBone, string.format("Song/NoteSymbol%d", note))
	bone_alpha(noteBone, 0.8)
	bone_alpha(noteBone, 0, t)
	bone_scale(noteBone, 4, 4)
	bone_scale(noteBone, 8, 8, t, 0, 0, 1)
	bone_color(noteBone, getNoteColor(note))
	--quad_setPosition(noteQuad, entity_x(me), entity_y(me))
	--quad_delete(noteQuad, t)
	nqtimer = t
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

