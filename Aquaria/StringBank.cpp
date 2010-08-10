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

StringBank::StringBank()
{
}

void StringBank::load(const std::string &file)
{
    //debugLog("StringBank::load("+file+")");
	stringMap.clear();

	std::ifstream in(file.c_str());

	std::string line;
	int idx;

	while (in >> idx)
	{
		std::getline(in, line);

		//std::ostringstream os;
		//os << idx << ": StringBank Read Line: " << line;
		//debugLog(os.str());

		if (!line.empty() && line[0] == ' ')
			line = line.substr(1, line.size());
		for (int i = 0; i < line.size(); i++)
		{
			if (line[i] == '|')
				line[i] = '\n';
		}
		stringMap[idx] = line;
	}

#ifdef BBGE_BUILD_PSP
	// Override a few for the PSP.
	stringMap[0] = "Press Cross twice in a row to eat the selected food.";
	stringMap[1] = "Press Square to select ingredients for cooking, Select to combine them, or Triangle to cancel a selection. Hold Select and press Triangle to discard food into the environment.";
	stringMap[4] = "You've found a new map token!\nPress Triangle to view the world map.";
	stringMap[13] = "Press Circle to let go, or hold the analog pad in a direction and press Circle to jump.";
	stringMap[14] = "The songs that Naija has learned are displayed here. Move the cursor to a song to see how it is sung. Press Circle or Cross to hear Naija's description.";
	stringMap[15] = "Use these icons to switch between menu pages.";
	stringMap[17] = "Naija's pets are displayed here. Press Circle or Cross to select the active pet.";
	stringMap[35] = "The Shield Song - Press START for Song Menu";
#endif
}

std::string StringBank::get(int idx)
{
	return stringMap[idx];
}

