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
#include "AquariaMenuItem.h"
#include "DSQ.h"
#include "Game.h"

bool AquariaSaveSlot::closed = false;

AquariaSaveSlot::AquariaSaveSlot(int slot) : AquariaGuiQuad()
{
	done = false;
	alpha = 0;
	renderQuad = false;
	alpha.interpolateTo(1, 0.5);
	mbDown = false;
	slotIndex = slot;
	empty = true;
	box = new Quad("HintBox", Vector(0,0,0));
	box->setWidthHeight(450, 96);
	box->alphaMod = 0;
	addChild(box, PM_POINTER);
	//setTexture("dialogue-bg");
	//setTexture("HintBox");
	//renderQuad = false;
	

	//shareAlphaWithChildren = 1;

	text1 = new BitmapText(&dsq->smallFont);
	text1->setFontSize(14);

	glowText = new BitmapText(&dsq->smallFont);
	glowText->alpha = 0;
	glowText->setBlendType(BLEND_ADD);
	glowText->setFontSize(14);

	text1->setAlign(ALIGN_LEFT);
	glowText->setAlign(ALIGN_LEFT);

	glowText->position = text1->position = Vector(-175, -25);


	/*
	TiXmlDocument doc;
	doc.LoadFile(dsq->continuity.getSaveFileName(slot, "xml"));
	*/


	bool tmp=false;

	// FIXME: Loading code here is duplicated from Continuity.cpp.
	// Ideally, we should coalesce them into a single routine.  --achurch

	TiXmlDocument doc;

#ifdef BBGE_BUILD_PSP

	const uint32_t size = 100000;  // Waaay more than enough.  Hopefully.
	char *buffer = new char[size];
	PSPTexture *texture;
	if (savefile_load(slot+1, buffer, size-1, &texture))
	{
		int32_t bytesRead;
		while (!savefile_status(&bytesRead)) {
			sys_time_delay(0.01);
		}
		if (bytesRead > 0)
		{
			buffer[bytesRead] = 0;
			doc.Parse(buffer);
		}
	}
	delete[] buffer;

#else  // !BBGE_BUILD_PSP

	std::string teh_file = dsq->continuity.getSaveFileName(slot, "aqs");

	if (!exists(teh_file, false))
	{
		teh_file = dsq->continuity.getSaveFileName(slot, "sav");

		if (!exists(teh_file, false))
		{
			teh_file = dsq->continuity.getSaveFileName(slot, "xml");
		}
		else
		{
			uncrunchFile(teh_file, dsq->getSaveDirectory() + "/poot2.tmp");
			unpackFile(dsq->getSaveDirectory() + "/poot2.tmp", dsq->getSaveDirectory() + "/poot.tmp");
			remove((dsq->getSaveDirectory() + "/poot2.tmp").c_str());

			teh_file = dsq->getSaveDirectory() + "/poot.tmp";
			tmp = true;
		}
	}
	else
	{
		unpackFile(teh_file, dsq->getSaveDirectory() + "/poot.tmp");

		teh_file = dsq->getSaveDirectory() + "/poot.tmp";
		tmp = true;
	}

	doc.LoadFile(teh_file);

	if (tmp)
	{
		remove(teh_file.c_str());
	}

#endif

	std::string description = getSaveDescription(doc);
	if (description.length() > 0)
	{
		std::ostringstream os;
		os << "Slot ";
		if (dsq->isDeveloperKeys())
			os << slot;
		else
			os << (slot+1);
		os << " - " << description;
		text1->setText(os.str());
		glowText->setText(os.str());

		empty = false;
	}
	else
	{
		std::ostringstream os;
		os << "Slot " << (slot+1) << " - Empty";
		text1->setText(os.str());
		glowText->setText(os.str());
		empty = true;
	}
	text1->setWidth(400);
	glowText->setWidth(400);
	addChild(text1, PM_POINTER);
	addChild(glowText, PM_POINTER);


	screen = new Quad;

#ifdef BBGE_BUILD_PSP

	if (texture)
	{
		// FIXME: This is horribly ugly, but I'm not sure there's
		// any good way to accomplish this within the current
		// engine's framework...  --achurch
		Texture *t = new Texture;
		std::ostringstream os;
		os << "__save" << slot;
		t->name = os.str();
		t->width = 144;
		t->height = 80;
		glGenTextures(1, &t->textures[0]);
		glBindTexture(GL_TEXTURE_2D, t->textures[0]);
		fakeglTexImagePSP(GL_TEXTURE_2D, texture);
		t->addRef();
		core->addResource(t);
		screen->setTexturePointer(t, NO_ADD_REF);
	}
	else
	{
		screen->setTexture("gui/savescreendefault");
	}

#else  // !BBGE_BUILD_PSP

	if (dsq->user.video.saveSlotScreens)
	{
		std::ostringstream os,os2;
		std::string tex, tex2;
		std::string pre="./";
#ifndef BBGE_BUILD_WINDOWS
		pre = "";
#endif
		os << pre << dsq->getSaveDirectory() << "/screen-" << numToZeroString(slot, 4) << ".tga";
		os2 << pre << dsq->getSaveDirectory() <<  "/screen-" << numToZeroString(slot, 4) << ".zga";
		tex = os.str();
		tex2 = os2.str();

		if (exists(tex2))
			screen->setTexture(tex2);
		else
			screen->setTexture(tex);
	}
	else
	{
		screen->setTexture("gui/savescreendefault");
	}

#endif  // BBGE_BUILD_PSP

	if (empty)
	{
		screen->alphaMod = 0;
	}
	else
		screen->alphaMod = 1;

	core->resetTimer();
#ifdef BBGE_BUILD_PSP
	const float cut = (((144.0f/80.0f) - (4.0f/3.0f)) / 2) / (144.0f/80.0f);
	screen->upperLeftTextureCoordinates  = Vector(0+cut, 0);
	screen->lowerRightTextureCoordinates = Vector(1-cut, 1);
#else
	screen->upperLeftTextureCoordinates  = Vector(0, 1);
	screen->lowerRightTextureCoordinates = Vector(1, 0.25);
#endif
	//screen->scale = Vector(0.4, 0.3);

	if (screen->getWidth() == 0)
		screen->color = 0;
	screen->setWidthHeight(0.4f*256, 0.3f*256);
	screen->scale = Vector(0.93,0.93);
	screen->position = Vector(-250, 0) + Vector(-1.5, -1.6);
	addChild(screen, PM_POINTER);

	closed = false;	

	selected = false;
}

bool AquariaSaveSlot::isGuiVisible()
{
	return alpha.x > 0 && alphaMod > 0;
}

void AquariaSaveSlot::hide()
{
	text1->alpha.interpolateTo(0, 0.5);
	glowText->alpha.interpolateTo(0, 0.5);
	box->alpha.interpolateTo(0, 0.5);
	if (!selected)
	{		
		screen->alpha.interpolateTo(0, 0.5);
	}
}

void AquariaSaveSlot::close(bool trans)
{
	done = true;
	
	if (trans)
	{
		screen->alpha.interpolateTo(0, 0.1);
		text1->alpha.interpolateTo(0, 0.1);
	
		setLife(1);
		setDecayRate(10);
		fadeAlphaWithLife = 1;
	}
	else
	{
		setLife(1);
		setDecayRate(2);
		fadeAlphaWithLife = 1;
	}
	//shareAlphaWithChildren = 1;
}

void AquariaSaveSlot::transition()
{
	if (selected)
	{
		screen->alpha.interpolateTo(0, 1.0);
#ifdef BBGE_BUILD_PSP 
		// Interpolate the coordinates to fill the entire (wide) screen.
		screen->scale.interpolateTo(Vector(core->getVirtualWidth()/float(screen->getWidth()), core->getVirtualHeight()/float(screen->getHeight())), 1.0);
		// Also interpolate the texture coordinates so the screenshot
		// doesn't get stretched.
		const float cut = (((144.0f/80.0f) - (480.0f/272.0f)) / 2) / (144.0f/80.0f);
		screen->upperLeftTextureCoordinates.interpolateTo(Vector(0+cut, 0), 1.0);
		screen->lowerRightTextureCoordinates.interpolateTo(Vector(1-cut, 1), 1.0);
#else  // !PSP
		screen->scale.interpolateTo(Vector(800/float(screen->getWidth()), 600/float(screen->getHeight())), 1.0);
#endif
	}
}

void AquariaSaveSlot::onUpdate(float dt)
{
	AquariaGuiQuad::updateMovement(dt);

	if (!(text1->alpha.isInterpolating() || text1->alpha == 0))
	{
		text1->alpha = this->alpha;
		box->alpha = this->alpha;
	}
	if (!(screen->alpha.isInterpolating() || screen->alpha == 0))
		screen->alpha = this->alpha;
	Quad::onUpdate(dt);

	if (done || closed) return;
	if (alpha.x == 1)
	{
		if (core->getNestedMains() < 3)
		{
			if (core->mouse.position.x < position.x + 150 && core->mouse.position.x > position.x - 300
				&& core->mouse.position.y < position.y+32 && core->mouse.position.y > position.y-32)
			{
				//setBlendType(BLEND_ADD);
				glowText->alpha.interpolateTo(0.5, 0.2);
				screen->color.interpolateTo(Vector(1,1,1), 0.1);
				//screen->scale.interpolateTo(Vector(1.2, 1.2), 0.2);
				if ((core->mouse.buttons.left || core->mouse.buttons.right) && !mbDown)
				{
					mbDown = true;
				}
				else if ((!core->mouse.buttons.left && !core->mouse.buttons.right) && mbDown)
				{
					mbDown = false;
					if (text1->getText().find("Empty")!=std::string::npos && dsq->saveSlotMode == SSM_LOAD)
					{
					
					}
					else
					{
						selected = true;
						// pick this file
						dsq->playMenuSelectSfx();					

						closed = true;
						if (dsq->saveSlotMode == SSM_LOAD)
						{
							//dsq->clearSaveSlots();
							dsq->hideSaveSlots();
							this->moveToFront();
							//screen->enableMotionBlur(10, 5);
							screen->position.interpolateTo(Vector(400-position.x, 300-position.y), 1.0, 0, 0, 1);
							dsq->tfader->alpha.interpolateTo(1, 1);
							dsq->toggleCursor(false);
							core->main(1);
							
							//core->main(2);						
						}

						bool didIt = dsq->onPickedSaveSlot(this);

						if (didIt)
						{
							done = true;
							
							//alpha = 0.9;
							//setBlendType(BLEND_DEFAULT);
							//glowText->alpha.interpolateTo(0, 0.2);
							return;
						}
						else
						{
							closed = false;
							selected = false;
						}
					}
				}
			}
			else
			{
				glowText->alpha.interpolateTo(0, 0.2);
				//screen->scale.interpolateTo(Vector(1, 1), 0.2);
				screen->color.interpolateTo(Vector(0.7, 0.7, 1), 0.3);
			}
			if ((core->mouse.buttons.left || core->mouse.buttons.right) && !mbDown)
			{
				mbDown = true;
			}
			else if ((!core->mouse.buttons.left && !core->mouse.buttons.right) && mbDown)
			{
				mbDown = false;
			}
		}
	}
	else
	{
		glowText->alpha.interpolateTo(0, 0.2);
	}
}


std::string AquariaSaveSlot::getSaveDescription(const TiXmlDocument &doc)
{
	const TiXmlElement *startData = doc.FirstChildElement("StartData");
	if (!startData)
		return "";

	int hours, minutes, seconds;
	hours = minutes = seconds = 0;

	int exp = 0, money = 0, time = 0;
	if (startData->Attribute("exp"))
		exp = atoi(startData->Attribute("exp"));
	if (startData->Attribute("money"))
		money = atoi(startData->Attribute("money"));
	if (startData->Attribute("seconds"))
	{
		std::istringstream is(startData->Attribute("seconds"));
		is >> time;
	}

	float s = dsq->continuity.seconds;
	dsq->continuity.seconds = time;
	dsq->continuity.getHoursMinutesSeconds(&hours, &minutes, &seconds);
	
	/*
	std::ostringstream os;
	os << "Slot: " << slot << " - " << startData->Attribute("scene") << " - exp: " << exp << " - wealth: " << money;
	os << " Time: " << hours << ": " << minutes << ": " << seconds << " T: " << time;
	*/
	std::string location = startData->Attribute("scene");
	stringToLower(location);
	if (location.find("boilerroom")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1000);
	}
	else if (location.find("seahorse")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1028);
	}
	else if (location.find("whale")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1001);
	}
	else if (location.find("frozenveil")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1002);
	}
	else if (location.find("bubblecave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1003);
	}
	else if (location.find("energytemple")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1004);
	}
	else if (location.find("trainingcave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1023);
	}
	else if (location.find("vedhacave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1005);
	}
	else if (location.find("naijacave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1006);
	}
	else if (location.find("songcave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1007);
	}
	else if (location.find("mainarea")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1008);
	}
	else if (location.find("openwater")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1009);
	}
	else if (location.find("forest")!=std::string::npos
		|| location.find("tree")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1010);
	}
	else if (location.find("mithalas")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1011);
	}
	else if (location.find("cathedral")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1012);
	}
	else if (location.find("suntemple")!=std::string::npos || location.find("sunworm")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1013);
	}
	else if (location.find("veil")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1014);
	}
	else if (location.find("abyss")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1015);
	}
	else if (location.find("sunkencity")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1016);
	}
	else if (location.find("fishcave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1017);
	}
	else if (location.find("octocave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1018);
	}
	else if (location.find("icecave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1019);
	}
	else if (location.find("secret")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1020);
	}
	else if (location.find("final")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1021);
	}
	else if (location.find("licave")!=std::string::npos)
	{
		location = dsq->continuity.stringBank.get(1029);
	}

	std::string showLoc;
	if (dsq->isDeveloperKeys())
	{
		showLoc = " (" + std::string(startData->Attribute("scene")) + ")";
	}
	std::ostringstream os;
	os << location << std::endl;
	os << hours << ":" << numToZeroString(minutes, 2) << showLoc;
	// << ": " << seconds;
	//" T: " << time;

	dsq->continuity.seconds = s;

	return os.str();
}
