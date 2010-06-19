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
#include "Quad.h"
#include "Core.h"

#include <assert.h>

std::vector<QuadLight> QuadLight::quadLights;

bool Quad::flipTY = true;

int Quad::_w2 = 0;
int Quad::_h2 = 0;

QuadLight::QuadLight(Vector position, Vector color, int dist)
{
	this->dist = dist;
	this->color = color;
	this->position = position;
}

void QuadLight::clearQuadLights()
{
	quadLights.clear();
}

void QuadLight::addQuadLight(const QuadLight &quadLight)
{
	quadLights.push_back(quadLight);
}

Vector Quad::renderBorderColor = Vector(1,1,1);

Quad::Quad(const std::string &tex, const Vector &pos)
: RenderObject()
{
	initQuad();
	position = pos;
	setTexture(tex);
}

/*
void Quad::initDefaultVBO()
{
}

void Quad::shutdownDefaultVBO()
{
}
*/

void Quad::setSegs(int x, int y, float dgox, float dgoy, float dgmx, float dgmy, float dgtm, bool dgo)
{
	deleteGrid();
	if (x == 0 || y == 0)
	{
		gridTimer = 0;
		xDivs = 0;
		yDivs = 0;
		doUpdateGrid = false;
	}
	else
	{
		this->drawGridOffsetX = dgox;
		this->drawGridOffsetY = dgoy;
		this->drawGridModX = dgmx;
		this->drawGridModY = dgmy;
		this->drawGridTimeMultiplier = dgtm;
		drawGridOut = dgo;
		xDivs = x;
		yDivs = y;

		createGrid(x, y);

		gridTimer = 0;
		
		doUpdateGrid = true;
	}
}

void Quad::createStrip(bool vert, int num)
{
	strip.resize(num);
	stripVert = vert;
	resetStrip();
}

void Quad::setStrip(const std::vector<Vector> &st)
{
	resetStrip();
	for (int i = 0; i < st.size(); i++)
	{
		if (i < strip.size())
		{
			strip[i].x += st[i].x;
			strip[i].y += st[i].y;
		}
	}
}

void Quad::createGrid(int xd, int yd)
{
	deleteGrid();
	
	xDivs = xd;
	yDivs = yd;
	
	drawGrid = new Vector * [xDivs];
	for (int i = 0; i < xDivs; i++)
	{
		drawGrid[i] = new Vector [yDivs];
		for (int j = 0; j < yDivs; j++)
		{
			drawGrid[i][j].z = 1;
		}
	}
	
	resetGrid();
}

void Quad::setDrawGridAlpha(int x, int y, float alpha)
{
	if (x < xDivs && x >= 0 && y < yDivs && y >= 0)
	{
		drawGrid[x][y].z = alpha;
	}
}

void Quad::setGridPoints(bool vert, const std::vector<Vector> &points)
{
	if (!drawGrid) return;
	resetGrid();
	for (int i = 0; i < points.size(); i++)
	{
		if (!vert) // horz
		{
			for (int y = 0; y < yDivs; y++)
			{
				for (int x = 0; x < xDivs; x++)
				{
					if (x < points.size())
					{
						drawGrid[x][y] += points[x];
					}
				}
			}
		}
		else
		{
			for (int x = 0; x < xDivs; x++)
			{
				for (int y = 0; y < yDivs; y++)
				{
					if (y < points.size())
					{
						drawGrid[x][y] += points[y];
					}
				}
			}
		}
	}
}

float Quad::getStripSegmentSize()
{
	return (1.0f/(float(strip.size())));
}

void Quad::resetStrip()
{
	if (!stripVert)
	{
		for (int i = 0; i < strip.size(); i++)
		{
			//float v = (i/(float)(strip.size()-1))-0.5f;
			float v = (i/(float(strip.size())));
			strip[i].x = v;
			strip[i].y = 0;
		}
	}
	else
	{
		errorLog("VERTICAL STRIP NOT SUPPORTED ^_-");
	}
}

void Quad::resetGrid()
{
	for (int i = 0; i < xDivs; i++)
	{
		for (int j = 0; j < yDivs; j++)
		{
			drawGrid[i][j].x = i/(float)(xDivs-1)-0.5f;
			drawGrid[i][j].y = j/(float)(yDivs-1)-0.5f;
		}
	}
}

void Quad::spawnChildClone(float t)
{
	if (!this->texture) return;
	Quad *q = new Quad;
	q->setTexture(this->texture->name);
	q->setLife(t+0.1f);
	q->setDecayRate(1);
	q->width = this->width;
	q->height = this->height;
	q->alpha = 1;
	q->alpha.interpolateTo(0, t);
	if (isfh())
		q->flipHorizontal();
	q->position = this->position;
	q->followCamera = this->followCamera;
	q->scale = this->scale;
	q->offset = this->offset;
	q->blendType = this->blendType;

	//q->parentManagedPointer = true;
	//q->renderBeforeParent = false;
	core->getTopStateData()->addRenderObject(q, this->layer);
	//addChild(q);
	int c = children.size();
}
/*
smoothly transition to texture
by creating a copy of the current quad on top and fading it out
*/
void Quad::setTextureSmooth(const std::string &texture, float t)
{
	if (this->texture && !this->texture->name.empty())
	{
		spawnChildClone(t);
		//core->getTopStateData()->addRenderObject(q, this->layer);
	}
	this->setTexture(texture);
}

void Quad::initQuad()
{
	repeatToFillScale = Vector(1,1);
	gridType = GRID_WAVY;
	gridTimer = 0;
	xDivs = 0;
	yDivs = 0;
	
	doUpdateGrid = false;

	autoWidth = autoHeight = 0;

	//debugLog("Quad::initQuad()");

	repeatingTextureToFill = false;
	_w2 = _h2 = 0;
	
	drawGrid = 0;

	lightingColor = Vector(1,1,1);
	quadLighting = false;

	renderBorder = false;
	renderCenter = true;
	width = 2; height = 2;
	//llalpha = Vector(1);
	//lralpha = Vector(1);
	//ulalpha = Vector(1);
	//uralpha = Vector(1);
	//oriented = false;
	upperLeftTextureCoordinates = Vector(0,0);
	lowerRightTextureCoordinates = Vector(1,1);
	renderQuad = true;
	//debugLog("End Quad::initQuad()");
}

Quad::Quad() : RenderObject()
{
	borderAlpha = 0.5;
	//debugLog("Quad::Quad()");
	initQuad();
	//debugLog("End Quad::Quad()");
	//textureSize = Vector(1,1);
}

void Quad::deleteGrid()
{
	if (drawGrid)
	{
		for (int i = 0; i < xDivs; i++)
		{
			delete[] drawGrid[i];
		}
		delete[] drawGrid;
		drawGrid = 0;
	}
}

void Quad::destroy()
{
	deleteGrid();
	RenderObject::destroy();
}

int Quad::getCullRadius()
{
	if (overrideCullRadius)
		return overrideCullRadius;
	int w = int(width*scale.x)+1;
	int h = int(height*scale.y)+1;
	return w + h;
}

bool Quad::isCoordinateInside(Vector coord, int minSize)
{
	int hw = fabsf((width)*getRealScale().x)*0.5f;
	int hh = fabsf((height)*getRealScale().y)*0.5f;
	if (hw < minSize)
		hw = minSize;
	if (hh < minSize)
		hh = minSize;

	if (coord.x >= getRealPosition().x - hw && coord.x <= getRealPosition().x + hw)
	{
		if (coord.y >= getRealPosition().y - hh && coord.y <= getRealPosition().y + hh)
		{
			return true;
		}
	}
	return false;
}

bool Quad::isCoordinateInsideWorld(const Vector &coord, int minSize)
{
	int hw = fabsf((width)*getRealScale().x)*0.5f;
	int hh = fabsf((height)*getRealScale().y)*0.5f;
	if (hw < minSize)
		hw = minSize;
	if (hh < minSize)
		hh = minSize;

	Vector pos = getWorldPosition();
	if (coord.x >= pos.x + offset.x - hw && coord.x <= pos.x + offset.x + hw)
	{
		if (coord.y >= pos.y + offset.y - hh && coord.y <= pos.y + offset.y + hh)
		{
			return true;
		}
	}
	return false;
}

bool Quad::isCoordinateInsideWorldRect(const Vector &coord, int w, int h)
{
	int hw = w*0.5f;
	int hh = h*0.5f;

	Vector pos = getWorldPosition();
	if (coord.x >= pos.x + offset.x - hw && coord.x <= pos.x + offset.x + hw)
	{
		if (coord.y >= pos.y + offset.y - hh && coord.y <= pos.y + offset.y + hh)
		{
			return true;
		}
	}
	return false;
}

void Quad::updateGrid(float dt)
{
	//if (xDivs == 0 && yDivs == 0) return;
	if (!doUpdateGrid) return;

	if (gridType == GRID_WAVY)
	{
		gridTimer += dt * drawGridTimeMultiplier;
		resetGrid();
		int hx = xDivs/2;
		for (int x = 0; x < xDivs; x++)
		{
			for (int y = 0; y < yDivs; y++)
			{
				float xoffset = y * drawGridOffsetX;
				float yoffset = x * drawGridOffsetY;
				if (drawGridModX != 0)
				{
					float add = (sinf(gridTimer+xoffset)*drawGridModX);
					if (drawGridOut && x < hx)
						drawGrid[x][y].x += add;
					else
						drawGrid[x][y].x -= add;
				}
				if (drawGridModY != 0)
					drawGrid[x][y].y += cosf(gridTimer+yoffset)*drawGridModY;
			}
		}
	}
}

void Quad::renderGrid()
{
#ifdef BBGE_BUILD_OPENGL
	float percentX, percentY;
	float baseX, baseY;
	{
		//if (this->texture)
		if (false)
		{
			//percentX = (this->getWidth()*scale.x)/this->texture->width;
			//percentY = (this->getHeight()*scale.y)/this->texture->height;

		}
		else
		{
			percentX = fabsf(this->lowerRightTextureCoordinates.x - this->upperLeftTextureCoordinates.x);
			percentY = fabsf(this->upperLeftTextureCoordinates.y - this->lowerRightTextureCoordinates.y);
#if defined(BBGE_BUILD_UNIX) || defined(BBGE_BUILD_PSP)
			if (lowerRightTextureCoordinates.x < upperLeftTextureCoordinates.x)
				baseX = lowerRightTextureCoordinates.x;
			else
				baseX = upperLeftTextureCoordinates.x;

			if (lowerRightTextureCoordinates.y < upperLeftTextureCoordinates.y)
				baseY = lowerRightTextureCoordinates.y;
			else
				baseY = upperLeftTextureCoordinates.y;
#else
			baseX = MIN(lowerRightTextureCoordinates.x, upperLeftTextureCoordinates.x);
			baseY = MIN(lowerRightTextureCoordinates.y, upperLeftTextureCoordinates.y);
#endif

		}
		//percentX = (float)screenWidth/(float)textureWidth;
		//percentY = (float)screenHeight/(float)textureHeight;
	}

	int w = this->getWidth();
	int h = this->getHeight();

	if (core->mode == Core::MODE_2D)
	{
		/*
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		*/
		glBegin(GL_QUADS);
		for (int i = 0; i < (xDivs-1); i++)
		{
			for (int j = 0; j < (yDivs-1); j++)
			{
				if (drawGrid[i][j].z != 0 || drawGrid[i][j+1].z != 0 || drawGrid[i+1][j] != 0 || drawGrid[i+1][j+1] != 0)
				{
					
					glColor4f(color.x, color.y, color.z, drawGrid[i][j].z*alpha.x*alphaMod);
					glTexCoord2f((i/(float)(xDivs-1)*percentX)+baseX,  1-((1*percentY-(j)/(float)(yDivs-1)*percentY))+baseY);//*upperLeftTextureCoordinates.y
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,i/(float)(xDivs-1)*percentX,  1*percentY-(j)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,0);
					glVertex2f(w*drawGrid[i][j].x,		h*drawGrid[i][j].y);
					//
					glColor4f(color.x, color.y, color.z, drawGrid[i][j+1].z*alpha.x*alphaMod);
					glTexCoord2f((i/(float)(xDivs-1)*percentX)+baseX, 1-((1*percentY-(j+1)/(float)(yDivs-1)*percentY))+baseY);//*lowerRightTextureCoordinates.y
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,i/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,0,(float)(screenHeight/(yDivs-1))/16);
					glVertex2f(w*drawGrid[i][j+1].x,		h*drawGrid[i][j+1].y);
					//
					glColor4f(color.x, color.y, color.z, drawGrid[i+1][j+1].z*alpha.x*alphaMod);
					glTexCoord2f(((i+1)/(float)(xDivs-1)*percentX)+baseX, 1-((1*percentY-(j+1)/(float)(yDivs-1)*percentY))+baseY);//*lowerRightTextureCoordinates.y
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,(i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j+1)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,(float)(screenHeight/(yDivs-1))/16);
					glVertex2f(w*drawGrid[i+1][j+1].x,	h*drawGrid[i+1][j+1].y);
					//
					glColor4f(color.x, color.y, color.z, drawGrid[i+1][j].z*alpha.x*alphaMod);
					glTexCoord2f(((i+1)/(float)(xDivs-1)*percentX)+baseX, 1-((1*percentY-(j)/(float)(yDivs-1)*percentY))+baseY);	//*upperLeftTextureCoordinates.y
						//glMultiTexCoord2fARB(GL_TEXTURE0_ARB,(i+1)/(float)(xDivs-1)*percentX, 1*percentY-(j)/(float)(yDivs-1)*percentY);
						//glMultiTexCoord2fARB(GL_TEXTURE1_ARB,(float)(screenWidth/(xDivs-1))/16,0);
					glVertex2f(w*drawGrid[i+1][j].x,		h*drawGrid[i+1][j].y);
				//glEnd();
				}
			}
		}
		glEnd();

		// debug points
		if (RenderObject::renderCollisionShape)
		{
			glBindTexture(GL_TEXTURE_2D, 0);
			glPointSize(2);
			glColor3f(1,0,0);
			glBegin(GL_POINTS);
				for (int i = 0; i < (xDivs-1); i++)
				{
					for (int j = 0; j < (yDivs-1); j++)
					{
						glVertex2f(w*drawGrid[i][j].x,		h*drawGrid[i][j].y);
						glVertex2f(w*drawGrid[i][j+1].x,		h*drawGrid[i][j+1].y);
						glVertex2f(w*drawGrid[i+1][j+1].x,	h*drawGrid[i+1][j+1].y);
						glVertex2f(w*drawGrid[i+1][j].x,		h*drawGrid[i+1][j].y);
					}
				}
			glEnd();
			if (texture)
				glBindTexture(GL_TEXTURE_2D, texture->textures[0]);
		}
	}
#endif
}

void Quad::renderSingle()
{
	// Get texture and vertex coordinates
	float s0 = upperLeftTextureCoordinates.x;
	float s1 = lowerRightTextureCoordinates.x;
	float t0, t1;
	if (Quad::flipTY)
	{
		t0 = 1 - upperLeftTextureCoordinates.y;
		t1 = 1 - lowerRightTextureCoordinates.y;
	}
	else
	{
		t0 = upperLeftTextureCoordinates.y;
		t1 = lowerRightTextureCoordinates.y;
	}
	float x0 = -_w2, y0 = +_h2;
	float x1 = +_w2, y1 = -_h2;

	// Remove empty areas of the texture (if we have one)
	if (texture)
	{
		float offset;
		if ((offset = texture->getLeftOffset()) > 0)
		{
			if (s0 == 0)
			{
				s0 += offset;
				x0 += offset * width;
			}
			else if (s1 == 0)
			{
				s1 += offset;
				x1 -= offset * width;
			}
		}
		if ((offset = texture->getRightOffset()) > 0)
		{
			if (s0 == 1)
			{
				s0 -= offset;
				x0 += offset * width;
			}
			else if (s1 == 1)
			{
				s1 -= offset;
				x1 -= offset * width;
			}
		}
		if ((offset = texture->getTopOffset()) > 0)
		{
			if (t0 == 0)
			{
				t0 += offset;
				y0 -= offset * height;
			}
			else if (t1 == 0)
			{
				t1 += offset;
				y1 += offset * height;
			}
		}
		if ((offset = texture->getBottomOffset()) > 0)
		{
			if (t0 == 1)
			{
				t0 -= offset;
				y0 -= offset * height;
			}
			else if (t1 == 1)
			{
				t1 -= offset;
				y1 += offset * height;
			}
		}
	}

	// Draw the quad
	glBegin(GL_QUADS);
	{
		glTexCoord2f(s0, t0);
		glVertex2f(x0, y0);
		glTexCoord2f(s1, t0);
		glVertex2f(x1, y0);
		glTexCoord2f(s1, t1);
		glVertex2f(x1, y1);
		glTexCoord2f(s0, t1);
		glVertex2f(x0, y1);
	}
	glEnd();
}

#ifdef BBGE_BUILD_PSP

// The PSP is only capable of handling polygons whose final screen
// coordinates fall in the range [-2048,+2048); if even a single vertex
// lies outside that range, the entire polygon is clipped.  That causes
// problems for us because some tiled backgrounds (normally drawn as a
// single, repeating-texture quad) extend over 10,000 virtual pixels, or
// over 5,000 native pixels -- easily enough to hit the hard-clip limit.
// To get around this, we break such repeating quads down into smaller
// pieces at the texture edges such that no piece has a displayed size
// larger than the offscreen boundary area, i.e. 2048 - 480/2 native
// pixels wide or 2048 - 272/2 high.

void Quad::renderRepeatForPSP()
{
	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	const float finalScaleX = m[0];
	const float finalScaleY = m[5];
	if (finalScaleX == 0 || finalScaleY == 0)
	{
		// Should be impossible, but avoid division by zero later.
		return renderSingle();
	}
	if (finalScaleX * width  < 2048 - 480/2
	 && finalScaleY * height < 2048 - 272/2)
	{
		// No need for the hack, so just draw it normally.
		return renderSingle();
	}

	// Get the texture coordinates, and calculate the texture's size in
	// texture units.
	float s0 = upperLeftTextureCoordinates.x;
	float s1 = lowerRightTextureCoordinates.x;
	float t0, t1;
	if (Quad::flipTY)
	{
		t0 = 1 - upperLeftTextureCoordinates.y;
		t1 = 1 - lowerRightTextureCoordinates.y;
	}
	else
	{
		t0 = upperLeftTextureCoordinates.y;
		t1 = lowerRightTextureCoordinates.y;
	}
	const float texCoordWidth  = fabsf(s1 - s0);
	const float texCoordHeight = fabsf(t1 - t0);
	if (texCoordWidth == 0 || texCoordHeight == 0)  // Avoid division by 0.
	{
		return renderSingle();
	}

	// Find the largest number of repetitions of the texture which will
	// fit within the boundary area.  (float -> int conversions truncate,
	// so this code is safe.)
	const float textureSizeX = (finalScaleX * width ) / texCoordWidth;
	const float textureSizeY = (finalScaleY * height) / texCoordHeight;
	const int maxRepetitionsX = int((2048 - 480/2) / textureSizeX);
	const int maxRepetitionsY = int((2048 - 272/2) / textureSizeY);
	const int maxRepetitions = (maxRepetitionsX < maxRepetitionsY) ? maxRepetitionsX : maxRepetitionsY;

	// Figure out how many subdivisions we need to draw in each direction.
	int numColumns, numRows;
	float ds, dt;  // Increments in each direction
	if (s1 > s0)
	{
		ds = maxRepetitions;
		numColumns = int(ceilf((s1 - floorf(s0)) / maxRepetitions));
	}
	else
	{
		ds = -maxRepetitions;
		numColumns = int(ceilf((s0 - floorf(s1)) / maxRepetitions));
	}
	if (t1 > t0)
	{
		dt = maxRepetitions;
		numRows = int(ceilf((t1 - floorf(t0)) / maxRepetitions));
	}
	else
	{
		dt = -maxRepetitions;
		numRows = int(ceilf((t0 - floorf(t1)) / maxRepetitions));
	}

	// Iterate over the texture coordinate range, drawing quads.
	const float x0 = -width/2, y0 = +height/2;
	const float recip_texCoordWidth  = 1/texCoordWidth;
	const float recip_texCoordHeight = 1/texCoordHeight;
	float t = t0, nextT;

	glBegin(GL_QUADS);
	for (int row = 0; row < numRows; row++, t = nextT)
	{
		nextT = (dt > 0 ? floorf(t) : ceilf(t)) + dt;
		if ((dt > 0 && nextT > t1) || (dt < 0 && nextT < t1))
			nextT = t1;
		const float y = y0 - (fabsf(t - t0) * recip_texCoordHeight) * height;
		const float nextY = y0 - (fabsf(nextT - t0) * recip_texCoordHeight) * height;
		float s = s0, nextS;
		for (int column = 0; column < numColumns; column++, s = nextS)
		{
			nextS = (ds > 0 ? floorf(s) : ceilf(s)) + ds;
			if ((ds > 0 && nextS > s1) || (ds < 0 && nextS < s1))
				nextS = s1;
			const float x = x0 + (fabsf(s - s0) * recip_texCoordWidth) * width;
			const float nextX = x0 + (fabsf(nextS - s0) * recip_texCoordWidth) * width;
			glTexCoord2f(s, t);
			glVertex2f(x, y);
			glTexCoord2f(nextS, t);
			glVertex2f(nextX, y);
			glTexCoord2f(nextS, nextT);
			glVertex2f(nextX, nextY);
			glTexCoord2f(s, nextT);
			glVertex2f(x, nextY);
		}
	}
	glEnd();
}

#endif  // BBGE_BUILD_PSP


Vector oldQuadColor;

void Quad::render()
{
	if (lightingColor.x != 1.0f || lightingColor.y != 1.0f || lightingColor.z != 1.0f)
	{
		oldQuadColor = color;
		color *= lightingColor;
		RenderObject::render();
		color = oldQuadColor;
	}
	else
	{
		RenderObject::render();
	}
}

void Quad::repeatTextureToFill(bool on)
{
	if (on)
	{
		repeatingTextureToFill = true;
		repeatTexture = true;
		refreshRepeatTextureToFill();
	}
	else
	{
		repeatingTextureToFill = false;
		repeatTexture = false;
		refreshRepeatTextureToFill();
	}
}

void Quad::onRender()
{
	if (!renderQuad) return;

#ifdef BBGE_BUILD_OPENGL

	_w2 = width/2;
	_h2 = height/2;

	if (!strip.empty())
	{
		//glDisable(GL_BLEND);gggg
		glDisable(GL_CULL_FACE);

		const float texBits = 1.0f / (strip.size()-1);

		glBegin(GL_QUAD_STRIP);

		if (!stripVert)
		{
			Vector pl, pr;
			for (int i = 0; i < strip.size(); i++)
			{
				//glNormal3f( 0.0f, 0.0f, 1.0f);

				if (i == strip.size()-1)
				{
				}
				else //if (i == 0)
				{
					Vector diffVec = strip[i+1] - strip[i];

					diffVec.setLength2D(_h2);
					pl = diffVec.getPerpendicularLeft();
					pr = diffVec.getPerpendicularRight();
				}

				glTexCoord2f(texBits*i, 0);
				glVertex2f(strip[i].x*width-_w2,  strip[i].y*_h2*10 - _h2);
				glTexCoord2f(texBits*i, 1);
				glVertex2f(strip[i].x*width-_w2,  strip[i].y*_h2*10 + _h2);
			}
		}
		glEnd();

		glEnable(GL_CULL_FACE);
		glBindTexture( GL_TEXTURE_2D, 0 );
		glColor4f(1,0,0,1);
		glPointSize(64);

		glBegin(GL_POINTS);
		for (int i = 0; i < strip.size(); i++)
		{
			glVertex2f((strip[i].x*width)-_w2, strip[i].y*height);
		}
		glEnd();
	}
	else
	{
		if (core->mode == Core::MODE_2D)
		{
			if (!drawGrid)
			{
#ifdef BBGE_BUILD_PSP
				if (repeatingTextureToFill)
					renderRepeatForPSP();
				else
#endif
				renderSingle();
			}
			else
			{
				renderGrid();
			}
		}
	}

	if (renderBorder)
	{
		glLineWidth(2);

		glBindTexture(GL_TEXTURE_2D, 0);

		glColor4f(renderBorderColor.x, renderBorderColor.y, renderBorderColor.z, borderAlpha*alpha.x*alphaMod);

		if (renderCenter)
		{
			glPointSize(16);
			glBegin(GL_POINTS);
				glVertex2f(0,0);
			glEnd();
		}

		glColor4f(renderBorderColor.x, renderBorderColor.y, renderBorderColor.z, 1*alpha.x*alphaMod);
		glBegin(GL_LINES);
			glVertex2f(-_w2, _h2);
			glVertex2f(_w2, _h2);
			glVertex2f(_w2, -_h2);
			glVertex2f(_w2, _h2);
			glVertex2f(-_w2, -_h2);
			glVertex2f(-_w2, _h2);
			glVertex2f(-_w2, -_h2);
			glVertex2f(_w2, -_h2);
		glEnd();
		RenderObject::lastTextureApplied = 0;
	}

#endif
#ifdef BBGE_BUILD_DIRECTX
	//core->setColor(color.x, color.y, color.z, alpha.x);
	//if (!children.empty() || useDXTransform)
	if (true)
	{
		if (this->texture)
		{
			if (upperLeftTextureCoordinates.x != 0 || upperLeftTextureCoordinates.y != 0
				|| lowerRightTextureCoordinates.x != 1 || lowerRightTextureCoordinates.y != 1)
			{
				//core->blitD3DEx(this->texture->d3dTexture, fontDrawSize/2, fontDrawSize/2, u, v-ybit, u+xbit, v+ybit-ybit);
				core->blitD3DEx(this->texture->d3dTexture, width, height, upperLeftTextureCoordinates.x, upperLeftTextureCoordinates.y, lowerRightTextureCoordinates.x, lowerRightTextureCoordinates.y);
			}
			else
				core->blitD3D(this->texture->d3dTexture, width, height);
		}
		else
		{
			core->blitD3D(0, width, height);
		}
	}
	else
	{
		if (this->texture)
			core->blitD3DPreTrans(this->texture->d3dTexture, position.x+offset.x, position.y+offset.y, width*scale.x, width.y*scale.y);
		else
			core->blitD3DPreTrans(0, position.x+offset.x, position.y+offset.y, width*scale.x, width.y*scale.y);
	}

	/*
	if (this->texture)
	{
		core->getD3DSprite()->Begin(D3DXSPRITE_ALPHABLEND);
		D3DXVECTOR2 scaling((1.0f/float(this->texture->width))*width*scale.x,
			(1.0f/float(this->texture->height))*height*scale.y);
		if (isfh())
			scaling.x = -scaling.x;
		D3DXVECTOR2 spriteCentre=D3DXVECTOR2((this->texture->width/2), (this->texture->height/2));
		///scale.x
		//D3DXVECTOR2 trans=D3DXVECTOR2(position.x, position.y);


		if (blendType == BLEND_DEFAULT)
		{
			core->getD3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			core->getD3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		}
		else
		{
			core->getD3DDevice()->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			core->getD3DDevice()->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		}

		D3DXVECTOR2 rotationCentre = spriteCentre;
		D3DXVECTOR2 trans=D3DXVECTOR2(position.x,position.y) - spriteCentre;
		if (followCamera != 1)
		{
			trans.x -= core->cameraPos.x;
			trans.y -= core->cameraPos.y;
		}
		D3DXMATRIX mat, scale, final;
		//D3DXVECTOR2 centre = trans + spriteCentre;
		float rotation = (this->rotation.z*PI)/180.0f;
		//D3DXVECTOR2 scaling((1.0f/float(this->texture->width))*width*scale.x,(1.0f/float(this->texture->height))*height*scale.y);

		//D3DXVECTOR2 scaling(1,1);
		const D3DCOLOR d3dColor=D3DCOLOR_ARGB(int(alpha.x*255), int(color.x*255), int(color.y*255), int(color.z*255));
		//const D3DCOLOR d3dColor=D3DCOLOR_ARGB(int(alpha.x*255), int(color.x*255), int(color.y*255), int(color.z*255));
		FLOAT scalingRotation = 0;
		//D3DXMatrixTransformation2D(&mat,NULL,0.0,&scaling,&spriteCentre,rotation,&trans);
		D3DXMatrixTransformation2D(&mat,
			&spriteCentre,
			scalingRotation,
			&scaling,
			&spriteCentre,
			rotation,
			&trans
		);

		if (followCamera != 1)
		{
			D3DXMatrixScaling(&scale,core->globalScale.x*core->globalResolutionScale.x,core->globalScale.y*core->globalResolutionScale.y,1);
			D3DXMatrixMultiply(&final, &mat, &scale);

			core->getD3DSprite()->SetTransform(&final);
		}
		else
		{
			D3DXMatrixScaling(&scale,core->globalResolutionScale.x,core->globalResolutionScale.y,1);
			D3DXMatrixMultiply(&final, &mat, &scale);
			core->getD3DSprite()->SetTransform(&final);
		}


		//mat = scale * mat;

		if (this->texture)
		{
			core->getD3DSprite()->Draw(this->texture->d3dTexture,NULL,NULL,NULL,d3dColor);//0xFFFFFFFF);//d3dColor);
			core->getD3DSprite()->End();
		}
		else
		{
			core->getD3DSprite()->End();
			D3DRECT rect;
			rect.x1 = trans.x - this->width/2;
			rect.x2 = trans.x + this->width/2;
			rect.y1 = trans.y - this->height/2;
			rect.y2 = trans.y + this->height/2;
			core->getD3DDevice()->Clear(1,&rect,D3DCLEAR_TARGET,d3dColor,0,0);
		}
		//core->getD3DSprite()->End();
	}
	*/

#endif
}


void Quad::flipHorizontal()
{
	RenderObject::flipHorizontal();
}

void Quad::flipVertical()
{
	if (!_fv)
	{
		lowerRightTextureCoordinates.y = 0;
		upperLeftTextureCoordinates.y = 1;
	}
	else
	{
		lowerRightTextureCoordinates.y = 1;
		upperLeftTextureCoordinates.y = 0;
	}
	RenderObject::flipVertical();
}

void Quad::calculateQuadLighting()
{
	Vector total;
	int c=0;
	for (int i = 0; i < QuadLight::quadLights.size(); i++)
	{
		QuadLight *q = &QuadLight::quadLights[i];
		Vector dist = q->position - position;
		if (dist.isLength2DIn(q->dist))
		{
			total += q->color;
			c++;
		}
	}
	if (c > 0)
		lightingColor = total/c;
}

void Quad::refreshRepeatTextureToFill()
{
	if (repeatingTextureToFill)
	{
		upperLeftTextureCoordinates.x = texOff.x;
		upperLeftTextureCoordinates.y = texOff.y;
		lowerRightTextureCoordinates.x = (width*scale.x*repeatToFillScale.x)/texture->width + texOff.x;
		lowerRightTextureCoordinates.y = (height*scale.y*repeatToFillScale.y)/texture->height + texOff.y;
	}
	else
	{
		if (fabsf(lowerRightTextureCoordinates.x) > 1 || fabsf(lowerRightTextureCoordinates.y)>1)
			lowerRightTextureCoordinates = Vector(1,1);
	}
}

void Quad::reloadDevice()
{
	RenderObject::reloadDevice();
}

void Quad::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);

	if (autoWidth == AUTO_VIRTUALWIDTH)
#ifdef BBGE_BUILD_PSP  // FIXME: Otherwise it ends up 1 pixel too narrow.  Rounding error?  --achurch
		width = core->getVirtualWidth() + 2;
#else
		width = core->getVirtualWidth();
#endif
	else if (autoWidth == AUTO_VIRTUALHEIGHT)
		width = core->getVirtualHeight();

	if (autoHeight == AUTO_VIRTUALWIDTH)
		height = core->getVirtualWidth();
	else if (autoHeight == AUTO_VIRTUALHEIGHT)
		height = core->getVirtualHeight();


	refreshRepeatTextureToFill();

	lowerRightTextureCoordinates.update(dt);
	upperLeftTextureCoordinates.update(dt);

	if (drawGrid && alpha.x > 0 && alphaMod > 0)
	{
		updateGrid(dt);
	}

	if (quadLighting)
	{
		calculateQuadLighting();
	}
}

void Quad::setWidthHeight(int w, int h)
{
	if (h == -1)
		height = w;
	else
		height = h;
	width = w;
}

void Quad::setWidth(int w)
{
	width = w;
}

void Quad::setHeight(int h)
{
	height = h;
}

void Quad::onSetTexture()
{
	if (texture)
	{
		width = this->texture->width;
		height = this->texture->height;
		_w2 = this->texture->width/2.0f;
		_h2 = this->texture->height/2.0f;
	}
}

PauseQuad::PauseQuad() : Quad(), pauseLevel(0)
{
}

void PauseQuad::onUpdate(float dt)
{
	if (core->particlesPaused <= pauseLevel)
	{
		Quad::onUpdate(dt);
	}
}

