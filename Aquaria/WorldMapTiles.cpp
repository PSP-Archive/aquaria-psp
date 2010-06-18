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

// Used by dataToString(), stringToData()
static const char base64Chars[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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
	if (!data)
	{
		os << "0 0";
		return;
	}

#ifdef AQUARIA_SAVE_MAPVIS_RAW

	const unsigned int rowSize = (dataSize+7)/8;
	const unsigned int totalBytes = rowSize * dataSize;
	char *outbuf = new char[(totalBytes+2)/3*4 + 1];
	char *ptr = outbuf;

	unsigned int i;
	for (i = 0; i+3 <= totalBytes; i += 3, ptr += 4)
	{
		ptr[0] = base64Chars[(               data[i+0]>>2) & 0x3F];
		ptr[1] = base64Chars[(data[i+0]<<4 | data[i+1]>>4) & 0x3F];
		ptr[2] = base64Chars[(data[i+1]<<2 | data[i+2]>>6) & 0x3F];
		ptr[3] = base64Chars[(data[i+2]<<0               ) & 0x3F];
	}
	if (i < totalBytes)
	{
		ptr[0] = base64Chars[(data[i+0]>>2) & 0x3F];
		if (i+1 < totalBytes)
		{
			ptr[1] = base64Chars[(data[i+0]<<4 | data[i+1]>>4) & 0x3F];
			ptr[2] = base64Chars[(data[i+1]<<2) & 0x3F];
		} else {
			ptr[1] = base64Chars[(data[i+0]<<4) & 0x3F];
			ptr[2] = '=';
		}
		ptr[3] = '=';
		ptr += 4;
	}
	*ptr = 0;

	os << dataSize << " b " // "b" for bitmap
	   << (dataSize==0 ? "====" : outbuf);  // Always write a non-empty string
	delete[] outbuf;

#else  // !AQUARIA_SAVE_MAPVIS_RAW

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
					tempStream << " " << (x+x2) << " " << y;
					count++;
				}
			}
		}
	}

	os << dataSize << " " << count << tempStream.str();

#endif
}

// Parse a string from a save file and store in the data array.
void WorldMapTile::stringToData(std::istringstream &is)
{
	delete[] data;
	data = 0;
	dataSize = 0;
	unsigned int rowSize = 0;

	std::string countOrType;
	is >> dataSize >> countOrType;
	if (dataSize > 0)
	{
		rowSize = (dataSize+7)/8;
		data = new unsigned char[rowSize * dataSize];
		memset(data, 0, rowSize * dataSize);
	}

	if (countOrType == "b")  // Raw bitmap (base64-encoded)
	{
		std::string encodedData = "";
		is >> encodedData;
		const char *in = encodedData.c_str();
		unsigned char *out = data;
		unsigned char * const top = data + (rowSize * dataSize);
		while (in[0] != 0 && in[1] != 0 && out < top)
		{
			unsigned char ch0, ch1, ch2, ch3;
			const char *temp;
			temp = strchr(base64Chars, in[0]);
				ch0 = temp ? temp - base64Chars : 0;
			temp = strchr(base64Chars, in[1]);
				ch1 = temp ? temp - base64Chars : 0;
			if (in[2] != 0)
			{
				temp = strchr(base64Chars, in[2]);
					ch2 = temp ? temp - base64Chars : 0;
				temp = strchr(base64Chars, in[3]);
					ch3 = temp ? temp - base64Chars : 0;
			}
			else
			{
				ch2 = ch3 = 0;
			}
			*out++ = ch0<<2 | ch1>>4;
			if (out >= top || in[2] == 0 || in[2] == '=')
				break;
			*out++ = ch1<<4 | ch2>>2;
			if (out >= top || in[3] == 0 || in[3] == '=')
				break;
			*out++ = ch2<<6 | ch3>>0;
			in += 4;
		}
	}
	else  // List of coordinate pairs
	{
		int count = 0;
		std::istringstream is2(countOrType);
		is2 >> count;

		for (int i = 0; i < count; i++)
		{
			int x, y;
			is >> x >> y;
			if (x >= 0 && x < dataSize && y >= 0 && y < dataSize)
				data[y*rowSize + x/8] |= 1 << (x%8);
		}
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

