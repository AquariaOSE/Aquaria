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
#include "../BBGE/DebugFont.h"

#include "DSQ.h"


ModSelector::ModSelector() : AquariaGuiQuad(), label(0)
{
	label = new BitmapText(&dsq->smallFont);
	//label->position = Vector(-200, 160);
	label->position = Vector(0, 160);
	label->setWidth(400);
	addChild(label, PM_POINTER);

	refreshTexture();

	mouseDown = false;
	refreshing = false;


	shareAlphaWithChildren = 1;
}

void ModSelector::refreshTexture()
{
	float t = 0.2;
	bool doit=false;
	refreshing = true;
	if (texture)
	{
		alpha.interpolateTo(0, t);
		scale.interpolateTo(Vector(0.5, 0.5), t);
		dsq->main(t);
		doit = true;
	}
	ModEntry *e = dsq->getSelectedModEntry();
	if (e)
	{
		std::string texToLoad = e->path + "/" + "mod-icon";
		texToLoad = dsq->mod.getBaseModPath() + texToLoad;
		setTexture(texToLoad);
		width = 256;
		height = 256;
	}
	else
	{
		return;
	}
	
	TiXmlDocument d;
	
	dsq->mod.loadModXML(&d, e->path);
	
	if (label)
	{
		label->setText("No Description");
		TiXmlElement *top = d.FirstChildElement("AquariaMod");
		if (top)
		{
			TiXmlElement *desc = top->FirstChildElement("Description");
			if (desc)
			{
				if (desc->Attribute("text"))
				{
					std::string txt = desc->Attribute("text");
					if (txt.size() > 255)
						txt.resize(255);
					label->setText(txt);
				}
			}
		}
	}
	if (doit)
	{
		alpha.interpolateTo(1, t);
		scale.interpolateTo(Vector(1, 1), t);
		dsq->main(t);
	}
	refreshing = false;
}

void ModSelector::onUpdate(float dt)
{
	AquariaGuiQuad::onUpdate(dt);

	if (!refreshing)
	{
		if (isCoordinateInside(core->mouse.position))
		{
			scale.interpolateTo(Vector(1.1, 1.1), 0.1);
			const bool anyButton = core->mouse.buttons.left || core->mouse.buttons.right;
			if (anyButton && !mouseDown)
			{
				mouseDown = true;
			}
			else if (!anyButton && mouseDown)
			{
				core->quitNestedMain();
				dsq->modIsSelected = true;
				dsq->sound->playSfx("click");
				dsq->sound->playSfx("pet-on");
				mouseDown = false;
			}
		}
		else
		{
			scale.interpolateTo(Vector(1, 1), 0.1);
			mouseDown = false;
		}
	}
}

