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

-- ================================================================================================
-- NEWT BLASTER
-- ================================================================================================

dofile("scripts/entities/entityinclude.lua")


-- ================================================================================================
-- L O C A L  V A R I A B L E S 
-- ================================================================================================

STATE_HIDE 		= 1000
STATE_MOVING	= 1001

hits = 0
delay = 1
moveDelay = 0
rangeNode = 0
target = 0
lastWeb = 0

fireDelayTime = 0.5
fireDelay = fireDelayTime
orient = ORIENT_UP

myWeb = 0
curPoint = -1
webPoint = 0

-- ================================================================================================
-- FUNCTIONS
-- ================================================================================================


function init(me)
	setupBasicEntity(
	me,
	"",								-- texture
	16,								-- health
	2,								-- manaballamount
	2,								-- exp
	10,								-- money
	40,								-- collideRadius (for hitting entities + spells)
	STATE_IDLE,						-- initState
	128,							-- sprite width	
	128,							-- sprite height
	1,								-- particle "explosion" type, 0 = none
	0,								-- 0/1 hit other entities off/on (uses collideRadius)
	4000,							-- updateCull -1: disabled, default: 4000
	-1
	)
	
	--entity_setDeathParticleEffect(me, "NewtExplode")
	entity_initSkeletal(me, "SpiderCrab")
	entity_setState(me, STATE_IDLE)
	
	entity_setCollideRadius(me, 64)
	
	entity_scale(me, 1.4, 1.4)
	--entity_setDropChance(me, 75)
	
	orient = math.random(4)	
	webPoint = entity_getBoneByName(me, "WebPoint")
	bone_alpha(webPoint, 0)
	
	entity_setDamageTarget(me, DT_ENEMY_WEB, false)
	entity_setCullRadius(me, 1024)
	
	entity_generateCollisionMask(me)
	
	entity_setDeathScene(me, true)
	
	entity_setEatType(me, EAT_NONE)
end

function postInit(me)
	rangeNode = entity_getNearestNode(me, "BOUND")
	myWeb = createWeb()
	curPoint = web_addPoint(myWeb)
end

function songNote(me, note)
end

block = 6
function isBlocked(x, y)	
	if rangeNode ~= 0 then
		return (isObstructedBlock(x, y, block)) or (not node_isPositionIn(rangeNode, x, y))
	end
	return (isObstructedBlock(x, y, block))
end

function update(me, dt)
	--[[
	if lastWeb ~= 0 then
		web_delete(lastWeb, 4)
		lastWeb = 0
	end
	]]--
	speed = 600
	if entity_isState(me, STATE_IDLE) then
		wx,wy = bone_getWorldPosition(webPoint)
		if myWeb~=0 and curPoint ~= -1 then					
			web_setPoint(myWeb, curPoint, wx, wy)
		end
		if delay > 0 then
			entity_rotate(me, entity_getRotation(me)+180*dt)
			delay = delay - dt
			if delay < 0 then
				delay = 0
				moveDelay = 3 + math.random(4)
				entity_animate(me, "move", -1)
				orient = math.random(4)
			end
		else
			moveDelay = moveDelay - dt
			if moveDelay < 0 then
				moveDelay = 0
				delay = math.random(50)/100.0 + 0.5
				entity_animate(me, "idle", -1)
				if myWeb~=0 then
					if curPoint > 8 then
						web_delete(myWeb, 2)
						myWeb = createWeb()
					end
					curPoint = web_addPoint(myWeb, wx, wy)
				else
					myWeb = createWeb()
				end
			else
				check = 128
				vx, vy = entity_getNormal(me)
				vx, vy = vector_setLength(vx, vy, 128)
				if not isBlocked(entity_x(me)+vx, entity_y(me)+vy) then
					vx, vy = vector_setLength(vx, vy, speed*dt)
					entity_setPosition(me, entity_x(me) + vx, entity_y(me) + vy)
				else
					moveDelay = 0
				end
			end
			--[[
			if orient == ORIENT_LEFT then			
				if not isBlocked(entity_x(me)-check, entity_y(me)) then
					entity_setPosition(me, entity_x(me)-speed*dt, entity_y(me))
				else
					orient = orient + 1
				end
			elseif orient == ORIENT_RIGHT then
				if not isBlocked(entity_x(me)+check, entity_y(me)) then
					entity_setPosition(me, entity_x(me)+speed*dt, entity_y(me))
				else
					orient = orient + 1
				end
			elseif orient == ORIENT_DOWN then
				if not isBlocked(entity_x(me), entity_y(me)+check) then
					entity_setPosition(me, entity_x(me), entity_y(me)+speed*dt)
				else
					orient = orient + 1
				end
			elseif orient == ORIENT_UP then
				if not isBlocked(entity_x(me), entity_y(me)-check) then
					entity_setPosition(me, entity_x(me), entity_y(me)-speed*dt)
				else
					orient = orient + 1
				end			
			else
				orient = 0
			end
			]]--
		end
	end
	
	entity_clearTargetPoints(me)
	if not entity_isState(me, STATE_DEATHSCENE) then
		entity_addTargetPoint(me, bone_getWorldPosition(webPoint))
	end
	entity_handleShotCollisionsSkeletal(me)
	
	entity_touchAvatarDamage(me, 64, 1, 500)	
end

function hitSurface(me)
end

function enterState(me)
	if entity_isState(me, STATE_IDLE) then
		entity_setEntityType(me, ET_ENEMY)
		entity_animate(me, "idle", LOOP_INF)
		
		entity_setColor(me, 1, 1, 1, 0.5)
		entity_alpha(me, 1, 0.5)
	elseif entity_isState(me, STATE_DEAD) then
		shakeCamera(5, 1.5)
		playSfx("RockHit-Big")	
		web_delete(myWeb, 3)
		myWeb = 0
	elseif entity_isState(me, STATE_DEATHSCENE) then
		cam_toEntity(me)
		ox = entity_x(me)
		oy = entity_y(me)

		--entity_rotate(me, 180, 1, 0, 0, 1)
		entity_setPosition(me, entity_x(me), entity_y(me)+1600, 1.5, 0, 0, 1)
		entity_setStateTime(me, 1.5)
		wait(1)
		if chance(100) then
			spawnIngredient("SpiderEgg", ox, oy)
		end
		cam_toEntity(getNaija())
	end
end

function exitState(me)
	if entity_isState(me, STATE_HIDE) or entity_isState(me, STATE_MOVING) then
		entity_setState(me, STATE_IDLE)
	end
end

function damage(me, attacker, bone, damageType, dmg)
	if damageType == DT_AVATAR_ENERGYBLAST or damageType == DT_AVATAR_SHOCK or damageType == DT_AVATAR_BITE then	
		nx, ny = entity_getNormal(me)
		cx, cy = getLastCollidePosition()
		dx = cx-entity_x(me)
		dy = cy-entity_y(me)
		dot = vector_dot(nx, ny, dx, dy)

		if dot < 0.9 then
			--[[		
			if damageType == DT_AVATAR_BITE then
				if myWeb ~= 0 then
					lastWeb = myWeb					
					myWeb = 0
				end
			end
			]]--
			return true
		end
	end
	
	return false
end

function dieNormal(me)
end
