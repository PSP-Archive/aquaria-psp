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

STATE_MOVING 	= 1001

n = 0
eyes = {}
eyeHits = {}
beams = {}
nut = 0
node = 0
beamTimer = 0
addBeamIdx = -1
addBeamDelay = 0
vdir = 0
hdir = -1
hdirTimer = 6
movingSpawnTimer = 3
movingBeamTimer = 8

soundDelay = 0

rotSpeed = 40

function clearBarriers()
	if node ~= 0 then
		-- do magic
		node_setElementsInLayerActive(node, 2, false)
		reconstructGrid()
	end
end

function init(me)
	setupEntity(me)
	entity_setEntityType(me, ET_ENEMY)
	entity_initSkeletal(me, "RotCore")	
	--entity_setAllDamageTargets(me, false)
	entity_generateCollisionMask(me)
	entity_setHealth(me, 34) -- 26
	
	for i=1,6 do
		eyes[i] = entity_getBoneByIdx(me, i)
		bone_rotate(eyes[i], bone_getRotation(eyes[o])+180, 2)
		eyeHits[i] = 0
		beams[i] = 0
	end
	
	entity_setState(me, STATE_IDLE)
	entity_setCullRadius(me, 1024)
	
	nut = entity_getBoneByName(me, "Nut")
	
	--entity_rotate(me, 360, 3)
	entity_offset(me, 0, 16, 3,-1,1,1)
	
	bone_setSegs(entity_getBoneByName(me, "Body"), 2, 8, 0.3, 0.3, -0.018, 0, 6, 1)
	bone_setSegs(entity_getBoneByName(me, "Tentacles"), 2, 8, 0.3, 0.3, -0.018, 0, 6, 1)
	entity_setDeathScene(me, true)
	entity_setBounce(me, 1)
	entity_setBounceType(me, BOUNCE_REAL)
	entity_setCollideRadius(me, 100)
	entity_setUpdateCull(me, 3000)
	entity_setDropChance(me, 100, 3)
	
	--entity_setDamageTarget(me, DT_AVATAR_PET, false)
	
	loadSound("rotcore-beam")
	loadSound("rotcore-idle")
	loadSound("rotcore-die")
	loadSound("rotcore-birth")
	loadSound("rotcore-hurt")
	loadSound("rotcore-die2")
end

function clearBeams()
	for i=1,6 do
		if beams[i]~=0 then
			beam_delete(beams[i])
			beams[i] = 0
		end
		eyeHits[i] = 0
		bone_rotateOffset(eyes[i], 0, 0.5, 0, 0, 1)
	end
	addBeamIdx = -1
end

function postInit(me)
	n = getNaija()
	entity_setTarget(me, n)
	node = entity_getNearestNode(me, "CORERANGE")
	door = getEntity("CathedralDoor")
	if entity_isFlag(me, 1) then
		clearBarriers()
		entity_delete(me)
		
		if door ~= 0 then
			entity_msg(door, "DoorDownPre")
		end		
	end
end

function cueAddEyeBeam(me, idx)
	eyeHits[idx] = 1000
	if beams[idx] == 0 then
		bone_rotateOffset(eyes[idx], 180, 0.5, 0, 0, 1)				
		if addBeamIdx ~= -1 and addBeamIdx ~= idx then
			addEyeBeam(me, addBeamIdx)
		end
		addBeamIdx = idx
		addBeamDelay = 0.5
		beamTimer = 6
	end
end

function addEyeBeam(me, idx)
	if beams[idx]==0 then
		bx, by = bone_getWorldPosition(eyes[idx])
		beams[idx] = createBeam(bx, by, bone_getWorldRotation(eyes[idx])-90)
		beam_setTexture(beams[idx], "particles/Beam")
		
		entity_sound(me, "rotcore-beam")
		
		c = 0
		for i=1,6 do 
			if beams[i] ~= 0 then
				c = c + 1
			end
		end
		if c >= 6 then
			beamTimer = 0
			entity_setState(me, STATE_OPEN)
		end
	end
end

seen = false

function update(me, dt)
	if addBeamDelay > 0 then
		addBeamDelay = addBeamDelay - dt
		if addBeamDelay < 0 then
			addBeamDelay = 0			
			addEyeBeam(me, addBeamIdx)
			addBeamIdx = -1
		end
	end
	
	soundDelay = soundDelay - dt
	if soundDelay <= 0 then
		soundDelay = 0.6 + math.random(2) * 0.6
		entity_sound(me, "rotcore-idle")
	end
	
	if entity_isEntityInRange(me, n, 900) then
		overrideZoom(0.5, 0.2)
	else
		overrideZoom(0)
	end
	
	if not seen and entity_isEntityInRange(me, n, 800) then
		emote(EMOTE_NAIJAUGH)
		seen = true
	end

	entity_handleShotCollisionsSkeletal(me)
	bone = entity_collideSkeletalVsCircle(me, n)
	if bone ~= 0 then
		nx,ny = entity_getPosition(n)
		cx,cy = entity_getPosition(me)
		x = nx-cx
		y = 0
		x,y = vector_setLength(x,y,2000)
		entity_addVel(n, x, y)
		entity_damage(n, me, 0.5)
	end
	if entity_isState(me, STATE_IDLE) then
		if beamTimer > 0 then
			beamTimer = beamTimer - dt
			if beamTimer <= 0 then
				clearBeams()
			end
		end
		if entity_isEntityInRange(me, n, 1000) then
			entity_setMaxSpeedLerp(me, 0.15, 0.1)
			entity_moveTowardsTarget(me, dt, 400)
		end
	end
	if entity_isState(me, STATE_MOVING) then
		hdirTimer = hdirTimer - dt
		if hdirTimer < 0 then
			hdirTimer = math.random(2)+4
			if hdir == -1 then
				hdir = 1
			else
				hdir = -1
			end
		end
		entity_setMaxSpeedLerp(me, 1, 0.1)
		entity_rotate(me, entity_getRotation(me)+dt*rotSpeed*hdir)
		if vdir == 0 then
			entity_addVel(me, 0, -500*dt)
		else
			entity_addVel(me, 0, 500*dt)
		end
		entity_addVel(me, hdir*100*dt, 0)
		movingSpawnTimer = movingSpawnTimer - dt
		if movingSpawnTimer < 0 then
			bx,by = bone_getWorldPosition(nut)
			for i=1,2 do
				createEntity("RotBaby-Form1", "", bx, by+i*10)
				playSfx("rotcore-birth")
			end	
			movingSpawnTimer = math.random(4) + 8
		end
		movingBeamTimer = movingBeamTimer - dt
		if movingBeamTimer < -100 then
			if movingBeamTimer <= -100 - 4 then
				clearBeams()
				movingBeamTimer = math.random(2) + 4
			end
		elseif movingBeamTimer < 0 then
			clearBeams()
			cueAddEyeBeam(me, math.random(6))
			movingBeamTimer = -100			
		end		
	end
	entity_updateMovement(me, dt)
	for i=1,6 do
		if beams[i]~=0 then
			bone = eyes[i]
			beam_setAngle(beams[i], bone_getWorldRotation(bone)+90)
			beam_setPosition(beams[i], bone_getWorldPosition(bone))
		end
	end
	entity_clearTargetPoints(me)
	if not entity_isState(me, STATE_OPEN) and not entity_isState(me, STATE_MOVING) then
		for i=1,6 do
			entity_addTargetPoint(me, bone_getWorldPosition(eyes[i]))
		end
	end
	if entity_isState(me, STATE_OPEN) or entity_isState(me, STATE_MOVING) then
		entity_addTargetPoint(me, bone_getWorldPosition(nut))
	end
end

incut = false

function enterState(me)
	if incut then return end
	
	if entity_isState(me, STATE_IDLE) then
		entity_animate(me, "idle", -1)
	elseif entity_isState(me, STATE_OPEN) then
		entity_animate(me, "showNut")
		entity_setStateTime(me, 8)
		entity_clearVel(me)
	elseif entity_isState(me, STATE_CLOSE) then
		clearBeams()
		entity_setStateTime(me, entity_animate(me, "hideNut"))
	elseif entity_isState(me, STATE_DEATHSCENE) then
		incut = true
		clearBeams()
		entity_setStateTime(me, 99)
		
		entity_idle(getNaija())
		
		
		
		entity_offset(me, -10, 0)
		entity_offset(me, 10, 0, 0.03, -1, 1)
		
		entity_scale(me, 1.5, 1.5, 4)
		
		cam_toEntity(me)
		shakeCamera(3, 5)
		
		watch(1)
		playSfx("rotcore-die2")
		watch(2)
		
		playSfx("rotcore-die")
		
		fade2(1, 0.2, 1, 1, 1)
		watch(0.2)
		
		entity_heal(n, 1)
		
		spawnParticleEffect("rotcore-die", entity_x(me), entity_y(me))
		
		entity_scale(me, 3, 3, 0.5)
		entity_alpha(me, 0, 0.5)
		
		fade2(0, 1, 1, 1, 1)
		watch(2)
		
		entity_setStateTime(me, 0.01)
		incut = false
	elseif entity_isState(me, STATE_MOVING) then
		clearBeams()
	elseif entity_isState(me, STATE_DEAD) then
		clearBarriers()
		if door ~= 0 then
			entity_msg(door, "DoorDown")
		end
		overrideZoom(0)
		entity_setFlag(me, 1)
	end
end

function exitState(me)
	if entity_isState(me, STATE_OPEN) then
		entity_setState(me, STATE_MOVING)
	elseif entity_isState(me, STATE_CLOSE) then
		bx,by = bone_getWorldPosition(nut)
		for i=1,6 do
			createEntity("RotBaby-Form1", "", bx, by+i*10)
		end
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if bone_isName(bone,"Eye") or bone_isName(bone, "Nut") then
		bone_damageFlash(bone)
	end
	if bone == nut then
		playSfx("rotcore-hurt")
		return true
	end
	if not entity_isState(me, STATE_OPEN) and not entity_isState(me, STATE_MOVING) then
		idx = bone_getidx(bone)
		if idx >= 1 and idx <= 6 then
			-- is an eye!
			eyeHits[idx] = eyeHits[idx] + dmg
			if eyeHits[idx] >= 1 and eyeHits[idx] < 1000 then
				cueAddEyeBeam(me, idx)
			end
		end
	end
	return false
end

function animationKey(me, key)
end

function hitSurface(me)
	if entity_isState(me, STATE_MOVING) then
		nx, ny = getWallNormal(getLastCollidePosition())
		if math.abs(ny) > 0.5 then
			if vdir==0 then
				vdir = 1
			else
				vdir = 0
			end
		end
	end
end

function songNote(me, note)
end

function songNoteDone(me, note)
end

function song(me, song)
end

function activate(me)
end

