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
#include "FlockEntity.h"

const int DEFAULT_MAX_FLOCKS = 20;  // Will be increased as needed
std::vector<Flock*> flocks(DEFAULT_MAX_FLOCKS);

FlockEntity::FlockEntity() : CollideEntity()
{
	flockType = FLOCK_FISH;
	flock = 0;

	angle = 0;

	collideRadius = 8;
}

void FlockEntity::addToFlock(int id)
{
	if (id >= flocks.size())
	{
		int curSize = flocks.size();
		flocks.resize(id+1);
		for (int i = curSize; i < id+1; i++)
			flocks[i] = 0;
	}
	if (!flocks[id])
	{
		flocks[id] = new Flock(id);
	}

	flock = flocks[id];
	nextInFlock = flock->firstEntity;
	prevInFlock = 0;
	if (flock->firstEntity)
		flock->firstEntity->prevInFlock = this;
	flock->firstEntity = this;
}

void FlockEntity::removeFromFlock()
{
	if (flock)
	{
		if (nextInFlock)
			nextInFlock->prevInFlock = prevInFlock;
		if (prevInFlock)
			prevInFlock->nextInFlock = nextInFlock;
		else
			flock->firstEntity = nextInFlock;
		if (!flock->firstEntity)
		{
			flocks[flock->flockID] = 0;
			delete flock;
		}
	}
	flock = 0;
}

void FlockEntity::destroy()
{
	removeFromFlock();
	CollideEntity::destroy();
}

Vector FlockEntity::getFlockCenter()
{
	Vector position;
	int sz = 0;
	for (FlockEntity *e = flock->firstEntity; e; e = e->nextInFlock)
	{
		position += e->position;
		sz++;
	}
	position /= sz;
	return position;
}

Vector FlockEntity::getFlockHeading()
{
	Vector v;
	int sz = 0;
	for (FlockEntity *e = flock->firstEntity; e; e = e->nextInFlock)
	{
		v += e->vel;
		sz++;
	}
	v /= sz;
	return v;
}

FlockEntity *FlockEntity::getNearestFlockEntity()
{
	FlockEntity *nearest = 0;
	float smallDist = -1;
	float dist = 0;
	Vector distVec;
	for (FlockEntity *e = flock->firstEntity; e; e = e->nextInFlock)
	{
		if (e != this)
		{
			distVec = (e->position - position);
			dist = distVec.getSquaredLength2D();
			if (dist < smallDist || smallDist == -1)
			{
				smallDist = dist;
				nearest = e;
			}
		}
	}
	return nearest;
}

Vector FlockEntity::averageVectors(const VectorSet &vectors, int maxNum)
{
	Vector avg;
	int c= 0;
	for (VectorSet::const_iterator i = vectors.begin(); i != vectors.end(); i++)
	{
		if (maxNum != 0 && c >= maxNum)
			break;
		avg.x += (*i).x;
		avg.y += (*i).y;
		c++;
		//avg.z += vectors[i].z;
	}
	//int sz = vectors.size();

	if (c != 0)
	{
		avg.x /= float(c);
		avg.y /= float(c);
	}
	//avg.z /= vectors.size();
	return avg;
}
