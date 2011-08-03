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
#include "DFSprite.h"
#include "MathFunctions.h"

DFSprite::DFSprite() : AnimatedSprite()
{
	//debugLog("DFSprite::DFSprite()");
	currentDatafile = 0;
	//debugLog("End DFSprite::DFSprite()");
}

/*
DFSprite::~DFSprite()
{
	destroy();
}
*/

void DFSprite::destroy()
{
	//make sure that animatedSprite::destroy() doesn't 
	// destroy our texture
	// since its just a pointer to a texture in the datafile
	texture = 0;
	for (int i = 0; i < datafiles.size(); i++)
	{
		datafiles[i]->destroy();
		delete datafiles[i];
	}
	datafiles.clear();
	AnimatedSprite::destroy();
}

void DFSprite::onUpdate(float dt)
{
	AnimatedSprite::onUpdate(dt);

	if (currentDatafile)
	{
		setTexturePointer(currentDatafile->get(frame), NO_ADD_REF);
	}
}

void DFSprite::onSetTexture()
{
	Quad::onSetTexture();
	/*
	width = texture->width;
	height = texture->height;
	*/

	//scale = Vector(texture->width, texture->height,0);
}

// returns index of datafile with the specified name
// returns -1 on failure
int DFSprite::findDatafile(const std::string &name)
{
	for (int i = 0; i < datafiles.size(); i++)
	{
		if (datafiles[i]->getName()==name)
		{
			return i;
		}
	}
	return -1;
}

// select datafile # index
void DFSprite::selectDatafile(int index)
{
	if (datafiles.empty())
		msg("Datafiles empty");
	if (index >= datafiles.size() || index <0)
	{
		std::ostringstream os;
		os << "Datafile selection index out of range [";
		os << index;
		os << "]";
		msg(os.str());
		errorLog(os.str());
		exit(0);
	}
	currentDatafile = datafiles[index];
}

int DFSprite::addDatafile()
{
	Datafile *d = new Datafile;
	datafiles.push_back(d);
	return datafiles.size()-1;
}

void DFSprite::onAnimData(AnimData &animData)
{
	selectDatafile(animData.datafile);
}

Datafile *DFSprite::getDatafile(int index)
{
	if (index < 0 || index >= datafiles.size())
		errorLog("Datafile index out of bounds");
	return datafiles[index];
}

// shorctut code
// select the datafile as specified by animData
void DFSprite::selectDatafile(AnimData &animData)
{
	selectDatafile(animData.datafile);
}

// get the datafile specified in animData
Datafile *DFSprite::getDatafile(AnimData &animData)
{
	return getDatafile(animData.datafile);
}

void DFSprite::setFrameBasedRotation(const Vector &p1, const Vector &p2)
{
	if (currentDatafile)
	{
		float angle;
		MathFunctions::calculateAngleBetweenVectorsInDegrees(p1, p2, angle);
		angle += 90;
		int bframe = frame;
		bframe = int((angle*float(currentDatafile->getSize())) / 360.0f);
		if (bframe >= currentDatafile->getSize())
			bframe = currentDatafile->getSize()-1;
		else if (bframe < 0)
			bframe = 0;
		frame = bframe;
	}
	else
	{
		msg ("No datafile selected");
	}
}

void DFSprite::selectDatafileBasedRotation(const Vector &p1, const Vector &p2, int datafileRange, int datafileIndexOffset, bool flipAngle, float angleOffset)
{
	float angle;
	MathFunctions::calculateAngleBetweenVectorsInDegrees(p1, p2, angle);
	angle += 90;
	angle += angleOffset;
	if (flipAngle)
		angle = 360 - angle;
	int bframe = frame;
	bframe = int((angle*float(datafileRange)) / 360.0f);
	if (bframe >= datafileRange)
		bframe = datafileRange-1;
	else if (bframe < 0)
		bframe = 0;
	selectDatafile(bframe+datafileIndexOffset);
}
