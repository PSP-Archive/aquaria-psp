/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "GridRender.h"
#include "Game.h"
#include "Avatar.h"

namespace MiniMapRenderSpace
{
	typedef std::vector<Quad*> Buttons;
	Buttons buttons;

	const int BUTTON_RADIUS = 15;

	// View area radius in virtual pixels
	const int miniMapRadius = 80;
	// Minimap scale (actual distance / displayed distance)
	const float miniMapScale = 40;
	// View area radius in world tiles
	const float miniMapTileRadius = miniMapRadius * miniMapScale / TILE_SIZE;
	// 1/2 size (width/height) of minimap GUI
	const float miniMapGuiSize = miniMapRadius * 1.5f;
	// Base radius of texture (texWaterBit) used to indicate open areas
	const float waterBitSize = 10;
	// Distance in tiles between adjacent water bits
	const int tileStep = 12;
	// Radius of the health bar circle
	const int healthBarRadius = miniMapRadius + 4;
	// Number of steps around health bar at which to draw bits
	const int healthSteps = 64;
	// 1/2 size (width/height) used for drawing health bar bits
	const int healthBitSizeLarge = 32;
	const int healthBitSizeSmall = 10;
	// 1/2 size (width/height) used for drawing the maximum health marker
	const int healthMarkerSize = 20;


	Texture *texCook = 0;
	Texture *texWaterBit = 0;
	Texture *texMinimapBtm = 0;
	Texture *texMinimapTop = 0;
	Texture *texRipple = 0;
	Texture *texNaija = 0;
	Texture *texHealthBar = 0;
	Texture *texMarker = 0;

	float waterSin = 0;

	int jumpOff = 0;
	float jumpTimer = 0.5;
	const float jumpTime = 1.5;
	float incr = 0;
}

using namespace MiniMapRenderSpace;

MiniMapRender::MiniMapRender() : RenderObject()
{
	toggleOn = 1;

	radarHide = false;

	doubleClickDelay = 0;
	mouseDown = false;
	_isCursorIn = false;
	lastCursorIn = false;
	followCamera = 1;
	doRender = true;
	float shade = 0.75;
	color = Vector(shade, shade, shade);
	cull = false;
	lightLevel = 1.0;

	texCook				= core->addTexture("GUI/ICON-FOOD");
	texWaterBit			= core->addTexture("GUI/MINIMAP/WATERBIT");
	texMinimapBtm		= core->addTexture("GUI/MINIMAP/BTM");
	texMinimapTop		= core->addTexture("GUI/MINIMAP/TOP");
	texRipple			= core->addTexture("GUI/MINIMAP/RIPPLE");
	texNaija			= core->addTexture("GEMS/NAIJA-TOKEN");
	texHealthBar		= core->addTexture("PARTICLES/glow-masked"); 
	texMarker			= core->addTexture("gui/minimap/marker");

	buttons.clear();

	Quad *q = 0;

	q = new Quad();
	q->setTexture("gui/open-menu");
	q->scale = Vector(1.5, 1.5);
	buttons.push_back(q);

	q->position = Vector(miniMapRadius, miniMapRadius);

	addChild(q, PM_POINTER, RBP_OFF);
}

void MiniMapRender::destroy()
{
	RenderObject::destroy();

	UNREFTEX(texCook);
	UNREFTEX(texWaterBit);
	UNREFTEX(texMinimapBtm);
	UNREFTEX(texMinimapTop);
	UNREFTEX(texRipple);
	UNREFTEX(texNaija);
	UNREFTEX(texHealthBar);
	UNREFTEX(texMarker);
}

bool MiniMapRender::isCursorIn()
{
	return _isCursorIn || lastCursorIn;
}

void MiniMapRender::slide(int slide)
{
	switch(slide)
	{
	case 0:
		dsq->game->miniMapRender->offset.interpolateTo(Vector(0, 0), 0.28, 0, 0, 1);
	break;
	case 1:
		dsq->game->miniMapRender->offset.interpolateTo(Vector(0, -470), 0.28, 0, 0, 1);
	break;
	}
}



bool MiniMapRender::isCursorInButtons()
{
	for (Buttons::iterator i = buttons.begin(); i != buttons.end(); i++)
	{
		if ((core->mouse.position - (*i)->getWorldPosition()).isLength2DIn(BUTTON_RADIUS))
		{
			return true;
		}
	}

	return ((core->mouse.position - position).isLength2DIn(50));
}

void MiniMapRender::clickEffect(int type)
{
	dsq->clickRingEffect(getWorldPosition(), type);
}

void MiniMapRender::toggle(int t)
{
	for (int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->renderQuad = (bool)t;
	}
	toggleOn = t;
}

void MiniMapRender::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);	
	position.z = 2.9;

	waterSin += dt;

	if (doubleClickDelay > 0)
	{
		doubleClickDelay -= dt;
	}

	position.x = core->getVirtualWidth() - 55 - core->getVirtualOffX();

	radarHide = false;

	if (dsq->darkLayer.isUsed() && dsq->game->avatar)
	{
		Path *p = dsq->game->getNearestPath(dsq->game->avatar->position, PATH_RADARHIDE);

		float t=dt*2;
		if ((dsq->continuity.form != FORM_SUN && dsq->game->avatar->isInDarkness())
			|| (p && p->isCoordinateInside(dsq->game->avatar->position)))
		{
			radarHide = true;
			lightLevel -= t;
			if (lightLevel < 0)
				lightLevel = 0;
		}
		else
		{
			lightLevel += t;
			if (lightLevel > 1)
				lightLevel = 1;
		}

	}
	else
	{
		lightLevel = 1;
	}

	if (dsq->game->avatar && dsq->game->avatar->isInputEnabled())
	{
		float v = dsq->game->avatar->health/5.0f;
		if (v < 0)
			v = 0;
		if (!lerp.isInterpolating())
			lerp.interpolateTo(v, 0.1);
		lerp.update(dt);


		jumpTimer += dt*0.5f;
		if (jumpTimer > jumpTime)
		{
			jumpTimer = 0.5;
		}
		incr += dt*2;
		if (incr > PI)
			incr -= PI;
	}

	_isCursorIn = false;
	if (alpha.x == 1)
	{		
		if (!dsq->game->isInGameMenu() && (!dsq->game->isPaused() || (dsq->game->isPaused() && dsq->game->worldMapRender->isOn())))
		{
			if (isCursorInButtons())
			{
				if (!core->mouse.buttons.left || mouseDown)
					_isCursorIn = true;
			}

			if (_isCursorIn || lastCursorIn)
			{

				if (core->mouse.buttons.left && !mouseDown)
				{
					mouseDown = true;
				}
				else if (!core->mouse.buttons.left && mouseDown)
				{
					mouseDown = false;

					bool btn=false;

					if (!dsq->game->worldMapRender->isOn())
					{
						for (int i = 0; i < buttons.size(); i++)
						{
							if ((buttons[i]->getWorldPosition() - core->mouse.position).isLength2DIn(BUTTON_RADIUS))
							{
								switch(i)
								{
								case 0:
								{
									doubleClickDelay = 0;
									dsq->game->showInGameMenu();
									btn = true;
								}
								break;
								}
							}
							if (btn) break;
						}
					}

					if (!btn && !dsq->mod.isActive() && !radarHide)
					{
						if (dsq->game->worldMapRender->isOn())
						{
							dsq->game->worldMapRender->toggle(false);
							clickEffect(1);
						}
						else
						{
							if (doubleClickDelay > 0)
							{
								
								if (dsq->continuity.gems.empty())
									dsq->continuity.pickupGem("Naija-Token");

								dsq->game->worldMapRender->toggle(true);

								clickEffect(0);

								doubleClickDelay = 0;
							}
							else
							{
								doubleClickDelay = DOUBLE_CLICK_DELAY;

								clickEffect(0);
							}
						}
					}
				}

				if (isCursorInButtons())
				{
					if (mouseDown)
					{
						_isCursorIn = true;
					}
				}
			}
			else
			{
				mouseDown = false;
			}
			lastCursorIn = _isCursorIn;
		}
	}
}

void MiniMapRender::onRender()
{
	if (!toggleOn)
		return;

	if (!dsq->game->avatar || dsq->game->avatar->getState() == Entity::STATE_TITLE || (dsq->disableMiniMapOnNoInput && !dsq->game->avatar->isInputEnabled()))
	{
		for (int i = 0; i < buttons.size(); i++)
			buttons[i]->renderQuad = false;
		return;
	}
	else
	{
		for (int i = 0; i < buttons.size(); i++)
			buttons[i]->renderQuad = true;
	}


#ifdef BBGE_BUILD_OPENGL

	glBindTexture(GL_TEXTURE_2D, 0);
	RenderObject::lastTextureApplied = 0;
	const float alphaValue = alpha.x;

	const TileVector centerTile(dsq->game->avatar->position);

	glLineWidth(1);
	
	if (alphaValue > 0)
	{
		texMinimapBtm->apply();

		glBegin(GL_QUADS);
			glColor4f(lightLevel, lightLevel, lightLevel, 1);
			glTexCoord2f(0, 1);
			glVertex2f(-miniMapGuiSize, miniMapGuiSize);
			glTexCoord2f(1, 1);
			glVertex2f(miniMapGuiSize, miniMapGuiSize);
			glTexCoord2f(1, 0);
			glVertex2f(miniMapGuiSize, -miniMapGuiSize);
			glTexCoord2f(0, 0);
			glVertex2f(-miniMapGuiSize, -miniMapGuiSize);
		glEnd();

		texMinimapBtm->unbind();


		if (lightLevel > 0)
		{
			texWaterBit->apply();

			glBlendFunc(GL_SRC_ALPHA,GL_ONE);
			glColor4f(0.1, 0.2, 0.9, 0.4f*lightLevel);
			bool curColorIsWater = true;

			const int xmin = int(ceilf(dsq->game->cameraMin.x / TILE_SIZE));
			const int ymin = int(ceilf(dsq->game->cameraMin.y / TILE_SIZE));
			const int xmax = int(floorf(dsq->game->cameraMax.x / TILE_SIZE));
			const int ymax = int(floorf(dsq->game->cameraMax.y / TILE_SIZE));

			int y1 = centerTile.y - miniMapTileRadius;
			int y2 = centerTile.y + miniMapTileRadius;
			// Round all coordinates to a multiple of tileStep, so
			// the minimap doesn't change as you scroll.
			y1 = (y1 / tileStep) * tileStep;
			y2 = ((y2 + tileStep-1) / tileStep) * tileStep;
			for (int y = y1; y <= y2; y += tileStep)
			{
				if (y < ymin) continue;
				if (y > ymax) break;

				const int dy = y - centerTile.y;
				const float widthFrac = cosf(float(dy) / miniMapTileRadius * (PI/2));
				const int halfTileWidth = int(ceilf(miniMapTileRadius * widthFrac));
				int x1 = centerTile.x - halfTileWidth;
				int x2 = centerTile.x + halfTileWidth;
				x1 = (x1 / tileStep) * tileStep;
				x2 = ((x2 + tileStep-1) / tileStep) * tileStep;
				for (int x = x1; x <= x2; x += tileStep)
				{
					if (x < xmin) continue;
					if (x > xmax) break;

					TileVector tile(x, y);
					if (!dsq->game->getGrid(tile))
					{
						const Vector tilePos(tile.worldVector());
						if (tilePos.y < dsq->game->waterLevel.x)
						{
							if (curColorIsWater)
							{
								glColor4f(0.1, 0.2, 0.5, 0.2f*lightLevel);
								curColorIsWater = false;
							}
						}
						else
						{
							if (curColorIsWater)
							{
								glColor4f(0.1, 0.2, 0.9, 0.4f*lightLevel);
								curColorIsWater = true;
							}
						}

						const Vector miniMapPos = Vector(tilePos - dsq->game->avatar->position) * (1.0f / miniMapScale);

						glTranslatef(miniMapPos.x, miniMapPos.y, 0);

						const float v = sinf(waterSin +  (tilePos.x + tilePos.y*miniMapTileRadius)*0.001f + sqr(tilePos.x+tilePos.y)*0.00001f);
						const int bitSize = (1+fabsf(v)) * waterBitSize;

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-bitSize, bitSize);
							glTexCoord2f(1, 1);
							glVertex2f(bitSize, bitSize);
							glTexCoord2f(1, 0);
							glVertex2f(bitSize, -bitSize);
							glTexCoord2f(0, 0);
							glVertex2f(-bitSize, -bitSize);
						glEnd();

						glTranslatef(-miniMapPos.x, -miniMapPos.y, 0);
					}
				}
			}
			texWaterBit->unbind();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, 0);

		}
	}

	if (!radarHide)
	{
		const int iconSize = sinf(game->getTimer()*PI)*6 + 14;
		texRipple->apply();
		for (int i = 0; i < dsq->game->paths.size(); i++)
		{
			Path *p = dsq->game->paths[i];
			if (!p->nodes.empty() && (p->pathType==PATH_COOK || p->pathType==PATH_SAVEPOINT || p->pathType==PATH_WARP))
			{
				bool render = true;
				Path *p2 = dsq->game->getNearestPath(p->nodes[0].position, PATH_RADARHIDE);
				if (p2 && p2->isCoordinateInside(p->nodes[0].position))
				{
					if (!p2->isCoordinateInside(dsq->game->avatar->position))
					{
						render = false;
					}
				}

				if (render)
				{
					Vector pt(p->nodes[0].position);
					Vector d = pt - dsq->game->avatar->position;
					d.capLength2D(miniMapRadius * miniMapScale * (7.0f/8.0f));
					const Vector miniMapPos = Vector(d)*Vector(1.0f/miniMapScale, 1.0f/miniMapScale);

					switch(p->pathType)
					{
					case PATH_COOK:
					{
						glColor4f(1, 1, 1, 1);

						glTranslatef(miniMapPos.x, miniMapPos.y, 0);
						int sz = 16;

						texCook->apply();

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-sz, sz);
							glTexCoord2f(1, 1);
							glVertex2f(sz, sz);
							glTexCoord2f(1, 0);
							glVertex2f(sz, -sz);
							glTexCoord2f(0, 0);
							glVertex2f(-sz, -sz);
						glEnd();

						glTranslatef(-miniMapPos.x, -miniMapPos.y, 0);
						texRipple->apply();
						render = false;  // Skip common rendering code
					}
					break;
					case PATH_SAVEPOINT:
					{
						glColor4f(1.0, 0, 0, alphaValue*0.75f);
					}
					break;
					case PATH_WARP:
					{
						if (p->naijaHome)
						{
							glColor4f(1.0, 0.9, 0.2, alphaValue*0.75f);	
						}
						else
						{
							glColor4f(1.0, 1.0, 1.0, alphaValue*0.75f);
						}
					}
					break;
					}

					if (render)
					{
						glTranslatef(miniMapPos.x, miniMapPos.y, 0);
						int sz = iconSize;

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-sz, sz);
							glTexCoord2f(1, 1);
							glVertex2f(sz, sz);
							glTexCoord2f(1, 0);
							glVertex2f(sz, -sz);
							glTexCoord2f(0, 0);
							glVertex2f(-sz, -sz);
						glEnd();

						glTranslatef(-miniMapPos.x, -miniMapPos.y, 0);
					}
				}
			}
		}
		texRipple->unbind();
	}

	glColor4f(1,1,1, alphaValue);

	const int hsz = 20;
	texNaija->apply();

	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(-hsz, hsz);
		glTexCoord2f(1, 1);
		glVertex2f(hsz, hsz);
		glTexCoord2f(1, 0);
		glVertex2f(hsz, -hsz);
		glTexCoord2f(0, 0);
		glVertex2f(-hsz, -hsz);
	glEnd();

	texNaija->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	glColor4f(1,1,1,1);

	texMinimapTop->apply();
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(-miniMapGuiSize, miniMapGuiSize);
		glTexCoord2f(1, 1);
		glVertex2f(miniMapGuiSize, miniMapGuiSize);
		glTexCoord2f(1, 0);
		glVertex2f(miniMapGuiSize, -miniMapGuiSize);
		glTexCoord2f(0, 0);
		glVertex2f(-miniMapGuiSize, -miniMapGuiSize);
	glEnd();
	texMinimapTop->unbind();

	glBindTexture(GL_TEXTURE_2D, 0);


	glLineWidth(10 * (core->width / 1024.0f));

	const float healthStepSize = 2*PI / healthSteps;
	const float oangle = -PI;
	const float eangle = oangle + PI*lerp.x;
	const float eangle2 = oangle + PI*(dsq->game->avatar->maxHealth/5.0f);

	Vector healthBarColor;

	if (lerp.x >= 1)
	{
		healthBarColor = Vector(0,1,0.5);
	}
	else
	{
		healthBarColor = Vector(1-lerp.x, lerp.x*1, lerp.x*0.5f);
		healthBarColor.normalize2D();
	}

	texHealthBar->apply();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(healthBarColor.x, healthBarColor.y, healthBarColor.z, 0.6);

	float angle = oangle;

	glBegin(GL_QUADS);
	while (lerp.x != 0 && angle <= eangle)
	{
		float x = cosf(angle)*healthBarRadius+2;
		float y = -sinf(angle)*healthBarRadius;

		glTexCoord2f(0, 1);
		glVertex2f(x-healthBitSizeSmall, y+healthBitSizeSmall);
		glTexCoord2f(1, 1);
		glVertex2f(x+healthBitSizeSmall, y+healthBitSizeSmall);
		glTexCoord2f(1, 0);
		glVertex2f(x+healthBitSizeSmall, y-healthBitSizeSmall);
		glTexCoord2f(0, 0);
		glVertex2f(x-healthBitSizeSmall, y-healthBitSizeSmall);

		angle += healthStepSize;
	}
	glEnd();


	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	angle = oangle;
	int jump = 0;

	glBegin(GL_QUADS);
	while (lerp.x != 0 && angle <= eangle)
	{
		float x = cosf(angle)*healthBarRadius+2;
		float y = -sinf(angle)*healthBarRadius;

		if (jump == 0)
		{
			glColor4f(healthBarColor.x, healthBarColor.y, healthBarColor.z, fabsf(cosf(angle-incr))*0.3f + 0.2f);

			glTexCoord2f(0, 1);
			glVertex2f(x-healthBitSizeLarge, y+healthBitSizeLarge);
			glTexCoord2f(1, 1);
			glVertex2f(x+healthBitSizeLarge, y+healthBitSizeLarge);
			glTexCoord2f(1, 0);
			glVertex2f(x+healthBitSizeLarge, y-healthBitSizeLarge);
			glTexCoord2f(0, 0);
			glVertex2f(x-healthBitSizeLarge, y-healthBitSizeLarge);
		}

		jump++;
		if (jump > 3)
			jump = 0;

		angle += healthStepSize;
	}
	glEnd();

	texHealthBar->unbind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);

	texMarker->apply();

	float x = cosf(eangle2)*healthBarRadius+2;
	float y = -sinf(eangle2)*healthBarRadius;

	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(x-healthMarkerSize, y+healthMarkerSize);
		glTexCoord2f(1, 1);
		glVertex2f(x+healthMarkerSize, y+healthMarkerSize);
		glTexCoord2f(1, 0);
		glVertex2f(x+healthMarkerSize, y-healthMarkerSize);
		glTexCoord2f(0, 0);
		glVertex2f(x-healthMarkerSize, y-healthMarkerSize);
	glEnd();

	texMarker->unbind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(1,1,1,1);

	glBindTexture(GL_TEXTURE_2D, 0);

#endif
}

