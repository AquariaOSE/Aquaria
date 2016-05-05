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
#ifndef __dfsprite__
#define __dfsprite__

#include "Quad.h"
#include "Datafile.h"
#include "AnimatedSprite.h"

class DFSprite : public AnimatedSprite
{
public:
	DFSprite();

	void destroy();



	int findDatafile(const std::string &name);
	void selectDatafile(int index);
	void selectDatafile(AnimData &animData);
	Datafile *getDatafile(int index);
	Datafile *getDatafile(AnimData &animData);
	int addDatafile();
	void setFrameBasedRotation(const Vector &p1, const Vector &p2);
	void selectDatafileBasedRotation(const Vector &p1, const Vector &p2, int datafileRange, int datafileIndexOffset=0, bool flipAngle=false, float angleOffset=0);
protected:
	void onAnimData(AnimData &animData);
	std::vector<Datafile*> datafiles;
	Datafile *currentDatafile;
	void onSetTexture();
	void onUpdate(float dt);
};

#endif
