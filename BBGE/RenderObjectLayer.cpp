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
#include "Core.h"

RenderObjectLayer::RenderObjectLayer()
{	
	followCamera = NO_FOLLOW_CAMERA;
	visible = true;
	startPass = endPass = 0;
	currentPass = 0;
	followCameraLock = FCL_NONE;
	cull = true;
	update = true;

	mode = Core::MODE_2D;

	color = Vector(1,1,1);
	
	fastCull = false;
	fastCullDist = 1024;

#ifdef RLT_FIXED
	renderObjects.clear();
	currentSize = 0;
	freeIdx = 0;
	setSize(64);
#endif
}

void RenderObjectLayer::setCull(bool v)
{
	this->cull = v;
}

void RenderObjectLayer::setSize(int sz)
{
#ifdef RLT_FIXED
	debugLog("setting fixed size");
	int oldSz = renderObjects.size();
	renderObjects.resize(sz);
	for (int i = oldSz; i < sz; i++)
	{
		renderObjects[i] = 0;
	}
#endif
#ifdef RLT_DYNAMIC
	//debugLog("Cannot set RenderObjectLayer size when RLT_DYNAMIC");
#endif
}

bool sortRenderObjectsByDepth(RenderObject *r1, RenderObject *r2)
{
	return r1->getSortDepth() < r2->getSortDepth();
}

void RenderObjectLayer::sort()
{
#ifdef RLT_FIXED
	for (int i = renderObjects.size()-1; i >= 0; i--)
	{
		bool flipped = false;
		if (!renderObjects[i]) continue;
		for (int j = 0; j < i; j++)
		{
			if (!renderObjects[j]) continue;
			if (!renderObjects[j+1]) continue;
			//position.z 
			//position.z
			//!renderObjects[j]->parent && !renderObjects[j+1]->parent && 
			if (renderObjects[j]->getSortDepth() > renderObjects[j+1]->getSortDepth())
			{
				RenderObject *temp;
				temp = renderObjects[j];
				int temp2 = renderObjects[j]->getIdx();
				renderObjects[j] = renderObjects[j+1];
				renderObjects[j+1] = temp;
					renderObjects[j]->setIdx(j);
				renderObjects[j+1]->setIdx(j+1);
					flipped = true;
			}
		}
		if (!flipped) break;
	}
#endif
#ifdef RLT_DYNAMIC
	renderObjectList.sort(sortRenderObjectsByDepth);
#endif
}

#ifdef RLT_FIXED
void RenderObjectLayer::findNextFreeIdx()
{
	int c = 0;
	int sz = renderObjects.size();
	//freeIdx++;
	while (renderObjects[freeIdx] != 0)
	{
		freeIdx ++;
		if (freeIdx >= sz)
			freeIdx = 0;
		c++;
		if (c > sz)
		{
			std::ostringstream os;
			os << "exceeded max renderobject count max[" << sz << "]";
			debugLog(os.str());
			return;
		}
	}
	if (freeIdx > currentSize)
	{
		currentSize = freeIdx+1;
		if (currentSize > sz)
		{
			currentSize = sz;
		}
		/*
		std::ostringstream os;
		os << "CurrentSize: " << currentSize;
		debugLog(os.str());
		*/
	}
	/*
	std::ostringstream os;
	os << "layer: " << index << " freeIdx: " << freeIdx;
	debugLog(os.str());
	*/
}
#endif  // RLT_FIXED

void RenderObjectLayer::add(RenderObject* r)
{
#ifdef RLT_FIXED
	renderObjects[freeIdx] = r;
	r->setIdx(freeIdx);
	
	findNextFreeIdx();

	//renderObjects[freeIdx] = r;
#endif
#ifdef RLT_DYNAMIC
	renderObjectList.push_back(r);
#endif
#ifdef RLT_MAP
	renderObjectMap[intptr_t(r)] = r;
#endif
}

void RenderObjectLayer::remove(RenderObject* r)
{
#ifdef RLT_FIXED
	if (r->getIdx() <= -1 || r->getIdx() >= renderObjects.size())
		errorLog("trying to remove renderobject with invalid idx");
	renderObjects[r->getIdx()] = 0;
	if (r->getIdx() < freeIdx)
		freeIdx = r->getIdx();

	int c = currentSize; 
	while (renderObjects[c] == 0 && c >= 0)
	{			
		c--;
	}
	currentSize = c+1;
	if (currentSize > renderObjects.size())
		currentSize = renderObjects.size();

	/*
	std::ostringstream os;
	os << "CurrentSize: " << currentSize;
	debugLog(os.str());
	*/
#endif
#ifdef RLT_DYNAMIC
	renderObjectList.remove(r);
#endif
#ifdef RLT_MAP
	renderObjectMap[intptr_t(r)] = 0;
#endif
}

void RenderObjectLayer::moveToFront(RenderObject *r)
{
#ifdef RLT_FIXED
	int idx = r->getIdx();
	int last = renderObjects.size()-1;
	int i = 0;
	for (i = renderObjects.size()-1; i >=0; i--)
	{
		if (renderObjects[i] == 0)
			last = i;
		else
			break; 
	}
	if (idx == last) return;
	for (i = idx; i < last; i++)
	{
		renderObjects[i] = renderObjects[i+1];
		if (renderObjects[i])
			renderObjects[i]->setIdx(i);
	}
	renderObjects[last] = r;
	r->setIdx(last);

	findNextFreeIdx();
#endif
#ifdef RLT_DYNAMIC
	renderObjectList.remove(r);
	renderObjectList.push_back(r);
#endif
}

void RenderObjectLayer::moveToBack(RenderObject *r)
{
#ifdef RLT_FIXED
	int idx = r->getIdx();
	if (idx == 0) return;
	for (int i = idx; i >= 1; i--)
	{
		renderObjects[i] = renderObjects[i-1];
		if (renderObjects[i])
			renderObjects[i]->setIdx(i);
	}
	renderObjects[0] = r;
	r->setIdx(0);

	findNextFreeIdx();
#endif
#ifdef RLT_DYNAMIC
        renderObjectList.remove(r);
	renderObjectList.push_front(r);
#endif
}
