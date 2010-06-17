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
#include "DSQ.h"

WorldMapTile::WorldMapTile()
{
	revealed = false;
	prerevealed = false;
	scale = scale2 = 1;
	layer = 0;
	index = -1;
	data = 0;
	dataSize = 0;
	vis = 0;
	visSize = 0;
	q = 0;
	stringIndex = 0;
}

void WorldMapTile::visToData()
{
	if (vis)
	{
		if (visSize % 8 != 0)
		{
			debugLog("visSize must be a multiple of 8!");
			return;
		}
		if (!data || dataSize != visSize)
		{
			delete[] data;
			dataSize = visSize;
			const unsigned int rowSize = (dataSize+7)/8;
			data = new unsigned char[rowSize * dataSize];
		}
		unsigned char *ptr = data;
		for (unsigned int y = 0; y < visSize; y++, ptr += (dataSize+7)/8)
		{
			for (unsigned int x = 0; x < visSize; x += 8)
			{
				unsigned char dataByte = 0;
				for (unsigned int x2 = 0; x2 < 8; x2++)
				{
					if (vis[x+x2][y].z > 0.5f)
						dataByte |= 1 << x2;
				}
				ptr[x/8] = dataByte;
			}
		}
	}
}

void WorldMapTile::dataToVis(float ab, float av)
{
	if (data)
	{
		if (dataSize == visSize)
		{
			unsigned char *ptr = data;
			for (unsigned int y = 0; y < dataSize; y++, ptr += (dataSize+7)/8)
			{
				for (unsigned int x = 0; x < dataSize; x += 8)
				{
					unsigned char dataByte = ptr[x/8];
					for (unsigned int x2 = 0; x2 < 8; x2++)
					{
						vis[x+x2][y].z = (dataByte & (1 << x2)) ? av : ab;
					}
				}
			}
			return;
		}
		else
		{
			std::ostringstream os;
			os << "dataSize " << dataSize << " != visSize " << visSize
			   << ", clearing data!";
			delete[] data;
			data = 0;
			dataSize = 0;
			// Fall through to vis[] clearing.
		}
	}

	/* No data, so set it to all empty */			
	for (int x = 0; x < visSize; x++)
	{
		for (int y = 0; y < visSize; y++)
		{
			vis[x][y].z = ab;
		}
	}
}

void WorldMapTile::clearData()
{
	delete[] data;
	data = 0;
	dataSize = 0;
}

// Convert the data array to string format for saving.
void WorldMapTile::dataToString(std::ostringstream &os)
{
	unsigned int count = 0;
	std::ostringstream tempStream;
	unsigned char *ptr = data;
	for (unsigned int y = 0; y < dataSize; y++, ptr += (dataSize+7)/8)
	{
		for (unsigned int x = 0; x < dataSize; x += 8)
		{
			unsigned char dataByte = ptr[x/8];
			for (unsigned int x2 = 0; x2 < 8; x2++)
			{
				if (dataByte & (1 << x2))
				{
					tempStream << (x+x2) << " " << y << " ";
					count++;
				}
			}
		}
	}

	os << dataSize << " " << count << " " << tempStream.str();
}

// Parse a string from a save file and store in the data array.
void WorldMapTile::stringToData(std::istringstream &is)
{
	delete[] data;
	data = 0;
	dataSize = 0;
	unsigned int rowSize = 0;

	int count = 0;
	is >> dataSize >> count;
	if (dataSize > 0)
	{
		rowSize = (dataSize+7)/8;
		data = new unsigned char[rowSize * dataSize];
		memset(data, 0, rowSize * dataSize);
	}

	for (int i = 0; i < count; i++)
	{
		int x, y;
		is >> x >> y;
		if (x >= 0 && x < dataSize && y >= 0 && y < dataSize)
			data[y*rowSize + x/8] |= 1 << (x%8);
	}
}


WorldMap::WorldMap()
{
	gw=gh=0;
}

void WorldMap::load(const std::string &file)
{
	worldMapTiles.clear();

	std::string line;

	std::ifstream in(file.c_str());
	
	while (std::getline(in, line))
	{
		WorldMapTile t;
		std::istringstream is(line);
		is >> t.index >> t.stringIndex >> t.name >> t.layer >> t.scale >> t.gridPos.x >> t.gridPos.y >> t.prerevealed >> t.scale2;
		t.revealed = t.prerevealed;
		stringToUpper(t.name);
		worldMapTiles.push_back(t);
	}
}

void WorldMap::save(const std::string &file)
{
	std::ofstream out(file.c_str());
	
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		WorldMapTile *t = &worldMapTiles[i];
		out << t->index << " " << t->name << " " << t->layer << " " << t->scale << " " << t->gridPos.x << " " << t->gridPos.y << " " << t->prerevealed << " " << t->scale2 << std::endl;
	}
}

void WorldMap::revealMap(const std::string &name)
{
	WorldMapTile *t = getWorldMapTile(name);
	if (t)
	{
		t->revealed = true;
	}
}

void WorldMap::revealMapIndex(int index)
{
	WorldMapTile *t = getWorldMapTileByIndex(index);
	if (t)
	{
		t->revealed = true;
	}
}

WorldMapTile *WorldMap::getWorldMapTile(const std::string &name)
{
	std::string n = name;
	stringToUpper(n);
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		if (worldMapTiles[i].name == n)
		{
			return &worldMapTiles[i];
		}
	}
	return 0;
}

WorldMapTile *WorldMap::getWorldMapTileByIndex(int index)
{
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		if (worldMapTiles[i].index == index)
		{
			return &worldMapTiles[i];
		}
	}
	return 0;
}

/*



void WorldMap::revealMapIndex(int index)
{
	if (index < 0 || index >= worldMapTiles.size()) return;

	worldMapTiles[index].revealed = true;
}
*/

void WorldMap::hideMap()
{
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		worldMapTiles[i].revealed = false;
	}
}

int WorldMap::getNumWorldMapTiles()
{
	return worldMapTiles.size();
}

WorldMapTile *WorldMap::getWorldMapTile(int index)
{
	if (index < 0 || index >= worldMapTiles.size()) return 0;

	return &worldMapTiles[index];
}

